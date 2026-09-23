#pragma once
// ============================================================
//  CONFIGURACIÓN — Planta UF (aquí se cambia TODO)
// ============================================================

// ---- PINES ----
// Driver DM860 en CÁTODO COMÚN: GPIO manda HIGH = opto ON (7.8 mA por pin)
constexpr uint8_t PIN_PUL         = 18;  // DM860 PUL+ (PUL- a GND común)
constexpr uint8_t PIN_DIR         = 19;  // DM860 DIR+ (DIR- a GND común)
constexpr uint8_t PIN_SENSOR_FEED = 14;  // Caudalímetro FEED
constexpr uint8_t PIN_SENSOR_PERM = 27;  // Caudalímetro PERMEADO

// ---- BOMBA PERISTÁLTICA ----
constexpr uint16_t PULSOS_POR_REV = 1600;      // DM860: 8 micropasos (SW5-8)
constexpr float ML_POR_VUELTA     = 4.2f;      // mL por giro del cabezal
constexpr float RPM_MIN           = 80.0f;     // 336 mL/min (mín. sensor: 300)
constexpr float RPM_MAX           = 140.0f;    // 588 mL/min (límite membrana: 600)
constexpr float RPM_INICIO        = 80.0f;
constexpr float ACEL_RPM_S        = 40.0f;     // rampa

// ---- CAUDALÍMETROS YF-S401 ----
// F (Hz) = K × Q (L/min).  >>> Calibrar cada sensor por gravimetría <<<
constexpr float K_FEED = 98.0f;
constexpr float K_PERM = 98.0f;

constexpr uint32_t FILTRO_RUIDO_US = 3000;      // pulso real más rápido ≈17 ms
constexpr float Q_MAX_FISICO_MLMIN = 700.0f;    // imposible con esta bomba → ruido

// ---- WI-FI ----
constexpr const char* SSID_AP  = "Bomba_Peristaltica_UF";
constexpr const char* PASS_AP  = "plantapiloto2";
constexpr const char* SSID_STA = "Box804";
constexpr const char* PASS_STA = "plantapiloto2";