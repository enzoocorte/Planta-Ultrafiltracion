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
// Desplazamiento manguera silicona ØInt 12mm (MBP-2000): ~15.4 mL/rev
constexpr uint16_t PULSOS_POR_REV = 1600;      // DM860: 8 micropasos (pasar a 3200 si se activa 16 micropasos)
constexpr float ML_POR_VUELTA     = 15.4f;     // mL por giro del cabezal (manguera 12mm)
constexpr float RPM_MIN           = 20.0f;     // 308 mL/min (mín. sensor: 300)
constexpr float RPM_MAX           = 42.0f;     // 646 mL/min (límite membrana: 600-650)
constexpr float RPM_INICIO        = 25.0f;     // 385 mL/min (punto óptimo inicial)
constexpr float ACEL_RPM_S        = 20.0f;     // rampa suave para bajo régimen

// ---- CAUDALÍMETROS YF-S401 ----
// F (Hz) = K × Q (L/min).  >>> Calibrar cada sensor por gravimetría <<<
constexpr float K_FEED = 98.0f;
constexpr float K_PERM = 98.0f;

constexpr uint32_t FILTRO_RUIDO_US = 3000;      // pulso real más rápido ≈17 ms
constexpr float Q_MAX_FISICO_MLMIN = 6000.0f;   // Límite físico YF-S401 (0.3 a 6 L/min). Permite prueba de soplido

// ---- WI-FI ----
constexpr const char* SSID_AP  = "Bomba_Peristaltica_UF";
constexpr const char* PASS_AP  = "plantapiloto2";
constexpr const char* SSID_STA = "Box804";
constexpr const char* PASS_STA = "plantapiloto2";