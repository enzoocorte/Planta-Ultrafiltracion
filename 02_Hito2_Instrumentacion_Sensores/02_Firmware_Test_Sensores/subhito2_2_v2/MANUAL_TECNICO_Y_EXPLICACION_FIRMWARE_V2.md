# 📘 MANUAL TÉCNICO Y EXPLICACIÓN INTEGRAL DEL FIRMWARE `subhito2_2_v2`
## Planta Piloto de Ultrafiltración FX100 • Subhito 2.1: Instrumentación y Control de Caudal
**Codirector**: Ing. Enzo  
**Tesistas**: Antonella Guitián & Owen Cañizares  
**Carrera**: Ingeniería Industrial / Química  
**Ubicación del Firmware**: [`02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v2/`](./)

---

# 📑 ÍNDICE
1. [Arquitectura General y Filosofía de Diseño](#1-arquitectura-general-y-filosofía-de-diseño)
2. [Fundamentos Físicos y Matemáticos](#2-fundamentos-físicos-y-matemáticos)
   - [¿De dónde sale el valor de 4.2 mL/vuelta de la bomba?](#21-de-dónde-sale-el-valor-de-42-mlvuelta-de-la-bomba)
   - [¿De dónde sale el factor K = 98 del caudalímetro YF-S401?](#22-de-dónde-sale-el-factor-k--98-del-caudalímetro-yf-s401)
   - [Cálculo Cinemático de RPM, Micropasos y Frecuencia](#23-cálculo-cinemático-de-rpm-micropasos-y-frecuencia)
3. [Conexionado Eléctrico: ¿Por qué 5V y qué función cumple la resistencia?](#3-conexionado-eléctrico-por-qué-5v-y-qué-función-cumple-la-resistencia)
4. [Análisis Exhaustivo Módulo por Módulo (Línea por Línea)](#4-análisis-exhaustivo-módulo-por-módulo-línea-por-línea)
   - [Módulo 1: config.h](#módulo-1-configh)
   - [Módulo 2: caudalimetro.h y caudalimetro.cpp](#módulo-2-caudalimetroh-y-caudalimetrocpp)
   - [Módulo 3: bomba.h](#módulo-3-bombah)
   - [Módulo 4: subhito2_2_v2.ino](#módulo-4-subhito2_2_v2ino)
   - [Módulo 5: index_html.h](#módulo-5-index_htmlh)
5. [Exportación de Datos a Excel (Tiempos, Sentidos y Volúmenes)](#5-exportación-de-datos-a-excel-tiempos-sentidos-y-volúmenes)
6. [Protocolo Experimental de Calibración con Probeta](#6-protocolo-experimental-de-calibración-con-probeta)

---

# 1. Arquitectura General y Filosofía de Diseño

En el ámbito industrial y académico de grado, el software embebido no debe ser un script monolítico ("espagueti"). El presente desarrollo adopta el estándar de **Separación de Responsabilidades** (*Separation of Concerns*) mediante programación orientada a objetos en C++:

```
                              ┌─────────────────────────┐
                              │        config.h         │
                              │ (Parámetros de Proceso) │
                              └────────────┬────────────┘
                                           │
                    ┌──────────────────────┼──────────────────────┐
                    ▼                      ▼                      ▼
         ┌────────────────────┐ ┌────────────────────┐ ┌────────────────────┐
         │      Bomba.h       │ │  caudalimetro.h/.cpp│ │   index_html.h     │
         │ Control Cinemático │ │ Abstracción Sensor │ │  Dashboard SCADA   │
         │  y Hardware LEDC   │ │   + Filtro IIR     │ │  (PROGMEM Flash)   │
         └──────────┬─────────┘ └──────────┬─────────┘ └──────────┬─────────┘
                    │                      │                      │
                    └──────────────────────┼──────────────────────┘
                                           ▼
                              ┌─────────────────────────┐
                              │    subhito2_2_v2.ino    │
                              │ Orquestador / FreeRTOS  │
                              │    Balance de Materia   │
                              └─────────────────────────┘
```

### Principios Fundamentales:
* **Cero Demoras Bloqueantes (`Non-blocking design`)**: Prohibición terminante de la función `delay()` en el lazo principal. Toda la temporización opera por deltas de tiempo ($\Delta t = t - t_{\text{anterior}}$) utilizando el contador del sistema `millis()`.
* **Generación de Pasos por Hardware (`LEDC`)**: El microcontrolador ESP32 no pierde ciclos de CPU alternando pines (`digitalWrite` con retardos). En su lugar, delega la generación de onda cuadrada al periférico de silicio LEDC.
* **Sincronización Atómica entre Núcleos (`FreeRTOS Spinlocks`)**: El ESP32 posee dos núcleos (Core 0 y Core 1). Para evitar que una interrupción de hardware colisione con el hilo principal que lee variables compartidas, se aplican cerrojos de giro atómicos (`portMUX_TYPE`).
* **SCADA Embebido sin Consumo de RAM (`PROGMEM`)**: El código HTML/CSS/JS de la interfaz gráfica web reside en la memoria Flash ($4\text{ MB}$), consumiendo $0\text{ bytes}$ de la memoria SRAM ($320\text{ KB}$).

---

# 2. Fundamentos Físicos y Matemáticos

---

## 2.1. ¿De dónde sale el valor de 4.2 mL/vuelta de la bomba?

Una bomba peristáltica es una **bomba de desplazamiento positivo volumétrico**. El fluido avanza porque los rodillos del cabezal aprietan (ocluyen) la manguera flexible contra una pista semicircular, atrapando un "bolsillo" hermético de líquido y empujándolo hacia la salida.

### Ecuación Teórica del Desplazamiento:
El volumen desplazado en una revolución completa del rotor depende de la geometría interna del tubo y del cabezal:

$$V_{\text{rev}} = \pi \times D_{\text{pista}} \times A_{\text{tubo}} \times \eta_v$$

Donde:
* $D_{\text{pista}}$: Diámetro promedio de la pista semicircular del cabezal $(\approx 7.0\text{ cm})$.
* $A_{\text{tubo}} = \frac{\pi \times d_i^2}{4}$: Área transversal interna de la manguera. Para una manguera de silicona grado médico con diámetro interno $d_i = 4.8\text{ mm}$ ($0.48\text{ cm}$):
  $$A_{\text{tubo}} = \frac{\pi \times (0.48)^2}{4} \approx 0.181\text{ cm}^2 = 0.181\text{ mL/cm}$$
* Longitud efectiva de la pista perimetral: $L = \pi \times D_{\text{pista}} \approx 3.1416 \times 7.0 \approx 22\text{ cm}$.
* Desplazamiento geométrico teórico:
  $$V_{\text{geom}} = 22\text{ cm} \times 0.181\text{ mL/cm} \approx 3.98\text{ a }4.3\text{ mL/vuelta}$$
* $\eta_v$: Rendimiento volumétrico de oclusión (normalmente $\approx 0.95 - 0.98$ a presiones moderadas).

De allí surge el valor inicial estándar de **$4.2\text{ mL/vuelta}$** para la bomba MBP-2000.  
> ⚠️ **Importante para la Tesis**: Este valor teórico debe ser calibrado experimentalmente mediante el ensayo de probeta de 100 vueltas, ya que la viscosidad del fluido, el aplastamiento de la manguera y la contrapresión de la membrana alteran ligeramente este factor.

---

## 2.2. ¿De dónde sale el factor K = 98 del caudalímetro YF-S401?

**Sí, sale directamente de la hoja de datos (*datasheet*) del fabricante del microcaudalímetro YF-S401.**

El sensor consta de una microturbina con 4 pequeños imanes de neodimio montados en sus aspas y un sensor de efecto Hall encapsulado en la carcasa plástica. Cada vez que un imán pasa frente al sensor Hall, éste genera un pulso digital (onda cuadrada).

### La Ecuación Característica del Fabricante:
$$F = 98 \times Q$$

Donde:
* $F$: Frecuencia del tren de pulsos en Hertz [$\text{Hz} = \text{pulsos/segundo}$].
* $Q$: Caudal volumétrico en Litros por minuto [$\text{L/min}$].
* $K = 98.0$: Constante de proporcionalidad intrínseca de la cámara de turbina.

### Demostración de Pulsos por Litro:
Si por el sensor fluye un caudal constante de $Q = 1\text{ L/min}$, la frecuencia generada es:
$$F = 98\text{ pulsos/segundo}$$
Como $1\text{ minuto} = 60\text{ segundos}$, el número total de pulsos emitidos al pasar exactamente $1\text{ Litro}$ es:
$$\text{Pulsos por Litro} = 98\text{ pulsos/s} \times 60\text{ s} = \mathbf{5880\text{ pulsos/Litro}}$$

Por tanto, cada pulso generado por el sensor representa un volumen infinitesimal exacto de:
$$\text{Volumen por pulso} = \frac{1\text{ Litro}}{5880} \approx 0.170068\text{ mL}$$

### Despeje del Caudal Instantáneo en mL/min:
Para el firmware, necesitamos el caudal en $\text{mL/min}$ para compararlo con la bomba peristáltica:
1. $Q\text{ (L/min)} = \frac{F}{98.0}$
2. Como $1\text{ L} = 1000\text{ mL}$:
   $$Q\text{ (mL/min)} = Q\text{ (L/min)} \times 1000 = \frac{F \times 1000}{98.0}$$

*(El error detectado en versiones preliminares consistía en haber dividido por 60 en esta fórmula, reduciendo el caudal calculado en un factor de 60)*.

---

## 2.3. Cálculo Cinemático de RPM, Micropasos y Frecuencia

Para accionar un motor paso a paso mediante un driver como el Leadshine DM860, el microcontrolador debe suministrar una secuencia de pulsos a una frecuencia específica.

### 1. Parámetros del Motor y Driver:
* **Paso angular nativo**: $1.8^\circ$ por paso $\implies \frac{360^\circ}{1.8^\circ} = 200\text{ pasos completos por revolución}$.
* **Configuración de micropasos en el DM860**: Microstepping en $\times 8$ (mediante llaves DIP SW5:ON, SW6:OFF, SW7:ON, SW8:ON).
* **Pulsos totales por revolución del eje**:
  $$\text{PULSOS\_POR\_REV} = 200 \times 8 = \mathbf{1600\text{ pulsos/vuelta}}$$

### 2. Conversión de RPM a Frecuencia de Pulsos (LEDC):
Si deseamos que el motor gire a una velocidad angular $\omega$ dada en $\text{RPM}$ (revoluciones por minuto):
$$\text{Revoluciones por segundo} = \frac{\text{RPM}}{60}$$
$$f_{\text{pulsos}} (\text{Hz}) = \left(\frac{\text{RPM}}{60}\right) \times \text{PULSOS\_POR\_REV} = \frac{\text{RPM} \times 1600}{60}$$

#### Ejemplos Numéricos Operativos:
* **A 72 RPM** (mínimo de caudalímetro: $302.4\text{ mL/min}$):
  $$f_{\text{pulsos}} = \frac{72 \times 1600}{60} = \mathbf{1920\text{ Hz}}$$
* **A 100 RPM** ($420\text{ mL/min}$):
  $$f_{\text{pulsos}} = \frac{100 \times 1600}{60} = \mathbf{2666.67\text{ Hz}}$$
* **A 140 RPM** (techo de membrana: $588\text{ mL/min}$):
  $$f_{\text{pulsos}} = \frac{140 \times 1600}{60} = \mathbf{3733.33\text{ Hz}}$$

> **¿Qué significaba "a 100 pulsos me andaba con el caudal correcto" que recordabas de una bomba anterior?**  
> En controladores antiguos o pequeños microcontroladores sin generador PWM por hardware, era muy común fijar una interrupción por timer de $100\text{ Hz}$ fijos, o usar drivers configurados a paso completo ($200\text{ pasos/rev}$).  
> A $200\text{ pulsos/rev}$, una frecuencia de $100\text{ pulsos/segundo}$ ($100\text{ Hz}$) resulta en:
> $$\text{RPM} = \frac{100\text{ pulsos/s} \times 60}{200\text{ pulsos/rev}} = 30\text{ RPM}$$
> Con $4.2\text{ mL/vuelta}$, $30\text{ RPM} \times 4.2 = 126\text{ mL/min}$. Al subir a 1600 micropasos para eliminar vibraciones mecánicas y ruido acústico, se requieren 8 veces más pulsos para la misma velocidad.

---

# 3. Conexionado Eléctrico: ¿Por qué 5V y qué función cumple la resistencia?

### 1. Alimentación de los Sensores (Cables Rojo y Negro):
* **Cable Rojo (VCC)**: Debe conectarse a **$5\text{V}$ (Pin VIN del ESP32)** cuando el microcontrolador está alimentado por USB.
  * *¿Por qué NO a 3.3V?*: El circuito integrado Hall y el comparador Schmitt-Trigger dentro del YF-S401 están especificados por hoja de datos para operar de $5\text{V}$ a $18\text{V}$ DC. A $3.3\text{V}$ están por debajo del voltaje mínimo de polarización del silicio; algunos sensores encienden de forma errática y otros quedan completamente mudos.
* **Cable Negro (GND)**: Debe conectarse a un pin **GND común** del ESP32 y de la fuente.

### 2. Señal de Salida (Cable Amarillo):
La etapa de salida del sensor YF-S401 es un **transistor NPN de colector abierto** (*Open-Collector*):

```
             VCC (+5V)
                │
                │ [Resistencia interna del sensor en algunos clones: 10kΩ]
                ├─── Cable Amarillo
                │          │
             ┌──┴──┐       ├─── [Resistencia serie 10kΩ] ─── GPIO ESP32 (3.3V)
             │ NPN │       │                                        │
             └──┬──┘       │                                  [Pull-up 45kΩ]
                │          │                                        │
               GND        GND                                     +3.3V
```

### ¿Para qué sirve la línea `pinMode(_pin, INPUT_PULLUP)` en el código?
Como la salida es un colector abierto, el transistor solo puede realizar una acción: conectar la línea a GND ($0\text{V}$) cuando detecta el imán. Cuando el imán se aleja, el transistor se abre (alta impedancia) y la línea queda flotando en el aire.  
La instrucción `INPUT_PULLUP` activa una resistencia interna del ESP32 ($\approx 45\text{ k}\Omega$) conectada a la barra de $3.3\text{V}$. Así:
* Imán ausente: la línea se eleva a **$3.3\text{V}$** (estado lógico HIGH).
* Imán presente: el transistor conduce a GND y la línea cae a **$0\text{V}$** (estado lógico LOW).

### ¿Por qué pusiste una resistencia de 10 kΩ en serie en el cable amarillo y para qué sirve?
Muchos clones comerciales del YF-S401 incluyen dentro de su propia resina una resistencia de pull-up conectada a su cable rojo ($+5\text{V}$).  
Si alimentas el sensor con $5\text{V}$, el cable amarillo emitirá pulsos de $0\text{V}$ a $5\text{V}$. Los pines GPIO del ESP32 están diseñados para una tensión lógica máxima de $3.3\text{V}$.
* **Función de la resistencia de 10 kΩ en serie**: Los pines del ESP32 poseen diodos internos de protección contra sobretensión (*ESD clamping diodes*) que derivan el exceso de voltaje a la línea de $3.3\text{V}$.
* Si conectas $5\text{V}$ directamente, circularía una corriente destructiva de varios miliamperios.
* Al colocar la resistencia de $10\text{ k}\Omega$ en serie, la corriente que ingresa al pin queda limitada por la Ley de Ohm a:
  $$I = \frac{5.0\text{V} - (3.3\text{V} + 0.5\text{V}_{\text{diodo}})}{10\,000\,\Omega} = \frac{1.2\text{V}}{10\,000\,\Omega} = \mathbf{0.12\text{ mA}} \quad (120\,\mu\text{A})$$
  Esta corriente es totalmente inocua para los diodos del ESP32 (diseñados para tolerar hasta $5\text{ mA}$).  
* **Conclusión práctica**:
  * **Alimentar ambos sensores a 5V (VIN)**.
  * **Colocar la resistencia de 10 kΩ en serie en el cable amarillo de cada sensor** antes de ingresar al pin GPIO (GPIO 14 para Feed, GPIO 27 para Permeado). Esto protege el microcontrolador y garantiza que el transistor Hall trabaje a su voltaje nominal de saturación.

---

# 4. Análisis Exhaustivo Módulo por Módulo (Línea por Línea)

---

## Módulo 1: `config.h`
```cpp
1: #pragma once
2: // ============================================================
3: //  CONFIGURACIÓN — Planta UF (aquí se cambia TODO)
4: // ============================================================
```
* **Línea 1**: `#pragma once` es una directiva de preprocesador que previene inclusiones cíclicas en el compilador.

```cpp
8:  constexpr uint8_t PIN_PUL         = 18;  // DM860 PUL+ (PUL- a GND común)
9:  constexpr uint8_t PIN_DIR         = 19;  // DM860 DIR+ (DIR- a GND común)
10: constexpr uint8_t PIN_SENSOR_FEED = 14;  // Caudalímetro FEED
11: constexpr uint8_t PIN_SENSOR_PERM = 27;  // Caudalímetro PERMEADO
```
* **Líneas 8-11**: Declaran de forma inmutable (`constexpr`) los pines físicos asignados. Usar `uint8_t` ahorra memoria al restringir el valor a 1 byte sin signo ($0-255$).

```cpp
14: constexpr uint16_t PULSOS_POR_REV = 1600;      // DM860: 8 micropasos (SW5-8)
15: constexpr float ML_POR_VUELTA     = 4.2f;      // mL por giro del cabezal
16: constexpr float RPM_MIN           = 72.0f;     // 302 mL/min (mín. sensor: 300)
17: constexpr float RPM_MAX           = 140.0f;    // 588 mL/min (límite membrana: 600)
18: constexpr float RPM_INICIO        = 72.0f;
19: constexpr float ACEL_RPM_S        = 40.0f;     // rampa
```
* **Líneas 14-19**: Parámetros de gobierno de la bomba peristáltica. Define la relación cinemática de pulsos, el volumen por vuelta estimado, y los umbrales de seguridad de la planta ($72-140\text{ RPM}$).

```cpp
23: constexpr float K_FEED = 98.0f;
24: constexpr float K_PERM = 98.0f;
26: constexpr uint32_t FILTRO_RUIDO_US = 3000;      // pulso real más rápido ≈17 ms
27: constexpr float Q_MAX_FISICO_MLMIN = 6000.0f;   // Límite físico YF-S401 (0.3 a 6 L/min). Permite prueba de soplido
```
* **Líneas 23-27**: Constantes de calibración de los dos sensores YF-S401. El filtro temporal de $3000\,\mu\text{s}$ descarta rebotes parásitos, y $Q_{\text{máx}}$ fija la cota física de rechazo para admitir la prueba del soplido ($< 6\text{ L/min}$).

```cpp
30: constexpr const char* SSID_AP  = "Bomba_Peristaltica_UF";
31: constexpr const char* PASS_AP  = "plantapiloto2";
32: constexpr const char* SSID_STA = "Box804";
33: constexpr const char* PASS_STA = "plantapiloto2";
```
* **Líneas 30-33**: Parámetros de red Wi-Fi para el modo punto de acceso y estación.

---

## Módulo 2: `caudalimetro.h` y `caudalimetro.cpp`

### `caudalimetro.h`:
```cpp
10: class Caudalimetro {
11: public:
12:   Caudalimetro(uint8_t pin, float k, const char* nombre)
13:     : _pin(pin), _k(k), _nombre(nombre) {}
```
* **Líneas 10-13**: Define la clase `Caudalimetro`. Mediante listas de inicialización en el constructor, se asignan los atributos de hardware de cada caudalímetro.

```cpp
15:   void begin() {
16:     pinMode(_pin, INPUT_PULLUP);
17:     attachInterruptArg(digitalPinToInterrupt(_pin), isrPuente, this, FALLING);
18:   }
```
* **Líneas 15-18**: Inicializa el pin con resistencia de pull-up y asocia la interrupción externa ante flancos de bajada (`FALLING`). La función `attachInterruptArg` pasa el puntero de la propia instancia (`this`) como contexto para la ISR estática.

```cpp
22:   void actualizar(float dt_s, bool bombaEmpuja) {
23:     portENTER_CRITICAL(&_mux);
24:     uint32_t n = _pulsos;
25:     _pulsos = 0;
26:     portEXIT_CRITICAL(&_mux);
```
* **Líneas 22-26**: Captura atómica de pulsos. Bloquea momentáneamente el acceso concurrente entre núcleos mediante un spinlock (`_mux`), toma una copia local de `_pulsos`, resetea el acumulador a 0 y libera el cerrojo.

```cpp
28:     _f = (dt_s > 0.0f) ? ((float)n / dt_s) : 0.0f;
29:     float q = (_f * 1000.0f) / _k;
```
* **Líneas 28-29**: Calcula la frecuencia física real ($F = n / \Delta t$) y el caudal instantáneo en $\text{mL/min}$ ($Q = \frac{F \times 1000}{K}$).

```cpp
30:     if (q > Q_MAX_FISICO_MLMIN) {
31:       q = 0.0f;
32:       Serial.printf("[%s] %lu pulsos falsos descartados (f=%.1f Hz)\n", _nombre, (unsigned long)n, _f);
33:     }
```
* **Líneas 30-33**: Filtro de consistencia física. Si el caudal excede $6000\text{ mL/min}$ (frecuencias $> 588\text{ Hz}$), descarta la muestra por tratarse de inducción electromagnética.

```cpp
34:     _q  = (n > 0) ? (0.3f * q + 0.7f * _q) : 0.0f;
35:     _vol += (float)n / (_k * 60.0f);
```
* **Línea 34**: Filtro digital paso bajo recursivo (IIR de primer orden): $y[k] = 0.3 x[k] + 0.7 y[k-1]$. Si no hay pulsos ($n = 0$), se apaga a 0 de inmediato.
* **Línea 35**: Integrador volumétrico absoluto. Suma el volumen exacto acumulado en Litros sin error de redondeo: $\sum \frac{n}{5880}$.

```cpp
38:     if (n > 0) {
39:       _segSinPulso = 0;
40:       _fallo = false;
41:     } else if (bombaEmpuja) {
42:       if (++_segSinPulso >= 5) _fallo = true;
43:     } else {
44:       _segSinPulso = 0;
45:       _fallo = false;
46:     }
```
* **Líneas 38-46**: Máquina de estado para diagnóstico de fallas. Si la bomba impulsa fluido pero transcurren 5 segundos continuos sin registrar pulsos, levanta la alarma de pérdida de señal.

```cpp
49:   float caudal_mLmin()  const { return _q; }
50:   float caudal_Lmin()   const { return _q / 1000.0f; }
51:   float frecuencia_Hz() const { return _f; }
52:   float volumen_L()     const { return _vol; }
53:   bool  sinSenal()      const { return _fallo; }
54:   void  resetVolumen()        { _vol = 0.0f; }
55:   const char* nombre()  const { return _nombre; }
```
* **Líneas 49-55**: Métodos consultores (*getters*) constantes (`const`), lo que garantiza que no modifican el estado interno del objeto al ser leídos.

```cpp
58: private:
59:   static void IRAM_ATTR isrPuente(void* arg);
60:   const uint8_t _pin;
61:   const float   _k;
62:   const char*   _nombre;
63:   volatile uint32_t _pulsos = 0, _t_ultimo = 0;
64:   float _f = 0.0f, _q = 0.0f, _vol = 0.0f;
65:   uint8_t _segSinPulso = 0;
66:   bool  _fallo = false;
67:   static portMUX_TYPE _mux;
```
* **Líneas 58-67**: Miembros privados. Se resalta el calificador `volatile` en `_pulsos` y `_t_ultimo`, que le indica al compilador que no optimice esas variables en registros de CPU porque cambian de forma asíncrona dentro de una interrupción de hardware.

### `caudalimetro.cpp`:
```cpp
1: #include "caudalimetro.h"
3: portMUX_TYPE Caudalimetro::_mux = portMUX_INITIALIZER_UNLOCKED;
5: void IRAM_ATTR Caudalimetro::isrPuente(void* arg) {
6:   Caudalimetro* c = reinterpret_cast<Caudalimetro*>(arg);
7:   uint32_t t = micros();
8:   if (t - c->_t_ultimo >= FILTRO_RUIDO_US) {
9:     portENTER_CRITICAL_ISR(&_mux);
10:     c->_pulsos++;
11:     portEXIT_CRITICAL_ISR(&_mux);
12:     c->_t_ultimo = t;
13:   }
14: }
```
* **Línea 5**: `IRAM_ATTR` coloca el binario de la función en la memoria RAM interna de alta velocidad.
* **Línea 6**: `reinterpret_cast` convierte el puntero genérico `void*` de vuelta a la clase `Caudalimetro*`.
* **Líneas 8-12**: Si pasaron más de $3000\,\mu\text{s}$ desde el pulso anterior, incrementa `_pulsos` de manera atómica con `portENTER_CRITICAL_ISR` y guarda la marca de tiempo `t`.

---

## Módulo 3: `bomba.h`
```cpp
10: class Bomba {
11: public:
12:   void begin() {
13:     pinMode(PIN_DIR, OUTPUT);
14:     fijarSentido(true);
15:     ledcAttach(PIN_PUL, 800, 10);
16:     ledcWrite(PIN_PUL, 0);
17:   }
```
* **Líneas 12-17**: Inicializa el pin de dirección como salida y vincula el pin de pulsos al temporizador de hardware LEDC con frecuencia inicial de $800\text{ Hz}$ y 10 bits de resolución. `ledcWrite(PIN_PUL, 0)` asegura reposo sin tensión.

```cpp
19:   void arrancar()        { _enMarcha = true; }
20:   void detener()         { _enMarcha = false; _invirtiendo = false; }
21:   void setRPM(float rpm) { _objetivo = constrain(rpm, RPM_MIN, RPM_MAX); }
```
* **Líneas 19-21**: Comandos de control. `setRPM` restringe la consigna dentro de los límites estrictos de seguridad de la planta ($72-140\text{ RPM}$) mediante `constrain`.

```cpp
23:   void toggleSentido() {
24:     if (_invirtiendo) return;
25:     if (_actual < 5.0f) fijarSentido(!_horario);
26:     else {
27:       _invirtiendo = true;
28:       _rpmGuardada = _objetivo;
29:       _objetivo = 0.0f;
30:     }
31:   }
```
* **Líneas 23-31**: Secuencia protegida de inversión de giro. Si el motor está en movimiento, frena primero a $0\text{ RPM}$ por rampa antes de activar el sentido inverso, previniendo sobrecargas mecánicas en la manguera y el motor.

```cpp
40:   void tick(float dt) {
41:     float objetivo = _enMarcha ? _objetivo : 0.0f;
42:     if      (_actual < objetivo) _actual = min(objetivo, _actual + ACEL_RPM_S * dt);
43:     else if (_actual > objetivo) _actual = max(objetivo, _actual - ACEL_RPM_S * dt);
```
* **Líneas 40-43**: Perfil cinemático de aceleración. Modula `_actual` con una tasa constante de $40\text{ RPM/s}$ multiplicada por el paso de tiempo discreto $dt$.

```cpp
45:     if (_invirtiendo && _actual <= 0.1f) {
46:       fijarSentido(!_horario);
47:       _invirtiendo = false;
48:       _objetivo = _rpmGuardada;
49:     }
```
* **Líneas 45-49**: Transición de inversión: una vez que el motor se detuvo ($< 0.1\text{ RPM}$), conmuta la señal física del pin `DIR` y restablece la velocidad deseada en el nuevo sentido.

```cpp
51:     if (_actual >= 5.0f) {
52:       uint32_t f = (uint32_t)(_actual * PULSOS_POR_REV / 60.0f);
53:       if (f != _fActual) { ledcChangeFrequency(PIN_PUL, f, 10); _fActual = f; }
54:       ledcWrite(PIN_PUL, 512);
55:     } else if (_fActual != 0) {
56:       ledcWrite(PIN_PUL, 0);
57:       _fActual = 0;
58:     }
59:   }
```
* **Línea 52**: Convierte la velocidad instantánea a pulsos por segundo ($f = \frac{\text{RPM} \times 1600}{60}$).
* **Línea 53**: `ledcChangeFrequency` altera el registro del temporizador de hardware sin detener el microprocesador.
* **Línea 54**: `ledcWrite(PIN_PUL, 512)` ajusta el ciclo de trabajo al 50% ($512/1024$), generando pulsos simétricos ideales para los optoacopladores del driver DM860.
* **Líneas 55-58**: Si la velocidad es menor a $5\text{ RPM}$, apaga la generación de pulsos y pone la salida a tierra ($0\text{V}$).

```cpp
62:   void fijarSentido(bool horario) {
63:     _horario = horario;
64:     digitalWrite(PIN_DIR, horario ? LOW : HIGH);
65:   }
```
* **Líneas 62-65**: Control del pin `DIR` en esquema de Cátodo Común: `LOW` = Sentido Horario (Filtración normal); `HIGH` = Sentido Antihorario (Retrolavado).

---

## Módulo 4: `subhito2_2_v2.ino`
```cpp
14: Caudalimetro sensorFeed(PIN_SENSOR_FEED, K_FEED, "FEED");
15: Caudalimetro sensorPerm(PIN_SENSOR_PERM, K_PERM, "PERMEADO");
16: Bomba bomba;
17: WebServer server(80);
```
* **Líneas 14-17**: Creación de las instancias del sistema y del servidor HTTP en el puerto estándar 80.

```cpp
22: void manejarRaiz()   { server.send_P(200, "text/html", INDEX_HTML); }
23: void manejarSet()    { if (server.hasArg("rpm")) bomba.setRPM(server.arg("rpm").toFloat()); server.send(200, "text/plain", "OK"); }
25: void manejarCmd() { ... }
```
* **Líneas 22-32**: Controladores de peticiones HTTP (Endpoints REST):
  * `/`: Entrega la página web SCADA almacenada en Flash (`send_P`).
  * `/set?rpm=XX`: Modifica la consigna de velocidad de la bomba.
  * `/cmd?act=START|STOP|DIR|RESET_VOL`: Ejecuta las maniobras de marcha, parada, inversión de giro o puesta a cero de contadores volumétricos.

```cpp
34: void manejarStatus() {
...
39:   snprintf(j, sizeof(j),
40:     "{\"on\":%d,\"inv\":%d,\"dir\":%d,\"rpm\":%.1f,\"pump_ml\":%.1f,"
41:     "\"f_feed\":%.1f,\"q_feed\":%.1f,\"vol_feed\":%.3f,\"feed_ok\":%d,"
42:     "\"f_perm\":%.1f,\"q_perm\":%.1f,\"vol_perm\":%.3f,\"perm_ok\":%d,"
43:     "\"q_ret\":%.1f,\"recov\":%.1f,\"delta\":%.1f,\"ip\":\"%s\"}", ...
```
* **Líneas 34-50**: Endpoint de telemetría `/status`. Empaqueta en formato JSON de alta velocidad todas las variables de proceso para consumo del navegador.

```cpp
60:   // 1. Conectar a Router primero (15s timeout) para fijar el canal Wi-Fi
61:   WiFi.mode(WIFI_AP_STA);
62:   WiFi.begin(SSID_STA, PASS_STA);
...
72:   WiFi.softAP(SSID_AP, PASS_AP);
```
* **Líneas 60-72**: Configuración Wi-Fi robusta: primero sincroniza el módem de radio con el canal RF del router `Box804`. Luego habilita el punto de acceso propio `Bomba_Peristaltica_UF`, asegurando que ambos operen en la misma frecuencia sin pérdidas de paquetes.

```cpp
103:   float dt = (t - tLoop) / 1000.0f;
104:   if (dt >= 0.05f) {
105:     tLoop = t;
106:     bomba.tick(dt);
107:     volBomba_L += (bomba.caudalTeorico_mLmin() / 60000.0f) * dt;
108:   }
```
* **Líneas 103-108**: Bucle periódico a $20\text{ Hz}$ ($50\text{ ms}$) para el cálculo de la rampa cinemática del motor e integración del volumen teórico de bombeo.

```cpp
111:   if (t - tCaudal >= 1000) {
112:     float dtc = (t - tCaudal) / 1000.0f;
113:     tCaudal = t;
114:     bool empuja = bomba.caudalTeorico_mLmin() > 150.0f;
115:     sensorFeed.actualizar(dtc, empuja);
116:     sensorPerm.actualizar(dtc, empuja);
```
* **Líneas 111-116**: Bucle de instrumentación a $1\text{ Hz}$ ($1000\text{ ms}$). Muestra los pulsos de los sensores y actualiza los caudales y volúmenes acumulados.

```cpp
121:     qRet_mLmin   = max(0.0f, qF - qP);
122:     recuperacion = (qF > 20.0f) ? 100.0f * qP / qF : 0.0f;
123:     deltaBomba   = (qT > 20.0f) ? 100.0f * (qF - qT) / qT : 0.0f;
```
* **Líneas 121-123**: Cálculos del balance de materia en la membrana de ultrafiltración:
  * Caudal de Retentado: $Q_{\text{ret}} = Q_{\text{feed}} - Q_{\text{perm}}$.
  * Tasa de Recuperación: $Y = \frac{Q_{\text{perm}}}{Q_{\text{feed}}} \times 100\%$.
  * Discrepancia volumétrica de la bomba contra el sensor medido: $\Delta\%$.

---

## Módulo 5: `index_html.h`
```cpp
2: const char INDEX_HTML[] PROGMEM = R"html(<!DOCTYPE html>
...
)html";
```
* Almacena en memoria de programa Flash la interfaz de usuario SCADA.
* Utiliza una cuadrícula responsiva basada en CSS moderno optimizado para pantallas táctiles de celulares y computadoras de laboratorio.
* El script interno ejecuta cada $500\text{ ms}$ la función `fetch('/status')`, procesa el JSON recibido y refresca dinámicamente las etiquetas del DOM sin parpadeo y sin recargar la página.

---

# 5. Exportación de Datos a Excel (Tiempos, Sentidos y Volúmenes)

### ¿Se puede volcar a Excel con el tiempo en horario, retrolavado, cambios, etc.?
**Totalmente SÍ**. En ingeniería de procesos esto se denomina **Datalogging**.

Podemos implementarlo de dos formas directas:

### Opción A (La más práctica - Desde el Dashboard Web con botón "Descargar CSV"):
Podemos incorporar en la propia página web un arreglo en memoria JavaScript que guarde un registro cada segundo o cada vez que cambia un estado (arranque, cambio a retrolavado, parada).  
Al presionar un botón **"📥 Descargar CSV para Excel"**, el navegador genera automáticamente un archivo `.csv` (valores separados por coma) con la siguiente estructura tabular:

| Timestamp (s) | Fecha/Hora | Estado | Sentido | RPM | Q_Feed (mL/min) | Q_Perm (mL/min) | Q_Ret (mL/min) | Vol_Feed (L) | Vol_Perm (L) | Y (%) |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| 10 | 14:30:10 | MARCHA | FILTRACIÓN | 72.0 | 301.2 | 45.1 | 256.1 | 0.050 | 0.007 | 15.0 |
| 120 | 14:32:00 | INVERSIÓN | FRENANDO | 35.0 | 150.0 | 20.0 | 130.0 | 0.600 | 0.090 | 13.3 |
| 125 | 14:32:05 | MARCHA | RETROLAVADO | 80.0 | 0.0 | 320.0 | 0.0 | 0.600 | 0.115 | 0.0 |

Al abrir ese archivo `.csv` con Microsoft Excel, se separan automáticamente las columnas y los tesistas pueden graficar:
1. Curvas de flujo de permeado en función del tiempo $J_p(t)$.
2. Duración de los ciclos de filtración vs. ciclos de retrolavado (*backwash*).
3. Volumen total purificado y balance de masa integrado.

---

# 6. Protocolo Experimental de Calibración con Probeta

Para validar con rigor los dos factores empíricos (`ML_POR_VUELTA` y `K_FEED`), los alumnos deben realizar este ensayo de 60 segundos:

```
[Bomba MBP-2000] ──► [Sensor YF-S401 FEED] ──► [Manguera libre] ──► [Probeta Graduada 1000 mL]
```

### Pasos:
1. Purgar la manguera con agua destilada o corriente para expulsar todas las burbujas de aire.
2. Descargar libremente a presión atmosférica dentro de la probeta graduada.
3. Presionar en el dashboard web **"Reset L"** (pone volumen a cero).
4. Configurar la bomba a **$72\text{ RPM}$**.
5. Con cronómetro en mano, arrancar la bomba durante exactamente **$60\text{ segundos}$** y detenerla.

### Análisis de Resultados:
1. **Volumen real recogido en probeta ($V_{\text{probeta}}$ en mL)**:
   * **Cálculo del desplazamiento real de la manguera**:
     $$\text{ML\_POR\_VUELTA}_{\text{real}} = \frac{V_{\text{probeta}}}{72\text{ vueltas}}$$
     *(Si recogieron $302\text{ mL}$, el valor es exactamente $4.2\text{ mL/vuelta}$)*.
2. **Volumen reportado en la web ($V_{\text{web}}$ en Litros)**:
   * Convertir la probeta a Litros ($V_{\text{probeta, L}} = V_{\text{probeta}} / 1000$).
   * Si la web muestra una discrepancia significativa:
     $$K_{\text{calibrado}} = K_{\text{actual}} \times \left(\frac{V_{\text{web}}}{V_{\text{probeta, L}}}\right)$$
   * Actualizar el valor resultante en `config.h` para que el error de medición sea $< 1\%$.
