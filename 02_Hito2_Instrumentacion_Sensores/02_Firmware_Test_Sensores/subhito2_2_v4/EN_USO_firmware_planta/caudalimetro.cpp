#include "caudalimetro.h"

Caudalimetro::Caudalimetro(uint8_t pin, float k, const char* nombre, bool esAlimentacion)
  : _pin(pin), _k(k), _nombre(nombre), _esAlimentacion(esAlimentacion) {}

void Caudalimetro::begin() {
  // Activar resistencia INPUT_PULLUP interna del ESP32 (45 kΩ a 3.3V) en paralelo con el circuito RC externo.
  // Garantiza máxima rigidez contra acoplamiento inductivo y asegura nivel lógico HIGH
  // evitando que el pin actúe como antena ante el campo magnético del motor NEMA 34.
  pinMode(_pin, INPUT_PULLUP);
  attachInterruptArg(digitalPinToInterrupt(_pin), isrPuente, this, FALLING);
}

void Caudalimetro::actualizar(float dt_s, bool bombaEmpuja) {
  // Captura atómica de variables acumuladas por la ISR en la ventana transcurrida
  portENTER_CRITICAL(&_mux);
  uint32_t n      = _pulsos;
  _pulsos         = 0;
  uint32_t per_us = _periodo_us;
  uint32_t t_prim = _t_primero;
  uint32_t t_ult  = _t_ultimo;
  portEXIT_CRITICAL(&_mux);

  // 1. CÁLCULO DE FRECUENCIA CON MÉTODO DE PERÍODO RECÍPROCO DE ALTA RESOLUCIÓN:
  // Exige al menos 2 pulsos continuos en la ventana de 1 segundo para validar rotación real.
  // Un pulso único o aislado es descartado como ruido transitorio electromagnético.
  if (n >= 2 && (t_ult - t_prim) > 0) {
    _f = ((float)(n - 1) * 1000000.0f) / (float)(t_ult - t_prim);
  } else {
    _f = 0.0f;
  }

  // 2. UMBRAL DE CORTE DE VELOCIDAD MÍNIMA (DEADBAND / ZERO-FLOW CUT-OFF):
  // La turbina YF-S401 tiene fricción mecánica estática en su eje cerámico. Físicamente no gira
  // de forma continua por debajo de ~2.5 Hz (< 20 mL/min).
  // Toda frecuencia < 2.0 Hz es cortada a cero absoluto para garantizar 0.0 mL/min cuando
  // no circula agua (eliminando el caudal fantasma de permeado por acoplamiento con el motor).
  if (_f < 2.0f) {
    _f = 0.0f;
  }

  // 3. CÁLCULO DE CAUDAL INSTANTÁNEO EN mL/min:
  // Q [mL/min] = (F * 1000.0) / K
  float q = (_f * 1000.0f) / _k;

  // Filtro de plausibilidad física (corte de picos transitorios por perturbación EMI)
  if (q > Q_MAX_FISICO_MLMIN) {
    q = 0.0f;
    _f = 0.0f;
    Serial.printf("[%s] Ruido EMI descartado: %lu pulsos espurios\n", _nombre, (unsigned long)n);
  } else if (_f > 0.0f) {
    // 4. INTEGRACIÓN DE VOLUMEN TOTALIZADO EN LITROS:
    // Solo se acumula volumen si hay flujo real continuo confirmado
    _vol += (float)n / (_k * 60.0f);
  }

  // 5. FILTRO EXPONENCIAL PONDERADO (EMA) SINTONIZADO PARA FLUJO PERISTÁLTICO:
  // Suaviza la pulsación rodillo a rodillo del cabezal peristáltico de 3 rodillos (tau ≈ 3.5 segundos).
  // Evita que la lectura salte o baile en pantalla al mover la manguera en la probeta.
  if (q > 0.0f) {
    if (_q == 0.0f) {
      _q = q; // Respuesta rápida desde reposo
    } else {
      _q = 0.20f * q + 0.80f * _q; // Filtrado estable
    }
  } else {
    // Decaimiento rápido a cero al detenerse el flujo
    _q = 0.5f * _q;
    if (_q < 1.0f) _q = 0.0f;
  }

  // 6. DIAGNÓSTICO ASIMÉTRICO DE PÉRDIDA DE SEÑAL / CABLE CORTADO:
  // - Alimentación: si la bomba empuja (RPM > 1) y pasan 5 segundos sin pulsos -> FALLA CRÍTICA.
  // - Permeado: caudal nulo es una condición admisible (válvula cerrada o baja TMP) -> NO alarma.
  if (n > 0 || _f > 0.05f) {
    _tiempoSinPulso_s = 0.0f;
    _fallo = false;
  } else if (bombaEmpuja && _esAlimentacion) {
    _tiempoSinPulso_s += dt_s;
    if (_tiempoSinPulso_s >= 5.0f) _fallo = true;
  } else {
    _tiempoSinPulso_s = 0.0f;
    _fallo = false;
  }
}

void IRAM_ATTR Caudalimetro::isrPuente(void* arg) {
  Caudalimetro* c = reinterpret_cast<Caudalimetro*>(arg);
  uint32_t t = micros();

  portENTER_CRITICAL_ISR(&c->_mux);
  // Diferencia sin signo uint32_t: segura ante desbordamiento de micros() cada 71.58 min
  uint32_t dt = t - c->_t_ultimo;

  // Blanking anti-rebote: descarta transitorios mecánicos y capacitivos (FILTRO_RUIDO_US = 1500 us)
  if (dt >= FILTRO_RUIDO_US) {
    if (c->_pulsos == 0) {
      c->_t_primero = t;
    }
    // Si el tiempo transcurrido es menor a 5 segundos, registramos el período inter-pulso real
    if (dt < 5000000UL) {
      c->_periodo_us = dt;
    } else {
      c->_periodo_us = 0; // Tras una parada prolongada, se descarta el período espurio
    }
    c->_t_ultimo = t;
    c->_pulsos++;
  }
  portEXIT_CRITICAL_ISR(&c->_mux);
}
