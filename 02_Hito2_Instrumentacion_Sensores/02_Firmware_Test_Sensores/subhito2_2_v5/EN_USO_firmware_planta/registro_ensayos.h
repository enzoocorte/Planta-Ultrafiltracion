#pragma once
// ==============================================================================
// REGISTRO DE ENSAYOS MULTI-SESIÓN SIN SOBREESCRITURA (Auditoría GPT Astra)
// - Estructura en RAM de 600 registros y hasta 20 sesiones de ensayo
// - Diferenciación estricta de eventos: INICIO (t=0), PERIODICA (10s) y FIN
// - Marcas de calidad metrológica (flags de bit) y tiempos reales de adquisición
// - Almacena conteos acumulados de pulsos y eventos rechazados (glitches EMI)
// ==============================================================================

#include <Arduino.h>
#include <math.h>
#include <stdint.h>
#include "esp_timer.h"
#include "caudalimetro.h"

class RegistroEnsayos {
public:
  enum Tipo : uint8_t {
    INICIO = 0,
    PERIODICA = 1,
    FIN = 2
  };

  enum Calidad : uint16_t {
    CAL_A            = (1U << 0),
    CAL_P            = (1U << 1),
    PERIODO_A        = (1U << 2),
    PERIODO_P        = (1U << 3),
    SIN_SENAL_A      = (1U << 4),
    SIN_SENAL_P      = (1U << 5),
    FUERA_RANGO_A    = (1U << 6),
    FUERA_RANGO_P    = (1U << 7),
    SECO_A           = (1U << 8),
    SECO_P           = (1U << 9),
    RUIDO_SECO_A     = (1U << 10),
    RUIDO_SECO_P     = (1U << 11),
    SIN_IMPULSION_P  = (1U << 12)
  };

  struct Fila {
    uint16_t sesion;
    uint8_t  tipo;
    uint16_t calidad;
    int64_t  tRegistro_us;
    int64_t  tAdquisicion_us;
    uint64_t nA;
    uint64_t nP;
    uint64_t rechazosA;
    uint64_t rechazosP;
    float    rpm;
    float    qA;
    float    qP;
    float    kA;
    float    kP;
  };

  static constexpr size_t   CAPACIDAD    = 600;
  static constexpr uint16_t SESIONES_MAX = 20;
  static constexpr int64_t  INTERVALO_US = 10000000; // 10 segundos

  Fila filas[CAPACIDAD];

  size_t   cantidad()            const { return _n; }
  bool     activo()              const { return _activo; }
  bool     incompleto()          const { return _incompleto; }
  uint32_t intervalosOmitidos()  const { return _omitidos; }
  uint16_t sesionActual()        const { return _sesion; }

  bool iniciar(int64_t tAdquisicion_us, float rpm, const Caudalimetro& a, const Caudalimetro& p) {
    if (_n + 2 > CAPACIDAD || _sesion >= SESIONES_MAX) {
      _incompleto = true;
      return false;
    }
    _sesion++;
    _inicio_us = esp_timer_get_time();
    _proximo_us = _inicio_us + INTERVALO_US;
    _activo = true;
    return guardar(INICIO, _inicio_us, tAdquisicion_us, rpm, a, p);
  }

  void tick(int64_t tAdquisicion_us, float rpm, const Caudalimetro& a, const Caudalimetro& p) {
    if (!_activo) return;
    const int64_t ahora = esp_timer_get_time();
    if (ahora < _proximo_us) return;

    const uint64_t vencidos = (uint64_t)((ahora - _proximo_us) / INTERVALO_US) + 1;
    _proximo_us += (int64_t)vencidos * INTERVALO_US;

    if (vencidos > 1) {
      _omitidos += (uint32_t)(vencidos - 1);
      _incompleto = true;
    }

    if (_n >= CAPACIDAD - 1) { // Reserva plaza obligatoria para el evento FIN
      _omitidos++;
      _incompleto = true;
      return;
    }

    guardar(PERIODICA, ahora, tAdquisicion_us, rpm, a, p);
  }

  bool finalizar(int64_t tAdquisicion_us, float rpm, const Caudalimetro& a, const Caudalimetro& p) {
    if (!_activo) return false;
    const bool ok = guardar(FIN, esp_timer_get_time(), tAdquisicion_us, rpm, a, p);
    _activo = false;
    return ok;
  }

  void limpiar() {
    _n = 0;
    _sesion = 0;
    _activo = false;
    _incompleto = false;
    _omitidos = 0;
  }

private:
  size_t   _n = 0;
  uint16_t _sesion = 0;
  bool     _activo = false;
  bool     _incompleto = false;
  uint32_t _omitidos = 0;
  int64_t  _inicio_us = 0;
  int64_t  _proximo_us = 0;

  static uint16_t calcularCalidad(const Caudalimetro& a, const Caudalimetro& p) {
    uint16_t f = 0;
    if (a.calibrado())            f |= CAL_A;
    if (p.calibrado())            f |= CAL_P;
    if (a.periodoDisponible())    f |= PERIODO_A;
    if (p.periodoDisponible())    f |= PERIODO_P;
    if (a.sinSenal())             f |= SIN_SENAL_A;
    if (p.sinSenal())             f |= SIN_SENAL_P;
    if (a.fueraDeRango())         f |= FUERA_RANGO_A;
    if (p.fueraDeRango())         f |= FUERA_RANGO_P;
    if (a.modoSeco())             f |= SECO_A;
    if (p.modoSeco())             f |= SECO_P;
    if (a.ruidoDetectadoEnSeco()) f |= RUIDO_SECO_A;
    if (p.ruidoDetectadoEnSeco()) f |= RUIDO_SECO_P;
    if (p.flujoSinImpulsion())    f |= SIN_IMPULSION_P;
    return f;
  }

  bool guardar(Tipo tipo, int64_t ahora, int64_t adquisicion, float rpm, const Caudalimetro& a, const Caudalimetro& p) {
    if (_n >= CAPACIDAD) {
      _incompleto = true;
      return false;
    }
    Fila& r = filas[_n++];
    r.sesion = _sesion;
    r.tipo = (uint8_t)tipo;
    r.calidad = calcularCalidad(a, p);
    r.tRegistro_us = ahora - _inicio_us;
    r.tAdquisicion_us = (adquisicion >= _inicio_us) ? (adquisicion - _inicio_us) : -1;
    r.nA = a.pulsosLiquidoAcumulados();
    r.nP = p.pulsosLiquidoAcumulados();
    r.rechazosA = a.rechazosAcumulados();
    r.rechazosP = p.rechazosAcumulados();
    r.rpm = rpm;

    bool tieneMedicion = (tipo != INICIO && r.tAdquisicion_us >= 0);
    r.qA = tieneMedicion ? a.caudal_mLmin() : 0.0f;
    r.qP = tieneMedicion ? p.caudal_mLmin() : 0.0f;
    r.kA = a.getK();
    r.kP = p.getK();
    return true;
  }
};
