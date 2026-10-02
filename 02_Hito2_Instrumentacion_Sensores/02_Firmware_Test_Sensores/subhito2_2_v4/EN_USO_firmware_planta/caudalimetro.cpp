#include "caudalimetro.h"

Caudalimetro::Caudalimetro(uint8_t pin, float k, const char* nombre, bool esAlimentacion)
  : _pin(pin), _k(k), _nombre(nombre), _esAlimentacion(esAlimentacion) {}

void Caudalimetro::begin() {
  // Placa 2 ya dispone de pull-up externo de 4.7 kΩ a 3.3V y capacitor de 100 nF.
  // Se configura como INPUT de alta impedancia para respetar los niveles del front-end.
  pinMode(_pin, INPUT);
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

  uint32_t tAhora = micros();
  // Diferencia sin signo uint32_t: matemáticamente inmune al desbordamiento (rollover de 71.58 min)
  uint32_t tSinFlanco = tAhora - t_ult;

  // 1. CÁLCULO DE FRECUENCIA CON MÉTODO DE PERÍODO RECÍPROCO DE ALTA RESOLUCIÓN:
  // - n >= 2 pulsos: medimos el tiempo exacto entre el 1er y último pulso dentro de la ventana.
  //   Resolución en microsegundos; elimina por completo el error de discretización ±1 pulso.
  // - n == 1 pulso: la ventana solo capturó un flanco, se usa el período entre pulsos sucesivos per_us.
  // - n == 0 pulsos: no hubo eventos en la ventana (régimen bajo o detención).
  if (n >= 2 && (t_ult - t_prim) > 0) {
    _f = ((float)(n - 1) * 1000000.0f) / (float)(t_ult - t_prim);
  } else if (n == 1 && per_us > 0) {
    _f = 1000000.0f / (float)per_us;
  }

  // 2. COTA FÍSICA SUPERIOR CONTINUA EN AUSENCIA DE PULSOS RECIENTES:
  // Si transcurrió tSinFlanco microsegundos desde el último pulso registrado,
  // la física impone que la frecuencia instantánea real no puede exceder 1e6 / tSinFlanco.
  // Esto garantiza un decaimiento suave y asintótico hacia cero cuando la bomba frena,
  // impidiendo que la frecuencia quede congelada artificialmente.
  if (tSinFlanco > 0) {
    float f_max_posible = 1000000.0f / (float)tSinFlanco;
    if (_f > f_max_posible) {
      _f = f_max_posible;
    }
  }

  // Decaimiento estricto a cero absoluto tras 3.0 segundos sin pulsos
  if (n == 0 && tSinFlanco > 3000000UL) {
    _f = 0.0f;
  }

  // 3. CÁLCULO DE CAUDAL INSTANTÁNEO EN mL/min:
  // Por definición metrológica: K está en [Hz / (L/min)].
  // Q [L/min] = F / K  ===>  Q [mL/min] = (F * 1000.0) / K
  float q = (_f * 1000.0f) / _k;

  // Filtro de plausibilidad física (corte de picos transitorios por perturbación EMI)
  if (q > Q_MAX_FISICO_MLMIN) {
    q = 0.0f;
    _f = 0.0f;
    Serial.printf("[%s] Ruido descartado: %lu pulsos espurios\n", _nombre, (unsigned long)n);
  } else {
    // 4. INTEGRACIÓN DE VOLUMEN TOTALIZADO EN LITROS:
    // 1 L/min = (1/60) L/s  ==>  Pulsos por Litro = K * 60
    // Vol [L] = pulsos / (K * 60)
    // Solo se acumulan pulsos físicamente plausibles.
    _vol += (float)n / (_k * 60.0f);
  }

  // 5. FILTRO EXPONENCIAL PONDERADO (EMA):
  // Atenúa el rizo de presión y caudal producido por los 3 rodillos del cabezal peristáltico.
  // Constante de tiempo aproximada: tau ~ 2 segundos.
  if (n > 0 || _f > 0.05f) {
    _q = 0.4f * q + 0.6f * _q;
  } else {
    _q = 0.0f;
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
