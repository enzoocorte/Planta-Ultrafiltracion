# ESPECIFICACIÓN TÉCNICA Y FIRMWARE: CAUDALÍMETROS Y MOTOR DM860 (ESP32)
**Proyecto:** Planta Piloto de Ultrafiltración - Tesis de Grado UNSa  
**Hardware CPU:** ESP32 NodeMCU-32S (38 pines / 30 pines) montado en Bornera Adaptadora Goouuu V4  

---

## 1. Mapeo de Pines (Pinout Físico)

| Periférico / Señal | GPIO ESP32 | Modo Pin | Función / Acondicionamiento |
| :--- | :---: | :---: | :--- |
| **Caudalímetro 1 (Permeado)** | `GPIO 27` | `INPUT` | Pulsos Hall NPN. Pull-up $4.7\text{ k}\Omega$ a 3.3V + Cap $100\text{ nF}$ a GND |
| **Caudalímetro 2 (Retentado)**| `GPIO 14` | `INPUT` | Pulsos Hall NPN. Pull-up $4.7\text{ k}\Omega$ a 3.3V + Cap $100\text{ nF}$ a GND |
| **Driver DM860 (PUL+ / STEP)**| `GPIO 18` | `OUTPUT` | Tren de pulsos paso a paso (PWM o Timer) |
| **Driver DM860 (DIR+ / DIR)** | `GPIO 19` | `OUTPUT` | Dirección de giro (HIGH = Horario / LOW = Antihorario) |
| **Driver DM860 (PUL- y DIR-)**| `GND` | Común | Masa lógica común con la fuente y el ESP32 |
| **Driver DM860 (ENA+ / ENA-)**| *Flotante* | Sin conexión | Habilitado por defecto por hardware |

---

## 2. Parámetros de Calibración Hidráulica (YF-S401)

- **Factor nominal K:** $5880\text{ pulsos/Litro}$ ($\approx 98\text{ Hz}$ por cada $1\text{ L/min}$).
- **Fórmula de caudal:**
  $$Q\,[\text{L/min}] = \frac{f\,[\text{Hz}] \times 60}{K} = \frac{f\,[\text{Hz}]}{98}$$
- **Filtro antirebote por software (Glitch Filter):** $10\text{ ms}$ ($10000\,\mu\text{s}$) de ventana de inhibición en la ISR.

---

## 3. Estructura de Archivos del Proyecto

```text
firmware_planta/
├── firmware_planta.ino     // Archivo principal con FreeRTOS / Loop
└── caudalimetro.h          // Clase encapsulada con sección crítica y glitch filter
```

---

## 4. Código: `caudalimetro.h`

```cpp
/**
 * @file caudalimetro.h
 * @brief Controlador y filtro anti-rebote (glitch filter) para Caudalímetros YF-S401
 * Proyecto: Planta Piloto de Ultrafiltración - Tesis de Grado UNSa
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
     * @param debounceMs Filtro por software ante ruidos espurios (defecto: 10 ms)
     */
    Caudalimetro(uint8_t pin, float factorK = 5880.0f, uint32_t debounceMs = 10) 
        : _pin(pin), _pulseCount(0), _lastPulseMicros(0), 
          _debounceUs(debounceMs * 1000), _factorK(factorK) {
        _mux = portMUX_INITIALIZER_UNLOCKED;
    }

    void begin(void (*isrCallback)()) {
        // Modo INPUT simple: el pull-up de 4.7k a 3.3V y el capacitor de 100nF ya están en hardware
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

        float freqHz = (float)pulses / dtSeconds;
        return (freqHz * 60.0f) / _factorK;
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
```

---

## 5. Código Principal: `firmware_planta.ino`

```cpp
/**
 * @file firmware_planta.ino
 * @brief Lectura de Caudalímetros YF-S401 y Control de Motor Paso a Paso con Driver DM860
 * Proyecto: Planta Piloto de Ultrafiltración - Tesis de Grado UNSa
 */

#include <Arduino.h>
#include "caudalimetro.h"

// ==========================================
// DEFINICIÓN DE PINES
// ==========================================
#define PIN_FLOW_PERMEADO   27
#define PIN_FLOW_RETENTADO  14

#define PIN_STEP            18
#define PIN_DIR             19

// ==========================================
// INSTANCIAS DE SENSORES
// ==========================================
Caudalimetro flowPermeado(PIN_FLOW_PERMEADO, 5880.0f, 10);
Caudalimetro flowRetentado(PIN_FLOW_RETENTADO, 5880.0f, 10);

// ==========================================
// RUTINAS DE INTERRUPCIÓN (ISRs)
// ==========================================
void IRAM_ATTR isrPermeado() {
    flowPermeado.handleInterrupt();
}

void IRAM_ATTR isrRetentado() {
    flowRetentado.handleInterrupt();
}

// Variables de temporización
unsigned long lastSampleTime = 0;
const unsigned long sampleInterval = 1000; // Muestreo cada 1 segundo (1000 ms)

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=================================================");
    Serial.println(" Planta Piloto de Ultrafiltracion - UNSa");
    Serial.println(" Inicializando Caudalimetros y Driver DM860...");
    Serial.println("=================================================");

    // Configuración de pines del Driver DM860
    pinMode(PIN_STEP, OUTPUT);
    pinMode(PIN_DIR, OUTPUT);
    digitalWrite(PIN_STEP, LOW);
    digitalWrite(PIN_DIR, HIGH); // Sentido horario por defecto

    // Inicialización de caudalímetros e interrupciones
    flowPermeado.begin(isrPermeado);
    flowRetentado.begin(isrRetentado);

    Serial.println("[OK] Sistema iniciado y listo.");
}

void loop() {
    unsigned long currentMillis = millis();

    // Lectura periódica de caudal cada 1 segundo
    if (currentMillis - lastSampleTime >= sampleInterval) {
        float dt = (currentMillis - lastSampleTime) / 1000.0f;
        lastSampleTime = currentMillis;

        float qPermeado = flowPermeado.getFlowRateLmin(dt);
        float qRetentado = flowRetentado.getFlowRateLmin(dt);
        float qTotal = qPermeado + qRetentado;

        // Telemetría por consola Serial
        Serial.printf("[CAUDALES] Permeado: %.3f L/min | Retentado: %.3f L/min | Total: %.3f L/min\n", 
                      qPermeado, qRetentado, qTotal);
    }

    // Aquí se integra la máquina de estados o lógica de control de la bomba
}
```

---

## 6. Puntos Clave de Verificación

1. **Hardware en Bornera:**
   - La resistencia de $4.7\text{ k}\Omega$ (Amarillo-Violeta-Rojo-Oro) conecta **3.3V** con el terminal de señal Hall de cada sensor.
   - Los capacitores cerámicos 104 ($100\text{ nF}$) están en paralelo entre Señal y GND, filtrando el ruido de conmutación electromagnético del motor.
2. **Conexión Driver DM860:**
   - `D18` $\rightarrow$ `PUL+`
   - `D19` $\rightarrow$ `DIR+`
   - `GND` del ESP32 $\rightarrow$ `PUL-` y `DIR-` (comunes en puente)
   - `ENA+` y `ENA-` se dejan sin conectar.
3. **Firmware:**
   - El modo de los pines de caudal es `INPUT` estricto (no `INPUT_PULLUP`), ya que el voltaje de pull-up está fijado externamente por la resistencia de $4.7\text{ k}\Omega$ conectada a la línea de 3.3V.
   - Las rutinas `handleInterrupt()` se ejecutan en RAM (`IRAM_ATTR`) para evitar retrasos por acceso a la memoria Flash.
