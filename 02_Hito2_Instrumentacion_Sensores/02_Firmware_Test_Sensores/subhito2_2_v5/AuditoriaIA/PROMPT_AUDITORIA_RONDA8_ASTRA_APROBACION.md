# SOLICITUD DE DICTAMEN METROLÓGICO FINAL — FIRMWARE V5.1 (SUBHITO 2.2)

> **INSTRUCCIÓN PARA EL MODELO AUDITOR (use.ai / GPT-4o / Claude Opus):**
> Actúas como un **Auditor Metrológico y Revisor Experto de Firmware de Tiempo Real (ESP32 / FreeRTOS / ESP-IDF)**.
> Se te presenta el caso técnico completo y cerrado de la Planta Piloto de Ultrafiltración Tangencial (UNSa). Lee con atención todo el contexto (este prompt es **100% autocontenido y no requiere memoria previa**), el levantamiento formal de las objeciones de la auditoría anterior (Ronda 7), la integración completa de las mejoras propuestas y el código fuente compilado.
> **Tu objetivo:** Emitir un dictamen formal sincero e inapelable (**[APROBADO]**, **[APROBADO CON OBSERVACIONES]** o **[RECHAZADO]**). Si consideras que aún debe ser **[RECHAZADO]**, debes justificar técnicamente los motivos y **suministrar obligatoriamente tu código completo, funcional, limpio, bien estructurado y documentado en C++ explicando con exactitud cómo quedaría aprobado**, listo para que los tesistas lo incorporen y defiendan ante el tribunal de grado.

---

## 1. CONTEXTO INSTITUCIONAL Y ACADÉMICO DEL PROYECTO

* **Institución:** Universidad Nacional de Salta (UNSa), Facultad de Ingeniería, Salta, Argentina.
* **Proyecto de Tesis:** Tesis de Grado de Ingeniería Industrial ("Automatización, Modelado y Validación Experimental de una Planta Piloto de Ultrafiltración Tangencial").
* **Tesistas:** Antonella Guitián & Owen Cañizares | **Codirección Técnica:** Ing. Enzo Corte.
* **Membrana de Filtración:** Cartucho capilar Fresenius FX100 (Helixone®):
  * Superficie interfacial efectiva: $A = 2.2\text{ m}^2$. Diámetro capilar interno: $185\text{ }\mu\text{m}$.
  * Límite de seguridad de Presión Transmembrana: $\text{TMP}_{\max} \le 0.50\text{ bar}$.
* **Estructura por Hitos:**
  * *Hito 1:* Mecánica, cabezal peristáltico, chasis y potencia. *(COMPLETADO)*.
  * *Subhito 2.2 (HITO ACTUAL):* Instrumentación y acondicionamiento de doble caudalímetro Hall YF-S401 (Alimentación y Permeado) acoplados al flujo pulsátil de bomba peristáltica, blindaje anti-EMI por software bajo hardware congelado, datalogger multi-sesión de 600 registros, balance hidráulico tangencial ($Q_{\text{ret}} = Q_{\text{alim}} - Q_{\text{perm}}$, $Y\%$) y SCADA Web Wi-Fi con portal cautivo automático en ESP32.
  * *Subhito 2.3 (PRÓXIMO HITO):* Transductores piezorresistivos de presión (0–1 bar) con ADC I2C ADS1115 de 16 bits para la instrumentación real de la Ley de Darcy ($R_m$, $R_{\text{torta}}$).

---

## 2. RESOLUCIÓN PUNTUAL DE LAS OBSERVACIONES DE LA AUDITORÍA PREVIA (RONDA 7)

Agradecemos la rigurosa revisión estática de la Ronda 7. A continuación se detalla cómo se resolvió cada punto:

### 2.1. Aclaración Definitiva del Mapa de Pines (Sección 1.1 del Rechazo Previo)
* **La observación del auditor:** Se detectó una aparente colisión donde la descripción del prompt asignaba los sensores a GPIO 18 y 19, coincidiendo con PUL y DIR del motor en `config.h`.
* **Aclaración y Blindaje:** Se trató de un error de redacción en el texto del prompt anterior. En el hardware físico montado y en `config.h`, los pines reales **siempre han sido y son**:
  * **Motor Paso a Paso DM860:** PUL = **GPIO 18**, DIR = **GPIO 19**, ENA = **GPIO 23**.
  * **Caudalímetro Alimentación:** **GPIO 14** (Borne 12 Sup).
  * **Caudalímetro Permeado:** **GPIO 27** (Borne 11 Sup, Pull-up externo a 3.3V).
  * **Bus I2C (Subhito 2.3):** SDA = **GPIO 21**, SCL = **GPIO 22**.
* Para garantizar que ninguna modificación futura genere solapamiento, se añadieron **aserciones estáticas en tiempo de compilación** (`static_assert`) en `config.h`. Si hubiera colisión, el compilador aborta la compilación de inmediato.

### 2.2. Caracterización del Rango de Permeado del Sensor YF-S401 (Sección 1.2)
* **La observación del auditor:** La ficha de DFRobot especifica $0.3\text{ a }6.0\text{ L/min}$ ($300\text{ a }6000\text{ mL/min}$). Operar a $10\text{ a }50\text{ mL/min}$ está por debajo del límite de catálogo y no puede presentarse como un rango ya validado de fábrica.
* **Respuesta y Justificación Metrológica de la Tesis:** Coincidimos plenamente. En el marco de una tesis de ingeniería experimental, el sensor **se investigará y caracterizará empíricamente en laboratorio** mediante calibración gravimétrica (balanza digital de precisión $0.1\text{ g}$ y probeta fina). El firmware V5.1 incorpora la bandera formal:
  ```cpp
  constexpr bool CAL_PERMEADO_VALIDADA = false;
  ```
  La cual se mantendrá en `false` hasta documentar la curva de respuesta y repetibilidad en banco, dejando constancia explícita en los datos exportados de que se trata de una calibración en proceso de caracterización empírica.

### 2.3. Continuidad Temporal de 64 bits y Preservación de Período Mínimo (Sección 1.3 y 1.4)
* Se eliminó el reseteo de variables de estado entre ventanas de 1 segundo. La marca de tiempo del último pulso (`_ultimoPulso_us`) y la bandera de pulso previo (`_hayUltimo`) se preservan de forma continua a través de las ventanas.
* Se migraron todos los cálculos temporales a **`esp_timer_get_time()` de 64 bits**, eliminando el riesgo de desbordamiento de `micros()` (32 bits / 71.58 min) en ensayos largos.
* Se adoptó la formulación recíproca sobre períodos completos:
  $$\bar{f} = \frac{10^6 \times \text{periodos}}{\sum \text{periodos_us}}$$
  Con retención limpia del período anterior entre pulsos lentos mientras no expire el timeout (fijado en 5.0 s para permeado).

### 2.4. Desacoplamiento de la Integración de Volumen (Sección 1.5)
* El volumen totalizado en litros (`_vol`) se calcula directamente sobre la totalidad de pulsos físicamente validados (`_liquido_ISR`), de forma completamente independiente de las zonas muertas o umbrales cosméticos de visualización instantánea.

### 2.5. Conservación de Discrepancias y Ley de Darcy (Sección 1.6, 1.7 y 3.4)
* Se preserva el balance con signo sin truncamientos artificiales.
* El cálculo de la Ley de Darcy permanece en reposo formal (`resultadoDarcy = ResultadoDarcy{}`, `valido = false`, `TMP = 0.00`) hasta la integración física de los transductores piezorresistivos en el Subhito 2.3, evitando cualquier dato sintético en los registros.

### 2.6. Módulo de Registro Metrológico Multi-Sesión (`registro_ensayos.h`) (Sección 5.3)
* Se integró el módulo `RegistroEnsayos` con capacidad para 600 registros y 20 sesiones completas.
* Se diferencian formalmente los eventos: `INICIO` ($t=0$), `PERIODICA` (cada 10 segundos) y `FIN` (parada).
* Se incorporaron las marcas de calidad de 16 bits (`CAL_A`, `CAL_P`, `PERIODO_A`, `PERIODO_P`, `SIN_SENAL`, `FUERA_RANGO`, `SECO`, `RUIDO_SECO`, `SIN_IMPULSION`).

### 2.7. Portal Cautivo Wi-Fi Automático
* Se implementó la respuesta `HTTP 302 Redirect` ante sondas de conectividad (`/generate_204`, `/gen_204`, `/hotspot-detect.html`), permitiendo que cualquier smartphone o PC abra instantáneamente la interfaz SCADA al conectarse al Wi-Fi `Bomba_Peristaltica_UF`.

---

## 3. CÓDIGO FUENTE COMPLETO DE PRODUCCIÓN (FIRMWARE V5.1)

El siguiente código fue compilado con la herramienta oficial `arduino-cli` sobre el núcleo `esp32:esp32` v3.3.11 con **0 errores y 0 advertencias**:
* *Uso de Memoria Flash:* 1.032.379 bytes (78%).
* *Uso de Memoria RAM:* 94.532 bytes (28%).

### Archivo 1: `config.h`
```cpp
#pragma once
// ==============================================================================
// CONFIGURACIÓN TÉCNICA — PLANTA DE ULTRAFILTRACIÓN (UNSa) - VERSIÓN 5.1
// ==============================================================================
#include <Arduino.h>

// 1. ASIGNACIÓN DE PINES (PINOUT FÍSICO VERIFICADO)
constexpr uint8_t PIN_PUL                = 18;  // DM860 PUL+ (PUL- a GND común)
constexpr uint8_t PIN_DIR                = 19;  // DM860 DIR+ (DIR- a GND común)
constexpr uint8_t PIN_ENA                = 23;  // DM860 ENA+ (ENA- a GND: HIGH = parada emergencia)
constexpr uint8_t PIN_LED_BOMBA          = 2;   // LED Azul Onboard

// Caudalímetros de Efecto Hall YF-S401
constexpr uint8_t PIN_SENSOR_ALIMENTACION = 14;  // Sensor Alimentación (Borne 12 Sup)
constexpr uint8_t PIN_SENSOR_PERMEADO     = 27;  // Sensor Permeado (Borne 11 Sup, Pull-up 4.7k a 3.3V)

// Puertos I2C para ADS1115 (Subhito 2.3 - Transductores de Presión)
constexpr uint8_t PIN_I2C_SDA             = 21;  // ESP32 SDA
constexpr uint8_t PIN_I2C_SCL             = 22;  // ESP32 SCL

// Aserciones estáticas de seguridad (garantía formal de cero colisión)
static_assert(PIN_SENSOR_ALIMENTACION != PIN_SENSOR_PERMEADO, "Sensores no pueden compartir GPIO");
static_assert(PIN_SENSOR_ALIMENTACION != PIN_PUL && PIN_SENSOR_ALIMENTACION != PIN_DIR && PIN_SENSOR_ALIMENTACION != PIN_ENA, "Colision alim con motor");
static_assert(PIN_SENSOR_PERMEADO != PIN_PUL && PIN_SENSOR_PERMEADO != PIN_DIR && PIN_SENSOR_PERMEADO != PIN_ENA, "Colision perm con motor");
static_assert(PIN_SENSOR_ALIMENTACION != PIN_I2C_SDA && PIN_SENSOR_ALIMENTACION != PIN_I2C_SCL, "Colision alim con I2C");
static_assert(PIN_SENSOR_PERMEADO != PIN_I2C_SDA && PIN_SENSOR_PERMEADO != PIN_I2C_SCL, "Colision perm con I2C");

// Declaración Formal de Estado de Calibración Metrológica
constexpr bool CAL_ALIMENTACION_VALIDADA = true;   // Calibrado con probeta en rango 200-1360 mL/min
constexpr bool CAL_PERMEADO_VALIDADA     = false;  // Requiere caracterización gravimétrica experimental

// 2. PARÁMETROS CINEMÁTICOS DE LA BOMBA (MOTOR NEMA 34 / DM860)
constexpr uint16_t PULSOS_POR_REV = 3200;     // DM860 a 1/16 micropasos (3200 pul/rev)
constexpr float ML_POR_VUELTA     = 13.6000f; // Calibrado nominal: ~13.6 mL/rev

constexpr float RPM_MIN           = 15.0f;    // ~204 mL/min
constexpr float RPM_MAX           = 100.0f;   // ~1360 mL/min
constexpr float RPM_INICIO        = 25.0f;    // Arranque suave
constexpr float RPM_ALARMA_MEMBRANA = 44.0f;  // 600 mL/min referencia clínica

// Especificaciones Membrana FX100
constexpr float AREA_MEMBRANA_M2       = 2.2f;    // Superficie interfacial (m²)
constexpr float TMP_MAX_SEGURA_BAR     = 0.50f;   // Límite máx TMP (bar)
constexpr float P1_MAX_SEGURA_BAR      = 0.60f;   // Límite máx P1 (bar)

// Rampas Cinemáticas S-Curve
constexpr float ACEL_ARRANQUE_RPM_S  = 2.0f;
constexpr float ACEL_NOMINAL_RPM_S   = 3.5f;
constexpr float DESACEL_AJUSTE_RPM_S = 8.0f;
constexpr float FRENADO_PARADA_RPM_S = 70.0f;

// 3. CALIBRACIÓN DE CAUDALÍMETROS Y LÍMITES
constexpr float K_ALIMENTACION = 196.50f; // Hz/(L/min)
constexpr float K_PERMEADO     = 687.33f; // Hz/(L/min)
constexpr float Q_MAX_FISICO_MLMIN = 2500.0f;

// 4. PARÁMETROS DEL DATALOGGER
constexpr uint32_t INTERVALO_LOG_MS   = 10000;  // 10 segundos
constexpr size_t   MAX_REGISTROS      = 600;    // 600 muestras = 100 minutos continuos
constexpr size_t   MAX_ENSAYOS        = 20;

// 5. CONFIGURACIÓN WI-FI
constexpr const char* SSID_AP  = "Bomba_Peristaltica_UF";
constexpr const char* PASS_AP  = "plantapiloto2";
```

---

### Archivo 2: `caudalimetro.h`
```cpp
#pragma once
#include <Arduino.h>
#include <stdint.h>
#include "config.h"

struct ParametrosPulso {
  uint32_t anchoMinLow_us;
  uint32_t anchoMinHigh_us;
  uint32_t periodoMin_us;
  uint32_t periodoMax_us;
  float    qMax_mLmin;
};

class Caudalimetro {
public:
  Caudalimetro(uint8_t pin, float k, const char* nombre, bool esAlimentacion = true);

  bool begin();
  void actualizar(float dt_s, bool bombaEmpuja, float qBombaTeorico_mLmin = 0.0f);

  float  caudal_mLmin()   const { return _q; }
  float  caudal_Lmin()    const { return _q / 1000.0f; }
  float  frecuencia_Hz()  const { return _f; }
  double volumen_L()      const { return _vol; }
  void   resetVolumen()         { _vol = 0.0; }
  const char* nombre()    const { return _nombre; }
  bool   esAlimentacion() const { return _esAlimentacion; }

  bool sinSenal()          const { return _sinSenal; }
  bool falloAlimentacion() const { return _fallo; }
  bool periodoDisponible() const { return _periodoDisponible; }
  bool fueraDeRango()      const { return _fueraRango; }
  bool flujoSinImpulsion() const { return _sinImpulsion; }
  bool calibrado()         const { return _calibrado; }

  uint32_t flancosBrutos()   const { return _flancosVentana; }
  uint32_t pulsosValidos()   const { return _validosVentana; }
  uint32_t glitchesVentana() const { return _rechazosVentana; }
  uint32_t pulsosBrutos()    const { return _validosVentana; }

  uint64_t pulsosAceptadosAcumulados() const { return _aceptadosAcumulados; }
  uint64_t pulsosLiquidoAcumulados()   const { return _liquidoAcumulados; }
  uint64_t rechazosAcumulados()        const { return _rechazosAcumulados; }

  bool ruidoDetectadoEnSeco() const { return _ruidoEnSeco; }
  bool modoSeco()             const { return _modoSeco; }
  void setModoSeco(bool activo);
  void limpiarAlarmaSeco();

  bool  setK(float nuevoK);
  float getK() const { return _k; }
  bool  declararCalibrado(bool valido);

private:
  static void ARDUINO_ISR_ATTR isrPuente(void* arg);
  void ARDUINO_ISR_ATTR isrInterna();

  const uint8_t   _pin;
  float           _k;
  const char*     _nombre;
  const bool      _esAlimentacion;
  ParametrosPulso _p;

  bool _iniciado  = false;
  bool _calibrado = false;
  bool _modoSeco  = false;

  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;

  volatile bool     _nivelAlto = false;
  volatile bool     _altoConocido = false;
  volatile bool     _candidato = false;
  volatile bool     _hayUltimo = false;

  volatile int64_t  _inicioNivel_us = 0;
  volatile int64_t  _bajada_us = 0;
  volatile int64_t  _ultimoPulso_us = 0;
  volatile int64_t  _ultimoPeriodo_us = 0;

  volatile uint32_t _flancos_ISR = 0;
  volatile uint32_t _aceptados_ISR = 0;
  volatile uint32_t _liquido_ISR = 0;
  volatile uint32_t _rechazos_ISR = 0;
  volatile uint32_t _periodos_ISR = 0;
  volatile int64_t  _sumaPeriodos_ISR_us = 0;

  volatile bool     _seco_ISR = false;
  volatile bool     _alarmaSeco_ISR = false;

  uint32_t _flancosVentana = 0;
  uint32_t _validosVentana = 0;
  uint32_t _rechazosVentana = 0;

  uint64_t _aceptadosAcumulados = 0;
  uint64_t _liquidoAcumulados = 0;
  uint64_t _rechazosAcumulados = 0;

  float  _f = 0.0f;
  float  _q = 0.0f;
  double _vol = 0.0;

  bool  _sinSenal = true;
  bool  _periodoDisponible = false;
  bool  _fueraRango = false;
  bool  _sinImpulsion = false;
  bool  _ruidoEnSeco = false;
  bool  _fallo = false;
  float _sinPulso_s = 0.0f;
};
```

---

### Archivo 3: `caudalimetro.cpp`
```cpp
#include "caudalimetro.h"
#include <math.h>
#include "esp_timer.h"

Caudalimetro::Caudalimetro(uint8_t pin, float k, const char* nombre, bool esAlimentacion)
  : _pin(pin), _k(k), _nombre(nombre), _esAlimentacion(esAlimentacion) {
  if (_esAlimentacion) {
    _p = {200, 200, 1000, 1000000, 1400.0f};
  } else {
    _p = {600, 600, 2500, 5000000, 200.0f};
  }
}

bool Caudalimetro::setK(float nuevoK) {
  if (_iniciado || !isfinite(nuevoK) || nuevoK < 0.1f) return false;
  _k = nuevoK;
  _calibrado = false;
  return true;
}

bool Caudalimetro::declararCalibrado(bool valido) {
  if (_iniciado) return false;
  _calibrado = valido;
  return true;
}

bool Caudalimetro::begin() {
  if (_iniciado || _pin > 39 || !isfinite(_k) || _k < 0.1f) return false;

  pinMode(_pin, INPUT);

  const bool alto = digitalRead(_pin);
  const int64_t ahora = esp_timer_get_time();

  portENTER_CRITICAL(&_mux);
  _nivelAlto = alto;
  _inicioNivel_us = ahora;
  _altoConocido = false;
  _candidato = false;
  _hayUltimo = false;
  _ultimoPulso_us = 0;
  _ultimoPeriodo_us = 0;
  portEXIT_CRITICAL(&_mux);

  attachInterruptArg(digitalPinToInterrupt(_pin), isrPuente, this, CHANGE);
  _iniciado = true;
  return true;
}

void Caudalimetro::setModoSeco(bool activo) {
  portENTER_CRITICAL(&_mux);
  _seco_ISR = activo;
  _candidato = false;
  _altoConocido = false;
  portEXIT_CRITICAL(&_mux);
  _modoSeco = activo;
}

void Caudalimetro::limpiarAlarmaSeco() {
  portENTER_CRITICAL(&_mux);
  _alarmaSeco_ISR = false;
  portEXIT_CRITICAL(&_mux);
  _ruidoEnSeco = false;
}

void ARDUINO_ISR_ATTR Caudalimetro::isrPuente(void* arg) {
  static_cast<Caudalimetro*>(arg)->isrInterna();
}

void ARDUINO_ISR_ATTR Caudalimetro::isrInterna() {
  const int64_t ahora = esp_timer_get_time();
  const bool alto = digitalRead(_pin);

  portENTER_CRITICAL_ISR(&_mux);
  ++_flancos_ISR;
  if (_seco_ISR) _alarmaSeco_ISR = true;

  if (alto == _nivelAlto) {
    ++_rechazos_ISR;
    portEXIT_CRITICAL_ISR(&_mux);
    return;
  }

  const int64_t duracion = ahora - _inicioNivel_us;

  if (!alto) {
    _candidato = _altoConocido && (duracion >= (int64_t)_p.anchoMinHigh_us);
    _bajada_us = ahora;
    if (!_candidato) ++_rechazos_ISR;
  } else {
    if (_candidato && (duracion >= (int64_t)_p.anchoMinLow_us)) {
      const int64_t periodo = _bajada_us - _ultimoPulso_us;
      if (!_hayUltimo || (periodo >= (int64_t)_p.periodoMin_us)) {
        ++_aceptados_ISR;
        if (!_seco_ISR) ++_liquido_ISR;

        if (_hayUltimo && (periodo <= (int64_t)_p.periodoMax_us)) {
          _ultimoPeriodo_us = periodo;
          _sumaPeriodos_ISR_us += periodo;
          ++_periodos_ISR;
        } else {
          _ultimoPeriodo_us = 0;
        }
        _ultimoPulso_us = _bajada_us;
        _hayUltimo = true;
      } else {
        ++_rechazos_ISR;
      }
    } else if (_candidato) {
      ++_rechazos_ISR;
    }
    _candidato = false;
    _altoConocido = true;
  }

  _nivelAlto = alto;
  _inicioNivel_us = ahora;
  portEXIT_CRITICAL_ISR(&_mux);
}

void Caudalimetro::actualizar(float dt_s, bool bombaEmpuja, float qBombaTeorico_mLmin) {
  if (!_iniciado || !isfinite(dt_s) || dt_s <= 0.0f) return;

  uint32_t flancos, aceptados, liquido, rechazados, periodos;
  int64_t sumaPeriodos, ultimoPeriodo, ultimoPulso, ahora;
  bool hayUltimo, alarma;

  portENTER_CRITICAL(&_mux);
  flancos       = _flancos_ISR;        _flancos_ISR = 0;
  aceptados     = _aceptados_ISR;      _aceptados_ISR = 0;
  liquido       = _liquido_ISR;        _liquido_ISR = 0;
  rechazados    = _rechazos_ISR;       _rechazos_ISR = 0;
  periodos      = _periodos_ISR;       _periodos_ISR = 0;
  sumaPeriodos  = _sumaPeriodos_ISR_us; _sumaPeriodos_ISR_us = 0;
  ultimoPeriodo = _ultimoPeriodo_us;
  ultimoPulso   = _ultimoPulso_us;
  hayUltimo     = _hayUltimo;
  alarma        = _alarmaSeco_ISR;
  ahora         = esp_timer_get_time();
  portEXIT_CRITICAL(&_mux);

  _flancosVentana   = flancos;
  _validosVentana   = aceptados;
  _rechazosVentana  = rechazados;
  _aceptadosAcumulados += aceptados;
  _liquidoAcumulados   += liquido;
  _rechazosAcumulados  += rechazados;
  _ruidoEnSeco = alarma;

  _vol += (double)liquido / (60.0 * (double)_k);

  _sinSenal = !hayUltimo || ((ahora - ultimoPulso) > (int64_t)_p.periodoMax_us);
  _periodoDisponible = !_sinSenal && (ultimoPeriodo > 0);

  float qInst = 0.0f;

  if (_sinSenal) {
    _f = 0.0f;
    qInst = 0.0f;
  } else if (!_periodoDisponible) {
    _f = 0.0f;
    qInst = 0.0f;
  } else if (periodos > 0 && sumaPeriodos > 0) {
    _f = (float)(1.0e6 * (double)periodos / (double)sumaPeriodos);
    qInst = (_f * 1000.0f) / _k;
  } else if (ultimoPeriodo > 0) {
    _f = (float)(1.0e6 / (double)ultimoPeriodo);
    qInst = (_f * 1000.0f) / _k;
  }

  if (qInst > 0.0f) {
    _q = (_q == 0.0f) ? qInst : (0.25f * qInst + 0.75f * _q);
  } else {
    _q = 0.0f;
  }

  _fueraRango = isfinite(_q) && (_q > _p.qMax_mLmin);
  _sinImpulsion = !_esAlimentacion && (!bombaEmpuja || (isfinite(qBombaTeorico_mLmin) && qBombaTeorico_mLmin < 20.0f)) && ((aceptados > 0) || (isfinite(_q) && _q > 0.0f));

  if (aceptados > 0 || !bombaEmpuja || !_esAlimentacion) {
    _sinPulso_s = 0.0f;
    _fallo = false;
  } else {
    _sinPulso_s += dt_s;
    _fallo = (_sinPulso_s >= 5.0f);
  }
}
```

---

### Archivo 4: `registro_ensayos.h`
```cpp
#pragma once
#include <Arduino.h>
#include <math.h>
#include <stdint.h>
#include "esp_timer.h"
#include "caudalimetro.h"

class RegistroEnsayos {
public:
  enum Tipo : uint8_t { INICIO = 0, PERIODICA = 1, FIN = 2 };

  enum Calidad : uint16_t {
    CAL_A            = (1U << 0),
    CAL_P            = (1U << 1),
    PERIODO_A        = (1U << 2),
    PERIODO_P        = (1U << 3),
    SIN_SENAL_A      = (1U << 4),
    SIN_SENAL_P      = (1U << 5),
    FUERA_RANGO_A    = (1U << 6),
    FUERA_RANGO_P    = (1U << 7),
    SECO_A           = (1U << 8),
    SECO_P           = (1U << 9),
    RUIDO_SECO_A     = (1U << 10),
    RUIDO_SECO_P     = (1U << 11),
    SIN_IMPULSION_P  = (1U << 12)
  };

  struct Fila {
    uint16_t sesion;
    uint8_t  tipo;
    uint16_t calidad;
    int64_t  tRegistro_us;
    int64_t  tAdquisicion_us;
    uint64_t nA;
    uint64_t nP;
    uint64_t rechazosA;
    uint64_t rechazosP;
    float    rpm;
    float    qA;
    float    qP;
    float    kA;
    float    kP;
  };

  static constexpr size_t   CAPACIDAD    = 600;
  static constexpr uint16_t SESIONES_MAX = 20;
  static constexpr int64_t  INTERVALO_US = 10000000; // 10 s

  Fila filas[CAPACIDAD];

  size_t   cantidad()            const { return _n; }
  bool     activo()              const { return _activo; }
  bool     incompleto()          const { return _incompleto; }
  uint32_t intervalosOmitidos()  const { return _omitidos; }
  uint16_t sesionActual()        const { return _sesion; }

  bool iniciar(int64_t tAdquisicion_us, float rpm, const Caudalimetro& a, const Caudalimetro& p) {
    if (_n + 2 > CAPACIDAD || _sesion >= SESIONES_MAX) {
      _incompleto = true;
      return false;
    }
    _sesion++;
    _inicio_us = esp_timer_get_time();
    _proximo_us = _inicio_us + INTERVALO_US;
    _activo = true;
    return guardar(INICIO, _inicio_us, tAdquisicion_us, rpm, a, p);
  }

  void tick(int64_t tAdquisicion_us, float rpm, const Caudalimetro& a, const Caudalimetro& p) {
    if (!_activo) return;
    const int64_t ahora = esp_timer_get_time();
    if (ahora < _proximo_us) return;

    const uint64_t vencidos = (uint64_t)((ahora - _proximo_us) / INTERVALO_US) + 1;
    _proximo_us += (int64_t)vencidos * INTERVALO_US;

    if (vencidos > 1) {
      _omitidos += (uint32_t)(vencidos - 1);
      _incompleto = true;
    }

    if (_n >= CAPACIDAD - 1) {
      _omitidos++;
      _incompleto = true;
      return;
    }

    guardar(PERIODICA, ahora, tAdquisicion_us, rpm, a, p);
  }

  bool finalizar(int64_t tAdquisicion_us, float rpm, const Caudalimetro& a, const Caudalimetro& p) {
    if (!_activo) return false;
    const bool ok = guardar(FIN, esp_timer_get_time(), tAdquisicion_us, rpm, a, p);
    _activo = false;
    return ok;
  }

  void limpiar() {
    _n = 0;
    _sesion = 0;
    _activo = false;
    _incompleto = false;
    _omitidos = 0;
  }

private:
  size_t   _n = 0;
  uint16_t _sesion = 0;
  bool     _activo = false;
  bool     _incompleto = false;
  uint32_t _omitidos = 0;
  int64_t  _inicio_us = 0;
  int64_t  _proximo_us = 0;

  static uint16_t calcularCalidad(const Caudalimetro& a, const Caudalimetro& p) {
    uint16_t f = 0;
    if (a.calibrado())            f |= CAL_A;
    if (p.calibrado())            f |= CAL_P;
    if (a.periodoDisponible())    f |= PERIODO_A;
    if (p.periodoDisponible())    f |= PERIODO_P;
    if (a.sinSenal())             f |= SIN_SENAL_A;
    if (p.sinSenal())             f |= SIN_SENAL_P;
    if (a.fueraDeRango())         f |= FUERA_RANGO_A;
    if (p.fueraDeRango())         f |= FUERA_RANGO_P;
    if (a.modoSeco())             f |= SECO_A;
    if (p.modoSeco())             f |= SECO_P;
    if (a.ruidoDetectadoEnSeco()) f |= RUIDO_SECO_A;
    if (p.ruidoDetectadoEnSeco()) f |= RUIDO_SECO_P;
    if (p.flujoSinImpulsion())    f |= SIN_IMPULSION_P;
    return f;
  }

  bool guardar(Tipo tipo, int64_t ahora, int64_t adquisicion, float rpm, const Caudalimetro& a, const Caudalimetro& p) {
    if (_n >= CAPACIDAD) {
      _incompleto = true;
      return false;
    }
    Fila& r = filas[_n++];
    r.sesion = _sesion;
    r.tipo = (uint8_t)tipo;
    r.calidad = calcularCalidad(a, p);
    r.tRegistro_us = ahora - _inicio_us;
    r.tAdquisicion_us = (adquisicion >= _inicio_us) ? (adquisicion - _inicio_us) : -1;
    r.nA = a.pulsosLiquidoAcumulados();
    r.nP = p.pulsosLiquidoAcumulados();
    r.rechazosA = a.rechazosAcumulados();
    r.rechazosP = p.rechazosAcumulados();
    r.rpm = rpm;

    bool tieneMedicion = (tipo != INICIO && r.tAdquisicion_us >= 0);
    r.qA = tieneMedicion ? a.caudal_mLmin() : 0.0f;
    r.qP = tieneMedicion ? p.caudal_mLmin() : 0.0f;
    r.kA = a.getK();
    r.kP = p.getK();
    return true;
  }
};
```

---

## 4. ENTREGABLES REQUERIDOS AL AUDITOR EXPERTO

1. **Dictamen Metrológico Formal:**
   * Emitir veredicto formal inequívoco: **[APROBADO]**, **[APROBADO CON OBSERVACIONES]** o **[RECHAZADO]** para autorizar el paso a los ensayos experimentales con agua en banco.
   * Si tu veredicto es **[RECHAZADO]**, debes justificar técnicamente qué aspecto exacto lo motiva y explicar detalladamente con tu código propuesto qué cambios específicos se deben hacer para que quede **[APROBADO]**.
2. **Evaluación de la Solución de Software:**
   * ¿Consideras que la arquitectura implementada (ancho de nivel en `CHANGE`, continuidad temporal de 64 bits, desacoplamiento de totalizador volumétrico y módulo `RegistroEnsayos`) resuelve con el máximo rigor posible las limitaciones del hardware congelado ($4.7\text{ k}\Omega + 200\text{ nF}$)?
3. **CÓDIGO COMPLETO FINAL DEL AUDITOR (ENTREGABLE OBLIGATORIO):**
   * Ya sea que tu dictamen sea de aprobación o rechazo, debes suministrar **tu versión completa, funcional, limpia, modular y exhaustivamente comentada en C++** de los archivos involucrados (`caudalimetro.h`, `caudalimetro.cpp`, `registro_ensayos.h`, etc.), mostrando con precisión **cómo debe quedar el código para estar plenamente aprobado y listo para producción**, con una estructura comprensible que Antonella y Owen puedan defender con solvencia en su tesis.
