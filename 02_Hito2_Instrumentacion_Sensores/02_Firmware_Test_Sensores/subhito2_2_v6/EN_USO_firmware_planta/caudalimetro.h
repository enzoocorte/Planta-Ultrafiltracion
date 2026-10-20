#pragma once
// ==============================================================================
// CLASE CAUDALÍMETRO — VERSIÓN V6.4 (Sensor de Efecto Hall YF-S401 - Tesis UNSa)
// - Interrupción por flanco FALLING con Blanking Físico Diferenciado:
//   * Alimentación (GPIO 14): 1200 us (máx 833 Hz)
//   * Permeado (GPIO 27):     3500 us (máx 285 Hz -> bloquea ruidos espurios)
// - Estimador Híbrido de Frecuencia (Recíproco de Ráfaga + Conteo de Ventana)
// - Filtro Mediana de 3 puntos + EMA para lectura estable en tiempo real.
// ==============================================================================

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include "config.h"

class Caudalimetro {
public:
  enum Calidad : uint16_t {
    INICIADO                = (1U << 0),
    CAL_DOCUMENTADA         = (1U << 1),
    PERIODO_DISPONIBLE      = (1U << 2),
    SIN_SENAL               = (1U << 3),
    FUERA_RANGO_CAL         = (1U << 4),
    FUERA_RANGO_OPERATIVO   = (1U << 5),
    SECO                    = (1U << 6),
    RUIDO_SECO              = (1U << 7),
    SIN_IMPULSION           = (1U << 8),
    MEDICION_EN_RANGO_CAL   = (1U << 9)
  };

  struct Muestra {
    int64_t  t_us = 0;
    int64_t  ultimoAceptado_us = -1;
    int64_t  edadPulso_us = -1;
    uint64_t flancos = 0;
    uint64_t aceptados = 0;
    uint64_t pulsosMedicion = 0;
    uint64_t rechazados = 0;
    uint32_t epoca = 0;
    uint16_t calidad = 0;
    double   k = 0.0;
    double   frecuencia_Hz = 0.0;
    double   q_mLmin = 0.0;
    double   volumenEstimado_L = 0.0;
  };

  explicit Caudalimetro(const ConfigSensor& config, const char* nombre, bool esAlimentacion = true);
  Caudalimetro(const Caudalimetro&) = delete;
  Caudalimetro& operator=(const Caudalimetro&) = delete;

  bool begin();
  Muestra capturar(bool bombaEmpuja);

  void setModoSeco(bool activo);
  void setBloqueado(bool bloqueado);
  bool bloqueado() const { return _bloqueado; }
  void limpiarAlarmaSeco();
  void resetVolumen();

  static double caudalMedio_mLmin(const Muestra& inicial, const Muestra& final);

  float       caudal_mLmin()          const;
  float       caudal_Lmin()           const;
  float       frecuencia_Hz()         const;
  double      volumen_L()             const { return _volumenEstimadoAcumulado_L; }
  const char* nombre()                const { return _nombre; }
  bool        esAlimentacion()        const { return _esAlimentacion; }
  const Muestra& ultimaMuestra()      const { return _ultimaMuestra; }

  bool sinSenal()                     const { return (_ultimaMuestra.calidad & SIN_SENAL) != 0; }
  bool periodoDisponible()            const { return (_ultimaMuestra.calidad & PERIODO_DISPONIBLE) != 0; }
  bool fueraDeRango()                 const { return (_ultimaMuestra.calidad & FUERA_RANGO_OPERATIVO) != 0; }
  bool fueraRangoCalibracion()        const { return (_ultimaMuestra.calidad & FUERA_RANGO_CAL) != 0; }
  bool medicionEnRangoCalibrado()     const { return (_ultimaMuestra.calidad & MEDICION_EN_RANGO_CAL) != 0; }
  bool flujoSinImpulsion()            const { return (_ultimaMuestra.calidad & SIN_IMPULSION) != 0; }
  bool calibrado()                    const { return (_ultimaMuestra.calidad & CAL_DOCUMENTADA) != 0; }
  bool ruidoDetectadoEnSeco()         const { return (_ultimaMuestra.calidad & RUIDO_SECO) != 0; }
  bool modoSeco()                     const { return (_ultimaMuestra.calidad & SECO) != 0; }
  bool falloAlimentacion()            const { return _falloAlimentacion; }

  uint32_t flancosBrutos()            const { return _flancosVentana; }
  uint32_t pulsosValidos()            const { return _validosVentana; }
  uint32_t glitchesVentana()          const { return _rechazosVentana; }
  uint64_t pulsosAceptadosAcumulados() const { return _ultimaMuestra.aceptados; }
  uint64_t pulsosLiquidoAcumulados()  const { return _ultimaMuestra.pulsosMedicion; }
  uint64_t rechazosAcumulados()       const { return _ultimaMuestra.rechazados; }

  bool  setK(double nuevoK, bool calibracionDocumentada = false);
  float getK()                        const { return (float)_cfg.k_Hz_por_Lmin; }
  bool  setQMinDetectable(double nuevoQMin_mLmin);
  float getQMinDetectable()           const { return (float)_cfg.qMinDetectable_mLmin; }
  bool  declararCalibrado(bool valido);

private:
  static void ARDUINO_ISR_ATTR isrPuente(void* arg);
  void ARDUINO_ISR_ATTR isr();
  void ARDUINO_ISR_ATTR reiniciarEstimadorLocked();
  static float mediana3(float a, float b, float c);

  ConfigSensor _cfg;
  const char*  _nombre;
  const bool   _esAlimentacion;

  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
  bool _iniciado = false;

  volatile bool     _seco = false;
  volatile bool     _bloqueado = false;
  volatile bool     _alarmaSeco = false;
  volatile bool     _hayAnterior = false;

  volatile int64_t  _primeroVentana_us = 0;
  volatile int64_t  _ultimoVentana_us = 0;
  volatile int64_t  _bajadaAnterior_us = 0;
  volatile int64_t  _ultimoAceptado_us = -1;
  volatile int64_t  _ultimoPeriodo_us = 0;
  volatile uint32_t _pulsosVentana = 0;

  volatile uint64_t _flancos = 0;
  volatile uint64_t _aceptados = 0;
  volatile uint64_t _pulsosMedicion = 0;
  volatile uint64_t _rechazados = 0;
  volatile uint32_t _epoca = 1;

  int64_t  _tCapturaPrev_us = 0;
  uint64_t _anteriorMedicion = 0;
  uint64_t _pulsosOffset = 0;
  uint32_t _flancosVentana = 0;
  uint32_t _validosVentana = 0;
  uint32_t _rechazosVentana = 0;
  uint64_t _flancosPrevios = 0;
  uint64_t _aceptadosPrevios = 0;
  uint64_t _rechazosPrevios = 0;
  double   _volumenEstimadoAcumulado_L = 0.0;
  float    _qHist[3] = {0.0f, 0.0f, 0.0f};
  uint8_t  _qHistCount = 0;
  uint8_t  _cerosConsecutivos = 0;
  float    _qSuavizadoDisplay = 0.0f;
  int64_t  _inicioSinPulso_us = -1;
  bool     _falloAlimentacion = false;
  Muestra  _ultimaMuestra;
};
