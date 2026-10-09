#include "caudalimetro.h"

// ==============================================================================
// IMPLEMENTACIÓN DE CAUDALÍMETRO (V5 - Anti-EMI & Validación de Ancho de Pulso)
// ==============================================================================

Caudalimetro::Caudalimetro(uint8_t pin, float k, const char* nombre, bool esAlimentacion)
  : _pin(pin), _k(k), _nombre(nombre), _esAlimentacion(esAlimentacion) {
  if (_esAlimentacion) {
    // Alimentación: Flujos de 200 a 1400 mL/min (Frecuencia hasta ~270 Hz)
    // A 270 Hz, el semi-período es ~1850 us. Filtramos glitches < 200 us.
    _p.anchoMinLow_us  = 200;
    _p.anchoMinHigh_us = 200;
    _p.periodoMin_us   = 1000;    // Máx 1000 Hz admisible
    _p.periodoMax_us   = 1000000; // 1 s sin pulsos => detenido
    _p.qMin_mLmin      = 30.0f;   // Umbral de corte de cuantificación mecánica
  } else {
    // Permeado: Flujos de 10 a 150 mL/min (Frecuencia hasta ~70 Hz con K=687 o ~20 Hz con K=196)
    // A 70 Hz, el semi-período es ~7100 us. Filtramos glitches < 600 us.
    _p.anchoMinLow_us  = 600;
    _p.anchoMinHigh_us = 600;
    _p.periodoMin_us   = 2500;    // Máx 400 Hz admisible
    _p.periodoMax_us   = 1500000; // 1.5 s sin pulsos => detenido
    _p.qMin_mLmin      = 4.0f;    // Umbral adaptativo bajo para ultrafiltración (no trunca a 30 mL/min)
  }
}

void Caudalimetro::begin() {
  pinMode(_pin, INPUT_PULLUP);
  _tSubida = micros();
  _tBajada = _tSubida;
  _tUltimoValido = _tSubida;
  // Interrupción en CHANGE para monitorear transiciones de subida y bajada
  attachInterruptArg(digitalPinToInterrupt(_pin), isrPuente, this, CHANGE);
}

void IRAM_ATTR Caudalimetro::isrPuente(void* arg) {
  reinterpret_cast<Caudalimetro*>(arg)->isrInterna();
}

void IRAM_ATTR Caudalimetro::isrInterna() {
  uint32_t t = micros();
  bool nivelAlto = digitalRead(_pin);

  portENTER_CRITICAL_ISR(&_mux);
  _flancosTotal++;

  if (!nivelAlto) {
    // Flanco de bajada (HIGH -> LOW)
    // Para que sea un flanco válido de álabe de turbina, el nivel HIGH previo
    // debe haber durado al menos _p.anchoMinHigh_us (diferencia sin signo)
    uint32_t duracionHigh = t - _tSubida;
    if (duracionHigh >= _p.anchoMinHigh_us) {
      _bajadaCandidata = true;
      _tBajada = t;
    } else {
      _bajadaCandidata = false;
      _glitchesTotal++;
    }
  } else {
    // Flanco de subida (LOW -> HIGH)
    // El nivel LOW que finaliza debe haber durado al menos _p.anchoMinLow_us
    uint32_t duracionLow = t - _tBajada;
    if (_bajadaCandidata && duracionLow >= _p.anchoMinLow_us) {
      uint32_t periodo = _tBajada - _tUltimoValido;
      if (_validosTotal == 0 || periodo >= _p.periodoMin_us) {
        if (_validosTotal == 0) {
          _tPrimeroValido = _tBajada;
        }
        _tUltimoValido = _tBajada;
        _validosTotal++;
      } else {
        _glitchesTotal++; // Rechazado por período menor al físico admisible
      }
    } else {
      _glitchesTotal++; // Rechazado: pulso estrecho (glitch EMI o rebote de rampa RC)
    }
    _bajadaCandidata = false;
    _tSubida = t;
  }
  portEXIT_CRITICAL_ISR(&_mux);
}

void Caudalimetro::actualizar(float dt_s, bool bombaEmpuja, float qBombaTeorico_mLmin) {
  // Captura atómica de eventos acumulados en la ventana de 1 segundo
  portENTER_CRITICAL(&_mux);
  uint32_t flancos  = _flancosTotal;  _flancosTotal  = 0;
  uint32_t nValidos = _validosTotal;  _validosTotal  = 0;
  uint32_t nGlitch  = _glitchesTotal; _glitchesTotal = 0;
  uint32_t tPrim    = _tPrimeroValido;
  uint32_t tUlt     = _tUltimoValido;
  portEXIT_CRITICAL(&_mux);

  _flancosVentana  = flancos;
  _validosVentana  = nValidos;
  _glitchesVentana = nGlitch;

  // En Modo Seco: si se detecta cualquier flanco o pulso, enclavar alarma
  if (_modoSeco && (flancos > 0 || nValidos > 0)) {
    _ruidoEnSeco = true;
  }

  // 1. Cálculo de frecuencia sobre pulsos físicamente validados
  if (nValidos >= 2 && (tUlt - tPrim) > 0) {
    _f = ((float)(nValidos - 1) * 1000000.0f) / (float)(tUlt - tPrim);
  } else if (nValidos == 1) {
    _f = 1.0f / dt_s;
  } else {
    _f = 0.0f;
  }

  // Si transcurrió más de periodoMax sin pulsos, flujo detenido
  uint32_t tAhora = micros();
  if ((tAhora - _tUltimoValido) > _p.periodoMax_us) {
    _f = 0.0f;
  }

  // 2. Cálculo de caudal instantáneo en mL/min
  float q = (_f * 1000.0f) / _k;

  // Deadband adaptativo por canal
  if (q < _p.qMin_mLmin || _f < 0.5f) {
    q = 0.0f;
    _f = 0.0f;
  }

  // Sanity Gate (Hallazgo GLM): En permeado, si la bomba está parada o caudal teórico < 20 mL/min,
  // físicamente no puede haber flujo transmembrana impulsado
  if (!_esAlimentacion && (qBombaTeorico_mLmin < 20.0f || !bombaEmpuja)) {
    q = 0.0f;
    _f = 0.0f;
  }

  // Plausibilidad física máxima
  if (q > Q_MAX_FISICO_MLMIN) {
    q = 0.0f;
    _f = 0.0f;
    Serial.printf("[%s] Ruido EMI descartado por plausibilidad: %lu flancos\n", _nombre, (unsigned long)flancos);
  } else if (_f > 0.0f && q > 0.0f && !_modoSeco) {
    // 3. Integración de volumen totalizado en litros
    _vol += (float)nValidos / (_k * 60.0f);
  }

  // 4. Filtrado EMA
  if (q > 0.0f) {
    if (_q == 0.0f) {
      _q = q;
    } else {
      _q = 0.25f * q + 0.75f * _q;
    }
  } else {
    // Corte inmediato a 0 (elimina cola lenta de 10s tras presionar STOP)
    _q = 0.0f;
  }

  // 5. Diagnóstico de pérdida de señal
  if (nValidos > 0 || _f > 0.05f) {
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
