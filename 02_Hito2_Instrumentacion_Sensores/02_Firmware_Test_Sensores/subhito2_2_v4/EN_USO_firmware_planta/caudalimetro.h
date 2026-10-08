#pragma once
#include <Arduino.h>
#include "config.h"

// ==============================================================================
// CLASE CAUDALÍMETRO (Sensor de Efecto Hall YF-S401)
// - Lectura por interrupción en memoria IRAM (IRAM_ATTR) con paso de puntero
// - Blanking anti-rebote (FILTRO_RUIDO_US = 1500 us) + Filtro RC analógico
// - Medición por período recíproco de alta resolución (inmune a cuantización ±1 pulso)
// - Cota física superior continua en ausencia de pulsos (decaimiento asintótico suave a cero)
// - Sincronización atómica multinúcleo con spinlock FreeRTOS (portMUX_TYPE)
// - Calibración en tiempo real (setK / getK) en unidades [Hz / (L/min)]
// - Diagnóstico de señal asimétrico (Alarma solo en Alimentación)
// ==============================================================================

class Caudalimetro {
public:
  Caudalimetro(uint8_t pin, float k, const char* nombre, bool esAlimentacion = true);

  void begin();
  void actualizar(float dt_s, bool bombaEmpuja);

  float caudal_mLmin()   const { return _q; }
  float caudal_Lmin()    const { return _q / 1000.0f; }
  float frecuencia_Hz()  const { return _f; }
  float volumen_L()      const { return _vol; }
  bool  sinSenal()       const { return _fallo; }
  void  resetVolumen()         { _vol = 0.0f; }
  const char* nombre()   const { return _nombre; }
  bool  esAlimentacion() const { return _esAlimentacion; }
  uint32_t pulsosBrutos()const { return _pulsosBrutos; } // Diagnóstico de ráfagas EMI para prueba en seco

  // Calibración dinámica sin recompilar [Hz / (L/min)]
  void  setK(float nuevoK)     { if (nuevoK > 0.1f) _k = nuevoK; }
  float getK()           const { return _k; }

private:
  static void IRAM_ATTR isrPuente(void* arg);

  const uint8_t _pin;
  float         _k;
  const char*   _nombre;
  const bool    _esAlimentacion;

  volatile uint32_t _pulsos = 0;
  volatile uint32_t _t_ultimo = 0;
  volatile uint32_t _t_primero = 0;
  volatile uint32_t _dt_min_us = 0xFFFFFFFF;
  volatile uint32_t _dt_max_us = 0;

  float    _f = 0.0f;
  float    _q = 0.0f;
  float    _vol = 0.0f;
  float    _tiempoSinPulso_s = 0.0f;
  uint32_t _pulsosBrutos = 0;
  bool     _fallo = false;

  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
};
