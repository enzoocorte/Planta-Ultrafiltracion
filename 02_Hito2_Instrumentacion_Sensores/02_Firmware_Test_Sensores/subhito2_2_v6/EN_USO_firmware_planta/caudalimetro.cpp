#include "caudalimetro.h"
#include <math.h>
#include "driver/gpio.h"
#include "esp_timer.h"

// ==============================================================================
// IMPLEMENTACIÓN DE CAUDALÍMETRO — VERSIÓN V6.4 (Estabilidad Total + Anti-Picos)
// - Blanking de 3000 us en ISR (rechaza de raíz cualquier chispazo > 333 Hz como
//   el de 373 Hz, dejando pasar el 100% del rango de 0 a 100 RPM = 0 a 230 Hz).
// - Filtro de Mediana de 3 puntos (mediana3) + Suavizado Exponencial (EMA) para
//   eliminar variaciones bruscas de 1 segundo sin usar capacitores que ahoguen
//   la señal a altas RPM.
// ==============================================================================

Caudalimetro::Caudalimetro(const ConfigSensor& config, const char* nombre, bool esAlimentacion)
  : _cfg(config), _nombre(nombre), _esAlimentacion(esAlimentacion) {
}

float Caudalimetro::mediana3(float a, float b, float c) {
  if ((a >= b && a <= c) || (a <= b && a >= c)) return a;
  if ((b >= a && b <= c) || (b <= a && b >= c)) return b;
  return c;
}

void ARDUINO_ISR_ATTR Caudalimetro::reiniciarEstimadorLocked() {
  _hayAnterior       = false;
  _primeroVentana_us = 0;
  _ultimoVentana_us  = 0;
  _bajadaAnterior_us = 0;
  _ultimoAceptado_us = -1;
  _ultimoPeriodo_us  = 0;
  _pulsosVentana     = 0;
}

bool Caudalimetro::setK(double nuevoK) {
  if (!std::isfinite(nuevoK) || nuevoK < 10.0 || nuevoK > 1000.0) {
    return false;
  }

  portENTER_CRITICAL(&_mux);
  _cfg.k_Hz_por_Lmin = nuevoK;
  _cfg.calibracionDocumentada = false;
  ++_epoca;

  _pulsosOffset = _pulsosMedicion;
  _anteriorMedicion = _pulsosMedicion;

  reiniciarEstimadorLocked();
  portEXIT_CRITICAL(&_mux);

  _volumenEstimadoAcumulado_L = 0.0;
  _qHist[0] = _qHist[1] = _qHist[2] = 0.0f;
  _qHistCount = 0;
  _cerosConsecutivos = 0;
  _qSuavizadoDisplay = 0.0f;
  _inicioSinPulso_us = -1;
  _falloAlimentacion = false;

  _ultimaMuestra = Muestra{};
  _ultimaMuestra.k = nuevoK;
  _ultimaMuestra.frecuencia_Hz = 0.0;
  _ultimaMuestra.q_mLmin = 0.0;
  _ultimaMuestra.volumenEstimado_L = 0.0;

  return true;
}

bool Caudalimetro::setQMinDetectable(double nuevoQMin_mLmin) {
  if (!std::isfinite(nuevoQMin_mLmin) || nuevoQMin_mLmin < 0.0 ||
      nuevoQMin_mLmin >= _cfg.qMaxOperativo_mLmin) {
    return false;
  }
  _cfg.qMinDetectable_mLmin = nuevoQMin_mLmin;
  return true;
}

bool Caudalimetro::declararCalibrado(bool valido) {
  if (_iniciado) return false;
  _cfg.calibracionDocumentada = valido;
  return true;
}

void Caudalimetro::resetVolumen() {
  portENTER_CRITICAL(&_mux);
  _pulsosOffset = _pulsosMedicion;
  portEXIT_CRITICAL(&_mux);
  _volumenEstimadoAcumulado_L = 0.0;
}

void Caudalimetro::setBloqueado(bool bloqueado) {
  portENTER_CRITICAL(&_mux);
  if (_bloqueado != bloqueado) {
    _bloqueado = bloqueado;
    reiniciarEstimadorLocked();
  }
  portEXIT_CRITICAL(&_mux);
  if (bloqueado) {
    _qHist[0] = _qHist[1] = _qHist[2] = 0.0f;
    _qHistCount = 0;
    _cerosConsecutivos = 0;
    _qSuavizadoDisplay = 0.0f;
  }
}

bool Caudalimetro::begin() {
  if (_iniciado) return false;
  if (!GPIO_IS_VALID_GPIO((gpio_num_t)_cfg.pin) ||
      !std::isfinite(_cfg.k_Hz_por_Lmin) || _cfg.k_Hz_por_Lmin <= 0.0) {
    return false;
  }

  pinMode(_cfg.pin, INPUT_PULLUP);

  portENTER_CRITICAL(&_mux);
  reiniciarEstimadorLocked();
  portEXIT_CRITICAL(&_mux);

  _tCapturaPrev_us = esp_timer_get_time();
  attachInterruptArg(digitalPinToInterrupt(_cfg.pin), isrPuente, this, FALLING);
  _iniciado = true;
  return true;
}

void ARDUINO_ISR_ATTR Caudalimetro::isrPuente(void* arg) {
  static_cast<Caudalimetro*>(arg)->isr();
}

void ARDUINO_ISR_ATTR Caudalimetro::isr() {
  portENTER_CRITICAL_ISR(&_mux);
  const int64_t ahora = esp_timer_get_time();

  if (_bloqueado) {
    portEXIT_CRITICAL_ISR(&_mux);
    return;
  }

  ++_flancos;
  if (_seco) _alarmaSeco = true;

  const int64_t dt = _hayAnterior ? (ahora - _bajadaAnterior_us) : 0;

  // Blanking digital diferenciado por canal (Anti-EMI y respeto a la cinemática de pulsación):
  // - Alimentación (GPIO 14): 1200 us (f_max = 833 Hz). Deja pasar el 100% del régimen pulsátil
  //   de la bomba peristáltica (picos instantáneos de hasta ~410 Hz a 90-100 RPM) bloqueando
  //   cualquier transitorio EMI de alta frecuencia (> 1 kHz).
  // - Permeado (GPIO 27): 3500 us (f_max = 285 Hz). Bloquea rebotes mecánicos y falsos contactos
  //   como el pico espurio de 373 Hz, admitiendo caudales transmembrana de hasta 800 mL/min.
  const int64_t blanking_us = _esAlimentacion ? 1200LL : 3500LL;

  if (!_hayAnterior || dt >= blanking_us) {
    if (_pulsosVentana == 0) {
      _primeroVentana_us = ahora;
    }
    _ultimoVentana_us = ahora;
    ++_pulsosVentana;

    if (_hayAnterior && dt < 5000000LL) {
      _ultimoPeriodo_us = dt;
    } else {
      _ultimoPeriodo_us = 0;
    }

    _bajadaAnterior_us = ahora;
    _ultimoAceptado_us = ahora;
    _hayAnterior = true;

    ++_aceptados;
    if (!_seco) ++_pulsosMedicion;
  } else {
    ++_rechazados;
  }

  portEXIT_CRITICAL_ISR(&_mux);
}

void Caudalimetro::setModoSeco(bool activo) {
  portENTER_CRITICAL(&_mux);
  if (_seco != activo) {
    _seco = activo;
    ++_epoca;
    reiniciarEstimadorLocked();
  }
  portEXIT_CRITICAL(&_mux);
}

void Caudalimetro::limpiarAlarmaSeco() {
  portENTER_CRITICAL(&_mux);
  _alarmaSeco = false;
  portEXIT_CRITICAL(&_mux);
}

Caudalimetro::Muestra Caudalimetro::capturar(bool bombaEmpuja) {
  Muestra m;
  m.k = _cfg.k_Hz_por_Lmin;
  m.frecuencia_Hz = 0.0;
  m.q_mLmin = 0.0;
  m.volumenEstimado_L = 0.0;

  if (!_iniciado) {
    m.t_us = esp_timer_get_time();
    _ultimaMuestra = m;
    return m;
  }

  bool seco, bloqueado, alarma, hayAnterior;
  int64_t primero_us, ultimo_us, ultimoPeriodo;
  uint32_t nVentana;

  portENTER_CRITICAL(&_mux);
  m.t_us              = esp_timer_get_time();
  m.ultimoAceptado_us = _ultimoAceptado_us;
  m.flancos           = _flancos;
  m.aceptados         = _aceptados;
  m.pulsosMedicion    = _pulsosMedicion;
  m.rechazados        = _rechazados;
  m.epoca             = _epoca;
  seco                = _seco;
  bloqueado           = _bloqueado;
  alarma              = _alarmaSeco;
  hayAnterior         = _hayAnterior;
  primero_us          = _primeroVentana_us;
  ultimo_us           = _ultimoVentana_us;
  ultimoPeriodo       = _ultimoPeriodo_us;
  nVentana            = _pulsosVentana;
  _pulsosVentana      = 0;
  portEXIT_CRITICAL(&_mux);

  int64_t dtVentana_us = m.t_us - _tCapturaPrev_us;
  _tCapturaPrev_us = m.t_us;
  if (dtVentana_us < 500000LL || dtVentana_us > 2500000LL) {
    dtVentana_us = 1000000LL;
  }

  _flancosVentana  = (uint32_t)(m.flancos - _flancosPrevios);       _flancosPrevios = m.flancos;
  _validosVentana  = (uint32_t)(m.aceptados - _aceptadosPrevios);   _aceptadosPrevios = m.aceptados;
  _rechazosVentana = (uint32_t)(m.rechazados - _rechazosPrevios);   _rechazosPrevios = m.rechazados;

  m.calidad = INICIADO;
  if (_cfg.calibracionDocumentada) m.calidad |= CAL_DOCUMENTADA;
  if (seco)   m.calidad |= SECO;
  if (alarma) m.calidad |= RUIDO_SECO;

  if (hayAnterior && m.ultimoAceptado_us > 0) {
    m.edadPulso_us = m.t_us - m.ultimoAceptado_us;
  }

  const int64_t timeout_us = (int64_t)_cfg.timeout_us;
  const bool sinSenal = bloqueado || !hayAnterior ||
                        m.edadPulso_us < 0 || m.edadPulso_us > timeout_us;
  if (sinSenal) {
    m.calidad |= SIN_SENAL;
  }

  if (!seco && !bloqueado && !sinSenal) {
    double f = 0.0;
    const double fConteo = ((double)nVentana * 1.0e6) / (double)dtVentana_us;
    const int64_t span_us = ultimo_us - primero_us;

    if (nVentana >= 2 && span_us >= 350000LL) {
      f = ((double)(nVentana - 1) * 1.0e6) / (double)span_us;
      if (f > fConteo * 1.35 + 1.5 || f < fConteo * 0.65 - 1.5) {
        f = fConteo;
      }
    } else if (nVentana >= 1) {
      f = fConteo;
    } else if (nVentana == 0 && m.edadPulso_us > 0 && m.edadPulso_us < 2500000LL) {
      double fMax = 1.0e6 / (double)m.edadPulso_us;
      double fPrev = std::isfinite(_ultimaMuestra.frecuencia_Hz) ? _ultimaMuestra.frecuencia_Hz : 0.0;
      f = (fPrev < fMax) ? fPrev : fMax;
      if (f < 0.25) f = 0.0;
    }

    const double fMaxPermitida = _esAlimentacion ? 600.0 : 250.0;
    if (f > 0.05 && f <= fMaxPermitida) {
      m.calidad |= PERIODO_DISPONIBLE;
      m.frecuencia_Hz = f;
      m.q_mLmin = (f * 1000.0) / m.k;
      if (m.q_mLmin > _cfg.qMaxOperativo_mLmin) {
        m.q_mLmin = _cfg.qMaxOperativo_mLmin;
      }
    }
  }

  const uint64_t pulsosNetos = (m.pulsosMedicion > _pulsosOffset) ? (m.pulsosMedicion - _pulsosOffset) : 0;
  m.volumenEstimado_L = (double)pulsosNetos / (60.0 * m.k);
  _volumenEstimadoAcumulado_L = m.volumenEstimado_L;

  const bool nuevos = (m.pulsosMedicion != _anteriorMedicion);
  _anteriorMedicion = m.pulsosMedicion;
  if (!seco && !bombaEmpuja && nuevos) {
    m.calidad |= SIN_IMPULSION;
  }

  // Filtro Mediana de 3 ventanas + Suavizado Exponencial EMA
  if (!sinSenal && (nVentana > 0 || m.frecuencia_Hz > 0.05)) {
    float qInst = (float)m.q_mLmin;
    _qHist[0] = _qHist[1];
    _qHist[1] = _qHist[2];
    _qHist[2] = qInst;
    if (_qHistCount < 3) _qHistCount++;

    float qMed = (_qHistCount >= 3) ? mediana3(_qHist[0], _qHist[1], _qHist[2]) : qInst;
    _qSuavizadoDisplay = (_qSuavizadoDisplay == 0.0f)
                           ? qMed
                           : (0.35f * qMed + 0.65f * _qSuavizadoDisplay);
    _cerosConsecutivos = 0;
  } else {
    // Si la bomba sigue en marcha pero hubo una ventana aislada sin pulso, no borrar de golpe.
    // Solo si se confirman 2 ventanas consecutivas sin pulsos o si la bomba se detuvo, resetear a 0.
    _cerosConsecutivos++;
    if (_cerosConsecutivos >= 2 || !bombaEmpuja || bloqueado) {
      _qHist[0] = _qHist[1] = _qHist[2] = 0.0f;
      _qHistCount = 0;
      _qSuavizadoDisplay = 0.0f;
    } else {
      _qSuavizadoDisplay *= 0.5f; // Decaimiento suave
    }
  }

  if (!_esAlimentacion || seco || !bombaEmpuja) {
    _inicioSinPulso_us = -1;
    _falloAlimentacion = false;
  } else if (nuevos) {
    _inicioSinPulso_us = (m.ultimoAceptado_us >= 0) ? m.ultimoAceptado_us : m.t_us;
    _falloAlimentacion = false;
  } else {
    if (_inicioSinPulso_us < 0) {
      _inicioSinPulso_us = m.t_us;
    }
    _falloAlimentacion = (m.t_us - _inicioSinPulso_us >= 5000000LL);
  }

  _ultimaMuestra = m;
  return m;
}

float Caudalimetro::caudal_mLmin() const {
  return _qSuavizadoDisplay;
}

float Caudalimetro::caudal_Lmin() const {
  return _qSuavizadoDisplay / 1000.0f;
}

float Caudalimetro::frecuencia_Hz() const {
  return std::isfinite(_ultimaMuestra.frecuencia_Hz) ? (float)_ultimaMuestra.frecuencia_Hz : 0.0f;
}

double Caudalimetro::caudalMedio_mLmin(const Muestra& inicial, const Muestra& final) {
  if (!(inicial.calidad & INICIADO) || !(final.calidad & INICIADO) ||
      inicial.epoca != final.epoca ||
      (inicial.calidad & SECO) || (final.calidad & SECO) ||
      !std::isfinite(inicial.k) || inicial.k <= 0.0 || inicial.k != final.k ||
      final.t_us <= inicial.t_us ||
      final.pulsosMedicion < inicial.pulsosMedicion) {
    return NAN;
  }

  const uint64_t dn = final.pulsosMedicion - inicial.pulsosMedicion;
  const int64_t  dt = final.t_us - inicial.t_us;
  return ((double)dn * 1.0e9) / (inicial.k * (double)dt);
}
