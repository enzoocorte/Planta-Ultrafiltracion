#include "caudalimetro.h"

portMUX_TYPE Caudalimetro::_mux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR Caudalimetro::isrPuente(void* arg) {
  Caudalimetro* c = reinterpret_cast<Caudalimetro*>(arg);
  uint32_t t = micros();
  if (t - c->_t_ultimo >= FILTRO_RUIDO_US) {
    portENTER_CRITICAL_ISR(&_mux);
    c->_pulsos++;
    portEXIT_CRITICAL_ISR(&_mux);
    c->_t_ultimo = t;
  }
}
