#pragma once
#include <Arduino.h>
#include "config.h"

// ============================================================
//  BOMBA PERISTÁLTICA — rampa + inversión segura (NEMA 34/DM860)
//  Esquema CÁTODO COMÚN: PUL+/DIR+ a GPIO, PUL-/DIR- a GND.
//  HIGH = optoacoplador ON | LOW = optoacoplador OFF
// ============================================================
class Bomba {
public:
  void begin() {
    pinMode(PIN_DIR, OUTPUT);
    fijarSentido(true);
    ledcAttach(PIN_PUL, 800, 10);
    ledcWrite(PIN_PUL, 0);                 // reposo: LOW → opto OFF (motor libre de pulsos)
  }

  void arrancar()        { _enMarcha = true; }
  void detener()         { _enMarcha = false; _invirtiendo = false; }
  void setRPM(float rpm) { _objetivo = constrain(rpm, RPM_MIN, RPM_MAX); }

  void toggleSentido() {
    if (_invirtiendo) return;                        // inversión ya en curso: ignorar
    if (_actual < 5.0f) fijarSentido(!_horario);     // parado → giro directo
    else {                                           // en marcha → frenar, invertir, acelerar
      _invirtiendo = true;
      _rpmGuardada = _objetivo;
      _objetivo = 0.0f;
    }
  }

  float rpmActual() const           { return _actual; }
  bool  enMarcha() const            { return _enMarcha; }
  bool  invirtiendo() const         { return _invirtiendo; }
  bool  sentidoHorario() const      { return _horario; }
  float caudalTeorico_mLmin() const { return _actual * ML_POR_VUELTA; }

  // Llamar cada ~50 ms
  void tick(float dt) {
    float objetivo = _enMarcha ? _objetivo : 0.0f;
    if      (_actual < objetivo) _actual = min(objetivo, _actual + ACEL_RPM_S * dt);
    else if (_actual > objetivo) _actual = max(objetivo, _actual - ACEL_RPM_S * dt);

    if (_invirtiendo && _actual <= 0.1f) {           // llegó a 0 RPM
      fijarSentido(!_horario);
      _invirtiendo = false;
      _objetivo = _rpmGuardada;
    }

    if (_actual >= 5.0f) {                           // generar pulsos
      uint32_t f = (uint32_t)(_actual * PULSOS_POR_REV / 60.0f);
      if (f != _fActual) { ledcChangeFrequency(PIN_PUL, f, 10); _fActual = f; }
      ledcWrite(PIN_PUL, 512);                       // 50 % duty
    } else if (_fActual != 0) {
      ledcWrite(PIN_PUL, 0);                         // reposo: LOW → opto OFF
      _fActual = 0;
    }
  }

private:
  void fijarSentido(bool horario) {
    _horario = horario;
    // CÁTODO COMÚN: HIGH conduce el opto. Si al probar, "FILTRACIÓN" resulta
    // ser retrolavado, intercambia HIGH y LOW en esta línea.
    digitalWrite(PIN_DIR, horario ? HIGH : LOW);
  }

  bool _enMarcha = false, _horario = true, _invirtiendo = false;
  float _objetivo = RPM_INICIO, _actual = 0.0f, _rpmGuardada = RPM_INICIO;
  uint32_t _fActual = 0;
};