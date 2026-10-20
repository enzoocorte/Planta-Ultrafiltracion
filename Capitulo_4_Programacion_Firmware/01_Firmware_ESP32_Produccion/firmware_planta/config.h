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
constexpr uint8_t PIN_LED_BOMBA          = 2;   // LED Azul Onboard (indicador de marcha)

// Caudalímetros de Efecto Hall YF-S401
constexpr uint8_t PIN_SENSOR_ALIMENTACION = 14;  // Sensor de Alimentación (Borne 12 Sup)
constexpr uint8_t PIN_SENSOR_PERMEADO     = 27;  // Sensor de Permeado (Borne 11 Sup, Pull-up 4.7k a 3.3V)

// ------------------------------------------------------------------------------
// 2. PARÁMETROS CINEMÁTICOS DE LA BOMBA (MOTOR NEMA 34 / DM860)
// ------------------------------------------------------------------------------
constexpr uint16_t PULSOS_POR_REV = 3200;     // DM860 configurado a 1/16 micropasos (3200 pulsos/rev)
constexpr float ML_POR_VUELTA     = 13.6000f; // Calibrado: 680.0 mL/min / 50.0 RPM = 13.6000 mL/rev

// Rango de Operación en RPM
constexpr float RPM_MIN           = 15.0f;    // ~204 mL/min
constexpr float RPM_MAX           = 75.0f;    // ~1020 mL/min
constexpr float RPM_INICIO        = 50.0f;    // Consigna nominal de calibración (50 RPM -> 680 mL/min Alimentación)

// Rampas: Arranque Suave Confiable (LEDC Seguro) y Frenado Rápido (< 1.5s)
constexpr float ACEL_ARRANQUE_RPM_S  = 2.0f;   // 2.0 RPM/s despegue inicial suave y confiable
constexpr float ACEL_NOMINAL_RPM_S   = 3.5f;   // 3.5 RPM/s aceleración progresiva
constexpr float DESACEL_AJUSTE_RPM_S = 8.0f;   // 8.0 RPM/s desaceleración en marcha
constexpr float FRENADO_PARADA_RPM_S = 45.0f;  // 45.0 RPM/s frenado rápido al presionar STOP (< 1.5s)

// ------------------------------------------------------------------------------
// 3. CALIBRACIÓN DE FÁBRICA DE CAUDALÍMETROS YF-S401
// ------------------------------------------------------------------------------
constexpr float K_ALIMENTACION = 154.62f; // Factor K Alimentación (105.14 Hz = 680.0 mL/min)
constexpr float K_PERMEADO     = 55.00f;  // Factor K Permeado (5.50 Hz = 100.0 mL/min)

constexpr uint32_t FILTRO_RUIDO_US = 2000;    // 2.0 ms de blanking anti-rebote
constexpr float Q_MAX_FISICO_MLMIN = 6000.0f; // Límite de corte físico (6.0 L/min)

// ------------------------------------------------------------------------------
// 4. PARÁMETROS DEL DATALOGGER MULTI-SESIÓN Y AUTO-CALIBRACIÓN
// ------------------------------------------------------------------------------
constexpr uint32_t INTERVALO_LOG_MS   = 10000;  // Muestreo y almacenamiento cada 10 segundos
constexpr size_t   MAX_REGISTROS      = 600;    // 600 muestras = 1.66 horas de ensayo continuo
constexpr size_t   MAX_ENSAYOS        = 20;     // Hasta 20 corridas/ensayos registrados en memoria
constexpr uint8_t  MUESTRAS_AUTO_CAL  = 15;     // 15 muestras (15 seg) para auto-calibración en régimen permanente

// ------------------------------------------------------------------------------
// 5. CONFIGURACIÓN DE RED WI-FI Y SCADA
// ------------------------------------------------------------------------------
constexpr const char* SSID_AP  = "Bomba_Peristaltica_UF";
constexpr const char* PASS_AP  = "plantapiloto2";
constexpr const char* SSID_STA = "Box804";
constexpr const char* PASS_STA = "plantapiloto2";
