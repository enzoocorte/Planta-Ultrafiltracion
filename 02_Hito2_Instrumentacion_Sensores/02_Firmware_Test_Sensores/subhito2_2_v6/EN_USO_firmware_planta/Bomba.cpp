#include "Bomba.h"

// ==============================================================================
// IMPLEMENTACIÓN DE LA CLASE BOMBA (Tesis UNSa 2026)
// Corrección Core ESP32 v3.x: evita re-ejecutar ledcAttach() en cada arranque,
// utilizando ledcChangeFrequency() sobre el canal ya inicializado en begin().
// ==============================================================================

void Bomba::begin() {
  pinMode(PIN_DIR, OUTPUT);
  digitalWrite(PIN_DIR, LOW);
  fijarSentido(true); // Sentido horario por defecto (Filtración)

  pinMode(PIN_ENA, OUTPUT);
  digitalWrite(PIN_ENA, LOW); // Habilitar driver DM860

  // Inicializar temporizador LEDC para el pin de pulsos
#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
  ledcAttach(PIN_PUL, 800, 10);
  ledcWrite(PIN_PUL, 0); // Reposo seguro: Duty 0 = nivel bajo continuo sin ningún pulso
#else
  ledcSetup(0, 800, 10);
  ledcAttachPin(PIN_PUL, 0);
  ledcWrite(0, 0);
#endif

  _enMarcha = false;
  _enEmergencia = false;
  _actual = 0.0f;
  _fActual = 0;
  _tDetenido_ms = millis();
}

void Bomba::arrancar() {
  _enEmergencia = false;      // Desbloqueo ante orden de arranque
  _enMarcha = true;
  _tDetenido_ms = 0;
  digitalWrite(PIN_ENA, LOW); // Activa driver
  _tEnable_ms = millis();
  if (_objetivo < RPM_MIN) {
    _objetivo = (_rpmGuardada >= RPM_MIN) ? _rpmGuardada : _rpmInicio;
  }
}

void Bomba::detener() {
  _enMarcha = false;
  if (_invirtiendo) {
    // Si el usuario detiene la bomba mientras estaba frenando para invertir giro,
    // restauramos la consigna guardada para que el SCADA no muestre obj_rpm = 0.0
    _objetivo = (_rpmGuardada >= RPM_MIN) ? _rpmGuardada : _rpmInicio;
    _invirtiendo = false;
  } else if (_objetivo >= RPM_MIN) {
    _rpmGuardada = _objetivo;
  }

  if (_paradaSeca) {
    _actual = 0.0f;
    _fActual = 0;
#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    ledcWrite(PIN_PUL, 0);
#else
    ledcWrite(0, 0);
#endif
  }
}

void Bomba::paradaEmergencia() {
  _enEmergencia = true;
  digitalWrite(PIN_ENA, HIGH); // Corte fisico instantaneo en driver DM860 (< 1 ms)
  _enMarcha = false;
  _invirtiendo = false;
  _actual = 0.0f;
  _objetivo = 0.0f;
  _fActual = 0;
#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
  ledcWrite(PIN_PUL, 0);
#else
  ledcWrite(0, 0);
#endif
}

void Bomba::rearmarEmergencia() {
  _enEmergencia = false;
  digitalWrite(PIN_ENA, LOW); // Restablece habilitacion de potencia
  _actual = 0.0f;
  _objetivo = (_rpmGuardada >= RPM_MIN) ? _rpmGuardada : _rpmInicio; // V6.6: respeta RPM de inicio configurada
}

bool Bomba::setRPM(float rpm) {
  if (!std::isfinite(rpm) || _enEmergencia) return false;
  float r = constrain(rpm, RPM_MIN, RPM_MAX);
  if (_invirtiendo) {
    _rpmGuardada = r;   // Almacena consigna si el usuario mueve el slider durante el frenado de inversión
  } else {
    _objetivo = r;
    _rpmGuardada = r;
  }
  return true;
}

void Bomba::toggleSentido() {
  if (_invirtiendo || _enEmergencia) return;
  if (_actual < 1.0f) {
    fijarSentido(!_horario);
  } else {
    _invirtiendo = true;
    _rpmGuardada = (_objetivo >= RPM_MIN) ? _objetivo : _rpmInicio;
    _objetivo = 0.0f;
  }
}

void Bomba::fijarSentido(bool horario) {
  _horario = horario;
  digitalWrite(PIN_DIR, horario ? LOW : HIGH);
}

void Bomba::tick(float dt) {
  if (_enEmergencia) {
    _actual = 0.0f;
    _fActual = 0;
#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    ledcWrite(PIN_PUL, 0);
#else
    ledcWrite(0, 0);
#endif
    return;
  }

  // P0-2: Validación y limitación de dt para proteger la cinemática del NEMA 34
  if (!(dt > 0.0f)) return;
  if (dt > DT_MAX_RAMPA_S) dt = DT_MAX_RAMPA_S; // Nunca avanza más de 0.10s si el loop se demora

  // P0-3: Tiempo de establecimiento del driver DM860 tras habilitar ENA (100 ms)
  if (_enMarcha && (millis() - _tEnable_ms) < TIEMPO_ENA_MS) {
    return;
  }

  // Gestión de reposo del driver Leadshine DM860
  if (!_enMarcha && _actual <= 0.0f) {
    if (_tDetenido_ms == 0) {
      _tDetenido_ms = millis();
    }
  } else if (_enMarcha) {
    _tDetenido_ms = 0;
    digitalWrite(PIN_ENA, LOW); // Mantener habilitado con torque activo
  }

  float objetivo = _enMarcha ? _objetivo : 0.0f;

  if (_enMarcha && _actual < objetivo) {
    // Despegue inicial fluido: arranca en RPM_DESPEGUE (6.0 RPM = 320 Hz)
    // eliminando el arrastre lento de 1 RPM y la zona de resonancia/calentamiento
    if (_actual < RPM_DESPEGUE) {
      _actual = RPM_DESPEGUE;
    }

    // Rampa de aceleración ajustada por el dimmer de fluidez (0% a 100%)
    float factorAcel = constrain(_suavidadArranquePct, 5.0f, 100.0f) / 50.0f; // 50% = nominal (1.0)
    float acelArranque = constrain(ACEL_ARRANQUE_RPM_S * factorAcel, 3.0f, 40.0f);
    float acelNominal  = constrain(ACEL_NOMINAL_RPM_S  * factorAcel, 5.0f, 60.0f);

    float delta = objetivo - _actual;
    float tasaAcel = acelNominal;
    if (_actual < 12.0f) {
      tasaAcel = acelArranque + ((_actual - RPM_DESPEGUE) / (12.0f - RPM_DESPEGUE)) * (acelNominal - acelArranque);
    }
    // Suavizado en la llegada a la consigna
    if (delta < 2.0f) {
      tasaAcel = fmaxf(2.0f, tasaAcel * (delta / 2.0f));
    }
    _actual = fminf(objetivo, _actual + tasaAcel * dt);

  } else if (_actual > objetivo) {
    // Desaceleración / Frenado
    if (!_enMarcha) {
      if (_paradaSeca) {
        _actual = 0.0f;
      } else {
        // Parada rápida controlada en menos de 0.8s
        _actual = fmaxf(0.0f, _actual - FRENADO_PARADA_RPM_S * dt);
        if (_actual < 1.0f) {
          _actual = 0.0f;
        }
      }
    } else {
      // Reducción suave de velocidad en marcha
      float tasaDecel = DESACEL_AJUSTE_RPM_S;
      float delta = _actual - objetivo;
      if (delta < 3.0f) {
        tasaDecel = fmaxf(2.0f, tasaDecel * (delta / 3.0f));
      }
      _actual = fmaxf(objetivo, _actual - tasaDecel * dt);
    }
  }

  // Al llegar a 0 RPM durante una inversión: cortar pulsos, conmutar DIR y re-acelerar
  if (_invirtiendo && _actual <= 0.1f) {
#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    ledcWrite(PIN_PUL, 0);
#else
    ledcWrite(0, 0);
#endif
    _fActual = 0;
    fijarSentido(!_horario);
    _invirtiendo = false;
    _objetivo = (_rpmGuardada >= RPM_MIN) ? _rpmGuardada : _rpmInicio;
  }

  // Generación de pulsos PWM por hardware LEDC (mientras el motor deba girar y no haya emergencia)
  if (!_enEmergencia && _actual >= 1.0f) {
    uint32_t f = (uint32_t)(_actual * (float)_pulsosPorRev / 60.0f);
    if (f < 50) f = 50; // Límite de seguridad mínimo del temporizador LEDC de ESP32

#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    if (f != _fActual) {
      uint32_t retF = ledcChangeFrequency(PIN_PUL, f, 10);
      if (retF > 0) _fActual = retF;
      else _fActual = f;
    }
    ledcWrite(PIN_PUL, 512); // Duty 50% = emitiendo tren de pulsos
#else
    if (f != _fActual) {
      uint32_t retF = ledcSetup(0, f, 10);
      if (retF > 0) _fActual = retF;
      else _fActual = f;
    }
    ledcWrite(0, 512);
#endif
  } else {
    // Corte inmediato y seguro de pulsos al detenerse o en reposo (Duty 0 = GND constante)
#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    ledcWrite(PIN_PUL, 0);
#else
    ledcWrite(0, 0);
#endif
    _fActual = 0;
  }
}
