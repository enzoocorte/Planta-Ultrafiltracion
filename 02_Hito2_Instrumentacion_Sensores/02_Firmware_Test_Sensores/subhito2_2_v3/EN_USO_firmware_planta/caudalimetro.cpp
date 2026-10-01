#include "caudalimetro.h"

portMUX_TYPE Caudalimetro::_mux = portMUX_INITIALIZER_UNLOCKED;

Caudalimetro::Caudalimetro(uint8_t pin, float k, const char* nombre)
  : _pin(pin), _k(k), _nombre(nombre) {}

void Caudalimetro::begin() {
  pinMode(_pin, INPUT_PULLUP);
  attachInterruptArg(digitalPinToInterrupt(_pin), isrPuente, this, FALLING);
}

void Caudalimetro::actualizar(float dt_s, bool bombaEmpuja) {
  portENTER_CRITICAL(&_mux);
  uint32_t n = _pulsos;
  _pulsos = 0;
  portEXIT_CRITICAL(&_mux);

  // Frecuencia física instantánea en Hz
  _f = (dt_s > 0.0f) ? ((float)n / dt_s) : 0.0f;

  // Caudal instantáneo: Q (L/min) = F / K => Q (mL/min) = (F * 1000) / K
  float q = (_f * 1000.0f) / _k;

  if (q > Q_MAX_FISICO_MLMIN) {
    q = 0.0f;
    Serial.printf("[%s] Ruido descartado: %lu pulsos espurios (f=%.1f Hz)\n", _nombre, (unsigned long)n, _f);
  }

  // Suavizado exponencial ponderado (EMA)
  _q = (n > 0) ? (0.4f * q + 0.6f * _q) : 0.0f;

  // Integración de volumen totalizado en Litros: n / (K * 60)
  _vol += (float)n / (_k * 60.0f);

  // Diagnóstico de pérdida de flujo o cable desconectado
  if (n > 0) {
    _segSinPulso = 0;
    _fallo = false;
  } else if (bombaEmpuja) {
    if (++_segSinPulso >= 5) _fallo = true;
  } else {
    _segSinPulso = 0;
    _fallo = false;
  }
}

void IRAM_ATTR Caudalimetro::isrPuente(void* arg) {
  Caudalimetro* c = reinterpret_cast<Caudalimetro*>(arg);
  uint32_t t = micros();
  // Blanking anti-rebote en microsegundos
  if (t - c->_t_ultimo >= FILTRO_RUIDO_US) {
    portENTER_CRITICAL_ISR(&_mux);
    c->_pulsos++;
    portEXIT_CRITICAL_ISR(&_mux);
    c->_t_ultimo = t;
  }
}
