# 🧠 PROMPT MAESTRO PARA AUDITORÍA EXTERNA — RONDA 2 (PEER REVIEW v4)
> **Instrucciones para Ing. Enzo:**  
> Copia todo el bloque de abajo y pégalo directamente en las IAs de auditoría externa (**ChatGPT 6 Astra**, **Claude**, **GLM-5.3**, **Gemini / Modelo B**).  
> Este prompt recopila el consenso unánime de la Ronda 1, explica tu decisión experimental sobre las 100 RPM, presenta el código completo refactorizado de la **Versión 4 (Hito 2.2 v4)** y solicita la auditoría de cierre junto con el diseño del siguiente paso (Hito 2.3 FreeRTOS/ADS1115 y diseño factorial de Darcy).

---

```markdown
# 🏛️ AUDITORÍA TÉCNICA EXTERNA — RONDA 2: EVALUACIÓN DEL FIRMWARE v4 Y PLAN DE ENSAYOS
**Destinatarios:** Agente Auditor Senior / Revisor Externo de Sistemas Embebidos & Procesos de Separación por Membrana.  
**Interlocutores:** Antigravity (Google DeepMind), Ing. Enzo (Co-director Doctoral), Antonella Guitián & Owen Cañizares (Tesistas UNSa).  
**Contexto del Proyecto:** Planta Piloto de Ultrafiltración Tangencial (*Cross-Flow*) con Dializador Capilar Fresenius FX100 (Helixone®), bomba peristáltica industrial MBP-2000 (NEMA 34 + DM860) y microcontrolador ESP32.

---

## 📋 1. Retroalimentación de la Ronda 1 y Aclaración Experimental Clave

Agradecemos y validamos el dictamen unánime de la primera ronda de auditoría. Sus observaciones sobre la inconsistencia de caudal ($44.12\text{ RPM} = 600\text{ mL/min}$ con $13.60\text{ mL/rev}$), el bug de consigna en la inversión de marcha, la fragmentación de heap por concatenación de `String` y el error de cuantización a bajo caudal en el permeado ($5.5\text{ Hz} \rightarrow \pm 18\%$) fueron sumamente precisas y fundamentadas.

### ⚠️ Aclaración Fundamental del Co-Director (Ing. Enzo) sobre el Límite de RPM:
1. **Límite Clínico vs. Límite de Tesis de Ingeniería:**
   El límite estricto de **$600\text{ mL/min}$** ($\approx 44.12\text{ RPM}$) es una especificación médica diseñada para **hemodiálisis con sangre humana** (evitar hemólisis de eritrocitos y colapso venoso).
2. **Experimentación en la Tesis con Agua y Soluciones Modelo:**
   En este proyecto doctoral y de grado se trabaja con agua y suspensiones modelo tratadas con mucílago coagulante de *Opuntia ficus-indica*. El laboratorio cuenta con disponibilidad de membranas para realizar ensayos de caracterización hidrodinámica severa y **diseño factorial de experimentos**.
3. **Decisión de Diseño en Firmware v4:**
   - Se eleva **`RPM_MAX` a $100.0\text{ RPM}$** ($\approx 1360\text{ mL/min}$ / $1.36\text{ L/min}$).
   - Se preserva el umbral de **$44.0\text{ RPM}$** como **`RPM_ALARMA_MEMBRANA`** (aviso visual en UI y registro de telemetría indicando superación del régimen clínico nominal).
   - Se mantiene la consigna de arranque suave en **$25.0\text{ RPM}$** ($\approx 340\text{ mL/min}$).

---

## 🛠️ 2. Código Fuente Refactorizado Completo — Versión 4 (`subhito2_2_v4`)

Se ha estructurado la nueva versión **v4** incorporando todas las soluciones a los hallazgos críticos de la Ronda 1:

### A. `config.h` (v4)
```cpp
#pragma once
#include <Arduino.h>

// 1. ASIGNACIÓN DE PINES (PINOUT)
constexpr uint8_t PIN_PUL                = 18;  // DM860 PUL+ (Cátodo común: PUL- a GND)
constexpr uint8_t PIN_DIR                = 19;  // DM860 DIR+ (Cátodo común: DIR- a GND)
constexpr uint8_t PIN_LED_BOMBA          = 2;   // LED Azul Onboard

constexpr uint8_t PIN_SENSOR_ALIMENTACION = 14;  // Caudalímetro Feed (Front-End 4.7k + 100nF)
constexpr uint8_t PIN_SENSOR_PERMEADO     = 27;  // Caudalímetro Permeado (Front-End 4.7k + 100nF)

// 2. PARÁMETROS CINEMÁTICOS DE LA BOMBA (MOTOR NEMA 34 / DM860)
constexpr uint16_t PULSOS_POR_REV = 3200;     // DM860: 1/16 micropasos
constexpr float ML_POR_VUELTA     = 13.6000f; // Calibrado: 680.0 mL/min / 50.0 RPM

// Rango de Operación en RPM
constexpr float RPM_MIN           = 15.0f;    // ~204 mL/min
constexpr float RPM_MAX           = 100.0f;   // ~1360 mL/min (Habilitado para diseño factorial en agua)
constexpr float RPM_INICIO        = 25.0f;    // Consigna de arranque suave (~340 mL/min)
constexpr float RPM_ALARMA_MEMBRANA = 44.0f;  // 44.12 RPM = 600 mL/min (Umbral de referencia clínico)

// 3. ESPECIFICACIONES DE LA MEMBRANA FRESENIUS FX100 (HELIXONE®)
constexpr float AREA_MEMBRANA_M2       = 2.2f;    // Superficie interfacial efectiva (m²)
constexpr float K_UF_NOMINAL           = 73.0f;   // mL / (h * mmHg)
constexpr float DIAMETRO_CAPILAR_UM    = 185.0f;  // Diámetro interno capilar (μm)
constexpr float ESPESOR_PARED_UM       = 35.0f;   // Grosor de pared capilar (μm)
constexpr float TMP_MAX_SEGURA_BAR     = 0.50f;   // Límite máximo admisible de TMP (bar)
constexpr float Q_CLINICO_SANGRE_MAX   = 600.0f;  // mL/min (referencia clínica hemodiálisis)

// Rampas cinemáticas S-Curve
constexpr float ACEL_ARRANQUE_RPM_S  = 2.0f;   // 2.0 RPM/s despegue inicial suave
constexpr float ACEL_NOMINAL_RPM_S   = 3.5f;   // 3.5 RPM/s aceleración progresiva
constexpr float DESACEL_AJUSTE_RPM_S = 8.0f;   // 8.0 RPM/s desaceleración en marcha
constexpr float FRENADO_PARADA_RPM_S = 45.0f;  // 45.0 RPM/s parada rápida STOP (< 1.5s)

// 4. CALIBRACIÓN DE CAUDALÍMETROS YF-S401
constexpr float K_ALIMENTACION = 154.62f; // Factor K Alimentación (105.14 Hz = 680.0 mL/min)
constexpr float K_PERMEADO     = 55.00f;  // Factor K Permeado (5.50 Hz = 100.0 mL/min en régimen laminar)

constexpr uint32_t FILTRO_RUIDO_US = 1500;    // 1.5 ms blanking (hasta 666 Hz / ~4300 mL/min)
constexpr float Q_MAX_FISICO_MLMIN = 6000.0f; // Límite de corte físico

// 5. DATALOGGER Y AUTO-CALIBRACIÓN
constexpr uint32_t INTERVALO_LOG_MS   = 10000;  // 10s
constexpr size_t   MAX_REGISTROS      = 600;    // 600 muestras = 1.66 horas
constexpr size_t   MAX_ENSAYOS        = 20;     // Hasta 20 corridas
constexpr uint8_t  MUESTRAS_AUTO_CAL  = 15;     // 15 muestras para auto-cal

// 6. WI-FI
constexpr const char* SSID_AP  = "Bomba_Peristaltica_UF";
constexpr const char* PASS_AP  = "plantapiloto2";
```

### B. `Bomba.h` & `Bomba.cpp` (v4 — Fix de Inversión y `setRPM`)
```cpp
// Bomba.h (Extracto de firmas clave)
class Bomba {
public:
  void begin();
  void arrancar();
  void detener();
  bool setRPM(float rpm);
  void toggleSentido();
  // ... getters cinemáticos ...
  void tick(float dt);
private:
  void fijarSentido(bool horario);
  bool _enMarcha = false;
  bool _horario = true;
  bool _invirtiendo = false;
  float _objetivo = RPM_INICIO;
  float _actual = 0.0f;
  float _rpmGuardada = RPM_INICIO;
  uint32_t _fActual = 0;
  float _mlPorVuelta = ML_POR_VUELTA;
  uint16_t _pulsosPorRev = PULSOS_POR_REV;
};

// Bomba.cpp (Implementación corregida)
void Bomba::arrancar() {
  _enMarcha = true;
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

bool Bomba::setRPM(float rpm) {
  if (!std::isfinite(rpm)) return false;
  float r = constrain(rpm, RPM_MIN, RPM_MAX);
  if (_invirtiendo) {
    _rpmGuardada = r;   // Almacena consigna si el usuario mueve slider durante el frenado
  } else {
    _objetivo = r;
    _rpmGuardada = r;
  }
  return true;
}

void Bomba::toggleSentido() {
  if (_invirtiendo) return;
  if (_actual < 1.0f) {
    fijarSentido(!_horario);
  } else {
    _invirtiendo = true;
    _rpmGuardada = (_objetivo >= RPM_MIN) ? _objetivo : RPM_INICIO;
    _objetivo = 0.0f;
  }
}

void Bomba::tick(float dt) {
  float objetivo = _enMarcha ? _objetivo : 0.0f;

  if (_actual < objetivo) {
    if (_actual < 1.0f) _actual = 1.0f;
    float tasaAcel = ACEL_NOMINAL_RPM_S;
    if (_actual < 10.0f) {
      tasaAcel = ACEL_ARRANQUE_RPM_S + (_actual / 10.0f) * (ACEL_NOMINAL_RPM_S - ACEL_ARRANQUE_RPM_S);
    }
    float delta = objetivo - _actual;
    if (delta < 3.0f) tasaAcel = fmaxf(0.8f, tasaAcel * (delta / 3.0f));
    _actual = fminf(objetivo, _actual + tasaAcel * dt);

  } else if (_actual > objetivo) {
    if (!_enMarcha) {
      _actual = fmaxf(0.0f, _actual - FRENADO_PARADA_RPM_S * dt);
      if (_actual < 1.0f) _actual = 0.0f;
    } else {
      float tasaDecel = DESACEL_AJUSTE_RPM_S;
      float delta = _actual - objetivo;
      if (delta < 3.0f) tasaDecel = fmaxf(1.0f, tasaDecel * (delta / 3.0f));
      _actual = fmaxf(objetivo, _actual - tasaDecel * dt);
    }
  }

  // Conmutación segura en inversión al llegar a cero físico
  if (_invirtiendo && _actual <= 0.1f) {
    ledcWrite(PIN_PUL, 0); // Corte absoluto de pulsos
    _fActual = 0;
    fijarSentido(!_horario);
    _invirtiendo = false;
    _objetivo = (_rpmGuardada >= RPM_MIN) ? _rpmGuardada : RPM_INICIO;
  }

  // Generación PWM hardware LEDC
  if (_actual >= 1.0f) {
    uint32_t f = (uint32_t)(_actual * (float)_pulsosPorRev / 60.0f);
    if (f < 50) f = 50;
    if (f != _fActual) { 
      ledcChangeFrequency(PIN_PUL, f, 10); 
      _fActual = f; 
    }
    ledcWrite(PIN_PUL, 512); // Duty 50%
  } else if (_fActual != 0) {
    ledcWrite(PIN_PUL, 0);
    _fActual = 0;
  }
}
```

### C. `caudalimetro.h` & `caudalimetro.cpp` (v4 — Conteo Recíproco de Período y Mux Individual)
```cpp
// caudalimetro.h
class Caudalimetro {
public:
  Caudalimetro(uint8_t pin, float k, const char* nombre);
  void begin();
  void actualizar(float dt_s, bool bombaEmpuja);
  // ... getters ...
private:
  static void IRAM_ATTR isrPuente(void* arg);
  const uint8_t _pin;
  float _k;
  const char* _nombre;

  volatile uint32_t _pulsos = 0;
  volatile uint32_t _t_ultimo = 0;
  volatile uint32_t _t_primero = 0;
  volatile uint32_t _periodo_us = 0;

  float _f = 0.0f, _q = 0.0f, _vol = 0.0f;
  uint8_t _segSinPulso = 0;
  bool _fallo = false;

  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED; // Cerrojo individual por sensor
};

// caudalimetro.cpp
void IRAM_ATTR Caudalimetro::isrPuente(void* arg) {
  Caudalimetro* c = reinterpret_cast<Caudalimetro*>(arg);
  uint32_t t = micros();
  uint32_t dt = t - c->_t_ultimo;

  if (dt >= FILTRO_RUIDO_US) {
    portENTER_CRITICAL_ISR(&c->_mux);
    if (c->_pulsos == 0) c->_t_primero = t;
    c->_periodo_us = dt;
    c->_t_ultimo   = t;
    c->_pulsos++;
    portEXIT_CRITICAL_ISR(&c->_mux);
  }
}

void Caudalimetro::actualizar(float dt_s, bool bombaEmpuja) {
  portENTER_CRITICAL(&_mux);
  uint32_t n      = _pulsos;
  _pulsos         = 0;
  uint32_t per_us = _periodo_us;
  uint32_t t_prim = _t_primero;
  uint32_t t_ult  = _t_ultimo;
  portEXIT_CRITICAL(&_mux);

  // Conteo recíproco híbrido:
  if (n >= 2 && t_ult > t_prim) {
    _f = ((float)(n - 1) * 1000000.0f) / (float)(t_ult - t_prim);
  } else if (n == 1 && per_us > 0) {
    _f = 1000000.0f / (float)per_us;
  } else if (n == 0) {
    uint32_t tAhora = micros();
    uint32_t tSinFlanco = tAhora - t_ult;
    // Si el último pulso es reciente (< 2.5 períodos), mantiene frecuencia sin saltar a cero
    if (per_us > 0 && tSinFlanco < (25 * per_us) / 10 && _f > 0.1f) {
      // Mantiene estimación suave
    } else {
      _f = 0.0f;
    }
  }

  float q = (_f * 1000.0f) / _k;

  if (q > Q_MAX_FISICO_MLMIN) {
    q = 0.0f; _f = 0.0f;
    Serial.printf("[%s] Ruido descartado: %lu pulsos espurios\n", _nombre, (unsigned long)n);
  } else {
    // Integración de volumen SOLO con pulsos válidos
    _vol += (float)n / (_k * 60.0f);
  }

  _q = (n > 0 || _f > 0.1f) ? (0.4f * q + 0.6f * _q) : 0.0f;

  if (n > 0 || _f > 0.1f) {
    _segSinPulso = 0; _fallo = false;
  } else if (bombaEmpuja) {
    if (++_segSinPulso >= 5) _fallo = true;
  } else {
    _segSinPulso = 0; _fallo = false;
  }
}
```

### D. `darcy.h` (v4 — Modelo Físico de Membrana FX100)
```cpp
#pragma once
#include <Arduino.h>
#include <cmath>
#include "config.h"

struct ResultadoDarcy {
  float J_LMH, J20_LMH, mu_Pas, TCF, R_total, R_torta, TMP_bar;
  bool  valido;
};

class ModeloDarcy {
public:
  static constexpr float MU20 = 1.002e-3f; // Pa·s (Agua a 20 °C)

  // Ecuación de Vogel para viscosidad dinámica del agua
  static float viscosidadAgua(float temp_C) {
    const float T_K = temp_C + 273.15f;
    return 2.414e-5f * powf(10.0f, 247.8f / (T_K - 140.0f));
  }

  ResultadoDarcy calcular(float qPerm_mLmin, float tmp_bar, float temp_C = 20.0f) const {
    ResultadoDarcy r{};
    if (!std::isfinite(tmp_bar) || !std::isfinite(temp_C) || !std::isfinite(qPerm_mLmin)) return r;
    if (tmp_bar < 0.01f || qPerm_mLmin < 0.5f) return r;

    r.TMP_bar = tmp_bar;
    r.mu_Pas  = viscosidadAgua(temp_C);
    r.TCF     = r.mu_Pas / MU20;
    r.J_LMH   = (qPerm_mLmin * 0.06f) / AREA_MEMBRANA_M2;
    r.J20_LMH = r.J_LMH * r.TCF;

    const float J_SI   = r.J_LMH / 3.6e6f;
    const float TMP_Pa = tmp_bar * 100000.0f;
    r.R_total = TMP_Pa / (r.mu_Pas * J_SI);
    r.R_torta = r.R_total - _Rm;
    r.valido  = true;
    return r;
  }

  void acumularPuntoAguaLimpia(float J_SI, float TMP_Pa) {
    _sumJP += J_SI * TMP_Pa; _sumPP += TMP_Pa * TMP_Pa;
  }
  bool finalizarCalibracionRm(float temp_C = 20.0f) {
    if (_sumPP <= 0.0f || _sumJP <= 0.0f) return false;
    _Rm = 1.0f / (viscosidadAgua(temp_C) * (_sumJP / _sumPP));
    _sumJP = _sumPP = 0.0f;
    return true;
  }
private:
  float _Rm = 1.44e13f; // m^-1 (nominal FX100 Helixone)
  float _sumJP = 0.0f, _sumPP = 0.0f;
};
```

### E. Eliminación de Fragmentación de Heap en HTTP (`EN_USO_firmware_planta.ino`)
- **`handleStatus()`**: Armado con `char buf[768]` y `snprintf()` + streaming chunked para el array `ensayos`. Cero asignaciones `String +=`.
- **`handleExportCSV()`**: Streaming HTTP chunked (`server.setContentLength(CONTENT_LENGTH_UNKNOWN)`) fila a fila con buffer estático `char fila[200]`.
- **Telemetría de memoria**: Se exponen en `/status` los campos `ESP.getFreeHeap()` y `heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)` para auditoría en vivo.
- **Sanity check de cruce:** Si `qPerm > qAlim + 50.0f` y bomba en marcha $\rightarrow$ `flagCruceSensores = true`.

---

## 🎯 3. Preguntas de Auditoría para la Ronda 2

Como auditor externo senior, solicitamos tu evaluación técnica sobre los siguientes puntos:

### A. Validación del Firmware v4
1. ¿Consideras resueltos de forma robusta el bug de inversión de marcha, la cuantización a bajo flujo del permeado y el riesgo de fragmentación de heap?
2. Respecto al conteo recíproco: ¿Ves algún escenario de borde donde la retención de frecuencia durante pulsos lentos ($n=0$, $t < 2.5 \times \text{periodo}$) pueda introducir retardo o falsear una parada real de flujo?

### B. Transición a FreeRTOS con `Snapshot` (Hito 2.3)
1. Para desacoplar completamente el lazo de control del servidor HTTP en el ESP32:
   - ¿Cuál es la estructura mínima recomendada para crear `taskControl` (Core 1, prio 10, período 50 ms) y dejar `taskWeb` en Core 0?
   - ¿Qué mecanismo de sincronización recomiendas para el `Snapshot` (doble buffer con punteros atómicos vs. mutex FreeRTOS corto) para evitar cualquier contención?

### C. Estrategia de Instrumentación para ADS1115 y TDS (Hitos 2.3 y 2.4)
1. Con 3 transductores de presión (Entrada $P_1$, Retentado $P_2$, Permeado $P_3$) y una sonda TDS de conductividad:
   - ¿Recomiendas leer el ADS1115 en modo diferencial directo ($\Delta P_{\text{in}} = P_1 - P_3$, $\Delta P_{\text{out}} = P_2 - P_3$) o single-ended para conservar los valores individuales de presión manométrica?
   - Para no bloquear el bus I2C: ¿Qué tasa de muestreo (SPS) y máquina de estados no bloqueante recomiendas en Arduino / ESP-IDF?

### D. Propuesta de Diseño Factorial Experimental (Hito 5 - Darcy y Fouling)
1. Con la bomba capaz de operar entre $15$ y $100\text{ RPM}$ ($200$ a $1360\text{ mL/min}$) y la válvula de aguja regulando la contrapresión axial:
   - ¿Qué matriz experimental factorial (p.ej. $3 \times 3$ con niveles de RPM, TMP y concentración de coagulante de *Opuntia*) recomiendas para determinar de forma irrefutable el **flujo crítico ($J_c$)** y la compresibilidad de la torta coloidal ($\alpha$) en la tesis?
   - ¿Qué protocolo de lavado / enjuague sugieres ejecutar entre réplicas factoriales para asegurar reproducibilidad de la resistencia intrínseca $R_m$?
```
