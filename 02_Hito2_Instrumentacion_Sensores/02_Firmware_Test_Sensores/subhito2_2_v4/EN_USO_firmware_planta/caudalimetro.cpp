#include "caudalimetro.h"

Caudalimetro::Caudalimetro(uint8_t pin, float k, const char* nombre)
  : _pin(pin), _k(k), _nombre(nombre) {}

void Caudalimetro::begin() {
  pinMode(_pin, INPUT_PULLUP);
  attachInterruptArg(digitalPinToInterrupt(_pin), isrPuente, this, FALLING);
}

void Caudalimetro::actualizar(float dt_s, bool bombaEmpuja) {
  portENTER_CRITICAL(&_mux);
  uint32_t n       = _pulsos;
  _pulsos          = 0;
  uint32_t per_us  = _periodo_us;
  uint32_t t_prim  = _t_primero;
  uint32_t t_ult   = _t_ultimo;
  portEXIT_CRITICAL(&_mux);

  // 1. Cálculo de Frecuencia con Conteo Recíproco Híbrido:
  // - Con 2 o más pulsos en la ventana: f = (n - 1) / (t_ult - t_prim) -> resolución microsegundo
  // - Con 1 pulso: f = 1e6 / periodo_us
  // - Con 0 pulsos: verificar si el último pulso es reciente para evitar falsos escalones a 0
  if (n >= 2 && t_ult > t_prim) {
    _f = ((float)(n - 1) * 1000000.0f) / (float)(t_ult - t_prim);
  } else if (n == 1 && per_us > 0) {
    _f = 1000000.0f / (float)per_us;
  } else if (n == 0) {
    uint32_t tAhora = micros();
    uint32_t tSinFlanco = tAhora - t_ult;
    // Si no hubo pulsos en este segundo pero el flujo es bajo (<10 Hz) y el último pulso fue reciente:
    if (per_us > 0 && tSinFlanco < (25 * per_us) / 10 && _f > 0.1f) {
      // Mantener frecuencia estimada suavemente sin colapsar a 0
    } else {
      _f = 0.0f;
    }
  }

  // 2. Caudal instantáneo: Q (L/min) = F / K => Q (mL/min) = (F * 1000) / K
  float q = (_f * 1000.0f) / _k;

  if (q > Q_MAX_FISICO_MLMIN) {
    q = 0.0f;
    _f = 0.0f;
    Serial.printf("[%s] Ruido descartado: %lu pulsos espurios\n", _nombre, (unsigned long)n);
  } else {
    // 3. Integración de volumen totalizado en Litros: n / (K * 60)
    // SOLO se integran pulsos físicamente válidos (no contaminados por espurios)
    _vol += (float)n / (_k * 60.0f);
  }

  // 4. Suavizado exponencial ponderado (EMA)
  if (n > 0 || _f > 0.1f) {
    _q = 0.4f * q + 0.6f * _q;
  } else {
    _q = 0.0f;
  }

  // 5. Diagnóstico de pérdida de flujo o cable desconectado
  if (n > 0 || _f > 0.1f) {
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
  uint32_t dt = t - c->_t_ultimo;

  // Blanking anti-rebote en microsegundos
  if (dt >= FILTRO_RUIDO_US) {
    portENTER_CRITICAL_ISR(&c->_mux);
    if (c->_pulsos == 0) {
      c->_t_primero = t;
    }
    c->_periodo_us = dt;
    c->_t_ultimo   = t;
    c->_pulsos++;
    portEXIT_CRITICAL_ISR(&c->_mux);
  }
}
