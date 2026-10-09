# PROMPT DE AUDITORÍA EXTERNA — RONDA 6: CERTIFICACIÓN SUBHITO 2.2 (CAUDALÍMETROS Y FIRMWARE)
## Planta Piloto de Ultrafiltración FX100 — Tesis de Grado en Ingeniería Industrial (UNSa)
**Tesistas:** Antonella Guitián & Owen Cañizares  
**Codirección / Soporte:** Ing. Enzo  
**Materia:** Proyecto Final de Grado — Facultad de Ingeniería, Universidad Nacional de Salta  

---

### INSTRUCCIONES PARA EL AUDITOR DE IA:
Actúas como un **Auditor Senior en Sistemas Embebidos, Metrología Industrial y Control de Procesos (Nivel Doctorado / Perito Técnico Industrial)**.
No tienes memoria previa de rondas anteriores. Evalúa de manera crítica, rigurosa, cuantitativa e imparcial la documentación, el problema físico-experimental y el código completo presentado a continuación.
Tu dictamen debe indicar con total claridad: **APROBADO**, **APROBADO CON OBSERVACIONES**, o **RECHAZADO**, justificando cada punto matemáticamente y emitiendo recomendaciones directas de implementación.

> **REQUERIMIENTO OBLIGATORIO DE CÓDIGO FUNCIONAL:**  
> No te limites a emitir críticas conceptuales ni sugerencias abstractas. **Debes entregar tu propia propuesta de CÓDIGO COMPLETO, funcional y listo para compilar en ESP32 (Drop-in replacement)** para los módulos que consideres que deben corregirse (especialmente `caudalimetro.h`, `caudalimetro.cpp`, o una arquitectura alternativa con el periférico PCNT / DSP). El equipo de ingenieros contrastará tu código con el firmware actual para integrar tus mejoras directamente en el software real de la planta.

---

# 1. CONTEXTO GENERAL DEL PROYECTO Y OBJETIVOS

El proyecto consiste en la automatización, control cinemático, adquisición de datos y modelado matemático en tiempo real de una **Planta Piloto de Ultrafiltración Tangencial** de escala laboratorio/industrial, basada en una membrana capilar de hemodiálisis **Fresenius FX100 (Helixone®)**, para el tratamiento y separación de efluentes acuosos.

### Componentes del Sistema:
1. **Actuador Principal:** Bomba peristáltica con cabezal de 3 rodillos (manguera de silicona de 12 mm de diámetro exterior), impulsada por un motor paso a paso **NEMA 34** de alto torque acoplado directamente al cabezal.
2. **Driver de Potencia:** Leadshine **DM860** configurado a **3.0 A pico** y **3200 micropasos/rev** (1/16 paso).
3. **Sensores de Flujo:** Dos caudalímetros de turbina con sensor de efecto Hall **YF-S401** (uno en la línea de Alimentación y otro en la línea de Permeado).
4. **Unidad Central de Procesamiento:** Microcontrolador **ESP32 DevKit v1 (38 pines)** operando a 240 MHz con FreeRTOS y Wi-Fi SoftAP autónomo (192.168.4.1), sirviendo una interfaz Web SCADA en tiempo real y portal cautivo anti-desconexión.
5. **Membrana de Ultrafiltración:** Fresenius FX100:
   - Área interfacial efectiva: $A = 2.2\text{ m}^2$.
   - Coeficiente de ultrafiltración nominal: $K_{\text{UF}} = 73\text{ mL/(h}\cdot\text{mmHg)}$.
   - Límite de seguridad de Presión Transmembrana: $\text{TMP}_{\text{max}} = 0.50\text{ bar}$.

---

# 2. EL PROBLEMA EXPERIMENTAL Y EL RECHAZO EN RONDA 5

En la campaña experimental previa (Subhito 2.2: Calibración y Adquisición de Caudalímetros), los operadores realizaron ensayos de calibración con probeta graduada en banco real operando la bomba de 20 a 90 RPM.

### 2.1. Hallazgos Anómalos Observados en Laboratorio:
1. **Caudal Fantasma en Permeado:** Durante la prueba de probeta de la línea de alimentación, la línea de permeado estuvo **hidráulicamente vacía y seca** (desconectada sin circular agua). Sin embargo, la aplicación web SCADA y el datalogger registraron flujo de permeado continuo:
   - A 60 RPM del motor: el sensor seco registraba **15 a 20 Hz** (~25 a 35 mL/min).
   - A 90 RPM del motor: el sensor seco registraba **45 a 65 Hz** (~70 a 95 mL/min).
   - En una sesión de 28 minutos, acumuló más de **10.2 litros virtuales** en un sensor completamente seco. La frecuencia espuria aumentaba proporcionalmente a la velocidad del motor NEMA 34.
2. **Pérdida de Muestras de Bajas RPM en Datalogger:** El buffer en memoria RAM estaba limitado a 600 registros con muestreo cada 1 segundo (10 minutos de capacidad). En un ensayo continuo de 28 minutos sin detener la bomba, el buffer lineal desplazó las muestras antiguas con un `memmove`/loop FIFO, perdiéndose por completo los datos de 20 a 50 RPM y conservando únicamente los de 60 a 90 RPM a partir de $t = 1109\text{ s}$.
3. **Calibración Contaminada:** La recta de regresión lineal experimental para el caudalímetro de alimentación arrojó una **ordenada al origen de $+20.98\text{ Hz}$** ($F = 196.50 \cdot Q + 20.98$). Una turbina sin fluido no puede girar, lo que evidencia que la calibración absorbió el mismo ruido EMI del motor. Además, el factor $K$ nominal previo de permeado estaba en $687.33\text{ Hz/(L/min)}$, discrepando $3.5\times$ con el sensor gemelo de alimentación ($196.50\text{ Hz/(L/min)}$).
4. **Resonancia Sonora a 30–40 RPM:** A 30–40 RPM la bomba generaba un ruido mecánico/vibratorio estridente muy violento, que luego disminuía a 50–70 RPM y se volvía uniforme y suave a 80 RPM. Asimismo, el motor calentaba significativamente en operación sostenida.

### 2.2. Dictamen de la Auditoría Externa (Ronda 5):
La auditoría anterior dictaminó **RECHAZADO para certificar el cierre del Subhito 2.2**:
- Demostró matemáticamente que el filtro de software previo (que exigía $n \ge 2$ pulsos y aplicaba un corte de $f < 2.0\text{ Hz}$) **era totalmente inútil contra el fantasma medido**, ya que el ruido estaba entre **15 y 65 Hz** (muy por encima de 2 Hz).
- Demostró que el blanking de interrupción de $1500\text{ }\mu\text{s}$ ($f_{\text{corte}} \approx 666.7\text{ Hz}$) frente a la conmutación de pasos del motor (1067 a 4800 Hz) actuaba como un **submuestreador (aliasing)**, dejando pasar exactamente 1 pulso por ráfaga, generando una señal subarmónica sintética proporcional a los RPM.
- Demostró que el desplazamiento manual `bufferLog[i] = bufferLog[i+1]` consumía 38 KB de movimiento de memoria por segundo en el ESP32.
- Señaló que la turbina YF-S401 trabaja casi siempre por debajo de su rango lineal en ultrafiltración (permeado esperado en agua limpia: 20 a 100 mL/min vs umbral útil de la turbina: > 200 mL/min).

---

# 3. CONDICIÓN Y RESTRICCIÓN DE BANCO: HARDWARE CONGELADO

El operador de la planta (Ing. Enzo) implementó en la placa de acondicionamiento de hardware lo siguiente:
- Resistencia Pull-up externa de **4.7 kΩ** conectada entre la línea de señal y los 3.3V del ESP32.
- Capacitor cerámico de desacople **104 (100 nF)** entre señal y GND.
- **Un capacitor cerámico adicional** conectado en paralelo por seguridad frente a ruidos parásitos.
- La placa se encuentra montada y cableada en el gabinete de control.

> **RESTRICCIÓN CRÍTICA DE ESTA RONDA:**  
> Por limitaciones de tiempo, cronograma de entregas de tesis y disponibilidad de banco, **NO SE DESEA NI SE PUEDE MODIFICAR EL HARDWARE POR EL MOMENTO**.  
> El equipo busca determinar si mediante **arquitectura de firmware, procesamiento digital de señales (DSP), técnicas algorítmicas avanzadas o uso de periféricos integrados en silicio del ESP32 (como el PCNT)** es posible resolver el caudal fantasma, blindar la metrología y certificar el Subhito 2.2, o si existe una barrera física insalvable que obligue inexorablemente a una corrección de cableado.

---

# 4. MEJORAS DE FIRMWARE PROPUESTAS E IMPLEMENTADAS (V4.1)

A continuación se detallan las medidas de software desarrolladas para sortear el rechazo:

### 4.1. Algoritmo de Coherencia Temporal y Dispersión de Períodos (`caudalimetro.cpp`)
- **Premisa Física:** Una turbina mecánica impulsada por un fluido incompresible rota con inercia continua; los intervalos entre pulsos consecutivos en una ventana de 1 segundo son homogéneos ($dt_{\text{max}} / dt_{\text{min}} \le 4.0$).
- **Filtro Anti-Ráfaga EMI:** El ruido electromagnético inducido por conmutación del DM860 se agrupa en ráfagas densas de pulsos en pocos microsegundos/milisegundos seguidas de largos silencios. La ISR captura atómicamente `_dt_min_us` y `_dt_max_us`. Si $n \ge 4$ y $(dt_{\text{max}} / dt_{\text{min}}) > 4.0$, el firmware **identifica la ráfaga como ruido espurio y descarta la frecuencia a cero**.
- **Ocupación de Ventana Temporal:** Para $n$ pulsos a baja frecuencia, se valida que el lapso entre el primer y último pulso ocupe al menos $30\text{ ms}$ de la ventana.
- **Deadband Físico Dual:** Corte estricto de frecuencia a $0.0\text{ Hz}$ si $f < 2.5\text{ Hz}$ y corte de caudal a $0.0\text{ mL/min}$ si $q < 30.0\text{ mL/min}$ (límite inferior de cuantificación de la turbina).
- **Diagnóstico de Pulsos Brutos:** Se expone `pulsosBrutos()` en la telemetría Serial para verificar experimentalmente en banco si el sensor seco capta cualquier conteo.

### 4.2. Datalogger con Buffer Circular Indexado $O(1)$ (`EN_USO_firmware_planta.ino`)
- **Eliminación de la copia en RAM:** Se suprimió el desplazamiento de 38 KB por segundo, sustituyéndolo por un Buffer Circular estático con punteros `bufferHead` y `bufferCount`.
- **Discriminación de Régimen Permanente en CSV:** Se añadió el campo booleano `en_regimen` (`bomba.enRegimenEstable()`), exportado al archivo CSV como la columna `Estable_1_0`. Esto permite a los tesistas filtrar en Excel las muestras transitorias de rampa de aceleración ($3.5\text{ RPM/s}$) y procesar únicamente datos estacionarios.
- **Blindaje del Slider Web SCADA:** En `handleSetRPM()`, solo se segmenta automáticamente un nuevo ensayo si la consigna cambia en $\ge 1.5\text{ RPM}$ **Y** el ensayo previo tuvo al menos 5 segundos de duración.

### 4.3. Parada Rápida y Parada de Emergencia Física por Hardware (`Bomba.cpp`)
- Desaceleración de parada rápida aumentada a $70.0\text{ RPM/s}$ (garantiza detención completa desde 100 RPM en $1.43\text{ s} < 1.5\text{ s}$).
- Implementación de `paradaEmergencia()`: corta pulsos PWM a cero y polariza `PIN_ENA = HIGH` (GPIO 23) en el driver DM860, desenergizando físicamente el motor en $< 1\text{ ms}$.

### 4.4. Ley de Darcy y Validación Termodinámica (`darcy.h`)
- Ecuación de Vogel para viscosidad del agua acotada estrictamente a $5.0\text{ }^\circ\text{C} \le T \le 60.0\text{ }^\circ\text{C}$.
- Chequeo de no-finitos con `std::isfinite()` y renombrado de $R_{\text{torta}}$ a $R_{\text{adicional}}$ para rigor metodológico.

---

# 5. CÓDIGO FUENTE COMPLETO DEL SISTEMA (ACTUALIZADO Y COMPILADO)

El firmware compila limpiamente al 100% con `arduino-cli` (ESP32 core 3.x, `FlashMode=dio, FlashFreq=40`):
- Flash: 1,030,227 bytes (78%).
- RAM Global: 94,284 bytes (28%). Memoria libre para heap local: 233,396 bytes.

A continuación se entrega el código fuente completo e íntegro de todos los módulos:

### Archivo 1: `config.h`
```cpp
#pragma once
// ==============================================================================
// CONFIGURACIÓN TÉCNICA — PLANTA DE ULTRAFILTRACIÓN (UNSa)
// Parámetros Cinemáticos, Calibración y Asignación de Pines
// ==============================================================================

#include <Arduino.h>

// ------------------------------------------------------------------------------
// 1. ASIGNACIÓN DE PINES (PINOUT)
// ------------------------------------------------------------------------------
// Driver DM860 en CÁTODO COMÚN: GPIO envía HIGH -> Optoacoplador ON
constexpr uint8_t PIN_PUL                = 18;  // DM860 PUL+ (PUL- a GND común)
constexpr uint8_t PIN_DIR                = 19;  // DM860 DIR+ (DIR- a GND común)
constexpr uint8_t PIN_ENA                = 23;  // DM860 ENA+ (ENA- a GND: HIGH = Driver deshabilitado en parada de emergencia)
constexpr uint8_t PIN_LED_BOMBA          = 2;   // LED Azul Onboard (indicador de marcha)

// Caudalímetros de Efecto Hall YF-S401
constexpr uint8_t PIN_SENSOR_ALIMENTACION = 14;  // Sensor de Alimentación (Borne 12 Sup)
constexpr uint8_t PIN_SENSOR_PERMEADO     = 27;  // Sensor de Permeado (Borne 11 Sup, Pull-up externo a 3.3V)

// Puertos I2C para ADS1115 (Subhito 2.3 - Transductores de Presión)
constexpr uint8_t PIN_I2C_SDA             = 21;  // ESP32 SDA
constexpr uint8_t PIN_I2C_SCL             = 22;  // ESP32 SCL

// ------------------------------------------------------------------------------
// 2. PARÁMETROS CINEMÁTICOS DE LA BOMBA (MOTOR NEMA 34 / DM860)
// ------------------------------------------------------------------------------
constexpr uint16_t PULSOS_POR_REV = 3200;     // DM860 configurado a 1/16 micropasos (3200 pulsos/rev)
constexpr float ML_POR_VUELTA     = 13.6000f; // Calibrado nominal: 13.6000 mL/rev (~680 mL/min a 50 RPM)

// Rango de Operación en RPM
constexpr float RPM_MIN           = 15.0f;    // ~204 mL/min
constexpr float RPM_MAX           = 100.0f;   // ~1360 mL/min (Habilitado para diseño factorial experimental en agua)
constexpr float RPM_INICIO        = 25.0f;    // Consigna de arranque suave (~340 mL/min)
constexpr float RPM_ALARMA_MEMBRANA = 44.0f;  // 44.12 RPM = 600 mL/min (Umbral clínico hemodiálisis / referencia)

// ------------------------------------------------------------------------------
// 3. ESPECIFICACIONES DE LA MEMBRANA FRESENIUS FX100 (HELIXONE®)
// ------------------------------------------------------------------------------
constexpr float AREA_MEMBRANA_M2       = 2.2f;    // Superficie interfacial efectiva (m²)
constexpr float K_UF_NOMINAL           = 73.0f;   // mL / (h * mmHg)
constexpr float DIAMETRO_CAPILAR_UM    = 185.0f;  // Diámetro interno capilar (μm)
constexpr float ESPESOR_PARED_UM       = 35.0f;   // Grosor de pared capilar (μm)
constexpr float TMP_MAX_SEGURA_BAR     = 0.50f;   // Límite máximo admisible de TMP (bar) - Disparo de interbloqueo
constexpr float P1_MAX_SEGURA_BAR      = 0.60f;   // Límite máximo de presión en entrada de cartucho (bar)
constexpr float Q_CLINICO_SANGRE_MAX   = 600.0f;  // mL/min (límite en hemodiálisis clínica)

// Rampas de Aceleración y Frenado Conforme a Auditoría Ronda 5 (< 1.5s parada garantizada)
constexpr float ACEL_ARRANQUE_RPM_S  = 2.0f;   // 2.0 RPM/s despegue inicial suave y confiable
constexpr float ACEL_NOMINAL_RPM_S   = 3.5f;   // 3.5 RPM/s aceleración progresiva
constexpr float DESACEL_AJUSTE_RPM_S = 8.0f;   // 8.0 RPM/s desaceleración en marcha
constexpr float FRENADO_PARADA_RPM_S = 70.0f;  // 70.0 RPM/s frenado rápido garantizado (< 1.43s desde 100 RPM)

// ------------------------------------------------------------------------------
// 4. CALIBRACIÓN DE FÁBRICA DE CAUDALÍMETROS YF-S401
// ------------------------------------------------------------------------------
// UNIDAD METROLÓGICA DE K: [Hz / (L/min)]
// Q [mL/min] = (F [Hz] * 1000) / K
constexpr float K_ALIMENTACION = 196.50f; // Hz/(L/min) -> 11790 pulsos/L (meseta experimental a 60-80 RPM)
constexpr float K_PERMEADO     = 687.33f; // Hz/(L/min) -> valor nominal sensor permeado

constexpr uint32_t FILTRO_RUIDO_US = 1500;    // 1.5 ms de blanking anti-rebote (hasta 666 Hz / ~4300 mL/min)
constexpr float Q_MAX_FISICO_MLMIN = 2500.0f; // Límite físico de plausibilidad (2.5 L/min)

// ------------------------------------------------------------------------------
// 5. PARÁMETROS DEL DATALOGGER MULTI-SESIÓN Y AUTO-CALIBRACIÓN
// ------------------------------------------------------------------------------
constexpr uint32_t INTERVALO_LOG_MS   = 10000;  // Muestreo cada 10 segundos (600 muestras = 100 minutos continuos)
constexpr size_t   MAX_REGISTROS      = 600;    // 600 muestras = 1.66 horas de ensayo continuo sin sobreescritura
constexpr size_t   MAX_ENSAYOS        = 20;     // Hasta 20 corridas/ensayos registrados en memoria
constexpr uint8_t  MUESTRAS_AUTO_CAL  = 15;     // 15 muestras (15 seg) para auto-calibración en régimen permanente

// ------------------------------------------------------------------------------
// 6. CONFIGURACIÓN DE RED WI-FI Y SCADA
// ------------------------------------------------------------------------------
constexpr const char* SSID_AP  = "Bomba_Peristaltica_UF";
constexpr const char* PASS_AP  = "plantapiloto2";
constexpr const char* SSID_STA = "Box804";
constexpr const char* PASS_STA = "plantapiloto2";
```

### Archivo 2: `caudalimetro.h`
```cpp
#pragma once
#include <Arduino.h>
#include "config.h"

// ==============================================================================
// CLASE CAUDALÍMETRO (Sensor de Efecto Hall YF-S401)
// ==============================================================================

class Caudalimetro {
public:
  Caudalimetro(uint8_t pin, float k, const char* nombre, bool esAlimentacion = true);

  void begin();
  void actualizar(float dt_s, bool bombaEmpuja);

  float caudal_mLmin()   const { return _q; }
  float caudal_Lmin()    const { return _q / 1000.0f; }
  float frecuencia_Hz()  const { return _f; }
  float volumen_L()      const { return _vol; }
  bool  sinSenal()       const { return _fallo; }
  void  resetVolumen()         { _vol = 0.0f; }
  const char* nombre()   const { return _nombre; }
  bool  esAlimentacion() const { return _esAlimentacion; }
  uint32_t pulsosBrutos()const { return _pulsosBrutos; } // Diagnóstico de ráfagas EMI para prueba en seco

  // Calibración dinámica sin recompilar [Hz / (L/min)]
  void  setK(float nuevoK)     { if (nuevoK > 0.1f) _k = nuevoK; }
  float getK()           const { return _k; }

private:
  static void IRAM_ATTR isrPuente(void* arg);

  const uint8_t _pin;
  float         _k;
  const char*   _nombre;
  const bool    _esAlimentacion;

  volatile uint32_t _pulsos = 0;
  volatile uint32_t _t_ultimo = 0;
  volatile uint32_t _t_primero = 0;
  volatile uint32_t _dt_min_us = 0xFFFFFFFF;
  volatile uint32_t _dt_max_us = 0;

  float    _f = 0.0f;
  float    _q = 0.0f;
  float    _vol = 0.0f;
  float    _tiempoSinPulso_s = 0.0f;
  uint32_t _pulsosBrutos = 0;
  bool     _fallo = false;

  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
};
```

### Archivo 3: `caudalimetro.cpp`
```cpp
#include "caudalimetro.h"

Caudalimetro::Caudalimetro(uint8_t pin, float k, const char* nombre, bool esAlimentacion)
  : _pin(pin), _k(k), _nombre(nombre), _esAlimentacion(esAlimentacion) {}

void Caudalimetro::begin() {
  pinMode(_pin, INPUT_PULLUP);
  attachInterruptArg(digitalPinToInterrupt(_pin), isrPuente, this, FALLING);
}

void Caudalimetro::actualizar(float dt_s, bool bombaEmpuja) {
  // Captura atómica de variables acumuladas por la ISR en la ventana de 1 segundo
  portENTER_CRITICAL(&_mux);
  uint32_t n       = _pulsos;
  _pulsos          = 0;
  uint32_t t_prim  = _t_primero;
  uint32_t t_ult   = _t_ultimo;
  uint32_t dt_min  = _dt_min_us;
  uint32_t dt_max  = _dt_max_us;
  _dt_min_us       = 0xFFFFFFFF;
  _dt_max_us       = 0;
  portEXIT_CRITICAL(&_mux);

  _pulsosBrutos = n; // Almacenado para telemetría y prueba de permeado seco

  // 1. CÁLCULO DE FRECUENCIA Y FILTRADO DE COHERENCIA TEMPORAL (DICTAMEN AUDITORÍA RONDA 5):
  // Una turbina hidráulica arrastrada por líquido posee inercia mecánica y emite pulsos
  // distribuidos homogéneamente en el tiempo (relación dt_max / dt_min < 4.0).
  // Una perturbación electromagnética (EMI) del motor NEMA 34 genera ráfagas concentradas
  // (varios pulsos en pocos milisegundos y el resto del segundo vacío).
  bool pulsosCoherentes = false;
  float f_calculada = 0.0f;

  if (n >= 2 && (t_ult > t_prim)) {
    uint32_t ventanaPulsos_us = t_ult - t_prim;

    // Validación de coherencia de rotación si n >= 3:
    // Rechaza ráfagas espurias donde los pulsos ocurrieron apretados seguidos de un largo silencio
    bool dispersionPeriodoOk = (n < 4) || (dt_min > 0 && ((float)dt_max / (float)dt_min <= 4.0f));

    // Verificación de ocupación de ventana: para n pulsos a baja frecuencia (< 60 Hz),
    // los pulsos deben estar distribuidos en una fracción razonable de la ventana (> 30 ms).
    bool ventanaTemporalOk = (ventanaPulsos_us >= 30000UL) || (n <= 3);

    if (dispersionPeriodoOk && ventanaTemporalOk) {
      f_calculada = ((float)(n - 1) * 1000000.0f) / (float)ventanaPulsos_us;
      pulsosCoherentes = true;
    } else {
      Serial.printf("[%s] Ráfaga EMI descartada por dispersión: n=%lu, dt_min=%lu us, dt_max=%lu us, span=%lu us\n",
                    _nombre, (unsigned long)n, (unsigned long)dt_min, (unsigned long)dt_max, (unsigned long)ventanaPulsos_us);
    }
  }

  _f = pulsosCoherentes ? f_calculada : 0.0f;

  // 2. DEAD-BAND METROLÓGICO (Fricción estática de eje cerámico YF-S401):
  // La turbina no gira de forma continua y estable por debajo de ~2.5 Hz (< 20-30 mL/min).
  // Toda frecuencia < 2.5 Hz es truncada a cero absoluto para evitar acumulación de sesgo.
  if (_f < 2.5f) {
    _f = 0.0f;
  }

  // 3. CÁLCULO DE CAUDAL INSTANTÁNEO EN mL/min:
  float q = (_f * 1000.0f) / _k;

  // Umbral de caudal mínimo medible del sensor (límite de cuantificación YF-S401 ~30 mL/min)
  if (q < 30.0f) {
    q = 0.0f;
    _f = 0.0f;
  }

  // Filtro de plausibilidad física (corte de picos transitorios)
  if (q > Q_MAX_FISICO_MLMIN) {
    q = 0.0f;
    _f = 0.0f;
    Serial.printf("[%s] Ruido EMI descartado: %lu pulsos espurios\n", _nombre, (unsigned long)n);
  } else if (_f > 0.0f && q > 0.0f) {
    // 4. INTEGRACIÓN DE VOLUMEN TOTALIZADO EN LITROS:
    _vol += (float)n / (_k * 60.0f);
  }

  // 5. FILTRO EXPONENCIAL PONDERADO (EMA) SINTONIZADO PARA FLUJO PERISTÁLTICO:
  // Suaviza la pulsación rodillo a rodillo del cabezal peristáltico (tau ≈ 4.5 s con dt=1s, alfa=0.20)
  if (q > 0.0f) {
    if (_q == 0.0f) {
      _q = q; // Respuesta ágil desde reposo
    } else {
      _q = 0.20f * q + 0.80f * _q; // Filtrado estable
    }
  } else {
    // Decaimiento rápido a cero al detenerse el flujo
    _q = 0.50f * _q;
    if (_q < 1.0f) _q = 0.0f;
  }

  // 6. DIAGNÓSTICO ASIMÉTRICO DE PÉRDIDA DE SEÑAL / CABLE CORTADO:
  if (n > 0 || _f > 0.05f) {
    _tiempoSinPulso_s = 0.0f;
    _fallo = false;
  } else if (bombaEmpuja && _esAlimentacion) {
    _tiempoSinPulso_s += dt_s;
    if (_tiempoSinPulso_s >= 5.0f) _fallo = true;
  } else {
    _tiempoSinPulso_s = 0.0f;
    _fallo = false;
  }
}

void IRAM_ATTR Caudalimetro::isrPuente(void* arg) {
  Caudalimetro* c = reinterpret_cast<Caudalimetro*>(arg);
  uint32_t t = micros();

  portENTER_CRITICAL_ISR(&c->_mux);
  uint32_t dt = t - c->_t_ultimo;

  // Blanking anti-rebote: descarta transitorios mecánicos y picos rápidos (< 1500 us)
  if (dt >= FILTRO_RUIDO_US) {
    if (c->_pulsos == 0) {
      c->_t_primero = t;
    } else {
      if (dt < c->_dt_min_us) c->_dt_min_us = dt;
      if (dt > c->_dt_max_us) c->_dt_max_us = dt;
    }
    c->_t_ultimo = t;
    c->_pulsos++;
  }
  portEXIT_CRITICAL_ISR(&c->_mux);
}
```

### Archivo 4: `Bomba.h`
```cpp
#pragma once
#include <Arduino.h>
#include <cmath>
#include "config.h"

// ==============================================================================
// CLASE BOMBA PERISTÁLTICA — DECLARACIÓN
// ==============================================================================

class Bomba {
public:
  void begin();
  void arrancar();
  void detener();
  void paradaEmergencia();       // Corte instantáneo físico y enclavado (< 1 ms, sin rampa)
  void rearmarEmergencia();      // Rearme manual tras condición segura
  bool setRPM(float rpm);
  void toggleSentido();

  float rpmActual() const           { return _actual; }
  float rpmObjetivo() const         { return _objetivo; }
  bool  enMarcha() const            { return _enMarcha; }
  bool  invirtiendo() const         { return _invirtiendo; }
  bool  sentidoHorario() const      { return _horario; }
  bool  enEmergencia() const        { return _enEmergencia; }
  bool  enRegimenEstable() const    { return (_enMarcha && !_enEmergencia && fabsf(_actual - _objetivo) < 0.3f && _actual > 5.0f); }
  float caudalTeorico_mLmin() const { return _actual * _mlPorVuelta; }

  void  setMlPorVuelta(float ml)    { if (ml > 0.1f) _mlPorVuelta = ml; }
  float getMlPorVuelta() const      { return _mlPorVuelta; }

  void     setPulsosPorRev(uint16_t p) { if (p >= 200) _pulsosPorRev = p; }
  uint16_t getPulsosPorRev() const     { return _pulsosPorRev; }

  void tick(float dt);

private:
  void fijarSentido(bool horario);

  bool _enMarcha = false;
  bool _horario = true;
  bool _invirtiendo = false;
  bool _enEmergencia = false;

  float _objetivo = RPM_INICIO;
  float _actual = 0.0f;
  float _rpmGuardada = RPM_INICIO;
  uint32_t _fActual = 0;

  float    _mlPorVuelta  = ML_POR_VUELTA;
  uint16_t _pulsosPorRev = PULSOS_POR_REV;
};
```

### Archivo 5: `Bomba.cpp`
```cpp
#include "Bomba.h"

void Bomba::begin() {
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_ENA, OUTPUT);
  digitalWrite(PIN_ENA, LOW); // ENA en LOW: Driver DM860 habilitado normalmente
  fijarSentido(true); // Sentido horario por defecto

#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
  ledcAttach(PIN_PUL, 800, 10);
  ledcWrite(PIN_PUL, 0); // Reposo: LOW -> Opto OFF (motor libre)
#else
  ledcSetup(0, 800, 10);
  ledcAttachPin(PIN_PUL, 0);
  ledcWrite(0, 0);
#endif
}

void Bomba::arrancar() {
  if (_enEmergencia) return; // Bloqueado si hay alarma de sobrepresión/emergencia activa
  _enMarcha = true;
  digitalWrite(PIN_ENA, LOW); // Asegura habilitación
  if (_objetivo < RPM_MIN) {
    _objetivo = (_rpmGuardada >= RPM_MIN) ? _rpmGuardada : RPM_INICIO;
  }
}

void Bomba::detener() {
  _enMarcha = false;
  _invirtiendo = false;
  if (_objetivo >= RPM_MIN) {
    _rpmGuardada = _objetivo;
  }
}

void Bomba::paradaEmergencia() {
  _enEmergencia = true;
  _enMarcha = false;
  _invirtiendo = false;
  _actual = 0.0f;
  _objetivo = 0.0f;
  _fActual = 0;
#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
  ledcWrite(PIN_PUL, 0);
#else
  ledcWrite(0, 0);
#endif
  digitalWrite(PIN_ENA, HIGH); // Corte físico instantáneo en el DM860 (< 1 ms)
}

void Bomba::rearmarEmergencia() {
  _enEmergencia = false;
  digitalWrite(PIN_ENA, LOW);
  _actual = 0.0f;
  _objetivo = (_rpmGuardada >= RPM_MIN) ? _rpmGuardada : RPM_INICIO;
}

bool Bomba::setRPM(float rpm) {
  if (!std::isfinite(rpm) || _enEmergencia) return false;
  float r = constrain(rpm, RPM_MIN, RPM_MAX);
  if (_invirtiendo) {
    _rpmGuardada = r;   // Almacena consigna si el usuario mueve slider durante el frenado de inversión
  } else {
    _objetivo = r;
    _rpmGuardada = r;
  }
  return true;
}

void Bomba::toggleSentido() {
  if (_invirtiendo || _enEmergencia) return;
  if (_actual < 1.0f) {
    fijarSentido(!_horario);
  } else {
    _invirtiendo = true;
    _rpmGuardada = (_objetivo >= RPM_MIN) ? _objetivo : RPM_INICIO;
    _objetivo = 0.0f;
  }
}

void Bomba::fijarSentido(bool horario) {
  _horario = horario;
  digitalWrite(PIN_DIR, horario ? LOW : HIGH);
}

void Bomba::tick(float dt) {
  if (_enEmergencia) {
    _actual = 0.0f;
    _fActual = 0;
#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    ledcWrite(PIN_PUL, 0);
#else
    ledcWrite(0, 0);
#endif
    digitalWrite(PIN_ENA, HIGH);
    return;
  }

  float objetivo = _enMarcha ? _objetivo : 0.0f;

  if (_actual < objetivo) {
    // Arranque suave
    if (_actual < 1.0f) {
      _actual = 1.0f;
    }
    
    float tasaAcel = ACEL_NOMINAL_RPM_S;
    if (_actual < 10.0f) {
      tasaAcel = ACEL_ARRANQUE_RPM_S + (_actual / 10.0f) * (ACEL_NOMINAL_RPM_S - ACEL_ARRANQUE_RPM_S);
    }
    float delta = objetivo - _actual;
    if (delta < 3.0f) {
      tasaAcel = fmaxf(0.8f, tasaAcel * (delta / 3.0f));
    }
    _actual = fminf(objetivo, _actual + tasaAcel * dt);

  } else if (_actual > objetivo) {
    // Desaceleración / Frenado
    if (!_enMarcha) {
      // PARADA RÁPIDA: Frenado ágil en menos de 1.5s (70 RPM/s)
      _actual = fmaxf(0.0f, _actual - FRENADO_PARADA_RPM_S * dt);
      if (_actual < 1.0f) {
        _actual = 0.0f;
      }
    } else {
      // Reducción suave de velocidad en marcha
      float tasaDecel = DESACEL_AJUSTE_RPM_S;
      float delta = _actual - objetivo;
      if (delta < 3.0f) {
        tasaDecel = fmaxf(1.0f, tasaDecel * (delta / 3.0f));
      }
      _actual = fmaxf(objetivo, _actual - tasaDecel * dt);
    }
  }

  // Al llegar a 0 RPM durante una inversión
  if (_invirtiendo && _actual <= 0.1f) {
#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    ledcWrite(PIN_PUL, 0);
#else
    ledcWrite(0, 0);
#endif
    _fActual = 0;
    fijarSentido(!_horario);
    _invirtiendo = false;
    _objetivo = (_rpmGuardada >= RPM_MIN) ? _rpmGuardada : RPM_INICIO;
  }

  // Generación de pulsos PWM por hardware LEDC
  if (_actual >= 1.0f) {
    uint32_t f = (uint32_t)(_actual * (float)_pulsosPorRev / 60.0f);
    if (f < 50) f = 50; // Límite de seguridad mínimo del temporizador LEDC de ESP32

#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    if (f != _fActual) { 
      ledcChangeFrequency(PIN_PUL, f, 10); 
      _fActual = f; 
    }
    ledcWrite(PIN_PUL, 512); // Ciclo de trabajo 50%
#else
    if (f != _fActual) { 
      ledcSetup(0, f, 10); 
      _fActual = f; 
    }
    ledcWrite(0, 512);
#endif
  } else if (_fActual != 0) {
    // Corte inmediato de pulsos al detenerse
#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    ledcWrite(PIN_PUL, 0); // Reposo: LOW -> Opto OFF
#else
    ledcWrite(0, 0);
#endif
    _fActual = 0;
  }
}
```

### Archivo 6: `darcy.h`
```cpp
#pragma once
#include <Arduino.h>
#include <cmath>
#include "config.h"

struct ResultadoDarcy {
  float J_LMH;       // Flujo de permeado volumétrico específico [L / (m² · h)]
  float J20_LMH;     // Flujo normalizado a 20 °C [LMH]
  float mu_Pas;      // Viscosidad dinámica del agua a temperatura T [Pa · s]
  float TCF;         // Factor de corrección por temperatura (mu(T) / mu_20)
  float R_total;     // Resistencia hidráulica total [m^-1]
  float R_adicional; // Resistencia hidráulica adicional aparente (torta + polarización) [m^-1]
  float TMP_bar;     // Presión transmembrana efectiva [bar]
  bool  valido;      // Estado de cálculo válido
};

class ModeloDarcy {
public:
  static constexpr float MU20 = 1.002e-3f; // Pa · s (Agua destilada a 20.0 °C)

  // Viscosidad dinámica del agua mediante ecuación de Vogel (válida 5 a 60 °C)
  static float viscosidadAgua(float temp_C) {
    float t_segura = constrain(temp_C, 5.0f, 60.0f);
    const float T_K = t_segura + 273.15f;
    return 2.414e-5f * powf(10.0f, 247.8f / (T_K - 140.0f));
  }

  void  setRm(float rm) { if (std::isfinite(rm) && rm > 1e11f && rm < 1e16f) _Rm = rm; }
  float Rm() const      { return _Rm; }

  // Cálculo hidráulico en tiempo real
  ResultadoDarcy calcular(float qPerm_mLmin, float tmp_bar, float temp_C = 20.0f) const {
    ResultadoDarcy r{};
    if (!std::isfinite(tmp_bar) || !std::isfinite(temp_C) || !std::isfinite(qPerm_mLmin)) {
      return r;
    }
    if (tmp_bar < 0.01f || qPerm_mLmin < 0.5f) {
      return r;
    }

    float t_val = constrain(temp_C, 5.0f, 60.0f);
    r.TMP_bar = tmp_bar;
    r.mu_Pas  = viscosidadAgua(t_val);
    r.TCF     = r.mu_Pas / MU20;

    // J [LMH] = Q [L/h] / Area [m²] = (qPerm [mL/min] * 0.06) / 2.2 m²
    r.J_LMH   = (qPerm_mLmin * 0.06f) / AREA_MEMBRANA_M2;
    r.J20_LMH = r.J_LMH * r.TCF;

    // J en unidades SI [m/s]: 1 LMH = 1 / 3.6e6 m/s
    const float J_SI   = r.J_LMH / 3.6e6f;
    const float TMP_Pa = tmp_bar * 100000.0f;

    // Ley de Darcy: J = TMP / (mu * R_total) => R_total = TMP / (mu * J)
    if (J_SI > 1e-12f && r.mu_Pas > 1e-6f) {
      r.R_total     = TMP_Pa / (r.mu_Pas * J_SI);
      r.R_adicional = r.R_total - _Rm;
      r.valido      = std::isfinite(r.R_total) && std::isfinite(r.R_adicional);
    }

    return r;
  }

  // Calibración experimental de Rm con agua limpia
  void acumularPuntoAguaLimpia(float J_SI, float TMP_Pa) {
    _sumJP += J_SI * TMP_Pa;
    _sumPP += TMP_Pa * TMP_Pa;
  }

  bool finalizarCalibracionRm(float temp_C = 20.0f) {
    if (_sumPP <= 0.0f || _sumJP <= 0.0f) return false;
    const float Lp = _sumJP / _sumPP;
    const float mu = viscosidadAgua(temp_C);
    _Rm = 1.0f / (mu * Lp);
    _sumJP = 0.0f;
    _sumPP = 0.0f;
    return true;
  }

private:
  float _Rm = 2.09e13f; // Nominal FX100 Helixone
  float _sumJP = 0.0f;
  float _sumPP = 0.0f;
};
```

### Archivo 7: `EN_USO_firmware_planta.ino`
```cpp
/* ==============================================================================
 * PLANTA PILOTO DE ULTRAFILTRACIÓN — TESIS INGENIERÍA INDUSTRIAL (UNSa 2026)
 * Firmware de Control, Adquisición, Modo Desarrollador, Auto-Calibración y Datalogger
 * Arquitectura C++ Optimizada y Wi-Fi SoftAP de Alta Estabilidad (Anti-Desconexión)
 * ============================================================================== */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "config.h"
#include "caudalimetro.h"
#include "Bomba.h"
#include "darcy.h"
#include "index_html.h"

// Servidor DNS para Portal Cautivo Anti-Desconexión en Android / iOS / Windows
DNSServer dnsServer;
constexpr uint16_t DNS_PORT = 53;

// ------------------------------------------------------------------------------
// ESTRUCTURAS DE DATOS PARA EL DATALOGGER Y GESTIÓN DE ENSAYOS
// ------------------------------------------------------------------------------
struct RegistroCalibracion {
  uint16_t id_ensayo;      // 16 bits: previene desbordamiento
  uint32_t t_relativo_s;
  float    rpm;
  bool     en_regimen;     // Distingue régimen permanente vs transitorio dinámico de rampa
  float    q_bomba;
  float    f_alim;
  float    q_alim;
  float    vol_alim;
  float    f_perm;
  float    q_perm;
  float    vol_perm;
  float    q_ret;
  float    recov;
  float    delta;
  float    k_alim;
  float    k_perm;
  float    j_lmh;
  float    tmp_bar;        // Preparado para integración Subhito 2.3
};

struct EnsayoInfo {
  uint16_t id;
  float    rpm_consigna;
  uint32_t t_inicio_ms;
  uint32_t duracion_s;
  uint16_t muestras;
  float    vol_alim;
  float    vol_perm;
};

// Buffers de almacenamiento en RAM con Buffer Circular (Cero desplazamiento de memoria / O(1))
RegistroCalibracion bufferLog[MAX_REGISTROS];
size_t bufferHead = 0;   // Índice circular de inserción
size_t bufferCount = 0;  // Cantidad de muestras almacenadas (0 .. MAX_REGISTROS)

EnsayoInfo listaEnsayos[MAX_ENSAYOS];
size_t numEnsayos = 0;
uint16_t ensayoActualId = 1;

// Variables de estado del ensayo actual
uint32_t tInicioEnsayo_ms = 0;
float volAlimInicioEnsayo = 0.0f;
float volPermInicioEnsayo = 0.0f;
bool  bombaEnMarchaAnterior = false;

// Variables de Auto-Calibración en Marcha
bool    autoCalibrando = false;
uint8_t autoCalMuestras = 0;
float   autoCalSumFrecAlim = 0.0f;
float   autoCalSumFrecPerm = 0.0f;
String  autoCalMensaje = "";

// Instanciación de componentes
Caudalimetro sensorAlimentacion(PIN_SENSOR_ALIMENTACION, K_ALIMENTACION, "ALIMENTACION", true);
Caudalimetro sensorPermeado(PIN_SENSOR_PERMEADO, K_PERMEADO, "PERMEADO", false);
Bomba bomba;
WebServer server(80);
Preferences prefs;

// Variables hidráulicas y de control
float qRet_mLmin = 0.0f;
float recuperacion = 0.0f;
float deltaBomba = 0.0f;
float jLMH_actual = 0.0f;
bool  flagCruceSensores = false;

ModeloDarcy modeloDarcy;
ResultadoDarcy resultadoDarcy;

uint32_t tLoop = 0;
uint32_t tCaudal = 0;
uint32_t tDatalogger = 0;

// ------------------------------------------------------------------------------
// GESTIÓN DE PREFERENCES (MEMORIA FLASH NVS)
// ------------------------------------------------------------------------------
void cargarParametrosNVS() {
  prefs.begin("planta_uf", false);
  float ka = prefs.getFloat("k_alim", K_ALIMENTACION);
  float kp = prefs.getFloat("k_perm", K_PERMEADO);
  float ml = prefs.getFloat("ml_rev", ML_POR_VUELTA);
  uint16_t pul = prefs.getUShort("pul_rev", PULSOS_POR_REV);

  sensorAlimentacion.setK(ka);
  sensorPermeado.setK(kp);
  bomba.setMlPorVuelta(ml);
  bomba.setPulsosPorRev(pul);

  Serial.printf("\n[NVS] Parametros cargados: K_Alim=%.2f | K_Perm=%.2f | mL/rev=%.4f | Pul/Rev=%u\n",
                ka, kp, ml, pul);
}

void guardarParametrosNVS(float ka, float kp, float ml, uint16_t pul) {
  prefs.putFloat("k_alim", ka);
  prefs.putFloat("k_perm", kp);
  prefs.putFloat("ml_rev", ml);
  prefs.putUShort("pul_rev", pul);
  Serial.println("[NVS] Parametros guardados en memoria Flash con exito.");
}

// ------------------------------------------------------------------------------
// FUNCIÓN PARA GUARDAR MUESTRA EN EL DATALOGGER (CADA 10 SEGUNDOS)
// Inserción en Buffer Circular Indexado (O(1) - Cero fragmentación / Cero copia)
// ------------------------------------------------------------------------------
void guardarMuestraDatalogger() {
  if (tInicioEnsayo_ms == 0) {
    tInicioEnsayo_ms = millis();
  }

  uint32_t t_rel_s = (millis() - tInicioEnsayo_ms) / 1000;

  RegistroCalibracion reg;
  reg.id_ensayo    = ensayoActualId;
  reg.t_relativo_s = t_rel_s;
  reg.rpm          = bomba.rpmActual();
  reg.en_regimen   = bomba.enRegimenEstable(); // Filtro clave para análisis experimental
  reg.q_bomba      = bomba.caudalTeorico_mLmin();
  reg.f_alim       = sensorAlimentacion.frecuencia_Hz();
  reg.q_alim       = sensorAlimentacion.caudal_mLmin();
  reg.vol_alim     = sensorAlimentacion.volumen_L();
  reg.f_perm       = sensorPermeado.frecuencia_Hz();
  reg.q_perm       = sensorPermeado.caudal_mLmin();
  reg.vol_perm     = sensorPermeado.volumen_L();
  reg.q_ret        = qRet_mLmin;
  reg.recov        = recuperacion;
  reg.delta        = deltaBomba;
  reg.k_alim       = sensorAlimentacion.getK();
  reg.k_perm       = sensorPermeado.getK();
  reg.j_lmh        = (reg.q_perm * 0.06f) / AREA_MEMBRANA_M2;
  reg.tmp_bar      = resultadoDarcy.valido ? resultadoDarcy.TMP_bar : 0.0f;

  // Inserción O(1) en Buffer Circular Indexado
  bufferLog[bufferHead] = reg;
  bufferHead = (bufferHead + 1) % MAX_REGISTROS;
  if (bufferCount < MAX_REGISTROS) {
    bufferCount++;
  }

  Serial.printf("[LOG #%u][Ensayo %u] t=%us | RPM=%.1f | %s | Q_Alim=%.1f mL/min | Q_Perm=%.1f mL/min | J=%.2f LMH | Y=%.1f%%\n",
                (unsigned int)bufferCount, ensayoActualId, t_rel_s, reg.rpm,
                reg.en_regimen ? "ESTABLE" : "RAMPA",
                reg.q_alim, reg.q_perm, reg.j_lmh, reg.recov);
}

// ------------------------------------------------------------------------------
// GESTIÓN AUTOMÁTICA DE SESIONES Y ENSAYOS
// ------------------------------------------------------------------------------
void finalizarEnsayoActual() {
  uint32_t duracion_s = (tInicioEnsayo_ms > 0) ? ((millis() - tInicioEnsayo_ms) / 1000) : 0;
  uint16_t muestrasEnsayo = 0;
  for (size_t i = 0; i < bufferCount; i++) {
    if (bufferLog[i].id_ensayo == ensayoActualId) muestrasEnsayo++;
  }

  // Si no hubo muestras registradas en este ensayo pero duró más de 5s, registrar una de cierre
  if (muestrasEnsayo == 0 && duracion_s >= 5) {
    guardarMuestraDatalogger();
    muestrasEnsayo = 1;
  }

  if (muestrasEnsayo > 0) {
    // Si la lista de resúmenes de ensayos está llena, rotar el más antiguo (FIFO)
    if (numEnsayos >= MAX_ENSAYOS) {
      for (size_t i = 0; i < MAX_ENSAYOS - 1; i++) {
        listaEnsayos[i] = listaEnsayos[i + 1];
      }
      numEnsayos = MAX_ENSAYOS - 1;
    }

    listaEnsayos[numEnsayos].id           = ensayoActualId;
    listaEnsayos[numEnsayos].rpm_consigna = bomba.rpmObjetivo();
    listaEnsayos[numEnsayos].t_inicio_ms  = tInicioEnsayo_ms;
    listaEnsayos[numEnsayos].duracion_s   = duracion_s;
    listaEnsayos[numEnsayos].muestras     = muestrasEnsayo;
    listaEnsayos[numEnsayos].vol_alim     = sensorAlimentacion.volumen_L() - volAlimInicioEnsayo;
    listaEnsayos[numEnsayos].vol_perm     = sensorPermeado.volumen_L() - volPermInicioEnsayo;
    numEnsayos++;

    Serial.printf("\n<<< [ENSAYO #%u FINALIZADO Y REGISTRADO] Consigna: %.1f RPM | Duracion: %us | Muestras: %u | Vol Perm: %.3f L >>>\n\n",
                  ensayoActualId, bomba.rpmObjetivo(), duracion_s, muestrasEnsayo, sensorPermeado.volumen_L() - volPermInicioEnsayo);
  }
}

void iniciarNuevoEnsayo(float consignaRpm) {
  uint16_t muestrasPrevias = 0;
  for (size_t i = 0; i < bufferCount; i++) {
    if (bufferLog[i].id_ensayo == ensayoActualId) muestrasPrevias++;
  }
  if (muestrasPrevias > 0) {
    ensayoActualId++;
  }
  tInicioEnsayo_ms = millis();
  volAlimInicioEnsayo = sensorAlimentacion.volumen_L();
  volPermInicioEnsayo = sensorPermeado.volumen_L();
  tDatalogger = millis();
  guardarMuestraDatalogger(); // Muestra inicial garantizada en t = 0s
  Serial.printf("\n>>> [ENSAYO #%u INICIADO] Consigna: %.1f RPM <<<\n", ensayoActualId, consignaRpm);
}

// ------------------------------------------------------------------------------
// MANEJADORES DE RUTAS DEL SERVIDOR WEB
// ------------------------------------------------------------------------------
void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleStatus() {
  uint32_t t_act_s = (tInicioEnsayo_ms > 0) ? ((millis() - tInicioEnsayo_ms) / 1000) : 0;
  uint8_t prog = (uint8_t)((autoCalMuestras * 100) / MUESTRAS_AUTO_CAL);
  if (prog > 100) prog = 100;

  float qAlim = sensorAlimentacion.caudal_mLmin();
  float qPerm = sensorPermeado.caudal_mLmin();
  jLMH_actual = (qPerm * 0.06f) / AREA_MEMBRANA_M2;

  char buf[800];
  int n = snprintf(buf, sizeof(buf),
    "{"
    "\"rpm\":%.1f,\"obj_rpm\":%.1f,\"on\":%s,\"inv\":%s,\"dir\":%s,\"en_regimen\":%s,\"emergencia\":%s,"
    "\"ip\":\"%s\",\"alim_ok\":%s,\"perm_ok\":%s,\"f_alim\":%.2f,\"q_alim\":%.1f,\"vol_alim\":%.4f,"
    "\"f_perm\":%.2f,\"q_perm\":%.1f,\"vol_perm\":%.4f,\"q_ret\":%.1f,\"recov\":%.2f,"
    "\"pulsos_alim\":%lu,\"pulsos_perm\":%lu,"
    "\"pump_ml\":%.1f,\"delta\":%.2f,\"j_lmh\":%.2f,\"cruce\":%s,"
    "\"k_alim\":%.2f,\"k_perm\":%.2f,\"ml_rev\":%.4f,\"pul_rev\":%u,"
    "\"auto_cal\":%s,\"auto_cal_prog\":%u,\"auto_cal_res\":\"%s\","
    "\"n_logs\":%u,\"ensayo_act\":%u,\"t_ensayo_s\":%lu,"
    "\"heap\":%u,\"maxblk\":%u,"
    "\"ensayos\":[",
    bomba.rpmActual(), bomba.rpmObjetivo(),
    bomba.enMarcha() ? "true" : "false",
    bomba.invirtiendo() ? "true" : "false",
    bomba.sentidoHorario() ? "true" : "false",
    bomba.enRegimenEstable() ? "true" : "false",
    bomba.enEmergencia() ? "true" : "false",
    WiFi.softAPIP().toString().c_str(),
    !sensorAlimentacion.sinSenal() ? "true" : "false",
    !sensorPermeado.sinSenal() ? "true" : "false",
    sensorAlimentacion.frecuencia_Hz(), qAlim, sensorAlimentacion.volumen_L(),
    sensorPermeado.frecuencia_Hz(), qPerm, sensorPermeado.volumen_L(),
    qRet_mLmin, recuperacion,
    (unsigned long)sensorAlimentacion.pulsosBrutos(), (unsigned long)sensorPermeado.pulsosBrutos(),
    bomba.caudalTeorico_mLmin(), deltaBomba, jLMH_actual, flagCruceSensores ? "true" : "false",
    sensorAlimentacion.getK(), sensorPermeado.getK(),
    bomba.getMlPorVuelta(), bomba.getPulsosPorRev(),
    autoCalibrando ? "true" : "false", prog, autoCalMensaje.c_str(),
    (unsigned)bufferCount, (unsigned)ensayoActualId, (unsigned long)t_act_s,
    ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)
  );

  if (n >= (int)sizeof(buf)) {
    n = sizeof(buf) - 1;
    Serial.println("⚠️ [WARN] handleStatus buf truncado");
  }

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  server.sendContent(buf, n);

  char item[128];
  for (size_t i = 0; i < numEnsayos; i++) {
    int itemLen = snprintf(item, sizeof(item),
      "%s{\"id\":%u,\"rpm\":%.1f,\"duracion_s\":%lu,\"muestras\":%u,\"vol_alim\":%.4f,\"vol_perm\":%.4f}",
      (i > 0) ? "," : "",
      listaEnsayos[i].id, listaEnsayos[i].rpm_consigna,
      (unsigned long)listaEnsayos[i].duracion_s,
      listaEnsayos[i].muestras,
      listaEnsayos[i].vol_alim, listaEnsayos[i].vol_perm
    );
    if (itemLen >= (int)sizeof(item)) itemLen = sizeof(item) - 1;
    server.sendContent(item, itemLen);
  }

  server.sendContent("]}");
  server.sendContent(""); // Cierra el streaming chunked
}

void handleCmd() {
  if (!server.hasArg("act")) {
    server.send(400, "text/plain", "Falta argumento act");
    return;
  }
  String act = server.arg("act");
  if (act == "START") {
    bomba.arrancar();
  } else if (act == "STOP") {
    bomba.detener();
    if (autoCalibrando) {
      autoCalibrando = false;
      autoCalMensaje = "Auto-calibracion cancelada al apagar bomba.";
    }
  } else if (act == "EMERGENCY") {
    bomba.paradaEmergencia();
  } else if (act == "REARM") {
    bomba.rearmarEmergencia();
  } else if (act == "DIR") {
    bomba.toggleSentido();
  } else if (act == "RESET_VOL") {
    sensorAlimentacion.resetVolumen();
    sensorPermeado.resetVolumen();
    volAlimInicioEnsayo = 0.0f;
    volPermInicioEnsayo = 0.0f;
  }
  server.send(200, "text/plain", "OK");
}

void handleSetRPM() {
  if (server.hasArg("rpm")) {
    float nuevoRpm = server.arg("rpm").toFloat();
    if (!std::isfinite(nuevoRpm) || nuevoRpm < RPM_MIN || nuevoRpm > RPM_MAX) {
      server.send(400, "text/plain", "ERROR: RPM fuera de rango o invalido");
      return;
    }

    float rpmActualConsigna = bomba.rpmObjetivo();

    // Protección anti-saturación de ensayos (Auditoría Ronda 5):
    // Solo segmentamos si el cambio es sustancial (>= 1.5 RPM) Y el ensayo actual ya tuvo
    // tiempo de registrar datos (> 5 segundos). Si el operador cambia antes, solo ajusta la consigna.
    if (bomba.enMarcha() && fabsf(nuevoRpm - rpmActualConsigna) >= 1.5f) {
      uint32_t duracionActual = (tInicioEnsayo_ms > 0) ? ((millis() - tInicioEnsayo_ms) / 1000) : 0;
      if (duracionActual >= 5) {
        finalizarEnsayoActual();
        if (bomba.setRPM(nuevoRpm)) {
          iniciarNuevoEnsayo(nuevoRpm);
          server.send(200, "text/plain", "OK");
          return;
        }
      }
    }

    if (bomba.setRPM(nuevoRpm)) {
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "ERROR al ajustar RPM");
    }
  } else {
    server.send(400, "text/plain", "Falta argumento rpm");
  }
}

// Configuración en Modo Desarrollador
void handleSetDev() {
  if (server.hasArg("ka")) {
    float ka = server.arg("ka").toFloat();
    sensorAlimentacion.setK(ka);
  }
  if (server.hasArg("kp")) {
    float kp = server.arg("kp").toFloat();
    sensorPermeado.setK(kp);
  }
  if (server.hasArg("ml")) {
    float ml = server.arg("ml").toFloat();
    bomba.setMlPorVuelta(ml);
  }
  if (server.hasArg("pul")) {
    uint16_t pul = (uint16_t)server.arg("pul").toInt();
    bomba.setPulsosPorRev(pul);
  }

  bool saveNvs = (server.hasArg("save") && server.arg("save") == "1");
  if (saveNvs) {
    guardarParametrosNVS(sensorAlimentacion.getK(), sensorPermeado.getK(),
                         bomba.getMlPorVuelta(), bomba.getPulsosPorRev());
  }

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// Iniciar Auto-Calibración Inteligente en Régimen Permanente
void handleIniciarAutoCal() {
  if (!bomba.enMarcha()) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Enciende la bomba primero\"}");
    return;
  }
  autoCalibrando = true;
  autoCalMuestras = 0;
  autoCalSumFrecAlim = 0.0f;
  autoCalSumFrecPerm = 0.0f;
  autoCalMensaje = "";
  server.send(200, "application/json", "{\"status\":\"iniciada\"}");
}

void handleCancelarAutoCal() {
  autoCalibrando = false;
  autoCalMensaje = "Auto-calibracion cancelada.";
  server.send(200, "application/json", "{\"status\":\"cancelada\"}");
}

// Calibrador Completo por RPM y Caudal de Probeta
void handleCalibrarRpmQ() {
  if (!server.hasArg("rpm") || !server.hasArg("qa")) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Faltan argumentos (requiere rpm y qa)\"}");
    return;
  }

  float rpm = server.arg("rpm").toFloat();
  float qa  = server.arg("qa").toFloat();
  float qp  = server.hasArg("qp") ? server.arg("qp").toFloat() : 0.0f;

  if (rpm <= 0.0f || qa <= 0.0f) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"RPM y Caudal de Alimentacion deben ser > 0\"}");
    return;
  }

  // 1. Cilindrada real de la bomba
  float nuevaCilindrada = qa / rpm;
  bomba.setMlPorVuelta(nuevaCilindrada);

  // 2. Factores K basados en frecuencia actual
  float fa = sensorAlimentacion.frecuencia_Hz();
  float fp = sensorPermeado.frecuencia_Hz();

  if (fa > 1.0f) {
    float nuevoKa = (fa * 1000.0f) / qa;
    sensorAlimentacion.setK(nuevoKa);
  }
  if (qp > 0.0f && fp > 0.3f) {
    float nuevoKp = (fp * 1000.0f) / qp;
    sensorPermeado.setK(nuevoKp);
  }

  guardarParametrosNVS(sensorAlimentacion.getK(), sensorPermeado.getK(),
                       bomba.getMlPorVuelta(), bomba.getPulsosPorRev());

  String json = "{";
  json += "\"status\":\"ok\",";
  json += "\"ml_rev\":" + String(bomba.getMlPorVuelta(), 4) + ",";
  json += "\"k_alim\":" + String(sensorAlimentacion.getK(), 2) + ",";
  json += "\"k_perm\":" + String(sensorPermeado.getK(), 2);
  json += "}";

  server.send(200, "application/json", json);
}

// Restablecer parámetros de fábrica
void handleResetDev() {
  sensorAlimentacion.setK(K_ALIMENTACION);
  sensorPermeado.setK(K_PERMEADO);
  bomba.setMlPorVuelta(ML_POR_VUELTA);
  bomba.setPulsosPorRev(PULSOS_POR_REV);
  
  guardarParametrosNVS(K_ALIMENTACION, K_PERMEADO, ML_POR_VUELTA, PULSOS_POR_REV);
  server.send(200, "application/json", "{\"status\":\"reset_ok\"}");
}

// Exportación a Excel (.CSV) con filtrado por Ensayo o Histórico Completo
void handleExportCSV() {
  String targetEnsayo = server.hasArg("ensayo") ? server.arg("ensayo") : "all";
  
  String filename = "PlantaUF_Calibracion_";
  if (targetEnsayo == "all") {
    filename += "HistoricoCompleto.csv";
  } else if (targetEnsayo == "actual") {
    filename += "Ensayo_" + String(ensayoActualId) + "_EnVivo.csv";
  } else {
    filename += "Ensayo_" + targetEnsayo + ".csv";
  }

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader("Content-Type", "text/csv; charset=UTF-8");
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
  server.sendHeader("Connection", "close");
  server.send(200, "text/csv; charset=UTF-8", "");
  
  // Encabezado CSV con columna Estable_1_0 para filtrado riguroso en análisis de datos
  server.sendContent("sep=;\n"
                     "PLANTA DE ULTRAFILTRACION FX100 - REGISTRO DE ENSAYOS Y CALIBRACION\n"
                     "Ensayo_ID;Tiempo_s;Tiempo_MinSec;RPM_Bomba;Estable_1_0;Q_Bomba_Teorico_mLmin;Frec_Alimentacion_Hz;Q_Alimentacion_mLmin;Vol_Alimentacion_L;Frec_PERMEADO_Hz;Q_PERMEADO_mLmin;Vol_PERMEADO_L;Q_Retentado_mLmin;Recuperacion_Y_Pct;Desviacion_Bomba_Alim_Pct;K_Alim;K_Perm;J_LMH\n");

  uint16_t filtroId = 0;
  if (targetEnsayo == "actual") {
    filtroId = ensayoActualId;
  } else if (targetEnsayo != "all") {
    filtroId = (uint16_t)targetEnsayo.toInt();
  }

  char fila[256];
  for (size_t i = 0; i < bufferCount; i++) {
    // Lectura en orden cronológico dentro del buffer circular
    size_t idx = (bufferHead + MAX_REGISTROS - bufferCount + i) % MAX_REGISTROS;
    const RegistroCalibracion& r = bufferLog[idx];
    
    if (filtroId > 0 && r.id_ensayo != filtroId) {
      continue;
    }

    uint32_t mins = r.t_relativo_s / 60;
    uint32_t secs = r.t_relativo_s % 60;

    int len = snprintf(fila, sizeof(fila),
      "%u;%lu;%02lu:%02lu;%.1f;%d;%.1f;%.2f;%.1f;%.4f;%.2f;%.1f;%.4f;%.1f;%.2f;%.2f;%.2f;%.2f;%.2f\n",
      (unsigned)r.id_ensayo, (unsigned long)r.t_relativo_s, (unsigned long)mins, (unsigned long)secs,
      r.rpm, r.en_regimen ? 1 : 0, r.q_bomba, r.f_alim, r.q_alim, r.vol_alim,
      r.f_perm, r.q_perm, r.vol_perm, r.q_ret, r.recov,
      r.delta, r.k_alim, r.k_perm, r.j_lmh
    );

    if (len > 0 && (size_t)len < sizeof(fila)) {
      server.sendContent(fila, len);
    } else if (len >= (int)sizeof(fila)) {
      server.sendContent(fila, sizeof(fila) - 1);
    }

    if ((i & 15) == 0) yield();
  }

  server.sendContent(""); // Cierra el streaming chunked
}

void handleClearCSV() {
  bufferHead = 0;
  bufferCount = 0;
  numEnsayos = 0;
  ensayoActualId = 1;
  tInicioEnsayo_ms = millis();
  volAlimInicioEnsayo = sensorAlimentacion.volumen_L();
  volPermInicioEnsayo = sensorPermeado.volumen_L();
  server.send(200, "text/plain", "LOGS_CLEARED");
}

// ------------------------------------------------------------------------------
// SETUP DEL MICROCONTROLADOR ESP32
// ------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n==================================================");
  Serial.println(" PLANTA PILOTO DE ULTRAFILTRACIÓN FX100 — UNSa   ");
  Serial.println(" Rampa S-Curve, Auto-Calibracion & Datalogger     ");
  Serial.println("==================================================");

  // 1. Cargar parámetros de calibración desde NVS Flash
  cargarParametrosNVS();

  // 2. Inicialización de Hardware
  pinMode(PIN_LED_BOMBA, OUTPUT);
  digitalWrite(PIN_LED_BOMBA, LOW);

  sensorAlimentacion.begin();
  sensorPermeado.begin();
  bomba.begin();

  // 3. Configuración Wi-Fi Robusta (AP Dedicado Anti-Desconexión)
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(SSID_AP, PASS_AP, 1, 0, 4);

  Serial.println("[WIFI] Punto de Acceso Estable Creado:");
  Serial.printf("       SSID: %s | Pass: %s\n", SSID_AP, PASS_AP);
  Serial.printf("       IP AP: http://%s\n", WiFi.softAPIP().toString().c_str());

  if (MDNS.begin("bomba")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("[mDNS] Servidor publicado en: http://bomba.local");
  }

  // 4. Servidor DNS para Portal Cautivo Anti-Desconexión
  dnsServer.start(DNS_PORT, "*", local_IP);
  Serial.println("[DNS] Servidor DNS Captive Portal activo en puerto 53 (Anti-Desconexion)");

  // 5. Enrutamiento del Servidor Web
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/cmd", HTTP_GET, handleCmd);
  server.on("/set", HTTP_GET, handleSetRPM);
  server.on("/set_dev", HTTP_GET, handleSetDev);
  server.on("/reset_dev", HTTP_GET, handleResetDev);
  server.on("/iniciar_auto_cal", HTTP_GET, handleIniciarAutoCal);
  server.on("/cancelar_auto_cal", HTTP_GET, handleCancelarAutoCal);
  server.on("/calibrar_rpm_q", HTTP_GET, handleCalibrarRpmQ);
  server.on("/export_csv", HTTP_GET, handleExportCSV);
  server.on("/clear_csv", HTTP_GET, handleClearCSV);

  // Rutas de sondeo de conectividad de sistemas operativos para Portal Cautivo
  server.on("/generate_204", HTTP_GET, handleRoot);
  server.on("/gen_204", HTTP_GET, handleRoot);
  server.on("/ncsi.txt", HTTP_GET, handleRoot);
  server.on("/connecttest.txt", HTTP_GET, handleRoot);
  server.on("/hotspot-detect.html", HTTP_GET, handleRoot);
  server.onNotFound(handleRoot);

  server.begin();
  Serial.println("[HTTP] Servidor Web SCADA iniciado con exito en puerto 80.\n");

  tLoop = millis();
  tCaudal = millis();
  tDatalogger = millis();
  tInicioEnsayo_ms = millis();
}

// ------------------------------------------------------------------------------
// BUCLE PRINCIPAL (LOOP NO BLOQUEANTE)
// ------------------------------------------------------------------------------
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  uint32_t tAhora = millis();

  // 1. Control cinemático de la bomba cada 50 ms (Rampa S-Curve Progresiva)
  if (tAhora - tLoop >= 50) {
    float dt = (tAhora - tLoop) / 1000.0f;
    tLoop = tAhora;
    bomba.tick(dt);

    digitalWrite(PIN_LED_BOMBA, (bomba.rpmActual() > 0.5f) ? HIGH : LOW);

    bool enMarcha = bomba.enMarcha();
    
    // Flanco de subida: Iniciar sesión de ensayo
    if (enMarcha && !bombaEnMarchaAnterior) {
      iniciarNuevoEnsayo(bomba.rpmObjetivo());
    }
    
    // Flanco de bajada: Finalizar sesión y registrar
    if (!enMarcha && bombaEnMarchaAnterior) {
      finalizarEnsayoActual();
    }

    bombaEnMarchaAnterior = enMarcha;
  }

  // 2. Adquisición y cálculo de caudales cada 1000 ms (1 segundo)
  if (tAhora - tCaudal >= 1000) {
    float dt = (tAhora - tCaudal) / 1000.0f;
    tCaudal = tAhora;

    bool bombaEmpuja = (bomba.rpmActual() > 1.0f);
    sensorAlimentacion.actualizar(dt, bombaEmpuja);
    sensorPermeado.actualizar(dt, bombaEmpuja);

    float qAlim = sensorAlimentacion.caudal_mLmin();
    float qPerm = sensorPermeado.caudal_mLmin();

    flagCruceSensores = (qPerm > qAlim + 50.0f) && (bomba.rpmActual() > 5.0f);
    if (flagCruceSensores) {
      Serial.printf("⚠️ [ALERTA] Cruce de cables o sensor invertido: Q_Perm (%.1f mL/min) > Q_Alim (%.1f mL/min)\n", qPerm, qAlim);
    }

    // Balance Hidráulico Tangencial y Flujo Darcy en tiempo real
    qRet_mLmin   = fmaxf(0.0f, qAlim - qPerm);
    recuperacion = (qAlim > 50.0f) ? ((qPerm / qAlim) * 100.0f) : 0.0f;
    jLMH_actual  = (qPerm * 0.06f) / AREA_MEMBRANA_M2;

    // Cálculo continuo del modelo de Darcy (TMP nominal 0.20 bar a 20°C hasta conectar transductores ADS1115)
    resultadoDarcy = modeloDarcy.calcular(qPerm, 0.20f, 20.0f);

    float qBomba = bomba.caudalTeorico_mLmin();
    deltaBomba   = (qBomba > 1.0f) ? (((qAlim - qBomba) / qBomba) * 100.0f) : 0.0f;

    // Máquina de estados de Auto-Calibración en régimen permanente
    if (autoCalibrando) {
      if (bomba.enRegimenEstable()) {
        autoCalSumFrecAlim += sensorAlimentacion.frecuencia_Hz();
        autoCalSumFrecPerm += sensorPermeado.frecuencia_Hz();
        autoCalMuestras++;

        Serial.printf("[AUTO-CAL] Muestra %u/%u | F_Alim=%.2f Hz | F_Perm=%.2f Hz\n",
                      autoCalMuestras, MUESTRAS_AUTO_CAL, sensorAlimentacion.frecuencia_Hz(), sensorPermeado.frecuencia_Hz());

        if (autoCalMuestras >= MUESTRAS_AUTO_CAL) {
          float fPromAlim = autoCalSumFrecAlim / (float)MUESTRAS_AUTO_CAL;
          float fPromPerm = autoCalSumFrecPerm / (float)MUESTRAS_AUTO_CAL;
          float qRefAlim  = bomba.rpmActual() * bomba.getMlPorVuelta();

          if (qRefAlim > 10.0f && fPromAlim > 1.0f) {
            float nuevoKa = (fPromAlim * 1000.0f) / qRefAlim;
            sensorAlimentacion.setK(nuevoKa);
            Serial.printf("[AUTO-CAL] K_Alim ajustado: %.2f Hz/(L/min)\n", nuevoKa);
          }
          float kpActual = sensorPermeado.getK();

          guardarParametrosNVS(sensorAlimentacion.getK(), kpActual,
                               bomba.getMlPorVuelta(), bomba.getPulsosPorRev());

          autoCalibrando = false;
          autoCalMensaje = "✅ Auto-Calibracion OK: K_Alim=" + String(sensorAlimentacion.getK(), 2) + " (K_Perm intacto=" + String(kpActual, 2) + ")";
          Serial.printf("\n>>> %s <<<\n\n", autoCalMensaje.c_str());
        }
      } else {
        Serial.println("[AUTO-CAL] Esperando estabilizacion de RPM...");
      }
    }

    // Telemetría periódica por Serial (incluye pulsos brutos para auditoría de permeado seco)
    Serial.printf("[TELEMETRIA] RPM: %4.1f | %s | Q_Alim: %5.1f mL/min (%lu pul) | Q_Perm: %5.1f mL/min (%lu pul) | J: %4.2f LMH | Y: %4.1f%% | V_Perm: %.3f L\n",
                  bomba.rpmActual(), bomba.enRegimenEstable() ? "ESTABLE" : "RAMPA",
                  qAlim, (unsigned long)sensorAlimentacion.pulsosBrutos(),
                  qPerm, (unsigned long)sensorPermeado.pulsosBrutos(),
                  jLMH_actual, recuperacion, sensorPermeado.volumen_L());
  }

  // 3. Muestreo del Datalogger cada 10 segundos (SOLO mientras la bomba está en marcha)
  if (tAhora - tDatalogger >= INTERVALO_LOG_MS) {
    tDatalogger = tAhora;
    if (bomba.enMarcha()) {
      guardarMuestraDatalogger();
    }
  }
}
```

---

# 6. CUESTIONARIO Y PUNTOS DE DICTAMEN PARA EL AUDITOR

Como perito y auditor del proyecto, responde de forma exhaustiva y fundamentada a las siguientes cinco preguntas:

### PREGUNTA 1: ¿Es matemáticamente y físicamente suficiente el nuevo filtro de coherencia temporal por software para garantizar 0 pulsos fantasmas en permeado seco SIN TOCAR HARDWARE?
- Analiza si el filtro de dispersión ($dt_{\text{max}} / dt_{\text{min}} \le 4.0$) y la ventana mínima de $30\text{ ms}$ rechazan de forma efectiva las ráfagas espurias causadas por el aliasing del blanking de 1500 µs frente a la conmutación de pasos del motor (1067 a 4800 Hz).
- ¿Existe algún escenario operativo donde los pulsos de ruido EMI se distribuyan periódicamente de forma tan perfecta que engañen al algoritmo de dispersión y simulen una turbina en rotación?

### PREGUNTA 2: Análisis del circuito RC existente en banco (Pull-up 4.7 kΩ + dos capacitores 100 nF en paralelo = 200 nF): ¿Ayuda o empeora la situación?
- Teniendo en cuenta la constante de tiempo $\tau = R \cdot C = 4700\,\Omega \times 200 \times 10^{-9}\text{ F} \approx 0.94\text{ ms}$:
  - ¿Cuál es la frecuencia de corte analógica resultante?
  - ¿Puede una rampa de subida tan lenta ($~1\text{ ms}$) provocar oscilaciones múltiples o falsos disparos por rebote si el pin GPIO del ESP32 opera en zona de umbral lógico indefinido sin suficiente histéresis?
  - Considerando que Enzo no modificará el hardware por ahora, ¿debe el software aplicar alguna técnica específica para mitigar transiciones lentas?

### PREGUNTA 3: Uso del periférico en silicio PCNT (Pulse Counter) del ESP32 vs Interrupciones GPIO (ISR)
- El ESP32 incorpora 8 canales de contadores por hardware (PCNT) con un filtro de glitches programable (`filter_val`) capaz de suprimir pulsos espurios de hasta 1023 ciclos de reloj APB (~12.8 µs) sin consumir ciclos de CPU ni generar interrupciones por pulso.
- Si la solución actual con ISR aún dejara pasar ruido en banco, ¿recomiendas migrar el código a la API PCNT del ESP32 (`driver/pulse_cnt.h` o `driver/pcnt.h`)? ¿Cómo se implementaría de forma compacta en este firmware?

### PREGUNTA 4: Metrología y validez de la turbina YF-S401 en permeado (< 100 mL/min)
- Siendo que la membrana FX100 en agua limpia entrega entre 20 y 100 mL/min a presiones seguras (< 0.5 bar), y sabiendo que la turbina comercial YF-S401 pierde linealidad o se frena mecánicamente por debajo de 200–300 mL/min:
  - ¿Es metodológicamente admisible en una tesis de grado certificar el flujo de permeado con la turbina YF-S401 aplicando calibración matemática?
  - ¿O el tribunal de tesis objetará la validez del balance de materia si no se incorpora una celda de carga gravimétrica (HX711) midiendo el peso acumulado en el tiempo ($Q = \Delta m / \Delta t$)?
  - Si se mantiene la turbina en esta etapa, ¿qué procedimiento de calibración exacto debe realizar Antonella para el permeado?

### PREGUNTA 5: Veredicto de Certificación del Subhito 2.2
- A la luz de las mejoras implementadas en el firmware entregado (Buffer Circular $O(1)$, columna `Estable_1_0`, filtros de coherencia, parada rápida $< 1.5\text{ s}$ y parada de emergencia instantánea física con `PIN_ENA`):
  - ¿Apruebas el cierre del Subhito 2.2 bajo condición de superar con éxito la "Prueba de Permeado Seco" en banco (0 pulsos durante 5 min a 20, 40, 60, 80 y 100 RPM)?
  - ¿O consideras que existen observaciones bloqueantes no resueltas en el software que impidan su certificación? Emite tu dictamen técnico final con las acciones recomendadas en orden de prioridad.

### PREGUNTA 6 / ENTREGA OBLIGATORIA DE CÓDIGO COMPLETO: Tu propuesta de implementación definitiva
- Como auditor experto, **proporciona el código fuente C++ completo y listo para compilar** de la solución que consideres 100% correcta y robusta para resolver este desafío de medición sin tocar el hardware actual:
  - Si consideras que la solución consiste en perfeccionar el procesamiento sobre la ISR actual, entrega tu versión completa de `caudalimetro.h` y `caudalimetro.cpp`.
  - Si consideras que la solución superior es utilizar el periférico de hardware `PCNT` (Pulse Counter) del ESP32 con su filtro de glitches en silicio, escribe la implementación completa y funcional en C++ lista para reemplazar la clase actual.
  - Si consideras que se requiere un estimador estadístico, filtro de mediana móvil, o descarte por plausibilidad espectral, escribe el código C++ completo con su lógica desarrollada.
- **REGLA ESTRICTA DE ENTREGA:** No utilices pseudocódigo ni comentarios evasivos como `// implementar aqui mas adelante`. El código debe estar 100% escrito, limpio y listo para ser integrado y probado en el microcontrolador ESP32 de la planta real.
