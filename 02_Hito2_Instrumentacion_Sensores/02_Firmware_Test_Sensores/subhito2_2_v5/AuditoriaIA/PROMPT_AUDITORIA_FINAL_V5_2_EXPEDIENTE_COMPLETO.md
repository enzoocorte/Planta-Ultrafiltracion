# EXPEDIENTE DE AUDITORÍA TÉCNICA — FIRMWARE V5.2 (SUBHITO 2.2)
## BANCO EXPERIMENTAL DE ULTRAFILTRACIÓN TANGENCIAL (TESIS DE GRADO EN INGENIERÍA INDUSTRIAL - UNSA 2026)

---

### 1. ALCANCE Y CONTEXTO DEL DICTAMEN SOLICITADO
Estimado auditor de IA (GPT Astra / use.ai / Claude / ChatGPT):

Por favor, analiza este expediente técnico completo y emite tu dictamen para **habilitar la compilación, carga en el ESP32 y ejecución de la campaña de ensayos de calibración en banco de laboratorio supervisado**.

> **Aclaración de Alcance Metrológico y de Seguridad:**
> 1. Este firmware **NO** es para uso clínico en pacientes ni para una planta de producción industrial no atendida.
> 2. Su propósito exclusivo es la **campaña experimental de calibración metrológica en banco de laboratorio universitario supervisado** con recirculación atmosférica de agua desmineralizada a baja presión.
> 3. La campaña cuenta con **protección hidráulica pasiva independiente** (recirculación abierta a presión atmosférica sin estrangulamiento de retentado) y supervisión manual continua del operador.
> 4. Los términos "calibración" o "validación" se entienden en el marco de la caracterización experimental del Hito 2.2 de la tesis, sujeta a verificación empírica con probeta graduada y cronómetro.

#### A. Marco Institucional y Autores
- **Proyecto:** Tesis de Grado para optar al título de Ingeniero Industrial.
- **Institución:** Universidad Nacional de Salta (UNSa), Facultad de Ingeniería, Salta, Argentina (2026).
- **Tesistas:** Antonella Guitián & Owen Cañizares.
- **Codirector:** Ing. Enzo Corte.
- **Objeto de Estudio:** Banco experimental de ultrafiltración tangencial con bomba peristáltica (motor paso a paso NEMA 23/34 y driver DM860 en cátodo común) y cartucho capilar Fresenius FX100 (membrana Helixone® de fibra hueca, 2.2 m² de área interfacial).

#### B. Arquitectura de Hardware y Pines (Aserción Estática de Cero Colisiones)
- **Driver DM860:** Pulsos STEP en GPIO 18 (generados por periférico hardware LEDC PWM), Dirección DIR en GPIO 19, Habilitación ENA en GPIO 23 (HIGH = inhibición física < 1 ms).
- **Caudalímetro de Alimentación (YF-S401):** GPIO 14 (Borne 12, pull-up externo 4.7 kΩ a 3.3V + filtro RC pasa-bajos pasivo de 200 nF a GND).
- **Caudalímetro de Permeado (YF-S401):** GPIO 27 (Borne 11, pull-up externo 4.7 kΩ a 3.3V + filtro RC pasa-bajos pasivo de 200 nF a GND).
- **Bus I2C (Reservado para ADS1115 en Subhito 2.3):** SDA en GPIO 21, SCL en GPIO 22.

---

### 2. MATRIZ DE RESOLUCIÓN INTEGRAL DE LOS 10 HALLAZGOS DE AUDITORÍA (RONDAS 9, 10 Y 11)

En las revisiones previas, señalaste observaciones técnicas críticas. Cada una de ellas ha sido abordada y resuelta en el código adjunto:

| # | Observación de Auditoría | Solución Implementada en el Firmware | Archivo y Ubicación |
|---|---|---|---|
| **1** | **Bug JavaScript en SCADA:** `$('lbl_kp')` no existía en el DOM, provocando excepción unhandled que congelaba la telemetría periódica. | Se incorporó el elemento `<b id="lbl_kp">` en la tarjeta de Permeado del SCADA, visualizando su factor K en tiempo real sin excepciones. | `index_html.cpp` (l. 331) |
| **2** | **Carrera crítica en lectura de 64 bits en tara (`resetVolumen`):** `_pulsosOffset = _pulsosMedicion;` se ejecutaba fuera de la sección crítica. | Lectura protegida con spinlock SMP: `portENTER_CRITICAL(&_mux); _pulsosOffset = _pulsosMedicion; portEXIT_CRITICAL(&_mux);`. | `caudalimetro.cpp` (`resetVolumen`) |
| **3** | **Trazabilidad de calibración NVS en `setup()`:** `declararCalibrado(true)` en `setup()` pisaba la invalidación legítima de `setK(ka)` de NVS. | Se eliminó la llamada redundante en `setup()`. Los parámetros cargados desde NVS permanecen estrictamente como `calibracionDocumentada = false`. | `EN_USO_firmware_planta.ino` (`setup`) |
| **4** | **Cambio de K alteraba volumen acumulado y no cambiaba época:** `setK()` permitía cambiar K sin cerrar época ni proteger la sesión. | 1) `setK()` ahora incrementa `_epoca++`, inicia nueva época de volumen conservando contadores brutos y resetea estimadores con `reiniciarEstimadorLocked()`.<br>2) Se prohíbe reconfigurar K mientras hay un ensayo activo (`registroEnsayos.activo() -> HTTP 409`). | `caudalimetro.cpp` (`setK`), `EN_USO_firmware_planta.ino` |
| **5** | **Validación previa en endpoints REST:** Se aplicaban parámetros parciales antes de comprobar errores. | Los endpoints `/set_dev`, `/reset_dev`, `/iniciar_auto_cal` y `/calibrar_rpm_q` validan exhaustivamente todos los rangos y bloquean reconfiguraciones en marcha o ensayo activo. | `EN_USO_firmware_planta.ino` |
| **6** | **Streaming HTTP metrológico mezclaba `WiFiClient` con chunked:** `sendContent` tras socket TCP rompía el framing HTTP. | Se implementó la clase adaptadora `ServerPrintAdapter : public Print` que escribe buffers formateados exclusivamente mediante `server.sendContent()`. | `EN_USO_firmware_planta.ino` |
| **7** | **Exportación metrológica con sesión activa:** `/export_metrologia` enviaba HTTP 200 y fallaba silenciosamente si había sesión activa. | Si `registroEnsayos.activo()`, responde `HTTP 409 Conflict` avisando que se debe detener la bomba para exportar. | `EN_USO_firmware_planta.ino` |
| **8** | **Retorno ignorado en LEDC:** `ledcChangeFrequency` o `ledcSetup` podían retornar 0 por error y se actualizaba `_fActual`. | Se verifica `uint32_t retF = ledcChangeFrequency(...); if (retF > 0) _fActual = retF;` garantizando consistencia del estado del temporizador. | `Bomba.cpp` (`tick`) |
| **9** | **RPM dinámicas en bordes de ensayo:** `iniciar` recibía consigna teórica y `finalizar` recibía 0.0 durante el frenado. | Ambos eventos registran ahora la velocidad cinemática instantánea real del eje: `bomba.rpmActual()`. | `EN_USO_firmware_planta.ino` |
| **10** | **Retención de último período en bajo flujo:** El estimador no inventa flujo a velocidad cero. | Solo se calcula caudal recíproco si `hayPeriodosNuevos = (periodos > 0 && suma > 0)` en la ventana. Sin pulsos nuevos, la velocidad instantánea cae a 0.0 mL/min. | `caudalimetro.cpp` |
| **11** | **Atributo IRAM en llamada desde ISR:** `reiniciarEstimadorLocked()` se ejecuta desde la interrupción. | Se añadió el atributo `ARDUINO_ISR_ATTR` en la declaración y definición de `reiniciarEstimadorLocked()`. | `caudalimetro.h`, `caudalimetro.cpp` |
| **12** | **Reserva dinámica en `RegistroEnsayos`:** `malloc` sin llamada a constructores. | Se reemplazó por `new (std::nothrow) Fila[MAX_FILAS]{}`, destructor `delete[]`, copia eliminada y bandera `FALLO_CONTROL`. | `registro_ensayos.h` |

---

### 3. EXPEDIENTE COMPLETO DEL CÓDIGO FUENTE (10 ARCHIVOS ÍNTEGROS)

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
  double   qMinDetectable_mLmin;
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
  20.0,                    // Umbral mínimo detectable (rechazo de ruido espurio)
  true,                    // Calibración documentada en laboratorio
  200.0,                   // Q mínimo del intervalo calibrado (mL/min)
  1360.0                   // Q máximo del intervalo calibrado (mL/min)
};

// Sensor de Permeado: mismo modelo de caudalímetro YF-S401 que alimentación; ajuste fino en banco
constexpr ConfigSensor SENSOR_PERM_CFG = {
  PIN_SENSOR_PERMEADO,     // Pin 27
  196.50,                  // K nominal base (idéntico modelo YF-S401; ajustable desde SCADA)
  600,                     // Ancho mínimo de nivel LOW (600 us)
  600,                     // Ancho mínimo de nivel HIGH (600 us)
  2500,                    // Período mínimo admisible (2500 us = 400 Hz)
  5000000,                 // Timeout de pérdida de señal (5 s)
  200.0,                   // Q máximo operativo admisible (mL/min)
  4.0,                     // Umbral mínimo detectable para suprimir lecturas fantasmas
  false,                   // Requiere balanza gravimétrica en Subhito 2.2
  0.0,                     // Q min
  0.0                      // Q max
};

// Constantes globales de K para compatibilidad con NVS
constexpr float K_ALIMENTACION = 196.50f;
constexpr float K_PERMEADO     = 196.50f;

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

---

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
  bool  setQMinDetectable(double nuevoQMin_mLmin);
  float getQMinDetectable()           const { return (float)_cfg.qMinDetectable_mLmin; }
  bool  declararCalibrado(bool valido);

private:
  static void ARDUINO_ISR_ATTR isrPuente(void* arg);
  void ARDUINO_ISR_ATTR isr();
  void ARDUINO_ISR_ATTR reiniciarEstimadorLocked();

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
  uint64_t _pulsosOffset = 0;
  uint32_t _flancosVentana = 0;
  uint32_t _validosVentana = 0;
  uint32_t _rechazosVentana = 0;
  uint64_t _flancosPrevios = 0;
  uint64_t _aceptadosPrevios = 0;
  uint64_t _rechazosPrevios = 0;
  double   _volumenEstimadoAcumulado_L = 0.0;
  float    _qSuavizadoDisplay = 0.0f;
  int64_t  _inicioSinPulso_us = -1;
  bool     _falloAlimentacion = false;
  Muestra  _ultimaMuestra;
};

```

---

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

void ARDUINO_ISR_ATTR Caudalimetro::reiniciarEstimadorLocked() {
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
  if (!std::isfinite(nuevoK) || nuevoK < 10.0 || nuevoK > 1000.0) {
    return false;
  }

  portENTER_CRITICAL(&_mux);
  _cfg.k_Hz_por_Lmin = nuevoK;
  _cfg.calibracionDocumentada = false;
  ++_epoca;

  // Nueva época de volumen: los contadores brutos se conservan
  _pulsosOffset = _pulsosMedicion;
  _anteriorMedicion = _pulsosMedicion;

  reiniciarEstimadorLocked();

  if (_iniciado) {
    _nivelAlto = (digitalRead(_cfg.pin) == HIGH);
    _inicioNivel_us = esp_timer_get_time();
  }
  portEXIT_CRITICAL(&_mux);

  _volumenEstimadoAcumulado_L = 0.0;
  _qSuavizadoDisplay = 0.0f;
  _inicioSinPulso_us = -1;
  _falloAlimentacion = false;

  // No publicar como actual una muestra de la época anterior
  _ultimaMuestra = Muestra{};
  _ultimaMuestra.k = nuevoK;
  _ultimaMuestra.frecuencia_Hz = NAN;
  _ultimaMuestra.q_mLmin = NAN;
  _ultimaMuestra.volumenEstimado_L = NAN;

  return true;
}

bool Caudalimetro::setQMinDetectable(double nuevoQMin_mLmin) {
  if (!std::isfinite(nuevoQMin_mLmin) || nuevoQMin_mLmin < 0.0 ||
      nuevoQMin_mLmin >= _cfg.qMaxOperativo_mLmin) {
    return false;
  }
  _cfg.qMinDetectable_mLmin = nuevoQMin_mLmin;
  return true;
}

bool Caudalimetro::declararCalibrado(bool valido) {
  if (_iniciado) return false;
  _cfg.calibracionDocumentada = valido;
  return true;
}

void Caudalimetro::resetVolumen() {
  portENTER_CRITICAL(&_mux);
  _pulsosOffset = _pulsosMedicion;
  portEXIT_CRITICAL(&_mux);
  _volumenEstimadoAcumulado_L = 0.0;
}

bool Caudalimetro::begin() {
  if (_iniciado) return false;
  if (!GPIO_IS_VALID_GPIO((gpio_num_t)_cfg.pin) ||
      !std::isfinite(_cfg.k_Hz_por_Lmin) || _cfg.k_Hz_por_Lmin <= 0.0 ||
      _cfg.lowMin_us == 0 || _cfg.highMin_us == 0 ||
      _cfg.periodoMin_us == 0 || _cfg.timeout_us < _cfg.periodoMin_us ||
      !std::isfinite(_cfg.qMaxOperativo_mLmin) || _cfg.qMaxOperativo_mLmin <= 0.0 ||
      !std::isfinite(_cfg.qMinDetectable_mLmin) || _cfg.qMinDetectable_mLmin < 0.0 ||
      _cfg.qMinDetectable_mLmin >= _cfg.qMaxOperativo_mLmin) {
    return false;
  }

  if (_cfg.calibracionDocumentada &&
      (!std::isfinite(_cfg.qMinCal_mLmin) || !std::isfinite(_cfg.qMaxCal_mLmin) ||
       _cfg.qMinCal_mLmin < 0.0 || _cfg.qMaxCal_mLmin < _cfg.qMinCal_mLmin)) {
    return false;
  }

  // Pull-up interno + externo: evita pin flotante y lecturas fantasmas por EMI.
  pinMode(_cfg.pin, INPUT_PULLUP);

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
  // Sección 4.2 de Astra: Solo publicar estimación recíproca si hay períodos NUEVOS en la ventana
  const bool hayPeriodosNuevos = (periodos > 0 && suma > 0);

  if (!seco && !sinSenal && hayPeriodosNuevos) {
    m.calidad |= PERIODO_DISPONIBLE;
    m.frecuencia_Hz = 1.0e6 * static_cast<double>(periodos) / static_cast<double>(suma);
    m.q_mLmin = 1000.0 * m.frecuencia_Hz / m.k;

    // Piso metrológico: por debajo de este umbral se considera ruido/no cuantificable.
    if (m.q_mLmin < _cfg.qMinDetectable_mLmin || m.frecuencia_Hz < 0.5) {
      m.q_mLmin = 0.0;
      m.frecuencia_Hz = 0.0;
    }

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

  // Volumen totalizado exacto a partir del totalizador de pulsos acumulados menos el offset de tara
  const uint64_t pulsosNetos = (m.pulsosMedicion > _pulsosOffset) ? (m.pulsosMedicion - _pulsosOffset) : 0;
  m.volumenEstimado_L = (double)pulsosNetos / (60.0 * m.k);
  _volumenEstimadoAcumulado_L = m.volumenEstimado_L;

  const bool nuevos = (m.pulsosMedicion != _anteriorMedicion);
  _anteriorMedicion = m.pulsosMedicion;
  if (!seco && !bombaEmpuja && (nuevos || std::isfinite(m.q_mLmin))) {
    m.calidad |= SIN_IMPULSION;
  }

  // Suavizado EMA solo para indicación visual en SCADA (si no hay flujo nuevo, cae a 0)
  if (std::isfinite(m.q_mLmin) && m.q_mLmin > 0.0) {
    _qSuavizadoDisplay = (_qSuavizadoDisplay == 0.0f) ? (float)m.q_mLmin : (0.25f * (float)m.q_mLmin + 0.75f * _qSuavizadoDisplay);
  } else {
    _qSuavizadoDisplay = 0.0f;
  }

  // Sección 4.3 de Astra: Detección rigurosa de tiempo real transcurrido para fallo de alimentación
  if (!_esAlimentacion || seco || !bombaEmpuja) {
    _inicioSinPulso_us = -1;
    _falloAlimentacion = false;
  } else if (nuevos) {
    _inicioSinPulso_us = (m.ultimoAceptado_us >= 0) ? m.ultimoAceptado_us : m.t_us;
    _falloAlimentacion = false;
  } else {
    if (_inicioSinPulso_us < 0) {
      _inicioSinPulso_us = m.t_us;
    }
    _falloAlimentacion = (m.t_us - _inicioSinPulso_us >= 5000000LL);
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

---

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

#include <new>

class RegistroEnsayos {
public:
  enum class Tipo : uint8_t {
    INICIO    = 0,
    PERIODICA = 1,
    FIN       = 2
  };

  enum Estado : uint16_t {
    NORMAL        = 0,
    CAMBIO_EPOCA  = (1U << 0),
    OMISIONES     = (1U << 1),
    FALLO_CONTROL = (1U << 2)
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
    _filas = new (std::nothrow) Fila[MAX_FILAS]{};
  }

  ~RegistroEnsayos() {
    delete[] _filas;
  }

  RegistroEnsayos(const RegistroEnsayos&) = delete;
  RegistroEnsayos& operator=(const RegistroEnsayos&) = delete;

  bool     disponible()          const { return _filas != nullptr; }
  bool     activo()              const { return _activo; }
  bool     incompleto()          const { return _incompleto; }
  size_t   cantidad()            const { return _n; }
  uint64_t intervalosOmitidos()  const { return _omitidos; }
  uint16_t sesionActual()        const { return _sesion; }
  const Fila* fila(size_t i)     const { return (_filas && i < _n) ? &_filas[i] : nullptr; }

  void marcarFalloControl() {
    _incompleto = true;
    _estadoSesion |= FALLO_CONTROL;
  }

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

  bool tick(const Caudalimetro::Muestra& ma, const Caudalimetro::Muestra& mp, double rpm) {
    if (!_activo) return false;
    const int64_t ahora = esp_timer_get_time();
    if (ahora < _proximo_us) return false;

    const uint64_t vencidos = static_cast<uint64_t>((ahora - _proximo_us) / INTERVALO_LOG_US) + 1;
    _proximo_us += static_cast<int64_t>(vencidos) * INTERVALO_LOG_US;

    if (_n >= MAX_FILAS - 1 || !std::isfinite(rpm) || rpm < 0.0 || !iniciadas(ma, mp)) {
      omitir(vencidos);
      return false;
    }

    if (vencidos > 1) {
      omitir(vencidos - 1);
    }

    guardar(Tipo::PERIODICA, ahora - _inicio_us, rpm, ma, mp);
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

---

#### ARCHIVO 5: Bomba.h
```cpp
#pragma once
#include <Arduino.h>
#include <cmath>
#include "config.h"

// ==============================================================================
// CLASE BOMBA PERISTÁLTICA — DECLARACIÓN
// - Control de frecuencia por hardware LEDC PWM (Driver DM860)
// - Rampa Cuadrática S-Curve de arranque suave (sin golpe de torque)
// - Parada rápida en < 1.5s
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

---

#### ARCHIVO 6: Bomba.cpp
```cpp
#include "Bomba.h"

// ==============================================================================
// IMPLEMENTACIÓN DE LA CLASE BOMBA
// Generación PWM por hardware LEDC con frecuencia mínima segura (>= 50 Hz)
// ==============================================================================

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
    // Si la bomba está arrancando desde 0, iniciamos suavemente en 1.0 RPM (53 Hz LEDC)
    if (_actual < 1.0f) {
      _actual = 1.0f;
    }
    
    // Rampa de aceleración progresiva
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
      // PARADA RÁPIDA: Frenado ágil en menos de 1.5s
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

  // Al llegar a 0 RPM durante una inversión: cortar pulsos, conmutar DIR con margen de seguridad y re-acelerar
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
      uint32_t retF = ledcChangeFrequency(PIN_PUL, f, 10); 
      if (retF > 0) _fActual = retF;
    }
    ledcWrite(PIN_PUL, 512); // Ciclo de trabajo 50%
#else
    if (f != _fActual) { 
      uint32_t retF = ledcSetup(0, f, 10); 
      if (retF > 0) _fActual = retF;
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

---

#### ARCHIVO 7: index_html.h
```cpp
#pragma once
#include <Arduino.h>

// ==============================================================================
// DECLARACIÓN DE LA INTERFAZ WEB SCADA (Flash PROGMEM)
// El contenido completo se encuentra en index_html.cpp.
// Esto permite compilación incremental en GCC/Clang: no se reprocesa el HTML
// cuando se modifica la lógica de la bomba o el firmware en el .ino.
// ==============================================================================

extern const char INDEX_HTML[] PROGMEM;

```

---

#### ARCHIVO 8: index_html.cpp
```cpp
#include "index_html.h"

// ==============================================================================
// DEFINICIÓN DEL CONTENIDO HTML / CSS / JS (PROGMEM)
// Al estar en un archivo .cpp separado, GCC compila este archivo una sola vez
// generando el objeto index_html.cpp.o, acelerando drásticamente la compilación.
// ==============================================================================

const char INDEX_HTML[] PROGMEM = R"html(<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Planta UF - SCADA, Auto-Calibración & Modo Dev</title>
  <style>
    :root {
      --bg: #0b1020;
      --card: #141c30;
      --card-dev: #171d33;
      --border: #263450;
      --text: #e2e8f0;
      --muted: #8296b3;
      --primary: #38bdf8;
      --alim: #06b6d4;
      --perm: #facc15;
      --ret: #f59e0b;
      --success: #10b981;
      --danger: #ef4444;
      --excel: #107c41;
      --dev: #a855f7;
      --autocal: #0ea5e9;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      background: var(--bg);
      color: var(--text);
      padding: 14px;
      display: flex;
      justify-content: center;
    }
    .wrap { max-width: 530px; width: 100%; display: flex; flex-direction: column; gap: 12px; }
    .card {
      background: var(--card);
      border: 1px solid var(--border);
      border-radius: 14px;
      padding: 16px;
      box-shadow: 0 4px 20px rgba(0,0,0,0.4);
    }
    .card-dev {
      border: 1px solid rgba(168,85,247,0.5);
      background: #15162c;
    }
    h1 { font-size: 16px; color: var(--primary); text-align: center; margin-bottom: 2px; }
    .sub { font-size: 11px; color: var(--muted); text-align: center; margin-bottom: 12px; }
    .disp {
      text-align: center;
      background: #0a0f1e;
      border-radius: 10px;
      padding: 12px;
      margin-bottom: 10px;
      border: 1px solid #1a253c;
    }
    .rpm { font-size: 46px; font-weight: 800; color: var(--primary); line-height: 1; }
    .disp u { display: block; font-size: 10px; color: var(--muted); letter-spacing: 2px; text-decoration: none; margin-top: 4px; }
    .pill {
      display: inline-block;
      padding: 3px 12px;
      border-radius: 99px;
      font-size: 10px;
      font-weight: 700;
      margin-top: 8px;
    }
    .off { background: #2a3448; color: #aab7cc; }
    .on  { background: rgba(16,185,129,.25); color: #34d399; }
    .inv { background: rgba(245,158,11,.25); color: #fbbf24; }
    .al {
      display: none;
      background: rgba(239,68,68,.15);
      border: 1px solid var(--danger);
      color: #fca5a5;
      border-radius: 8px;
      padding: 8px;
      font-size: 11px;
      text-align: center;
      margin-bottom: 10px;
    }
    .vis { display: block !important; }
    .row { display: flex; justify-content: space-between; font-size: 11px; color: var(--muted); margin-bottom: 6px; align-items: center; }
    input[type=range] { width: 100%; accent-color: var(--primary); cursor: pointer; }
    .grid { display: grid; grid-template-columns: repeat(5,1fr); gap: 6px; margin: 10px 0; }
    button {
      border: none;
      border-radius: 8px;
      padding: 8px 4px;
      font-weight: 700;
      font-size: 11px;
      cursor: pointer;
      background: #22304e;
      color: var(--text);
      transition: all 0.2s;
    }
    button:hover { filter: brightness(1.15); }
    .btns { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }
    .go { background: var(--success); color: #04301c; }
    .no { background: var(--danger); color: #3d0808; }
    .dir { grid-column: span 2; background: #1c2a45; border: 1px solid var(--border); }
    .btn-excel {
      background: var(--excel);
      color: #ffffff;
      padding: 10px;
      font-size: 12px;
      font-weight: 800;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 6px;
      border-radius: 8px;
      text-decoration: none;
      border: none;
      width: 100%;
      cursor: pointer;
    }
    .btn-excel:hover { background: #0d6535; }
    .btn-dev {
      background: var(--dev);
      color: #ffffff;
      padding: 6px 12px;
      font-size: 11px;
      font-weight: 700;
      border-radius: 6px;
    }
    .btn-autocal {
      background: linear-gradient(135deg, #0284c7, #0369a1);
      color: #ffffff;
      padding: 8px;
      font-size: 11px;
      font-weight: 800;
      border-radius: 6px;
      width: 100%;
      margin-top: 6px;
    }

    /* Sensores dispuestos uno abajo de otro */
    .sens {
      display: flex;
      flex-direction: column;
      gap: 10px;
      margin-top: 8px;
    }
    .s {
      background: #0a0f1e;
      border-radius: 10px;
      padding: 12px 14px;
      border: 1px solid #1a253c;
    }
    .s h3 { font-size: 12px; margin-bottom: 6px; display: flex; justify-content: space-between; align-items: center; }
    .s .val-row { display: flex; justify-content: space-between; align-items: baseline; }
    .s .v { font-size: 26px; font-weight: 800; }
    .s .info-row { display: flex; justify-content: space-between; font-size: 11px; color: var(--muted); margin-top: 4px; }
    .badge { display: none; color: var(--danger); font-size: 9px; font-weight: bold; }
    
    .bal {
      background: #0a0f1e;
      border-radius: 10px;
      padding: 10px;
      margin-top: 10px;
      font-size: 11px;
      border: 1px solid #1a253c;
    }
    .bal div { display: flex; justify-content: space-between; padding: 3px 0; border-bottom: 1px solid #151f32; }
    .bal div:last-child { border-bottom: none; }
    .lbl { color: var(--muted); }
    .val { font-weight: 700; }
    
    .input-group { display: flex; flex-direction: column; gap: 4px; margin-bottom: 8px; }
    .input-group label { font-size: 11px; color: #cbd5e1; font-weight: 600; }
    .input-group input, .input-group select {
      background: #0a0f1e;
      border: 1px solid #263450;
      color: #38bdf8;
      padding: 8px;
      border-radius: 6px;
      font-size: 12px;
      font-weight: 700;
    }
    .input-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }
    
    .table-box {
      margin-top: 10px;
      max-height: 180px;
      overflow-y: auto;
      border: 1px solid #1a253c;
      border-radius: 8px;
      background: #080d1a;
    }
    table { width: 100%; border-collapse: collapse; font-size: 10px; text-align: left; }
    th { background: #131b2e; color: var(--muted); padding: 6px 8px; position: sticky; top: 0; }
    td { padding: 6px 8px; border-bottom: 1px solid #131b2e; color: #e2e8f0; }
    tr:hover { background: #17233d; }

    .toast {
      display: none;
      background: #1e1b4b;
      border: 1px solid #a855f7;
      color: #e9d5ff;
      padding: 10px 14px;
      border-radius: 8px;
      font-size: 11px;
      text-align: center;
      margin-bottom: 8px;
      box-shadow: 0 4px 15px rgba(168,85,247,0.3);
    }

    /* Progress bar for auto-cal */
    .progress-bar-bg {
      background: #0a0f1e;
      border: 1px solid #293855;
      border-radius: 99px;
      height: 14px;
      overflow: hidden;
      margin-top: 6px;
      position: relative;
    }
    .progress-bar-fill {
      background: linear-gradient(90deg, #38bdf8, #a855f7);
      height: 100%;
      width: 0%;
      transition: width 0.3s;
    }
    .progress-text {
      position: absolute;
      width: 100%;
      text-align: center;
      font-size: 9px;
      line-height: 14px;
      color: #fff;
      font-weight: 800;
      top: 0;
      left: 0;
    }
    footer { text-align: center; font-size: 10px; color: var(--muted); padding: 6px; }
  </style>
</head>
<body>
  <div class="wrap">
    
    <!-- CARD 1: CONTROL DE BOMBA PERISTÁLTICA -->
    <div class="card">
      <div class="row" style="margin-bottom:4px">
        <h1 style="text-align:left; margin-bottom:0">PLANTA UF • MBP-2000</h1>
        <button class="btn-dev" onclick="toggleDev()">🛠️ Modo Dev</button>
      </div>
      <p class="sub" style="text-align:left">Rampa Fluida S-Curve • Despegue Suave & Parada Rápida</p>
      
      <div class="disp">
        <div class="rpm" id="rpm">0.0</div>
        <u>RPM INSTANTÁNEA (OBJETIVO: <span id="obj_rpm_lbl" style="color:var(--primary)">50</span> RPM)</u>
        <div class="pill off" id="pil">DETENIDA</div>
      </div>

      <div class="al" id="al">⚠ ALERTA: Caudal Alimentación excede límite (> 1200 mL/min)</div>

      <div class="row">
        <span>Consigna de Operación:</span>
        <span><b id="lc" style="color:var(--primary)">50</b> RPM</span>
      </div>
      <input type="range" id="sl" min="15" max="100" step="1" value="25">
      
      <div class="grid">
        <button class="rpm-btn" data-rpm="25" onclick="setR(25)">25 RPM</button>
        <button class="rpm-btn" data-rpm="35" onclick="setR(35)">35 RPM</button>
        <button class="rpm-btn" data-rpm="44" onclick="setR(44)" title="Límite clínico de sangre 600 mL/min">44 RPM 🩸</button>
        <button class="rpm-btn" data-rpm="50" onclick="setR(50)">50 RPM</button>
        <button class="rpm-btn" data-rpm="75" onclick="setR(75)">75 RPM</button>
        <button class="rpm-btn" data-rpm="100" onclick="setR(100)" title="Diseño factorial de agua (1360 mL/min)">100 RPM 🧪</button>
      </div>

      <div class="btns">
        <button class="go" onclick="cmd('START')">▶ ARRANCAR</button>
        <button class="no" onclick="cmd('STOP')">⏹ PARAR</button>
        <button class="dir" id="bd" onclick="cmd('DIR')">🔄 HORARIO (FILTRACIÓN)</button>
      </div>
    </div>

    <!-- CARD 2: INSTRUMENTACIÓN Y BALANCE HIDRÁULICO (UNO ABAJO DE OTRO) -->
    <div class="card">
      <div class="row">
        <b style="font-size:12px; color:#fff">🌊 CAUDALÍMETROS YF-S401</b>
        <div style="display:flex; gap:6px; align-items:center;">
          <button id="btn_modo_seco" style="font-size:10px; padding:3px 8px; background:#1e293b; border:1px solid #475569" onclick="toggleModoSeco()">🧪 Modo Seco</button>
          <button style="font-size:10px; padding:3px 8px" onclick="cmd('RESET_VOL')">Reset Vol</button>
        </div>
      </div>

      <div class="sens">
        <!-- 1. Sensor Alimentacion (Arriba) -->
        <div class="s" style="border-left: 4px solid var(--alim);">
          <h3 style="color:var(--alim)">
            <span>ALIMENTACIÓN (GPIO 14)</span>
            <span class="badge" id="ba">SIN SEÑAL</span>
          </h3>
          <div class="val-row">
            <div>
              <span class="v" id="va" style="color:var(--alim)">0.0</span>
              <small style="font-size:12px; font-weight:700; color:var(--muted)">mL/min</small>
            </div>
            <div style="font-size:14px; font-weight:700; color:#e2e8f0" id="la">0.000 L</div>
          </div>
          <div class="info-row">
            <span>Frecuencia: <b id="fa" style="color:#e2e8f0">0.0 Hz</b></span>
            <span>Factor K: <b id="lbl_ka" style="color:var(--alim)">154.62</b> Hz/(L/min)</span>
          </div>
        </div>

        <!-- 2. Sensor Permeado (Abajo) -->
        <div class="s" style="border-left: 4px solid var(--perm);">
          <h3 style="color:var(--perm)">
            <span>PERMEADO (GPIO 27)</span>
            <span class="badge" id="bp">SIN SEÑAL</span>
          </h3>
          <div class="val-row">
            <div>
              <span class="v" id="vp" style="color:var(--perm)">0.0</span>
              <small style="font-size:12px; font-weight:700; color:var(--muted)">mL/min</small>
            </div>
            <div style="font-size:14px; font-weight:700; color:#e2e8f0" id="lp">0.000 L</div>
          </div>
          <div class="info-row">
            <span>Frecuencia: <b id="fp" style="color:#e2e8f0">0.0 Hz</b></span>
            <span>Factor K: <b id="lbl_kp" style="color:var(--perm)">196.50</b> Hz/(L/min)</span>
          </div>
          <div class="info-row" style="margin-top:2px">
            <span>Flujo Darcy: <b id="j_lmh" style="color:var(--perm)">0.00 LMH</b></span>
          </div>
        </div>
      </div>

      <!-- Balance Hidráulico -->
      <div class="bal">
        <div>
          <span class="lbl">Caudal Retentado (Qalim − Qperm):</span>
          <span class="val" id="qr" style="color:var(--ret)">0.0 mL/min</span>
        </div>
        <div>
          <span class="lbl">Tasa de Recuperación (Y% = Qperm/Qalim):</span>
          <span class="val" id="rv" style="color:var(--primary)">0.0 %</span>
        </div>
        <div>
          <span class="lbl">Caudal Teórico Bomba:</span>
          <span class="val" id="qb">0.0 mL/min</span>
        </div>
        <div>
          <span class="lbl">Desviación (Δ Bomba vs Alimentación):</span>
          <span class="val" id="db">0.0 %</span>
        </div>
      </div>
    </div>

    <!-- CARD 3: MODO DESARROLLADOR (CIERRA AL GUARDAR) -->
    <div class="card card-dev" id="card_dev" style="display:none">
      <div class="row">
        <b style="font-size:12px; color:var(--dev)">🛠️ CALIBRACIÓN EN CALIENTE Y AUTO-TUNING</b>
        <button style="background:#334155; font-size:10px; padding:2px 8px" onclick="toggleDev()">✕ Cerrar</button>
      </div>
      <div class="toast" id="toast_msg"></div>

      <!-- SECCIÓN A: AUTO-CALIBRACIÓN INTELIGENTE EN MARCHA -->
      <div style="background:#0e1122; padding:10px; border-radius:8px; border:1px solid #293855; margin-bottom:10px">
        <div class="row" style="margin-bottom:2px">
          <b style="font-size:11px; color:#38bdf8">⚡ AUTO-CALIBRACIÓN EN RÉGIMEN PERMANENTE</b>
          <span id="regimen_badge" style="font-size:9px; color:#aab7cc">Esperando estabilidad...</span>
        </div>
        <p style="font-size:10px; color:var(--muted)">Calcula automáticamente los factores K de ambos sensores durante 15s estables a las RPM actuales.</p>
        
        <div id="autocal_progress_box" style="display:none">
          <div class="progress-bar-bg">
            <div class="progress-bar-fill" id="autocal_fill"></div>
            <span class="progress-text" id="autocal_txt">Muestreando... 0%</span>
          </div>
        </div>

        <div style="display:flex; gap:6px; margin-top:6px">
          <button class="btn-autocal" id="btn_start_autocal" onclick="iniciarAutoCal()">
            ▶ Iniciar Auto-Calibración Automática (15s)
          </button>
          <button id="btn_cancel_autocal" style="display:none; background:var(--danger); color:#fff; font-size:10px; padding:6px" onclick="cancelarAutoCal()">
            ✖ Cancelar
          </button>
        </div>
      </div>

      <!-- SECCIÓN B: CALIBRACIÓN EXACTA POR RPM Y CAUDALES MEDIDOS -->
      <div style="background:#0e1122; padding:12px; border-radius:10px; border:1px solid #293855; margin-bottom:10px">
        <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:4px">
          <b style="font-size:12px; color:#facc15">🎯 CALIBRAR POR RPM & CAUDAL DE PROBETA</b>
          <span style="font-size:9px; background:#1e293b; color:#94a3b8; padding:2px 6px; border-radius:4px">Cálculo en ESP32</span>
        </div>
        <p style="font-size:10px; color:var(--muted); margin-bottom:8px">Ingresa las RPM y los caudales reales medidos con probeta para recalcular simultáneamente la cilindrada y los factores K:</p>
        
        <div style="display:flex; flex-direction:column; gap:6px;">
          <!-- 1. RPM -->
          <div style="display:flex; align-items:center; justify-content:space-between; background:#141c30; padding:6px 10px; border-radius:8px; border-left:3px solid var(--primary)">
            <div>
              <div style="font-size:11px; font-weight:700; color:#e2e8f0">1. Velocidad de Bomba</div>
              <div style="font-size:9px; color:var(--muted)">RPM del ensayo cronometrado</div>
            </div>
            <div style="display:flex; align-items:center; gap:4px">
              <input type="number" id="cal_rpm" value="50.0" step="1" style="width:70px; background:#0a0f1e; border:1px solid #263450; color:#38bdf8; padding:5px; border-radius:6px; font-size:12px; font-weight:700; text-align:center;">
              <span style="font-size:10px; color:var(--muted)">RPM</span>
            </div>
          </div>

          <!-- 2. Q Alimentación -->
          <div style="display:flex; align-items:center; justify-content:space-between; background:#141c30; padding:6px 10px; border-radius:8px; border-left:3px solid var(--alim)">
            <div>
              <div style="font-size:11px; font-weight:700; color:var(--alim)">2. Caudal Alimentación (Q alim)</div>
              <div style="font-size:9px; color:var(--muted)">Medido en probeta 1000 mL (vol / min)</div>
            </div>
            <div style="display:flex; align-items:center; gap:4px">
              <input type="number" id="cal_q_alim" value="720.0" step="5" style="width:70px; background:#0a0f1e; border:1px solid #263450; color:var(--alim); padding:5px; border-radius:6px; font-size:12px; font-weight:700; text-align:center;">
              <span style="font-size:10px; color:var(--muted)">mL/min</span>
            </div>
          </div>

          <!-- 3. Q Permeado -->
          <div style="display:flex; align-items:center; justify-content:space-between; background:#141c30; padding:6px 10px; border-radius:8px; border-left:3px solid var(--perm)">
            <div>
              <div style="font-size:11px; font-weight:700; color:var(--perm)">3. Caudal Permeado (Q perm)</div>
              <div style="font-size:9px; color:var(--muted)">Medido en probeta 100 mL (o 0 si no se midió)</div>
            </div>
            <div style="display:flex; align-items:center; gap:4px">
              <input type="number" id="cal_q_perm" value="45.0" step="1" style="width:70px; background:#0a0f1e; border:1px solid #263450; color:var(--perm); padding:5px; border-radius:6px; font-size:12px; font-weight:700; text-align:center;">
              <span style="font-size:10px; color:var(--muted)">mL/min</span>
            </div>
          </div>
        </div>

        <button style="background:#4f46e5; color:#fff; width:100%; padding:9px; font-weight:800; font-size:11px; border-radius:6px; margin-top:8px" onclick="calibrarPorRpmYQ()">
          🚀 Calcular y Guardar Calibración en ESP32
        </button>
      </div>

      <!-- SECCIÓN C: EDICIÓN DIRECTA DE PARÁMETROS -->
      <div class="input-grid">
        <div class="input-group">
          <label>Factor K Alimentación [Hz/(L/min)]:</label>
          <div style="display:flex; gap:4px; align-items:center;">
            <button type="button" style="padding:6px 10px; font-size:14px; font-weight:900; background:#334155" onclick="adjVal('inp_ka', -1.0)">−</button>
            <input type="number" id="inp_ka" step="0.1" value="196.50" style="width:100%; text-align:center;">
            <button type="button" style="padding:6px 10px; font-size:14px; font-weight:900; background:#334155" onclick="adjVal('inp_ka', +1.0)">+</button>
          </div>
        </div>
        <div class="input-group">
          <label>Factor K PERMEADO [Hz/(L/min)]:</label>
          <div style="display:flex; gap:4px; align-items:center;">
            <button type="button" style="padding:6px 10px; font-size:14px; font-weight:900; background:#334155" onclick="adjVal('inp_kp', -1.0)">−</button>
            <input type="number" id="inp_kp" step="0.1" value="196.50" style="width:100%; text-align:center;">
            <button type="button" style="padding:6px 10px; font-weight:900; background:#334155" onclick="adjVal('inp_kp', +1.0)">+</button>
          </div>
        </div>
        <div class="input-group">
          <label>Cilindrada Bomba (mL/rev):</label>
          <input type="number" id="inp_ml" step="0.01" value="13.60">
        </div>
        <div class="input-group">
          <label>Micropasos (Pulsos/Rev):</label>
          <input type="number" id="inp_pul" step="100" value="3200">
        </div>
      </div>

      <div class="btns" style="margin-top:6px">
        <button style="background:var(--dev); color:#fff" onclick="guardarParametros(1)">💾 Guardar en Flash ESP32</button>
        <button style="background:#334155" onclick="guardarParametros(0)">⚡ Aplicar Temporal</button>
        <button style="background:#1e293b; grid-column:span 2; border:1px solid #475569" onclick="resetParametros()">🔄 Restablecer Valores de Fábrica</button>
      </div>
    </div>

    <!-- CARD 4: DATALOGGER Y DESCARGA MULTI-SESIÓN A EXCEL -->
    <div class="card">
      <div class="row">
        <b style="font-size:12px; color:#fff">📊 HISTORIAL DE ENSAYOS & EXCEL (.CSV)</b>
        <button style="font-size:10px; padding:3px 8px; background:#475569" onclick="clearLog()">🗑️ Limpiar Todo</button>
      </div>
      
      <div style="background:#0a0f1e; border:1px solid #1a253c; border-radius:10px; padding:12px; margin-top:8px">
        <div class="row" style="margin-bottom:6px">
          <span>Ensayo actual en curso:</span>
          <span><b id="ensayo_actual_lbl" style="color:var(--primary)">Ensayo #1</b> (<span id="t_log">00:00</span> min)</span>
        </div>
        <div class="row" style="margin-bottom:8px">
          <span>Muestras registradas:</span>
          <span><b id="n_logs" style="color:#f8fafc">0</b> muestras guardadas</span>
        </div>

        <div class="input-group" style="margin-bottom:8px">
          <label style="color:var(--muted)">Seleccionar Ensayo para Descargar:</label>
          <select id="sel_ensayo" style="background:#131c31; border:1px solid #293855; color:#fff; padding:8px; border-radius:6px; font-size:11px">
            <option value="all">📦 Todos los Ensayos (Histórico Completo)</option>
            <option value="actual">🔴 Ensayo Actual en Vivo</option>
          </select>
        </div>

        <button class="btn-excel" onclick="descargarExcel()">
          📥 Descargar Ensayo Seleccionado en Excel (.CSV - 16 cols)
        </button>
        <button style="background:#4338ca; color:#ffffff; padding:10px; font-size:12px; font-weight:800; display:flex; align-items:center; justify-content:center; gap:6px; border-radius:8px; border:none; width:100%; cursor:pointer; margin-top:6px;" onclick="descargarMetrologia()">
          🔬 Descargar Registro Metrológico Completo (29 cols)
        </button>
      </div>

      <!-- Tabla de ensayos registrados al apagar la bomba -->
      <div class="row" style="margin-top:10px; margin-bottom:2px">
        <span style="font-weight:700; color:#cbd5e1">Corridas Registradas al Apagar Bomba:</span>
        <span id="cant_ensayos" style="color:var(--muted)">0 ensayos</span>
      </div>
      <div class="table-box">
        <table id="tabla_ensayos">
          <thead>
            <tr>
              <th>ID</th>
              <th>Consigna</th>
              <th>Duración</th>
              <th>Muestras</th>
              <th>Vol Perm</th>
              <th>Acción</th>
            </tr>
          </thead>
          <tbody id="body_ensayos">
            <tr><td colspan="6" style="text-align:center; color:var(--muted)">No hay ensayos finalizados aún. Arranca y apaga la bomba para registrar.</td></tr>
          </tbody>
        </table>
      </div>
    </div>

    <footer>
      ESP32 UF • <b id="ip">Cargando...</b> • <a href="http://bomba.local" style="color:var(--primary);text-decoration:none">http://bomba.local</a>
    </footer>
  </div>

  <script>
    const $ = id => document.getElementById(id);
    let devOpen = false;
    let rawStatus = {};
    let lastStableFa = 0;
    let lastStableFp = 0;

    function toggleDev() {
      devOpen = !devOpen;
      $('card_dev').style.display = devOpen ? 'block' : 'none';
    }

    function showToast(msg) {
      const t = $('toast_msg');
      t.innerText = msg;
      t.style.display = 'block';
      setTimeout(() => { t.style.display = 'none'; }, 4000);
    }

    function updateRpmHighlight(v) {
      document.querySelectorAll('.rpm-btn').forEach(btn => {
        const isMatch = Math.abs(parseFloat(btn.dataset.rpm) - parseFloat(v)) < 0.1;
        btn.style.border = isMatch ? '1px solid var(--primary)' : '1px solid transparent';
        btn.style.background = isMatch ? '#1a2c4e' : '#22304e';
        btn.style.color = isMatch ? 'var(--primary)' : 'var(--text)';
      });
    }

    function cmd(act) { fetch('/cmd?act=' + act); }
    function setR(v) {
      $('sl').value = v;
      $('lc').innerText = v;
      $('obj_rpm_lbl').innerText = v;
      updateRpmHighlight(v);
      fetch('/set?rpm=' + v);
    }

    function iniciarAutoCal() {
      if (!rawStatus.on) {
        alert("Primero enciende la bomba y deja que alcance las RPM consignadas.");
        return;
      }
      fetch('/iniciar_auto_cal')
        .then(r => r.json())
        .then(res => {
          showToast("⚡ Rutina de Auto-Calibración iniciada. Manteniendo 15s de régimen permanente...");
        });
    }

    function cancelarAutoCal() {
      fetch('/cancelar_auto_cal')
        .then(r => r.json())
        .then(() => showToast("Auto-calibración cancelada."));
    }

    function calibrarPorRpmYQ() {
      const rpm = parseFloat($('cal_rpm').value);
      const qa = parseFloat($('cal_q_alim').value);
      let qp = parseFloat($('cal_q_perm').value);
      if (isNaN(qp) || qp < 0) qp = 0;

      if (isNaN(rpm) || isNaN(qa) || rpm <= 0 || qa <= 0) {
        alert("Ingresa RPM y Caudal de Alimentación válidos mayores a cero.");
        return;
      }

      let fa = (rawStatus && rawStatus.f_alim > 1.0) ? rawStatus.f_alim : lastStableFa;
      let fp = (rawStatus && rawStatus.f_perm > 0.3) ? rawStatus.f_perm : lastStableFp;

      let url = `/calibrar_rpm_q?rpm=${rpm}&qa=${qa}`;
      if (fa > 1.0) url += `&fa=${fa.toFixed(2)}`;
      if (qp > 0) {
        url += `&qp=${qp}`;
        if (fp > 0.3) url += `&fp=${fp.toFixed(2)}`;
      }

      fetch(url)
        .then(r => r.json())
        .then(res => {
          let msg = `🎯 Calibración completada: Cilindrada=${res.ml_rev} mL/rev | K_Alim=${res.k_alim}`;
          if (res.k_perm) msg += ` | K_Perm=${res.k_perm}`;
          showToast(msg);
          // Cerrar automáticamente el Modo Dev al guardar
          setTimeout(() => {
            devOpen = false;
            $('card_dev').style.display = 'none';
          }, 1500);
        })
        .catch(() => alert("Error al comunicar con ESP32"));
    }

    function guardarParametros(guardarNvs) {
      const ka = parseFloat($('inp_ka').value);
      const kp = parseFloat($('inp_kp').value);
      const ml = parseFloat($('inp_ml').value);
      const pul = parseInt($('inp_pul').value);

      if (isNaN(ka) || isNaN(kp) || isNaN(ml) || isNaN(pul)) {
        alert("Por favor ingresa números válidos.");
        return;
      }

      fetch(`/set_dev?ka=${ka}&kp=${kp}&ml=${ml}&pul=${pul}&save=${guardarNvs}`)
        .then(r => r.json())
        .then(res => {
          showToast(guardarNvs ? "✅ Factores guardados en memoria Flash ESP32 (NVS)!" : "⚡ Parámetros aplicados en caliente (RAM).");
          // Cerrar automáticamente el Modo Dev al guardar en Flash
          if (guardarNvs) {
            setTimeout(() => {
              devOpen = false;
              $('card_dev').style.display = 'none';
            }, 1000);
          }
        })
        .catch(() => alert("Error al comunicarse con el ESP32"));
    }

    function resetParametros() {
      if (confirm("¿Restablecer factores K y calibración a los valores de fábrica?")) {
        fetch('/reset_dev')
          .then(r => r.json())
          .then(res => {
            showToast("🔄 Factores de fábrica restablecidos con éxito.");
            setTimeout(() => {
              devOpen = false;
              $('card_dev').style.display = 'none';
            }, 1000);
          });
      }
    }

    function descargarExcel() {
      const sel = $('sel_ensayo').value;
      window.location.href = '/export_csv?ensayo=' + sel;
    }

    function descargarEnsayoId(id) {
      window.location.href = '/export_csv?ensayo=' + id;
    }

    function descargarMetrologia() {
      window.location.href = '/export_metrologia';
    }

    let modoSecoActivo = false;
    function toggleModoSeco() {
      let nuevo = !modoSecoActivo;
      cmd(nuevo ? 'MODO_SECO_ON' : 'MODO_SECO_OFF');
      showToast(nuevo ? "🧪 Modo Seco ACTIVADO (Test Cero/Anti-Ruido)" : "💧 Modo Normal RESTAURADO");
    }

    function adjVal(id, delta) {
      let el = $(id);
      let v = parseFloat(el.value) || 0;
      v = Math.max(0.1, Math.round((v + delta) * 10) / 10);
      el.value = v.toFixed(2);
      guardarParametros(0);
    }

    function clearLog() {
      if (confirm("¿Deseas reiniciar todo el historial de ensayos y registros?")) {
        fetch('/clear_csv').then(() => {
          $('n_logs').innerText = "0";
          $('t_log').innerText = "00:00";
        });
      }
    }

    $('sl').oninput = e => {
      $('lc').innerText = e.target.value;
      $('obj_rpm_lbl').innerText = e.target.value;
      updateRpmHighlight(e.target.value);
    };
    $('sl').onchange = e => setR(e.target.value);

    // Bucle de telemetría cada 500 ms
    setInterval(() => {
      fetch('/status')
        .then(r => r.json())
        .then(d => {
          rawStatus = d;
          $('rpm').innerText = d.rpm.toFixed(1);
          $('obj_rpm_lbl').innerText = d.obj_rpm.toFixed(0);
          updateRpmHighlight(d.obj_rpm);
          $('ip').innerText = d.ip;
          $('pil').className = 'pill ' + (d.inv ? 'inv' : d.on ? 'on' : 'off');
          $('pil').innerText = d.inv ? 'INVIRTIENDO' : d.on ? 'EN MARCHA' : 'DETENIDA';
          $('bd').innerText = d.dir ? '🔄 HORARIO (FILTRACIÓN)' : '🔄 ANTIHORARIO (RETROLAVADO)';
          $('al').classList.toggle('vis', d.cruce || d.q_alim > 1600);
          if (d.cruce) {
            $('al').innerText = '⚠ ALERTA CRÍTICA: Cruce de sensores detectado (Q_Perm > Q_Alim)';
          } else if (d.q_alim > 1600) {
            $('al').innerText = '⚠ ALERTA: Caudal Alimentación excede límite (> 1600 mL/min)';
          }
          $('ba').classList.toggle('vis', !d.alim_ok);
          $('bp').classList.toggle('vis', !d.perm_ok);

          modoSecoActivo = !!d.modo_seco;
          const bms = $('btn_modo_seco');
          if (bms) {
            bms.innerText = modoSecoActivo ? '⚠ Seco: ON' : '🧪 Modo Seco';
            bms.style.background = modoSecoActivo ? '#b45309' : '#1e293b';
            bms.style.borderColor = modoSecoActivo ? '#f59e0b' : '#475569';
          }
          $('va').innerText = d.q_alim.toFixed(1);
          $('fa').innerText = d.f_alim.toFixed(1) + ' Hz';
          $('la').innerText = d.vol_alim.toFixed(3) + ' L';
          $('vp').innerText = d.q_perm.toFixed(1);
          $('fp').innerText = d.f_perm.toFixed(1) + ' Hz';
          $('lp').innerText = d.vol_perm.toFixed(3) + ' L';

          if (d.f_alim && d.f_alim > 1.0) lastStableFa = d.f_alim;
          if (d.f_perm && d.f_perm > 0.3) lastStableFp = d.f_perm;
          if ($('j_lmh')) $('j_lmh').innerText = (d.j_lmh || 0).toFixed(2) + ' LMH';
          $('qr').innerText = d.q_ret.toFixed(1) + ' mL/min';
          $('rv').innerText = d.recov.toFixed(1) + ' %';
          $('qb').innerText = d.pump_ml.toFixed(1) + ' mL/min';
          $('db').innerText = (d.delta > 0 ? '+' : '') + d.delta.toFixed(1) + ' %';
          
          $('lbl_ka').innerText = d.k_alim.toFixed(2);
          $('lbl_kp').innerText = d.k_perm.toFixed(2);
          
          // Badge de régimen
          $('regimen_badge').innerText = d.en_regimen ? "🟢 Régimen Permanente Estable" : "🟡 Transición / Ajustando";
          $('regimen_badge').style.color = d.en_regimen ? "#34d399" : "#fbbf24";

          // Auto-Calibración en curso
          if (d.auto_cal) {
            $('autocal_progress_box').style.display = 'block';
            $('btn_cancel_autocal').style.display = 'block';
            $('btn_start_autocal').innerText = '⏳ Auto-Calibrando...';
            $('autocal_fill').style.width = d.auto_cal_prog + '%';
            $('autocal_txt').innerText = `Muestreando régimen... ${d.auto_cal_prog}%`;
          } else {
            $('autocal_progress_box').style.display = 'none';
            $('btn_cancel_autocal').style.display = 'none';
            $('btn_start_autocal').innerText = '▶ Iniciar Auto-Calibración Automática (15s)';
            if (d.auto_cal_res && d.auto_cal_res.length > 0) {
              showToast(d.auto_cal_res);
            }
          }

          if (!document.activeElement || document.activeElement.tagName !== 'INPUT') {
            $('inp_ka').value = d.k_alim.toFixed(2);
            $('inp_kp').value = d.k_perm.toFixed(2);
            $('inp_ml').value = d.ml_rev.toFixed(2);
            $('inp_pul').value = d.pul_rev;
          }

          $('n_logs').innerText = d.n_logs;
          $('ensayo_actual_lbl').innerText = 'Ensayo #' + d.ensayo_act;
          
          let segTot = d.t_ensayo_s;
          let mins = Math.floor(segTot / 60);
          let secs = segTot % 60;
          $('t_log').innerText = (mins < 10 ? '0' : '') + mins + ':' + (secs < 10 ? '0' : '') + secs;

          // Renderizar tabla y dropdown de ensayos
          $('cant_ensayos').innerText = d.ensayos.length + ' ensayos';
          
          const sel = $('sel_ensayo');
          const currentVal = sel.value;
          let optHtml = '<option value="all">📦 Todos los Ensayos (Histórico Completo)</option>';
          optHtml += '<option value="actual">🔴 Ensayo Actual en Vivo (#' + d.ensayo_act + ')</option>';
          d.ensayos.forEach(e => {
            optHtml += `<option value="${e.id}">Ensayo #${e.id}: ${e.rpm.toFixed(1)} RPM (${e.muestras} m, ${e.vol_perm.toFixed(3)} L perm)</option>`;
          });
          if (sel.options.length !== d.ensayos.length + 2) {
            sel.innerHTML = optHtml;
            sel.value = currentVal;
          }

          const tbody = $('body_ensayos');
          if (d.ensayos.length === 0) {
            tbody.innerHTML = '<tr><td colspan="6" style="text-align:center; color:var(--muted)">No hay ensayos finalizados aún. Arranca y apaga la bomba para registrar.</td></tr>';
          } else {
            let trHtml = '';
            d.ensayos.slice().reverse().forEach(e => {
              let m = Math.floor(e.duracion_s / 60);
              let s = e.duracion_s % 60;
              let timeStr = (m < 10 ? '0' : '') + m + ':' + (s < 10 ? '0' : '') + s;
              trHtml += `<tr>
                <td><b>#${e.id}</b></td>
                <td>${e.rpm.toFixed(1)} RPM</td>
                <td>${timeStr}</td>
                <td>${e.muestras}</td>
                <td style="color:var(--perm)">${e.vol_perm.toFixed(3)} L</td>
                <td><button style="padding:2px 6px; font-size:9px; background:#107c41" onclick="descargarEnsayoId(${e.id})">📥 CSV</button></td>
              </tr>`;
            });
            tbody.innerHTML = trHtml;
          }
        })
        .catch(() => {});
    }, 500);
  </script>
</body>
</html>
)html";

```

---

#### ARCHIVO 9: darcy.h
```cpp
#pragma once
// ==============================================================================
// MODELO DE TRANSPORTE Y RESISTENCIAS EN SERIE (LEY DE DARCY) — MEMBRANA FX100
// Diseñado para la Tesis de Grado (Antonella Guitián & Owen Cañizares / Codir. Ing. Enzo)
// ==============================================================================

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

  // Calibración experimental de Rm con agua limpia (regresión lineal J vs TMP sin ordenada al origen)
  void acumularPuntoAguaLimpia(float J_SI, float TMP_Pa) {
    _sumJP += J_SI * TMP_Pa;
    _sumPP += TMP_Pa * TMP_Pa;
  }

  bool finalizarCalibracionRm(float temp_C = 20.0f) {
    if (_sumPP <= 0.0f || _sumJP <= 0.0f) return false;
    const float Lp = _sumJP / _sumPP; // Permeabilidad hidráulica en m / (s · Pa)
    const float mu = viscosidadAgua(temp_C);
    _Rm = 1.0f / (mu * Lp);
    _sumJP = 0.0f;
    _sumPP = 0.0f;
    return true;
  }

private:
  // Resistencia intrínseca nominal de la membrana FX100 Helixone® (K_UF = 73 mL/h*mmHg, 2.2 m²).
  // Según ISO 8637, K_UF se especifica a 37 °C (mu_37 = 0.6915 mPa·s).
  // Lp = (73 mL/h/mmHg) / (2.2 m² * 3600 s/h * 133.322 Pa/mmHg) = 6.9135e-11 m/(s·Pa)
  // Rm_nominal = 1 / (mu_37 * Lp) = 2.09e13 m^-1 (a 20 °C daría 1.44e13 m^-1, subestimado 31%).
  float _Rm = 2.09e13f; 
  float _sumJP = 0.0f;
  float _sumPP = 0.0f;
};

```

---

#### ARCHIVO 10: EN_USO_firmware_planta.ino
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
  reg.tmp_bar      = resultadoDarcy.valido ? resultadoDarcy.TMP_bar : NAN;

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

  char buf[1088];
  int n = snprintf(buf, sizeof(buf),
    "{"
    "\"rpm\":%.1f,\"obj_rpm\":%.1f,\"on\":%s,\"inv\":%s,\"dir\":%s,\"en_regimen\":%s,\"emergencia\":%s,"
    "\"ip\":\"%s\",\"alim_ok\":%s,\"perm_ok\":%s,\"f_alim\":%.2f,\"q_alim\":%.1f,\"vol_alim\":%.4f,"
    "\"f_perm\":%.2f,\"q_perm\":%.1f,\"vol_perm\":%.4f,\"q_ret\":%.1f,\"recov\":%.2f,"
    "\"flan_alim\":%lu,\"val_alim\":%lu,\"gl_alim\":%lu,"
    "\"flan_perm\":%lu,\"val_perm\":%lu,\"gl_perm\":%lu,"
    "\"modo_seco\":%s,\"ruido_seco\":%s,"
    "\"pump_ml\":%.1f,\"delta\":%.2f,\"j_lmh\":%.2f,\"cruce\":%s,"
    "\"k_alim\":%.2f,\"k_perm\":%.2f,\"qmin_alim\":%.2f,\"qmin_perm\":%.2f,\"ml_rev\":%.4f,\"pul_rev\":%u,"
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
    sensorAlimentacion.getQMinDetectable(), sensorPermeado.getQMinDetectable(),
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
  if (registroEnsayos.activo()) {
    server.send(409, "application/json", "{\"status\":\"error\",\"msg\":\"No se pueden modificar factores metrologicos durante un ensayo activo\"}");
    return;
  }

  // 1. Validar primero todos los parámetros antes de aplicar
  float ka = -1.0f, kp = -1.0f, ml = -1.0f, qmina = -1.0f, qminp = -1.0f;
  int32_t pul = -1;

  if (server.hasArg("ka")) {
    ka = server.arg("ka").toFloat();
    if (!std::isfinite(ka) || ka < 10.0f || ka > 1000.0f) {
      server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Factor K Alimentacion fuera de rango (10-1000)\"}");
      return;
    }
  }
  if (server.hasArg("kp")) {
    kp = server.arg("kp").toFloat();
    if (!std::isfinite(kp) || kp < 10.0f || kp > 1000.0f) {
      server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Factor K Permeado fuera de rango (10-1000)\"}");
      return;
    }
  }
  if (server.hasArg("ml")) {
    ml = server.arg("ml").toFloat();
    if (!std::isfinite(ml) || ml < 1.0f || ml > 50.0f) {
      server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Cilindrada fuera de rango (1-50 mL/rev)\"}");
      return;
    }
  }
  if (server.hasArg("pul")) {
    pul = server.arg("pul").toInt();
    if (pul < 200 || pul > 25600) {
      server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Pulsos/rev fuera de rango (200-25600)\"}");
      return;
    }
  }
  if (server.hasArg("qmina")) {
    qmina = server.arg("qmina").toFloat();
    if (!std::isfinite(qmina) || qmina < 0.0f || qmina > 100.0f) {
      server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Qmin detectable alimentacion invalido\"}");
      return;
    }
  }
  if (server.hasArg("qminp")) {
    qminp = server.arg("qminp").toFloat();
    if (!std::isfinite(qminp) || qminp < 0.0f || qminp > 100.0f) {
      server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Qmin detectable permeado invalido\"}");
      return;
    }
  }

  // 2. Aplicar únicamente si todos pasaron la validación
  if (ka > 0.0f) sensorAlimentacion.setK(ka);
  if (kp > 0.0f) sensorPermeado.setK(kp);
  if (ml > 0.0f) bomba.setMlPorVuelta(ml);
  if (pul > 0)   bomba.setPulsosPorRev((uint16_t)pul);
  if (qmina >= 0.0f) sensorAlimentacion.setQMinDetectable(qmina);
  if (qminp >= 0.0f) sensorPermeado.setQMinDetectable(qminp);

  bool saveNvs = (server.hasArg("save") && server.arg("save") == "1");
  if (saveNvs) {
    guardarParametrosNVS(sensorAlimentacion.getK(), sensorPermeado.getK(),
                         bomba.getMlPorVuelta(), bomba.getPulsosPorRev());
  }

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// Iniciar Auto-Calibración Inteligente en Régimen Permanente
void handleIniciarAutoCal() {
  if (registroEnsayos.activo()) {
    server.send(409, "application/json", "{\"status\":\"error\",\"msg\":\"Bloqueado durante ensayo activo\"}");
    return;
  }
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
  if (registroEnsayos.activo()) {
    server.send(409, "application/json", "{\"status\":\"error\",\"msg\":\"No se puede calibrar mientras un ensayo metrologico esta en curso\"}");
    return;
  }
  if (!server.hasArg("rpm") || !server.hasArg("qa")) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Faltan argumentos (requiere rpm y qa)\"}");
    return;
  }

  float rpm = server.arg("rpm").toFloat();
  float qa  = server.arg("qa").toFloat();
  float qp  = server.hasArg("qp") ? server.arg("qp").toFloat() : 0.0f;

  if (rpm <= 0.0f || qa <= 0.0f || !std::isfinite(rpm) || !std::isfinite(qa) || !std::isfinite(qp) || qp < 0.0f) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Valores numericos invalidos o fuera de rango\"}");
    return;
  }

  // 1. Cilindrada real de la bomba
  float nuevaCilindrada = qa / rpm;
  if (nuevaCilindrada >= 1.0f && nuevaCilindrada <= 50.0f) {
    bomba.setMlPorVuelta(nuevaCilindrada);
  }

  // 2. Frecuencias observadas (pasadas por SCADA) o instantaneas en vivo
  float fa = server.hasArg("fa") ? server.arg("fa").toFloat() : sensorAlimentacion.frecuencia_Hz();
  float fp = server.hasArg("fp") ? server.arg("fp").toFloat() : sensorPermeado.frecuencia_Hz();

  bool kaOk = false;
  bool kpOk = false;

  if (fa > 1.0f) {
    float nuevoKa = (fa * 1000.0f) / qa;
    if (nuevoKa >= 10.0f && nuevoKa <= 1000.0f) {
      kaOk = sensorAlimentacion.setK(nuevoKa);
    }
  }
  if (qp > 0.0f && fp > 0.3f) {
    float nuevoKp = (fp * 1000.0f) / qp;
    if (nuevoKp >= 10.0f && nuevoKp <= 1000.0f) {
      kpOk = sensorPermeado.setK(nuevoKp);
    }
  }

  guardarParametrosNVS(sensorAlimentacion.getK(), sensorPermeado.getK(),
                       bomba.getMlPorVuelta(), bomba.getPulsosPorRev());

  String json = "{";
  json += "\"status\":\"ok\",";
  json += "\"ml_rev\":" + String(bomba.getMlPorVuelta(), 4) + ",";
  json += "\"k_alim\":" + String(sensorAlimentacion.getK(), 2) + ",";
  json += "\"k_perm\":" + String(sensorPermeado.getK(), 2) + ",";
  json += "\"ka_actualizado\":" + String(kaOk ? "true" : "false") + ",";
  json += "\"kp_actualizado\":" + String(kpOk ? "true" : "false");
  json += "}";

  server.send(200, "application/json", json);
}

// Restablecer parámetros de fábrica
void handleResetDev() {
  if (registroEnsayos.activo() || bomba.enMarcha()) {
    server.send(409, "application/json", "{\"status\":\"error\",\"msg\":\"Bloqueado durante operacion o ensayo activo\"}");
    return;
  }
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
  if (registroEnsayos.activo()) {
    server.send(409, "text/plain", "ENSAYO_ACTIVO: Detenga la bomba antes de limpiar los registros.");
    return;
  }
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

// Adaptador de Print para streaming HTTP chunked seguro en WebServer (Auditoría GPT Astra)
class ServerPrintAdapter : public Print {
public:
  WebServer& _s;
  char _buf[256];
  size_t _idx = 0;

  ServerPrintAdapter(WebServer& s) : _s(s) {}

  size_t write(uint8_t c) override {
    _buf[_idx++] = (char)c;
    if (_idx >= sizeof(_buf) - 1 || c == '\n') {
      _buf[_idx] = '\0';
      _s.sendContent(_buf, _idx);
      _idx = 0;
    }
    return 1;
  }

  void flush() {
    if (_idx > 0) {
      _buf[_idx] = '\0';
      _s.sendContent(_buf, _idx);
      _idx = 0;
    }
  }
};

// Exportación CSV con Esquema Metrológico Certificado (GPT Astra)
void handleExportMetrologia() {
  if (registroEnsayos.activo()) {
    server.send(409, "text/plain", "ENSAYO_ACTIVO: Detenga la bomba o finalice el ensayo antes de exportar el registro metrologico.");
    return;
  }
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader("Content-Type", "text/csv; charset=UTF-8");
  server.sendHeader("Content-Disposition", "attachment; filename=\"PlantaUF_Metrologia_Certificada.csv\"");
  server.sendHeader("Connection", "close");
  server.send(200, "text/csv; charset=UTF-8", "");
  ServerPrintAdapter adapter(server);
  registroEnsayos.exportarCSV(adapter);
  adapter.flush();
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
      registroEnsayos.iniciar(sensorAlimentacion, sensorPermeado, bomba.rpmActual(), true);
    }
    
    // Flanco de bajada: Finalizar sesión y registrar
    if (!enMarcha && bombaEnMarchaAnterior) {
      finalizarEnsayoActual();
      registroEnsayos.finalizar(sensorAlimentacion, sensorPermeado, bomba.rpmActual(), false);
    }

    bombaEnMarchaAnterior = enMarcha;
  }

  // 2. Adquisición y cálculo de caudales cada 1000 ms (1 segundo)
  if (tAhora - tCaudal >= 1000) {
    tCaudal = tAhora;

    bool bombaEmpuja = (bomba.rpmActual() > 1.0f);
    float qBomba = bomba.caudalTeorico_mLmin();

    const auto ma = sensorAlimentacion.capturar(bombaEmpuja);
    const auto mp = sensorPermeado.capturar(bombaEmpuja);
    registroEnsayos.tick(ma, mp, bomba.rpmActual());

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

---
