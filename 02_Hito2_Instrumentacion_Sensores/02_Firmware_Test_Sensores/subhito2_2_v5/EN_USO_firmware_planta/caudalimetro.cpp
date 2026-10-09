#include "caudalimetro.h"
#include <math.h>
#include "esp_timer.h"

// ==============================================================================
// IMPLEMENTACIÓN DE CAUDALÍMETRO (V5.1 - Auditoría GPT Astra / Certificado)
// ==============================================================================

Caudalimetro::Caudalimetro(uint8_t pin, float k, const char* nombre, bool esAlimentacion)
  : _pin(pin), _k(k), _nombre(nombre), _esAlimentacion(esAlimentacion) {
  if (_esAlimentacion) {
    // Alimentación: Flujos de 200 a 1400 mL/min (Frecuencia hasta ~270 Hz)
    // anchoMin: 200 us | periodoMin: 1000 us (1000 Hz) | periodoMax: 1000000 us (1 s)
    _p = {200, 200, 1000, 1000000, 1400.0f};
  } else {
    // Permeado: Flujos de 10 a 200 mL/min (Frecuencias lentas)
    // anchoMin: 600 us | periodoMin: 2500 us (400 Hz) | periodoMax: 5000000 us (5 s timeout)
    _p = {600, 600, 2500, 5000000, 200.0f};
  }
}

bool Caudalimetro::setK(float nuevoK) {
  if (_iniciado || !isfinite(nuevoK) || nuevoK < 0.1f) return false;
  _k = nuevoK;
  _calibrado = false;
  return true;
}

bool Caudalimetro::declararCalibrado(bool valido) {
  if (_iniciado) return false;
  _calibrado = valido;
  return true;
}

bool Caudalimetro::begin() {
  if (_iniciado || _pin > 39 || !isfinite(_k) || _k < 0.1f) return false;

  pinMode(_pin, INPUT); // Se utiliza el pull-up externo existente de 4.7k a 3.3V

  const bool alto = digitalRead(_pin);
  const int64_t ahora = esp_timer_get_time();

  portENTER_CRITICAL(&_mux);
  _nivelAlto = alto;
  _inicioNivel_us = ahora;
  _altoConocido = false;
  _candidato = false;
  _hayUltimo = false;
  _ultimoPulso_us = 0;
  _ultimoPeriodo_us = 0;
  portEXIT_CRITICAL(&_mux);

  attachInterruptArg(digitalPinToInterrupt(_pin), isrPuente, this, CHANGE);
  _iniciado = true;
  return true;
}

void Caudalimetro::setModoSeco(bool activo) {
  portENTER_CRITICAL(&_mux);
  _seco_ISR = activo;
  _candidato = false;
  _altoConocido = false;
  portEXIT_CRITICAL(&_mux);
  _modoSeco = activo;
}

void Caudalimetro::limpiarAlarmaSeco() {
  portENTER_CRITICAL(&_mux);
  _alarmaSeco_ISR = false;
  portEXIT_CRITICAL(&_mux);
  _ruidoEnSeco = false;
}

void ARDUINO_ISR_ATTR Caudalimetro::isrPuente(void* arg) {
  static_cast<Caudalimetro*>(arg)->isrInterna();
}

void ARDUINO_ISR_ATTR Caudalimetro::isrInterna() {
  const int64_t ahora = esp_timer_get_time();
  const bool alto = digitalRead(_pin);

  portENTER_CRITICAL_ISR(&_mux);
  ++_flancos_ISR;
  if (_seco_ISR) _alarmaSeco_ISR = true;

  // Si no hubo cambio real de nivel lógico (rebote o latencia)
  if (alto == _nivelAlto) {
    ++_rechazos_ISR;
    portEXIT_CRITICAL_ISR(&_mux);
    return;
  }

  const int64_t duracion = ahora - _inicioNivel_us;

  if (!alto) {
    // Flanco descendente: Termina nivel HIGH y comienza nivel LOW
    _candidato = _altoConocido && (duracion >= (int64_t)_p.anchoMinHigh_us);
    _bajada_us = ahora;
    if (!_candidato) ++_rechazos_ISR;
  } else {
    // Flanco ascendente: Termina nivel LOW y comienza nivel HIGH
    if (_candidato && (duracion >= (int64_t)_p.anchoMinLow_us)) {
      const int64_t periodo = _bajada_us - _ultimoPulso_us;
      // CONTINUIDAD METROLÓGICA (Hallazgo GPT Astra):
      // El control de período mínimo cruza las ventanas de actualizar()
      if (!_hayUltimo || (periodo >= (int64_t)_p.periodoMin_us)) {
        ++_aceptados_ISR;
        if (!_seco_ISR) ++_liquido_ISR;

        if (_hayUltimo && (periodo <= (int64_t)_p.periodoMax_us)) {
          _ultimoPeriodo_us = periodo;
          _sumaPeriodos_ISR_us += periodo;
          ++_periodos_ISR;
        } else {
          _ultimoPeriodo_us = 0;
        }
        _ultimoPulso_us = _bajada_us;
        _hayUltimo = true;
      } else {
        ++_rechazos_ISR; // Período físicamente menor al admisible
      }
    } else if (_candidato) {
      ++_rechazos_ISR; // Nivel LOW demasiado corto (glitch EMI)
    }
    _candidato = false;
    _altoConocido = true;
  }

  _nivelAlto = alto;
  _inicioNivel_us = ahora;
  portEXIT_CRITICAL_ISR(&_mux);
}

void Caudalimetro::actualizar(float dt_s, bool bombaEmpuja, float qBombaTeorico_mLmin) {
  if (!_iniciado || !isfinite(dt_s) || dt_s <= 0.0f) return;

  uint32_t flancos, aceptados, liquido, rechazados, periodos;
  int64_t sumaPeriodos, ultimoPeriodo, ultimoPulso, ahora;
  bool hayUltimo, alarma;

  portENTER_CRITICAL(&_mux);
  flancos       = _flancos_ISR;        _flancos_ISR = 0;
  aceptados     = _aceptados_ISR;      _aceptados_ISR = 0;
  liquido       = _liquido_ISR;        _liquido_ISR = 0;
  rechazados    = _rechazos_ISR;       _rechazos_ISR = 0;
  periodos      = _periodos_ISR;       _periodos_ISR = 0;
  sumaPeriodos  = _sumaPeriodos_ISR_us; _sumaPeriodos_ISR_us = 0;
  ultimoPeriodo = _ultimoPeriodo_us;
  ultimoPulso   = _ultimoPulso_us;
  hayUltimo     = _hayUltimo;
  alarma        = _alarmaSeco_ISR;
  ahora         = esp_timer_get_time();
  portEXIT_CRITICAL(&_mux);

  _flancosVentana   = flancos;
  _validosVentana   = aceptados;
  _rechazosVentana  = rechazados;
  _aceptadosAcumulados += aceptados;
  _liquidoAcumulados   += liquido;
  _rechazosAcumulados  += rechazados;
  _ruidoEnSeco = alarma;

  // 1. Integración íntegra de volumen totalizado (desacoplada del display cosmético)
  _vol += (double)liquido / (60.0 * (double)_k);

  // 2. Detección de presencia de señal y período disponible
  _sinSenal = !hayUltimo || ((ahora - ultimoPulso) > (int64_t)_p.periodoMax_us);
  _periodoDisponible = !_sinSenal && (ultimoPeriodo > 0);

  float qInst = 0.0f;

  if (_sinSenal) {
    _f = 0.0f;
    qInst = 0.0f;
  } else if (!_periodoDisponible) {
    _f = 0.0f;
    qInst = 0.0f;
  } else if (periodos > 0 && sumaPeriodos > 0) {
    // Estimación recíproca sobre períodos completos genuinos
    _f = (float)(1.0e6 * (double)periodos / (double)sumaPeriodos);
    qInst = (_f * 1000.0f) / _k;
  } else if (ultimoPeriodo > 0) {
    // Retención entre pulsos en régimen lento
    _f = (float)(1.0e6 / (double)ultimoPeriodo);
    qInst = (_f * 1000.0f) / _k;
  }

  // 3. Filtrado EMA adaptativo para visualización
  if (qInst > 0.0f) {
    _q = (_q == 0.0f) ? qInst : (0.25f * qInst + 0.75f * _q);
  } else {
    _q = 0.0f;
  }

  // 4. Banderas de diagnóstico metrológico
  _fueraRango = isfinite(_q) && (_q > _p.qMax_mLmin);
  _sinImpulsion = !_esAlimentacion && (!bombaEmpuja || (isfinite(qBombaTeorico_mLmin) && qBombaTeorico_mLmin < 20.0f)) && ((aceptados > 0) || (isfinite(_q) && _q > 0.0f));

  // Diagnóstico asimétrico de pérdida de señal en alimentación
  if (aceptados > 0 || !bombaEmpuja || !_esAlimentacion) {
    _sinPulso_s = 0.0f;
    _fallo = false;
  } else {
    _sinPulso_s += dt_s;
    _fallo = (_sinPulso_s >= 5.0f);
  }
}
