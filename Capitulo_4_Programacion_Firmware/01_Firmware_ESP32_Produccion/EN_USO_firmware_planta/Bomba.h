#pragma once
#include <Arduino.h>
#include <cmath>
#include "config.h"

// ==============================================================================
// CLASE BOMBA PERISTÁLTICA — DECLARACIÓN (Tesis UNSa 2026)
// - Control de frecuencia por hardware LEDC PWM (Driver DM860 en Cátodo Común)
// - Rampa progresiva de arranque suave (sin golpe de ariete ni pérdida de pasos)
// - Parada rápida en < 1.5s y corte físico de emergencia por pin ENA (< 1 ms)
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

  void  setRpmInicio(float rpm)         { if (rpm >= RPM_MIN && rpm <= RPM_MAX) _rpmInicio = rpm; }
  float getRpmInicio() const            { return _rpmInicio; }

  void  setModoParadaSeca(bool seca)    { _paradaSeca = seca; }
  bool  getModoParadaSeca() const       { return _paradaSeca; }

  void  setSuavidadArranquePct(float p) { _suavidadArranquePct = constrain(p, 5.0f, 100.0f); }
  float getSuavidadArranquePct() const  { return _suavidadArranquePct; }

  void tick(float dt);

private:
  void fijarSentido(bool horario);

  bool _enMarcha = false;
  bool _horario = true;
  bool _invirtiendo = false;
  bool _enEmergencia = false;
  bool _paradaSeca = false;

  float _objetivo = RPM_INICIO;
  float _actual = 0.0f;
  float _rpmGuardada = RPM_INICIO;
  float _rpmInicio = RPM_INICIO;
  float _suavidadArranquePct = 50.0f; // 50% = rampa nominal calibrada
  uint32_t _fActual = 0;
  uint32_t _tDetenido_ms = 0;
  uint32_t _tEnable_ms = 0;

  float    _mlPorVuelta  = ML_POR_VUELTA;
  uint16_t _pulsosPorRev = PULSOS_POR_REV;
};
