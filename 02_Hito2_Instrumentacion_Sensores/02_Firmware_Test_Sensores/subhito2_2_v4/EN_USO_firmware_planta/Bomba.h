#pragma once
#include <Arduino.h>
#include <cmath>
#include "config.h"

// ==============================================================================
// CLASE BOMBA PERISTÁLTICA — DECLARACIÓN
// - Control de frecuencia por hardware LEDC PWM (Driver DM860)
// - Rampa Cuadrática S-Curve de arranque suave (sin golpe de torque)
// - Parada rápida en < 1.5s
// ==============================================================================

class Bomba {
public:
  void begin();
  void arrancar();
  void detener();
  void paradaEmergencia();       // Corte instantáneo físico y enclavado (< 1 ms, sin rampa)
  void rearmarEmergencia();      // Rearme manual tras condición segura
  bool setRPM(float rpm);
  void toggleSentido();

  float rpmActual() const           { return _actual; }
  float rpmObjetivo() const         { return _objetivo; }
  bool  enMarcha() const            { return _enMarcha; }
  bool  invirtiendo() const         { return _invirtiendo; }
  bool  sentidoHorario() const      { return _horario; }
  bool  enEmergencia() const        { return _enEmergencia; }
  bool  enRegimenEstable() const    { return (_enMarcha && !_enEmergencia && fabsf(_actual - _objetivo) < 0.3f && _actual > 5.0f); }
  float caudalTeorico_mLmin() const { return _actual * _mlPorVuelta; }

  void  setMlPorVuelta(float ml)    { if (ml > 0.1f) _mlPorVuelta = ml; }
  float getMlPorVuelta() const      { return _mlPorVuelta; }

  void     setPulsosPorRev(uint16_t p) { if (p >= 200) _pulsosPorRev = p; }
  uint16_t getPulsosPorRev() const     { return _pulsosPorRev; }

  void tick(float dt);

private:
  void fijarSentido(bool horario);

  bool _enMarcha = false;
  bool _horario = true;
  bool _invirtiendo = false;
  bool _enEmergencia = false;

  float _objetivo = RPM_INICIO;
  float _actual = 0.0f;
  float _rpmGuardada = RPM_INICIO;
  uint32_t _fActual = 0;

  float    _mlPorVuelta  = ML_POR_VUELTA;
  uint16_t _pulsosPorRev = PULSOS_POR_REV;
};
