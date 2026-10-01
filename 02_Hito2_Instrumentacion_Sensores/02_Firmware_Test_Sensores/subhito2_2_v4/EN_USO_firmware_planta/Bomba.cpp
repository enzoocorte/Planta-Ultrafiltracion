#include "Bomba.h"

// ==============================================================================
// IMPLEMENTACIÓN DE LA CLASE BOMBA
// Generación PWM por hardware LEDC con frecuencia mínima segura (>= 50 Hz)
// ==============================================================================

void Bomba::begin() {
  pinMode(PIN_DIR, OUTPUT);
  fijarSentido(true); // Sentido horario por defecto

#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
  ledcAttach(PIN_PUL, 800, 10);
  ledcWrite(PIN_PUL, 0); // Reposo: LOW -> Opto OFF (motor libre)
#else
  ledcSetup(0, 800, 10);
  ledcAttachPin(PIN_PUL, 0);
  ledcWrite(0, 0);
#endif
}

void Bomba::arrancar() {
  _enMarcha = true;
  if (_objetivo < RPM_MIN) {
    _objetivo = (_rpmGuardada >= RPM_MIN) ? _rpmGuardada : RPM_INICIO;
  }
}

void Bomba::detener() {
  _enMarcha = false;
  _invirtiendo = false;
  if (_objetivo >= RPM_MIN) {
    _rpmGuardada = _objetivo;
  }
}

bool Bomba::setRPM(float rpm) {
  if (!std::isfinite(rpm)) return false;
  float r = constrain(rpm, RPM_MIN, RPM_MAX);
  if (_invirtiendo) {
    _rpmGuardada = r;   // Almacena consigna si el usuario mueve slider durante el frenado de inversión
  } else {
    _objetivo = r;
    _rpmGuardada = r;
  }
  return true;
}

void Bomba::toggleSentido() {
  if (_invirtiendo) return;
  if (_actual < 1.0f) {
    fijarSentido(!_horario);
  } else {
    _invirtiendo = true;
    _rpmGuardada = (_objetivo >= RPM_MIN) ? _objetivo : RPM_INICIO;
    _objetivo = 0.0f;
  }
}

void Bomba::fijarSentido(bool horario) {
  _horario = horario;
  digitalWrite(PIN_DIR, horario ? LOW : HIGH);
}

void Bomba::tick(float dt) {
  float objetivo = _enMarcha ? _objetivo : 0.0f;

  if (_actual < objetivo) {
    // Si la bomba está arrancando desde 0, iniciamos suavemente en 1.0 RPM (53 Hz LEDC)
    if (_actual < 1.0f) {
      _actual = 1.0f;
    }
    
    // Rampa de aceleración progresiva
    float tasaAcel = ACEL_NOMINAL_RPM_S;
    if (_actual < 10.0f) {
      tasaAcel = ACEL_ARRANQUE_RPM_S + (_actual / 10.0f) * (ACEL_NOMINAL_RPM_S - ACEL_ARRANQUE_RPM_S);
    }
    float delta = objetivo - _actual;
    if (delta < 3.0f) {
      tasaAcel = fmaxf(0.8f, tasaAcel * (delta / 3.0f));
    }
    _actual = fminf(objetivo, _actual + tasaAcel * dt);

  } else if (_actual > objetivo) {
    // Desaceleración / Frenado
    if (!_enMarcha) {
      // PARADA RÁPIDA: Frenado ágil en menos de 1.5s
      _actual = fmaxf(0.0f, _actual - FRENADO_PARADA_RPM_S * dt);
      if (_actual < 1.0f) {
        _actual = 0.0f;
      }
    } else {
      // Reducción suave de velocidad en marcha
      float tasaDecel = DESACEL_AJUSTE_RPM_S;
      float delta = _actual - objetivo;
      if (delta < 3.0f) {
        tasaDecel = fmaxf(1.0f, tasaDecel * (delta / 3.0f));
      }
      _actual = fmaxf(objetivo, _actual - tasaDecel * dt);
    }
  }

  // Al llegar a 0 RPM durante una inversión: cortar pulsos, conmutar DIR con margen de seguridad y re-acelerar
  if (_invirtiendo && _actual <= 0.1f) {
#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    ledcWrite(PIN_PUL, 0);
#else
    ledcWrite(0, 0);
#endif
    _fActual = 0;
    fijarSentido(!_horario);
    _invirtiendo = false;
    _objetivo = (_rpmGuardada >= RPM_MIN) ? _rpmGuardada : RPM_INICIO;
  }

  // Generación de pulsos PWM por hardware LEDC
  if (_actual >= 1.0f) {
    uint32_t f = (uint32_t)(_actual * (float)_pulsosPorRev / 60.0f);
    if (f < 50) f = 50; // Límite de seguridad mínimo del temporizador LEDC de ESP32

#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    if (f != _fActual) { 
      ledcChangeFrequency(PIN_PUL, f, 10); 
      _fActual = f; 
    }
    ledcWrite(PIN_PUL, 512); // Ciclo de trabajo 50%
#else
    if (f != _fActual) { 
      ledcSetup(0, f, 10); 
      _fActual = f; 
    }
    ledcWrite(0, 512);
#endif
  } else if (_fActual != 0) {
    // Corte inmediato de pulsos al detenerse
#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    ledcWrite(PIN_PUL, 0); // Reposo: LOW -> Opto OFF
#else
    ledcWrite(0, 0);
#endif
    _fActual = 0;
  }
}
