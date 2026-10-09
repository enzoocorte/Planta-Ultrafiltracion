# AUDITORÍA TÉCNICA Y DICTAMEN METROLÓGICO — FIRMWARE V5.2
## PROYECTO: PLANTA PILOTO DE ULTRAFILTRACIÓN (TESIS DE GRADO EN INGENIERÍA INDUSTRIAL - UNSA 2026)

---

### 1. CONTEXTO GENERAL, OBJETIVOS Y CONDICIÓN DE AUDITORÍA
Estimado auditor de IA (GPT Astra / use.ai):
Por favor, lee atentamente este expediente completo antes de emitir tu dictamen. **Asume que este es un nuevo proceso de auditoría y no tienes memoria de interacciones previas.**

#### A. Marco Institucional y Propósito
Este proyecto corresponde a la Tesis de Grado para optar al título de Ingeniero Industrial en la **Universidad Nacional de Salta (UNSa)**, Argentina (2026), desarrollada por los tesistas Enzo, Antonella y Owen.
El proyecto consiste en el diseño, instrumentación y automatización de una **Planta Piloto de Ultrafiltración Tangencial** equipada con una membrana capilar de hemodiálisis Fresenius FX100 (fibra hueca Helixone®, área interfacial de 2.2 m²).

El plan de trabajo de la tesis se estructura en hitos:
- **Hito 1:** Diseño cinemático y electromecánico de la bomba peristáltica (motor paso a paso NEMA 34 con driver DM860 en cátodo común, 3200 pulsos/rev, micropasos 1/16, desplazamiento volumétrico nominal calibrado de 13.60 mL/vuelta). [COMPLETADO].
- **Hito 2.1:** Control de lazo abierto y rampas cinemáticas S-Curve (< 1.5 s de frenado seguro). [COMPLETADO].
- **Hito 2.2 (HITO ACTUAL):** Instrumentación de caudalímetros y caracterización metrológica en banco de laboratorio con recirculación atmosférica de agua desmineralizada.
- **Hito 2.3:** Integración de transductores de presión diferencial y transmembrana (TMP) mediante ADC I2C ADS1115 y validación del modelo de Darcy.
- **Hito 3:** Validación del proceso de ultrafiltración y resistencia hidráulica de ensuciamiento.

#### B. Problema Físico y Justificación de la Investigación (Sensor YF-S401)
El circuito hidráulico cuenta con dos caudalímetros de turbina/efecto Hall comerciales modelo **YF-S401**:
1. **Canal de Alimentación:** Borne de entrada, pin GPIO 14 del ESP32.
2. **Canal de Permeado:** Borne de filtrado, pin GPIO 27 del ESP32.

**La problemática científica:**
- El fabricante especifica el YF-S401 para un rango nominal de **300 a 6000 mL/min** (0.3 a 6.0 L/min) con factor constante K = 98 Hz/(L/min).
- Sin embargo, en ultrafiltración de laboratorio con la bomba peristáltica girando entre 15 y 100 RPM, los caudales reales son:
  - Alimentación: **200 a 1360 mL/min** (zona de transición y bajo régimen).
  - Permeado: **10 a 200 mL/min** (muy por debajo del límite inferior del fabricante).
- Ensayos empíricos previos en laboratorio con probeta graduada y cronómetro demostraron que a bajo caudal con flujo pulsátil peristáltico, el factor K real del sensor en alimentación es de **196.50 Hz/(L/min)** (casi el doble del nominal de fábrica), debido a la inercia del álabe y la naturaleza pulsátil del flujo.
- **Interferencias electromagnéticas (EMI):** La cercanía física del motor paso a paso NEMA 34 y del driver DM860 genera glitches y transitorios que se acoplan capacitiva e inductivamente al cableado de señal.
- **Acondicionamiento físico ya instalado en hardware:**
  - Resistencias pull-up externas dedicadas de 4.7 kΩ a línea regulada de 3.3V.
  - Capacitores cerámicos 104 (100 nF) entre señal y GND como filtro pasa-bajos analógico en cada sensor.
  - Alimentaciones de potencia (24V motor) y lógica (5V/3.3V) con tierras en estrella y optoacopladores DM860 independientes.

---

### 2. ANTECEDENTES DE AUDITORÍA Y HALLAZGOS QUE MOTIVARON EL RECHAZO EN V5.1
En la revisión previa (V5.1), Astra emitió un dictamen de [RECHAZADO] para producción industrial e incondicional, dejando asentado explícitamente:
> "Este dictamen no rechaza investigar experimentalmente el YF-S401 por debajo de su rango nominal. Esa caracterización es legítima si se identifica como tal, se conserva el dato primario y se realiza en un banco supervisado con protección hidráulica independiente. Mi alcance es una revisión técnica documental..."

Los 5 hallazgos técnicos y documentales que impidieron la aprobación en V5.1 fueron:
1. **Expediente incompleto:** El prompt anterior solo adjuntó 4 extractos modulares, faltando el programa principal (.ino), la cinemática de la bomba, el portal cautivo, el servidor web y la sincronización entre tareas.
2. **Modo seco sin aislamiento estricto:** setModoSeco() borraba candidato y nivel alto, pero conservaba períodos anteriores y acumuladores de ventana, arriesgando contaminar la estimación de caudal al retornar a modo húmedo.
3. **Validez de calibración desacoplada del rango:** Las banderas no distinguían si la lectura estaba dentro del rango calibrado experimental (200-1360 mL/min) o si excedía el intervalo documentado.
4. **Fronteras de sesión no atómicas en el registro:** RegistroEnsayos no vinculaba de forma protegida el tiempo con los contadores de cada sensor, no rechazaba inicios superpuestos y no reservaba plaza fija para garantizar el guardado del evento FIN.
5. **Declaración espuria de Presión / Darcy:** Sin instrumentación física de presión (prevista para el Subhito 2.3), declarar TMP = 0.00 bar comunicaba una presión falsa inexistente.

---

### 3. MEJORAS ARQUITECTÓNICAS IMPLEMENTADAS EN EL FIRMWARE V5.2
Hemos adoptado integralmente la propuesta arquitectónica de 25 páginas provista por Astra:

1. **Aislamiento Total del Modo Seco con Épocas de Configuración:**
   Se incorporó la función atómica reiniciarEstimadorLocked() que borra la totalidad de acumuladores temporales (bajada_us, ultimoPeriodo_us, periodosVentana, sumaPeriodosVentana_us, hayAnterior), e incrementa monótonamente _epoca en cada transición seco/húmedo. Una sesión detecta el cambio de época y marca CAMBIO_EPOCA en el log.
2. **Instantáneas Inmutables (struct Muestra):**
   La tarea adquiere periódicamente mediante capturar(bool bombaEmpuja) una copia inmutable protegida por spinlock portENTER_CRITICAL. Cada muestra conserva sus contadores brutos de 64 bits (flancos, aceptados, pulsosMedicion, rechazados), su timestamp microsegundo de adquisición t_us, edad del último pulso, y cálculo recíproco puro sin supresión cosmética.
3. **Primitiva Pura de Caudal Medio Integral:**
   Se implementó Caudalimetro::caudalMedio_mLmin(inicial, final) para calcular con rigor experimental:
   $$\overline{Q}_{\mathrm{mL/min}} = \frac{\Delta N \times 10^9}{K \times \Delta t_{\mu s}}$$
   Devuelve NAN si las épocas difieren, si se midió en seco o si los tiempos son inconsistentes.
4. **Máscara de Bits de Calidad Metrológica (enum Calidad):**
   Flags rigurosos: INICIADO, CAL_DOCUMENTADA, PERIODO_DISPONIBLE, SIN_SENAL, FUERA_RANGO_CAL, FUERA_RANGO_OPERATIVO, SECO, RUIDO_SECO, SIN_IMPULSION, MEDICION_EN_RANGO_CAL.
5. **Datalogger Multi-Sesión en Heap Dinámico con Reserva Obligatoria de FIN:**
   RegistroEnsayos asigna su buffer en memoria dinámica durante el arranque (malloc/new), evitando el desbordamiento de la sección estática .bss (.dram0.data) del ESP32. Garantiza que jamás se acepte una sesión superpuesta, reserva 1 plaza obligatoria para el evento FIN, y exporta 29 columnas metrológicas documentando valores faltantes como nan.
6. **Transparencia en Darcy y Presión:**
   TMP y Darcy permanecen desactivados (valido = false) hasta la integración física del bus I2C en el Subhito 2.3.
7. **Verificación de Compilación Real (arduino-cli v3.3.11):**
   - Firmware integrado de planta: Exit Code 0 (0 errores). Flash: 1,037,915 bytes (79%), RAM: 69,708 bytes (21%), 257,972 bytes libres de Heap.
   - Banco aislado de comprobación metrológica: Exit Code 0 (0 errores). Flash: 281,656 bytes (21%), RAM: 23,364 bytes (7%), 304,316 bytes libres de Heap.

---

### 4. EXPEDIENTE COMPLETO DEL CÓDIGO FUENTE (FIRMWARE V5.2)

#### ARCHIVO 1: config.h
```cpp
#pragma once
// ==============================================================================
// CONFIGURACIÓN TÉCNICA — PLANTA DE ULTRAFILTRACIÓN (UNSa 2026)
// Parámetros Cinemáticos, Metrología Certificada y Asignación de Pines
// ==============================================================================

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

// ------------------------------------------------------------------------------
// 1. ASIGNACIÓN DE PINES (PINOUT)
// ------------------------------------------------------------------------------
// Driver DM860 en CÁTODO COMÚN: GPIO envía HIGH -> Optoacoplador ON
constexpr uint8_t PIN_PUL                = 18;  // DM860 PUL+ (PUL- a GND común)
constexpr uint8_t PIN_DIR                = 19;  // DM860 DIR+ (DIR- a GND común)
constexpr uint8_t PIN_ENA                = 23;  // DM860 ENA+ (ENA- a GND: HIGH = Driver deshabilitado)
constexpr uint8_t PIN_LED_BOMBA          = 2;   // LED Azul Onboard (indicador de marcha)

// Caudalímetros de Efecto Hall YF-S401
constexpr uint8_t PIN_SENSOR_ALIMENTACION = 14;  // Sensor de Alimentación (Borne 12 Sup)
constexpr uint8_t PIN_SENSOR_PERMEADO     = 27;  // Sensor de Permeado (Borne 11 Sup, Pull-up externo a 3.3V)

// Puertos I2C para ADS1115 (Subhito 2.3 - Transductores de Presión)
constexpr uint8_t PIN_I2C_SDA             = 21;  // ESP32 SDA
constexpr uint8_t PIN_I2C_SCL             = 22;  // ESP32 SCL

// ASERCIÓN ESTÁTICA EXHAUSTIVA DE PINES (Auditoría GPT Astra Ronda 8 / V5.1)
// Verifica todos los pares posibles del sistema para evitar cualquier colisión de GPIO
constexpr uint8_t PINES_SISTEMA[] = {
  PIN_PUL, PIN_DIR, PIN_ENA, PIN_LED_BOMBA,
  PIN_SENSOR_ALIMENTACION, PIN_SENSOR_PERMEADO,
  PIN_I2C_SDA, PIN_I2C_SCL
};

constexpr bool pinesUnicos() {
  for (size_t i = 0; i < sizeof(PINES_SISTEMA) / sizeof(PINES_SISTEMA[0]); ++i) {
    for (size_t j = i + 1; j < sizeof(PINES_SISTEMA) / sizeof(PINES_SISTEMA[0]); ++j) {
      if (PINES_SISTEMA[i] == PINES_SISTEMA[j]) return false;
    }
  }
  return true;
}
static_assert(pinesUnicos(), "Hay GPIO compartidos en el sistema");

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

// Rampas de Aceleración y Frenado Conforme a Auditoría (< 1.5s parada garantizada)
constexpr float ACEL_ARRANQUE_RPM_S  = 2.0f;   // 2.0 RPM/s despegue inicial suave y confiable
constexpr float ACEL_NOMINAL_RPM_S   = 3.5f;   // 3.5 RPM/s aceleración progresiva
constexpr float DESACEL_AJUSTE_RPM_S = 8.0f;   // 8.0 RPM/s desaceleración en marcha
constexpr float FRENADO_PARADA_RPM_S = 70.0f;  // 70.0 RPM/s frenado rápido garantizado (< 1.43s desde 100 RPM)

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

// ------------------------------------------------------------------------------
// 4. ESTRUCTURA METROLÓGICA Y CALIBRACIÓN DE CAUDALÍMETROS YF-S401 (GPT Astra)
// ------------------------------------------------------------------------------
// UNIDAD METROLÓGICA DE K: [Hz / (L/min)]
// Relación matemática: Q [mL/min] = (F [Hz] * 1000) / K
struct ConfigSensor {
  uint8_t  pin;
  double   k_Hz_por_Lmin;
  uint32_t lowMin_us;
  uint32_t highMin_us;
  uint32_t periodoMin_us;
  uint32_t timeout_us;
  double   qMaxOperativo_mLmin;
  bool     calibracionDocumentada;
  double   qMinCal_mLmin;
  double   qMaxCal_mLmin;
};

// Sensor de Alimentación: caracterizado en laboratorio con probeta y bomba peristáltica (20 a 90 RPM)
constexpr ConfigSensor SENSOR_ALIM_CFG = {
  PIN_SENSOR_ALIMENTACION, // Pin 14
  196.50,                  // K experimental validado en probeta
  200,                     // Ancho mínimo de nivel LOW (200 us)
  200,                     // Ancho mínimo de nivel HIGH (200 us)
  1000,                    // Período mínimo admisible (1000 us = 1000 Hz)
  1000000,                 // Timeout de pérdida de señal (1 s)
  1400.0,                  // Q máximo operativo admisible (mL/min)
  true,                    // Calibración documentada en laboratorio
  200.0,                   // Q mínimo del intervalo calibrado (mL/min)
  1360.0                   // Q máximo del intervalo calibrado (mL/min)
};

// Sensor de Permeado: calibración provisional; requiere banco gravimétrico independiente
constexpr ConfigSensor SENSOR_PERM_CFG = {
  PIN_SENSOR_PERMEADO,     // Pin 27
  687.33,                  // K de fábrica / preliminar
  600,                     // Ancho mínimo de nivel LOW (600 us)
  600,                     // Ancho mínimo de nivel HIGH (600 us)
  2500,                    // Período mínimo admisible (2500 us = 400 Hz)
  5000000,                 // Timeout de pérdida de señal (5 s)
  200.0,                   // Q máximo operativo admisible (mL/min)
  false,                   // Requiere balanza gravimétrica en Subhito 2.2
  0.0,                     // Q min
  0.0                      // Q max
};

// Constantes globales de K para compatibilidad con NVS
constexpr float K_ALIMENTACION = 196.50f;
constexpr float K_PERMEADO     = 687.33f;

// ------------------------------------------------------------------------------
// 5. PARÁMETROS DEL DATALOGGER MULTI-SESIÓN Y AUTO-CALIBRACIÓN
// ------------------------------------------------------------------------------
constexpr uint32_t INTERVALO_LOG_MS   = 10000;  // Muestreo cada 10 segundos
constexpr size_t   MAX_REGISTROS      = 250;    // 250 muestras en buffer circular (41.6 minutos continuos)
constexpr size_t   MAX_ENSAYOS        = 20;     // Hasta 20 ensayos/sesiones
constexpr uint8_t  MUESTRAS_AUTO_CAL  = 15;     // 15 muestras para auto-calibración en régimen permanente

// ------------------------------------------------------------------------------
// 6. CONFIGURACIÓN DE RED WI-FI Y SCADA
// ------------------------------------------------------------------------------
constexpr const char* SSID_AP  = "Bomba_Peristaltica_UF";
constexpr const char* PASS_AP  = "plantapiloto2";
constexpr const char* SSID_STA = "Box804";
constexpr const char* PASS_STA = "plantapiloto2";

```

#### ARCHIVO 2: caudalimetro.h
```cpp
#pragma once
// ==============================================================================
// CLASE CAUDALÍMETRO (Sensor de Efecto Hall YF-S401 - V5.1 Metrología GPT Astra)
// - Detección por interrupción en ambos flancos (CHANGE)
// - Validación geométrica de nivel HIGH/LOW (rechaza glitches EMI y rebotes RC)
// - Continuidad temporal estricta de 64 bits con esp_timer_get_time()
// - Estimación de frecuencia recíproca sobre períodos completos reales
// - Totalización entera acumulada desacoplada del display cosmético
// - Modo seco estrictamente aislado con época de configuración y reset de estado
// - Instantáneas inmutables atómicas (struct Muestra) y flags de bit de Calidad
// ==============================================================================

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include "config.h"

class Caudalimetro {
public:
  enum Calidad : uint16_t {
    INICIADO                = (1U << 0),
    CAL_DOCUMENTADA         = (1U << 1),
    PERIODO_DISPONIBLE      = (1U << 2),
    SIN_SENAL               = (1U << 3),
    FUERA_RANGO_CAL         = (1U << 4),
    FUERA_RANGO_OPERATIVO   = (1U << 5),
    SECO                    = (1U << 6),
    RUIDO_SECO              = (1U << 7),
    SIN_IMPULSION           = (1U << 8),
    MEDICION_EN_RANGO_CAL   = (1U << 9)
  };

  // Instantánea inmutable atómica de adquisición
  struct Muestra {
    int64_t  t_us = 0;
    int64_t  ultimoAceptado_us = -1;
    int64_t  edadPulso_us = -1;
    uint64_t flancos = 0;
    uint64_t aceptados = 0;
    uint64_t pulsosMedicion = 0;
    uint64_t rechazados = 0;
    uint32_t epoca = 0;
    uint16_t calidad = 0;
    double   k = 0.0;
    double   frecuencia_Hz = 0.0;
    double   q_mLmin = 0.0;
    double   volumenEstimado_L = 0.0;
  };

  explicit Caudalimetro(const ConfigSensor& config, const char* nombre, bool esAlimentacion = true);
  Caudalimetro(const Caudalimetro&) = delete;
  Caudalimetro& operator=(const Caudalimetro&) = delete;

  // Inicialización (llamar en setup)
  bool begin();

  // Captura atómica inmutable (llamada periódica desde tarea principal en loop)
  Muestra capturar(bool bombaEmpuja);

  // Control estricto de Modo de Diagnóstico en Seco
  void setModoSeco(bool activo);
  void limpiarAlarmaSeco();
  void resetVolumen();

  // Primitiva matemática pura para calcular caudal medio exacto por delta de pulsos
  static double caudalMedio_mLmin(const Muestra& inicial, const Muestra& final);

  // Getters para compatibilidad con el SCADA Web y Datalogger
  float       caudal_mLmin()          const;
  float       caudal_Lmin()           const;
  float       frecuencia_Hz()         const;
  double      volumen_L()             const { return _volumenEstimadoAcumulado_L; }
  const char* nombre()                const { return _nombre; }
  bool        esAlimentacion()        const { return _esAlimentacion; }
  const Muestra& ultimaMuestra()      const { return _ultimaMuestra; }

  // Diagnósticos de calidad metrológica
  bool sinSenal()                     const { return (_ultimaMuestra.calidad & SIN_SENAL) != 0; }
  bool periodoDisponible()            const { return (_ultimaMuestra.calidad & PERIODO_DISPONIBLE) != 0; }
  bool fueraDeRango()                 const { return (_ultimaMuestra.calidad & FUERA_RANGO_OPERATIVO) != 0; }
  bool fueraRangoCalibracion()        const { return (_ultimaMuestra.calidad & FUERA_RANGO_CAL) != 0; }
  bool medicionEnRangoCalibrado()     const { return (_ultimaMuestra.calidad & MEDICION_EN_RANGO_CAL) != 0; }
  bool flujoSinImpulsion()            const { return (_ultimaMuestra.calidad & SIN_IMPULSION) != 0; }
  bool calibrado()                    const { return (_ultimaMuestra.calidad & CAL_DOCUMENTADA) != 0; }
  bool ruidoDetectadoEnSeco()         const { return (_ultimaMuestra.calidad & RUIDO_SECO) != 0; }
  bool modoSeco()                     const { return (_ultimaMuestra.calidad & SECO) != 0; }
  bool falloAlimentacion()            const { return _falloAlimentacion; }

  // Métricas de ventana
  uint32_t flancosBrutos()            const { return _flancosVentana; }
  uint32_t pulsosValidos()            const { return _validosVentana; }
  uint32_t glitchesVentana()          const { return _rechazosVentana; }
  uint64_t pulsosAceptadosAcumulados() const { return _ultimaMuestra.aceptados; }
  uint64_t pulsosLiquidoAcumulados()  const { return _ultimaMuestra.pulsosMedicion; }
  uint64_t rechazosAcumulados()       const { return _ultimaMuestra.rechazados; }

  // Configuración y calibración
  bool  setK(double nuevoK);
  float getK()                        const { return (float)_cfg.k_Hz_por_Lmin; }
  bool  declararCalibrado(bool valido);

private:
  static void ARDUINO_ISR_ATTR isrPuente(void* arg);
  void ARDUINO_ISR_ATTR isr();
  void reiniciarEstimadorLocked();

  ConfigSensor _cfg;
  const char*  _nombre;
  const bool   _esAlimentacion;

  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
  bool _iniciado = false;

  // Variables compartidas con la ISR bajo _mux
  volatile bool     _seco = false;
  volatile bool     _alarmaSeco = false;
  volatile bool     _nivelAlto = false;
  volatile bool     _altoConocido = false;
  volatile bool     _candidato = false;
  volatile bool     _hayAnterior = false;

  volatile int64_t  _inicioNivel_us = 0;
  volatile int64_t  _bajada_us = 0;
  volatile int64_t  _bajadaAnterior_us = 0;
  volatile int64_t  _ultimoAceptado_us = -1;
  volatile int64_t  _ultimoPeriodo_us = 0;

  volatile uint64_t _flancos = 0;
  volatile uint64_t _aceptados = 0;
  volatile uint64_t _pulsosMedicion = 0;
  volatile uint64_t _rechazados = 0;
  volatile uint64_t _periodosVentana = 0;
  volatile uint64_t _sumaPeriodosVentana_us = 0;
  volatile uint32_t _epoca = 1;

  // Variables privadas de la tarea principal
  uint64_t _anteriorMedicion = 0;
  uint32_t _flancosVentana = 0;
  uint32_t _validosVentana = 0;
  uint32_t _rechazosVentana = 0;
  uint64_t _flancosPrevios = 0;
  uint64_t _aceptadosPrevios = 0;
  uint64_t _rechazosPrevios = 0;
  double   _volumenEstimadoAcumulado_L = 0.0;
  float    _qSuavizadoDisplay = 0.0f;
  float    _sinPulso_s = 0.0f;
  bool     _falloAlimentacion = false;
  Muestra  _ultimaMuestra;
};

```

#### ARCHIVO 3: caudalimetro.cpp
```cpp
#include "caudalimetro.h"
#include <math.h>
#include "driver/gpio.h"
#include "esp_timer.h"

// ==============================================================================
// IMPLEMENTACIÓN DE CAUDALÍMETRO (V5.1 - Auditoría Metrológica GPT Astra)
// ==============================================================================

Caudalimetro::Caudalimetro(const ConfigSensor& config, const char* nombre, bool esAlimentacion)
  : _cfg(config), _nombre(nombre), _esAlimentacion(esAlimentacion) {
}

void Caudalimetro::reiniciarEstimadorLocked() {
  _altoConocido           = false;
  _candidato              = false;
  _hayAnterior            = false;
  _bajada_us              = 0;
  _bajadaAnterior_us      = 0;
  _ultimoAceptado_us      = -1;
  _ultimoPeriodo_us       = 0;
  _periodosVentana        = 0;
  _sumaPeriodosVentana_us = 0;
}

bool Caudalimetro::setK(double nuevoK) {
  if (_iniciado || !std::isfinite(nuevoK) || nuevoK < 0.1) return false;
  _cfg.k_Hz_por_Lmin = nuevoK;
  _cfg.calibracionDocumentada = false;
  return true;
}

bool Caudalimetro::declararCalibrado(bool valido) {
  if (_iniciado) return false;
  _cfg.calibracionDocumentada = valido;
  return true;
}

void Caudalimetro::resetVolumen() {
  _volumenEstimadoAcumulado_L = 0.0;
}

bool Caudalimetro::begin() {
  if (_iniciado) return false;
  if (!GPIO_IS_VALID_GPIO((gpio_num_t)_cfg.pin) ||
      !std::isfinite(_cfg.k_Hz_por_Lmin) || _cfg.k_Hz_por_Lmin <= 0.0 ||
      _cfg.lowMin_us == 0 || _cfg.highMin_us == 0 ||
      _cfg.periodoMin_us == 0 || _cfg.timeout_us < _cfg.periodoMin_us ||
      !std::isfinite(_cfg.qMaxOperativo_mLmin) || _cfg.qMaxOperativo_mLmin <= 0.0) {
    return false;
  }

  if (_cfg.calibracionDocumentada &&
      (!std::isfinite(_cfg.qMinCal_mLmin) || !std::isfinite(_cfg.qMaxCal_mLmin) ||
       _cfg.qMinCal_mLmin < 0.0 || _cfg.qMaxCal_mLmin < _cfg.qMinCal_mLmin)) {
    return false;
  }

  pinMode(_cfg.pin, INPUT); // Utiliza el pull-up externo existente de 4.7k a 3.3V

  portENTER_CRITICAL(&_mux);
  _nivelAlto = (digitalRead(_cfg.pin) == HIGH);
  _inicioNivel_us = esp_timer_get_time();
  reiniciarEstimadorLocked();
  portEXIT_CRITICAL(&_mux);

  attachInterruptArg(digitalPinToInterrupt(_cfg.pin), isrPuente, this, CHANGE);
  _iniciado = true;
  return true;
}

void ARDUINO_ISR_ATTR Caudalimetro::isrPuente(void* arg) {
  static_cast<Caudalimetro*>(arg)->isr();
}

void ARDUINO_ISR_ATTR Caudalimetro::isr() {
  portENTER_CRITICAL_ISR(&_mux);
  const int64_t ahora = esp_timer_get_time();
  const bool alto = (digitalRead(_cfg.pin) == HIGH);

  ++_flancos;
  if (_seco) _alarmaSeco = true;

  if (alto == _nivelAlto) {
    // Rebote o perturbación diferida sin cambio de nivel lógico real
    ++_rechazados;
    reiniciarEstimadorLocked();
    _inicioNivel_us = ahora;
    portEXIT_CRITICAL_ISR(&_mux);
    return;
  }

  const int64_t duracion = ahora - _inicioNivel_us;

  if (!alto) {
    // Flanco descendente: termina HIGH, comienza LOW
    _candidato = _altoConocido && (duracion >= (int64_t)_cfg.highMin_us);
    _bajada_us = ahora;
    if (!_candidato) ++_rechazados;
  } else {
    // Flanco ascendente: termina LOW, comienza HIGH
    if (_candidato && (duracion >= (int64_t)_cfg.lowMin_us)) {
      const int64_t periodo = _bajada_us - _bajadaAnterior_us;
      // Continuidad temporal que no se interrumpe entre llamadas de la tarea
      if (!_hayAnterior || (periodo >= (int64_t)_cfg.periodoMin_us)) {
        ++_aceptados;
        if (!_seco) ++_pulsosMedicion;

        if (_hayAnterior && (periodo < (int64_t)_cfg.timeout_us)) {
          _ultimoPeriodo_us = periodo;
          ++_periodosVentana;
          _sumaPeriodosVentana_us += (uint64_t)periodo;
        } else {
          _ultimoPeriodo_us = 0;
          _periodosVentana = 0;
          _sumaPeriodosVentana_us = 0;
        }
        _bajadaAnterior_us = _bajada_us;
        _ultimoAceptado_us = ahora;
        _hayAnterior = true;
      } else {
        ++_rechazados; // Período menor al mínimo admisible físicamente
      }
    } else if (_candidato) {
      ++_rechazados; // Pulso demasiado estrecho (glitch EMI)
    }
    _candidato = false;
    _altoConocido = true;
  }

  _nivelAlto = alto;
  _inicioNivel_us = ahora;
  portEXIT_CRITICAL_ISR(&_mux);
}

void Caudalimetro::setModoSeco(bool activo) {
  portENTER_CRITICAL(&_mux);
  if (_seco != activo) {
    _seco = activo;
    ++_epoca; // Cada cambio de modo inicia una nueva época metrológica
    reiniciarEstimadorLocked();
    _nivelAlto = (digitalRead(_cfg.pin) == HIGH);
    _inicioNivel_us = esp_timer_get_time();
  }
  portEXIT_CRITICAL(&_mux);
}

void Caudalimetro::limpiarAlarmaSeco() {
  portENTER_CRITICAL(&_mux);
  _alarmaSeco = false;
  portEXIT_CRITICAL(&_mux);
}

Caudalimetro::Muestra Caudalimetro::capturar(bool bombaEmpuja) {
  Muestra m;
  m.k = _cfg.k_Hz_por_Lmin;
  m.frecuencia_Hz = NAN;
  m.q_mLmin = NAN;
  m.volumenEstimado_L = NAN;

  if (!_iniciado) {
    m.t_us = esp_timer_get_time();
    _ultimaMuestra = m;
    return m;
  }

  bool seco, alarma, hayAnterior;
  int64_t periodo;
  uint64_t periodos, suma;

  portENTER_CRITICAL(&_mux);
  m.t_us               = esp_timer_get_time();
  m.ultimoAceptado_us  = _ultimoAceptado_us;
  m.flancos            = _flancos;
  m.aceptados          = _aceptados;
  m.pulsosMedicion     = _pulsosMedicion;
  m.rechazados         = _rechazados;
  m.epoca              = _epoca;
  seco                 = _seco;
  alarma               = _alarmaSeco;
  hayAnterior          = _hayAnterior;
  periodo              = _ultimoPeriodo_us;
  periodos             = _periodosVentana;
  suma                 = _sumaPeriodosVentana_us;
  _periodosVentana     = 0;
  _sumaPeriodosVentana_us = 0;
  portEXIT_CRITICAL(&_mux);

  // Diagnósticos de la última ventana
  _flancosVentana  = (uint32_t)(m.flancos - _flancosPrevios);       _flancosPrevios = m.flancos;
  _validosVentana  = (uint32_t)(m.aceptados - _aceptadosPrevios);   _aceptadosPrevios = m.aceptados;
  _rechazosVentana = (uint32_t)(m.rechazados - _rechazosPrevios);   _rechazosPrevios = m.rechazados;

  m.calidad = INICIADO;
  if (_cfg.calibracionDocumentada) m.calidad |= CAL_DOCUMENTADA;
  if (seco)   m.calidad |= SECO;
  if (alarma) m.calidad |= RUIDO_SECO;

  if (hayAnterior && m.ultimoAceptado_us > 0) {
    m.edadPulso_us = m.t_us - m.ultimoAceptado_us;
  }
  const bool sinSenal = !hayAnterior || m.edadPulso_us < 0 || m.edadPulso_us > (int64_t)_cfg.timeout_us;
  if (sinSenal) {
    m.calidad |= SIN_SENAL;
  }

  // En modo seco no se genera estimación de caudal
  if (!seco && !sinSenal && periodo > 0) {
    m.calidad |= PERIODO_DISPONIBLE;
    m.frecuencia_Hz = (periodos > 0 && suma > 0) ?
                      (1.0e6 * (double)periodos / (double)suma) :
                      (1.0e6 / (double)periodo);
    m.q_mLmin = (1000.0 * m.frecuencia_Hz) / m.k;

    if (m.q_mLmin > _cfg.qMaxOperativo_mLmin) {
      m.calidad |= FUERA_RANGO_OPERATIVO;
    }

    if (_cfg.calibracionDocumentada) {
      if (m.q_mLmin < _cfg.qMinCal_mLmin || m.q_mLmin > _cfg.qMaxCal_mLmin) {
        m.calidad |= FUERA_RANGO_CAL;
      } else {
        m.calidad |= MEDICION_EN_RANGO_CAL;
      }
    }
  }

  // Volumen totalizado exacto a partir del totalizador de pulsos acumulados (sin errores de redondeo)
  m.volumenEstimado_L = (double)m.pulsosMedicion / (60.0 * m.k);
  _volumenEstimadoAcumulado_L = m.volumenEstimado_L;

  const bool nuevos = (m.pulsosMedicion != _anteriorMedicion);
  _anteriorMedicion = m.pulsosMedicion;
  if (!seco && !bombaEmpuja && (nuevos || std::isfinite(m.q_mLmin))) {
    m.calidad |= SIN_IMPULSION;
  }

  // Suavizado EMA solo para indicación visual en SCADA
  if (std::isfinite(m.q_mLmin) && m.q_mLmin > 0.0) {
    _qSuavizadoDisplay = (_qSuavizadoDisplay == 0.0f) ? (float)m.q_mLmin : (0.25f * (float)m.q_mLmin + 0.75f * _qSuavizadoDisplay);
  } else {
    _qSuavizadoDisplay = 0.0f;
  }

  // Detección de fallo en alimentación si la bomba gira pero no se detectan pulsos en 5 s
  if (_validosVentana > 0 || !bombaEmpuja || !_esAlimentacion) {
    _sinPulso_s = 0.0f;
    _falloAlimentacion = false;
  } else {
    _sinPulso_s += 1.0f; // periodo de captura aproximado
    _falloAlimentacion = (_sinPulso_s >= 5.0f);
  }

  _ultimaMuestra = m;
  return m;
}

float Caudalimetro::caudal_mLmin() const {
  return _qSuavizadoDisplay;
}

float Caudalimetro::caudal_Lmin() const {
  return _qSuavizadoDisplay / 1000.0f;
}

float Caudalimetro::frecuencia_Hz() const {
  return std::isfinite(_ultimaMuestra.frecuencia_Hz) ? (float)_ultimaMuestra.frecuencia_Hz : 0.0f;
}

double Caudalimetro::caudalMedio_mLmin(const Muestra& inicial, const Muestra& final) {
  if (!(inicial.calidad & INICIADO) || !(final.calidad & INICIADO) ||
      inicial.epoca != final.epoca ||
      (inicial.calidad & SECO) || (final.calidad & SECO) ||
      !std::isfinite(inicial.k) || inicial.k <= 0.0 || inicial.k != final.k ||
      final.t_us <= inicial.t_us ||
      final.pulsosMedicion < inicial.pulsosMedicion) {
    return NAN;
  }

  const uint64_t dn = final.pulsosMedicion - inicial.pulsosMedicion;
  const int64_t  dt = final.t_us - inicial.t_us;
  return ((double)dn * 1.0e9) / (inicial.k * (double)dt);
}

```

#### ARCHIVO 4: registro_ensayos.h
```cpp
#pragma once
// ==============================================================================
// REGISTRO DE ENSAYOS MULTI-SESIÓN SIN SOBREESCRITURA (Auditoría GPT Astra)
// - Estructura en RAM de 600 registros y hasta 20 sesiones de ensayo
// - Diferenciación estricta de eventos: INICIO (t=0), PERIODICA (10s) y FIN
// - Reserva fija de plaza para garantizar que el evento FIN nunca se descarte
// - Registro atómico de instantáneas Muestra independientes para cada sensor
// - Exportación limpia a CSV sin asignación dinámica de memoria (heap seguro)
// ==============================================================================

#include <Arduino.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_timer.h"
#include "caudalimetro.h"
#include "config.h"

class RegistroEnsayos {
public:
  enum class Tipo : uint8_t {
    INICIO    = 0,
    PERIODICA = 1,
    FIN       = 2
  };

  enum Estado : uint16_t {
    NORMAL       = 0,
    CAMBIO_EPOCA = (1U << 0),
    OMISIONES    = (1U << 1)
  };

  struct Fila {
    uint16_t sesion = 0;
    Tipo     tipo = Tipo::INICIO;
    uint16_t estado = 0;
    int64_t  tRegistro_us = 0;
    double   rpm = 0.0;
    Caudalimetro::Muestra a;
    Caudalimetro::Muestra p;
  };

  static constexpr size_t   MAX_FILAS = 300;
  static constexpr uint16_t MAX_SESIONES = 20;
  static constexpr int64_t  INTERVALO_LOG_US = 10000000; // 10 segundos

  RegistroEnsayos() {
    _filas = (Fila*)malloc(MAX_FILAS * sizeof(Fila));
    if (_filas) {
      for (size_t i = 0; i < MAX_FILAS; ++i) {
        _filas[i] = Fila{};
      }
    }
  }

  bool     activo()              const { return _activo; }
  bool     incompleto()          const { return _incompleto; }
  size_t   cantidad()            const { return _n; }
  uint64_t intervalosOmitidos()  const { return _omitidos; }
  uint16_t sesionActual()        const { return _sesion; }
  const Fila* fila(size_t i)     const { return (_filas && i < _n) ? &_filas[i] : nullptr; }

  bool iniciar(Caudalimetro& a, Caudalimetro& p, double rpm, bool bombaEmpuja) {
    if (!_filas || _activo || _sesion >= MAX_SESIONES || _n + 2 > MAX_FILAS ||
        !std::isfinite(rpm) || rpm < 0.0) {
      return false;
    }

    const int64_t origen = esp_timer_get_time();
    const auto ma = a.capturar(bombaEmpuja);
    const auto mp = p.capturar(bombaEmpuja);
    if (!iniciadas(ma, mp)) return false;

    _sesion++;
    _activo = true;
    _inicio_us = origen;
    _proximo_us = origen + INTERVALO_LOG_US;
    _epocaA = ma.epoca;
    _epocaP = mp.epoca;
    _estadoSesion = NORMAL;

    guardar(Tipo::INICIO, 0, rpm, ma, mp);
    return true;
  }

  bool tick(Caudalimetro& a, Caudalimetro& p, double rpm, bool bombaEmpuja) {
    if (!_activo) return false;
    const int64_t ahora = esp_timer_get_time();
    if (ahora < _proximo_us) return false;

    const uint64_t vencidos = (uint64_t)((ahora - _proximo_us) / INTERVALO_LOG_US) + 1;
    _proximo_us += (int64_t)vencidos * INTERVALO_LOG_US;

    // Se reserva 1 fila fija para FIN
    if (_n >= MAX_FILAS - 1 || !std::isfinite(rpm) || rpm < 0.0) {
      omitir(vencidos);
      return false;
    }

    const auto ma = a.capturar(bombaEmpuja);
    const auto mp = p.capturar(bombaEmpuja);
    if (!iniciadas(ma, mp)) {
      omitir(vencidos);
      return false;
    }

    if (vencidos > 1) {
      omitir(vencidos - 1);
    }

    guardar(Tipo::PERIODICA, esp_timer_get_time() - _inicio_us, rpm, ma, mp);
    return true;
  }

  bool finalizar(Caudalimetro& a, Caudalimetro& p, double rpm, bool bombaEmpuja) {
    if (!_activo || !std::isfinite(rpm) || rpm < 0.0) return false;

    const auto ma = a.capturar(bombaEmpuja);
    const auto mp = p.capturar(bombaEmpuja);
    if (!iniciadas(ma, mp)) return false;

    const int64_t ahora = esp_timer_get_time();
    if (ahora >= _proximo_us) {
      const uint64_t vencidos = (uint64_t)((ahora - _proximo_us) / INTERVALO_LOG_US) + 1;
      omitir(vencidos);
    }

    guardar(Tipo::FIN, ahora - _inicio_us, rpm, ma, mp);
    _activo = false;
    return true;
  }

  bool limpiar() {
    if (_activo) return false;
    _n = 0;
    _sesion = 0;
    _omitidos = 0;
    _incompleto = false;
    _estadoSesion = NORMAL;
    return true;
  }

  bool exportarCSV(Print& salida) const {
    if (_activo) return false;
    salida.clearWriteError();
    salida.println("# esquema=metrologia_nucleo_v5_1");
    salida.println("# tiempo_sensor_us=desde_arranque");
    salida.println("# volumen_L=estimados_por_pulsos_acumulados");
    salida.println("# nan=dato_no_disponible");
    salida.print("# incompleto=");
    salida.println(_incompleto ? "1" : "0");
    salida.print("# intervalos_omitidos=");
    imprimirU64(salida, _omitidos);
    salida.println();

    salida.println("sesion,tipo,estado,t_reg_us,rpm,"
                   "tA_us,epocaA,calidadA,kA,flancosA,aceptadosA,nA,rechazosA,edadA_us,fA_Hz,qA_mLmin,vA_L,"
                   "tP_us,epocaP,calidadP,kP,flancosP,aceptadosP,nP,rechazosP,edadP_us,fP_Hz,qP_mLmin,vP_L");

    for (size_t i = 0; i < _n; ++i) {
      const Fila& r = _filas[i];
      salida.print(r.sesion); salida.print(',');
      salida.print(static_cast<unsigned>(r.tipo)); salida.print(',');
      salida.print(r.estado); salida.print(',');
      imprimirI64(salida, r.tRegistro_us); salida.print(',');
      imprimirReal(salida, r.rpm); salida.print(',');
      imprimirMuestra(salida, r.a); salida.print(',');
      imprimirMuestra(salida, r.p);
      salida.println();
    }
    return (salida.getWriteError() == 0);
  }

private:
  Fila*    _filas = nullptr;
  size_t   _n = 0;
  uint16_t _sesion = 0;
  bool     _activo = false;
  bool     _incompleto = false;
  uint64_t _omitidos = 0;
  int64_t  _inicio_us = 0;
  int64_t  _proximo_us = 0;
  uint32_t _epocaA = 0;
  uint32_t _epocaP = 0;
  uint16_t _estadoSesion = NORMAL;

  static bool iniciadas(const Caudalimetro::Muestra& a, const Caudalimetro::Muestra& p) {
    return ((a.calidad & Caudalimetro::INICIADO) && (p.calidad & Caudalimetro::INICIADO));
  }

  void omitir(uint64_t cantidad) {
    if (cantidad == 0) return;
    _omitidos += cantidad;
    _incompleto = true;
    _estadoSesion |= OMISIONES;
  }

  void guardar(Tipo tipo, int64_t relativo, double rpm, const Caudalimetro::Muestra& a, const Caudalimetro::Muestra& p) {
    if (!_filas || _n >= MAX_FILAS) {
      _incompleto = true;
      return;
    }
    if (a.epoca != _epocaA || p.epoca != _epocaP) {
      _estadoSesion |= CAMBIO_EPOCA;
      _incompleto = true;
    }

    Fila& r = _filas[_n++];
    r.sesion       = _sesion;
    r.tipo         = tipo;
    r.estado       = _estadoSesion;
    r.tRegistro_us = relativo;
    r.rpm          = rpm;
    r.a            = a;
    r.p            = p;
  }

  static void imprimirU64(Print& s, uint64_t valor) {
    char texto[24];
    snprintf(texto, sizeof(texto), "%llu", (unsigned long long)valor);
    s.print(texto);
  }

  static void imprimirI64(Print& s, int64_t valor) {
    char texto[24];
    snprintf(texto, sizeof(texto), "%lld", (long long)valor);
    s.print(texto);
  }

  static void imprimirReal(Print& s, double valor) {
    if (std::isfinite(valor)) {
      s.print(valor, 4);
    } else {
      s.print("nan");
    }
  }

  static void imprimirMuestra(Print& s, const Caudalimetro::Muestra& m) {
    imprimirI64(s, m.t_us);              s.print(',');
    s.print(m.epoca);                    s.print(',');
    s.print(m.calidad);                  s.print(',');
    imprimirReal(s, m.k);                s.print(',');
    imprimirU64(s, m.flancos);           s.print(',');
    imprimirU64(s, m.aceptados);         s.print(',');
    imprimirU64(s, m.pulsosMedicion);    s.print(',');
    imprimirU64(s, m.rechazados);        s.print(',');
    imprimirI64(s, m.edadPulso_us);      s.print(',');
    imprimirReal(s, m.frecuencia_Hz);    s.print(',');
    imprimirReal(s, m.q_mLmin);          s.print(',');
    imprimirReal(s, m.volumenEstimado_L);
  }
};

```

#### ARCHIVO 5: EN_USO_firmware_planta.ino (PROGRAMA PRINCIPAL DE PLANTA)
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
#include "registro_ensayos.h"
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

// Instanciación de componentes con especificaciones metrológicas de GPT Astra
Caudalimetro sensorAlimentacion(SENSOR_ALIM_CFG, "ALIMENTACION", true);
Caudalimetro sensorPermeado(SENSOR_PERM_CFG, "PERMEADO", false);
RegistroEnsayos registroEnsayos;
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

// Redirección Automática de Portal Cautivo:
// Hace que al conectarse al Wi-Fi, Android / iOS / Windows abran automáticamente el SCADA
void handleCaptivePortal() {
  String host = server.hostHeader();
  if (host.indexOf("192.168.4.1") >= 0 || host.indexOf("bomba.local") >= 0) {
    handleRoot();
  } else {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
  }
}

void handleStatus() {
  uint32_t t_act_s = (tInicioEnsayo_ms > 0) ? ((millis() - tInicioEnsayo_ms) / 1000) : 0;
  uint8_t prog = (uint8_t)((autoCalMuestras * 100) / MUESTRAS_AUTO_CAL);
  if (prog > 100) prog = 100;

  float qAlim = sensorAlimentacion.caudal_mLmin();
  float qPerm = sensorPermeado.caudal_mLmin();
  jLMH_actual = (qPerm * 0.06f) / AREA_MEMBRANA_M2;

  char buf[960];
  int n = snprintf(buf, sizeof(buf),
    "{"
    "\"rpm\":%.1f,\"obj_rpm\":%.1f,\"on\":%s,\"inv\":%s,\"dir\":%s,\"en_regimen\":%s,\"emergencia\":%s,"
    "\"ip\":\"%s\",\"alim_ok\":%s,\"perm_ok\":%s,\"f_alim\":%.2f,\"q_alim\":%.1f,\"vol_alim\":%.4f,"
    "\"f_perm\":%.2f,\"q_perm\":%.1f,\"vol_perm\":%.4f,\"q_ret\":%.1f,\"recov\":%.2f,"
    "\"flan_alim\":%lu,\"val_alim\":%lu,\"gl_alim\":%lu,"
    "\"flan_perm\":%lu,\"val_perm\":%lu,\"gl_perm\":%lu,"
    "\"modo_seco\":%s,\"ruido_seco\":%s,"
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
    (unsigned long)sensorAlimentacion.flancosBrutos(), (unsigned long)sensorAlimentacion.pulsosValidos(), (unsigned long)sensorAlimentacion.glitchesVentana(),
    (unsigned long)sensorPermeado.flancosBrutos(), (unsigned long)sensorPermeado.pulsosValidos(), (unsigned long)sensorPermeado.glitchesVentana(),
    sensorPermeado.modoSeco() ? "true" : "false", sensorPermeado.ruidoDetectadoEnSeco() ? "true" : "false",
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
  } else if (act == "MODO_SECO_ON") {
    sensorPermeado.setModoSeco(true);
    sensorAlimentacion.setModoSeco(true);
    Serial.println("[AUDITORIA] Modo Seco ACTIVADO");
  } else if (act == "MODO_SECO_OFF") {
    sensorPermeado.setModoSeco(false);
    sensorAlimentacion.setModoSeco(false);
    Serial.println("[AUDITORIA] Modo Seco DESACTIVADO");
  } else if (act == "LIMPIAR_ALARMA_SECO") {
    sensorPermeado.limpiarAlarmaSeco();
    sensorAlimentacion.limpiarAlarmaSeco();
    Serial.println("[AUDITORIA] Alarmas de Ruido Seco Limpiadas");
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
  registroEnsayos.limpiar();
  server.send(200, "text/plain", "LOGS_CLEARED");
}

// Exportación CSV con Esquema Metrológico Certificado (GPT Astra)
void handleExportMetrologia() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader("Content-Type", "text/csv; charset=UTF-8");
  server.sendHeader("Content-Disposition", "attachment; filename=\"PlantaUF_Metrologia_Certificada.csv\"");
  server.sendHeader("Connection", "close");
  server.send(200, "text/csv; charset=UTF-8", "");
  WiFiClient client = server.client();
  registroEnsayos.exportarCSV(client);
  server.sendContent("");
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

  sensorAlimentacion.declararCalibrado(SENSOR_ALIM_CFG.calibracionDocumentada);
  sensorPermeado.declararCalibrado(SENSOR_PERM_CFG.calibracionDocumentada);
  const bool inicioAlim = sensorAlimentacion.begin();
  const bool inicioPerm = sensorPermeado.begin();
  if (!inicioAlim || !inicioPerm) {
    Serial.println("⚠️ [ERROR CRÍTICO] Fallo en la inicialización de caudalímetros");
  }
  bomba.begin();

  // 3. Configuración Wi-Fi Robusta (AP Dedicado Anti-Desconexión)
  WiFi.disconnect(true);           // Limpiar estados previos
  delay(100);
  WiFi.mode(WIFI_AP);              // Modo AP Puro (evita escaneos STA que botan clientes)
  WiFi.setSleep(false);            // CRÍTICO: Desactiva ahorro de energía del módem
  WiFi.setTxPower(WIFI_POWER_19_5dBm); // Máxima potencia de transmisión RF

  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(SSID_AP, PASS_AP, 1, 0, 4); // Canal 1 fijo, SSID visible, hasta 4 clientes

  Serial.println("[WIFI] Punto de Acceso Estable Creado:");
  Serial.printf("       SSID: %s | Pass: %s\n", SSID_AP, PASS_AP);
  Serial.printf("       IP AP: http://%s\n", WiFi.softAPIP().toString().c_str());

  if (MDNS.begin("bomba")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("[mDNS] Servidor publicado en: http://bomba.local");
  }

  // 4. Servidor DNS para Portal Cautivo Anti-Desconexión
  // ¿POR QUÉ SE HACE ESTO?
  // Sistemas operativos modernos (Android, iOS, Windows) envían consultas DNS ocultas
  // (p. ej. connectivitycheck.gstatic.com, msftconnecttest.com) para verificar si la red tiene Internet.
  // Al no haber conexión externa, el teléfono/PC asume que el Wi-Fi está roto y se desconecta solo.
  // Este servidor DNS responde a cualquier dominio ("*") con la IP local 192.168.4.1.
  // El sistema operativo reconoce la red como "Portal Cautivo" (estilo hotel/aeropuerto),
  // ANULA el descarte automático y mantiene la conexión Wi-Fi fija y permanente al SCADA.
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
  server.on("/export_metrologia", HTTP_GET, handleExportMetrologia);
  server.on("/clear_csv", HTTP_GET, handleClearCSV);

  // Rutas de sondeo de conectividad de sistemas operativos para Portal Cautivo
  server.on("/generate_204", HTTP_GET, handleCaptivePortal);        // Android Captive Portal Check
  server.on("/gen_204", HTTP_GET, handleCaptivePortal);             // Android alternativo
  server.on("/ncsi.txt", HTTP_GET, handleCaptivePortal);            // Windows Network Connectivity Status
  server.on("/connecttest.txt", HTTP_GET, handleCaptivePortal);     // Windows alternativo
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptivePortal); // Apple iOS / macOS
  server.onNotFound(handleCaptivePortal);                           // Redirección 302 automática al SCADA

  server.begin();
  Serial.println("[HTTP] Servidor Web SCADA iniciado con exito en puerto 80.\n");

  tLoop = millis();
  tCaudal = millis();
  tDatalogger = millis();
  tInicioEnsayo_ms = 0; // Se inicializa en 0 hasta que el operador inicie la primera prueba
}

// ------------------------------------------------------------------------------
// BUCLE PRINCIPAL (LOOP NO BLOQUEANTE)
// ------------------------------------------------------------------------------
void loop() {
  dnsServer.processNextRequest(); // Atiende consultas DNS en < 2 us sin bloquear la ejecucion
  server.handleClient();

  uint32_t tAhora = millis();

  // 1. Control cinemático de la bomba cada 50 ms (Rampa S-Curve Progresiva)
  if (tAhora - tLoop >= 50) {
    float dt = (tAhora - tLoop) / 1000.0f;
    tLoop = tAhora;
    bomba.tick(dt);

    // Testigo LED onboard
    digitalWrite(PIN_LED_BOMBA, (bomba.rpmActual() > 0.5f) ? HIGH : LOW);

    // Detección de flancos de la bomba para registro de ensayos
    bool enMarcha = bomba.enMarcha();
    
    // Flanco de subida: Iniciar sesión de ensayo
    if (enMarcha && !bombaEnMarchaAnterior) {
      iniciarNuevoEnsayo(bomba.rpmObjetivo());
      registroEnsayos.iniciar(sensorAlimentacion, sensorPermeado, bomba.rpmObjetivo(), true);
    }
    
    // Flanco de bajada: Finalizar sesión y registrar
    if (!enMarcha && bombaEnMarchaAnterior) {
      finalizarEnsayoActual();
      registroEnsayos.finalizar(sensorAlimentacion, sensorPermeado, 0.0, false);
    }

    bombaEnMarchaAnterior = enMarcha;
  }

  // 2. Adquisición y cálculo de caudales cada 1000 ms (1 segundo)
  if (tAhora - tCaudal >= 1000) {
    tCaudal = tAhora;

    bool bombaEmpuja = (bomba.rpmActual() > 1.0f);
    float qBomba = bomba.caudalTeorico_mLmin();

    sensorAlimentacion.capturar(bombaEmpuja);
    sensorPermeado.capturar(bombaEmpuja);
    registroEnsayos.tick(sensorAlimentacion, sensorPermeado, bomba.rpmActual(), bombaEmpuja);

    float qAlim = sensorAlimentacion.caudal_mLmin();
    float qPerm = sensorPermeado.caudal_mLmin();

    // Sanity-check de cruce de sensores (el permeado no puede exceder físicamente al caudal de alimentación)
    flagCruceSensores = (qPerm > qAlim + 50.0f) && (bomba.rpmActual() > 5.0f);
    if (flagCruceSensores) {
      Serial.printf("⚠️ [ALERTA] Cruce de cables o sensor invertido: Q_Perm (%.1f mL/min) > Q_Alim (%.1f mL/min)\n", qPerm, qAlim);
    }

    // Balance Hidráulico Tangencial y Flujo Darcy en tiempo real
    qRet_mLmin   = fmaxf(0.0f, qAlim - qPerm);
    recuperacion = (qAlim > 50.0f) ? ((qPerm / qAlim) * 100.0f) : 0.0f;
    jLMH_actual  = (qPerm * 0.06f) / AREA_MEMBRANA_M2;

    // NOTA AUDITORÍA: El modelo de Darcy requiere TMP real medida por transductores de presión (Subhito 2.3).
    // Para evitar fabricar datos sintéticos en el registro de calibración, Darcy permanece en reposo
    // hasta la integración del bus I2C / ADS1115.
    resultadoDarcy = ResultadoDarcy{};

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
          // NOTA METROLÓGICA (Auditoría Ronda 4/5): Permeado depende de Darcy y ensuciamiento,
          // no de RPM de la bomba. Se calibra con balanza gravimétrica independiente.
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

    // Telemetría periódica por Serial (incluye métricas de diagnóstico anti-EMI Ronda 6)
    Serial.printf("[TELEMETRIA] RPM: %4.1f | %s | Q_Alim: %5.1f mL/min (F:%lu, V:%lu, G:%lu) | Q_Perm: %5.1f mL/min (F:%lu, V:%lu, G:%lu) | J: %4.2f LMH | Y: %4.1f%%\n",
                  bomba.rpmActual(), bomba.enRegimenEstable() ? "ESTABLE" : "RAMPA",
                  qAlim, (unsigned long)sensorAlimentacion.flancosBrutos(), (unsigned long)sensorAlimentacion.pulsosValidos(), (unsigned long)sensorAlimentacion.glitchesVentana(),
                  qPerm, (unsigned long)sensorPermeado.flancosBrutos(), (unsigned long)sensorPermeado.pulsosValidos(), (unsigned long)sensorPermeado.glitchesVentana(),
                  jLMH_actual, recuperacion);
  }

  // 3. Muestreo del Datalogger cada 10 segundos (SOLO mientras la bomba está en marcha)
  // Al presionar STOP, se corta el registro para evitar muestras espurias por flujo residual
  if (tAhora - tDatalogger >= INTERVALO_LOG_MS) {
    tDatalogger = tAhora;
    if (bomba.enMarcha()) {
      guardarMuestraDatalogger();
    }
  }
}

```

#### ARCHIVO 6: banco_metrologia.ino (SKETCH DE DIAGNÓSTICO AISLADO DE BANCO)
```cpp
#include <Arduino.h>
#include "driver/gpio.h"
#include "esp_timer.h"
#include "config.h"
#include "caudalimetro.h"
#include "registro_ensayos.h"

// ==============================================================================
// BANCO DE COMPROBACIÓN METROLÓGICA (Auditoría GPT Astra - V5.1)
// - Programa aislado para verificar adquisición, filtros de ruido y registro
// - No acciona el motor (ENA deshabilitado) ni enciende radio Wi-Fi
// - Permite certificar el comportamiento del sensor y registrar CSV por Serial
// ==============================================================================

static Caudalimetro alimentacion(SENSOR_ALIM_CFG, "ALIMENTACION", true);
static Caudalimetro permeado(SENSOR_PERM_CFG, "PERMEADO", false);
static RegistroEnsayos registro;
static bool listo = false;

static void indicarResultado(const char* operacion, bool ok) {
  Serial.print(operacion);
  Serial.println(ok ? ": OK" : ": NO REALIZADO");
}

void setup() {
  // Mantener driver de motor en estado seguro de parada
  digitalWrite(PIN_ENA, HIGH);
  pinMode(PIN_ENA, OUTPUT);
  digitalWrite(PIN_PUL, LOW);
  pinMode(PIN_PUL, OUTPUT);
  digitalWrite(PIN_DIR, LOW);
  pinMode(PIN_DIR, OUTPUT);

  Serial.begin(115200);
  delay(500);

  Serial.println("\n==================================================");
  Serial.println(" BANCO DE COMPROBACIÓN METROLÓGICA (GPT Astra)   ");
  Serial.println(" No genera pasos de motor. Diagnóstico en reposo. ");
  Serial.println("==================================================");

  alimentacion.declararCalibrado(SENSOR_ALIM_CFG.calibracionDocumentada);
  permeado.declararCalibrado(SENSOR_PERM_CFG.calibracionDocumentada);

  const bool okA = alimentacion.begin();
  const bool okP = permeado.begin();
  if (!okA || !okP) {
    Serial.println("❌ Fallo inicializando sensores; motor detenido.");
    return;
  }

  // Estado inicial de diagnóstico en seco, sin totalizar volumen espurio
  alimentacion.setModoSeco(true);
  permeado.setModoSeco(true);
  listo = true;

  Serial.println("\nComandos disponibles vía Serial:");
  Serial.println("  d : Activar Modo Seco (fuera de sesión)");
  Serial.println("  h : Activar Modo Medición Húmedo (fuera de sesión)");
  Serial.println("  i : Iniciar sesión de ensayo");
  Serial.println("  f : Finalizar sesión de ensayo");
  Serial.println("  e : Exportar CSV metrológico (fuera de sesión)");
  Serial.println("  l : Limpiar registros en RAM (fuera de sesión)");
  Serial.println("  a : Limpiar alarmas de ruido seco");
  Serial.println("--------------------------------------------------\n");
}

void loop() {
  if (!listo) {
    delay(10);
    return;
  }

  constexpr bool bombaEmpuja = false;
  constexpr double rpm = 0.0;

  registro.tick(alimentacion, permeado, rpm, bombaEmpuja);

  if (Serial.available()) {
    const char comando = static_cast<char>(Serial.read());
    switch (comando) {
      case 'd':
      case 'h': {
        if (registro.activo()) {
          Serial.println("Finalice la sesión antes de cambiar de modo.");
          break;
        }
        const bool seco = (comando == 'd');
        alimentacion.setModoSeco(seco);
        permeado.setModoSeco(seco);
        Serial.println(seco ? "Modo seco activado." : "Modo medicion humedo activado.");
        break;
      }
      case 'i':
        indicarResultado("Inicio de sesión", registro.iniciar(alimentacion, permeado, rpm, bombaEmpuja));
        break;
      case 'f':
        indicarResultado("Fin de sesión", registro.finalizar(alimentacion, permeado, rpm, bombaEmpuja));
        break;
      case 'e':
        if (!registro.activo()) {
          registro.exportarCSV(Serial);
        } else {
          Serial.println("Finalice la sesión antes de exportar CSV.");
        }
        break;
      case 'l':
        if (!registro.activo()) {
          indicarResultado("Limpieza de registro", registro.limpiar());
        } else {
          Serial.println("Finalice la sesión antes de limpiar.");
        }
        break;
      case 'a':
        alimentacion.limpiarAlarmaSeco();
        permeado.limpiarAlarmaSeco();
        Serial.println("Alarmas de ruido seco limpiadas.");
        break;
      default:
        break;
    }
  }

  delay(1);
}

```
---

### 5. SOLICITUD DE DICTAMEN TÉCNICO FORMAL
Habiendo resuelto la totalidad de los hallazgos previos y presentado el expediente completo e integrado:
1. ¿Consideras que el Firmware V5.2 cumple con los requisitos de integridad metrológica, trazabilidad del dato primario, aislamiento de ruido en seco y seguridad operativa para ser **APROBADO** para la campaña de ensayos de caracterización experimental en banco de pruebas de laboratorio?
2. Si identificas algún aspecto puntual susceptible de optimización, por favor detalla tu fundamento metrológico y provee el fragmento de código de reemplazo correspondiente.
