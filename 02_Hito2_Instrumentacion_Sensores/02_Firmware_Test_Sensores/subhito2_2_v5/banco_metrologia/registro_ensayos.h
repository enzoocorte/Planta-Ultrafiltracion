#pragma once
// ==============================================================================
// REGISTRO DE ENSAYOS MULTI-SESIÓN SIN SOBREESCRITURA (Auditoría GPT Astra)
// - Estructura en RAM de 600 registros y hasta 20 sesiones de ensayo
// - Diferenciación estricta de eventos: INICIO (t=0), PERIODICA (10s) y FIN
// - Reserva fija de plaza para garantizar que el evento FIN nunca se descarte
// - Registro atómico de instantáneas Muestra independientes para cada sensor
// - Exportación limpia a CSV sin asignación dinámica de memoria (heap seguro)
// ==============================================================================

#include <Arduino.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_timer.h"
#include "caudalimetro.h"
#include "config.h"

class RegistroEnsayos {
public:
  enum class Tipo : uint8_t {
    INICIO    = 0,
    PERIODICA = 1,
    FIN       = 2
  };

  enum Estado : uint16_t {
    NORMAL       = 0,
    CAMBIO_EPOCA = (1U << 0),
    OMISIONES    = (1U << 1)
  };

  struct Fila {
    uint16_t sesion = 0;
    Tipo     tipo = Tipo::INICIO;
    uint16_t estado = 0;
    int64_t  tRegistro_us = 0;
    double   rpm = 0.0;
    Caudalimetro::Muestra a;
    Caudalimetro::Muestra p;
  };

  static constexpr size_t   MAX_FILAS = 300;
  static constexpr uint16_t MAX_SESIONES = 20;
  static constexpr int64_t  INTERVALO_LOG_US = 10000000; // 10 segundos

  RegistroEnsayos() {
    _filas = (Fila*)malloc(MAX_FILAS * sizeof(Fila));
    if (_filas) {
      for (size_t i = 0; i < MAX_FILAS; ++i) {
        _filas[i] = Fila{};
      }
    }
  }

  bool     activo()              const { return _activo; }
  bool     incompleto()          const { return _incompleto; }
  size_t   cantidad()            const { return _n; }
  uint64_t intervalosOmitidos()  const { return _omitidos; }
  uint16_t sesionActual()        const { return _sesion; }
  const Fila* fila(size_t i)     const { return (_filas && i < _n) ? &_filas[i] : nullptr; }

  bool iniciar(Caudalimetro& a, Caudalimetro& p, double rpm, bool bombaEmpuja) {
    if (!_filas || _activo || _sesion >= MAX_SESIONES || _n + 2 > MAX_FILAS ||
        !std::isfinite(rpm) || rpm < 0.0) {
      return false;
    }

    const int64_t origen = esp_timer_get_time();
    const auto ma = a.capturar(bombaEmpuja);
    const auto mp = p.capturar(bombaEmpuja);
    if (!iniciadas(ma, mp)) return false;

    _sesion++;
    _activo = true;
    _inicio_us = origen;
    _proximo_us = origen + INTERVALO_LOG_US;
    _epocaA = ma.epoca;
    _epocaP = mp.epoca;
    _estadoSesion = NORMAL;

    guardar(Tipo::INICIO, 0, rpm, ma, mp);
    return true;
  }

  bool tick(Caudalimetro& a, Caudalimetro& p, double rpm, bool bombaEmpuja) {
    if (!_activo) return false;
    const int64_t ahora = esp_timer_get_time();
    if (ahora < _proximo_us) return false;

    const uint64_t vencidos = (uint64_t)((ahora - _proximo_us) / INTERVALO_LOG_US) + 1;
    _proximo_us += (int64_t)vencidos * INTERVALO_LOG_US;

    // Se reserva 1 fila fija para FIN
    if (_n >= MAX_FILAS - 1 || !std::isfinite(rpm) || rpm < 0.0) {
      omitir(vencidos);
      return false;
    }

    const auto ma = a.capturar(bombaEmpuja);
    const auto mp = p.capturar(bombaEmpuja);
    if (!iniciadas(ma, mp)) {
      omitir(vencidos);
      return false;
    }

    if (vencidos > 1) {
      omitir(vencidos - 1);
    }

    guardar(Tipo::PERIODICA, esp_timer_get_time() - _inicio_us, rpm, ma, mp);
    return true;
  }

  bool finalizar(Caudalimetro& a, Caudalimetro& p, double rpm, bool bombaEmpuja) {
    if (!_activo || !std::isfinite(rpm) || rpm < 0.0) return false;

    const auto ma = a.capturar(bombaEmpuja);
    const auto mp = p.capturar(bombaEmpuja);
    if (!iniciadas(ma, mp)) return false;

    const int64_t ahora = esp_timer_get_time();
    if (ahora >= _proximo_us) {
      const uint64_t vencidos = (uint64_t)((ahora - _proximo_us) / INTERVALO_LOG_US) + 1;
      omitir(vencidos);
    }

    guardar(Tipo::FIN, ahora - _inicio_us, rpm, ma, mp);
    _activo = false;
    return true;
  }

  bool limpiar() {
    if (_activo) return false;
    _n = 0;
    _sesion = 0;
    _omitidos = 0;
    _incompleto = false;
    _estadoSesion = NORMAL;
    return true;
  }

  bool exportarCSV(Print& salida) const {
    if (_activo) return false;
    salida.clearWriteError();
    salida.println("# esquema=metrologia_nucleo_v5_1");
    salida.println("# tiempo_sensor_us=desde_arranque");
    salida.println("# volumen_L=estimados_por_pulsos_acumulados");
    salida.println("# nan=dato_no_disponible");
    salida.print("# incompleto=");
    salida.println(_incompleto ? "1" : "0");
    salida.print("# intervalos_omitidos=");
    imprimirU64(salida, _omitidos);
    salida.println();

    salida.println("sesion,tipo,estado,t_reg_us,rpm,"
                   "tA_us,epocaA,calidadA,kA,flancosA,aceptadosA,nA,rechazosA,edadA_us,fA_Hz,qA_mLmin,vA_L,"
                   "tP_us,epocaP,calidadP,kP,flancosP,aceptadosP,nP,rechazosP,edadP_us,fP_Hz,qP_mLmin,vP_L");

    for (size_t i = 0; i < _n; ++i) {
      const Fila& r = _filas[i];
      salida.print(r.sesion); salida.print(',');
      salida.print(static_cast<unsigned>(r.tipo)); salida.print(',');
      salida.print(r.estado); salida.print(',');
      imprimirI64(salida, r.tRegistro_us); salida.print(',');
      imprimirReal(salida, r.rpm); salida.print(',');
      imprimirMuestra(salida, r.a); salida.print(',');
      imprimirMuestra(salida, r.p);
      salida.println();
    }
    return (salida.getWriteError() == 0);
  }

private:
  Fila*    _filas = nullptr;
  size_t   _n = 0;
  uint16_t _sesion = 0;
  bool     _activo = false;
  bool     _incompleto = false;
  uint64_t _omitidos = 0;
  int64_t  _inicio_us = 0;
  int64_t  _proximo_us = 0;
  uint32_t _epocaA = 0;
  uint32_t _epocaP = 0;
  uint16_t _estadoSesion = NORMAL;

  static bool iniciadas(const Caudalimetro::Muestra& a, const Caudalimetro::Muestra& p) {
    return ((a.calidad & Caudalimetro::INICIADO) && (p.calidad & Caudalimetro::INICIADO));
  }

  void omitir(uint64_t cantidad) {
    if (cantidad == 0) return;
    _omitidos += cantidad;
    _incompleto = true;
    _estadoSesion |= OMISIONES;
  }

  void guardar(Tipo tipo, int64_t relativo, double rpm, const Caudalimetro::Muestra& a, const Caudalimetro::Muestra& p) {
    if (!_filas || _n >= MAX_FILAS) {
      _incompleto = true;
      return;
    }
    if (a.epoca != _epocaA || p.epoca != _epocaP) {
      _estadoSesion |= CAMBIO_EPOCA;
      _incompleto = true;
    }

    Fila& r = _filas[_n++];
    r.sesion       = _sesion;
    r.tipo         = tipo;
    r.estado       = _estadoSesion;
    r.tRegistro_us = relativo;
    r.rpm          = rpm;
    r.a            = a;
    r.p            = p;
  }

  static void imprimirU64(Print& s, uint64_t valor) {
    char texto[24];
    snprintf(texto, sizeof(texto), "%llu", (unsigned long long)valor);
    s.print(texto);
  }

  static void imprimirI64(Print& s, int64_t valor) {
    char texto[24];
    snprintf(texto, sizeof(texto), "%lld", (long long)valor);
    s.print(texto);
  }

  static void imprimirReal(Print& s, double valor) {
    if (std::isfinite(valor)) {
      s.print(valor, 4);
    } else {
      s.print("nan");
    }
  }

  static void imprimirMuestra(Print& s, const Caudalimetro::Muestra& m) {
    imprimirI64(s, m.t_us);              s.print(',');
    s.print(m.epoca);                    s.print(',');
    s.print(m.calidad);                  s.print(',');
    imprimirReal(s, m.k);                s.print(',');
    imprimirU64(s, m.flancos);           s.print(',');
    imprimirU64(s, m.aceptados);         s.print(',');
    imprimirU64(s, m.pulsosMedicion);    s.print(',');
    imprimirU64(s, m.rechazados);        s.print(',');
    imprimirI64(s, m.edadPulso_us);      s.print(',');
    imprimirReal(s, m.frecuencia_Hz);    s.print(',');
    imprimirReal(s, m.q_mLmin);          s.print(',');
    imprimirReal(s, m.volumenEstimado_L);
  }
};
