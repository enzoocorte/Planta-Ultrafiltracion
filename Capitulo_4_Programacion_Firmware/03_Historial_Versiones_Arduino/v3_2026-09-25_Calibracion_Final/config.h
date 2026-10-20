#pragma once
// ============================================================
//  CONFIGURACIÓN — Planta UF | Versión 3 Calibrada (25/09/2026)
//  Calibración empírica real en probeta:
//  - FEED a 72 RPM: 512 mL en 30s -> Q_feed = 1024 mL/min (F = 112.7 Hz)
//  - PERMEADO: Q_perm = 150 mL/min (F = 16.6 Hz)
//  - K_FEED = 110.06 pulsos/L | K_PERM = 110.45 pulsos/L
//  - Cilindrada Bomba: 14.222 mL/vuelta
// ============================================================

// ---- PINES ----
// Driver DM860 en CÁTODO COMÚN: GPIO manda HIGH = opto ON (7.8 mA por pin)
constexpr uint8_t PIN_PUL         = 18;  // DM860 PUL+ (PUL- a GND común)
constexpr uint8_t PIN_DIR         = 19;  // DM860 DIR+ (DIR- a GND común)
constexpr uint8_t PIN_SENSOR_FEED = 14;  // Caudalímetro FEED
constexpr uint8_t PIN_SENSOR_PERM = 27;  // Caudalímetro PERMEADO

// ---- BOMBA PERISTÁLTICA MBP-2000 (CALIBRADA) ----
constexpr uint16_t PULSOS_POR_REV = 1600;      // DM860: 8 micropasos (SW5-8)
constexpr float ML_POR_VUELTA     = 14.2222f;  // Calibrado: 1024 mL/min / 72 RPM = 14.2222 mL/giro real
constexpr float RPM_MIN           = 15.0f;     // ~213 mL/min
constexpr float RPM_MAX           = 75.0f;     // ~1066 mL/min (operación tangencial óptima)
constexpr float RPM_INICIO        = 72.0f;     // Punto de operación de prueba (1024 mL/min)
constexpr float ACEL_RPM_S        = 8.0f;      // Rampa suave de aceleración (0 a 72 RPM en 9s)
constexpr float DESACEL_RPM_S     = 12.0f;     // Rampa suave de frenado (72 a 0 RPM en 6s)

// ---- CAUDALÍMETROS YF-S401 (CALIBRADOS POR GRAVIMETRÍA INDIVIDUAL) ----
// F (Hz) = K × Q (L/min)  -->  K = F / Q(L/min) = 112.7 Hz / 1.024 L/min = 110.06
constexpr float K_FEED = 110.06f;  // Sensor FEED: 112.7 Hz a 1024 mL/min (1.024 L/min)
constexpr float K_PERM = 110.45f;  // Sensor PERMEADO: Mismo modelo de sensor YF-S401

constexpr uint32_t FILTRO_RUIDO_US = 2000;      // Filtro anti-rebote adaptado
constexpr float Q_MAX_FISICO_MLMIN = 6000.0f;   // Límite físico YF-S401

// ---- WI-FI ----
constexpr const char* SSID_AP  = "Bomba_Peristaltica_UF";
constexpr const char* PASS_AP  = "plantapiloto2";
constexpr const char* SSID_STA = "Box804";
constexpr const char* PASS_STA = "plantapiloto2";
