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
