#pragma once
// ============================================================
//  CONFIGURACIÓN — Planta UF | Versión 2 Calibrada (25/09/2026)
//  Calibración experimental: 72 RPM = 150 mL en 1 min (112.8 Hz)
// ============================================================

// ---- PINES ----
// Driver DM860 en CÁTODO COMÚN: GPIO manda HIGH = opto ON (7.8 mA por pin)
constexpr uint8_t PIN_PUL         = 18;  // DM860 PUL+ (PUL- a GND común)
constexpr uint8_t PIN_DIR         = 19;  // DM860 DIR+ (DIR- a GND común)
constexpr uint8_t PIN_SENSOR_FEED = 14;  // Caudalímetro FEED
constexpr uint8_t PIN_SENSOR_PERM = 27;  // Caudalímetro PERMEADO

// ---- BOMBA PERISTÁLTICA MBP-2000 (CALIBRADA) ----
constexpr uint16_t PULSOS_POR_REV = 1600;      // DM860: 8 micropasos (SW5-8)
constexpr float ML_POR_VUELTA     = 2.0833f;   // Calibrado: 150 mL / 72 RPM = 2.0833 mL/giro real
constexpr float RPM_MIN           = 50.0f;     // ~104 mL/min
constexpr float RPM_MAX           = 200.0f;    // ~416 mL/min (seguro para membrana FX100 <= 600 mL/min)
constexpr float RPM_INICIO        = 72.0f;     // Punto de operación de prueba (150 mL/min)
constexpr float ACEL_RPM_S        = 8.0f;      // Rampa suave de aceleración (0 a 72 RPM en 9s)
constexpr float DESACEL_RPM_S     = 12.0f;     // Rampa suave de frenado (72 a 0 RPM en 6s)

// ---- CAUDALÍMETROS YF-S401 (CALIBRADOS POR GRAVIMETRÍA) ----
// F (Hz) = K × Q (L/min)  -->  K = F / Q(L/min) = 112.7 Hz / 0.150 L/min = 751.3
constexpr float K_FEED = 751.3f;   // Sensor FEED: 112.7 Hz a 150 mL/min
constexpr float K_PERM = 754.0f;   // Sensor PERMEADO: 113.1 Hz a 150 mL/min

constexpr uint32_t FILTRO_RUIDO_US = 1500;      // Filtro anti-rebote adaptado a pulsos de alta frecuencia
constexpr float Q_MAX_FISICO_MLMIN = 6000.0f;   // Límite físico YF-S401

// ---- WI-FI ----
constexpr const char* SSID_AP  = "Bomba_Peristaltica_UF";
constexpr const char* PASS_AP  = "plantapiloto2";
constexpr const char* SSID_STA = "Box804";
constexpr const char* PASS_STA = "plantapiloto2";
