# 📘 MANUAL TÉCNICO EXHAUSTIVO Y GUÍA LÍNEA POR LÍNEA — FIRMWARE V6.5 DE PRODUCCIÓN
## Planta Piloto de Ultrafiltración Tangencial FX100 (Helixone®) • Tesis de Grado en Ingeniería Industrial & Química
**Universidad Nacional de Salta (UNSa) — Facultad de Ingeniería**  
**Tesistas de Grado:** Antonella Guitián & Owen Cañizares  
**Codirector de Tesis:** Ing. Enzo Corte *(Investigador Doctoral — Especialista en Procesos con Membranas)*  
**Directores:** Dr. Ing. Jorge Emilio Almazán  
**Versión de Firmware:** `V6.5 de Producción` (`EN_USO_firmware_planta`) — Octubre 2026  

---

## 🧭 1. Introducción y Propósito Pedagógico de este Documento

Este documento ha sido redactado para que los tesistas comprendan **cada línea de código, cada constante física y cada decisión de ingeniería** implementada en el microcontrolador ESP32 de la Planta Piloto. 

El firmware no es un simple programa de Arduino: es un **sistema mecatrónico embebido en tiempo real** que gobierna tres fenómenos acoplados:
1. **Cinemática y Electromecánica:** Control determinístico de un motor paso a paso NEMA 34 ($4.5\text{ Nm}$) acoplado a una bomba peristáltica MBP-2000 mediante trenes de pulsos de hardware LEDC con rampas cinemáticas fluidas.
2. **Metrología y Filtrado Anti-EMI:** Adquisición por interrupciones de hardware en silicio (IRAM) de dos caudalímetros de efecto Hall YF-S401, con blanking diferenciado por canal y filtro híbrido Mediana-3 + EMA para capturar el flujo pulsátil sin dejar pasar el ruido eléctrico de los motores.
3. **Fisicoquímica de Transporte en Membranas (Ley de Darcy):** Adquisición de temperatura por bus 1-Wire con verificación de redundancia cíclica (CRC-8), cálculo en vivo de la Presión Transmembrana ($\text{TMP}$), viscosidad dinámica del agua según correlación de Vogel, flujo normalizado a 20 °C ($J_{20}$), merma relativa ($J/J_0$) y resistencia hidráulica de la torta de bioflóculos ($R_{\text{torta}}$).

---

## 🗂️ 2. Mapa de Archivos del Proyecto

El firmware está modularizado en 6 componentes C++ y el sketch principal:

```
EN_USO_firmware_planta/
├── config.h                   --> El "diccionario maestro": pines, constantes cinemáticas y metrología.
├── Bomba.h / Bomba.cpp        --> Clase Bomba: gestión de rampas, timers LEDC y cortes de potencia ENA.
├── caudalimetro.h / .cpp      --> Clase Caudalimetro: ISRs atómicas, blanking físico y estimador híbrido.
├── darcy.h                    --> Ecuaciones de transporte: Ley de Darcy, viscosidad Vogel y resistencias.
├── registro_ensayos.h         --> Auditoría metrológica de alta precisión para las sesiones experimentales.
├── index_html.h / .cpp        --> Servidor Web SCADA embebido en Flash PROGMEM con telemetría interactiva.
└── EN_USO_firmware_planta.ino --> Orquestador maestro: setup, loop, driver DS18B20 y endpoints REST.
```

---

## 🔬 3. Explicación Detallada Módulo por Módulo y Línea por Línea

---

### 3.1. Archivo `config.h` (Configuración Técnica y Parámetros Calibrados)

`config.h` centraliza todas las variables que definen el comportamiento físico del equipo. Si cambia una manguera o un pin, solo se modifica este archivo.

#### Bloque 1: Asignación de Pines (Pinout)
```cpp
constexpr uint8_t PIN_PUL                = 18;  // DM860 PUL+ (PUL- a GND común)
constexpr uint8_t PIN_DIR                = 19;  // DM860 DIR+ (DIR- a GND común)
constexpr uint8_t PIN_ENA                = 23;  // DM860 ENA+ (ENA- a GND: HIGH = Driver deshabilitado)
constexpr uint8_t PIN_LED_BOMBA          = 2;   // LED Azul Onboard (indicador de marcha)
constexpr uint8_t PIN_SENSOR_ALIMENTACION = 14;  // Caudalímetro Feed (Borne 12 Sup - D14)
constexpr uint8_t PIN_SENSOR_PERMEADO     = 27;  // Caudalímetro Permeado (Borne 11 Sup - D27)
constexpr uint8_t PIN_SENSOR_TEMP_DS18B20 = 4;   // Sonda de Temperatura OneWire (Borne 8 Inf - D4)
constexpr uint8_t PIN_I2C_SDA             = 21;  // Bus I2C para futuro ADC ADS1115
constexpr uint8_t PIN_I2C_SCL             = 22;  // Bus I2C SCL
```
* **¿Por qué `constexpr` en vez de `#define`?**  
  `constexpr` le otorga tipo de dato estricto (`uint8_t`) al compilador de C++. Si intentas usar un número negativo o un tipo erróneo, el compilador frena antes de subir el código, evitando cuelgues raros en tiempo de ejecución.
* **Topología Cátodo Común del DM860:**  
  Todos los terminales negativos (`PUL-`, `DIR-`, `ENA-`) van unidos a la masa (`GND`) de la placa de control. Los pines del ESP32 envían pulsos positivos de $3.3\text{ V}$.
  * En `ENA`: Un nivel lógico **`HIGH` ($3.3\text{ V}$)** enciende el optoacoplador interno del DM860 y **deshabilita** el puente de potencia (motor libre). Un nivel **`LOW` ($0\text{ V}$)** deja el optoacoplador apagado y **habilita** el driver (motor con torque de retención).

#### Bloque 2: Aserción Estática de Pines Únicos
```cpp
constexpr bool pinesUnicos() { ... }
static_assert(pinesUnicos(), "ERROR: Hay pines GPIO compartidos en el sistema");
```
* **Línea de oro de seguridad:** `static_assert` corre en la computadora mientras compilas. Si un tesista por error asigna el pin 14 a dos sensores distintos, la compilación se detiene arrojando un cartel rojo en pantalla.

#### Bloque 3: Cinemática Calibrada de la Bomba Peristáltica
```cpp
constexpr uint16_t PULSOS_POR_REV = 3200;     // DM860 configurado a 1/16 micropasos
constexpr float ML_POR_VUELTA     = 13.5496f; // Calibración Ensayo 2 (20 a 90 RPM): Media = 13.55 mL/rev
constexpr float RPM_MIN           = 15.0f;    // ~203 mL/min
constexpr float RPM_MAX           = 100.0f;   // ~1355 mL/min (Diseño factorial en agua)
constexpr float RPM_INICIO        = 50.0f;    // Consigna inicial nominal (~677 mL/min)
```
* **¿De dónde sale `PULSOS_POR_REV = 3200`?**  
  El motor NEMA 34 da un paso completo cada $1.8^\circ$ ($360^\circ / 1.8^\circ = 200\text{ pasos/rev}$). El driver Leadshine DM860 está configurado mediante microinterruptores a **$1/16$ de micropaso**:
  $$\text{Pulsos por vuelta} = 200 \times 16 = \mathbf{3200\ \text{pulsos/rev}}$$
* **¿De dónde sale `ML_POR_VUELTA = 13.5496`?**  
  En el **Ensayo 2**, se recolectó agua con probeta clase A graduada a distintas velocidades fijas ($20, 30, 40, \dots, 90\text{ RPM}$) cronometradas. Al dividir el volumen total recolectado por el número de vueltas exactas que dio el rotor, la media aritmética de la cilindrada dio **$13.55\text{ mL/vuelta}$** con un coeficiente de variación de apenas $8.1\%$.
* **Cálculo de Caudal Teórico de Bomba ($Q_{\text{bomba}}$):**
  $$Q\ [\text{mL/min}] = \text{RPM} \times 13.5496\ [\text{mL/rev}]$$
  A $50\text{ RPM}$: $Q = 50 \times 13.5496 = \mathbf{677.5\ \text{mL/min}}$.

#### Bloque 4: Parámetros de Rampas Dinámicas
```cpp
constexpr float RPM_DESPEGUE         = 6.0f;   // 6.0 RPM despegue instantáneo sin resonancia (320 Hz)
constexpr float ACEL_ARRANQUE_RPM_S  = 12.0f;  // 12.0 RPM/s aceleración inicial
constexpr float ACEL_NOMINAL_RPM_S   = 18.0f;  // 18.0 RPM/s aceleración progresiva
constexpr float DESACEL_AJUSTE_RPM_S = 16.0f;  // 16.0 RPM/s desaceleración en marcha
constexpr float FRENADO_PARADA_RPM_S = 60.0f;  // 60.0 RPM/s frenado rápido en ~0.8s
constexpr float DT_MAX_RAMPA_S       = 0.10f;  // Tope temporal por ciclo de rampa (Parche P0-2)
constexpr uint32_t TIEMPO_ENA_MS     = 100;    // Retardo tras habilitar driver (Parche P0-3)
```
* **¿Por qué `RPM_DESPEGUE = 6.0`?**  
  A frecuencias muy bajas ($< 100\text{ Hz}$ o $< 2\text{ RPM}$), los pasos discretos del motor coinciden con la frecuencia natural del cabezal mecánico, generando resonancia, vibración y calentamiento. Arrancando en $6\text{ RPM}$ ($320\text{ Hz}$ en pulsos), el motor "despega" inmediatamente a una velocidad suave y silenciosa.
* **¿Por qué `DT_MAX_RAMPA_S = 0.10f`?**  
  Si un cliente en el celular solicita descargar el archivo CSV, el microcontrolador puede demorarse $1.5\text{ s}$ en despachar los paquetes de red. Si el código no tuviera este límite, al volver al cálculo de la rampa calcularía:
  $$\Delta \text{RPM} = 18\ \text{RPM/s} \times 1.5\ \text{s} = \mathbf{27\ \text{RPM de golpe}}$$
  Un salto de 27 RPM instantáneo sobre un NEMA 34 con manguera cargada causaría un golpe de ariete o pérdida de sincronismo (*stall*). Con este parche, el avance máximo por ciclo queda acotado a $18 \times 0.10 = \mathbf{1.8\ \text{RPM}}$, garantizando una curva matemática perfecta.

#### Bloque 5: Estructura y Configuración de Sensores YF-S401
```cpp
constexpr ConfigSensor SENSOR_ALIM_CFG = {
  PIN_SENSOR_ALIMENTACION, // Pin 14
  205.98,                  // K calibrado en Ensayo 3 [Hz / (L/min)] (12359 pulsos/L)
  1200,                    // Ancho mínimo LOW: 1200 us
  1200,                    // Ancho mínimo HIGH: 1200 us
  1200,                    // Período mínimo admisible: 1200 us (833 Hz -> tolera cresta pulsátil de 410 Hz)
  1500000,                 // Timeout de señal: 1.5 s
  2000.0,                  // Q máximo operativo admisible (mL/min)
  20.0,                    // Umbral mínimo de corte (mL/min)
  true,                    // Calibración documentada
  280.0, 1080.0            // Rango experimental documentado
};

constexpr ConfigSensor SENSOR_PERM_CFG = {
  PIN_SENSOR_PERMEADO,     // Pin 27
  205.98,                  // K homologado al mismo sensor físico
  3500,                    // Ancho mínimo LOW: 3500 us
  3500,                    // Ancho mínimo HIGH: 3500 us
  3500,                    // Período mínimo admisible: 3500 us (285 Hz -> bloquea ruidos espurios)
  3500000,                 // Timeout de señal: 3.5 s (permite medir gotas y caudales ultra-lentos)
  500.0, 2.0, false, 0.0, 0.0
};
```
* **La física del factor $K = 205.98\text{ Hz/(L/min)}$:**  
  La turbina interna del YF-S401 tiene un imán que gira con el paso del agua frente a un sensor Hall.  
  Relación lineal fundamental:
  $$F\ [\text{Hz}] = K \times Q\ [\text{L/min}] \iff Q\ [\text{mL/min}] = \frac{F\ [\text{Hz}] \times 1000}{K}$$
  A $1000\text{ mL/min}$ ($1\text{ L/min}$), la frecuencia es exactamente **$205.98\text{ Hz}$**.
  Pulsos generados por litro de agua:
  $$\text{Pulsos/Litro} = K \times 60 = 205.98 \times 60 = \mathbf{12,358.8\ \text{pulsos/L}}$$

---

### 3.2. Archivo `Bomba.cpp` (Control Cinemático del Motor NEMA 34)

Este archivo implementa el generador de pulsos y la máquina de estados de la bomba peristáltica.

#### `Bomba::begin()` — Inicialización Segura
```cpp
void Bomba::begin() {
  pinMode(PIN_DIR, OUTPUT);
  digitalWrite(PIN_DIR, LOW);
  fijarSentido(true); // Sentido horario por defecto (Filtración)

  pinMode(PIN_ENA, OUTPUT);
  digitalWrite(PIN_ENA, LOW); // Driver DM860 habilitado normalmente

#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
  ledcAttach(PIN_PUL, 800, 10);
  ledcWrite(PIN_PUL, 0); // Reposo seguro: Duty 0 = nivel bajo continuo sin pulsos
#else
  ledcSetup(0, 800, 10);
  ledcAttachPin(PIN_PUL, 0);
  ledcWrite(0, 0);
#endif
  ...
}
```
* **Compatibilidad ESP32 Core 3.x vs Core 2.x:**  
  Las directivas `#if defined(ESP_ARDUINO_VERSION)` permiten que este código compile tanto en computadoras con versiones viejas del core como en el nuevo Core 3.3.12 de Espressif.
* **¿Por qué `ledcWrite(PIN_PUL, 0)` en reposo?**  
  El temporizador PWM de hardware queda activo en segundo plano, pero al asignarle **Duty Cycle = 0**, el pin permanece fijado a masa ($0\text{ V}$ continuo). Esto evita llamar a `ledcDetach()` al parar la bomba, eliminando los fallos de re-enganche de temporizadores que bloqueaban las versiones anteriores.

#### `Bomba::paradaEmergencia()` y `Bomba::rearmarEmergencia()`
```cpp
void Bomba::paradaEmergencia() {
  _enEmergencia = true;
  digitalWrite(PIN_ENA, HIGH); // Corte físico instantáneo en el driver DM860 (< 1 ms)
  _enMarcha = false;
  _invirtiendo = false;
  _actual = 0.0f;
  _objetivo = 0.0f;
  _fActual = 0;
  ledcWrite(PIN_PUL, 0);
}

void Bomba::rearmarEmergencia() {
  _enEmergencia = false;
  digitalWrite(PIN_ENA, LOW); // Restablece habilitación de potencia
  _actual = 0.0f;
  _objetivo = (_rpmGuardada >= RPM_MIN) ? _rpmGuardada : RPM_INICIO;
}
```
* **Corte Físico Real:** Al presionar Parada de Emergencia en el SCADA, `digitalWrite(PIN_ENA, HIGH)` conmuta el optoacoplador del DM860, desconectando en menos de $1\text{ ms}$ la alimentación hacia las bobinas del motor, suprimiendo cualquier par motor de forma inmediata.

#### `Bomba::tick(float dt)` — El Algoritmo de Rampa Fluida
```cpp
void Bomba::tick(float dt) {
  if (_enEmergencia) {
    ...
    return;
  }

  if (!(dt > 0.0f)) return;
  if (dt > DT_MAX_RAMPA_S) dt = DT_MAX_RAMPA_S; // Parche P0-2: Evita tirones si el loop se demora

  // Parche P0-3: Tiempo de establecimiento del driver DM860 tras habilitar ENA (100 ms)
  if (_enMarcha && (millis() - _tEnable_ms) < TIEMPO_ENA_MS) {
    return;
  }
```
* Esta guarda `(millis() - _tEnable_ms) < TIEMPO_ENA_MS` asegura que las fuentes internas y bombas de carga del driver DM860 alcancen su tensión estable antes de aplicar la conmutación de potencia.

```cpp
  if (_enMarcha && _actual < objetivo) {
    if (_actual < RPM_DESPEGUE) {
      _actual = RPM_DESPEGUE; // Salto instantáneo a 6 RPM (320 Hz)
    }

    float delta = objetivo - _actual;
    float tasaAcel = ACEL_NOMINAL_RPM_S;
    if (_actual < 12.0f) {
      tasaAcel = ACEL_ARRANQUE_RPM_S + ((_actual - RPM_DESPEGUE) / (12.0f - RPM_DESPEGUE)) * (ACEL_NOMINAL_RPM_S - ACEL_ARRANQUE_RPM_S);
    }
    if (delta < 2.0f) {
      tasaAcel = fmaxf(3.0f, tasaAcel * (delta / 2.0f)); // Amortiguación de llegada
    }
    _actual = fminf(objetivo, _actual + tasaAcel * dt);
```
* **Fórmula de aceleración con amortiguación:**  
  La bomba calcula la velocidad actual integrando numéricamente $v(t) = v_{t-1} + a \cdot \Delta t$. Cuando se encuentra a menos de 2 RPM del objetivo (`delta < 2.0f`), reduce suavemente la aceleración para llegar a la consigna sin sobrepaso (*overshoot*) ni oscilación.

```cpp
  // Generación de pulsos PWM por hardware LEDC
  if (_enMarcha && _actual >= 1.0f) {
    uint32_t f = (uint32_t)(_actual * (float)_pulsosPorRev / 60.0f);
    if (f < 50) f = 50;

    if (f != _fActual) {
      uint32_t retF = ledcChangeFrequency(PIN_PUL, f, 10);
      if (retF > 0) _fActual = retF;
      else _fActual = f;
    }
    ledcWrite(PIN_PUL, 512); // Duty 50% (512 de 1024 niveles en resolución de 10 bits)
  }
```
* **La matemática de la frecuencia PWM del motor:**
  $$f\ [\text{Hz}] = \frac{\text{RPM} \times \text{Pulsos/Rev}}{60} = \frac{\text{RPM} \times 3200}{60}$$
  * A **$25\text{ RPM}$**: $f = (25 \times 3200) / 60 = \mathbf{1333.3\ \text{Hz}}$.
  * A **$50\text{ RPM}$**: $f = (50 \times 3200) / 60 = \mathbf{2666.7\ \text{Hz}}$.
  * A **$100\text{ RPM}$**: $f = (100 \times 3200) / 60 = \mathbf{5333.3\ \text{Hz}}$.
* **¿Por qué `ledcWrite(PIN_PUL, 512)`?**  
  En resolución de 10 bits, el rango de modulación va de $0$ a $2^{10}-1 = 1023$. Un valor de $512$ representa exactamente un **Duty Cycle del $50\%$** (onda cuadrada perfecta simétrica: $50\%$ tiempo en HIGH, $50\%$ tiempo en LOW), que es la señal óptima que exigen los optoacopladores rápidos del DM860.

---

### 3.3. Archivo `caudalimetro.cpp` (Algoritmo de Medición y Filtrado Anti-EMI)

Este módulo es el responsable de que los caudalímetros midan con precisión científica.

#### `Caudalimetro::isr()` — Rutina de Interrupción por Flanco en Silicio
```cpp
void ARDUINO_ISR_ATTR Caudalimetro::isr() {
  portENTER_CRITICAL_ISR(&_mux);
  const int64_t ahora = esp_timer_get_time();

  if (_bloqueado) {
    portEXIT_CRITICAL_ISR(&_mux);
    return;
  }

  ++_flancos;
  if (_seco) _alarmaSeco = true;

  const int64_t dt = _hayAnterior ? (ahora - _bajadaAnterior_us) : 0;

  // Blanking digital diferenciado por canal (Anti-EMI):
  const int64_t blanking_us = _esAlimentacion ? 1200LL : 3500LL;

  if (!_hayAnterior || dt >= blanking_us) {
    if (_pulsosVentana == 0) {
      _primeroVentana_us = ahora;
    }
    _ultimoVentana_us = ahora;
    ++_pulsosVentana;
    ...
    _bajadaAnterior_us = ahora;
    _ultimoAceptado_us = ahora;
    _hayAnterior = true;
    ++_aceptados;
    if (!_seco) ++_pulsosMedicion;
  } else {
    ++_rechazados;
  }

  portEXIT_CRITICAL_ISR(&_mux);
}
```
* **¿Qué hace `portENTER_CRITICAL_ISR`?**  
  El ESP32 tiene 2 núcleos de CPU que trabajan a 240 MHz. `portENTER_CRITICAL_ISR` toma un cerrojo (*spinlock*) a nivel de hardware que impide que el otro núcleo o el sistema operativo FreeRTOS interrumpan mientras se actualizan las variables de tiempo.
* **¿Por qué `esp_timer_get_time()` en vez de `micros()`?**  
  `esp_timer_get_time()` devuelve un entero de 64 bits (`int64_t`) con los microsegundos transcurridos desde el encendido. La función estándar `micros()` de Arduino es de 32 bits y se desborda (*overflow*) a los 71 minutos; `esp_timer_get_time()` tarda más de **292,000 años** en desbordarse, garantizando ensayos prolongados de vida útil sin fallos.
* **La Justificación del Blanking Diferenciado (1200 µs vs 3500 µs):**
  * **En Alimentación (GPIO 14):** La bomba peristáltica a 100 RPM genera un flujo medio de $279\text{ Hz}$, pero por la acción pulsátil de los 3 rodillos comprimiendo la manguera, la velocidad instantánea de cresta alcanza los **$410\text{ Hz}$** ($T = 2440\ \mu\text{s}$).  
    Con `blanking_us = 1200LL` ($f_{\text{máx}} = 833\text{ Hz}$), la cresta de $410\text{ Hz}$ pasa limpia y completa, mientras que cualquier chispa o rebote capacitivo de alta frecuencia ($> 1000\text{ Hz}$) es descartado como pulso espurio (`++_rechazados`).
  * **En Permeado (GPIO 27):** El caudal a través de la membrana nunca supera los $250\text{ mL/min}$ ($f < 52\text{ Hz}$, período $T > 19,000\ \mu\text{s}$). Con `blanking_us = 3500LL` ($f_{\text{máx}} = 285\text{ Hz}$), se bloquea el 100% de los picos inducidos por falso contacto (como el pico histórico de $373\text{ Hz}$), dejando pasar el flujo real.

#### `Caudalimetro::capturar()` — Estimador Híbrido y Filtro de Mediana
```cpp
Caudalimetro::Muestra Caudalimetro::capturar(bool bombaEmpuja) {
  ...
  // Sección crítica atómica: copia y resetea los acumuladores del segundo
  portENTER_CRITICAL(&_mux);
  m.t_us              = esp_timer_get_time();
  ...
  nVentana            = _pulsosVentana;
  _pulsosVentana      = 0; // Se resetea para el próximo segundo
  portEXIT_CRITICAL(&_mux);
```
* **Principio de No Destrucción:** Esta función se ejecuta **estrictamente una vez por segundo** en el `loop()` principal. Copia cuántos pulsos válidos entraron en la ventana y los reinicia a 0.

```cpp
  // Estimador Híbrido: Conteo Directo vs Período Recíproco de Ráfaga
  double f = 0.0;
  const double fConteo = ((double)nVentana * 1.0e6) / (double)dtVentana_us;
  const int64_t span_us = ultimo_us - primero_us;

  if (nVentana >= 2 && span_us >= 350000LL) {
    f = ((double)(nVentana - 1) * 1.0e6) / (double)span_us;
    if (f > fConteo * 1.35 + 1.5 || f < fConteo * 0.65 - 1.5) {
      f = fConteo; // Fallback seguro ante irregularidades
    }
  } else if (nVentana >= 1) {
    f = fConteo;
  }
```
* **¿Por qué un Estimador Híbrido?**
  * Para frecuencias altas ($n \ge 2$), medir el tiempo exacto entre el primer pulso y el último pulso de la ventana (`span_us`) ofrece una resolución continua de decimales, sin el "escalonamiento" discreto de contar pulsos enteros.
  * Para validar el período recíproco, se exige que la ráfaga cubra al menos $350\text{ ms}$ (`span_us >= 350000LL`). Si dos pulsos cayeran muy juntos por ruido, la condición no se cumple y el código adopta el conteo directo $n / \Delta t$, haciendo matemáticamente imposible que 2 pulsos falsos generen una lectura disparatada.

```cpp
  // Filtro Mediana de 3 puntos + Suavizado Exponencial EMA
  if (!sinSenal && (nVentana > 0 || m.frecuencia_Hz > 0.05)) {
    float qInst = (float)m.q_mLmin;
    _qHist[0] = _qHist[1];
    _qHist[1] = _qHist[2];
    _qHist[2] = qInst;
    if (_qHistCount < 3) _qHistCount++;

    float qMed = (_qHistCount >= 3) ? mediana3(_qHist[0], _qHist[1], _qHist[2]) : qInst;
    _qSuavizadoDisplay = (_qSuavizadoDisplay == 0.0f)
                           ? qMed
                           : (0.35f * qMed + 0.65f * _qSuavizadoDisplay);
    _cerosConsecutivos = 0;
  }
```
* **¿Cómo funciona `mediana3`?**  
  Si el caudalímetro venía midiendo $[650, 648]\text{ mL/min}$ y en un segundo una burbuja o movimiento de manguera causa una lectura de $200\text{ mL/min}$, el arreglo queda $[650, 648, 200]$.  
  Al ordenar los valores: $\{200, \mathbf{648}, 650\}$. La mediana selecciona **$648\text{ mL/min}$**, descartando completamente la caída abrupta de 1 segundo sin retrasar la respuesta del sistema.
* **Suavizado Exponencial EMA ($\alpha = 0.35$):**  
  Aplica la fórmula $Q_{\text{filtrado}} = 0.35 \cdot Q_{\text{mediana}} + 0.65 \cdot Q_{\text{anterior}}$, generando una lectura estable y estética en la pantalla del celular.

---

### 3.4. Archivo `darcy.h` (Modelado Físico de Membranas y Resistencias)

Este archivo implementa el modelo matemático de transporte que los tesistas defenderán en el tribunal de tesis.

#### Ecuación 1: Corrección de Viscosidad del Agua por Ecuación de Vogel
```cpp
inline float viscosidadAgua_Pa_s(float tempC) {
  float T_K = tempC + 273.15f;
  return 0.00002414f * powf(10.0f, 247.8f / (T_K - 140.0f));
}
```
* La viscosidad del agua ($\mu$) cambia notablemente con la temperatura: a $15\ ^\circ\text{C}$ es $\approx 1.14\text{ mPa}\cdot\text{s}$, mientras que a $25\ ^\circ\text{C}$ baja a $\approx 0.89\text{ mPa}\cdot\text{s}$ (una variación de casi el $28\%$).
* La ecuación de Vogel modela con precisión analítica la viscosidad dinámica $\mu(T)$ en $\text{Pa}\cdot\text{s}$.
* **Factor de Corrección Térmica ($TCF$):**
  $$TCF = \frac{\mu(20\ ^\circ\text{C})}{\mu(T)}$$
  Normaliza el flujo permeado real medido a una temperatura estándar de referencia de $20\ ^\circ\text{C}$:
  $$J_{20} = J_{\text{actual}} \times TCF$$

#### Ecuación 2: Ley de Darcy y Resistencias en Serie
```cpp
ResultadoDarcy calcular(float qPerm_mLmin, float tmp_bar, float tempC) const {
  ...
  float J_m_s = (qPerm_mLmin * 1e-6f / 60.0f) / AREA_MEMBRANA_M2; // Flujo en m/s
  res.J_LMH = J_m_s * 3.6e6f;                                     // Flujo en LMH [L/(m²·h)]
  ...
  float deltaP_Pa = tmp_bar * 100000.0f;                          // bar a Pascales
  res.R_total = deltaP_Pa / (mu * J_m_s);                          // Ley de Darcy: R_tot = TMP / (mu * J)
  res.R_adicional = fmaxf(0.0f, res.R_total - Rm_teorico);        // Resistencia de torta: R_torta = R_tot - Rm
```
* **La Ley Fundamental de Darcy en Ultrafiltración:**
  $$J = \frac{\Delta P}{\mu \cdot R_{\text{total}}} \iff R_{\text{total}} = \frac{\text{TMP}}{\mu \cdot J}$$
* **Modelo de Resistencias en Serie:**
  $$R_{\text{total}} = R_m + R_{\text{torta}}$$
  Donde:
  * $R_m$: Resistencia intrínseca de la membrana limpia ($4.56 \times 10^{11}\ \text{m}^{-1}$ para el dializador Fresenius FX100).
  * $R_{\text{torta}}$: Resistencia adicional acumulada por la deposición de sólidos, coloides y biopolímeros de la penca de tuna (*Opuntia ficus-indica*).  
  El firmware calcula $R_{\text{torta}}$ en cada segundo, permitiendo graficar en tiempo real el ensuciamiento hidráulico de la fibra capilar.

---

### 3.5. Archivo `EN_USO_firmware_planta.ino` (Orquestador Principal)

Es el archivo maestro de Arduino que vincula todos los objetos.

#### Driver Nativo 1-Wire DS18B20 (`namespace DS18B20_Native`)
```cpp
namespace DS18B20_Native {
  static portMUX_TYPE owMux = portMUX_INITIALIZER_UNLOCKED;

  // CRC-8 Dallas/Maxim (polinomio x^8 + x^5 + x^4 + 1, 0x8C reflejado)
  static uint8_t crc8(const uint8_t* d, uint8_t n) { ... }
```
* **¿Por qué se programó un driver propio en vez de usar librerías externas?**  
  Las librerías tradicionales de Arduino (`OneWire.h` y `DallasTemperature.h`) usan `delay(750)` milisegundos bloqueantes mientras el sensor convierte la temperatura. Además, la función `digitalRead` de esas librerías no está protegida contra las interrupciones de los caudalímetros.  
  El driver nativo de este firmware:
  1. Protege cada microsegundo del pulso 1-Wire con `portENTER_CRITICAL(&owMux)`, evitando que una interrupción deforme la comunicación.
  2. Calcula el **CRC-8 Dallas** en silicio. Si hay un cable flojo o interferencia, descarta el paquete.
  3. Trabaja de forma **asíncrona**: en el segundo actual lee la temperatura ya convertida y dispara la nueva conversión para el segundo siguiente, demandando apenas $5\text{ ms}$ de CPU por segundo.

#### Gestión Dinámica de Memoria RAM en `setup()`
```cpp
void setup() {
  ...
  // Asignación segura del buffer de datalogger en Heap (elimina 74.4 KB de .dram0.bss)
  bufferLog = new (std::nothrow) RegistroCalibracion[MAX_REGISTROS]{};
  if (bufferLog) {
    Serial.printf("[DATALOGGER] Buffer circular de %u registros asignado en Heap (%u bytes).\n",
                  (unsigned)MAX_REGISTROS, (unsigned)(MAX_REGISTROS * sizeof(RegistroCalibracion)));
  }
```
* **¿Qué resuelve `new (std::nothrow)`?**  
  En C++, las variables globales estáticas se alojan en la memoria `.dram0.bss`. El ESP32 solo tiene $\approx 320\text{ KB}$ de memoria RAM compartida entre el código, los buffers del stack Wi-Fi y los sockets TCP.  
  Tener un arreglo global fijo de $74.4\text{ KB}$ provocaba que el sistema colapsara con el error `assert failed: esp_startup_start_app (res == pdTRUE)` al iniciar el Wi-Fi. Al crearlo dinámicamente en el Heap con `new (std::nothrow)` y dimensionarlo en 300 registros ($34\text{ KB}$), el arranque es $100\%$ estable y sobra memoria para atender hasta 4 clientes celulares simultáneos.

#### El Bucle Principal `loop()`
```cpp
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  const uint32_t tAhora = millis();

  // 1. Tarea de Alta Frecuencia: Control Cinemático de Bomba (cada 20 a 50 ms)
  if (tAhora - tLoop >= 30) {
    float dt = (float)(tAhora - tLoop) / 1000.0f;
    tLoop = tAhora;
    bomba.tick(dt);
  }

  // 2. Tarea de 1 Segundo: Adquisición de Caudales, Temperatura y Darcy
  if (tAhora - tCaudal >= 1000) {
    tCaudal = tAhora;
    ...
  }

  // 3. Tarea de 10 Segundos: Datalogger Multi-Sesión
  if (tAhora - tDatalogger >= INTERVALO_LOG_MS) {
    tDatalogger = tAhora;
    if (bombaGirandoReal()) {
      guardarMuestraDatalogger();
    }
  }

  delay(2); // Cede tiempo a la tarea IDLE del FreeRTOS para resetear el Watchdog (WDT)
}
```
* **¿Por qué `delay(2)` al final del loop?**  
  En el ESP32, el sistema operativo FreeRTOS ejecuta el perro guardián (*Task Watchdog Timer - TWDT*). Si el loop no cede tiempo de CPU mediante un pequeño `delay()`, el microcontrolador asume que el programa se congeló y se reinicia por hardware con el error `rst:0x7 (TG0WDT_SYS_RESET)`. El `delay(2)` alimenta al perro guardián y mantiene el sistema operativo corriendo con total estabilidad.

---

## 🎓 4. Preguntas Frecuentes de Examen y Defensa de Tesis

Estas preguntas son típicas de un jurado de tesis de Ingeniería Industrial / Química al evaluar la instrumentación de la planta:

### P1: ¿Por qué los dos sensores YF-S401 tienen tiempos de blanking distintos si son el mismo modelo físico?
**Respuesta:** Porque operan en **regímenes hidrodinámicos radicalmente distintos**.  
El sensor de Alimentación maneja todo el caudal impulsado por los rodillos de la bomba peristáltica (hasta $1400\text{ mL/min}$ con crestas instantáneas de $410\text{ Hz}$), por lo que necesita un blanking corto (**$1200\ \mu\text{s}$**) para no perder pulsos reales del agua.  
El sensor de Permeado mide el líquido que logra atravesar los poros de la membrana capilar (un flujo laminar amortiguado de menos de $250\text{ mL/min}$ o $< 52\text{ Hz}$), por lo que un blanking de **$3500\ \mu\text{s}$** es ideal para filtrar cualquier inducción electromagnética del motor sin alterar la medición.

### P2: ¿Por qué se optó por un driver DM860 configurado a 3200 pulsos/rev y qué ventaja aporta en la tesis?
**Respuesta:** Un motor paso a paso convencional gira a saltos de $1.8^\circ$ ($200\text{ pasos/vuelta}$). Al accionar una bomba peristáltica a bajas RPM ($20\text{ a }35\text{ RPM}$), los pasos discretos generan una pulsación brusca (*torque ripple*) que fatiga la manguera de silicona y causa resonancia acústica.  
Configurando el driver a **$1/16$ de micropaso ($3200\text{ pulsos/rev}$)**, cada pulso eléctrico representa apenas $0.1125^\circ$ de giro. El movimiento resultante es continuo, ultrasuave y silencioso, alargando la vida útil del tubo peristáltico y estabilizando la presión de alimentación.

### P3: ¿Cómo se garantiza que el microcontrolador no pierda datos si se corta la conexión Wi-Fi?
**Respuesta:** El ESP32 es completamente **autónomo**. Toda la lógica de control, el cálculo de Darcy, la máquina de estados de la bomba y el almacenamiento circular del datalogger corren dentro de la memoria del microcontrolador. La interfaz web SCADA es únicamente un cliente de visualización pasiva; si el teléfono se apaga o pierde señal Wi-Fi, la planta sigue operando y registrando datos ininterrumpidamente cada 10 segundos.

---

## 🏁 5. Conclusión

Con la versión **V6.5 de Producción**, la Planta Piloto de Ultrafiltración de la UNSa cuenta con una base de firmware madura, segura y con rigor científico, preparada tanto para la experimentación en banco como para la redacción final del Capítulo 4 de la memoria de Tesis de Grado.
