#pragma once
// ==============================================================================
// CLASE CAUDALÍMETRO (Sensor de Efecto Hall YF-S401 - V5.1 Certificado / GPT Astra)
// - Detección por interrupción en ambos flancos (CHANGE)
// - Validación geométrica de ancho de nivel (rechaza glitches EMI y rebotes de rampa RC)
// - Continuidad temporal estricta de 64 bits (inmune a desbordamiento de micros y cortes de ventana)
// - Estimación de frecuencia recíproca sobre períodos completos reales
// - Acumulación íntegra de volumen sin supresión por zona muerta de pantalla
// - Modo de diagnóstico de prueba en seco atómico
// ==============================================================================

#include <Arduino.h>
#include <stdint.h>
#include "config.h"

struct ParametrosPulso {
  uint32_t anchoMinLow_us;    // Duración mínima de nivel LOW para aceptar álabe (us)
  uint32_t anchoMinHigh_us;   // Duración mínima de nivel HIGH previo (us)
  uint32_t periodoMin_us;     // Período mínimo admisible entre pulsos (us)
  uint32_t periodoMax_us;     // Período máximo antes de declarar flujo detenido (us)
  float    qMax_mLmin;        // Cota superior física del canal (mL/min)
};

class Caudalimetro {
public:
  Caudalimetro(uint8_t pin, float k, const char* nombre, bool esAlimentacion = true);

  // Inicialización (llamar en setup)
  bool begin();

  // Actualización periódica en el bucle principal (loop)
  void actualizar(float dt_s, bool bombaEmpuja, float qBombaTeorico_mLmin = 0.0f);

  // Getters de magnitudes físicas
  float  caudal_mLmin()   const { return _q; }
  float  caudal_Lmin()    const { return _q / 1000.0f; }
  float  frecuencia_Hz()  const { return _f; }
  double volumen_L()      const { return _vol; }
  void   resetVolumen()         { _vol = 0.0; }
  const char* nombre()    const { return _nombre; }
  bool   esAlimentacion() const { return _esAlimentacion; }

  // Diagnósticos metrológicos
  bool sinSenal()          const { return _sinSenal; }
  bool falloAlimentacion() const { return _fallo; }
  bool periodoDisponible() const { return _periodoDisponible; }
  bool fueraDeRango()      const { return _fueraRango; }
  bool flujoSinImpulsion() const { return _sinImpulsion; }
  bool calibrado()         const { return _calibrado; }

  // Métricas de la última ventana (1 s)
  uint32_t flancosBrutos()   const { return _flancosVentana; }
  uint32_t pulsosValidos()   const { return _validosVentana; }
  uint32_t glitchesVentana() const { return _rechazosVentana; }
  uint32_t pulsosBrutos()    const { return _validosVentana; } // compatibilidad

  // Contadores acumulados absolutos desde el arranque
  uint64_t pulsosAceptadosAcumulados() const { return _aceptadosAcumulados; }
  uint64_t pulsosLiquidoAcumulados()   const { return _liquidoAcumulados; }
  uint64_t rechazosAcumulados()        const { return _rechazosAcumulados; }

  // Control y estado del Modo de Prueba en Seco
  bool ruidoDetectadoEnSeco() const { return _ruidoEnSeco; }
  bool modoSeco()             const { return _modoSeco; }
  void setModoSeco(bool activo);
  void limpiarAlarmaSeco();

  // Calibración y certificación
  bool  setK(float nuevoK);
  float getK() const { return _k; }
  bool  declararCalibrado(bool valido);

private:
  static void ARDUINO_ISR_ATTR isrPuente(void* arg);
  void ARDUINO_ISR_ATTR isrInterna();

  const uint8_t   _pin;
  float           _k;
  const char*     _nombre;
  const bool      _esAlimentacion;
  ParametrosPulso _p;

  bool _iniciado  = false;
  bool _calibrado = false;
  bool _modoSeco  = false;

  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;

  // Variables atómicas de la ISR (64 bits, protegidas por spinlock)
  volatile bool     _nivelAlto = false;
  volatile bool     _altoConocido = false;
  volatile bool     _candidato = false;
  volatile bool     _hayUltimo = false;

  volatile int64_t  _inicioNivel_us = 0;
  volatile int64_t  _bajada_us = 0;
  volatile int64_t  _ultimoPulso_us = 0;
  volatile int64_t  _ultimoPeriodo_us = 0;

  volatile uint32_t _flancos_ISR = 0;
  volatile uint32_t _aceptados_ISR = 0;
  volatile uint32_t _liquido_ISR = 0;
  volatile uint32_t _rechazos_ISR = 0;
  volatile uint32_t _periodos_ISR = 0;
  volatile int64_t  _sumaPeriodos_ISR_us = 0;

  volatile bool     _seco_ISR = false;
  volatile bool     _alarmaSeco_ISR = false;

  // Variables de procesamiento de la tarea
  uint32_t _flancosVentana = 0;
  uint32_t _validosVentana = 0;
  uint32_t _rechazosVentana = 0;

  uint64_t _aceptadosAcumulados = 0;
  uint64_t _liquidoAcumulados = 0;
  uint64_t _rechazosAcumulados = 0;

  float  _f = 0.0f;
  float  _q = 0.0f;
  double _vol = 0.0;

  bool  _sinSenal = true;
  bool  _periodoDisponible = false;
  bool  _fueraRango = false;
  bool  _sinImpulsion = false;
  bool  _ruidoEnSeco = false;
  bool  _fallo = false;
  float _sinPulso_s = 0.0f;
};
