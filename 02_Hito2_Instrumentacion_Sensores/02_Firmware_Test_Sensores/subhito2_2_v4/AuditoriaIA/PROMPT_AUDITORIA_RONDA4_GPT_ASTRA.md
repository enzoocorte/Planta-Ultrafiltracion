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

# 3. EL CÓDIGO FUENTE TEMPLADO EN AUDITORÍA (`caudalimetro.h` y `caudalimetro.cpp`)

Este es el código implementado y comiteado en la rama `main` de nuestro repositorio. Te solicitamos revisarlo exhaustivamente línea por línea:

### Archivo `caudalimetro.h`:
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

### Archivo `caudalimetro.cpp`:
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

# ❓ EJES DE AUDITORÍA REQUERIDOS PARA GPT ASTRA

Por favor, estructura tu respuesta abordando rigurosamente los siguientes puntos:

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

### EJE 4: Dictamen Final y Lista de Chequeo para el Lunes
1. Emite tu veredicto: **APROBADO PARA BANCO DE ENSAYOS**, **APROBADO CONDICIONADO** o **RECHAZADO**.
2. Proporciona una lista de chequeo (*checklist*) rápida de 5 pasos para que Owen y Antonella ejecuten el lunes en el laboratorio de la UNSa con la probeta y la planilla Excel.
