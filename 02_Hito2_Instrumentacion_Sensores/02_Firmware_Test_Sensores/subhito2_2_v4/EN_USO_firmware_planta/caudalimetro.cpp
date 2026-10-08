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
  // Captura atómica de variables acumuladas por la ISR en la ventana de 1 segundo
  portENTER_CRITICAL(&_mux);
  uint32_t n       = _pulsos;
  _pulsos          = 0;
  uint32_t t_prim  = _t_primero;
  uint32_t t_ult   = _t_ultimo;
  uint32_t dt_min  = _dt_min_us;
  uint32_t dt_max  = _dt_max_us;
  _dt_min_us       = 0xFFFFFFFF;
  _dt_max_us       = 0;
  portEXIT_CRITICAL(&_mux);

  _pulsosBrutos = n; // Almacenado para telemetría y prueba de permeado seco

  // 1. CÁLCULO DE FRECUENCIA Y FILTRADO DE COHERENCIA TEMPORAL (DICTAMEN AUDITORÍA RONDA 5):
  // Una turbina hidráulica arrastrada por líquido posee inercia mecánica y emite pulsos
  // distribuidos homogéneamente en el tiempo (relación dt_max / dt_min < 3.5).
  // Una perturbación electromagnética (EMI) del motor NEMA 34 genera ráfagas concentradas
  // (varios pulsos en pocos milisegundos y el resto del segundo vacío).
  bool pulsosCoherentes = false;
  float f_calculada = 0.0f;

  if (n >= 2 && (t_ult > t_prim)) {
    uint32_t ventanaPulsos_us = t_ult - t_prim;

    // Validación de coherencia de rotación si n >= 3:
    // Rechaza ráfagas espurias donde los pulsos ocurrieron apretados seguidos de un largo silencio
    bool dispersionPeriodoOk = (n < 4) || (dt_min > 0 && ((float)dt_max / (float)dt_min <= 4.0f));

    // Verificación de ocupación de ventana: para n pulsos a baja frecuencia (< 60 Hz),
    // los pulsos deben estar distribuidos en una fracción razonable de la ventana (> 50 ms).
    bool ventanaTemporalOk = (ventanaPulsos_us >= 30000UL) || (n <= 3);

    if (dispersionPeriodoOk && ventanaTemporalOk) {
      f_calculada = ((float)(n - 1) * 1000000.0f) / (float)ventanaPulsos_us;
      pulsosCoherentes = true;
    } else {
      Serial.printf("[%s] Ráfaga EMI descartada por dispersión: n=%lu, dt_min=%lu us, dt_max=%lu us, span=%lu us\n",
                    _nombre, (unsigned long)n, (unsigned long)dt_min, (unsigned long)dt_max, (unsigned long)ventanaPulsos_us);
    }
  }

  _f = pulsosCoherentes ? f_calculada : 0.0f;

  // 2. DEAD-BAND METROLÓGICO (Fricción estática de eje cerámico YF-S401):
  // La turbina no gira de forma continua y estable por debajo de ~2.5 Hz (< 20-30 mL/min).
  // Toda frecuencia < 2.5 Hz es truncada a cero absoluto para evitar acumulación de sesgo.
  if (_f < 2.5f) {
    _f = 0.0f;
  }

  // 3. CÁLCULO DE CAUDAL INSTANTÁNEO EN mL/min:
  // Q [mL/min] = (F [Hz] * 1000) / K
  float q = (_f * 1000.0f) / _k;

  // Umbral de caudal mínimo medible del sensor (límite de cuantificación YF-S401 ~30 mL/min)
  if (q < 30.0f) {
    q = 0.0f;
    _f = 0.0f;
  }

  // Filtro de plausibilidad física (corte de picos transitorios)
  if (q > Q_MAX_FISICO_MLMIN) {
    q = 0.0f;
    _f = 0.0f;
    Serial.printf("[%s] Ruido EMI descartado: %lu pulsos espurios\n", _nombre, (unsigned long)n);
  } else if (_f > 0.0f && q > 0.0f) {
    // 4. INTEGRACIÓN DE VOLUMEN TOTALIZADO EN LITROS:
    // Solo se acumula volumen si hay flujo real continuo y validado
    _vol += (float)n / (_k * 60.0f);
  }

  // 5. FILTRO EXPONENCIAL PONDERADO (EMA) SINTONIZADO PARA FLUJO PERISTÁLTICO:
  // Suaviza la pulsación rodillo a rodillo del cabezal peristáltico (tau ≈ 4.5 s con dt=1s, alfa=0.20)
  if (q > 0.0f) {
    if (_q == 0.0f) {
      _q = q; // Respuesta ágil desde reposo
    } else {
      _q = 0.20f * q + 0.80f * _q; // Filtrado estable
    }
  } else {
    // Decaimiento rápido a cero al detenerse el flujo
    _q = 0.50f * _q;
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
  // Diferencia sin signo uint32_t: inmune a desbordamiento de micros() cada 71.58 min
  uint32_t dt = t - c->_t_ultimo;

  // Blanking anti-rebote: descarta transitorios mecánicos y picos rápidos (< 1500 us)
  if (dt >= FILTRO_RUIDO_US) {
    if (c->_pulsos == 0) {
      c->_t_primero = t;
    } else {
      if (dt < c->_dt_min_us) c->_dt_min_us = dt;
      if (dt > c->_dt_max_us) c->_dt_max_us = dt;
    }
    c->_t_ultimo = t;
    c->_pulsos++;
  }
  portEXIT_CRITICAL_ISR(&c->_mux);
}
