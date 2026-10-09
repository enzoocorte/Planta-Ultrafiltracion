# PROMPT DE AUDITORÍA EXTERNA — RONDA 7: CERTIFICACIÓN METROLÓGICA FINAL FIRMWARE V5 (SUBHITO 2.2)

> **INSTRUCCIÓN PARA EL MODELO AUDITOR (Claude Opus / GPT-4o / GLM-4 / Gemini Pro):**
> Actúas como un **Auditor Metrológico y Experto Senior en Sistemas Embebidos de Tiempo Real (ESP32 / FreeRTOS / ESP-IDF)**.
> Se te presenta el caso completo de ingeniería de una Planta Piloto de Ultrafiltración Tangencial. Lee atentamente todo el contexto, el análisis de fallas previas, las conclusiones de la auditoría anterior (Ronda 6) y el nuevo código implementado (Firmware Versión 5.0).
> **Tu objetivo:** Emitir un dictamen metrológico formal (**[APROBADO]**, **[APROBADO CON OBSERVACIONES]** o **[RECHAZADO]**), auditar la eficacia de la solución frente al hardware congelado, y **compartir tu código completo aprobado** listo para producción con las mejoras que consideres necesarias.

---

## 1. CONTEXTO GENERAL DEL PROYECTO Y TESIS DE GRADO

* **Institución:** Universidad Nacional de Salta (UNSa), Facultad de Ingeniería, Salta, Argentina.
* **Proyecto:** Tesis de Grado de Ingeniería Industrial ("Automatización, Modelado y Validación Experimental de una Planta Piloto de Ultrafiltración Tangencial").
* **Tesistas:** Antonella Guitián & Owen Cañizares.
* **Codirección Técnica:** Ing. Enzo Corte.
* **Membrana:** Cartucho capilar de hemodiálisis de alta permeabilidad **Fresenius FX100 (Helixone®)**:
  * Área superficial interfacial de filtración: $A = 2.2\text{ m}^2$.
  * Diámetro interno capilar: $d_i = 185\text{ }\mu\text{m}$, espesor de pared: $35\text{ }\mu\text{m}$.
  * Flujo de permeado objetivo: $J = \frac{Q_{\text{perm}} \cdot 0.06}{A}$ en $\text{LMH}$ ($\text{L}/(\text{m}^2\cdot\text{h})$).
  * Límite de seguridad de Presión Transmembrana: $\text{TMP}_{\max} \le 0.50\text{ bar}$.
* **Estructura por Hitos del Proyecto:**
  * **Hito 1:** Dimensionamiento mecánico, cabezal peristáltico, banco físico y fuente de potencia. *(COMPLETADO)*.
  * **Hito 2:** Instrumentación y Sensores.
    * *Subhito 2.1:* Caudalímetros individuales básicos en banco aislado. *(COMPLETADO)*.
    * *Subhito 2.2:* **(HITOS ACTUAL EN VALIDACIÓN)** Doble caudalímetro Hall (Alimentación y Permeado) acoplados al flujo pulsátil peristáltico, blindaje anti-EMI del motor paso a paso, datalogger multi-sesión de 600 registros, balance hidráulico tangencial ($Q_{\text{ret}} = Q_{\text{alim}} - Q_{\text{perm}}$, $Y\%$) y SCADA Web WiFi en ESP32.
    * *Subhito 2.3:* Transductores de presión piezorresistivos (0–1 bar) con bus I2C y ADC diferencial ADS1115 de 16 bits para el cálculo real de la Ley de Darcy en tiempo real ($R_m$, $R_{\text{torta}}$). *(PRÓXIMO HITO)*.
    * *Subhito 2.4:* Conductivímetro y temperatura (DS18B20).

---

## 2. ARQUITECTURA DE HARDWARE Y CONDICIÓN DE "HARDWARE CONGELADO"

### 2.1. Actuador y Planta Hidráulica
* **Bomba Peristáltica:** Cabezal de desplazamiento positivo de 4 rodillos acoplado a un motor paso a paso **NEMA 34** de alto torque (4.5 Nm).
* **Driver:** **Leadshine DM860** alimentado a 48V DC.
  * Configuración de micropasos: **3200 micro-pasos/rev** (1/16 de paso).
  * Corriente pico: **3.0 A RMS**.
  * Frecuencia de corte del chopper PWM: $\approx 20\text{ kHz}$.
  * Rango de operación: $15.0\text{ a }100.0\text{ RPM}$ (equivalente a $204\text{ a }1360\text{ mL/min}$ con cilindrada calibrada de $\approx 13.60\text{ mL/rev}$).

### 2.2. Microcontrolador y Sensores
* **MCU:** **ESP32 DOIT DevKit V1** (Core `arduino-esp32` v3.3.11 basado en ESP-IDF 5.x).
* **Sensores de Flujo:** 2 × **YF-S401** (Turbina mecánica tangencial de 6 álabes con sensor de Efecto Hall NPN Open-Collector).
  * **Canal Alimentación:** Conectado a **GPIO 18** (Borne 12 Sup). Rango esperado: $200\text{ a }1360\text{ mL/min}$ (frecuencias de $30\text{ a }270\text{ Hz}$).
  * **Canal Permeado:** Conectado a **GPIO 19** (Borne 11 Sup). Rango esperado: $0\text{ a }200\text{ mL/min}$ (frecuencias de $0\text{ a }70\text{ Hz}$).

### 2.3. RESTRICCIÓN INMUTABLE DE HARDWARE CONGELADO (NO MODIFICABLE)
El usuario tiene montado en bornera fija un circuito de acondicionamiento físico:
* Resistencia Pull-Up: $R = 4.7\text{ k}\Omega$ conectada a la línea de 3.3V.
* Filtro Pasa-Bajos RC: Capacitor cerámico multicapa con dos capacitores 104 en paralelo ($C = 200\text{ nF}$).
* **Constante de tiempo resultante:**
  $$\tau = R \cdot C = 4700\,\Omega \times 200 \times 10^{-9}\,\text{F} \approx 0.94\,\text{ms} \quad (f_c \approx 169\,\text{Hz})$$
* **El usuario indicó expresamente:**
  > *"El tema hardware no quiero tocar por ahora, quiero ver si con software lo solucionamos, ya que en hardware hice todo: el pull-up de la resistencia, el capacitor 104 e incluí un capacitor extra por las dudas. Es fundamental no desoldar componentes y validar todo por software para no estancar la tesis."*

---

## 3. HISTORIAL DE FALLAS EXPERIMENTALES Y DICTAMEN DE RONDA 6

Durante las pruebas de calibración volumétrica en banco con probeta graduada realizadas por Antonella, se observaron las siguientes anomalías críticas:

### A. Observaciones del Laboratorio (Reporte de Antonella):
1. **Prueba de Probeta con Permeado Desconectado:**
   * Se desconectó la manguera de alimentación para recoger el fluido en probeta graduada.
   * La línea de permeado quedó vacía y sin conexión hidráulica (caudal físico = $0.0\text{ mL/min}$).
   * **Fallo grave:** La aplicación SCADA marcaba caudal de permeado activo que aumentaba a medida que subían las RPM de la bomba (flujo fantasma).
2. **Ruido Acústico y Mecánico de la Bomba:**
   * A 30 y 40 RPM la bomba hacía ruidos fuertes y resonancia; a 60–80 RPM entraba en régimen suave y uniforme.
3. **Pérdida de Datos en Datalogger:**
   * El datalogger previo muestreaba a 1 Hz con búfer de 600 muestras (máximo 10 minutos). Al avanzar el ensayo escalonado de 20 a 90 RPM, los registros de 20, 30 y 40 RPM se sobreescribían y se perdían.
   * Además, el datalogger no comenzaba exactamente en $t = 0\text{ s}$.

### B. Diagnóstico Técnico de la Auditoría Previa (Ronda 6):
1. **Fallo del Filtro v4.1 (Blanking ciego 1500 µs + Relación de dispersión $\le 4.0$):**
   * El chopper del motor paso a paso NEMA 34 a RPM constantes conmuta a frecuencia estable. La radiación EMI inducida sobre el cableado genera un tren de pulsos falso pero **altamente regular**.
   * Como los intervalos $dt$ entre glitches eran similares, $\frac{dt_{\max}}{dt_{\min}} \approx 1.0 \le 4.0$. El filtro de dispersión v4.1 creía erróneamente que se trataba de una turbina girando perfectamente y validaba el flujo fantasma.
2. **Efecto Secundario del Filtro RC ($4.7\text{ k}\Omega + 200\text{ nF}$):**
   * Un $\tau = 0.94\text{ ms}$ genera un tiempo de subida de $t_r \approx 1.03\text{ ms}$ entre el 10% y el 90% de VDD.
   * Como el GPIO del microcontrolador no posee un trigger Schmitt de alta velocidad integrado para flancos lentos, cualquier rebote de masa (*ground bounce*) durante esa ventana de $1\text{ ms}$ provocaba múltiples disparos de interrupción por cada flanco real (*chattering*). Esto explicaba por qué el factor $K_{\text{alim}}$ experimental calibrado daba $\approx 196.5\text{ Hz/(L/min)}$ (el doble del valor nominal de fábrica de $98\text{ Hz/(L/min)}$).
3. **Zona Muerta Inadecuada:**
   * El firmware previo aplicaba un *deadband* simétrico de $30\text{ mL/min}$. Esto "cegaba" al sensor de permeado durante corridas reales de ultrafiltración de agua limpia, donde el flujo de permeado puede ser legítimamente de $10\text{ a }25\text{ mL/min}$.
4. **Violación de Integridad Metrológica (Objeción de use.ai):**
   * El código v4.1 simulaba una presión transmembrana de $0.20\text{ bar}$ en el modelo de Darcy para rellenar columnas del CSV. Los auditores dictaminaron que introducir valores ficticios contamina la validez de los datos de la tesis antes de tener instalados los sensores de presión del Subhito 2.3.

---

## 4. LA NUEVA ARQUITECTURA: FIRMWARE V5.0 (MOTOR DE VALIDACIÓN DE ANCHO DE NIVEL)

Para erradicar el 100% del ruido EMI y el chattering sin modificar el hardware, se rediseñó el núcleo del caudalímetro en el Firmware V5.0:

1. **Detección por `attachInterruptArg(..., CHANGE)` y Validación Geométrica de Ancho:**
   * En lugar de contar sólo flancos de bajada (`FALLING`), la ISR atiende ambos flancos (`CHANGE`) y lee inmediatamente el estado del pin (`digitalRead(_pin)`).
   * Al caer la señal (`HIGH -> LOW`), verifica que el semi-período en ALTO haya durado al menos `anchoMinHigh_us`.
   * Al subir la señal (`LOW -> HIGH`), verifica que el semi-período en BAJO haya durado al menos `anchoMinLow_us`.
   * **Filtro físico:** Un álabe mecánico real de turbina YF-S401 jamás puede generar un semi-período menor a $200\text{ }\mu\text{s}$ (Alimentación a $1360\text{ mL/min}$) o menor a $600\text{ }\mu\text{s}$ (Permeado a $200\text{ mL/min}$).
   * Los glitches de chopper EMI duran $< 50\text{ }\mu\text{s}$ y los rebotes de cruce del filtro RC duran $< 100\text{ }\mu\text{s}$. Todos estos transitorios son rechazados, contabilizados como `glitchesTotal` y descartados atómicamente.
2. **Zonas Muertas Adaptativas por Canal:**
   * Alimentación: $q_{\min} = 30.0\text{ mL/min}$.
   * Permeado: $q_{\min} = 4.0\text{ mL/min}$ (sensibilidad para ultrafiltración de bajo flujo sin ceguera).
3. **Compuerta de Plausibilidad Física (*Sanity Gate*):**
   * En permeado, si la bomba está apagada o su caudal teórico es $< 20.0\text{ mL/min}$, $Q_{\text{perm}} = 0.0\text{ mL/min}$ de forma estricta.
4. **Modo Seco Enclavable (`MODO_SECO_ON` / `OFF`):**
   * Permite calificar el banco sin líquido: si se activa el modo seco y la bomba gira a 80 RPM, cualquier pulso espurio enciende el testigo `ruido_seco = true`.
5. **Datalogger de 10 Segundos y Muestra Garantizada en $t = 0\text{ s}$:**
   * Se ajustó `INTERVALO_LOG_MS = 10000` (10 s). Las 600 muestras permiten **100 minutos continuos de ensayo**, capturando íntegramente desde 20 hasta 90 RPM.
   * Se fuerza una muestra inicial en $t = 0\text{ s}$ en el instante de encendido.
6. **Integridad del Modelo de Darcy:**
   * Se suspendió el cálculo con presión simulada. Darcy permanece en reposo (`valido = false`, $\text{TMP} = 0.00\text{ bar}$) hasta el Subhito 2.3.

---

## 5. CÓDIGO FUENTE COMPLETO DEL FIRMWARE V5.0

### Archivo 1: `caudalimetro.h`
```cpp
#pragma once
#include <Arduino.h>
#include "config.h"

// ==============================================================================
// CLASE CAUDALÍMETRO (Sensor de Efecto Hall YF-S401 - V5 Anti-EMI / Ronda 6)
// - Detección por interrupción en ambos flancos (CHANGE)
// - Validación física de ancho de nivel: rechaza glitches cortos (chopper motor y rebotes RC)
// - Medición de período recíproco sobre pulsos físicamente confirmados
// - Deadband asimétrico adaptativo por canal (Alimentación vs Permeado)
// - Modo de diagnóstico de prueba en seco (Enclavamiento de ruido EMI)
// - Sincronización atómica multinúcleo con spinlock FreeRTOS (portMUX_TYPE)
// ==============================================================================

struct ParametrosPulso {
  uint32_t anchoMinLow_us;    // Duración mínima de nivel LOW para aceptar álabe (us)
  uint32_t anchoMinHigh_us;   // Duración mínima de nivel HIGH previo (us)
  uint32_t periodoMin_us;     // Período mínimo admisible entre pulsos (us)
  uint32_t periodoMax_us;     // Período máximo antes de declarar turbina detenida (us)
  float    qMin_mLmin;        // Umbral de corte de caudal mínimo específico del canal
};

class Caudalimetro {
public:
  Caudalimetro(uint8_t pin, float k, const char* nombre, bool esAlimentacion = true);

  void begin();
  void actualizar(float dt_s, bool bombaEmpuja, float qBombaTeorico_mLmin = 0.0f);

  float caudal_mLmin()   const { return _q; }
  float caudal_Lmin()    const { return _q / 1000.0f; }
  float frecuencia_Hz()  const { return _f; }
  float volumen_L()      const { return _vol; }
  bool  sinSenal()       const { return _fallo; }
  void  resetVolumen()         { _vol = 0.0f; }
  const char* nombre()   const { return _nombre; }
  bool  esAlimentacion() const { return _esAlimentacion; }

  // Métricas de diagnóstico de la última ventana (1 s)
  uint32_t flancosBrutos()      const { return _flancosVentana; }
  uint32_t pulsosValidos()      const { return _validosVentana; }
  uint32_t glitchesVentana()    const { return _glitchesVentana; }
  uint32_t pulsosBrutos()       const { return _validosVentana; } // compatibilidad
  bool     ruidoDetectadoEnSeco() const { return _ruidoEnSeco; }

  // Control del Modo de Prueba en Seco (Condición C1 de Auditoría)
  void setModoSeco(bool activo) { _modoSeco = activo; if (activo) _ruidoEnSeco = false; }
  bool modoSeco() const { return _modoSeco; }

  // Calibración dinámica sin recompilar [Hz / (L/min)]
  void  setK(float nuevoK)     { if (nuevoK > 0.1f) _k = nuevoK; }
  float getK()           const { return _k; }

private:
  static void IRAM_ATTR isrPuente(void* arg);
  void IRAM_ATTR isrInterna();

  const uint8_t   _pin;
  float           _k;
  const char*     _nombre;
  const bool      _esAlimentacion;
  ParametrosPulso _p;

  // Variables de estado atómico de la ISR
  volatile bool     _bajadaCandidata = false;
  volatile uint32_t _tBajada = 0;
  volatile uint32_t _tSubida = 0;
  volatile uint32_t _tUltimoValido = 0;
  volatile uint32_t _tPrimeroValido = 0;

  volatile uint32_t _flancosTotal = 0;
  volatile uint32_t _validosTotal = 0;
  volatile uint32_t _glitchesTotal = 0;

  // Variables copiadas para procesamiento en la ventana
  uint32_t _flancosVentana = 0;
  uint32_t _validosVentana = 0;
  uint32_t _glitchesVentana = 0;

  float _f = 0.0f;
  float _q = 0.0f;
  float _vol = 0.0f;
  float _tiempoSinPulso_s = 0.0f;
  bool  _fallo = false;
  bool  _modoSeco = false;
  bool  _ruidoEnSeco = false;

  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
};
```

---

### Archivo 2: `caudalimetro.cpp`
```cpp
#include "caudalimetro.h"

// ==============================================================================
// IMPLEMENTACIÓN DE CAUDALÍMETRO (V5 - Anti-EMI & Validación de Ancho de Pulso)
// ==============================================================================

Caudalimetro::Caudalimetro(uint8_t pin, float k, const char* nombre, bool esAlimentacion)
  : _pin(pin), _k(k), _nombre(nombre), _esAlimentacion(esAlimentacion) {
  if (_esAlimentacion) {
    // Alimentación: Flujos de 200 a 1400 mL/min (Frecuencia hasta ~270 Hz)
    // A 270 Hz, el semi-período es ~1850 us. Filtramos glitches < 200 us.
    _p.anchoMinLow_us  = 200;
    _p.anchoMinHigh_us = 200;
    _p.periodoMin_us   = 1000;    // Máx 1000 Hz admisible
    _p.periodoMax_us   = 1000000; // 1 s sin pulsos => detenido
    _p.qMin_mLmin      = 30.0f;   // Umbral de corte de cuantificación mecánica
  } else {
    // Permeado: Flujos de 10 a 150 mL/min (Frecuencia hasta ~70 Hz con K=687 o ~20 Hz con K=196)
    // A 70 Hz, el semi-período es ~7100 us. Filtramos glitches < 600 us.
    _p.anchoMinLow_us  = 600;
    _p.anchoMinHigh_us = 600;
    _p.periodoMin_us   = 2500;    // Máx 400 Hz admisible
    _p.periodoMax_us   = 1500000; // 1.5 s sin pulsos => detenido
    _p.qMin_mLmin      = 4.0f;    // Umbral adaptativo bajo para ultrafiltración (no trunca a 30 mL/min)
  }
}

void Caudalimetro::begin() {
  pinMode(_pin, INPUT_PULLUP);
  _tSubida = micros();
  _tBajada = _tSubida;
  _tUltimoValido = _tSubida;
  // Interrupción en CHANGE para monitorear transiciones de subida y bajada
  attachInterruptArg(digitalPinToInterrupt(_pin), isrPuente, this, CHANGE);
}

void IRAM_ATTR Caudalimetro::isrPuente(void* arg) {
  reinterpret_cast<Caudalimetro*>(arg)->isrInterna();
}

void IRAM_ATTR Caudalimetro::isrInterna() {
  uint32_t t = micros();
  bool nivelAlto = digitalRead(_pin);

  portENTER_CRITICAL_ISR(&_mux);
  _flancosTotal++;

  if (!nivelAlto) {
    // Flanco de bajada (HIGH -> LOW)
    // Para que sea un flanco válido de álabe de turbina, el nivel HIGH previo
    // debe haber durado al menos _p.anchoMinHigh_us (diferencia sin signo)
    uint32_t duracionHigh = t - _tSubida;
    if (duracionHigh >= _p.anchoMinHigh_us) {
      _bajadaCandidata = true;
      _tBajada = t;
    } else {
      _bajadaCandidata = false;
      _glitchesTotal++;
    }
  } else {
    // Flanco de subida (LOW -> HIGH)
    // El nivel LOW que finaliza debe haber durado al menos _p.anchoMinLow_us
    uint32_t duracionLow = t - _tBajada;
    if (_bajadaCandidata && duracionLow >= _p.anchoMinLow_us) {
      uint32_t periodo = _tBajada - _tUltimoValido;
      if (_validosTotal == 0 || periodo >= _p.periodoMin_us) {
        if (_validosTotal == 0) {
          _tPrimeroValido = _tBajada;
        }
        _tUltimoValido = _tBajada;
        _validosTotal++;
      } else {
        _glitchesTotal++; // Rechazado por período menor al físico admisible
      }
    } else {
      _glitchesTotal++; // Rechazado: pulso estrecho (glitch EMI o rebote de rampa RC)
    }
    _bajadaCandidata = false;
    _tSubida = t;
  }
  portEXIT_CRITICAL_ISR(&_mux);
}

void Caudalimetro::actualizar(float dt_s, bool bombaEmpuja, float qBombaTeorico_mLmin) {
  // Captura atómica de eventos acumulados en la ventana de 1 segundo
  portENTER_CRITICAL(&_mux);
  uint32_t flancos  = _flancosTotal;  _flancosTotal  = 0;
  uint32_t nValidos = _validosTotal;  _validosTotal  = 0;
  uint32_t nGlitch  = _glitchesTotal; _glitchesTotal = 0;
  uint32_t tPrim    = _tPrimeroValido;
  uint32_t tUlt     = _tUltimoValido;
  portEXIT_CRITICAL(&_mux);

  _flancosVentana  = flancos;
  _validosVentana  = nValidos;
  _glitchesVentana = nGlitch;

  // En Modo Seco: si se detecta cualquier flanco o pulso, enclavar alarma
  if (_modoSeco && (flancos > 0 || nValidos > 0)) {
    _ruidoEnSeco = true;
  }

  // 1. Cálculo de frecuencia sobre pulsos físicamente validados
  if (nValidos >= 2 && (tUlt - tPrim) > 0) {
    _f = ((float)(nValidos - 1) * 1000000.0f) / (float)(tUlt - tPrim);
  } else if (nValidos == 1) {
    _f = 1.0f / dt_s;
  } else {
    _f = 0.0f;
  }

  // Si transcurrió más de periodoMax sin pulsos, flujo detenido
  uint32_t tAhora = micros();
  if ((tAhora - _tUltimoValido) > _p.periodoMax_us) {
    _f = 0.0f;
  }

  // 2. Cálculo de caudal instantáneo en mL/min
  float q = (_f * 1000.0f) / _k;

  // Deadband adaptativo por canal
  if (q < _p.qMin_mLmin || _f < 0.5f) {
    q = 0.0f;
    _f = 0.0f;
  }

  // Sanity Gate (Hallazgo GLM): En permeado, si la bomba está parada o caudal teórico < 20 mL/min,
  // físicamente no puede haber flujo transmembrana impulsado
  if (!_esAlimentacion && (qBombaTeorico_mLmin < 20.0f || !bombaEmpuja)) {
    q = 0.0f;
    _f = 0.0f;
  }

  // Plausibilidad física máxima
  if (q > Q_MAX_FISICO_MLMIN) {
    q = 0.0f;
    _f = 0.0f;
    Serial.printf("[%s] Ruido EMI descartado por plausibilidad: %lu flancos\n", _nombre, (unsigned long)flancos);
  } else if (_f > 0.0f && q > 0.0f && !_modoSeco) {
    // 3. Integración de volumen totalizado en litros
    _vol += (float)nValidos / (_k * 60.0f);
  }

  // 4. Filtrado EMA
  if (q > 0.0f) {
    if (_q == 0.0f) {
      _q = q;
    } else {
      _q = 0.25f * q + 0.75f * _q;
    }
  } else {
    // Corte inmediato a 0 (elimina cola lenta de 10s tras presionar STOP)
    _q = 0.0f;
  }

  // 5. Diagnóstico de pérdida de señal
  if (nValidos > 0 || _f > 0.05f) {
    _tiempoSinPulso_s = 0.0f;
    _fallo = false;
  } else if (bombaEmpuja && _esAlimentacion) {
    _tiempoSinPulso_s += dt_s;
    if (_tiempoSinPulso_s >= 5.0f) _fallo = true;
  } else {
    _tiempoSinPulso_s = 0.0f;
    _fallo = false;
  }
}
```

---

### Archivo 3: `config.h`
```cpp
#pragma once
// ==============================================================================
// CONFIGURACIÓN TÉCNICA — PLANTA DE ULTRAFILTRACIÓN (UNSa) - VERSIÓN 5.0
// Parámetros Cinemáticos, Calibración V5 Anti-EMI y Asignación de Pines
// ==============================================================================

#include <Arduino.h>

// 1. ASIGNACIÓN DE PINES (PINOUT)
constexpr uint8_t PIN_PUL                = 18;  // DM860 PUL+ (PUL- a GND)
constexpr uint8_t PIN_DIR                = 19;  // DM860 DIR+ (DIR- a GND)
constexpr uint8_t PIN_ENA                = 23;  // DM860 ENA+ (ENA- a GND)
constexpr uint8_t PIN_LED_BOMBA          = 2;   // LED Onboard indicador de marcha

// Caudalímetros de Efecto Hall YF-S401
constexpr uint8_t PIN_SENSOR_ALIMENTACION = 14;  // Sensor de Alimentación (Borne 12 Sup)
constexpr uint8_t PIN_SENSOR_PERMEADO     = 27;  // Sensor de Permeado (Borne 11 Sup)

// Puertos I2C para ADS1115 (Subhito 2.3)
constexpr uint8_t PIN_I2C_SDA             = 21;  // ESP32 SDA
constexpr uint8_t PIN_I2C_SCL             = 22;  // ESP32 SCL

// 2. PARÁMETROS CINEMÁTICOS DE LA BOMBA (MOTOR NEMA 34 / DM860)
constexpr uint16_t PULSOS_POR_REV = 3200;     // DM860 a 1/16 micropasos (3200 pul/rev)
constexpr float ML_POR_VUELTA     = 13.6000f; // Calibrado nominal: ~13.6 mL/rev

constexpr float RPM_MIN           = 15.0f;    // ~204 mL/min
constexpr float RPM_MAX           = 100.0f;   // ~1360 mL/min
constexpr float RPM_INICIO        = 25.0f;    // Consigna arranque suave
constexpr float RPM_ALARMA_MEMBRANA = 44.0f;  // 600 mL/min referencia

// Especificaciones Membrana FX100
constexpr float AREA_MEMBRANA_M2       = 2.2f;    // Superficie (m²)
constexpr float TMP_MAX_SEGURA_BAR     = 0.50f;   // Límite máx TMP (bar)
constexpr float P1_MAX_SEGURA_BAR      = 0.60f;   // Límite máx P1 (bar)

// Rampas Cinemáticas S-Curve
constexpr float ACEL_ARRANQUE_RPM_S  = 2.0f;
constexpr float ACEL_NOMINAL_RPM_S   = 3.5f;
constexpr float DESACEL_AJUSTE_RPM_S = 8.0f;
constexpr float FRENADO_PARADA_RPM_S = 70.0f;

// 3. CALIBRACIÓN DE CAUDALÍMETROS Y LÍMITES FÍSICOS
constexpr float K_ALIMENTACION = 196.50f; // Hz/(L/min)
constexpr float K_PERMEADO     = 687.33f; // Hz/(L/min)
constexpr float Q_MAX_FISICO_MLMIN = 2500.0f;

// 4. PARÁMETROS DEL DATALOGGER MULTI-SESIÓN
constexpr uint32_t INTERVALO_LOG_MS   = 10000;  // Muestreo cada 10 segundos
constexpr size_t   MAX_REGISTROS      = 600;    // 600 muestras = 100 minutos continuos
constexpr size_t   MAX_ENSAYOS        = 20;
constexpr uint8_t  MUESTRAS_AUTO_CAL  = 15;

// 5. CONFIGURACIÓN DE RED WI-FI Y SCADA
constexpr const char* SSID_AP  = "Bomba_Peristaltica_UF";
constexpr const char* PASS_AP  = "plantapiloto2";
```

---

### Archivo 4: `EN_USO_firmware_planta.ino` (Extracto del Loop, Datalogger y Servidor Web)
```cpp
// Sincronización cíclica del loop() (No bloqueante)
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  uint32_t tAhora = millis();

  // 1. Control cinemático de bomba cada 50 ms
  if (tAhora - tLoop >= 50) {
    float dt = (tAhora - tLoop) / 1000.0f;
    tLoop = tAhora;
    bomba.tick(dt);
    digitalWrite(PIN_LED_BOMBA, (bomba.rpmActual() > 0.5f) ? HIGH : LOW);

    bool enMarcha = bomba.enMarcha();
    if (enMarcha && !bombaEnMarchaAnterior) {
      iniciarNuevoEnsayo(bomba.rpmObjetivo());
    }
    if (!enMarcha && bombaEnMarchaAnterior) {
      finalizarEnsayoActual();
    }
    bombaEnMarchaAnterior = enMarcha;
  }

  // 2. Adquisición de Caudales cada 1000 ms
  if (tAhora - tCaudal >= 1000) {
    float dt = (tAhora - tCaudal) / 1000.0f;
    tCaudal = tAhora;

    bool bombaEmpuja = (bomba.rpmActual() > 1.0f);
    float qBomba = bomba.caudalTeorico_mLmin();

    // Actualización de motores con comprobación teórica de desplazamiento positivo
    sensorAlimentacion.actualizar(dt, bombaEmpuja, qBomba);
    sensorPermeado.actualizar(dt, bombaEmpuja, qBomba);

    float qAlim = sensorAlimentacion.caudal_mLmin();
    float qPerm = sensorPermeado.caudal_mLmin();

    // Sanity check de cruce físico
    flagCruceSensores = (qPerm > qAlim + 50.0f) && (bomba.rpmActual() > 5.0f);

    qRet_mLmin   = fmaxf(0.0f, qAlim - qPerm);
    recuperacion = (qAlim > 50.0f) ? ((qPerm / qAlim) * 100.0f) : 0.0f;
    jLMH_actual  = (qPerm * 0.06f) / AREA_MEMBRANA_M2;

    // En reposo hasta transductores Subhito 2.3
    resultadoDarcy = ResultadoDarcy{};
    deltaBomba = (qBomba > 1.0f) ? (((qAlim - qBomba) / qBomba) * 100.0f) : 0.0f;

    // Telemetría con diagnóstico de flancos, pulsos válidos y glitches filtrados
    Serial.printf("[TELEMETRIA] RPM: %4.1f | %s | Q_Alim: %5.1f mL/min (F:%lu, V:%lu, G:%lu) | Q_Perm: %5.1f mL/min (F:%lu, V:%lu, G:%lu) | J: %4.2f LMH | Y: %4.1f%%\n",
                  bomba.rpmActual(), bomba.enRegimenEstable() ? "ESTABLE" : "RAMPA",
                  qAlim, (unsigned long)sensorAlimentacion.flancosBrutos(), (unsigned long)sensorAlimentacion.pulsosValidos(), (unsigned long)sensorAlimentacion.glitchesVentana(),
                  qPerm, (unsigned long)sensorPermeado.flancosBrutos(), (unsigned long)sensorPermeado.pulsosValidos(), (unsigned long)sensorPermeado.glitchesVentana(),
                  jLMH_actual, recuperacion);
  }

  // 3. Muestreo de Datalogger cada 10 segundos
  if (tAhora - tDatalogger >= INTERVALO_LOG_MS) {
    tDatalogger = tAhora;
    if (bomba.enMarcha()) {
      guardarMuestraDatalogger();
    }
  }
}
```

---

## 6. REQUERIMIENTOS Y ENTREGABLES PARA EL AUDITOR

Como auditor metrológico externo, responde detalladamente a los siguientes 5 puntos:

1. **Dictamen Metrológico Formal:**
   * Declara categóricamente: **[APROBADO]**, **[APROBADO CON OBSERVACIONES]** o **[RECHAZADO]** para certificar el cierre del Subhito 2.2 de la Tesis.
2. **Evaluación de la Inmunidad al Ruido EMI bajo Hardware Congelado:**
   * ¿Es técnicamente viable y suficiente el motor de validación de ancho de nivel en `CHANGE` para rechazar el tren electromagnético periódico del chopper del motor NEMA 34 (3200 micropasos) y los rebotes de cruce del filtro RC ($4.7\text{ k}\Omega + 200\text{ nF}$), sin necesidad de desoldar componentes?
3. **Evaluación Metrológica del Datalogger y Modelo de Darcy:**
   * ¿Cumple la ventana de 10 segundos y la supresión de presiones ficticias en Darcy con los estándares de rigor de una tesis de ingeniería?
4. **Validación de Factores K:**
   * Con la erradicación del chattering de umbral, ¿debe Antonella esperar que el factor $K_{\text{alim}}$ real converja nuevamente hacia los $\approx 98\text{ Hz/(L/min)}$ de fábrica, o se mantendrá en $\approx 196\text{ Hz/(L/min)}$ debido a la hidrodinámica del cartucho?
5. **CÓDIGO COMPLETO APROBADO DEL AUDITOR (ENTREGABLE OBLIGATORIO):**
   * Suministra **tu código fuente completo y validado** de `caudalimetro.h`, `caudalimetro.cpp` (y modificaciones si las hubiera para `EN_USO_firmware_planta.ino` o `config.h`), listo para copiar y compilar directamente en Arduino IDE / ESP-IDF, justificando cada optimización realizada.
