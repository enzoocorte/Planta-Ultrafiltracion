#include "caudalimetro.h"
#include <math.h>
#include "driver/gpio.h"
#include "esp_timer.h"

// ==============================================================================
// IMPLEMENTACIÓN DE CAUDALÍMETRO (V5.1 - Auditoría Metrológica GPT Astra)
// ==============================================================================

Caudalimetro::Caudalimetro(const ConfigSensor& config, const char* nombre, bool esAlimentacion)
  : _cfg(config), _nombre(nombre), _esAlimentacion(esAlimentacion) {
}

void Caudalimetro::reiniciarEstimadorLocked() {
  _altoConocido           = false;
  _candidato              = false;
  _hayAnterior            = false;
  _bajada_us              = 0;
  _bajadaAnterior_us      = 0;
  _ultimoAceptado_us      = -1;
  _ultimoPeriodo_us       = 0;
  _periodosVentana        = 0;
  _sumaPeriodosVentana_us = 0;
}

bool Caudalimetro::setK(double nuevoK) {
  if (!std::isfinite(nuevoK) || nuevoK < 0.1) return false;
  _cfg.k_Hz_por_Lmin = nuevoK;
  _cfg.calibracionDocumentada = false;
  return true;
}

bool Caudalimetro::declararCalibrado(bool valido) {
  if (_iniciado) return false;
  _cfg.calibracionDocumentada = valido;
  return true;
}

void Caudalimetro::resetVolumen() {
  _volumenEstimadoAcumulado_L = 0.0;
}

bool Caudalimetro::begin() {
  if (_iniciado) return false;
  if (!GPIO_IS_VALID_GPIO((gpio_num_t)_cfg.pin) ||
      !std::isfinite(_cfg.k_Hz_por_Lmin) || _cfg.k_Hz_por_Lmin <= 0.0 ||
      _cfg.lowMin_us == 0 || _cfg.highMin_us == 0 ||
      _cfg.periodoMin_us == 0 || _cfg.timeout_us < _cfg.periodoMin_us ||
      !std::isfinite(_cfg.qMaxOperativo_mLmin) || _cfg.qMaxOperativo_mLmin <= 0.0) {
    return false;
  }

  if (_cfg.calibracionDocumentada &&
      (!std::isfinite(_cfg.qMinCal_mLmin) || !std::isfinite(_cfg.qMaxCal_mLmin) ||
       _cfg.qMinCal_mLmin < 0.0 || _cfg.qMaxCal_mLmin < _cfg.qMinCal_mLmin)) {
    return false;
  }

  pinMode(_cfg.pin, INPUT); // Utiliza el pull-up externo existente de 4.7k a 3.3V

  portENTER_CRITICAL(&_mux);
  _nivelAlto = (digitalRead(_cfg.pin) == HIGH);
  _inicioNivel_us = esp_timer_get_time();
  reiniciarEstimadorLocked();
  portEXIT_CRITICAL(&_mux);

  attachInterruptArg(digitalPinToInterrupt(_cfg.pin), isrPuente, this, CHANGE);
  _iniciado = true;
  return true;
}

void ARDUINO_ISR_ATTR Caudalimetro::isrPuente(void* arg) {
  static_cast<Caudalimetro*>(arg)->isr();
}

void ARDUINO_ISR_ATTR Caudalimetro::isr() {
  portENTER_CRITICAL_ISR(&_mux);
  const int64_t ahora = esp_timer_get_time();
  const bool alto = (digitalRead(_cfg.pin) == HIGH);

  ++_flancos;
  if (_seco) _alarmaSeco = true;

  if (alto == _nivelAlto) {
    // Rebote o perturbación diferida sin cambio de nivel lógico real
    ++_rechazados;
    reiniciarEstimadorLocked();
    _inicioNivel_us = ahora;
    portEXIT_CRITICAL_ISR(&_mux);
    return;
  }

  const int64_t duracion = ahora - _inicioNivel_us;

  if (!alto) {
    // Flanco descendente: termina HIGH, comienza LOW
    _candidato = _altoConocido && (duracion >= (int64_t)_cfg.highMin_us);
    _bajada_us = ahora;
    if (!_candidato) ++_rechazados;
  } else {
    // Flanco ascendente: termina LOW, comienza HIGH
    if (_candidato && (duracion >= (int64_t)_cfg.lowMin_us)) {
      const int64_t periodo = _bajada_us - _bajadaAnterior_us;
      // Continuidad temporal que no se interrumpe entre llamadas de la tarea
      if (!_hayAnterior || (periodo >= (int64_t)_cfg.periodoMin_us)) {
        ++_aceptados;
        if (!_seco) ++_pulsosMedicion;

        if (_hayAnterior && (periodo < (int64_t)_cfg.timeout_us)) {
          _ultimoPeriodo_us = periodo;
          ++_periodosVentana;
          _sumaPeriodosVentana_us += (uint64_t)periodo;
        } else {
          _ultimoPeriodo_us = 0;
          _periodosVentana = 0;
          _sumaPeriodosVentana_us = 0;
        }
        _bajadaAnterior_us = _bajada_us;
        _ultimoAceptado_us = ahora;
        _hayAnterior = true;
      } else {
        ++_rechazados; // Período menor al mínimo admisible físicamente
      }
    } else if (_candidato) {
      ++_rechazados; // Pulso demasiado estrecho (glitch EMI)
    }
    _candidato = false;
    _altoConocido = true;
  }

  _nivelAlto = alto;
  _inicioNivel_us = ahora;
  portEXIT_CRITICAL_ISR(&_mux);
}

void Caudalimetro::setModoSeco(bool activo) {
  portENTER_CRITICAL(&_mux);
  if (_seco != activo) {
    _seco = activo;
    ++_epoca; // Cada cambio de modo inicia una nueva época metrológica
    reiniciarEstimadorLocked();
    _nivelAlto = (digitalRead(_cfg.pin) == HIGH);
    _inicioNivel_us = esp_timer_get_time();
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
  m.frecuencia_Hz = NAN;
  m.q_mLmin = NAN;
  m.volumenEstimado_L = NAN;

  if (!_iniciado) {
    m.t_us = esp_timer_get_time();
    _ultimaMuestra = m;
    return m;
  }

  bool seco, alarma, hayAnterior;
  int64_t periodo;
  uint64_t periodos, suma;

  portENTER_CRITICAL(&_mux);
  m.t_us               = esp_timer_get_time();
  m.ultimoAceptado_us  = _ultimoAceptado_us;
  m.flancos            = _flancos;
  m.aceptados          = _aceptados;
  m.pulsosMedicion     = _pulsosMedicion;
  m.rechazados         = _rechazados;
  m.epoca              = _epoca;
  seco                 = _seco;
  alarma               = _alarmaSeco;
  hayAnterior          = _hayAnterior;
  periodo              = _ultimoPeriodo_us;
  periodos             = _periodosVentana;
  suma                 = _sumaPeriodosVentana_us;
  _periodosVentana     = 0;
  _sumaPeriodosVentana_us = 0;
  portEXIT_CRITICAL(&_mux);

  // Diagnósticos de la última ventana
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
  const bool sinSenal = !hayAnterior || m.edadPulso_us < 0 || m.edadPulso_us > (int64_t)_cfg.timeout_us;
  if (sinSenal) {
    m.calidad |= SIN_SENAL;
  }

  // En modo seco no se genera estimación de caudal
  // Sección 4.2 de Astra: Solo publicar estimación recíproca si hay períodos NUEVOS en la ventana
  const bool hayPeriodosNuevos = (periodos > 0 && suma > 0);

  if (!seco && !sinSenal && hayPeriodosNuevos) {
    m.calidad |= PERIODO_DISPONIBLE;
    m.frecuencia_Hz = 1.0e6 * static_cast<double>(periodos) / static_cast<double>(suma);
    m.q_mLmin = 1000.0 * m.frecuencia_Hz / m.k;

    if (m.q_mLmin > _cfg.qMaxOperativo_mLmin) {
      m.calidad |= FUERA_RANGO_OPERATIVO;
    }

    if (_cfg.calibracionDocumentada) {
      if (m.q_mLmin < _cfg.qMinCal_mLmin || m.q_mLmin > _cfg.qMaxCal_mLmin) {
        m.calidad |= FUERA_RANGO_CAL;
      } else {
        m.calidad |= MEDICION_EN_RANGO_CAL;
      }
    }
  }

  // Volumen totalizado exacto a partir del totalizador de pulsos acumulados (sin errores de redondeo)
  m.volumenEstimado_L = (double)m.pulsosMedicion / (60.0 * m.k);
  _volumenEstimadoAcumulado_L = m.volumenEstimado_L;

  const bool nuevos = (m.pulsosMedicion != _anteriorMedicion);
  _anteriorMedicion = m.pulsosMedicion;
  if (!seco && !bombaEmpuja && (nuevos || std::isfinite(m.q_mLmin))) {
    m.calidad |= SIN_IMPULSION;
  }

  // Suavizado EMA solo para indicación visual en SCADA (si no hay flujo nuevo, cae a 0)
  if (std::isfinite(m.q_mLmin) && m.q_mLmin > 0.0) {
    _qSuavizadoDisplay = (_qSuavizadoDisplay == 0.0f) ? (float)m.q_mLmin : (0.25f * (float)m.q_mLmin + 0.75f * _qSuavizadoDisplay);
  } else {
    _qSuavizadoDisplay = 0.0f;
  }

  // Sección 4.3 de Astra: Detección rigurosa de tiempo real transcurrido para fallo de alimentación
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
