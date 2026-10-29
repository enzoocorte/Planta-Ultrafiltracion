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
  _modoMantenimiento = MANT_INACTIVO;
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
  _invirtiendo = false;
  _modoMantenimiento = MANT_INACTIVO;
  if (_objetivo >= RPM_MIN) {
    _rpmGuardada = _objetivo;
  }
  if (_paradaSeca) {
    _actual = 0.0f;
    _objetivo = 0.0f;
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
  _modoMantenimiento = MANT_INACTIVO;
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
  _modoMantenimiento = MANT_INACTIVO;
  digitalWrite(PIN_ENA, LOW); // Restablece habilitacion de potencia
  _actual = 0.0f;
  _objetivo = (_rpmGuardada >= RPM_MIN) ? _rpmGuardada : RPM_INICIO;
}

// ------------------------------------------------------------------------------
// GESTIÓN DE MODO MANTENIMIENTO PREVENTIVO
// ------------------------------------------------------------------------------
void Bomba::iniciarVaiven(uint32_t duracion_s) {
  if (_enEmergencia) return;
  _modoMantenimiento = MANT_VAIVEN;
  _tFinVaiven_ms = millis() + (duracion_s * 1000UL);
  _tFaseVaiven_ms = millis();
  _faseVaiven = 0;
  _enMarcha = true;
  _invirtiendo = false;
  digitalWrite(PIN_ENA, LOW);
  _tEnable_ms = millis();
  fijarSentido(true); // Arranca adelante
  _actual = RPM_DESPEGUE;
  _objetivo = 25.0f;  // 25 RPM velocidad controlada para relubricación y flexión
}

void Bomba::detenerMantenimiento() {
  _modoMantenimiento = MANT_INACTIVO;
  detener();
}

void Bomba::iniciarJog(bool adelante) {
  if (_enEmergencia) return;
  _modoMantenimiento = adelante ? MANT_JOG_FWD : MANT_JOG_REV;
  _enMarcha = true;
  _invirtiendo = false;
  digitalWrite(PIN_ENA, LOW);
  _tEnable_ms = millis();
  fijarSentido(adelante);
  _actual = RPM_DESPEGUE;
  _objetivo = 18.0f;  // 18 RPM para arrastre suave y seguro en cambio de manguera
}

void Bomba::detenerJog() {
  if (_modoMantenimiento == MANT_JOG_FWD || _modoMantenimiento == MANT_JOG_REV) {
    detenerMantenimiento();
  }
}

void Bomba::ejecutarGuardianShift() {
  if (_enEmergencia || _enMarcha) return;
  _modoMantenimiento = MANT_GUARDIAN_SHIFT;
  _tFinVaiven_ms = millis() + 1200; // 1.2 segundos a 18 RPM = ~130° de rotación
  _enMarcha = true;
  _invirtiendo = false;
  digitalWrite(PIN_ENA, LOW);
  _tEnable_ms = millis();
  fijarSentido(true);
  _actual = RPM_DESPEGUE;
  _objetivo = 18.0f;
}

uint32_t Bomba::tiempoRestanteVaiven_s() const {
  if (_modoMantenimiento != MANT_VAIVEN) return 0;
  uint32_t now = millis();
  if (now >= _tFinVaiven_ms) return 0;
  return (uint32_t)((_tFinVaiven_ms - now) / 1000UL);
}

const char* Bomba::estadoMantenimientoStr() const {
  switch (_modoMantenimiento) {
    case MANT_VAIVEN:
      if (_faseVaiven == 0) return "Vaivén: Avanzando (+)";
      if (_faseVaiven == 1) return "Vaivén: Pausa intermedia";
      if (_faseVaiven == 2) return "Vaivén: Retornando (-)";
      if (_faseVaiven == 3) return "Vaivén: Pausa intermedia";
      return "Vaivén Activo";
    case MANT_JOG_FWD:
      return "Jog Adelante (+)";
    case MANT_JOG_REV:
      return "Jog Atrás (-)";
    case MANT_GUARDIAN_SHIFT:
      return "Guardián Standby: Rotación 120°";
    default:
      return "Inactivo";
  }
}

bool Bomba::setRPM(float rpm) {
  if (!std::isfinite(rpm) || _enEmergencia || _modoMantenimiento != MANT_INACTIVO) return false;
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
    _rpmGuardada = (_objetivo >= RPM_MIN) ? _objetivo : RPM_INICIO;
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

  // Máquina de estados de Modo Mantenimiento
  if (_modoMantenimiento == MANT_VAIVEN) {
    if (millis() >= _tFinVaiven_ms) {
      detenerMantenimiento();
      return;
    }
    uint32_t dtFase = millis() - _tFaseVaiven_ms;
    if (_faseVaiven == 0) { // Avanza (2.5s)
      fijarSentido(true);
      _objetivo = 25.0f;
      if (dtFase >= 2500) {
        _faseVaiven = 1;
        _tFaseVaiven_ms = millis();
        _objetivo = 0.0f;
      }
    } else if (_faseVaiven == 1) { // Pausa (0.8s)
      _objetivo = 0.0f;
      if (dtFase >= 800) {
        _faseVaiven = 2;
        _tFaseVaiven_ms = millis();
        fijarSentido(false);
        _objetivo = 25.0f;
      }
    } else if (_faseVaiven == 2) { // Retrocede (2.5s)
      fijarSentido(false);
      _objetivo = 25.0f;
      if (dtFase >= 2500) {
        _faseVaiven = 3;
        _tFaseVaiven_ms = millis();
        _objetivo = 0.0f;
      }
    } else { // Pausa (0.8s)
      _objetivo = 0.0f;
      if (dtFase >= 800) {
        _faseVaiven = 0;
        _tFaseVaiven_ms = millis();
        fijarSentido(true);
        _objetivo = 25.0f;
      }
    }
  } else if (_modoMantenimiento == MANT_GUARDIAN_SHIFT) {
    if (millis() >= _tFinVaiven_ms) {
      detenerMantenimiento();
      return;
    }
    _objetivo = 18.0f;
  } else if (_modoMantenimiento == MANT_JOG_FWD) {
    fijarSentido(true);
    _objetivo = 18.0f;
  } else if (_modoMantenimiento == MANT_JOG_REV) {
    fijarSentido(false);
    _objetivo = 18.0f;
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

  // Generación de pulsos PWM por hardware LEDC (solo si la bomba está en marcha)
  if (_enMarcha && _actual >= 1.0f) {
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
