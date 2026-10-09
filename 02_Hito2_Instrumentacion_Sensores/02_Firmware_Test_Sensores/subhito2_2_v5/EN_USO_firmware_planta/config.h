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
// Relación matemática fundamental:
//   F [Hz] = K * Q [L/min]  ===>  Q [mL/min] = (F [Hz] * 1000) / K
//   Pulsos por Litro = K * 60
// Nominal de fabricante YF-S401: F = 98 * Q (L/min) => K = 98.0 Hz/(L/min) (5880 pul/L)
// Calibración experimental con probeta validada en laboratorio (20 a 90 RPM):
constexpr float K_ALIMENTACION = 196.50f; // Hz/(L/min) -> 11790 pulsos/L (meseta experimental a 60-80 RPM)
constexpr float K_PERMEADO     = 687.33f; // Hz/(L/min) -> valor calibrado nominal sensor permeado

constexpr uint32_t FILTRO_RUIDO_US = 1500;    // 1.5 ms de blanking anti-rebote (hasta 666 Hz / ~4300 mL/min)
// Auditoría Ronda 4: A 100 RPM el caudal máximo de bomba es 1360 mL/min.
// Un límite de 2500 mL/min actúa como filtro activo anti-ruido EMI sin limitar el flujo real.
constexpr float Q_MAX_FISICO_MLMIN = 2500.0f; // Límite físico de plausibilidad (2.5 L/min)

// ------------------------------------------------------------------------------
// 4. PARÁMETROS DEL DATALOGGER MULTI-SESIÓN Y AUTO-CALIBRACIÓN
// ------------------------------------------------------------------------------
constexpr uint32_t INTERVALO_LOG_MS   = 10000;  // Muestreo cada 10 segundos (600 muestras = 100 minutos continuos)
constexpr size_t   MAX_REGISTROS      = 600;    // 600 muestras = 1.66 horas de ensayo continuo sin sobreescritura
constexpr size_t   MAX_ENSAYOS        = 20;     // Hasta 20 corridas/ensayos registrados en memoria
constexpr uint8_t  MUESTRAS_AUTO_CAL  = 15;     // 15 muestras (15 seg) para auto-calibración en régimen permanente

// ------------------------------------------------------------------------------
// 5. CONFIGURACIÓN DE RED WI-FI Y SCADA
// ------------------------------------------------------------------------------
constexpr const char* SSID_AP  = "Bomba_Peristaltica_UF";
constexpr const char* PASS_AP  = "plantapiloto2";
constexpr const char* SSID_STA = "Box804";
constexpr const char* PASS_STA = "plantapiloto2";
