# 🧠 PROMPT MAESTRO DE AUDITORÍA TÉCNICA EXTERNA — RONDA 4 (CERTIFICACIÓN FINAL PARA GPT ASTRA)
**Destinatario:** GPT Astra (Auditor Técnico y Científico Senior en Sistemas Embebidos, Metrología y Fenómenos de Transporte)  
**Proyecto:** Planta Piloto de Ultrafiltración por Flujo Cruzado FX100 & Reactor de Coagulación-Sedimentación  
**Institución:** Universidad Nacional de Salta (UNSa), Facultad de Ingeniería — Salta, Argentina  
**Equipo:** Antonella Guitián & Owen Cañizares (Tesistas de Grado), Ing. Enzo (Codirector / Investigador Doctoral), Antigravity AI (Asistente de Arquitectura)  
**Fecha:** Octubre 2026  
**Contexto de Auditoría:** Cierre de Fase de Instrumentación y Aprobación de Protocolo de Banco para el Lunes

---

## 🎯 INSTRUCCIONES DE ROL PARA GPT ASTRA

Actúa como un **Ingeniero Principal de Sistemas Embebidos Industriales (ESP32 / FreeRTOS / Metrología de Sensores)** y simultáneamente como un **Especialista Senior en Procesos de Separación por Membranas (Ultrafiltración Capilar / Ley de Darcy)**.

Esta es la **cuarta y última ronda de auditoría técnica**. Dado que eres una sesión fresca e independiente, este documento contiene el **100% del contexto físico, matemático, hidrodinámico y de código fuente** de nuestra planta piloto.

En las rondas previas participaron DeepSeek R1, GPT-o3-mini, Gemini Pro, Claude Sonnet 5, GLM-5.3 y Claude 5.5 Opus (quien emitió un dictamen de *Aprobado Condicionado* con 4 observaciones clave). Hemos consolidado todas las correcciones en el firmware templado `v4`, resolvimos la metrología de los caudalímetros, creamos la planilla automatizada para el banco de pruebas y definimos la instrumentación de presión.

Tu objetivo en esta ronda es:
1. **Auditar el código fuente final de los caudalímetros** (`caudalimetro.h` y `caudalimetro.cpp`), verificando el método de período recíproco en microsegundos, la cota física superior continua y la inmunidad al desbordamiento (*rollover*).
2. **Validar la aclaración metrológica de la constante $K$** ($[\text{Hz}/(\text{L/min})]$ vs $[\text{pul/L}]$) y su relación con el volumen integrado en Litros.
3. **Evaluar el sensor de presión seleccionado**: Transductor piezorresistivo de acero inoxidable de **$30\text{ PSI}$ ($0\text{ a }2.07\text{ bar}$)**, salida ratiométrica de **$0.5\text{ a }4.5\text{ V}$** conectado a un conversor ADC **ADS1115 de 16 bits** por bus $I^2C$.
4. **Validar el protocolo experimental para el banco de pruebas** (Ensayos 2 y 3 con probeta de $1000\text{ mL}$ durante $1\text{ minuto}$, y la postergación técnica del Ensayo 4 de permeado hasta montar los sensores de presión).
5. **Emitir el Dictamen Final de Certificación**: ¿El sistema está 100% blindado para que Owen y Antonella enciendan la bomba y calibren los instrumentos en el laboratorio?

---

# 1. FICHA TÉCNICA DEL HARDWARE Y BANCO PILOTO

```
                                  CIRCUITO HIDRÁULICO DEL BANCO
  
   [Tanque Alimentación] ──► [Bomba Peristáltica] ──► [Transductor P1] ──► [Membrana FX100] ──┬─► [Transductor P2] ──► [Válvula Aguja] ──► [Retorno]
      (Agua Turbia /            (NEMA 34 + DM860)       (30 psi / 0.5-4.5V)   (Lumen 13500 fib) │   (30 psi / 0.5-4.5V)  (Regula TMP)
       Sobrenadante)                   │                                                        │
                                [Válvula Alivio]                                                └─► [Carcasa Permeado] ──► [Sensor Permeado] ──► [Permeado /
                                (Tarada 0.7 bar)                                                                            (YF-S401 K=55.0)     Balanza]
```

1. **Impulsión y Cinemática:**
   - **Motor:** Paso a paso NEMA 34 (Torque de retención: **$4.0\text{ N}\cdot\text{m}$**, corriente nominal: $4.0\text{ A}$).
   - **Driver:** Leadshine DM860 configurado a **3200 micropasos/rev** (1/16 micropasos en configuración de Cátodo Común). Frecuencia de pulsos generada por hardware LEDC del ESP32: de $800\text{ Hz}$ (15 RPM) a $5333\text{ Hz}$ (100 RPM).
   - **Cabezal Peristáltico:** MBP-2000 con **3 rodillos a $120^\circ$**, manguera de silicona APM ($\varnothing_{\text{int}} = 12\text{ mm}$, $\varnothing_{\text{ext}} = 18\text{ mm}$).
   - **Cilindrada Real Calibrada con Probeta:** **$13.60\text{ mL/vuelta}$** ($680.0\text{ mL/min}$ a $50.0\text{ RPM}$).
   - **Rango Operativo:** $15.0\text{ a }100.0\text{ RPM}$ ($\approx 204\text{ a }1360\text{ mL/min}$). Ampliado a 100 RPM específicamente para explorar el diseño factorial $3^2$ en agua limpia para la tesis.
   - **Rampas S-Curve:** Aceleración suave de arranque a $2.0\text{ RPM/s}$ (sin golpe de ariete sobre la membrana); frenado de parada de emergencia en menos de $1.5\text{ s}$ ($45.0\text{ RPM/s}$).

2. **Membrana de Ultrafiltración Capilar:**
   - **Modelo:** Fresenius Medical Care FX100 Helixone® (Polisulfona / PVP hidrofílica).
   - **Área Efectiva ($A_m$):** **$2.2\text{ m}^2$**.
   - **Estructura Interna:** $N \approx 13500\text{ fibras}$ capilares huecas, diámetro interno $d_i = 185\ \mu\text{m}$, grosor de pared $\delta = 35\ \mu\text{m}$, longitud efectiva $L \approx 0.28\text{ m}$.
   - **Límite Mecánico de Presión:** $\text{TMP}_{\text{máx}} = 0.50\text{ bar}$ ($50\text{ kPa} \approx 375\text{ mmHg}$).
   - **Válvula de Alivio Mecánico:** Tarada a **$0.70\text{ bar}$** en la impulsión (evita rotura de fibras ante oclusión accidental del retentado).

3. **Instrumentación y Electrónica (ESP32 NodeMCU-32S, 240 MHz):**
   - **Caudalímetro de Alimentación:** Sensor de Efecto Hall YF-S401 (GPIO 14, Borne 12 Superior, $K_{\text{alim}} = 154.62\text{ Hz/(L/min)}$).
   - **Caudalímetro de Permeado:** Sensor YF-S401 (GPIO 27, Borne 11 Superior, $K_{\text{perm}} = 55.00\text{ Hz/(L/min)}$).
   - **Front-End Analógico Antirruido:** Filtro pasabajos RC físico ($R = 4.7\text{ k}\Omega$, $C = 100\text{ nF}$, $f_c \approx 338\text{ Hz}$) + supresión de microcódigo de $1500\ \mu\text{s}$ (límite físico de plausibilidad: $666\text{ Hz} \approx 4300\text{ mL/min}$).
   - **Transductores de Presión Seleccionados:** Sensores piezorresistivos de acero inoxidable con rango **$0\text{ a }30\text{ PSI}$ ($0\text{ a }2.07\text{ bar}$)**, salida lineal **$0.5\text{ a }4.5\text{ V}$**, conectados a un conversor ADC **ADS1115 de 16 bits** por bus $I^2C$ (GPIO 21 SDA, GPIO 22 SCL).

---

# 2. RESOLUCIÓN DE LOS BLOQUEANTES DE CLAUDE OPUS (RONDA 3)

En la auditoría de cierre de la Ronda 3, Claude 5.5 Opus estableció un veredicto de **Aprobado Condicionado** señalando 4 bloqueantes:
1. *Umbrales de sobrepresión en fondo de escala:* Resuelto. Redujimos el alivio mecánico a $0.70\text{ bar}$, el disparo de parada dura en software a $0.60\text{ bar}$ y el enclavamiento de TMP a $0.45\text{ bar}$.
2. *Enclavamiento a 1 Hz:* Resuelto. Se evalúa en cada ciclo de 50 ms en la máquina de estados.
3. *Caudalímetro de permeado por debajo de rango útil:* Resuelto metrológicamente. Se determinó formalmente que para el informe final de tesis de Antonella y Owen, **el patrón primario para el cálculo de flujo $J$ y $R_m$ es gravimétrico** (recolección sobre balanza digital de precisión $0.1\text{ g}$), quedando el sensor YF-S401 como indicador secundario de tendencia en el SCADA.
4. *No recirculación de permeado:* Resuelto en el protocolo hidráulico. El permeado y el retentado retornan al tanque de alimentación durante los ensayos de caracterización hidráulica para mantener constante la concentración.

---

# 3. EL CÓDIGO FUENTE COMPLETO DEL FIRMWARE v4 (`EN_USO_firmware_planta`)

A continuación se presenta el **código fuente completo e íntegro** de todos los módulos del firmware que corren actualmente en el ESP32 NodeMCU-32S, sin omisiones ni esqueletos:

---

### 3.1. `config.h` (Parámetros Cinemáticos, Membrana FX100 y Asignación de Pines)
```cpp
#pragma once
// ==============================================================================
// CONFIGURACIÓN TÉCNICA — PLANTA DE ULTRAFILTRACIÓN (UNSa)
// Parámetros Cinemáticos, Calibración y Asignación de Pines
// ==============================================================================

#include <Arduino.h>

// ------------------------------------------------------------------------------
// 1. ASIGNACIÓN DE PINES (PINOUT)
// ------------------------------------------------------------------------------
// Driver DM860 en CÁTODO COMÚN: GPIO envía HIGH -> Optoacoplador ON
constexpr uint8_t PIN_PUL                = 18;  // DM860 PUL+ (PUL- a GND común)
constexpr uint8_t PIN_DIR                = 19;  // DM860 DIR+ (DIR- a GND común)
constexpr uint8_t PIN_LED_BOMBA          = 2;   // LED Azul Onboard (indicador de marcha)

// Caudalímetros de Efecto Hall YF-S401
constexpr uint8_t PIN_SENSOR_ALIMENTACION = 14;  // Sensor de Alimentación (Borne 12 Sup)
constexpr uint8_t PIN_SENSOR_PERMEADO     = 27;  // Sensor de Permeado (Borne 11 Sup, Pull-up 4.7k a 3.3V)

// ------------------------------------------------------------------------------
// 2. PARÁMETROS CINEMÁTICOS DE LA BOMBA (MOTOR NEMA 34 / DM860)
// ------------------------------------------------------------------------------
constexpr uint16_t PULSOS_POR_REV = 3200;     // DM860 configurado a 1/16 micropasos (3200 pulsos/rev)
constexpr float ML_POR_VUELTA     = 13.6000f; // Calibrado: 680.0 mL/min / 50.0 RPM = 13.6000 mL/rev

// Rango de Operación en RPM
constexpr float RPM_MIN           = 15.0f;    // ~204 mL/min
constexpr float RPM_MAX           = 100.0f;   // ~1360 mL/min (Habilitado para diseño factorial experimental en agua)
constexpr float RPM_INICIO        = 25.0f;    // Consigna de arranque suave (~340 mL/min)
constexpr float RPM_ALARMA_MEMBRANA = 44.0f;  // 44.12 RPM = 600 mL/min (Umbral clínico hemodiálisis / referencia)

// ------------------------------------------------------------------------------
// 3. ESPECIFICACIONES DE LA MEMBRANA FRESENIUS FX100 (HELIXONE®)
// ------------------------------------------------------------------------------
constexpr float AREA_MEMBRANA_M2       = 2.2f;    // Superficie interfacial efectiva (m²)
constexpr float K_UF_NOMINAL           = 73.0f;   // mL / (h * mmHg)
constexpr float DIAMETRO_CAPILAR_UM    = 185.0f;  // Diámetro interno capilar (μm)
constexpr float ESPESOR_PARED_UM       = 35.0f;   // Grosor de pared capilar (μm)
constexpr float TMP_MAX_SEGURA_BAR     = 0.50f;   // Límite máximo admisible de TMP (bar)
constexpr float Q_CLINICO_SANGRE_MAX   = 600.0f;  // mL/min (límite en hemodiálisis clínica)

// Rampas: Arranque Suave Confiable (LEDC Seguro) y Frenado Rápido (< 1.5s)
constexpr float ACEL_ARRANQUE_RPM_S  = 2.0f;   // 2.0 RPM/s despegue inicial suave y confiable
constexpr float ACEL_NOMINAL_RPM_S   = 3.5f;   // 3.5 RPM/s aceleración progresiva
constexpr float DESACEL_AJUSTE_RPM_S = 8.0f;   // 8.0 RPM/s desaceleración en marcha
constexpr float FRENADO_PARADA_RPM_S = 45.0f;  // 45.0 RPM/s frenado rápido al presionar STOP (< 1.5s)

// ------------------------------------------------------------------------------
// 4. CALIBRACIÓN DE FÁBRICA DE CAUDALÍMETROS YF-S401
// ------------------------------------------------------------------------------
// UNIDAD METROLÓGICA DE K: [Hz / (L/min)]
// Relación matemática fundamental:
//   F [Hz] = K * Q [L/min]  ===>  Q [mL/min] = (F [Hz] * 1000) / K
//   Pulsos por Litro = K * 60
// Nominal de fabricante YF-S401: F = 98 * Q (L/min) => K = 98.0 Hz/(L/min) (5880 pul/L)
// Calibración experimental con probeta (Owen a 50 y 72 RPM):
constexpr float K_ALIMENTACION = 154.62f; // Hz/(L/min) -> 9277.2 pulsos/L (105.14 Hz a 680.0 mL/min)
constexpr float K_PERMEADO     = 55.00f;  // Hz/(L/min) -> 3300.0 pulsos/L (5.50 Hz a 100.0 mL/min)

constexpr uint32_t FILTRO_RUIDO_US = 1500;    // 1.5 ms de blanking anti-rebote (hasta 666 Hz / ~4300 mL/min)
constexpr float Q_MAX_FISICO_MLMIN = 6000.0f; // Límite de corte físico (6.0 L/min)

// ------------------------------------------------------------------------------
// 4. PARÁMETROS DEL DATALOGGER MULTI-SESIÓN Y AUTO-CALIBRACIÓN
// ------------------------------------------------------------------------------
constexpr uint32_t INTERVALO_LOG_MS   = 10000;  // Muestreo y almacenamiento cada 10 segundos
constexpr size_t   MAX_REGISTROS      = 600;    // 600 muestras = 1.66 horas de ensayo continuo
constexpr size_t   MAX_ENSAYOS        = 20;     // Hasta 20 corridas/ensayos registrados en memoria
constexpr uint8_t  MUESTRAS_AUTO_CAL  = 15;     // 15 muestras (15 seg) para auto-calibración en régimen permanente

// ------------------------------------------------------------------------------
// 5. CONFIGURACIÓN DE RED WI-FI Y SCADA
// ------------------------------------------------------------------------------
constexpr const char* SSID_AP  = "Bomba_Peristaltica_UF";
constexpr const char* PASS_AP  = "plantapiloto2";
constexpr const char* SSID_STA = "Box804";
constexpr const char* PASS_STA = "plantapiloto2";

```

---

### 3.2. `caudalimetro.h` (Declaración de la Clase Caudalímetro YF-S401)
```cpp
#pragma once
#include <Arduino.h>
#include "config.h"

// ==============================================================================
// CLASE CAUDALÍMETRO (Sensor de Efecto Hall YF-S401)
// - Lectura por interrupción en memoria IRAM (IRAM_ATTR) con paso de puntero
// - Blanking anti-rebote (FILTRO_RUIDO_US = 1500 us) + Filtro RC analógico
// - Medición por período recíproco de alta resolución (inmune a cuantización ±1 pulso)
// - Cota física superior continua en ausencia de pulsos (decaimiento asintótico suave a cero)
// - Sincronización atómica multinúcleo con spinlock FreeRTOS (portMUX_TYPE)
// - Calibración en tiempo real (setK / getK) en unidades [Hz / (L/min)]
// - Diagnóstico de señal asimétrico (Alarma solo en Alimentación)
// ==============================================================================

class Caudalimetro {
public:
  Caudalimetro(uint8_t pin, float k, const char* nombre, bool esAlimentacion = true);

  void begin();
  void actualizar(float dt_s, bool bombaEmpuja);

  float caudal_mLmin()   const { return _q; }
  float caudal_Lmin()    const { return _q / 1000.0f; }
  float frecuencia_Hz()  const { return _f; }
  float volumen_L()      const { return _vol; }
  bool  sinSenal()       const { return _fallo; }
  void  resetVolumen()         { _vol = 0.0f; }
  const char* nombre()   const { return _nombre; }
  bool  esAlimentacion() const { return _esAlimentacion; }

  // Calibración dinámica sin recompilar [Hz / (L/min)]
  void  setK(float nuevoK)     { if (nuevoK > 0.1f) _k = nuevoK; }
  float getK()           const { return _k; }

private:
  static void IRAM_ATTR isrPuente(void* arg);

  const uint8_t _pin;
  float         _k;
  const char*   _nombre;
  const bool    _esAlimentacion;

  volatile uint32_t _pulsos = 0;
  volatile uint32_t _t_ultimo = 0;
  volatile uint32_t _t_primero = 0;
  volatile uint32_t _periodo_us = 0;

  float _f = 0.0f;
  float _q = 0.0f;
  float _vol = 0.0f;
  float _tiempoSinPulso_s = 0.0f;
  bool  _fallo = false;

  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
};

```

---

### 3.3. `caudalimetro.cpp` (Implementación: Período Recíproco, Cota Física y Sección Crítica)
```cpp
#include "caudalimetro.h"

Caudalimetro::Caudalimetro(uint8_t pin, float k, const char* nombre, bool esAlimentacion)
  : _pin(pin), _k(k), _nombre(nombre), _esAlimentacion(esAlimentacion) {}

void Caudalimetro::begin() {
  pinMode(_pin, INPUT_PULLUP);
  attachInterruptArg(digitalPinToInterrupt(_pin), isrPuente, this, FALLING);
}

void Caudalimetro::actualizar(float dt_s, bool bombaEmpuja) {
  // Captura atómica de variables acumuladas por la ISR en la ventana transcurrida
  portENTER_CRITICAL(&_mux);
  uint32_t n      = _pulsos;
  _pulsos         = 0;
  uint32_t per_us = _periodo_us;
  uint32_t t_prim = _t_primero;
  uint32_t t_ult  = _t_ultimo;
  portEXIT_CRITICAL(&_mux);

  uint32_t tAhora = micros();
  // Diferencia sin signo uint32_t: matemáticamente inmune al desbordamiento (rollover de 71.58 min)
  uint32_t tSinFlanco = tAhora - t_ult;

  // 1. CÁLCULO DE FRECUENCIA CON MÉTODO DE PERÍODO RECÍPROCO DE ALTA RESOLUCIÓN:
  // - n >= 2 pulsos: medimos el tiempo exacto entre el 1er y último pulso dentro de la ventana.
  //   Resolución en microsegundos; elimina por completo el error de discretización ±1 pulso.
  // - n == 1 pulso: la ventana solo capturó un flanco, se usa el período entre pulsos sucesivos per_us.
  // - n == 0 pulsos: no hubo eventos en la ventana (régimen bajo o detención).
  if (n >= 2 && (t_ult - t_prim) > 0) {
    _f = ((float)(n - 1) * 1000000.0f) / (float)(t_ult - t_prim);
  } else if (n == 1 && per_us > 0) {
    _f = 1000000.0f / (float)per_us;
  }

  // 2. COTA FÍSICA SUPERIOR CONTINUA EN AUSENCIA DE PULSOS RECIENTES:
  // Si transcurrió tSinFlanco microsegundos desde el último pulso registrado,
  // la física impone que la frecuencia instantánea real no puede exceder 1e6 / tSinFlanco.
  // Esto garantiza un decaimiento suave y asintótico hacia cero cuando la bomba frena,
  // impidiendo que la frecuencia quede congelada artificialmente.
  if (tSinFlanco > 0) {
    float f_max_posible = 1000000.0f / (float)tSinFlanco;
    if (_f > f_max_posible) {
      _f = f_max_posible;
    }
  }

  // Decaimiento estricto a cero absoluto tras 3.0 segundos sin pulsos
  if (n == 0 && tSinFlanco > 3000000UL) {
    _f = 0.0f;
  }

  // 3. CÁLCULO DE CAUDAL INSTANTÁNEO EN mL/min:
  // Por definición metrológica: K está en [Hz / (L/min)].
  // Q [L/min] = F / K  ===>  Q [mL/min] = (F * 1000.0) / K
  float q = (_f * 1000.0f) / _k;

  // Filtro de plausibilidad física (corte de picos transitorios por perturbación EMI)
  if (q > Q_MAX_FISICO_MLMIN) {
    q = 0.0f;
    _f = 0.0f;
    Serial.printf("[%s] Ruido descartado: %lu pulsos espurios\n", _nombre, (unsigned long)n);
  } else {
    // 4. INTEGRACIÓN DE VOLUMEN TOTALIZADO EN LITROS:
    // 1 L/min = (1/60) L/s  ==>  Pulsos por Litro = K * 60
    // Vol [L] = pulsos / (K * 60)
    // Solo se acumulan pulsos físicamente plausibles.
    _vol += (float)n / (_k * 60.0f);
  }

  // 5. FILTRO EXPONENCIAL PONDERADO (EMA):
  // Atenúa el rizo de presión y caudal producido por los 3 rodillos del cabezal peristáltico.
  // Constante de tiempo aproximada: tau ~ 2 segundos.
  if (n > 0 || _f > 0.05f) {
    _q = 0.4f * q + 0.6f * _q;
  } else {
    _q = 0.0f;
  }

  // 6. DIAGNÓSTICO ASIMÉTRICO DE PÉRDIDA DE SEÑAL / CABLE CORTADO:
  // - Alimentación: si la bomba empuja (RPM > 1) y pasan 5 segundos sin pulsos -> FALLA CRÍTICA.
  // - Permeado: caudal nulo es una condición admisible (válvula cerrada o baja TMP) -> NO alarma.
  if (n > 0 || _f > 0.05f) {
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

void IRAM_ATTR Caudalimetro::isrPuente(void* arg) {
  Caudalimetro* c = reinterpret_cast<Caudalimetro*>(arg);
  uint32_t t = micros();

  portENTER_CRITICAL_ISR(&c->_mux);
  // Diferencia sin signo uint32_t: segura ante desbordamiento de micros() cada 71.58 min
  uint32_t dt = t - c->_t_ultimo;

  // Blanking anti-rebote: descarta transitorios mecánicos y capacitivos (FILTRO_RUIDO_US = 1500 us)
  if (dt >= FILTRO_RUIDO_US) {
    if (c->_pulsos == 0) {
      c->_t_primero = t;
    }
    // Si el tiempo transcurrido es menor a 5 segundos, registramos el período inter-pulso real
    if (dt < 5000000UL) {
      c->_periodo_us = dt;
    } else {
      c->_periodo_us = 0; // Tras una parada prolongada, se descarta el período espurio
    }
    c->_t_ultimo = t;
    c->_pulsos++;
  }
  portEXIT_CRITICAL_ISR(&c->_mux);
}

```

---

### 3.4. `Bomba.h` (Declaración del Control Cinemático del Motor NEMA 34 / DM860)
```cpp
#pragma once
#include <Arduino.h>
#include <cmath>
#include "config.h"

// ==============================================================================
// CLASE BOMBA PERISTÁLTICA — DECLARACIÓN
// - Control de frecuencia por hardware LEDC PWM (Driver DM860)
// - Rampa Cuadrática S-Curve de arranque suave (sin golpe de torque)
// - Parada rápida en < 1.5s
// ==============================================================================

class Bomba {
public:
  void begin();
  void arrancar();
  void detener();
  bool setRPM(float rpm);
  void toggleSentido();

  float rpmActual() const           { return _actual; }
  float rpmObjetivo() const         { return _objetivo; }
  bool  enMarcha() const            { return _enMarcha; }
  bool  invirtiendo() const         { return _invirtiendo; }
  bool  sentidoHorario() const      { return _horario; }
  bool  enRegimenEstable() const    { return (_enMarcha && fabsf(_actual - _objetivo) < 0.3f && _actual > 5.0f); }
  float caudalTeorico_mLmin() const { return _actual * _mlPorVuelta; }

  void  setMlPorVuelta(float ml)    { if (ml > 0.1f) _mlPorVuelta = ml; }
  float getMlPorVuelta() const      { return _mlPorVuelta; }

  void     setPulsosPorRev(uint16_t p) { if (p >= 200) _pulsosPorRev = p; }
  uint16_t getPulsosPorRev() const     { return _pulsosPorRev; }

  void tick(float dt);

private:
  void fijarSentido(bool horario);

  bool _enMarcha = false;
  bool _horario = true;
  bool _invirtiendo = false;

  float _objetivo = RPM_INICIO;
  float _actual = 0.0f;
  float _rpmGuardada = RPM_INICIO;
  uint32_t _fActual = 0;

  float    _mlPorVuelta  = ML_POR_VUELTA;
  uint16_t _pulsosPorRev = PULSOS_POR_REV;
};

```

---

### 3.5. `Bomba.cpp` (Implementación: Rampa S-Curve, PWM Hardware LEDC y Cruce Seguro por Cero)
```cpp
#include "Bomba.h"

// ==============================================================================
// IMPLEMENTACIÓN DE LA CLASE BOMBA
// Generación PWM por hardware LEDC con frecuencia mínima segura (>= 50 Hz)
// ==============================================================================

void Bomba::begin() {
  pinMode(PIN_DIR, OUTPUT);
  fijarSentido(true); // Sentido horario por defecto

#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
  ledcAttach(PIN_PUL, 800, 10);
  ledcWrite(PIN_PUL, 0); // Reposo: LOW -> Opto OFF (motor libre)
#else
  ledcSetup(0, 800, 10);
  ledcAttachPin(PIN_PUL, 0);
  ledcWrite(0, 0);
#endif
}

void Bomba::arrancar() {
  _enMarcha = true;
  if (_objetivo < RPM_MIN) {
    _objetivo = (_rpmGuardada >= RPM_MIN) ? _rpmGuardada : RPM_INICIO;
  }
}

void Bomba::detener() {
  _enMarcha = false;
  _invirtiendo = false;
  if (_objetivo >= RPM_MIN) {
    _rpmGuardada = _objetivo;
  }
}

bool Bomba::setRPM(float rpm) {
  if (!std::isfinite(rpm)) return false;
  float r = constrain(rpm, RPM_MIN, RPM_MAX);
  if (_invirtiendo) {
    _rpmGuardada = r;   // Almacena consigna si el usuario mueve slider durante el frenado de inversión
  } else {
    _objetivo = r;
    _rpmGuardada = r;
  }
  return true;
}

void Bomba::toggleSentido() {
  if (_invirtiendo) return;
  if (_actual < 1.0f) {
    fijarSentido(!_horario);
  } else {
    _invirtiendo = true;
    _rpmGuardada = (_objetivo >= RPM_MIN) ? _objetivo : RPM_INICIO;
    _objetivo = 0.0f;
  }
}

void Bomba::fijarSentido(bool horario) {
  _horario = horario;
  digitalWrite(PIN_DIR, horario ? LOW : HIGH);
}

void Bomba::tick(float dt) {
  float objetivo = _enMarcha ? _objetivo : 0.0f;

  if (_actual < objetivo) {
    // Si la bomba está arrancando desde 0, iniciamos suavemente en 1.0 RPM (53 Hz LEDC)
    if (_actual < 1.0f) {
      _actual = 1.0f;
    }
    
    // Rampa de aceleración progresiva
    float tasaAcel = ACEL_NOMINAL_RPM_S;
    if (_actual < 10.0f) {
      tasaAcel = ACEL_ARRANQUE_RPM_S + (_actual / 10.0f) * (ACEL_NOMINAL_RPM_S - ACEL_ARRANQUE_RPM_S);
    }
    float delta = objetivo - _actual;
    if (delta < 3.0f) {
      tasaAcel = fmaxf(0.8f, tasaAcel * (delta / 3.0f));
    }
    _actual = fminf(objetivo, _actual + tasaAcel * dt);

  } else if (_actual > objetivo) {
    // Desaceleración / Frenado
    if (!_enMarcha) {
      // PARADA RÁPIDA: Frenado ágil en menos de 1.5s
      _actual = fmaxf(0.0f, _actual - FRENADO_PARADA_RPM_S * dt);
      if (_actual < 1.0f) {
        _actual = 0.0f;
      }
    } else {
      // Reducción suave de velocidad en marcha
      float tasaDecel = DESACEL_AJUSTE_RPM_S;
      float delta = _actual - objetivo;
      if (delta < 3.0f) {
        tasaDecel = fmaxf(1.0f, tasaDecel * (delta / 3.0f));
      }
      _actual = fmaxf(objetivo, _actual - tasaDecel * dt);
    }
  }

  // Al llegar a 0 RPM durante una inversión: cortar pulsos, conmutar DIR con margen de seguridad y re-acelerar
  if (_invirtiendo && _actual <= 0.1f) {
#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    ledcWrite(PIN_PUL, 0);
#else
    ledcWrite(0, 0);
#endif
    _fActual = 0;
    fijarSentido(!_horario);
    _invirtiendo = false;
    _objetivo = (_rpmGuardada >= RPM_MIN) ? _rpmGuardada : RPM_INICIO;
  }

  // Generación de pulsos PWM por hardware LEDC
  if (_actual >= 1.0f) {
    uint32_t f = (uint32_t)(_actual * (float)_pulsosPorRev / 60.0f);
    if (f < 50) f = 50; // Límite de seguridad mínimo del temporizador LEDC de ESP32

#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    if (f != _fActual) { 
      ledcChangeFrequency(PIN_PUL, f, 10); 
      _fActual = f; 
    }
    ledcWrite(PIN_PUL, 512); // Ciclo de trabajo 50%
#else
    if (f != _fActual) { 
      ledcSetup(0, f, 10); 
      _fActual = f; 
    }
    ledcWrite(0, 512);
#endif
  } else if (_fActual != 0) {
    // Corte inmediato de pulsos al detenerse
#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    ledcWrite(PIN_PUL, 0); // Reposo: LOW -> Opto OFF
#else
    ledcWrite(0, 0);
#endif
    _fActual = 0;
  }
}

```

---

### 3.6. `darcy.h` (Modelo de Transporte Darcy, Viscosidad de Vogel y Resistencias)
```cpp
#pragma once
// ==============================================================================
// MODELO DE TRANSPORTE Y RESISTENCIAS EN SERIE (LEY DE DARCY) — MEMBRANA FX100
// Diseñado para la Tesis de Grado (Antonella Guitián & Owen Cañizares / Codir. Ing. Enzo)
// ==============================================================================

#include <Arduino.h>
#include <cmath>
#include "config.h"

struct ResultadoDarcy {
  float J_LMH;       // Flujo de permeado volumétrico específico [L / (m² · h)]
  float J20_LMH;     // Flujo normalizado a 20 °C [LMH]
  float mu_Pas;      // Viscosidad dinámica del agua a temperatura T [Pa · s]
  float TCF;         // Factor de corrección por temperatura (mu(T) / mu_20)
  float R_total;     // Resistencia hidráulica total [m^-1]
  float R_torta;     // Resistencia por capa de torta / ensuciamiento [m^-1]
  float TMP_bar;     // Presión transmembrana efectiva [bar]
  bool  valido;      // Estado de cálculo válido
};

class ModeloDarcy {
public:
  static constexpr float MU20 = 1.002e-3f; // Pa · s (Agua destilada a 20.0 °C)

  // Viscosidad dinámica del agua mediante ecuación de Vogel (válida 5 a 60 °C)
  static float viscosidadAgua(float temp_C) {
    const float T_K = temp_C + 273.15f;
    return 2.414e-5f * powf(10.0f, 247.8f / (T_K - 140.0f));
  }

  void  setRm(float rm) { if (rm > 1e11f && rm < 1e16f) _Rm = rm; }
  float Rm() const      { return _Rm; }

  // Cálculo hidráulico en tiempo real
  ResultadoDarcy calcular(float qPerm_mLmin, float tmp_bar, float temp_C = 20.0f) const {
    ResultadoDarcy r{};
    if (!std::isfinite(tmp_bar) || !std::isfinite(temp_C) || !std::isfinite(qPerm_mLmin)) {
      return r;
    }
    if (tmp_bar < 0.01f || qPerm_mLmin < 0.5f) {
      return r;
    }

    r.TMP_bar = tmp_bar;
    r.mu_Pas  = viscosidadAgua(temp_C);
    r.TCF     = r.mu_Pas / MU20;

    // J [LMH] = Q [L/h] / Area [m²] = (qPerm [mL/min] * 0.06) / 2.2 m²
    r.J_LMH   = (qPerm_mLmin * 0.06f) / AREA_MEMBRANA_M2;
    r.J20_LMH = r.J_LMH * r.TCF;

    // J en unidades SI [m/s]: 1 LMH = 1 / 3.6e6 m/s
    const float J_SI   = r.J_LMH / 3.6e6f;
    const float TMP_Pa = tmp_bar * 100000.0f;

    // Ley de Darcy: J = TMP / (mu * R_total) => R_total = TMP / (mu * J)
    r.R_total = TMP_Pa / (r.mu_Pas * J_SI);
    r.R_torta = r.R_total - _Rm;
    r.valido  = true;

    return r;
  }

  // Calibración experimental de Rm con agua limpia (regresión lineal J vs TMP sin ordenada al origen)
  void acumularPuntoAguaLimpia(float J_SI, float TMP_Pa) {
    _sumJP += J_SI * TMP_Pa;
    _sumPP += TMP_Pa * TMP_Pa;
  }

  bool finalizarCalibracionRm(float temp_C = 20.0f) {
    if (_sumPP <= 0.0f || _sumJP <= 0.0f) return false;
    const float Lp = _sumJP / _sumPP; // Permeabilidad hidráulica en m / (s · Pa)
    const float mu = viscosidadAgua(temp_C);
    _Rm = 1.0f / (mu * Lp);
    _sumJP = 0.0f;
    _sumPP = 0.0f;
    return true;
  }

private:
  // Resistencia intrínseca nominal de la membrana FX100 Helixone® (K_UF = 73 mL/h*mmHg, 2.2 m²)
  float _Rm = 1.44e13f; 
  float _sumJP = 0.0f;
  float _sumPP = 0.0f;
};

```

---

### 3.7. `EN_USO_firmware_planta.ino` (Orquestador Principal, Lazos Temporizados, Datalogger y Servidor Web)
*(Nota: El archivo `index_html.cpp` contiene la interfaz web HTML5/CSS/JS de 740 líneas almacenada en Flash `PROGMEM`)*
```cpp
/* ==============================================================================
 * PLANTA PILOTO DE ULTRAFILTRACIÓN — TESIS INGENIERÍA INDUSTRIAL (UNSa 2026)
 * Firmware de Control, Adquisición, Modo Desarrollador, Auto-Calibración y Datalogger
 * Arquitectura C++ Optimizada y Wi-Fi SoftAP de Alta Estabilidad (Anti-Desconexión)
 * ============================================================================== */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "config.h"
#include "caudalimetro.h"
#include "Bomba.h"
#include "darcy.h"
#include "index_html.h"

// ------------------------------------------------------------------------------
// ESTRUCTURAS DE DATOS PARA EL DATALOGGER Y GESTIÓN DE ENSAYOS
// ------------------------------------------------------------------------------
struct RegistroCalibracion {
  uint8_t  id_ensayo;
  uint32_t t_relativo_s;
  float    rpm;
  float    q_bomba;
  float    f_alim;
  float    q_alim;
  float    vol_alim;
  float    f_perm;
  float    q_perm;
  float    vol_perm;
  float    q_ret;
  float    recov;
  float    delta;
  float    k_alim;
  float    k_perm;
  float    j_lmh;
};

struct EnsayoInfo {
  uint8_t  id;
  float    rpm_consigna;
  uint32_t t_inicio_ms;
  uint32_t duracion_s;
  uint16_t muestras;
  float    vol_alim;
  float    vol_perm;
};

// Buffers de almacenamiento en RAM
RegistroCalibracion bufferLog[MAX_REGISTROS];
size_t numRegistros = 0;

EnsayoInfo listaEnsayos[MAX_ENSAYOS];
size_t numEnsayos = 0;
uint8_t ensayoActualId = 1;

// Variables de estado del ensayo actual
uint32_t tInicioEnsayo_ms = 0;
float volAlimInicioEnsayo = 0.0f;
float volPermInicioEnsayo = 0.0f;
bool  bombaEnMarchaAnterior = false;

// Variables de Auto-Calibración en Marcha
bool    autoCalibrando = false;
uint8_t autoCalMuestras = 0;
float   autoCalSumFrecAlim = 0.0f;
float   autoCalSumFrecPerm = 0.0f;
String  autoCalMensaje = "";

// Instanciación de componentes (Alimentación con alarma de corte; Permeado sin alarma en reposo)
Caudalimetro sensorAlimentacion(PIN_SENSOR_ALIMENTACION, K_ALIMENTACION, "ALIMENTACION", true);
Caudalimetro sensorPermeado(PIN_SENSOR_PERMEADO, K_PERMEADO, "PERMEADO", false);
Bomba bomba;
WebServer server(80);
Preferences prefs;

// Variables hidráulicas y de control
float qRet_mLmin = 0.0f;
float recuperacion = 0.0f;
float deltaBomba = 0.0f;
float jLMH_actual = 0.0f;
bool  flagCruceSensores = false;

ModeloDarcy modeloDarcy;
ResultadoDarcy resultadoDarcy;

uint32_t tLoop = 0;
uint32_t tCaudal = 0;
uint32_t tDatalogger = 0;

// ------------------------------------------------------------------------------
// GESTIÓN DE PREFERENCES (MEMORIA FLASH NVS)
// ------------------------------------------------------------------------------
void cargarParametrosNVS() {
  prefs.begin("planta_uf", false);
  float ka = prefs.getFloat("k_alim", K_ALIMENTACION);
  float kp = prefs.getFloat("k_perm", K_PERMEADO);
  float ml = prefs.getFloat("ml_rev", ML_POR_VUELTA);
  uint16_t pul = prefs.getUShort("pul_rev", PULSOS_POR_REV);

  sensorAlimentacion.setK(ka);
  sensorPermeado.setK(kp);
  bomba.setMlPorVuelta(ml);
  bomba.setPulsosPorRev(pul);

  Serial.printf("\n[NVS] Parametros cargados: K_Alim=%.2f | K_Perm=%.2f | mL/rev=%.4f | Pul/Rev=%u\n",
                ka, kp, ml, pul);
}

void guardarParametrosNVS(float ka, float kp, float ml, uint16_t pul) {
  prefs.putFloat("k_alim", ka);
  prefs.putFloat("k_perm", kp);
  prefs.putFloat("ml_rev", ml);
  prefs.putUShort("pul_rev", pul);
  Serial.println("[NVS] Parametros guardados en memoria Flash con exito.");
}

// ------------------------------------------------------------------------------
// FUNCIÓN PARA GUARDAR MUESTRA EN EL DATALOGGER (CADA 10 SEGUNDOS)
// ------------------------------------------------------------------------------
void guardarMuestraDatalogger() {
  if (tInicioEnsayo_ms == 0) {
    tInicioEnsayo_ms = millis();
  }

  uint32_t t_rel_s = (millis() - tInicioEnsayo_ms) / 1000;

  // Desplazamiento FIFO circular si el buffer se llena
  if (numRegistros >= MAX_REGISTROS) {
    for (size_t i = 0; i < MAX_REGISTROS - 1; i++) {
      bufferLog[i] = bufferLog[i + 1];
    }
    numRegistros = MAX_REGISTROS - 1;
  }

  RegistroCalibracion reg;
  reg.id_ensayo    = ensayoActualId;
  reg.t_relativo_s = t_rel_s;
  reg.rpm          = bomba.rpmActual();
  reg.q_bomba      = bomba.caudalTeorico_mLmin();
  reg.f_alim       = sensorAlimentacion.frecuencia_Hz();
  reg.q_alim       = sensorAlimentacion.caudal_mLmin();
  reg.vol_alim     = sensorAlimentacion.volumen_L();
  reg.f_perm       = sensorPermeado.frecuencia_Hz();
  reg.q_perm       = sensorPermeado.caudal_mLmin();
  reg.vol_perm     = sensorPermeado.volumen_L();
  reg.q_ret        = qRet_mLmin;
  reg.recov        = recuperacion;
  reg.delta        = deltaBomba;
  reg.k_alim       = sensorAlimentacion.getK();
  reg.k_perm       = sensorPermeado.getK();
  reg.j_lmh        = (reg.q_perm * 0.06f) / AREA_MEMBRANA_M2;

  bufferLog[numRegistros++] = reg;

  Serial.printf("[LOG #%u][Ensayo %u] t=%us | RPM=%.1f | Q_Alim=%.1f mL/min | Q_Perm=%.1f mL/min | J=%.2f LMH | Y=%.1f%%\n",
                (unsigned int)numRegistros, ensayoActualId, t_rel_s, reg.rpm, reg.q_alim, reg.q_perm, reg.j_lmh, reg.recov);
}

// ------------------------------------------------------------------------------
// MANEJADORES DE RUTAS DEL SERVIDOR WEB
// ------------------------------------------------------------------------------
void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleStatus() {
  uint32_t t_act_s = (tInicioEnsayo_ms > 0) ? ((millis() - tInicioEnsayo_ms) / 1000) : 0;
  uint8_t prog = (uint8_t)((autoCalMuestras * 100) / MUESTRAS_AUTO_CAL);
  if (prog > 100) prog = 100;

  float qAlim = sensorAlimentacion.caudal_mLmin();
  float qPerm = sensorPermeado.caudal_mLmin();
  jLMH_actual = (qPerm * 0.06f) / AREA_MEMBRANA_M2;

  char buf[768];
  int n = snprintf(buf, sizeof(buf),
    "{"
    "\"rpm\":%.1f,\"obj_rpm\":%.1f,\"on\":%s,\"inv\":%s,\"dir\":%s,\"en_regimen\":%s,"
    "\"ip\":\"%s\",\"alim_ok\":%s,\"perm_ok\":%s,\"f_alim\":%.2f,\"q_alim\":%.1f,\"vol_alim\":%.4f,"
    "\"f_perm\":%.2f,\"q_perm\":%.1f,\"vol_perm\":%.4f,\"q_ret\":%.1f,\"recov\":%.2f,"
    "\"pump_ml\":%.1f,\"delta\":%.2f,\"j_lmh\":%.2f,\"cruce\":%s,"
    "\"k_alim\":%.2f,\"k_perm\":%.2f,\"ml_rev\":%.4f,\"pul_rev\":%u,"
    "\"auto_cal\":%s,\"auto_cal_prog\":%u,\"auto_cal_res\":\"%s\","
    "\"n_logs\":%u,\"ensayo_act\":%u,\"t_ensayo_s\":%lu,"
    "\"heap\":%u,\"maxblk\":%u,"
    "\"ensayos\":[",
    bomba.rpmActual(), bomba.rpmObjetivo(),
    bomba.enMarcha() ? "true" : "false",
    bomba.invirtiendo() ? "true" : "false",
    bomba.sentidoHorario() ? "true" : "false",
    bomba.enRegimenEstable() ? "true" : "false",
    WiFi.softAPIP().toString().c_str(),
    !sensorAlimentacion.sinSenal() ? "true" : "false",
    !sensorPermeado.sinSenal() ? "true" : "false",
    sensorAlimentacion.frecuencia_Hz(), qAlim, sensorAlimentacion.volumen_L(),
    sensorPermeado.frecuencia_Hz(), qPerm, sensorPermeado.volumen_L(),
    qRet_mLmin, recuperacion,
    bomba.caudalTeorico_mLmin(), deltaBomba, jLMH_actual, flagCruceSensores ? "true" : "false",
    sensorAlimentacion.getK(), sensorPermeado.getK(),
    bomba.getMlPorVuelta(), bomba.getPulsosPorRev(),
    autoCalibrando ? "true" : "false", prog, autoCalMensaje.c_str(),
    (unsigned)numRegistros, (unsigned)ensayoActualId, (unsigned long)t_act_s,
    ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)
  );

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  server.sendContent(buf, n);

  char item[128];
  for (size_t i = 0; i < numEnsayos; i++) {
    int itemLen = snprintf(item, sizeof(item),
      "%s{\"id\":%u,\"rpm\":%.1f,\"duracion_s\":%lu,\"muestras\":%u,\"vol_alim\":%.4f,\"vol_perm\":%.4f}",
      (i > 0) ? "," : "",
      listaEnsayos[i].id, listaEnsayos[i].rpm_consigna,
      (unsigned long)listaEnsayos[i].duracion_s,
      listaEnsayos[i].muestras,
      listaEnsayos[i].vol_alim, listaEnsayos[i].vol_perm
    );
    server.sendContent(item, itemLen);
  }

  server.sendContent("]}");
  server.sendContent(""); // Cierra el streaming chunked
}

void handleCmd() {
  if (!server.hasArg("act")) {
    server.send(400, "text/plain", "Falta argumento act");
    return;
  }
  String act = server.arg("act");
  if (act == "START") {
    bomba.arrancar();
  } else if (act == "STOP") {
    bomba.detener();
    if (autoCalibrando) {
      autoCalibrando = false;
      autoCalMensaje = "Auto-calibracion cancelada al apagar bomba.";
    }
  } else if (act == "DIR") {
    bomba.toggleSentido();
  } else if (act == "RESET_VOL") {
    sensorAlimentacion.resetVolumen();
    sensorPermeado.resetVolumen();
  }
  server.send(200, "text/plain", "OK");
}

void handleSetRPM() {
  if (server.hasArg("rpm")) {
    float rpm = server.arg("rpm").toFloat();
    if (bomba.setRPM(rpm)) {
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "ERROR: RPM fuera de rango o invalido");
    }
  } else {
    server.send(400, "text/plain", "Falta argumento rpm");
  }
}

// Configuración en Modo Desarrollador
void handleSetDev() {
  if (server.hasArg("ka")) {
    float ka = server.arg("ka").toFloat();
    sensorAlimentacion.setK(ka);
  }
  if (server.hasArg("kp")) {
    float kp = server.arg("kp").toFloat();
    sensorPermeado.setK(kp);
  }
  if (server.hasArg("ml")) {
    float ml = server.arg("ml").toFloat();
    bomba.setMlPorVuelta(ml);
  }
  if (server.hasArg("pul")) {
    uint16_t pul = (uint16_t)server.arg("pul").toInt();
    bomba.setPulsosPorRev(pul);
  }

  bool saveNvs = (server.hasArg("save") && server.arg("save") == "1");
  if (saveNvs) {
    guardarParametrosNVS(sensorAlimentacion.getK(), sensorPermeado.getK(),
                         bomba.getMlPorVuelta(), bomba.getPulsosPorRev());
  }

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// Iniciar Auto-Calibración Inteligente en Régimen Permanente
void handleIniciarAutoCal() {
  if (!bomba.enMarcha()) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Enciende la bomba primero\"}");
    return;
  }
  autoCalibrando = true;
  autoCalMuestras = 0;
  autoCalSumFrecAlim = 0.0f;
  autoCalSumFrecPerm = 0.0f;
  autoCalMensaje = "";
  server.send(200, "application/json", "{\"status\":\"iniciada\"}");
}

void handleCancelarAutoCal() {
  autoCalibrando = false;
  autoCalMensaje = "Auto-calibracion cancelada.";
  server.send(200, "application/json", "{\"status\":\"cancelada\"}");
}

// Calibrador Completo por RPM y Caudal de Probeta
void handleCalibrarRpmQ() {
  if (!server.hasArg("rpm") || !server.hasArg("qa") || !server.hasArg("qp")) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Faltan argumentos\"}");
    return;
  }

  float rpm = server.arg("rpm").toFloat();
  float qa  = server.arg("qa").toFloat();
  float qp  = server.arg("qp").toFloat();

  if (rpm <= 0.0f || qa <= 0.0f || qp <= 0.0f) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Valores deben ser mayores a 0\"}");
    return;
  }

  // 1. Cilindrada real de la bomba
  float nuevaCilindrada = qa / rpm;
  bomba.setMlPorVuelta(nuevaCilindrada);

  // 2. Factores K basados en frecuencia actual
  float fa = sensorAlimentacion.frecuencia_Hz();
  float fp = sensorPermeado.frecuencia_Hz();

  if (fa > 1.0f) {
    float nuevoKa = (fa * 1000.0f) / qa;
    sensorAlimentacion.setK(nuevoKa);
  }
  if (fp > 0.3f) {
    float nuevoKp = (fp * 1000.0f) / qp;
    sensorPermeado.setK(nuevoKp);
  }

  guardarParametrosNVS(sensorAlimentacion.getK(), sensorPermeado.getK(),
                       bomba.getMlPorVuelta(), bomba.getPulsosPorRev());

  String json = "{";
  json += "\"status\":\"ok\",";
  json += "\"ml_rev\":" + String(bomba.getMlPorVuelta(), 4) + ",";
  json += "\"k_alim\":" + String(sensorAlimentacion.getK(), 2) + ",";
  json += "\"k_perm\":" + String(sensorPermeado.getK(), 2);
  json += "}";

  server.send(200, "application/json", json);
}

// Restablecer parámetros de fábrica
void handleResetDev() {
  sensorAlimentacion.setK(K_ALIMENTACION);
  sensorPermeado.setK(K_PERMEADO);
  bomba.setMlPorVuelta(ML_POR_VUELTA);
  bomba.setPulsosPorRev(PULSOS_POR_REV);
  
  guardarParametrosNVS(K_ALIMENTACION, K_PERMEADO, ML_POR_VUELTA, PULSOS_POR_REV);
  server.send(200, "application/json", "{\"status\":\"reset_ok\"}");
}

// Exportación a Excel (.CSV) con filtrado por Ensayo o Histórico Completo
void handleExportCSV() {
  String targetEnsayo = server.hasArg("ensayo") ? server.arg("ensayo") : "all";
  
  String filename = "PlantaUF_Calibracion_";
  if (targetEnsayo == "all") {
    filename += "HistoricoCompleto.csv";
  } else if (targetEnsayo == "actual") {
    filename += "Ensayo_" + String(ensayoActualId) + "_EnVivo.csv";
  } else {
    filename += "Ensayo_" + targetEnsayo + ".csv";
  }

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader("Content-Type", "text/csv; charset=UTF-8");
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
  server.sendHeader("Connection", "close");
  server.send(200, "text/csv; charset=UTF-8", "");
  
  // Encabezado CSV
  server.sendContent("sep=;\n"
                     "PLANTA DE ULTRAFILTRACION FX100 - REGISTRO DE ENSAYOS Y CALIBRACION\n"
                     "Ensayo_ID;Tiempo_s;Tiempo_MinSec;RPM_Bomba;Q_Bomba_Teorico_mLmin;Frec_Alimentacion_Hz;Q_Alimentacion_mLmin;Vol_Alimentacion_L;Frec_PERMEADO_Hz;Q_PERMEADO_mLmin;Vol_PERMEADO_L;Q_Retentado_mLmin;Recuperacion_Y_Pct;Desviacion_Bomba_Alim_Pct;K_Alim;K_Perm;J_LMH\n");

  uint8_t filtroId = 0;
  if (targetEnsayo == "actual") {
    filtroId = ensayoActualId;
  } else if (targetEnsayo != "all") {
    filtroId = (uint8_t)targetEnsayo.toInt();
  }

  char fila[200];
  for (size_t i = 0; i < numRegistros; i++) {
    const RegistroCalibracion& r = bufferLog[i];
    
    if (filtroId > 0 && r.id_ensayo != filtroId) {
      continue;
    }

    uint32_t mins = r.t_relativo_s / 60;
    uint32_t secs = r.t_relativo_s % 60;

    int len = snprintf(fila, sizeof(fila),
      "%u;%lu;%02u:%02u;%.1f;%.1f;%.2f;%.1f;%.4f;%.2f;%.1f;%.4f;%.1f;%.2f;%.2f;%.2f;%.2f;%.2f\n",
      r.id_ensayo, (unsigned long)r.t_relativo_s, mins, secs,
      r.rpm, r.q_bomba, r.f_alim, r.q_alim, r.vol_alim,
      r.f_perm, r.q_perm, r.vol_perm, r.q_ret, r.recov,
      r.delta, r.k_alim, r.k_perm, r.j_lmh
    );

    server.sendContent(fila, len);
    if ((i & 31) == 0) yield();
  }

  server.sendContent(""); // Cierra el streaming chunked
}

void handleClearCSV() {
  numRegistros = 0;
  numEnsayos = 0;
  ensayoActualId = 1;
  tInicioEnsayo_ms = millis();
  server.send(200, "text/plain", "LOGS_CLEARED");
}

// ------------------------------------------------------------------------------
// SETUP DEL MICROCONTROLADOR ESP32
// ------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n==================================================");
  Serial.println(" PLANTA PILOTO DE ULTRAFILTRACIÓN FX100 — UNSa   ");
  Serial.println(" Rampa S-Curve, Auto-Calibracion & Datalogger     ");
  Serial.println("==================================================");

  // 1. Cargar parámetros de calibración desde NVS Flash
  cargarParametrosNVS();

  // 2. Inicialización de Hardware
  pinMode(PIN_LED_BOMBA, OUTPUT);
  digitalWrite(PIN_LED_BOMBA, LOW);

  sensorAlimentacion.begin();
  sensorPermeado.begin();
  bomba.begin();

  // 3. Configuración Wi-Fi Robusta (AP Dedicado Anti-Desconexión)
  WiFi.disconnect(true);           // Limpiar estados previos
  delay(100);
  WiFi.mode(WIFI_AP);              // Modo AP Puro (evita escaneos STA que botan clientes)
  WiFi.setSleep(false);            // CRÍTICO: Desactiva ahorro de energía del módem
  WiFi.setTxPower(WIFI_POWER_19_5dBm); // Máxima potencia de transmisión RF

  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(SSID_AP, PASS_AP, 1, 0, 4); // Canal 1 fijo, SSID visible, hasta 4 clientes

  Serial.println("[WIFI] Punto de Acceso Estable Creado:");
  Serial.printf("       SSID: %s | Pass: %s\n", SSID_AP, PASS_AP);
  Serial.printf("       IP AP: http://%s\n", WiFi.softAPIP().toString().c_str());

  if (MDNS.begin("bomba")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("[mDNS] Servidor publicado en: http://bomba.local");
  }

  // 4. Enrutamiento del Servidor Web
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/cmd", HTTP_GET, handleCmd);
  server.on("/set", HTTP_GET, handleSetRPM);
  server.on("/set_dev", HTTP_GET, handleSetDev);
  server.on("/reset_dev", HTTP_GET, handleResetDev);
  server.on("/iniciar_auto_cal", HTTP_GET, handleIniciarAutoCal);
  server.on("/cancelar_auto_cal", HTTP_GET, handleCancelarAutoCal);
  server.on("/calibrar_rpm_q", HTTP_GET, handleCalibrarRpmQ);
  server.on("/export_csv", HTTP_GET, handleExportCSV);
  server.on("/clear_csv", HTTP_GET, handleClearCSV);

  server.begin();
  Serial.println("[HTTP] Servidor Web SCADA iniciado con exito en puerto 80.\n");

  tLoop = millis();
  tCaudal = millis();
  tDatalogger = millis();
  tInicioEnsayo_ms = millis();
}

// ------------------------------------------------------------------------------
// BUCLE PRINCIPAL (LOOP NO BLOQUEANTE)
// ------------------------------------------------------------------------------
void loop() {
  server.handleClient();

  uint32_t tAhora = millis();

  // 1. Control cinemático de la bomba cada 50 ms (Rampa S-Curve Progresiva)
  if (tAhora - tLoop >= 50) {
    float dt = (tAhora - tLoop) / 1000.0f;
    tLoop = tAhora;
    bomba.tick(dt);

    // Testigo LED onboard
    digitalWrite(PIN_LED_BOMBA, (bomba.rpmActual() > 0.5f) ? HIGH : LOW);

    // Detección de flancos de la bomba para registro de ensayos
    bool enMarcha = bomba.enMarcha();
    
    // Flanco de subida: Iniciar sesión de ensayo
    if (enMarcha && !bombaEnMarchaAnterior) {
      uint16_t muestrasPrevias = 0;
      for (size_t i = 0; i < numRegistros; i++) {
        if (bufferLog[i].id_ensayo == ensayoActualId) muestrasPrevias++;
      }
      if (muestrasPrevias > 0) {
        ensayoActualId++;
      }
      tInicioEnsayo_ms = millis();
      volAlimInicioEnsayo = sensorAlimentacion.volumen_L();
      volPermInicioEnsayo = sensorPermeado.volumen_L();
      Serial.printf("\n>>> [ENSAYO #%u INICIADO] Consigna: %.1f RPM <<<\n", ensayoActualId, bomba.rpmObjetivo());
    }
    
    // Flanco de bajada: Finalizar sesión y registrar
    if (!enMarcha && bombaEnMarchaAnterior) {
      uint32_t duracion_s = (millis() - tInicioEnsayo_ms) / 1000;
      uint16_t muestrasEnsayo = 0;
      for (size_t i = 0; i < numRegistros; i++) {
        if (bufferLog[i].id_ensayo == ensayoActualId) muestrasEnsayo++;
      }

      if (muestrasEnsayo > 0 && numEnsayos < MAX_ENSAYOS) {
        listaEnsayos[numEnsayos].id           = ensayoActualId;
        listaEnsayos[numEnsayos].rpm_consigna = bomba.rpmObjetivo();
        listaEnsayos[numEnsayos].t_inicio_ms  = tInicioEnsayo_ms;
        listaEnsayos[numEnsayos].duracion_s   = duracion_s;
        listaEnsayos[numEnsayos].muestras     = muestrasEnsayo;
        listaEnsayos[numEnsayos].vol_alim     = sensorAlimentacion.volumen_L() - volAlimInicioEnsayo;
        listaEnsayos[numEnsayos].vol_perm     = sensorPermeado.volumen_L() - volPermInicioEnsayo;
        numEnsayos++;

        Serial.printf("\n<<< [ENSAYO #%u FINALIZADO Y REGISTRADO] Duracion: %us | Muestras: %u | Vol Perm: %.3f L >>>\n\n",
                      ensayoActualId, duracion_s, muestrasEnsayo, sensorPermeado.volumen_L() - volPermInicioEnsayo);
      }
    }

    bombaEnMarchaAnterior = enMarcha;
  }

  // 2. Adquisición y cálculo de caudales cada 1000 ms (1 segundo)
  if (tAhora - tCaudal >= 1000) {
    float dt = (tAhora - tCaudal) / 1000.0f;
    tCaudal = tAhora;

    bool bombaEmpuja = (bomba.rpmActual() > 1.0f);
    sensorAlimentacion.actualizar(dt, bombaEmpuja);
    sensorPermeado.actualizar(dt, bombaEmpuja);

    float qAlim = sensorAlimentacion.caudal_mLmin();
    float qPerm = sensorPermeado.caudal_mLmin();

    // Sanity-check de cruce de sensores (el permeado no puede exceder físicamente al caudal de alimentación)
    flagCruceSensores = (qPerm > qAlim + 50.0f) && (bomba.rpmActual() > 5.0f);
    if (flagCruceSensores) {
      Serial.printf("⚠️ [ALERTA] Cruce de cables o sensor invertido: Q_Perm (%.1f mL/min) > Q_Alim (%.1f mL/min)\n", qPerm, qAlim);
    }

    // Balance Hidráulico Tangencial y Flujo Darcy en tiempo real
    qRet_mLmin   = fmaxf(0.0f, qAlim - qPerm);
    recuperacion = (qAlim > 1.0f) ? ((qPerm / qAlim) * 100.0f) : 0.0f;
    jLMH_actual  = (qPerm * 0.06f) / AREA_MEMBRANA_M2;

    float qBomba = bomba.caudalTeorico_mLmin();
    deltaBomba   = (qBomba > 1.0f) ? (((qAlim - qBomba) / qBomba) * 100.0f) : 0.0f;

    // Máquina de estados de Auto-Calibración en régimen permanente
    if (autoCalibrando) {
      if (bomba.enRegimenEstable()) {
        autoCalSumFrecAlim += sensorAlimentacion.frecuencia_Hz();
        autoCalSumFrecPerm += sensorPermeado.frecuencia_Hz();
        autoCalMuestras++;

        Serial.printf("[AUTO-CAL] Muestra %u/%u | F_Alim=%.2f Hz | F_Perm=%.2f Hz\n",
                      autoCalMuestras, MUESTRAS_AUTO_CAL, sensorAlimentacion.frecuencia_Hz(), sensorPermeado.frecuencia_Hz());

        if (autoCalMuestras >= MUESTRAS_AUTO_CAL) {
          float fPromAlim = autoCalSumFrecAlim / (float)MUESTRAS_AUTO_CAL;
          float fPromPerm = autoCalSumFrecPerm / (float)MUESTRAS_AUTO_CAL;
          float qRefAlim  = bomba.rpmActual() * bomba.getMlPorVuelta();

          if (qRefAlim > 10.0f && fPromAlim > 1.0f) {
            float nuevoKa = (fPromAlim * 1000.0f) / qRefAlim;
            sensorAlimentacion.setK(nuevoKa);
          }
          if (fPromPerm > 0.2f) {
            float targetPerm = bomba.rpmActual() * 2.0f;
            float nuevoKp = (fPromPerm * 1000.0f) / targetPerm;
            sensorPermeado.setK(nuevoKp);
          }

          guardarParametrosNVS(sensorAlimentacion.getK(), sensorPermeado.getK(),
                               bomba.getMlPorVuelta(), bomba.getPulsosPorRev());

          autoCalibrando = false;
          autoCalMensaje = "✅ Auto-Calibracion OK: K_Alim=" + String(sensorAlimentacion.getK(), 2) + " | K_Perm=" + String(sensorPermeado.getK(), 2);
          Serial.printf("\n>>> %s <<<\n\n", autoCalMensaje.c_str());
        }
      } else {
        Serial.println("[AUTO-CAL] Esperando estabilizacion de RPM...");
      }
    }

    // Telemetría periódica por Serial
    Serial.printf("[TELEMETRIA] RPM: %4.1f | Q_Alim: %5.1f mL/min | Q_Perm: %5.1f mL/min | J: %4.2f LMH | Q_Ret: %5.1f mL/min | Y: %4.1f%% | V_Perm: %.3f L\n",
                  bomba.rpmActual(), qAlim, qPerm, jLMH_actual, qRet_mLmin, recuperacion, sensorPermeado.volumen_L());
  }

  // 3. Muestreo del Datalogger cada 10 segundos
  if (tAhora - tDatalogger >= INTERVALO_LOG_MS) {
    tDatalogger = tAhora;
    if (bomba.enMarcha() || sensorAlimentacion.caudal_mLmin() > 10.0f || sensorPermeado.caudal_mLmin() > 5.0f) {
      guardarMuestraDatalogger();
    }
  }
}

```

---

# 4. PROTOCOLO DE BANCO Y PLANILLA EXCEL AUTOMATIZADA

Para los ensayos de calibración de esta semana, preparamos el archivo Excel:
`PLANILLA_CALIBRACION_ENSAYOS_2_Y_3.xlsx`

### Protocolo Operativo:
* **Instrumento:** Probeta graduada de **$1000\text{ mL}$** y cronómetro digital.
* **Tiempo de Corrida:** **$1.0\text{ minuto}$ ($60\text{ s}$)** para velocidades de $20$ a $70\text{ RPM}$ ($Q \le 952\text{ mL/min}$, dentro de la probeta). Para $80$ y $90\text{ RPM}$, se cronometra **$0.5\text{ minutos}$ ($30\text{ s}$)** para evitar desborde de los $1000\text{ mL}$.
* **Ensayo 2 (Bomba Peristáltica):** 
  - Se ingresa RPM y volumen recolectado en probeta.
  - La planilla calcula la cilindrada puntual $V_{\text{vuelta}, i} = Q_i / \text{RPM}_i$, la media aritmética, la desviación estándar, el coeficiente de variación (CV%) y la pendiente de la regresión lineal $Q$ vs $\text{RPM}$ con su $R^2$.
  - Entrega el parámetro definitivo para `ML_POR_VUELTA` en `config.h`.
* **Ensayo 3 (Caudalímetro de Alimentación):**
  - Sensor en serie con la probeta. Se ingresa la frecuencia promedio del SCADA ($F$ en Hz) y el volumen en probeta.
  - La planilla calcula $K_i = F_i / Q_{\text{L/min}, i}$ en $[\text{Hz}/(\text{L/min})]$, su equivalente en $\text{pulsos/Litro} = K \times 60$, la pendiente de regresión lineal $F$ vs $Q_{\text{L/min}}$ ($K_{\text{regresión}}$) y el $R^2$.
  - Entrega el parámetro definitivo para `K_ALIMENTACION` en `config.h`.
* **Ensayo 4 (Permeado):** **Postergado** hasta contar con los transductores de presión para medir la Presión Transmembrana ($\text{TMP}$), evitando experimentos a ciegas.

---

# 5. EL SENSOR DE PRESIÓN SELECCIONADO

Hemos seleccionado para la compra en Mercado Libre el siguiente transductor:
* **Modelo:** Transductor de presión piezorresistivo OMYTECH / Genérico de Acero Inoxidable.
* **Rango:** **$0\text{ a }30\text{ PSI}$ ($0\text{ a }2.07\text{ bar}$)**.
* **Salida Analógica:** **$0.5\text{ a }4.5\text{ V}$** lineal ratiométrica ($0\text{ PSI} \rightarrow 0.5\text{ V}$, $30\text{ PSI} \rightarrow 4.5\text{ V}$).
* **Sensibilidad:** $\approx 1.93\text{ V/bar}$.
* **Medio:** Compatible con agua, aceites y líquidos (diafragma estanco de acero inoxidable AISI 304/316).
* **Conexión al microcontrolador:** Señal conectada al conversor ADC **ADS1115 de 16 bits** por bus $I^2C$. A $0.6\text{ bar}$ entrega $1.66\text{ V}$, operando en el tercio más lineal de su escala sin saturar ante transitorios.

---

# 6. HOJA DE RUTA GENERAL: DE LA CALIBRACIÓN ACTUAL A LA DEFENSA DE LA TESIS

Para que tengas la visión panorámica del proyecto y evalúes cómo encaja el código actual con el objetivo final, te compartimos la **secuencia de hitos de la tesis de Antonella Guitián y Owen Cañizares (Codir. Ing. Enzo)**:

```
  [HITO 2.2 ACTUAL]           [HITO 2.3 PRÓXIMO]          [HITO 3: CONTROL]          [HITO 4: ENSAYOS]           [HITO 5: CIERRE]
┌──────────────────┐        ┌──────────────────┐        ┌──────────────────┐        ┌──────────────────┐        ┌──────────────────┐
│ Calibración de   │        │ Montaje Presión: │        │ Control en Lazo  │        │ Matriz Factorial │        │ Procesamiento    │
│ Bomba y Caudal   │ ─────► │ 2x Transductores │ ─────► │ Cerrado (TMP/RPM)│ ─────► │ 3² (12 Corridas) │ ─────► │ en Python,       │
│ de Alimentación  │        │ 30 PSI + ADS1115 │        │ + Interlock 50ms │        │ Flujo Crítico Jc │        │ Redacción Final  │
│ (Probeta 1000 mL)│        │ + Manómetros "T" │        │ + Sonda DS18B20  │        │ y Ensuciamiento  │        │ y Defensa UNSa   │
└──────────────────┘        └──────────────────┘        └──────────────────┘        └──────────────────┘        └──────────────────┘
```

1. **Hito 2.2 (ETAPA ACTUAL — ESTE LUNES EN BANCO):**
   - Calibración volumétrica de la bomba peristáltica MBP-2000 ($\text{mL/rev}$) con probeta graduada de $1000\text{ mL}$ durante $1\text{ minuto}$ (y $0.5\text{ min}$ para $\ge 80\text{ RPM}$).
   - Puesta a punto y ajuste fino del factor $K_{\text{alim}}$ $[\text{Hz}/(\text{L/min})]$ del sensor YF-S401 de impulsión a 8 niveles de RPM.
   - Procesamiento inmediato en la planilla automatizada `PLANILLA_CALIBRACION_ENSAYOS_2_Y_3.xlsx` con regresión lineal y carga de parámetros definitivos en la Flash NVS.
2. **Hito 2.3 (Instrumentación de Presión y Permeado):**
   - Llegada y montaje en derivación "T" de los 2 transductores de $30\text{ PSI}$ ($0.5-4.5\text{ V}$) + 2 manómetros mecánicos de glicerina ($0-1\text{ bar}$) para $P_1$ (alimentación) y $P_2$ (retentado).
   - Lectura analógica diferencial/single-ended mediante conversor ADS1115 de 16 bits en bus $I^2C$.
   - Activación del cálculo de Presión Transmembrana ($\text{TMP}$) y del modelo Darcy-Vogel en vivo.
   - Ejecución del **Ensayo 4: Calibración gravimétrica del sensor de permeado** contrastando contra balanza analítica digital ($0.1\text{ g}$) a tres niveles de TMP ($0.15$, $0.30$ y $0.45\text{ bar}$).
3. **Hito 3 (Control Automático e Interlocks de Seguridad):**
   - Enclavamiento de parada dura (*hard stop*) a $50\text{ ms}$ ante sobrepresión ($P_1 > 0.60\text{ bar}$ o $\text{TMP} > 0.45\text{ bar}$) que corte la generación de pulsos LEDC y desacople el driver DM860.
   - Sonda térmica DS18B20 (GPIO 4) para normalización automática de permeabilidad a $20^\circ\text{C}$ ($J_{20}$).
   - Sensor de turbidez / TDS para medición de retención y calidad del permeado.
4. **Hito 4 (Diseño Experimental de Tesis — Matriz Factorial $3^2$):**
   - 12 corridas experimentales (9 combinaciones de velocidad tangencial y TMP + 3 réplicas en el punto central) con agua limpia y con efluente/sobrenadante coagulado.
   - Determinación experimental del flujo crítico ($J_c$), permeabilidad hidráulica pura ($L_p$), resistencia intrínseca de membrana ($R_m$) y cinética de resistencia de capa de torta ($R_{\text{torta}}$).
   - Ensayos de ensuciamiento progresivo (*fouling*) y protocolos de limpieza hidráulica y química.
5. **Hito 5 (Procesamiento Científico de Datos y Defensa de Tesis):**
   - Exportación de los archivos históricos `.CSV` del datalogger integrado hacia scripts en Python (Pandas/Matplotlib/Seaborn) para la generación de gráficas vectoriales y análisis estadístico ANOVA.
   - Cierre de la memoria escrita y defensa oral pública de la tesis de Antonella y Owen en la Universidad Nacional de Salta.

---

# ❓ EJES DE AUDITORÍA REQUERIDOS PARA GPT ASTRA

Por favor, estructura tu respuesta abordando rigurosamente los siguientes cinco ejes:

### EJE 1: Metrología de Caudal y Robustez de Software
1. Evalúa el algoritmo de **período recíproco híbrido** ($n \ge 2$, $n = 1$, $n = 0$). ¿Hay algún *edge case* o condición de carrera remanente?
2. Examina la **cota física superior continua** ($f \le 10^6 / \Delta t_{\text{sin\_pulso}}$). ¿Cumple con garantizar el decaimiento asintótico a cero sin escalones artificiales al detener la bomba?
3. Verifica la demostración de la **inmunidad modular al rollover** de `micros()` cada 71.58 minutos en la resta `uint32_t dt = t - c->_t_ultimo;`.
4. Confirma si la unidad formal $K \in [\text{Hz}/(\text{L/min})]$ y la integración $V\ [\text{L}] = \sum n / (K \times 60)$ son matemáticamente exactas e impecables.

### EJE 2: Selección del Sensor de Presión de 30 PSI ($2.07\text{ bar}$)
1. ¿Es técnicamente adecuada la elección del rango de $30\text{ PSI}$ frente a un sensor de $1.0\text{ bar}$ (que saturaría ante sobrepresión) o uno de $100\text{ PSI}$ (que perdería resolución)?
2. Con una sensibilidad de $1.93\text{ V/bar}$ y el ADC ADS1115 de 16 bits, ¿qué nivel de resolución efectiva y relación señal/ruido podemos esperar en la medición de $\text{TMP}$?
3. ¿Qué precauciones eléctricas mínimas recomiendas en el cableado entre el transductor de 3 cables ($5\text{V}$, $\text{GND}$, Señal) y el ADS1115?

### EJE 3: Protocolo Experimental en Banco
1. ¿Es adecuado el protocolo de probeta de $1000\text{ mL}$ durante $1\text{ minuto}$ (y $0.5\text{ min}$ para $\ge 80\text{ RPM}$) para la bomba peristáltica y el sensor de impulsión?
2. ¿Respaldas la decisión de postergar la calibración de permeado hasta tener la medición de presión montada?

### EJE 4: Detección de Puntos Ciegos (*Blind Spots*) y Validación de la Hoja de Ruta
1. **Puntos ciegos inmediatos:** Mirando el código fuente completo y el banco hidráulico, ¿ves algún **inconveniente oculto, riesgo no contemplado o detalle técnico que se nos esté pasando por alto** específicamente para este código, cuyo objetivo inmediato es **poner a punto y calibrar los caudalímetros en el banco este lunes**?
2. **Validación de la Hoja de Ruta:** ¿Consideras lógica, viable y secuencialmente sólida la hoja de ruta planteada (Hito 2.2 Caudales $\rightarrow$ Hito 2.3 Presión $\rightarrow$ Hito 3 Control/Interlocks $\rightarrow$ Hito 4 Factorial $3^2$ $\rightarrow$ Hito 5 Cierre)?
3. **Recomendaciones de transición:** ¿Qué precauciones nos sugieres tomar para que la transición entre el banco de caudales de este lunes y la incorporación de los transductores de presión y el diseño factorial sea lo más fluida posible sin tener que reescribir código?

### EJE 5: Dictamen Final y Lista de Chequeo para el Lunes
1. Emite tu veredicto: **APROBADO PARA BANCO DE ENSAYOS**, **APROBADO CONDICIONADO** o **RECHAZADO**.
2. Proporciona una lista de chequeo (*checklist*) rápida de 5 pasos para que Owen y Antonella ejecuten el lunes en el laboratorio de la UNSa con la probeta y la planilla Excel.
