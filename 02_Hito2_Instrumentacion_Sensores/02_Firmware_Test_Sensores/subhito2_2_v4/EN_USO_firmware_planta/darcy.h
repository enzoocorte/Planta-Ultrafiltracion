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
  float R_torta;     // Resistencia por capa de torta / ensuciamiento [m^-1]
  float TMP_bar;     // Presión transmembrana efectiva [bar]
  bool  valido;      // Estado de cálculo válido
};

class ModeloDarcy {
public:
  static constexpr float MU20 = 1.002e-3f; // Pa · s (Agua destilada a 20.0 °C)

  // Viscosidad dinámica del agua mediante ecuación de Vogel (válida 5 a 60 °C)
  static float viscosidadAgua(float temp_C) {
    const float T_K = temp_C + 273.15f;
    return 2.414e-5f * powf(10.0f, 247.8f / (T_K - 140.0f));
  }

  void  setRm(float rm) { if (rm > 1e11f && rm < 1e16f) _Rm = rm; }
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

    r.TMP_bar = tmp_bar;
    r.mu_Pas  = viscosidadAgua(temp_C);
    r.TCF     = r.mu_Pas / MU20;

    // J [LMH] = Q [L/h] / Area [m²] = (qPerm [mL/min] * 0.06) / 2.2 m²
    r.J_LMH   = (qPerm_mLmin * 0.06f) / AREA_MEMBRANA_M2;
    r.J20_LMH = r.J_LMH * r.TCF;

    // J en unidades SI [m/s]: 1 LMH = 1 / 3.6e6 m/s
    const float J_SI   = r.J_LMH / 3.6e6f;
    const float TMP_Pa = tmp_bar * 100000.0f;

    // Ley de Darcy: J = TMP / (mu * R_total) => R_total = TMP / (mu * J)
    r.R_total = TMP_Pa / (r.mu_Pas * J_SI);
    r.R_torta = r.R_total - _Rm;
    r.valido  = true;

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
  // Resistencia intrínseca nominal de la membrana FX100 Helixone® (K_UF = 73 mL/h*mmHg, 2.2 m²)
  float _Rm = 1.44e13f; 
  float _sumJP = 0.0f;
  float _sumPP = 0.0f;
};
