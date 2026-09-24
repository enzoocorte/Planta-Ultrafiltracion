#pragma once
#include <Arduino.h>
#include "config.h"

// ============================================================
//  CAUDALÍMETRO (turbina tipo YF-S401)
//  Cada instancia registra su propia interrupción: para agregar
//  otro sensor basta crear otra instancia y llamar begin().
// ============================================================
class Caudalimetro {
public:
  Caudalimetro(uint8_t pin, float k, const char* nombre)
    : _pin(pin), _k(k), _nombre(nombre) {}

  void begin() {
    pinMode(_pin, INPUT_PULLUP);
    // attachInterruptArg pasa 'this' a la ISR → cada sensor se auto-registra
    attachInterruptArg(digitalPinToInterrupt(_pin), isrPuente, this, FALLING);
  }

  // Llamar 1 vez por segundo. bombaEmpuja: la bomba está moviendo líquido.
  void actualizar(float dt_s, bool bombaEmpuja) {
    portENTER_CRITICAL(&_mux);              // sección atómica (ESP32 es dual-core)
    uint32_t n = _pulsos;
    _pulsos = 0;
    portEXIT_CRITICAL(&_mux);

    _f = (dt_s > 0.0f) ? ((float)n / dt_s) : 0.0f;     // Frecuencia física real en Hz (pulsos/seg)
    float q = (_f * 1000.0f) / _k;                      // mL/min instantáneo real: Q (L/min) = F/K -> mL/min = (F*1000)/K
    if (q > Q_MAX_FISICO_MLMIN) {                       // defensa anti-ruido por encima de 6 L/min
      q = 0.0f;
      Serial.printf("[%s] %lu pulsos falsos descartados (f=%.1f Hz)\n", _nombre, (unsigned long)n, _f);
    }
    _q  = (n > 0) ? (0.3f * q + 0.7f * _q) : 0.0f;      // suavizado (estabiliza display, cae a 0 si se detiene)
    _vol += (float)n / (_k * 60.0f);                    // volumen EXACTO por conteo de pulsos: K*60 = 5880 pulsos/L

    // Bomba empujando + 5 s sin NI UN pulso → burbuja de aire o cable suelto
    _fallo = bombaEmpuja && (micros() - _t_ultimo > 5000000UL);
  }

  float caudal_mLmin()  const { return _q; }
  float caudal_Lmin()   const { return _q / 1000.0f; }
  float frecuencia_Hz() const { return _f; }
  float volumen_L()     const { return _vol; }
  bool  sinSenal()      const { return _fallo; }
  void  resetVolumen()        { _vol = 0.0f; }
  const char* nombre()  const { return _nombre; }

private:
  static void IRAM_ATTR isrPuente(void* arg);

  const uint8_t _pin;
  const float   _k;
  const char*   _nombre;
  volatile uint32_t _pulsos = 0, _t_ultimo = 0;
  float _f = 0.0f, _q = 0.0f, _vol = 0.0f;
  bool  _fallo = false;
  static portMUX_TYPE _mux;                 // mutex compartido entre instancias
};