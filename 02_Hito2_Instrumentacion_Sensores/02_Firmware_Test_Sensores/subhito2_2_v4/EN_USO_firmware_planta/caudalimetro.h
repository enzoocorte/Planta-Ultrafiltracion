#pragma once
#include <Arduino.h>
#include "config.h"

// ==============================================================================
// CLASE CAUDALÍMETRO (Sensor de Efecto Hall YF-S401 - V5 Anti-EMI / Ronda 6)
// - Detección por interrupción en ambos flancos (CHANGE)
// - Validación física de ancho de nivel: rechaza glitches cortos (chopper motor y rebotes RC)
// - Medición de período recíproco sobre pulsos físicamente confirmados
// - Deadband asimétrico adaptativo por canal (Alimentación vs Permeado)
// - Modo de diagnóstico de prueba en seco (Enclavamiento de ruido EMI)
// - Sincronización atómica multinúcleo con spinlock FreeRTOS (portMUX_TYPE)
// ==============================================================================

struct ParametrosPulso {
  uint32_t anchoMinLow_us;    // Duración mínima de nivel LOW para aceptar álabe (us)
  uint32_t anchoMinHigh_us;   // Duración mínima de nivel HIGH previo (us)
  uint32_t periodoMin_us;     // Período mínimo admisible entre pulsos (us)
  uint32_t periodoMax_us;     // Período máximo antes de declarar turbina detenida (us)
  float    qMin_mLmin;        // Umbral de corte de caudal mínimo específico del canal
};

class Caudalimetro {
public:
  Caudalimetro(uint8_t pin, float k, const char* nombre, bool esAlimentacion = true);

  void begin();
  void actualizar(float dt_s, bool bombaEmpuja, float qBombaTeorico_mLmin = 0.0f);

  float caudal_mLmin()   const { return _q; }
  float caudal_Lmin()    const { return _q / 1000.0f; }
  float frecuencia_Hz()  const { return _f; }
  float volumen_L()      const { return _vol; }
  bool  sinSenal()       const { return _fallo; }
  void  resetVolumen()         { _vol = 0.0f; }
  const char* nombre()   const { return _nombre; }
  bool  esAlimentacion() const { return _esAlimentacion; }

  // Métricas de diagnóstico de la última ventana (1 s)
  uint32_t flancosBrutos()      const { return _flancosVentana; }
  uint32_t pulsosValidos()      const { return _validosVentana; }
  uint32_t glitchesVentana()    const { return _glitchesVentana; }
  uint32_t pulsosBrutos()       const { return _validosVentana; } // compatibilidad
  bool     ruidoDetectadoEnSeco() const { return _ruidoEnSeco; }

  // Control del Modo de Prueba en Seco (Condición C1 de Auditoría)
  void setModoSeco(bool activo) { _modoSeco = activo; if (activo) _ruidoEnSeco = false; }
  bool modoSeco() const { return _modoSeco; }

  // Calibración dinámica sin recompilar [Hz / (L/min)]
  void  setK(float nuevoK)     { if (nuevoK > 0.1f) _k = nuevoK; }
  float getK()           const { return _k; }

private:
  static void IRAM_ATTR isrPuente(void* arg);
  void IRAM_ATTR isrInterna();

  const uint8_t   _pin;
  float           _k;
  const char*     _nombre;
  const bool      _esAlimentacion;
  ParametrosPulso _p;

  // Variables de estado atómico de la ISR
  volatile bool     _bajadaCandidata = false;
  volatile uint32_t _tBajada = 0;
  volatile uint32_t _tSubida = 0;
  volatile uint32_t _tUltimoValido = 0;
  volatile uint32_t _tPrimeroValido = 0;

  volatile uint32_t _flancosTotal = 0;
  volatile uint32_t _validosTotal = 0;
  volatile uint32_t _glitchesTotal = 0;

  // Variables copiadas para procesamiento en la ventana
  uint32_t _flancosVentana = 0;
  uint32_t _validosVentana = 0;
  uint32_t _glitchesVentana = 0;

  float _f = 0.0f;
  float _q = 0.0f;
  float _vol = 0.0f;
  float _tiempoSinPulso_s = 0.0f;
  bool  _fallo = false;
  bool  _modoSeco = false;
  bool  _ruidoEnSeco = false;

  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
};
