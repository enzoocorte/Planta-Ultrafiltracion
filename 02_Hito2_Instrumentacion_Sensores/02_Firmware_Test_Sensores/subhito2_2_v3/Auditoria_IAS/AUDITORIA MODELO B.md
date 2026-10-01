# New conversation

## User

[empty message]

## Assistant

# Auditoría externa — Firmware planta UF (Hito 2.2 V3)
**Para:** Ing. Enzo · **Consumidor de los parches:** Antigravity · **Alcance:** revisión estática del texto de `pasted-text.txt` (no compilé ni ejecuté el binario; los números hidráulicos se derivan con las suposiciones indicadas).

---

## 1. Diagnóstico ejecutivo

**Lo que está bien y hay que preservar:**
- Generación de pulsos por **hardware (LEDC)**, no por software en el loop: si el loop se atasca 2 s, el motor no pierde paso. Decisión correcta.
- ISR mínimo, `IRAM_ATTR`, `attachInterruptArg`, timestamp con `micros()` y blanking: el patrón es el adecuado.
- Persistencia NVS de parámetros, S-Curve, SoftAP dedicado + `WiFi.setSleep(false)`, `modo AP` en canal 1: correcto y robusto.
- Elección del **ADS1115 en lugar de ADC2** para los transductores: acertada (con Wi-Fi activo, ADC2 es inservible).

**Vulnerabilidades críticas (ordenadas por riesgo):**

| # | Hallazgo | Severidad |
|---|---|---|
| V1 | **No hay failsafe de hardware.** Si el CPU se cuelga, el LEDC sigue generando pulsos: la bomba continúa a la última frecuencia. `ENA` del DM860 no está cableado y no existe paro de emergencia fuera del MCU. | 🔴 Crítica |
| V2 | **Interlocks declarados pero no implementados.** `RPM_ALARMA_MEMBRANA = 36.0f` no se usa en ninguna parte; `setRPM()` permite hasta `RPM_MAX = 75` (1020 mL/min). El límite de 600 mL/min y TMP ≤ 0.50 bar sólo existen como texto. | 🔴 Crítica |
| V3 | **Inconsistencia numérica del propio pliego:** 600 mL/min ÷ 13.60 mL/rev = **44.12 RPM**, no 36 RPM (36 RPM = 489.6 mL/min). Si se codifica el interlock con 36 RPM, se fija el límite un 18 % por debajo del real; si se codifica con 600 mL/min, hay que resolver la discrepancia antes. | 🔴 Crítica |
| V4 | **El filtro RC no combate la interferencia real.** 4.7 kΩ ‖ 100 nF → fc = **338.6 Hz**; a 100 Hz atenúa sólo 4 % (factor 0.96). La interferencia de red está *dentro* de la banda de señal. El bug de flujo fantasma está mitigado por el blanking, no resuelto por el RC. | 🟠 Alta |
| V5 | **El blanking de 2 ms impone un techo de 500 Hz**, y el corte declarado `Q_MAX_FISICO_MLMIN = 6000` equivale a **927.7 Hz**: es inalcanzable. Peor: la frecuencia se satura en 500 Hz = **3233 mL/min** y nunca dispara el corte → la protección "de corte físico" es letra muerta. | 🟠 Alta |
| V6 | **Dos caudalímetros idénticos (YF-S401) con K de 154.62 y 55.0** (2.8× de diferencia). Además el K de alimentación parece derivado de la cilindrada teórica (105.14 Hz "=" 680 mL/min) y no gravimétricamente. La recuperación puede estar errada por decenas de %. | 🟠 Alta |
| V7 | **No hay conteo de pasos.** La bomba es open-loop absoluta: no existe volumen *entregado* redundante ni control dosimétrico por pasos. | 🟠 Alta |
| V8 | **`String` + `+=` en `handleStatus()`/`handleExportCSV()` dentro del loop**: fragmentación de heap y bloqueo del lazo de control (ver §3). | 🟠 Alta |
| V9 | `_q = (n > 0) ? (0.4*q + 0.6*_q) : 0.0f;` → **caída instantánea a cero** cuando la ventana no captura pulsos (con 5.5 Hz de permeado eso ocurre en ~0.4 % de las ventanas: escalones de −100 %). | 🟡 Media |
| V10 | `portMUX_TYPE Caudalimetro::_mux` es **estático y compartido**: los dos ISR se excluyen mutuamente sin necesidad. | 🟡 Media |
| V11 | `max()/min()` de Arduino son macros (doble evaluación, colisión con `std::min`, tipos mixtos en `max(0.8f, ...)`). Usar `fmaxf/fminf`. | 🟡 Media |
| V12 | `qPerm` no se valida contra `qAlim`; `recuperacion` puede superar 100 % sin bandera; `qRet = max(0, qAlim − qPerm)` **oculta** el error en lugar de reportarlo. | 🟡 Media |

---

## 2. Hallazgos críticos y código refactorizado

### 2.1 Capa de seguridad (V1, V2, V3) — prioridad uno

```cpp
// seguridad.h  — una única fuente de verdad en CAUDAL, no en RPM
constexpr float A_M_M2            = 2.2000f;   // m² de membrana FX100
constexpr float ML_POR_VUELTA_REF = 13.6000f; // mL/rev
constexpr float TMP_MAX_BAR       = 0.5000f;  // límite hidráulico duro
constexpr float Q_MAX_MEMBRANA    = 600.0f;   // mL/min  (¡= 44.12 RPM, NO 36!)
constexpr float RPM_MAX_MEMBRANA  = Q_MAX_MEMBRANA / ML_POR_VUELTA_REF; // 44.12

struct Seguridad {
  bool  membranaInstalada = true;   // perfil activo
  bool  alarmaLatcheada   = false;
  char  causa[32] = "";
};

// Cableado requerido (fuera del alcance del ESP32, imprescindible):
//  - ENA+/ENA- del DM860 accionados por GPIO independiente + relé/optocomando
//    que se abra por hardware ante un WDT no refrescado.
//  - Pulsador de PARO en serie con la alimentación de la etapa de potencia.
inline void evaluarInterlocks(float rpm, float tmp_bar, Seguridad& s) {
  if (s.alarmaLatcheada) return;
  if (s.membranaInstalada && rpm > RPM_MAX_MEMBRANA) {
    s.alarmaLatcheada = true; strncpy(s.causa, "RPM>LIMITE_MEMBRANA", 31);
  } else if (tmp_bar > TMP_MAX_BAR) {
    s.alarmaLatcheada = true; strncpy(s.causa, "TMP>0.50bar", 31);
  }
}
// En el lazo de control, ANTES de bomba.tick():
//   evaluarInterlocks(bomba.rpmActual(), h.tmp_bar, seg);
//   if (seg.alarmaLatcheada) { bomba.detener(); digitalWrite(PIN_ENA, HIGH); }
```
`detener()` debe ser **latcheado** (sólo se rearma con un POST explícito en la web + TMP < 0.1 bar + rpm = 0).

### 2.2 Generador de pulsos (V7 y D) — migrar a `FastAccelStepper` (RMT)

Tu análisis honesto: `ledcChangeFrequency()` cada 50 ms **no produce pérdida de paso** (el tren tiene 50 % de duty, ~187 µs de HIGH a 15 RPM; un glitch de reconfiguración tendría que recortar el pulso por debajo de 2.5 µs — improbable), pero sí produce dos defectos reales:
1. Es una **velocidad en escalera** con salto de ~9.3 Hz por tick (`3200/60 × 3.5`), audible como chirrido y no una rampa verdadera.
2. **No hay contador de pasos** → no hay volumen entregado.

Recomendación: usar el periférico **RMT** a 40 MHz de resolución (o MCPWM) con `FastAccelStepper`, que da aceleración continua, **conteo exacto de pasos** y libera el LEDC:

```cpp
// Bomba.cpp (núcleo) — reemplazo del bloque LEDC
FastAccelStepperEngine engine;
FastAccelStepper* stepper = nullptr;

void Bomba::begin() {
  pinMode(PIN_DIR, OUTPUT); pinMode(PIN_ENA, OUTPUT);
  digitalWrite(PIN_ENA, LOW);            // driver habilitado (activo bajo típ.)
  fijarSentido(true);
  engine.init();
  stepper = engine.stepperConnectToPin(PIN_PUL);
  stepper->setDirectionPin(PIN_DIR);
  stepper->setEnablePin(PIN_ENA, true);
  stepper->setAutoEnable(true);
  stepper->setSpeedInHz(800);                    // 15 RPM
  stepper->setAcceleration(4000 /*Hz/s*/);       // ~1.25 RPM/s en el arranque
}

inline float Bomba::rpmDePasos() const {
  return stepper->getCurrentSpeedInMilliHz() / 1000.0f * 60.0f / _pulsosPorRev;
}
uint32_t Bomba::pasosTotales() const { return stepper->getCurrentPosition(); }
float    Bomba::volumenEntregado_mL() const {
  return (pasosTotales() / (float)_pulsosPorRev) * _mlPorVuelta;   // ¡redundancia!
}
```
Con esto obtienes **la referencia de oro del caudal de alimentación**: la cilindrada por paso es mucho más estable (peristáltica calibrada, ±1 % en ventanas largas) que cualquier turbina. Úsala para el balance de recuperación (§4) y como testigo de desgaste de manguera.

### 2.3 Concurrencia FreeRTOS (B) — respuesta directa

- **Contención:** despreciable. `portENTER_CRITICAL()` en el ESP32 **eleva el nivel de máscara de interrupciones** core-local además de tomar el spinlock; la sección crítica aquí son ~5 instrucciones (~100 ns). No hay inversión de prioridad significativa.
- **Dos correcciones reales:** (a) el mux debe ser **por instancia**, no `static` compartido; (b) `_t_ultimo` es privado del ISR, no necesita crítico.
- **El verdadero problema de determinismo no es la ISR, es `server.handleClient()`**: un `sendContent(String)` de 50–60 KB bloquea el task del loop (Core 1) cientos de ms con un cliente lento, y eso sí degrada el lazo de 50 ms (tick de rampa + timestamp del datalogger).
- **Solución arquitectónica:** dos tasks + snapshot:

```cpp
struct Snapshot { float rpm, qAlim, qPerm, qRet, recov, volAlim, volPerm, tmp, j20; uint32_t ms; };
static Snapshot g_snap, g_snapWeb;          // doble buffer, sin locks largos
static SemaphoreHandle_t g_snapMux;

void taskControl(void*) {                    // Core 1, prio 10
  TickType_t t = xTaskGetTickCount();
  for (;;) {
    // 1) interlocks  2) bomba.tick(dt)  3) caudal (ver §4)  4) copiar snapshot
    xSemaphoreTake(g_snapMux, portMAX_DELAY); g_snapWeb = g_snap; xSemaphoreGive(g_snapMux);
    vTaskDelayUntil(&t, pdMS_TO_TICKS(50));
  }
}
void taskWeb(void*) {                        // Core 0, prio 2, 4 KB de stack
  for (;;) { server.handleClient(); vTaskDelay(2); }
}
// setup(): xTaskCreatePinnedToCore(...)
// El loop() de Arduino queda vacío (o como supervisor de heap/WDT).
```
Y en el camino crítico: `esp_task_wdt_add(NULL)` en `taskControl` con `esp_task_wdt_config_t{ .timeout_ms = 1000, ... }`, además del failsafe de hardware de §2.1.

---

## 3. Optimización del datalogger y eliminación de fragmentación de heap

**Respuesta corta a la pregunta A: sí, el riesgo es real.** No por "fuga" sino por **fragmentación**: cada request construye un `String` de ~50–65 KB reasignado logarítmicamente (1 KB→2→4→…→64 KB). Tras varios ciclos, `heap_caps_get_largest_free_block()` cae y una reasignación falla → *abort/panic*. Con 2–4 h de ensayo y clientes refrescando `/status` cada 2 s, esto es cuestión de tiempo.

**Reglas:** cero `String` en cualquier ruta HTTP; cero `String` en el loop; `snprintf` a buffers **estáticos**; salida **en chunks**.

### 3.1 `/status` — JSON por chunks, sin asignación dinámica

```cpp
void handleStatus() {
  char b[320];
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  int n = snprintf(b, sizeof(b),
      "{\"rpm\":%.1f,\"qalim\":%.1f,\"qperm\":%.1f,\"recov\":%.1f,"
      "\"vtot\":%.2f,\"alarma\":%d,\"heap\":%u,\"maxblk\":%u,\"f\":%.1f}",
      snap.rpm, snap.qAlim, snap.qPerm, snap.recov, snap.volAlim,
      (int)seg.alarmaLatcheada, ESP.getFreeHeap(),
      heap_caps_get_largest_free_block(MALLOC_CAP_8BIT), snap.fPerm);
  server.sendContent(b, n);       // chunk de 1 línea
  server.sendContent("");         // cierra el chunked
}
```
Usa **enteros escalados** en el JSON (`q*10`) si quieres reducir bytes. Expón siempre `maxblk` (mayor bloque libre) como telemetría de salud de memoria: es tu indicador temprano de degradación.

### 3.2 CSV del datalogger — streaming y registro compacto

```cpp
void handleExportCSV() {
  char b[256];
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/csv", "Content-Disposition: attachment; filename=\"ensayo.csv\"\r\n\r\n");
  int n = snprintf(b, sizeof(b),
    "id,t_s,rpm,qbom,qali,falim,valim,fperm,qperm,vperm,qret,recov,dP,ka,kp\r\n");
  server.sendContent(b, n);
  for (size_t i = 0; i < numRegistros; ++i) {
    const RegistroCalibracion& r = bufferLog[i];
    n = snprintf(b, sizeof(b),
      "%u,%lu,%.1f,%.1f,%.1f,%.2f,%.3f,%.2f,%.1f,%.3f,%.1f,%.1f,%.2f,%.2f,%.2f\r\n",
      r.id_ensayo, (unsigned long)r.t_relativo_s, r.rpm, r.q_bomba, r.q_alim,
      r.f_alim, r.vol_alim, r.f_perm, r.q_perm, r.vol_perm, r.q_ret, r.recov,
      r.delta, r.k_alim, r.k_perm);
    server.sendContent(b, n);
  }
  server.sendContent("");
}
```
Y **reduce el registro de 56 a ~28 bytes** con campos escalados (`uint16_t` con factor 10⁻¹…10⁻²), lo que además permite 1200 muestras en la misma RAM (`MAX_REGISTROS 560 B/set`). Guárdalo en `struct __attribute__((packed))`, añade un `uint16_t crc` por registro y una cabecera de sesión con `k_alim, k_perm, ml_rev, pul_rev` → CSV autodescriptivo y trazable para la tesis.

### 3.3 RAM vs LittleFS (C) — recomendación

**Híbrido, no o una u otra:**
- Anillo en RAM **como buffer de escritura** (rápido, atómico).
- Volcado **append-only a LittleFS** cada 512 B (~40 registros ≈ 6.7 min) desde `taskLog` (Core 0, prio 1) — nunca desde el loop ni desde la web. LittleFS (no SPIFFS) por *wear-levelling* real, resistencia a corte de energía y menos overhead de escritura.
- Desgaste: 8640 escrituras/día si escribieras cada 10 s; con bloques de 512 B bajas a ~205/día, y LittleFS reubica bloques: con 1 MB de partición y ~30 k–100 k ciclos/sector, la vida útil es de **años**. No es un problema si se agrupa; **sí lo es** si se hace `file.write()` por registro.
- NVS sólo para parámetros y **una** bandera `sesion_activa` + índice de ficheros; nunca para logs (NVS es caro en ciclos y en espacio).
- API: `/export_csv?file=N` sirviendo el fichero ya generado evita por completo el armado en memoria.

---

## 4. Medición de bajo caudal (E) — eliminación de la cuantización

**Cuantificación del problema.** Con la ventana de 1 s y contaje de pulsos: 1 pulso = `1000/K` mL/min, es decir **±18.2 mL/min con K = 55** → **±18 % a 100 mL/min**. Y peor: con la bomba peristáltica, la pulsación propia (rodillos) modula el flujo a `rodillos × RPM/60` Hz — a 50 RPM con 5 rodillos son **4.2 Hz**, del mismo orden que la señal de permeado → *aliasing* y error sistemático.

**Solución: medición de período + ventana sincronizada a los pasos.**

```cpp
// caudalimetro.h — añadir estado de período
volatile uint32_t _periodo_us = 0;   // último período entre flancos
volatile uint32_t _t_ultimo   = 0;

// ISR
void IRAM_ATTR Caudalimetro::isrPuente(void* arg) {
  auto* c = reinterpret_cast<Caudalimetro*>(arg);
  uint32_t t = micros();
  uint32_t dt = t - c->_t_ultimo;
  if (dt >= FILTRO_RUIDO_US) {
    portENTER_CRITICAL_ISR(&c->_mux);
    c->_periodo_us = dt;
    c->_t_ultimo   = t;
    c->_pulsos++;
    portEXIT_CRITICAL_ISR(&c->_mux);
  }
}

// Lectura híbrida — conteo en alto caudal, período en bajo caudal
float Caudalimetro::frecuenciaHz(float dt_s, uint32_t& nOut) {
  portENTER_CRITICAL(&_mux);
  uint32_t n = _pulsos; _pulsos = 0;
  uint32_t per = _periodo_us, tUlt = _t_ultimo;
  portEXIT_CRITICAL(&_mux);
  nOut = n;
  const float fConteo   = n / dt_s;                       // robusto si n > 5
  const float fPeriodo  = per ? 1e6f / per : 0.0f;        // 1 µs de resolución
  const bool  vivo      = (micros() - tUlt) < 4u * per;   // sin flancos recientes -> 0
  if (n >= 5)            return fConteo;                  // >~30 Hz
  if (vivo && fPeriodo > 0.2f) return fPeriodo;           // < 30 Hz
  return 0.0f;
}
```
Mejora de cuantización: con período medido por `micros()` el escalón pasa de ±18.2 mL/min a ~±0.02 mL/min (el error queda dominado por la jitter mecánica de la turbina, ±2–5 %). Sigue con **mediana de las últimas 5 medidas** (rechazo de espurios) + EMA suave.

**Ventana sincronizada (elimina el aliasing de rodillos):** aprovechando el conteo de pasos de §2.2, muestrea exactamente cada N revoluciones (p. ej. 4 × 3200 pasos); así la integral de flujo coincide con un número entero de pulsos de bomba y la modulación periódica se cancela. Es la diferencia entre ±18 % y ±3 % en el permeado.

**Validación cruzada obligatoria** (resuelve V5/V6/V12 y de paso el fantasma de red):
```cpp
const float enTol = 0.10f;    // 10 %
if (bomba.enRegimenEstable()) {
  float qTeo = bomba.volumenEntregado_mL();  // o rpmActual()*_mlPorVuelta
  if (fabsf(qAlim - qTeo) > enTol * qTeo) flagSensorAlim = true;  // aire/manguera/K
}
if (qPerm > qAlim * 0.98f) flagInconsistencia = true;             // permeado > alimentación
```
Y **calibración gravimétrica** real: balanza + cronómetro a 3 caudales (100/300/600 mL/min de alimentación y permeado), K por regresión `K = f/Q`. Verifica si los dos YF-S401 comparten K; si el permeado trabaja realmente a 98 Hz/(L/min), tu K = 55 sobreestima el caudal un **78 %**.

**Nota sobre la interferencia (V4):** ningún filtro pasabajos puede separar 50/100 Hz de una señal útil de 5–160 Hz. La solución correcta es **histéresis**: un 74HC14 o comparador con histéresis sobre el nodo (tras el RC) convierte una senoide de ruido de pocos cientos de mV en ninguna transición, y deja intacto el flanco de riel completo del Hall. Complementar con cable apantallado a un solo extremo, 100 nF cerámico en el pin, y mover los caudalímetros a **GPIO 34/35/36/39** (input-only, sin strapping ni glitches de arranque; ya tienes pull-up externo, así que no necesitas `INPUT_PULLUP`).

---

## 5. Arquitectura para TMP / ADS1115 (Hitos 2.3 y 2.4)

**Temporización:** el ADS1115 en modo *single-shot* tarda 1/128 s ≈ 7.8 ms por canal. Si escaneas 4 canales dentro del lazo de 50 ms, lo bloqueas. Reglas:

1. **Task dedicado `taskAdq` (Core 0, prio 5)** con `g_i2cMux` (SemaphoreHandle) y `Wire.setTimeOut(50)` — sin timeout, un bus colgado bloquea para siempre.
2. **Mejor aún: 2 × ADS1115** (ADDR a GND/3V3 → 0x48 y 0x49), cada uno en **modo continuo** sobre un par **diferencial**. La lectura de un registro son ~150 µs sin espera → cero bloqueo.
3. **Lee TMP en diferencial, no por diferencia de lecturas absolutas:**
   - A0−A1 = `P_alim − P_permeado` (= TMP de entrada)
   - A2−A3 = `P_ret − P_permeado` (= TMP de salida)
   - `TMP_media = (TMP_in + TMP_out)/2`
   Con transductores 0–1 MPa / 0.5–4.5 V (4 mV/kPa) y PGA = ±1.024 V, el LSB es 31 µV ≈ **8 Pa**: eliminas el error de offset común que arruina todo TMP calculado por resta.
4. **Alimentación/entorno:** PGA ±6.144 V si lees en single-ended (0.5–4.5 V **recorta** con ±4.096 V); desacople 100 nF + 10 µF junto al ADS; pull-ups I²C 4.7 k a 3.3 V; masa de señal única en el Front-End. Nunca `Wire` desde una ISR.
5. **Puesta a cero por ensayo:** guarda en NVS los offsets a presión atmosférica de cada transductor antes de cada corrida (los transductores deriva con la temperatura). Imprescindible para el Hito 5.
6. **TDS:** el canal restante + **temperatura obligatoria** (DS18B20 o NTC por otro canal): la conductividad se normaliza a 25 °C (`σ25 = σT / (1 + 0.02(T−25))`). Calibración a dos puntos (1413 µS/cm y 12.88 mS/cm) guardada en NVS, y verificación de que la sonda recibe excitación AC (los módulos DFRobot lo hacen).
7. **FB con histéresis en firmware:** mediana de 3 + rechazo si `|TMP| > 0.6 bar` (fuera de rango físico) → alarma + paro latcheado.

**Futuro (Hito 2.4+): todos los datos de entrada en un único `Snapshot` inmutable** (doble buffer, §2.3), nunca variables sueltas leídas por la task web: cada lector tiene entonces una *vista consistente* del proceso.

---

## 6. Consideraciones hidráulicas y modelado Darcy (FX100)

### 6.1 Velocidad tangencial y corte — lo que tus números dicen

Con `A_m = N·π·d_i·L`, se tiene `v_cap = Q/(N π d_i²/4) = 4·L·Q/(A_m·d_i)`. Asumiendo **L = 0.25 m** (dato que conviene confirmar del folleto del FX100), `d_i = 185 µm`, `ρ = 998 kg/m³`, `µ = 8.9·10⁻⁴ Pa·s`:

| Q (mL/min) | RPM | v_cap (mm/s) | Re | γ̇_w = 8v/d (s⁻¹) | τ_w (Pa) | ΔP axial (mbar) |
|---|---|---|---|---|---|---|
| 200 | 14.7 | **8.2** | **1.7** | 354 | **0.32** | 17 |
| 400 | 29.4 | 16.4 | 3.4 | 709 | 0.63 | 34 |
| 600 | 44.1 | **24.6** | **5.1** | 1062 | 0.95 | 51 |
| 1020 (75 RPM) | 75.0 | 41.8 | 8.7 | 1806 | 1.61 | 87 |

(ΔP axial por Hagen–Poiseuille laminar: `ΔP = 32 µ L v / d_i²`.)

**Conclusión dura que hay que poner en la tesis:** Re ≪ 2300 → régimen **estrictamente laminar**; y v_cap ≈ 1–4 cm/s está **20–100× por debajo** de los 0.5–2 m/s típicos de una UF de fibra hueca, con τ_w ≈ 0.3–1.6 Pa frente a los 2–8 Pa habituales. Es decir: con la bomba actual **no puedes generar el esfuerzo de corte necesario para controlar la torta de mucílago de Opuntia**. Consecuencias operativas:
- El permeado máximo que da el catálogo (73 mL/h·mmHg → 12.4 LMH a 0.5 bar) **no es alcanzable**: tu limita el flujo (2.7 LMH a 100 mL/min), no la membrana. Reconoce que el cuello de botella es *cake-limited*, no *membrane-limited*.
- Opera a **mínima conversión por paso** (recuperación < 10–15 %) y, si puedes añadir válvulas externas (no modificas cabezal ni bomba, así que es admisible), en **modo reciclo total** con concentración por lotes: así el caudal de alimentación atraviesa la membrana muchas veces por unidad de permeado y el corte efectivo sube por unidad de volumen procesado.
- Reversión de la bomba: sirve como **enjuague axial del lumen** (ciclo de flush), no como *backwash* transmembrana — para eso haría falta una vía de permeado valvulada, que hoy no tienes. No lo vendas como backwash en la memoria.

### 6.2 `J20` y `R_total` en tiempo real sin sacrificar el determinismo

`K_UF = 73 mL/(h·mmHg)` sobre 2.2 m² → `Lp = 6.92·10⁻¹¹ m/(Pa·s)` → **`R_m = 1/(Lp·µ₂₀) = 1.63·10¹³ m⁻¹`**. Verificación cruzada (consistencia perfecta): con `TMP = 0.5 bar = 50 kPa`, `J = TMP/(µ R_m) = 12.4 LMH` = `73 mL/h/mmHg × 375 mmHg / 2.2 m²`. Buen punto de partida para el modelo.

```cpp
// hidraulica.h — se ejecuta en taskControl a 1 Hz (nunca en la ISR ni en la web)
inline float viscosidadPaS(float T_C) {          // agua, válida 5-40 °C
  return 2.414e-5f * powf(10.0f, 247.8f / (T_C + 273.15f - 140.0f));
}

struct Darcy {
  float tmp_bar, j_lmh, j20_lmh, rtot_e12, rtort_e12, ratio_fouling;
};

inline Darcy calcular(const Snapshot& s, float T_C, float p_in_Pa, float p_ret_Pa,
                      float p_perm_Pa, float qperm_m3s) {
  Darcy d{};
  const float mu = viscosidadPaS(T_C);
  const float mu20 = 8.9e-4f;
  d.tmp_bar  = (((p_in_Pa + p_ret_Pa) * 0.5f) - p_perm_Pa) / 1e5f;   // Pa -> bar
  const float J = qperm_m3s / A_M_M2;                               // m/s
  d.j_lmh    = J * 3600.0f * 1000.0f;                               // LMH
  d.j20_lmh  = d.j_lmh * (mu / mu20);                               // normalizado en viscosidad
  d.rtot_e12 = (d.tmp_bar * 1e5f) / (mu * J) * 1e-12f;              // 1/m (×10^12)
  d.rtort_e12 = d.rtot_e12 - (R_M * 1e-12f);                        // R_m = 1.63e13 m^-1
  d.ratio_fouling = d.rtot_e12 / (R_M * 1e-12f);                    // índice robusto de ensuciamiento
  return d;
}
```
**Precaución conceptual:** `J20` sólo es comparable si el TMP al que se define es fijo y **se reporta**. Para tendencia de fouling usa **`R_total` normalizado por viscosidad y por área** (invariante frente a TMP e intensidad): es el único indicador que no te va a mentir entre corridas.

**Ajuste automático de `R_m` y `α` (α = resistencia específica de torta, m/kg):** guarda pares `(t, V_perm)` y ajusta por mínimos cuadrados la forma clásica de filtración con torta:
`t/V = [µ α C_b / (2 A² ΔP)] · V + µ R_m / (A ΔP)`
```cpp
// regresión recursiva simple sobre los (t, V) del ensayo actual
struct RegLin { double sx=0, sy=0, sxx=0, sxy=0; int n=0;
  void add(double x, double y){ sx+=x; sy+=y; sxx+=x*x; sxy+=x*y; n++; }
  bool fit(double& m, double& b) const {
    if (n < 6) return false;
    double den = n*sxx - sx*sx; if (fabs(den) < 1e-18) return false;
    m = (n*sxy - sx*sy)/den;  b = (sy - m*sx)/n;  return true; }
};
// R_m = b·A·ΔP/µ   ;   α = 2·A²·ΔP·m/(µ·C_b)
```
`C_b` (concentración de sólidos de la suspensión de Opuntia) debe entrarse como parámetro y medirse por gravimetría (g de sólido seco por litro). **Sin `C_b` no hay `α` válido**: es el dato que la tesis debe producir.

**Ensayo de flujo crítico (esto sí es un aporte fuerte para el Hito 5):** implementa una máquina de estados `auto_flujo_critico()` que suba TMP en escalones de 5 kPa cada 10 min y registre `dTMP/dt` vs `J`; el quiebre (donde `dTMP/dt` deja de ser plano) define el **flujo crítico** de la suspensión. Es un experimento de 1–2 h, automatizable con el firmware ya existente, y da la curva central de la tesis. Añade el ciclo de *flush* entre escalones para garantizar línea base reproducible.

---

### Resumen accionable (orden de implementación)
1. **Hardware:** `ENA` del DM860 + paro de emergencia que corte el tren de pulsos fuera del MCU. Sin esto, no hay operación desatendida.
2. **Interlocks de firmware** (§2.1) con la fuente de verdad en caudal y resolución de la discrepancia 36 RPM ↔ 600 mL/min.
3. **Snapshot + tasks** (§2.3) y **eliminación de `String`** (§3) + telemetría `maxblk`.
4. **Medición de período + ventana sincronizada a pasos** (§4) y **calibración gravimétrica** de ambos K.
5. **ADS1115 diferencial** + temperatura (§5).
6. **Darcy/`J20`/`R_total` + regresión de α** y **ensayo de flujo crítico** (§6).

Puedo entregarte en la próxima iteración un **parche unificado en formato diff** (`config.h`, `seguridad.h`, `Bomba`, `caudalimetro`, `hidraulica.h`, `tasks`) listo para aplicar por Antigravity, o un documento Word/PDF con esta auditoría formateada como anexo técnico de la tesis — dime cuál prefieres, Enzo.
