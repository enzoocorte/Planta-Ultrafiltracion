/**
 * @file caudalimetro.h
 * @brief Controlador y filtro anti-rebote (glitch filter) para Caudalímetros YF-S401
 * Proyecto: Planta Piloto de Ultrafiltración - Tesis de Grado UNSa
 * Autor: Ing. en formación
 */

#ifndef CAUDALIMETRO_H
#define CAUDALIMETRO_H

#include <Arduino.h>

class Caudalimetro {
private:
    uint8_t _pin;
    volatile uint32_t _pulseCount;
    volatile uint32_t _lastPulseMicros;
    uint32_t _debounceUs;     // Ventana de inhibición (ej. 10000 us = 10 ms)
    float _factorK;           // Factor de conversión (5880 pulsos/L para YF-S401)
    portMUX_TYPE _mux;

public:
    /**
     * @param pin GPIO del ESP32 donde entra la señal (ej. 27 o 14)
     * @param factorK Pulsos por litro según calibración (defecto: 5880.0)
     * @param debounceMs Filtro por software ante ruidos espurios rápidos (defecto: 10 ms)
     */
    Caudalimetro(uint8_t pin, float factorK = 5880.0f, uint32_t debounceMs = 10) 
        : _pin(pin), _pulseCount(0), _lastPulseMicros(0), 
          _debounceUs(debounceMs * 1000), _factorK(factorK) {
        _mux = portMUX_INITIALIZER_UNLOCKED;
    }

    void begin(void (*isrCallback)()) {
        // Modo INPUT simple: el pull-up de 4.7k a 3.3V y el cap de 100nF ya están en hardware
        pinMode(_pin, INPUT);
        attachInterrupt(digitalPinToInterrupt(_pin), isrCallback, FALLING);
    }

    // Llamado dentro de la Rutina de Servicio de Interrupción (ISR)
    void IRAM_ATTR handleInterrupt() {
        uint32_t now = micros();
        if (now - _lastPulseMicros >= _debounceUs) {
            _pulseCount++;
            _lastPulseMicros = now;
        }
    }

    /**
     * @brief Lee el caudal instantáneo en L/min y resetea el contador bajo sección crítica
     * @param dtSeconds Intervalo transcurrido desde la última lectura (ej. 1.0 s)
     * @return Caudal en Litros por minuto (L/min)
     */
    float getFlowRateLmin(float dtSeconds) {
        uint32_t pulses;
        portENTER_CRITICAL(&_mux);
        pulses = _pulseCount;
        _pulseCount = 0;
        portEXIT_CRITICAL(&_mux);

        if (dtSeconds <= 0 || _factorK <= 0) return 0.0f;

        // Frecuencia en Hz
        float freqHz = (float)pulses / dtSeconds;
        // Caudal Q = (Hz / 98) = (pulsos / dt) * 60 / 5880 L/min
        float flowLmin = (freqHz * 60.0f) / _factorK;
        return flowLmin;
    }

    uint32_t getTotalPulses() {
        uint32_t pulses;
        portENTER_CRITICAL(&_mux);
        pulses = _pulseCount;
        portEXIT_CRITICAL(&_mux);
        return pulses;
    }
};

#endif // CAUDALIMETRO_H
