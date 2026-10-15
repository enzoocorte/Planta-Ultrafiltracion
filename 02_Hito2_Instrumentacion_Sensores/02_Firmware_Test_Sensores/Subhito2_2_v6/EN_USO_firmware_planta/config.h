#pragma once
// ==============================================================================
// CONFIGURACIÓN TÉCNICA — PLANTA DE ULTRAFILTRACIÓN (UNSa 2026) — VERSIÓN V6
// Tesistas: Antonella Guitián & Owen Cañizares | Codirector: Ing. Enzo Corte
// Parámetros Cinemáticos Calibrados (Ensayos 2 y 3), Anti-EMI y Monitoreo de Torta
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

// Puertos I2C para ADS1115 (Subhito 2.3 - Transductores de Presión y Sonda TDS)
constexpr uint8_t PIN_I2C_SDA             = 21;  // ESP32 SDA
constexpr uint8_t PIN_I2C_SCL             = 22;  // ESP32 SCL

// ASERCIÓN ESTÁTICA EXHAUSTIVA DE PINES
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
static_assert(pinesUnicos(), "ERROR: Hay pines GPIO compartidos en el sistema");

// ------------------------------------------------------------------------------
// 2. PARÁMETROS CINEMÁTICOS DE LA BOMBA (CALIBRADOS EN ENSAYO 2 CON PROBETA)
// ------------------------------------------------------------------------------
constexpr uint16_t PULSOS_POR_REV = 3200;     // DM860 configurado a 1/16 micropasos (3200 pulsos/rev)
constexpr float ML_POR_VUELTA     = 13.5496f; // Calibración Ensayo 2 (20 a 90 RPM): Media = 13.55 mL/rev (CV = 8.12%)

// Rango de Operación en RPM
constexpr float RPM_MIN           = 15.0f;    // ~203 mL/min
constexpr float RPM_MAX           = 100.0f;   // ~1355 mL/min (Habilitado para diseño factorial experimental en agua)
constexpr float RPM_INICIO        = 25.0f;    // Consigna de arranque suave (~339 mL/min)
constexpr float RPM_ALARMA_MEMBRANA = 44.0f;  // 44.28 RPM = 600 mL/min (Umbral clínico hemodiálisis / referencia)

// Rampas de Aceleración y Frenado (< 1.5s parada garantizada)
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
constexpr float TMP_MAX_SEGURA_BAR     = 0.50f;   // Límite máximo admisible de TMP (bar)
constexpr float P1_MAX_SEGURA_BAR      = 0.60f;   // Límite máximo de presión en entrada de cartucho (bar)
constexpr float Q_CLINICO_SANGRE_MAX   = 600.0f;  // mL/min (límite en hemodiálisis clínica)

// ------------------------------------------------------------------------------
// 4. ESTRUCTURA METROLÓGICA Y CALIBRACIÓN DE CAUDALÍMETROS YF-S401 (VERSIÓN V6)
// ------------------------------------------------------------------------------
// Ambos sensores son del mismo modelo físico (YF-S401) y comparten el factor K
// calibrado en laboratorio en el Ensayo 3: K = 205.98 Hz/(L/min) (12,359 pulsos/L).
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

// Sensor de Alimentación (GPIO 14 - Borne 12)
constexpr ConfigSensor SENSOR_ALIM_CFG = {
  PIN_SENSOR_ALIMENTACION, // Pin 14
  205.98,                  // K calibrado en Ensayo 3 [Hz / (L/min)] (12359 pulsos/L)
  1200,                    // Ancho mínimo LOW: 1200 us (supera constante RC de 940 us)
  1200,                    // Ancho mínimo HIGH: 1200 us
  2800,                    // Período mínimo admisible: 2800 us (357 Hz)
  1500000,                 // Timeout de pérdida de señal: 1.5 s
  1500.0,                  // Q máximo operativo admisible (mL/min)
  20.0,                    // Umbral mínimo detectable (mL/min)
  true,                    // Calibración documentada en Ensayo 3
  280.0,                   // Q mínimo calibrado en probeta (20 RPM = 280 mL/min)
  1080.0                   // Q máximo calibrado en probeta (90 RPM = 1080 mL/min)
};

// Sensor de Permeado (GPIO 27 - Borne 11)
constexpr ConfigSensor SENSOR_PERM_CFG = {
  PIN_SENSOR_PERMEADO,     // Pin 27
  205.98,                  // K homologado al mismo modelo físico YF-S401 (ajustable desde SCADA)
  3500,                    // Ancho mínimo LOW: 3500 us (3.5 ms -> inmune a picos EMI + filtro RC de 940 us)
  3500,                    // Ancho mínimo HIGH: 3500 us (3.5 ms)
  8000,                    // Período mínimo admisible: 8000 us (125 Hz)
  3500000,                 // Timeout de pérdida de señal: 3.5 s (permite medir frecuencias bajas hasta 0.4 Hz)
  350.0,                   // Q máximo operativo admisible (mL/min)
  2.0,                     // Umbral mínimo detectable (2.0 mL/min) para seguir la merma de permeado
  false,                   // Sujeto a asistencia por probeta cuando Q_perm cae bajo el umbral mecánico
  0.0,                     // Q min
  0.0                      // Q max
};

// Constantes globales calibradas (Ensayos 2 y 3 - UNSa 2026 - Versión V6)
constexpr float    K_ALIMENTACION       = 205.98f;
constexpr float    K_PERMEADO           = 205.98f;
constexpr uint8_t  VERSION_CALIBRACION  = 6; // Versión V6: homologa ambos K en 205.98 y cilindrada en 13.55 en NVS

// ------------------------------------------------------------------------------
// 5. PARÁMETROS DEL DATALOGGER MULTI-SESIÓN (10 SEGUNDOS / 600 REGISTROS = 100 MIN)
// ------------------------------------------------------------------------------
// CORRECCIÓN OBSERVACIÓN WORD: Muestreo cada 10 segundos desde t = 0 s con capacidad
// de 600 registros (100 minutos continuos), evitando que se pisen las muestras de 20 RPM.
constexpr uint32_t INTERVALO_LOG_MS   = 10000;  // Muestreo cada 10 segundos exactos
constexpr size_t   MAX_REGISTROS      = 600;    // 600 muestras en buffer circular (100 minutos continuos sin sobrescribir)
constexpr size_t   MAX_ENSAYOS        = 20;     // Hasta 20 ensayos/sesiones independientes
constexpr uint8_t  MUESTRAS_AUTO_CAL  = 15;     // 15 muestras (15 s) para auto-calibración en régimen permanente

// ------------------------------------------------------------------------------
// 6. CONFIGURACIÓN DE RED WI-FI Y SCADA
// ------------------------------------------------------------------------------
constexpr const char* SSID_AP  = "Bomba_Peristaltica_UF";
constexpr const char* PASS_AP  = "plantapiloto2";
constexpr const char* SSID_STA = "Box804";
constexpr const char* PASS_STA = "plantapiloto2";
