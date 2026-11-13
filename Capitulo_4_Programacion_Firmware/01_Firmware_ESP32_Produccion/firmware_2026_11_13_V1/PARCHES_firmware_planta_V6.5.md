# Parches propuestos — Firmware Planta UF (V6.4 → V6.5)

> **Estado: NO COMPILADO, NO PROBADO EN HARDWARE.** Son parches puntuales sobre los archivos que
> subieron. Cada uno indica archivo, motivo y prioridad. Las llamadas a la API de Arduino-ESP32 /
> ESP-IDF están escritas de memoria: **verifiquen la firma exacta contra la versión de core que tengan
> instalada** antes de compilar (sobre todo `esp_task_wdt_*`, `ledcAttach` y `ledcChangeFrequency`).
>
> Prioridades: **P0** seguridad / integridad de datos · **P1** robustez y memoria · **P2** limpieza.

Cifras citadas (calculadas sobre el código subido, no medidas en el equipo):

| Dato | Valor |
|---|---|
| `sizeof(RegistroEnsayos::Fila)` | 216 B × 150 filas = 31,6 KB contiguos |
| `sizeof(RegistroCalibracion)` | 116 B × 300 registros = 34,0 KB contiguos (el comentario del .ino dice 74,4 KB: está desactualizado) |
| JSON de `/status` (sin lista de ensayos) | ≈1053 B típico, ≈1131 B peor caso, de un buffer de 1200 B (estimación por reconstrucción del formato) |
| Salto de rampa si el loop se bloquea `dt` s | 960 Hz por cada segundo de bloqueo (18 RPM/s × 3200 pulsos/rev) |
| DS18B20 nativo | ≈5,2 ms bloqueantes por segundo (suma de los `delayMicroseconds`) |

---

## P0-1 · Driver deshabilitado desde el primer instante  (`EN_USO_firmware_planta.ino`)

Hoy `ENA` recién se pone en HIGH dentro de `bomba.begin()`, después de `Serial.begin`, `delay(500)`,
reserva de heap, DS18B20 y `begin()` de sensores. Mientras tanto el pin flota y (según su propia
convención: HIGH = driver deshabilitado) el driver queda habilitado y con corriente en bobinas.

```cpp
void setup() {
  // PRIMERAS líneas del programa, antes de Serial/delay/WiFi:
  pinMode(PIN_ENA, OUTPUT); digitalWrite(PIN_ENA, HIGH); // driver deshabilitado
  pinMode(PIN_PUL, OUTPUT); digitalWrite(PIN_PUL, LOW);  // sin pulsos
  pinMode(PIN_DIR, OUTPUT); digitalWrite(PIN_DIR, LOW);

  Serial.begin(115200);
  delay(500);
  // ... resto igual
```

Ventana de arranque (reset/brown-out) que este parche no cubre: evaluar un pull-up externo en ENA
para que "reset = driver deshabilitado". Verifiquen en el manual del DM860 la corriente de entrada de
ENA a 3,3 V antes de elegir la resistencia.

## P0-2 · Limitar `dt` de la rampa  (`Bomba.cpp`, `config.h`)

`loop()` calcula `dt = (tAhora - tLoop)/1000` sin tope. Si `server.handleClient()` se bloquea (cliente
lento, descarga de CSV), el siguiente `tick()` suma `18 RPM/s × dt` de golpe: con 2 s de bloqueo son
36 RPM = 1920 Hz en un solo escalón sobre un NEMA 34 con carga. Es exactamente el tirón que quieren evitar.

```cpp
// config.h
constexpr float DT_MAX_RAMPA_S = 0.10f;

// Bomba.cpp
void Bomba::tick(float dt) {
  if (!(dt > 0.0f)) return;
  if (dt > DT_MAX_RAMPA_S) dt = DT_MAX_RAMPA_S;   // una rampa nunca avanza más de 0,1 s por llamada
  // ... resto igual
```

## P0-3 · Tiempo de establecimiento tras habilitar el driver + chequeo de `ledcAttach`  (`Bomba.h/.cpp`, `config.h`)

`arrancar()` pone ENA en LOW y el primer `tick()` (≤50 ms después) ya emite pulsos a 320 Hz. Además
`ledcAttach()` devuelve `bool` y se ignora: si falla, `_fActual` queda seteado y el sistema "gira"
sin pulsos y sin alarma.

```cpp
// config.h  (valor orientativo: confirmar el mínimo en el datasheet del DM860)
constexpr uint32_t TIEMPO_ENA_MS = 100;

// Bomba.h  (privado)
uint32_t _tEnable_ms = 0;

// Bomba.cpp — arrancar(), justo después de digitalWrite(PIN_ENA, LOW):
_tEnable_ms = millis();

// Bomba.cpp — tick(), antes de calcular la rampa:
if (_enMarcha && (millis() - _tEnable_ms) < TIEMPO_ENA_MS) return; // driver aún energizando

// Bomba.cpp — rama core v3.x de generación de pulsos:
if (_fActual == 0) {
  if (!ledcAttach(PIN_PUL, f, 10)) {   // fallo de hardware PWM: no seguir "en marcha" sin pulsos
    paradaEmergencia();
    return;
  }
  _fActual = f;
  ledcWrite(PIN_PUL, 512);
}
```

## P0-4 · `setK()` borra la marca de calibración documentada  (`caudalimetro.h/.cpp`, `.ino`)

`setK()` hace `_cfg.calibracionDocumentada = false`. `cargarParametrosNVS()` lo llama en cada arranque,
después de `begin()`; y `declararCalibrado()` devuelve `false` si `_iniciado` (y nadie lo invoca).
Resultado: el bit `CAL_DOCUMENTADA` **nunca** llega a las muestras ni al CSV de metrología, aunque K
sea exactamente la calibración del Ensayo 3.

```cpp
// caudalimetro.h
bool setK(double nuevoK, bool calibracionDocumentada = false);

// caudalimetro.cpp
bool Caudalimetro::setK(double nuevoK, bool documentada) {
  // ... validación y sección crítica iguales ...
  _cfg.k_Hz_por_Lmin = nuevoK;
  _cfg.calibracionDocumentada = documentada;
  // ...
}

// .ino — cargarParametrosNVS()
sensorAlimentacion.setK(ka, fabsf(ka - K_ALIMENTACION) < 0.005f);
sensorPermeado.setK(kp, false);   // el permeado no tiene calibración propia documentada

// .ino — handleResetDev()
sensorAlimentacion.setK(K_ALIMENTACION, true);
```

## P0-5 · Presiones por defecto presentadas como medidas  (`.ino`)

`p1=0.30`, `p2=0.20`, `tmp=0.25 bar` son valores por defecto; el Darcy y el CSV los usan igual que si
alguien hubiera leído el manómetro. Un `R_torta` calculado con TMP ficticia queda en el CSV sin marca.

```cpp
bool presionIngresada = false;   // global

// handleSetPresion(): al aceptar cualquier de p1/p2/p3 → presionIngresada = true;

// loop():
resultadoDarcy = presionIngresada
  ? modeloDarcy.calcular(qPermEfectivo_mLmin, tmp_efectiva_bar, temp_agua_C)
  : ResultadoDarcy{};

// guardarMuestraDatalogger():
reg.p1_bar  = presionIngresada ? p1_manometro_bar : NAN;
reg.p2_bar  = presionIngresada ? p2_manometro_bar : NAN;
reg.tmp_bar = presionIngresada ? tmp_efectiva_bar : NAN;
```
`snprintf("%.3f", NAN)` imprime `nan` en el CSV (consistente con el CSV de metrología). Ojo: `NAN` **no** es
JSON válido; en `/status` usen `null` si el front-end va a mostrar esos campos.

## P0-6 · Ventana de caudal: no forzar 1 s cuando el loop se atrasó  (`caudalimetro.cpp`)

```cpp
// ANTES
if (dtVentana_us < 500000LL || dtVentana_us > 2500000LL) dtVentana_us = 1000000LL;
// DESPUÉS
if (dtVentana_us < 200000LL) dtVentana_us = 1000000LL;  // primera captura / glitch de reloj
// los dt largos se usan tal cual: conteo / dt real
```
Con el clamp actual, un bloqueo de 5 s da `fConteo` 5× alto; la comprobación de cordura
`f < fConteo*0.65 - 1.5` descarta el estimador correcto (el de ráfaga) y se queda con el equivocado.
Verifiquen que el filtro mediana-3/EMA tolera el escalón en la primera muestra tras un bloqueo.

## P0-7 · `/status` al límite del buffer, con truncado silencioso  (`.ino`)

Si el JSON supera `buf[1200]` el código recorta `n` y envía igual: el navegador recibe JSON inválido y el
SCADA se congela sin mensaje (el front-end hace `.catch(() => {})`). El peor caso estimado deja ~70 B de
margen, ~20 B si `autoCalMensaje` tiene texto.

```cpp
char buf[1536];
// ... snprintf igual ...
if (n < 0 || n >= (int)sizeof(buf)) {
  Serial.println("[STATUS] JSON truncado: aumentar buf[]");
  server.send(500, "application/json", "{\"error\":\"status_truncado\"}");
  return;
}
```

---

## P1-1 · Control de la bomba en su propia tarea FreeRTOS, con cola de comandos

El `.ino` usa `WebServer` **síncrono** (no ESPAsyncWebServer) y todo corre en `loop()`. Hoy los
"race conditions" son pocos porque hay una sola tarea; pero el lazo de la bomba comparte el tiempo con
HTTP (bloqueos → P0-2) y, si migran a ESPAsyncWebServer, los handlers pasan a ejecutarse en otra tarea y
**todas** las globales (`usarPermManual`, `autoCalibrando`, `String autoCalMensaje`, `bomba.*`) pasan a
ser carreras reales. El patrón de abajo sirve para ambos casos.

```cpp
#include "esp_task_wdt.h"

enum class CmdTipo : uint8_t { START, STOP, EMERGENCY, REARM, DIR, SET_RPM };
struct CmdMsg { CmdTipo tipo; float valor; };

struct EstadoBomba {
  float rpm, objetivo;
  bool  on, inv, horario, regimen, emergencia;
};

static QueueHandle_t     qCmd       = nullptr;
static SemaphoreHandle_t mtxEstado  = nullptr;
static EstadoBomba       estadoBomba{};          // copia publicada por la tarea de control

static void tareaControl(void*) {
  esp_task_wdt_add(NULL);                        // verificar API del core instalado
  TickType_t t0 = xTaskGetTickCount();
  uint32_t tPrev = millis();
  for (;;) {
    CmdMsg m;
    while (xQueueReceive(qCmd, &m, 0) == pdTRUE) {
      switch (m.tipo) {
        case CmdTipo::START:     bomba.arrancar();          break;
        case CmdTipo::STOP:      bomba.detener();           break;
        case CmdTipo::EMERGENCY: bomba.paradaEmergencia();  break;
        case CmdTipo::REARM:     bomba.rearmarEmergencia(); break;
        case CmdTipo::DIR:       bomba.toggleSentido();     break;
        case CmdTipo::SET_RPM:   bomba.setRPM(m.valor);     break;
      }
    }
    uint32_t t = millis();
    bomba.tick((t - tPrev) / 1000.0f);           // dt limitado dentro de tick() (P0-2)
    tPrev = t;

    if (xSemaphoreTake(mtxEstado, pdMS_TO_TICKS(2)) == pdTRUE) {
      estadoBomba = { bomba.rpmActual(), bomba.rpmObjetivo(), bomba.enMarcha(), bomba.invirtiendo(),
                      bomba.sentidoHorario(), bomba.enRegimenEstable(), bomba.enEmergencia() };
      xSemaphoreGive(mtxEstado);
    }
    esp_task_wdt_reset();
    vTaskDelayUntil(&t0, pdMS_TO_TICKS(50));
  }
}

// setup(), después de bomba.begin():
qCmd      = xQueueCreate(8, sizeof(CmdMsg));
mtxEstado = xSemaphoreCreateMutex();
xTaskCreatePinnedToCore(tareaControl, "ctl_bomba", 4096, nullptr, 3, nullptr, 1);

// handleCmd(): en vez de llamar a bomba.* directamente
CmdMsg m{CmdTipo::START, 0};          // ... según act
xQueueSend(qCmd, &m, 0);
// EMERGENCY: xQueueSendToFront(qCmd, &m, 0);
```

Quitar `bomba.tick()` y la lógica de transición de ensayo (`iniciarNuevoEnsayo`/`finalizarEnsayoActual`)
del bloque de 50 ms de `loop()`, o dejar esa lógica en `loop()` leyendo `estadoBomba` (copia protegida).
`handleStatus()` también debe leer `estadoBomba` y no `bomba.*`.

**Límite honesto de este diseño:** el paro de emergencia web sigue dependiendo de WiFi + HTTP + una tarea
(latencia ≤50 ms en el mejor caso). No es una función de seguridad. Recomendación: pulsador físico de
emergencia que corte la alimentación del driver (o ENA) **por hardware**, independiente del ESP32.

## P1-2 · DS18B20: CRC y protección de los slots  (`.ino`, namespace `DS18B20_Native`)

El comentario dice "no bloqueante", pero son ≈5,2 ms de `delayMicroseconds` por segundo con `pinMode()`
conmutando en cada bit. Además no se verifica CRC: las ISR de los Hall (alimentación hasta ~400 Hz de
pulsos aceptados) pueden alargar un slot de lectura (la muestra se toma a ~13 µs de una ventana de 15 µs)
y corromper bits. Estimación mía: del orden de un par de % de lecturas afectadas; **no lo he medido**,
confírmenlo con analizador lógico o contando fallos de CRC durante una hora.

```cpp
namespace DS18B20_Native {
  static portMUX_TYPE owMux = portMUX_INITIALIZER_UNLOCKED;

  static uint8_t crc8(const uint8_t* d, uint8_t n) {       // CRC-8 Dallas/Maxim
    uint8_t crc = 0;
    while (n--) {
      uint8_t b = *d++;
      for (uint8_t i = 8; i; i--) {
        uint8_t mix = (crc ^ b) & 1;
        crc >>= 1;
        if (mix) crc ^= 0x8C;
        b >>= 1;
      }
    }
    return crc;
  }

  // Envolver SOLO cada slot de bit (≤70 µs), no el reset (960 µs):
  static void writeBit(uint8_t bit) {
    portENTER_CRITICAL(&owMux);
    /* ...cuerpo actual... */
    portEXIT_CRITICAL(&owMux);
  }
  static uint8_t readBit() {
    portENTER_CRITICAL(&owMux);
    /* ...cuerpo actual... */
    portEXIT_CRITICAL(&owMux);
    return r;
  }

  static bool leerTemperatura(float& tempOut) {
    if (!reset()) return false;
    writeByte(0xCC);
    writeByte(0xBE);
    uint8_t sp[9];
    for (uint8_t i = 0; i < 9; i++) sp[i] = readByte();
    if (crc8(sp, 8) != sp[8]) return false;                // lectura corrupta → se descarta
    int16_t raw = (int16_t)((sp[1] << 8) | sp[0]);
    float t = raw / 16.0f;
    if (t < 1.0f || t > 65.0f || fabsf(t - 85.0f) < 0.01f) return false;
    tempOut = t;
    return true;
  }
}
```
Leer 9 bytes sube el bloqueo a ~9 ms/s: conviene correr esto en una tarea propia de baja prioridad
(1 Hz) en lugar de `loop()`. Si `pinMode()` agrega demasiado retardo a los slots, pasar a
`gpio_set_direction(..., GPIO_MODE_INPUT_OUTPUT_OD)` + `gpio_set_level()` (ESP-IDF) — verificar con analizador.

## P1-3 · Strings → `snprintf` (fragmentación de heap)  (`.ino`)

Con ~66 KB en dos bloques contiguos y WiFi activo, cada `String` temporal es evitable.

```cpp
// handleSetPresion(): reemplaza las 9 concatenaciones de String
char json[288];
snprintf(json, sizeof(json),
  "{\"status\":\"ok\",\"p1\":%.3f,\"p2\":%.3f,\"p3\":%.3f,\"tmp\":%.3f,\"temp\":%.1f,"
  "\"j20\":%.2f,\"tcf\":%.3f,\"r_total\":%.3e,\"r_torta\":%.3e,\"darcy_ok\":%s}",
  p1_manometro_bar, p2_manometro_bar, p3_manometro_bar, tmp_efectiva_bar, temp_agua_C,
  resultadoDarcy.valido ? resultadoDarcy.J20_LMH : jLMH_actual,
  resultadoDarcy.valido ? resultadoDarcy.TCF : 1.0f,
  resultadoDarcy.valido ? resultadoDarcy.R_total : 0.0f,
  resultadoDarcy.valido ? resultadoDarcy.R_adicional : 0.0f,
  resultadoDarcy.valido ? "true" : "false");
server.send(200, "application/json", json);

// IP del AP: una vez en setup(), no un String por cada poll de /status
static char ipStr[16];
// setup(): strlcpy(ipStr, WiFi.softAPIP().toString().c_str(), sizeof(ipStr));
// handleStatus(): usar ipStr en lugar de WiFi.softAPIP().toString().c_str()

// autoCalMensaje: char autoCalMensaje[64] en lugar de String
```
`handleCalibrarRpmQ()` tiene el mismo patrón de `String json`.

## P1-4 · CSV por bloques de ~1,4 KB en vez de una fila por chunk  (`.ino`)

Con `CONTENT_LENGTH_UNKNOWN` cada `sendContent()` es un chunk HTTP: hoy hasta 300 chunks de ~150 B.
Menos tiempo bloqueando el loop y menos paquetes.

```cpp
static char blk[1400];
size_t used = 0;
// dentro del for, tras el snprintf a `fila` con longitud len (0 < len < sizeof(fila)):
if (used + len > sizeof(blk)) { server.sendContent(blk, used); used = 0; }
memcpy(blk + used, fila, len);
used += len;
// al terminar el for:
if (used) server.sendContent(blk, used);
```
Lo mismo para `ServerPrintAdapter` (hoy envía un chunk por línea porque hace flush en `'\n'`): acumular hasta
llenar el buffer y recién ahí enviar.

## P1-5 · Dos registros duplicados ≈66 KB → compactar  (`registro_ensayos.h`, `.ino`)

`RegistroEnsayos` (Fila de 216 B, `double` e `int64_t` en todo) y `bufferLog` (116 B) guardan casi lo
mismo en dos bloques contiguos. Además tienen capacidades distintas (150 vs 300 filas), y
`registroEnsayos.iniciar()` devuelve `false` en silencio cuando se llena o llega a 20 sesiones.

Fila compacta (sin `double`; contadores a 32 bits alcanzan: 1 kHz durante 49 días):

```cpp
struct MuestraC {                // 40 B
  uint32_t flancos, aceptados, pulsosMed, rechazados, edad_ms;
  uint16_t epoca, calidad;
  float    f_Hz, q_mLmin, v_L, k;
};
struct FilaC {                   // 92 B
  uint16_t sesion; uint8_t tipo, estado;
  uint32_t t_ms;  float rpm;
  MuestraC a, p;
};
```
150 × 92 B ≈ 13,8 KB frente a 31,6 KB. En `bufferLog` pueden no guardarse los derivados
(`merma_*`, `j_sobre_j0`, `recov`, `delta`, `j_lmh`, `j20`, `tcf`, `r_torta`) y recalcularlos al exportar,
con el cuidado de que la fórmula de export sea la misma. Decidan primero si quieren **un** solo registro.

## P1-6 · Front-end: polling sin control de concurrencia  (`index_html.cpp`, ~línea 937)

`setInterval(..., 500)` lanza un `fetch('/status')` cada 500 ms sin esperar la respuesta anterior, con
`.catch(() => {})`. Con servidor síncrono y hasta 4 clientes en el AP, un bloqueo del loop acumula
peticiones y empeora el bloqueo. Además, si falla, el operador ve datos congelados sin aviso.

```js
let busy = false;
async function poll() {
  if (busy) return;
  busy = true;
  const ac = new AbortController();
  const to = setTimeout(() => ac.abort(), 1500);
  try {
    const r = await fetch('/status', { signal: ac.signal });
    const d = await r.json();
    aplicarStatus(d);            // = el cuerpo actual del .then(d => {...}), movido a una función
    setSinConexion(false);
  } catch (e) {
    setSinConexion(true);        // banner visible "SIN CONEXIÓN — datos congelados"
  } finally {
    clearTimeout(to);
    busy = false;
  }
}
setInterval(poll, 1000);
```

---

## P2 · Limpieza

- **Qué no hace nada hoy** (confirmado por búsqueda en los archivos subidos): `TMP_MAX_SEGURA_BAR`,
  `P1_MAX_SEGURA_BAR`, `RPM_ALARMA_MEMBRANA`, `Q_CLINICO_SANGRE_MAX`, `K_UF_NOMINAL`,
  `DIAMETRO_CAPILAR_UM`, `ESPESOR_PARED_UM`, `SSID_STA`, `PASS_STA`;
  campos de `ConfigSensor` `lowMin_us`, `highMin_us`, `periodoMin_us`, `qMinCal_mLmin`, `qMaxCal_mLmin`;
  bits `FUERA_RANGO_CAL`, `FUERA_RANGO_OPERATIVO`, `MEDICION_EN_RANGO_CAL` (sus getters devuelven siempre
  `false`); `autoCalSumFrecPerm`; `RegistroEnsayos::marcarFalloControl()`; `declararCalibrado()`.
- `qMinDetectable` se puede editar desde el SCADA, pero **no se aplica en `capturar()`**: cambiarlo no
  modifica ninguna medición.
- El blanking de la ISR está fijo en 1200/3500 µs en el código en vez de leer `_cfg.periodoMin_us`
  (que tiene esos mismos valores): dejar una única fuente de verdad.
- `handleClearCSV()` llama a `registroEnsayos.limpiar()` aunque la bomba esté girando: deja `_activo=false`
  y el registro de metrología deja de grabar hasta el próximo arranque.
- Comentario de `Bomba.cpp` dice que se evita re-ejecutar `ledcAttach()` en cada arranque; el código lo
  ejecuta en cada arranque (porque `begin()` hace `ledcDetach`). Corregir comentario o implementación.
- Los comandos destructivos (`/clear_csv`, `/reset_dev`, `/cmd?act=RESET_VOL`) son `GET` sin
  confirmación ni autenticación; pasar a `POST` o exigir un parámetro de confirmación.
- `rpmRealConfirmada()` / `bombaGirandoReal()` devuelven la consigna de rampa, no la velocidad real
  (lazo abierto, sin encoder). Renombrar, o implementar detección de pérdida de sincronismo con
  `deltaBomba` (ej.: |Q_alim − Q_teórico| > umbral durante N s en régimen → parada + `marcarFalloControl()`).

---

## Checklist de verificación física (lo que no se puede resolver leyendo código)

1. **Pulsos por vuelta real.** Marca en el eje + cronómetro: a 10 RPM comandados, 1 vuelta física debe
   tardar 6 s. `config.h` asume 3200 pulsos/rev (1/16) y la configuración de DIP de la bitácora anterior
   era 1600 (1/8); confirmen que el DM860 está en el valor que el firmware supone.
2. **Analizador lógico en PUL** durante una rampa completa: buscar huecos o pulsos truncados cada vez que
   `ledcChangeFrequency()` se ejecuta (cada ≥2 Hz de cambio, hasta 20 veces por segundo).
3. **Prueba del bloqueo:** pedir una rampa y descargar el CSV al mismo tiempo; comparar el salto de
   frecuencia con y sin P0-2.
4. **Tasa de fallos de CRC del DS18B20** durante 1 hora con la bomba al máximo.
5. **`maxblk` (bloque libre más grande)** después de 1 hora con 2 clientes conectados, antes y después de P1-3.
6. **Retardo ENA→primer pulso** que el DM860 realmente necesita (datasheet + prueba), para fijar `TIEMPO_ENA_MS`.
7. **Pulsador físico de emergencia** cableado a la alimentación o ENA del driver.
