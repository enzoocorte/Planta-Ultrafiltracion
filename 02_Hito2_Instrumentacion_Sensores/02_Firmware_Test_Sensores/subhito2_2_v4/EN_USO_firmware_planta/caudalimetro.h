#pragma once
#include <Arduino.h>
#include "config.h"

// ==============================================================================
// CLASE CAUDALÍMETRO (Sensor de Efecto Hall YF-S401)
// - Lectura por interrupción en memoria IRAM (IRAM_ATTR)
// - Blanking anti-rebote por microcódigo (FILTRO_RUIDO_US) + Filtro RC físico
// - Sincronización atómica con cerrojos de sección crítica FreeRTOS (portMUX_TYPE)
// - Calibración dinámica en tiempo real (setK / getK)
// - Detección de pérdida de flujo / cable cortado
// ==============================================================================

class Caudalimetro {
public:
  Caudalimetro(uint8_t pin, float k, const char* nombre);

  void begin();
  void actualizar(float dt_s, bool bombaEmpuja);

  float caudal_mLmin()  const { return _q; }
  float caudal_Lmin()   const { return _q / 1000.0f; }
  float frecuencia_Hz() const { return _f; }
  float volumen_L()     const { return _vol; }
  bool  sinSenal()      const { return _fallo; }
  void  resetVolumen()        { _vol = 0.0f; }
  const char* nombre()  const { return _nombre; }

  // Calibración dinámica sin recompilar
  void  setK(float nuevoK)    { if (nuevoK > 0.1f) _k = nuevoK; }
  float getK()          const { return _k; }

private:
  static void IRAM_ATTR isrPuente(void* arg);

  const uint8_t _pin;
  float         _k;
  const char*   _nombre;

  volatile uint32_t _pulsos = 0;
  volatile uint32_t _t_ultimo = 0;
  volatile uint32_t _t_primero = 0;
  volatile uint32_t _periodo_us = 0;

  float _f = 0.0f;
  float _q = 0.0f;
  float _vol = 0.0f;
  float _tiempoSinPulso_s = 0.0f;
  bool _fallo = false;

  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
};
