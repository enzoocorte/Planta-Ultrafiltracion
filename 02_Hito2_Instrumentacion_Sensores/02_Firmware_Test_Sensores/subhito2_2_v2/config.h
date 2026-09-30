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
// Desplazamiento nominal manguera 12mm: 15.4 mL/rev (se recalibra en P6 con probeta)
constexpr uint16_t PULSOS_POR_REV   = 3200;      // ⚠️ IGUAL a los DIP SW5-SW8 del DM860 (3200 para 16 micropasos)
constexpr float ML_POR_VUELTA       = 15.4f;     // mL por giro del cabezal
constexpr float RPM_MIN             = 20.0f;     // 308 mL/min (mín. sensor: 300)
constexpr float RPM_MAX             = 100.0f;    // máx exploratorio con agua
constexpr float RPM_INICIO          = 25.0f;     // 385 mL/min
constexpr float ACEL_RPM_S          = 20.0f;     // rampa suave

// Alarma blanda: a 36 RPM con manguera 12mm se alcanzan ~600 mL/min (límite FX100).
// Por encima: solo ensayos controlados con agua y retentado abierto.
constexpr float RPM_ALARMA_MEMBRANA = 36.0f;

// ---- CAUDALÍMETROS YF-S401 ----
// F (Hz) = K × Q (L/min).  >>> Calibrar cada sensor por gravimetría <<<
constexpr float K_FEED = 98.0f;
constexpr float K_PERM = 98.0f;

// Filtro digital por software (ISR):
// • 12000 µs (12 ms): Máxima inmunidad contra el hum de 108 Hz durante validación inicial (P1 a P10).
//   Techo de medición: ~850 mL/min (~55 RPM). Para el rango nominal (≤ 36 RPM = ~554 mL/min) es perfecto.
// • 4500 µs (4.5 ms): Techo 222 Hz (~2260 mL/min / >100 RPM). Descomentar tras validar P5 limpia (PERM = 0.0 Hz).
constexpr uint32_t FILTRO_RUIDO_US = 12000;
// constexpr uint32_t FILTRO_RUIDO_US = 4500;  // Activar tras pasar P5
constexpr float Q_MAX_FISICO_MLMIN = 6000.0f;   // Límite físico YF-S401 (0.3 a 6 L/min). Permite prueba de soplido

// ---- WI-FI ----
constexpr const char* SSID_AP  = "Bomba_Peristaltica_UF";
constexpr const char* PASS_AP  = "plantapiloto2";
constexpr const char* SSID_STA = "Box804";
constexpr const char* PASS_STA = "plantapiloto2";