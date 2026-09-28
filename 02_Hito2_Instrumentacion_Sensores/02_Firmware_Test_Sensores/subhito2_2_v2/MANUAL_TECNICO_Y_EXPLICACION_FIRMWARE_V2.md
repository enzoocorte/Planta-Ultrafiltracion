# 📘 MANUAL TÉCNICO Y EXPLICACIÓN INTEGRAL DEL FIRMWARE `subhito2_2_v2`
## Planta Piloto de Ultrafiltración FX100 • Subhito 2.1: Instrumentación y Control de Caudal
**Codirector**: Ing. Enzo  
**Tesistas**: Antonella Guitián & Owen Cañizares  
**Carrera**: Ingeniería Industrial / Química — Universidad Nacional de Salta  
**Fecha de Actualización**: Septiembre 2026  
**Ubicación del Firmware**: [`02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v2/`](./)

---

# 📑 ÍNDICE GENERAL

1. [Arquitectura General y Filosofía de Diseño Industrial](#1-arquitectura-general-y-filosofía-de-diseño-industrial)
2. [Fundamentos Físicos, Hidráulicos y Cinemáticos](#2-fundamentos-físicos-hidráulicos-y-cinemáticos)
   - [2.1. Geometría Real de la Manguera y Desplazamiento Volumétrico (12 mm vs 4.8 mm)](#21-geometría-real-de-la-manguera-y-desplazamiento-volumétrico-12-mm-vs-48-mm)
   - [2.2. La Paradoja de Contrapresión: Descarga Libre (1200 mL/min) vs Membrana (150 mL/min)](#22-la-paradoja-de-contrapresión-descarga-libre-1200-mlmin-vs-membrana-150-mlmin)
   - [2.3. Principio del Caudalímetro YF-S401 ($K = 98.0$, 5880 pulsos/L)](#23-principio-del-caudalímetro-yf-s401-k--980-5880-pulsosl)
   - [2.4. Hallazgo Crítico del Filtro de Software: 12000 µs vs 3000 µs](#24-hallazgo-crítico-del-filtro-de-software-12000-µs-vs-3000-µs)
   - [2.5. Cinemática del Accionamiento DM860 (1600 vs 3200 micropasos)](#25-cinemática-del-accionamiento-dm860-1600-vs-3200-micropasos)
3. [Diagnóstico Eléctrico: ¿Por qué Fallaba Antes y Cómo lo Resuelve la Placa 2?](#3-diagnóstico-eléctrico-por-qué-fallaba-antes-y-cómo-lo-resuelve-la-placa-2)
   - [3.1. La Falla del Divisor Resistivo Anterior (10 kΩ serie + 4.2V)](#31-la-falla-del-divisor-resistivo-anterior-10-kω-serie--42v)
   - [3.2. Circuito Front-End Antirruido Homologado (Placa 2: RC pasabajos $f_c \approx 338\text{ Hz}$)](#32-circuito-front-end-antirruido-homologado-placa-2-rc-pasabajos-f_c-approx-338text-hz)
4. [Análisis Exhaustivo Módulo por Módulo (Línea por Línea)](#4-análisis-exhaustivo-módulo-por-módulo-línea-por-línea)
   - [4.1. Módulo 1: `config.h`](#41-módulo-1-configh)
   - [4.2. Módulo 2: `caudalimetro.h` y `caudalimetro.cpp`](#42-módulo-2-caudalimetroh-y-caudalimetrocpp)
   - [4.3. Módulo 3: `bomba.h`](#43-módulo-3-bombah)
   - [4.4. Módulo 4: `subhito2_2_v2.ino`](#44-módulo-4-subhito2_2_v2ino)
   - [4.5. Módulo 5: `index_html.h`](#45-módulo-5-index_htmlh)
5. [Auditoría del Código: Detección de "Cosas Raras" y Correcciones de Ingeniería](#5-auditoría-del-código-detección-de-cosas-raras-y-correcciones-de-ingeniería)
6. [Protocolo Experimental de Pruebas de Laboratorio para el Lunes](#6-protocolo-experimental-de-pruebas-de-laboratorio-para-el-lunes)

---

# 1. Arquitectura General y Filosofía de Diseño Industrial

En sistemas de grado industrial y proyectos de tesis de ingeniería, el software embebido debe estructurarse bajo el principio de **Separación de Responsabilidades** (*Separation of Concerns*). El firmware `subhito2_2_v2` se compone de cinco módulos desacoplados:

```
                              ┌─────────────────────────┐
                              │        config.h         │
                              │ (Parámetros de Proceso) │
                              └────────────┬────────────┘
                                           │
                    ┌──────────────────────┼──────────────────────┐
                    ▼                      ▼                      ▼
         ┌────────────────────┐ ┌────────────────────┐ ┌────────────────────┐
         │      bomba.h       │ │caudalimetro.h/.cpp │ │   index_html.h     │
         │ Control Cinemático │ │ Abstracción Sensor │ │  Dashboard SCADA   │
         │  y Hardware LEDC   │ │  + Filtros ISR/IIR │ │  (PROGMEM Flash)   │
         └──────────┬─────────┘ └──────────┬─────────┘ └──────────┬─────────┘
                    │                      │                      │
                    └──────────────────────┼──────────────────────┘
                                           ▼
                              ┌─────────────────────────┐
                              │    subhito2_2_v2.ino    │
                              │ Orquestador / FreeRTOS  │
                              │ Balance Masa + Red Wi-Fi│
                              └─────────────────────────┘
```

### Principios Fundamentales del Sistema:
1. **Ejecución No Bloqueante (`Non-blocking design`)**: Prohibición de la función `delay()` en el lazo continuo. Las tareas se calendarizan con marcas de tiempo diferenciales ($\Delta t = t_{\text{actual}} - t_{\text{anterior}}$) utilizando `millis()` y `micros()`.
2. **Generación de Pasos por Hardware (`LEDC`)**: El ESP32 no conmuta pines de pulsos por software. Delega la onda cuadrada al temporizador de silicio periférico `LEDC`, liberando ambos núcleos Tensilica LX6 de jitter temporal.
3. **Secciones Críticas Atómicas (`FreeRTOS Spinlocks`)**: Dado que el ESP32 es dual-core y los pulsos ingresan por interrupciones de hardware asíncronas (`ISR`), la lectura y reinicio de contadores se protege con cerrojos de giro atómicos (`portENTER_CRITICAL` / `portEXIT_CRITICAL`).
4. **Cero Consumo de Memoria SRAM para la Web (`PROGMEM`)**: Toda la interfaz HTML5, estilos CSS3, motor JavaScript de telemetría y exportador CSV residen en la memoria Flash ($4\text{ MB}$), dejando intactos los $320\text{ KB}$ de SRAM para buffers de red y variables de control.

---

# 2. Fundamentos Físicos, Hidráulicos y Cinemáticos

---

## 2.1. Geometría Real de la Manguera y Desplazamiento Volumétrico (12 mm vs 4.8 mm)

Una bomba peristáltica es una **máquina de desplazamiento positivo volumétrico**. El fluido es impulsado porque los rodillos del rotor comprimen (ocluyen) la manguera flexible contra la pista semicircular del cabezal, desplazando un volumen confinado en cada ciclo.

### Datos Reales del Conjunto Motor-Bomba (Laboratorio de Salta):
* **Bomba Peristáltica**: Modelo **MBP-2000** (Caudal máximo nominal: $2.0\text{ L/min} = 2000\text{ mL/min}$).
* **Cuerpo**: Polietileno de Alto Peso Molecular (APM), atóxico, inodoro y resistente a corrosión química.
* **Tubo de Silicona Flexible**:
  * Diámetro Exterior: $\varnothing_{\text{ext}} = 18\text{ mm}$
  * Diámetro Interior: $\varnothing_{\text{int}} = 12\text{ mm}$ ($1.2\text{ cm}$)
* **Motor Paso a Paso**: NEMA 34, Torque $4.0\text{ N}\cdot\text{m}$, Corriente nominal $6.0\text{ A}$, Inductancia $4\text{ mH}$, Longitud de carcasa $100\text{ mm}$.

### Comparación Matemática: ¿Por qué 4.2 mL/vuelta estaba mal y 15.4 mL/vuelta es lo correcto?

El área de la sección transversal interna del tubo determina directamente el volumen atrapado por centímetro de pista:

$$A_{\text{tubo}} = \frac{\pi \times d_i^2}{4}$$

* **Caso Anterior (Manguera Fina de Laboratorio Clínico, $d_i = 4.8\text{ mm}$)**:
  $$A_{\text{tubo}} = \frac{\pi \times (0.48\text{ cm})^2}{4} = 0.181\text{ cm}^2 = 0.181\text{ mL/cm}$$
  Para una pista perimetral de $23\text{ cm}$ y 2 rodillos:
  $$V_{\text{rev}} \approx 23\text{ cm} \times 0.181\text{ mL/cm} \approx \mathbf{4.16 \approx 4.2\text{ mL/vuelta}}$$
  *(Este valor correspondía al prototipo teórico con manguera delgada de 4.8 mm)*.

* **Caso Real Actual (Manguera MBP-2000, $d_i = 12.0\text{ mm}$)**:
  $$A_{\text{tubo}} = \frac{\pi \times (1.20\text{ cm})^2}{4} = \mathbf{1.131\text{ cm}^2} = \mathbf{1.131\text{ mL/cm}}$$
  ¡El área interna es **6.25 veces mayor** que la de 4.8 mm!
  El volumen teórico geométrico desplazado por vuelta resulta:
  $$V_{\text{geom}} = L_{\text{efectiva}} \times A_{\text{tubo}} \approx 15\text{ a }18\text{ mL/vuelta}$$

### Demostración con la Prueba Experimental Real de Enzo:
En el ensayo de probeta a descarga libre realizado por Enzo:
* Velocidad: $72\text{ RPM}$
* Tiempo: $1\text{ minuto}$ ($60\text{ segundos}$)
* Volumen recolectado en probeta: **$1200\text{ mL}$**

Calculando el desplazamiento real directo:
$$\text{Desplazamiento Real} = \frac{1200\text{ mL}}{72\text{ vueltas}} = \mathbf{16.66\text{ mL/vuelta}}$$

Considerando una ligera deformación por aplastamiento elástico de la silicona y pérdidas dinámicas, fijamos inicialmente en el código:
$$\mathbf{ML\_POR\_VUELTA = 15.4\text{ mL/vuelta}}$$
*(Ajustable exactamente a 16.6 o al valor promedio que arroje el ensayo de calibración en probeta P6)*.

---

## 2.2. La Paradoja de Contrapresión: Descarga Libre (1200 mL/min) vs Membrana (150 mL/min)

En las pruebas preliminares surgió una discrepancia desconcertante:
1. Con la manguera libre (sin conectar al filtro), a $72\text{ RPM}$ se recolectaron **$1200\text{ mL/min}$**.
2. Con la membrana conectada y la bomba a $72\text{ RPM}$, la probeta sólo capturó **$150\text{ mL/min}$**.

### Explicación Hidráulica:
El cartucho **Fresenius FX100** contiene miles de microfibras capilares de polisulfona *Helixone* con poros de ultrafiltración ($< 0.01\,\mu\text{m}$).  
* Si la válvula de la línea de **retentado** está cerrada o muy estrangulada, el fluido no puede salir libremente por el extremo axial de las fibras.
* El agua se ve forzada a atravesar la pared porosa de las fibras hacia la carcasa de permeado, lo cual ofrece una **altísima resistencia hidráulica (pérdida de carga)**.
* Aunque las bombas peristálticas son de desplazamiento positivo, las mangueras de silicona son elásticas. Ante presiones elevadas, la manguera se expande antes del rodillo y el rodillo no logra un sellado hermético al 100% (*slip* o retroflujo interno por contrapresión).
* **Solución operativa**: Para la calibración inicial con agua, la válvula de retentado debe permanecer **completamente abierta**, permitiendo el flujo cruzado sin generar contrapresiones excesivas.

---

## 2.3. Principio del Caudalímetro YF-S401 ($K = 98.0$, 5880 pulsos/L)

El microcaudalímetro YF-S401 dispone de una pequeña turbina de flujo tangencial con imanes embebidos y un sensor de efecto Hall integrado.

### Ecuación de Calibración Nominal:
$$F = K \times Q$$
Donde:
* $F$: Frecuencia en Hertz ($\text{pulsos/segundo}$).
* $Q$: Caudal volumétrico en Litros por minuto ($\text{L/min}$).
* $K = 98.0$: Constante característica del sensor.

### Constante de Pulsos por Litro:
A un caudal de $1\text{ L/min}$, el sensor emite $98\text{ pulsos/segundo}$. En $1\text{ minuto}$ ($60\text{ s}$):
$$\text{Pulsos/Litro} = 98 \times 60 = \mathbf{5880\text{ pulsos/Litro}}$$
Cada pulso individual equivale exactamente a:
$$V_{\text{pulso}} = \frac{1000\text{ mL}}{5880} \approx \mathbf{0.17007\text{ mL/pulso}}$$

### Cálculo del Caudal Instantáneo en mL/min:
$$Q\text{ (L/min)} = \frac{F}{K} \implies Q\text{ (mL/min)} = \frac{F \times 1000}{K}$$

---

## 2.4. El Dilema del Filtro de Software: Ancho de Banda vs Inmunidad al Ruido

El ajuste de `FILTRO_RUIDO_US` es un caso clásico de ingeniería de control y procesamiento de señales: el **compromiso (*trade-off*) entre inmunidad al ruido electromagnético y ancho de banda útil de medición**.

### Tabla de Análisis del Compromiso (Trade-Off):

| Consigna RPM | Caudal Real ($15.4\text{ mL/rev}$) | Frecuencia Turbina YF-S401 | Período entre Pulsos | ¿Pasa el Filtro de 12 ms (12000 µs)? | ¿Pasa el Filtro de 4.5 ms (4500 µs)? |
| :---: | :---: | :---: | :---: | :---: | :---: |
| **20 RPM** | $308\text{ mL/min}$ | $30.2\text{ Hz}$ | $33.1\text{ ms}$ | ✅ Pasa perfecto ($> 12\text{ ms}$) | ✅ Pasa holgado |
| **36 RPM** (límite FX100) | $554\text{ mL/min}$ | $54.3\text{ Hz}$ | $18.4\text{ ms}$ | ✅ Pasa perfecto ($> 12\text{ ms}$) | ✅ Pasa holgado |
| **50 RPM** | $770\text{ mL/min}$ | $75.5\text{ Hz}$ | $13.2\text{ ms}$ | ✅ Pasa justo ($> 12\text{ ms}$) | ✅ Pasa holgado |
| **55+ RPM** | $> 850\text{ mL/min}$ | $> 83.3\text{ Hz}$ | $< 12.0\text{ ms}$ | ❌ **Recorta pulsos** (techo ~850 mL/min) | ✅ Pasa holgado |
| **72 RPM** (ensayo libre) | $1108\text{ a }1200\text{ mL/min}$ | $108.6\text{ a }117.6\text{ Hz}$ | $8.5\text{ a }9.2\text{ ms}$ | ❌ Sub-cuenta a ~850 mL/min | ✅ Pasa perfecto ($> 4.5\text{ ms}$) |
| **100 RPM** (máx. exploratorio) | $1540\text{ mL/min}$ | $150.9\text{ Hz}$ | $6.6\text{ ms}$ | ❌ Sub-cuenta a ~850 mL/min | ✅ Pasa holgado ($> 4.5\text{ ms}$) |

### Estrategia Operativa Homologada:
1. **Fase 1 (Validación Inicial en Banco — Pruebas P1 a P10)**:  
   Se mantiene configurado **`FILTRO_RUIDO_US = 12000` ($12\text{ ms}$)**.  
   * **Justificación**: Para el rango de operación nominal de la planta de ultrafiltración ($\le 36\text{ RPM} = 554\text{ mL/min} = 54.3\text{ Hz}$), el período entre pulsos físicos ($18.4\text{ ms}$) es mayor a $12\text{ ms}$, por lo que los pulsos pasan sin atenuación. A su vez, bloquea al 100% cualquier acoplamiento parásito de $108\text{ Hz}$ del motor NEMA 34.
   * **Nota**: Por encima de $\sim 55\text{ RPM}$, el caudalímetro alcanzará su techo de $\sim 850\text{ mL/min}$. En esa zona de ensayo exploratorio, la referencia es el caudal teórico de bomba + medición por probeta.
2. **Fase 2 (Tras validar que la Prueba P5 dé PERMEADO = 0.0 Hz con motor girando en seco)**:  
   Una vez confirmado en mesa de trabajo que el hardware de la Placa 2 (filtro RC $4.7\text{ k}\Omega + 100\text{ nF}$) extinguió completamente las espigas del motor, se cambia en `config.h` a:
   ```cpp
   constexpr uint32_t FILTRO_RUIDO_US = 4500;  // Techo 222 Hz (~2260 mL/min) -> Cubre 100 RPM con margen
   ```

---

## 2.5. Cinemática del Accionamiento DM860 (1600 vs 3200 micropasos)

Para controlar el NEMA 34 con suavidad y evitar resonancias mecánicas a bajas velocidades, el driver Leadshine DM860 divide cada paso físico de $1.8^\circ$ ($200\text{ pasos/vuelta}$) en micropasos:

$$\text{PULSOS\_POR\_REV} = 200 \times \text{Micropasos}$$

$$\text{Frecuencia de Pulsos } f_{\text{LEDC}} (\text{Hz}) = \frac{\text{RPM} \times \text{PULSOS\_POR\_REV}}{60}$$

### Tabla de Frecuencias Cinemáticas según Configuración:

| Velocidad Consigna | Caudal Teórico ($15.4\text{ mL/rev}$) | Frecuencia con 1600 p/rev ($\times 8$) | Frecuencia con 3200 p/rev ($\times 16$) |
| :---: | :---: | :---: | :---: |
| **20 RPM** (mínimo) | $308\text{ mL/min}$ | $533.3\text{ Hz}$ | $1066.7\text{ Hz}$ |
| **25 RPM** (arranque) | $385\text{ mL/min}$ | $666.7\text{ Hz}$ | $1333.3\text{ Hz}$ |
| **36 RPM** (límite FX100) | $554.4\text{ mL/min}$ | $960.0\text{ Hz}$ | $1920.0\text{ Hz}$ |
| **50 RPM** | $770\text{ mL/min}$ | $1333.3\text{ Hz}$ | $2666.7\text{ Hz}$ |
| **72 RPM** (ensayo anterior) | $1108.8\text{ mL/min}$ | $1920.0\text{ Hz}$ | $3840.0\text{ Hz}$ |
| **80 RPM** | $1232\text{ mL/min}$ | $2133.3\text{ Hz}$ | $4266.7\text{ Hz}$ |
| **100 RPM** (máx. exploratorio) | $1540\text{ mL/min}$ | $2666.7\text{ Hz}$ | $5333.3\text{ Hz}$ |

---

# 3. Diagnóstico Eléctrico: ¿Por qué Fallaba Antes y Cómo lo Resuelve la Placa 2?

---

## 3.1. La Falla del Divisor Resistivo Anterior (10 kΩ serie + 4.2V)

En los primeros ensayos se observaron tres síntomas anómalos:
1. Al soplar en el sensor con el motor apagado, marcaba $0\text{ pulsos}$.
2. Con el motor girando a $72\text{ RPM}$ y la manguera de permeado desconectada (cero agua), el display marcaba $1100\text{ mL/min}$ en permeado.
3. Se medían $4.2\text{ V}$ en el cable amarillo.

### ¿Qué estaba ocurriendo internamente en el circuito?
El circuito anterior tenía una resistencia de $10\text{ k}\Omega$ en serie en el cable amarillo y el microcontrolador activaba `pinMode(pin, INPUT_PULLUP)` (con su resistencia interna a $3.3\text{V}$ de $\approx 45\text{ k}\Omega$).

```
               +5V (Sensor YF-S401)
                │
                │ [Pull-up interno del clon chino: ~10kΩ]
                ▼
          Cable Amarillo (4.2V medidos)
                │
                ├─── [Resistencia Serie 10kΩ] ─── Borne GPIO ESP32
                │                                        │
           ┌────┴────┐                              [Pull-up Interno 45kΩ]
           │ NPN     │                                   │
           │ Sensor  │                                  +3.3V
           └────┬────┘
               GND
```

1. **El divisor impidió llegar al cero lógico ($0\text{ V}$)**:  
   Cuando la turbina giraba y el transistor Hall NPN conducía a masa para emitir un pulso, la resistencia de $10\text{ k}\Omega$ en serie formaba un divisor resistivo con el pull-up interno del ESP32.  
   La tensión mínima en el pin del ESP32 no bajaba a $0\text{ V}$, sino que quedaba clavada en:
   $$V_{\text{pin, LOW}} = 3.3\text{ V} \times \frac{10\text{ k}\Omega}{10\text{ k}\Omega + 45\text{ k}\Omega} \approx 0.60\text{ a }0.85\text{ V}$$
   Como el umbral garantizado de nivel bajo ($V_{IL}$) del ESP32 es de máximo $0.8\text{ V}$, **la conmutación mecánica de la turbina jamás cruzaba el umbral de disparo**. Por eso, ¡al soplar suavemente no registraba nada!
2. **El acoplamiento electromagnético del motor**:  
   Con una línea de alta impedancia ($>10\text{ k}\Omega$), el cable actuó como una antena receptora. El motor NEMA 34 tiene 50 dientes de rotor conmutando corrientes de 6A. Al girar a $72\text{ RPM}$ ($1.2\text{ rev/s}$):
   $$f_{\text{armónicos}} = 50 \text{ dientes} \times 1.2\text{ rev/s} \times 2 \approx 120\text{ Hz}$$
   Las espigas inducidas de $120\text{ Hz}$ tenían amplitud suficiente para cruzar el umbral, haciendo que el firmware leyera falsamente $\approx 1100\text{ mL/min}$ en permeado ¡incluso con la manguera suelta en el aire!

---

## 3.2. Circuito Front-End Antirruido Homologado (Placa 2: RC pasabajos $f_c \approx 338\text{ Hz}$)

La arquitectura de dos placas shield ZS-1057 resuelve todos estos problemas sin una sola soldadura:

```
[ Sensor YF-S401 ]                     PLACA 2 (FRONT-END PASIVO)             PLACA 1 (ESP32 MASTER)
  Cable Rojo (5V)   ─────────────► [ Borne 5V ] ◄─────────────────────────── [ Borne 5V ESP32 ]
  Cable Negro (GND) ─────────────► [ Borne GND ] ◄────────────────────────── [ Borne GND ESP32 ]
                                         │
  Cable Amarillo    ─────────────► [ Borne P14 ] ────── Cable Directo ─────► [ GPIO 14 (INPUT) ]
  (Colector Abierto)                     │
                                   ┌─────┴───────────────┐
                                   │  Pull-up 4.7kΩ      │  Capacitor 100nF
                                   │  a [Borne 3.3V]     │  a [Borne GND]
                                   ▼                     ▼
                             (Nivel seguro 3.3V)   (Filtro RC pasabajos)
```

1. **Pull-up externo a 3.3V**: Al estar la resistencia de $4.7\text{ k}\Omega$ conectada a la barra de $3.3\text{V}$, en reposo el cable amarillo se mantiene en $3.3\text{ V}$ estables (protección total contra sobretensión).
2. **Conexión directa a GPIO (`INPUT`)**: Cuando el transistor del sensor conduce, la tensión cae a **$0.0\text{ V}$ sólido**. Al no haber resistencia en serie, el pulso digital es nítido y la prueba de soplido funciona al 100%.
3. **Filtro RC Pasabajos ($4.7\text{ k}\Omega + 100\text{ nF}$)**:  
   $$f_c = \frac{1}{2 \pi \times 4700 \times 100 \times 10^{-9}} \approx \mathbf{338.6\text{ Hz}}$$
   Deriva de forma pasiva a GND cualquier ruido electromagnético de alta frecuencia proveniente de los cables de potencia del motor NEMA 34.

---

# 4. Análisis Exhaustivo Módulo por Módulo (Línea por Línea)

---

## 4.1. Módulo 1: `config.h`

```cpp
1:  #pragma once
2:  // ============================================================
3:  //  CONFIGURACIÓN — Planta UF (aquí se cambia TODO)
4:  // ============================================================
```
* **Línea 1**: `#pragma once` evita la doble inclusión del archivo durante la compilación en diferentes unidades de traducción.

```cpp
6:  // ---- PINES ----
7:  // Driver DM860 en CÁTODO COMÚN: GPIO manda HIGH = opto ON (7.8 mA por pin)
8:  constexpr uint8_t PIN_PUL         = 18;  // DM860 PUL+ (PUL- a GND común)
9:  constexpr uint8_t PIN_DIR         = 19;  // DM860 DIR+ (DIR- a GND común)
10: constexpr uint8_t PIN_SENSOR_FEED = 14;  // Caudalímetro FEED
11: constexpr uint8_t PIN_SENSOR_PERM = 27;  // Caudalímetro PERMEADO
```
* **Líneas 8-11**: Declaran como constantes inmutables en tiempo de compilación (`constexpr`) los pines GPIO del ESP32 asignados a cada función. Usar `uint8_t` asegura que solo ocupen 1 byte en memoria.

```cpp
13: // ---- BOMBA PERISTÁLTICA ----
14: // Desplazamiento nominal manguera 12mm: 15.4 mL/rev (se recalibra en P6 con probeta)
15: constexpr uint16_t PULSOS_POR_REV   = 1600;      // ⚠️ IGUAL a los DIP SW5-SW8 del DM860 (3200 si 16 micropasos)
16: constexpr float ML_POR_VUELTA       = 15.4f;     // mL por giro del cabezal
17: constexpr float RPM_MIN             = 20.0f;     // 308 mL/min (mín. sensor: 300)
18: constexpr float RPM_MAX             = 100.0f;    // máx exploratorio con agua
19: constexpr float RPM_INICIO          = 25.0f;     // 385 mL/min
20: constexpr float ACEL_RPM_S          = 20.0f;     // rampa suave
```
* **Línea 15**: Define cuántos pulsos debe emitir el ESP32 para que el eje dé un giro completo. Debe coincidir con las llaves DIP SW5-SW8 del DM860. Si se sube a 16 micropasos, se cambia este valor a `3200`.
* **Línea 16**: Volumen en mililitros desplazado por revolución completa del cabezal con la manguera de 12 mm.
* **Línea 17**: Velocidad mínima permitida ($20\text{ RPM} \implies 308\text{ mL/min}$), garantizando superar el umbral de arranque de la turbina YF-S401 ($300\text{ mL/min}$).
* **Línea 18**: Velocidad máxima permitida ($100\text{ RPM} \implies 1540\text{ mL/min}$), otorgando margen para caracterización hidráulica con agua.
* **Línea 19**: Velocidad por defecto al encender el sistema ($25\text{ RPM} \implies 385\text{ mL/min}$).
* **Línea 20**: Pendiente de la rampa de aceleración/desaceleración ($20\text{ RPM/s}$), protegiendo la manguera contra picos mecánicos de presión.

```cpp
22: // Alarma blanda: a 36 RPM con manguera 12mm se alcanzan ~600 mL/min (límite FX100).
23: // Por encima: solo ensayos controlados con agua y retentado abierto.
24: constexpr float RPM_ALARMA_MEMBRANA = 36.0f;
```
* **Línea 24**: Umbral cinemático donde el caudal bombeado alcanza el límite seguro continuo del filtro de hemodiálisis Fresenius FX100 ($600\text{ mL/min}$).

```cpp
26: // ---- CAUDALÍMETROS YF-S401 ----
27: // F (Hz) = K × Q (L/min).  >>> Calibrar cada sensor por gravimetría <<<
28: constexpr float K_FEED = 98.0f;
29: constexpr float K_PERM = 98.0f;
30: 
31: // Filtro digital por software (ISR):
32: // • 12000 µs (12 ms): Máxima inmunidad contra el hum de 108 Hz durante validación inicial (P1 a P10).
33: //   Techo de medición: ~850 mL/min (~55 RPM). Para el rango nominal (≤ 36 RPM = ~554 mL/min) es perfecto.
34: // • 4500 µs (4.5 ms): Techo 222 Hz (~2260 mL/min / >100 RPM). Descomentar tras validar P5 limpia (PERM = 0.0 Hz).
35: constexpr uint32_t FILTRO_RUIDO_US = 12000;
36: // constexpr uint32_t FILTRO_RUIDO_US = 4500;  // Activar tras pasar P5
37: constexpr float Q_MAX_FISICO_MLMIN = 6000.0f;   // Límite físico YF-S401 (0.3 a 6 L/min). Permite prueba de soplido
```
* **Líneas 28-29**: Factores de calibración de turbina ($K$).
* **Líneas 31-36**: Tiempo de guarda mínimo entre pulsos consecutivos dentro de la interrupción ISR. Al fijar $12000\,\mu\text{s}$, protege al sistema de cualquier falso pulso a $108\text{ Hz}$ mientras se valida el banco en frío y a velocidades nominales ($\le 36\text{ RPM}$). Se deja lista la opción a $4500\,\mu\text{s}$ para cuando la prueba P5 confirme ruido cero.
* **Línea 37**: Límite superior de seguridad física ($6000\text{ mL/min}$). Si una ráfaga supera este valor, el firmware la clasifica como ruido y la descarta.

```cpp
37: // ---- WI-FI ----
38: constexpr const char* SSID_AP  = "Bomba_Peristaltica_UF";
39: constexpr const char* PASS_AP  = "plantapiloto2";
40: constexpr const char* SSID_STA = "Box804";
41: constexpr const char* PASS_STA = "plantapiloto2";
```
* **Líneas 38-41**: Credenciales de red para el Punto de Acceso autónomo (`AP`) y la red local del laboratorio (`STA`).

---

## 4.2. Módulo 2: `caudalimetro.h` y `caudalimetro.cpp`

### `caudalimetro.h`:
```cpp
10: class Caudalimetro {
11: public:
12:   Caudalimetro(uint8_t pin, float k, const char* nombre)
13:     : _pin(pin), _k(k), _nombre(nombre) {}
```
* **Líneas 10-13**: Declara la clase `Caudalimetro`. El constructor inicializa el pin GPIO, el factor $K$ y el identificador textual de diagnóstico.

```cpp
15:   void begin() {
16:     pinMode(_pin, INPUT);     // Pull-up externo de 4.7k a 3.3V en el front-end (Placa 2)
17:     // attachInterruptArg pasa 'this' a la ISR → cada sensor se auto-registra
18:     attachInterruptArg(digitalPinToInterrupt(_pin), isrPuente, this, FALLING);
19:   }
```
* **Línea 16**: Configura el pin en modo `INPUT` puro. No activa el pull-up interno del ESP32 porque el nivel alto lo sostiene la resistencia física de $4.7\text{ k}\Omega$ conectada a $3.3\text{V}$ en la Placa 2.
* **Línea 18**: Vincula la interrupción por hardware ante flancos descendentes (`FALLING`). La función `attachInterruptArg` pasa el puntero de la propia instancia (`this`), permitiendo que una única rutina ISR atienda a múltiples caudalímetros independientemente.

```cpp
21:   // Llamar 1 vez por segundo. bombaEmpuja: la bomba está moviendo líquido.
22:   void actualizar(float dt_s, bool bombaEmpuja) {
23:     portENTER_CRITICAL(&_mux);              // sección atómica (ESP32 es dual-core)
24:     uint32_t n = _pulsos;
25:     _pulsos = 0;
26:     portEXIT_CRITICAL(&_mux);
```
* **Líneas 23-26**: Captura atómica de pulsos mediante un spinlock (`_mux`). Deshabilita temporalmente las interrupciones en el núcleo, copia `_pulsos` a una variable local `n`, resetea el contador a cero y libera el cerrojo. Esto evita condiciones de carrera (*race conditions*) entre el Core 0 y el Core 1.

```cpp
28:     _f = (dt_s > 0.0f) ? ((float)n / dt_s) : 0.0f;     // Frecuencia física real en Hz (pulsos/seg)
29:     float q = (_f * 1000.0f) / _k;                      // mL/min instantáneo real: Q (L/min) = F/K -> mL/min = (F*1000)/K
30:     if (q > Q_MAX_FISICO_MLMIN) {                       // defensa anti-ruido por encima de 6 L/min
31:       q = 0.0f;
32:       Serial.printf("[%s] %lu pulsos falsos descartados (f=%.1f Hz)\n", _nombre, (unsigned long)n, _f);
33:     } else {
34:       _vol += (float)n / (_k * 60.0f);                  // volumen EXACTO por conteo de pulsos válidos
35:     }
36:     _q  = (n > 0) ? (0.3f * q + 0.7f * _q) : 0.0f;      // suavizado (estabiliza display, cae a 0 si se detiene)
```
* **Línea 28**: Calcula la frecuencia física instantánea $F = n / \Delta t$.
* **Línea 29**: Convierte la frecuencia a caudal volumétrico en $\text{mL/min}$ ($Q = \frac{F \times 1000}{K}$).
* **Líneas 30-35**: Filtro de sanidad física y totalización protegida: si el cálculo supera los $6000\text{ mL/min}$, anula el caudal espurio y **evita sumar esos pulsos falsos al acumulador de volumen** (`else`). Si los pulsos son válidos, acumula el volumen exacto en Litros ($\sum \frac{n}{5880}$).
* **Línea 36**: Filtro digital paso bajo recursivo (IIR de primer orden): $y[k] = 0.3 x[k] + 0.7 y[k-1]$. Suaviza el parpadeo en pantalla ante pulsaciones peristálticas normales. Si no hay pulsos ($n = 0$), cae a cero de inmediato.

```cpp
37:     // Detección de pérdida de flujo / cable suelto:
38:     // Si la bomba empuja pero transcurren 5 segundos continuos sin pulsos -> alerta
39:     if (n > 0) {
40:       _segSinPulso = 0;
41:       _fallo = false;
42:     } else if (bombaEmpuja) {
43:       if (++_segSinPulso >= 5) _fallo = true;
44:     } else {
45:       _segSinPulso = 0;
46:       _fallo = false;
47:     }
48:   }
```
* **Líneas 39-47**: Máquina de estados para diagnóstico de fallas: si la bomba está funcionando (`bombaEmpuja == true`) pero transcurren 5 segundos continuos con $0\text{ pulsos}$, levanta el flag `_fallo = true` (indicando burbuja de aire, manguera estrangulada o cable suelto). Con la bomba parada, el flag se mantiene apagado.

```cpp
50:   float caudal_mLmin()  const { return _q; }
51:   float caudal_Lmin()   const { return _q / 1000.0f; }
52:   float frecuencia_Hz() const { return _f; }
53:   float volumen_L()     const { return _vol; }
54:   bool  sinSenal()      const { return _fallo; }
55:   void  resetVolumen()        { _vol = 0.0f; }
56:   const char* nombre()  const { return _nombre; }
```
* **Líneas 50-56**: Métodos de consulta (*getters*) declarados como constantes (`const`), garantizando que su lectura jamás altera las variables internas del objeto.

```cpp
59:   static void IRAM_ATTR isrPuente(void* arg);
60: 
61:   const uint8_t _pin;
62:   const float   _k;
63:   const char*   _nombre;
64:   volatile uint32_t _pulsos = 0, _t_ultimo = 0;
65:   float _f = 0.0f, _q = 0.0f, _vol = 0.0f;
66:   uint8_t _segSinPulso = 0;
67:   bool  _fallo = false;
68:   static portMUX_TYPE _mux;                 // mutex compartido entre instancias
69: };
```
* **Líneas 61-68**: Atributos privados. Las variables `_pulsos` y `_t_ultimo` llevan el modificador `volatile` para forzar al compilador a leerlas siempre desde la memoria RAM física y nunca cachearlas en registros de CPU, ya que se modifican dentro de la interrupción.

### `caudalimetro.cpp`:
```cpp
1:  #include "caudalimetro.h"
2:  
3:  portMUX_TYPE Caudalimetro::_mux = portMUX_INITIALIZER_UNLOCKED;
4:  
5:  void IRAM_ATTR Caudalimetro::isrPuente(void* arg) {
6:    Caudalimetro* c = reinterpret_cast<Caudalimetro*>(arg);
7:    uint32_t t = micros();
8:    if (t - c->_t_ultimo >= FILTRO_RUIDO_US) {
9:      portENTER_CRITICAL_ISR(&_mux);
10:     c->_pulsos++;
11:     portEXIT_CRITICAL_ISR(&_mux);
12:     c->_t_ultimo = t;
13:   }
14: }
```
* **Línea 3**: Inicializa el cerrojo de exclusión mutua en estado desbloqueado.
* **Línea 5**: `IRAM_ATTR` instruye al compilador a ubicar el código de la interrupción en la memoria SRAM interna del ESP32 (evitando latencias de lectura desde la memoria Flash externa SPI).
* **Línea 6**: Reconstruye el puntero de la instancia mediante `reinterpret_cast`.
* **Líneas 8-13**: Compara el tiempo transcurrido en microsegundos contra `FILTRO_RUIDO_US` ($3000\,\mu\text{s}$). Si pasaron más de 3 ms, incrementa `_pulsos` de forma atómica y guarda la nueva marca de tiempo.

---

## 4.3. Módulo 3: `bomba.h`

```cpp
10: class Bomba {
11: public:
12:   void begin() {
13:     pinMode(PIN_DIR, OUTPUT);
14:     fijarSentido(true);
15:     ledcAttach(PIN_PUL, 800, 10);
16:     ledcWrite(PIN_PUL, 0);                 // reposo: LOW → opto OFF (motor libre de pulsos)
17:   }
```
* **Líneas 12-17**: Inicializa el pin de dirección como salida y vincula el pin de pulsos (`PIN_PUL = 18`) al generador de hardware LEDC con resolución de 10 bits ($0-1023$). En reposo escribe ciclo de trabajo 0, asegurando que el optoacoplador del driver permanezca apagado.

```cpp
19:   void arrancar() {
20:     _enMarcha = true;
21:     if (_objetivo < RPM_MIN) _objetivo = (_rpmGuardada >= RPM_MIN) ? _rpmGuardada : RPM_INICIO;
22:   }
23:   void detener()         { _enMarcha = false; _invirtiendo = false; }
24:   void setRPM(float rpm) {
25:     float r = constrain(rpm, RPM_MIN, RPM_MAX);
26:     if (_invirtiendo) _rpmGuardada = r;   // consigna post-inversión si cambia durante frenado
27:     else              _objetivo = r;
28:   }
```
* **Líneas 19-28**: Comandos de control.  
  🐛 **Solución del Bug 1 (Estado en Inversión)**: Si el usuario mueve el slider de RPM mientras la bomba frena para invertir (`_invirtiendo == true`), la consigna se guarda en `_rpmGuardada` en lugar de pisar `_objetivo`.  
  🐛 **Solución del Bug Hermano y Edge Case (Consigna Huérfana)**: Si se pulsa `STOP` durante una inversión en curso, `_objetivo` queda en 0. Al pulsar `START` posteriormente, `arrancar()` detecta que `_objetivo < RPM_MIN` y restaura automáticamente `_rpmGuardada`, o cae al fallback seguro `RPM_INICIO` ($25\text{ RPM}$) si la propia `_rpmGuardada` también fue afectada. La máquina de estados queda cerrada en el 100% de los caminos posibles.

```cpp
30:   void toggleSentido() {
31:     if (_invirtiendo) return;                        // inversión ya en curso: ignorar
32:     if (_actual < 5.0f) fijarSentido(!_horario);     // parado → giro directo
33:     else {                                           // en marcha → frenar, invertir, acelerar
34:       _invirtiendo = true;
35:       if (_objetivo >= RPM_MIN) _rpmGuardada = _objetivo; // previene contagio de 0 si venía de STOP
36:       _objetivo = 0.0f;
37:     }
38:   }
```
* **Líneas 30-38**: Maniobra de inversión protegida. Si el motor está en movimiento, memoriza la consigna válida (sin contagiarse de cero si venía de una detención), frena la bomba a $0\text{ RPM}$ siguiendo la rampa de deceleración y recién allí invierte el sentido de giro. Esto previene roturas mecánicas por inercia en el cabezal peristáltico.

```cpp
33:   float rpmActual() const           { return _actual; }
34:   bool  enMarcha() const            { return _enMarcha; }
35:   bool  invirtiendo() const         { return _invirtiendo; }
36:   bool  sentidoHorario() const      { return _horario; }
37:   float caudalTeorico_mLmin() const { return _actual * ML_POR_VUELTA; }
```
* **Líneas 33-37**: Consultores de estado. `caudalTeorico_mLmin()` multiplica las RPM instantáneas por el factor geométrico de la manguera ($15.4\text{ mL/rev}$).

```cpp
40:   void tick(float dt) {
41:     float objetivo = _enMarcha ? _objetivo : 0.0f;
42:     if      (_actual < objetivo) _actual = min(objetivo, _actual + ACEL_RPM_S * dt);
43:     else if (_actual > objetivo) _actual = max(objetivo, _actual - ACEL_RPM_S * dt);
```
* **Líneas 40-43**: Calculador cinemático de rampa. Se ejecuta cada $50\text{ ms}$ en el lazo principal. Incrementa o decrementa `_actual` con una tasa de $20\text{ RPM/s}$ multiplicada por el paso de tiempo $\Delta t$.

```cpp
45:     if (_invirtiendo && _actual <= 0.1f) {           // llegó a 0 RPM
46:       fijarSentido(!_horario);
47:       _invirtiendo = false;
48:       _objetivo = _rpmGuardada;
49:     }
```
* **Líneas 45-49**: Transición de inversión: al alcanzar el reposo absoluto ($< 0.1\text{ RPM}$), conmuta físicamente la señal lógica en el pin `DIR` y restaura la consigna guardada para acelerar en el sentido opuesto.

```cpp
51:     if (_actual >= 5.0f) {                           // generar pulsos
52:       uint32_t f = (uint32_t)(_actual * PULSOS_POR_REV / 60.0f);
53:       if (f != _fActual) { ledcChangeFrequency(PIN_PUL, f, 10); _fActual = f; }
54:       ledcWrite(PIN_PUL, 512);                       // 50 % duty
55:     } else if (_fActual != 0) {
56:       ledcWrite(PIN_PUL, 0);                         // reposo: LOW → opto OFF
57:       _fActual = 0;
58:     }
59:   }
```
* **Línea 52**: Aplica la fórmula cinemática $f = \frac{\text{RPM} \times \text{PULSOS\_POR\_REV}}{60}$.
* **Línea 53**: `ledcChangeFrequency` actualiza la frecuencia del hardware PWM en caliente sin generar transitorios ni interrumpir la CPU.
* **Línea 54**: `ledcWrite(PIN_PUL, 512)` fija el ciclo de trabajo en exactamente el 50% ($512/1024$), generando una onda cuadrada simétrica óptima para la conmutación de los optoacopladores del DM860.
* **Líneas 55-58**: Por debajo de $5\text{ RPM}$, corta la emisión de pulsos y asegura nivel bajo.

```cpp
62:   void fijarSentido(bool horario) {
63:     _horario = horario;
64:     // CÁTODO COMÚN: LOW = opto OFF (sentido horario / filtración)
65:     //                HIGH = opto ON (sentido antihorario / retrolavado)
66:     digitalWrite(PIN_DIR, horario ? LOW : HIGH);
67:   }
```
* **Líneas 62-67**: Control físico de dirección bajo esquema de cátodo común: `LOW` = Sentido Horario (Filtración directa); `HIGH` = Sentido Antihorario (Retrolavado).

---

## 4.4. Módulo 4: `subhito2_2_v2.ino`

```cpp
6:  #include <WiFi.h>
7:  #include <WebServer.h>
8:  #include <ESPmDNS.h>
9:  #include "config.h"
10: #include "caudalimetro.h"
11: #include "bomba.h"
12: #include "index_html.h"
```
* **Líneas 6-12**: Inclusión de bibliotecas oficiales del entorno ESP32 y los módulos del proyecto.

```cpp
14: Caudalimetro sensorFeed(PIN_SENSOR_FEED, K_FEED, "FEED");
15: Caudalimetro sensorPerm(PIN_SENSOR_PERM, K_PERM, "PERMEADO");
16: Bomba bomba;
17: WebServer server(80);
19: float qRet_mLmin = 0, recuperacion = 0, deltaBomba = 0, volBomba_L = 0;
20: uint32_t tLoop = 0, tCaudal = 0;
```
* **Líneas 14-20**: Instanciación de los dos sensores de caudal, el objeto de control de la bomba y el servidor HTTP en el puerto 80. Variables globales para el balance de materia y temporización.

```cpp
22: void manejarRaiz()   { server.send_P(200, "text/html", INDEX_HTML); }
23: void manejarSet()    { if (server.hasArg("rpm")) bomba.setRPM(server.arg("rpm").toFloat()); server.send(200, "text/plain", "OK"); }
25: void manejarCmd() {
26:   String a = server.arg("act");
27:   if      (a == "START")      bomba.arrancar();
28:   else if (a == "STOP")       bomba.detener();
29:   else if (a == "DIR")        bomba.toggleSentido();
30:   else if (a == "RESET_VOL")  { sensorFeed.resetVolumen(); sensorPerm.resetVolumen(); volBomba_L = 0; }
31:   server.send(200, "text/plain", "OK");
32: }
```
* **Líneas 22-32**: Enrutadores de comandos HTTP:
  * `/`: Despacha la interfaz gráfica almacenada en Flash (`INDEX_HTML`).
  * `/set?rpm=XX`: Recibe la nueva consigna de velocidad.
  * `/cmd?act=...`: Ejecuta arranque, parada, inversión de marcha o puesta a cero de contadores volumétricos.

```cpp
34: void manejarStatus() {
35:   char ip[24];
36:   if (WiFi.status() == WL_CONNECTED) WiFi.localIP().toString().toCharArray(ip, 24);
37:   else strcpy(ip, "solo AP");
38:   char j[460];
39:   snprintf(j, sizeof(j),
40:     "{\"on\":%d,\"inv\":%d,\"dir\":%d,\"rpm\":%.1f,\"rpm_al\":%.1f,\"pump_ml\":%.1f,"
41:     "\"f_feed\":%.1f,\"q_feed\":%.1f,\"vol_feed\":%.3f,\"feed_ok\":%d,"
42:     "\"f_perm\":%.1f,\"q_perm\":%.1f,\"vol_perm\":%.3f,\"perm_ok\":%d,"
43:     "\"q_ret\":%.1f,\"recov\":%.1f,\"delta\":%.1f,\"ip\":\"%s\"}",
44:     bomba.enMarcha(), bomba.invirtiendo(), bomba.sentidoHorario(),
45:     bomba.rpmActual(), RPM_ALARMA_MEMBRANA, bomba.caudalTeorico_mLmin(),
46:     sensorFeed.frecuencia_Hz(), sensorFeed.caudal_mLmin(), sensorFeed.volumen_L(), !sensorFeed.sinSenal(),
47:     sensorPerm.frecuencia_Hz(), sensorPerm.caudal_mLmin(), sensorPerm.volumen_L(), !sensorPerm.sinSenal(),
48:     qRet_mLmin, recuperacion, deltaBomba, ip);
49:   server.send(200, "application/json", j);
50: }
```
* **Líneas 34-50**: Telemetría JSON de alta velocidad (`/status`). Empaqueta todas las magnitudes físicas de proceso para actualización del dashboard web cada 500 ms. Se destaca la inclusión de `"rpm_al": 36.0` y el dimensionamiento del buffer a 460 bytes.

```cpp
52: void setup() {
53:   Serial.begin(115200);
54:   delay(500);
55:   sensorFeed.begin();
56:   sensorPerm.begin();
57:   bomba.begin();
58:   Serial.println("\n=== PLANTA UF | Hito 2.1 (v2.2 cátodo común) ===");
```
* **Líneas 52-58**: Inicialización de la consola serie y periféricos de hardware.

```cpp
60:   // 1. Conectar a Router primero (15s timeout) para fijar el canal Wi-Fi
61:   WiFi.mode(WIFI_AP_STA);
62:   Serial.printf("Conectando a '%s' ", SSID_STA);
63:   WiFi.begin(SSID_STA, PASS_STA);
64:   uint32_t t0 = millis();
65:   while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
66:     delay(300);
67:     Serial.print(".");
68:   }
69:   Serial.println(WiFi.status() == WL_CONNECTED ? " -> OK" : " -> TIMEOUT (queda AP activo)");
70: 
71:   // 2. Levantar el SoftAP (hereda el canal RF del router si conectó)
72:   WiFi.softAP(SSID_AP, PASS_AP);
73:   MDNS.begin("bomba");
```
* **Líneas 60-73**: Secuencia de inicialización de red en modo dual (`WIFI_AP_STA`). Primero intenta asociarse al router del laboratorio (`Box804`) durante 15 segundos para sintonizar el canal físico de radiofrecuencia (RF). Luego levanta el SoftAP propio `Bomba_Peristaltica_UF` y el servicio mDNS (`http://bomba.local`).

```cpp
75:   server.on("/", HTTP_GET, manejarRaiz);
76:   server.on("/set", HTTP_GET, manejarSet);
77:   server.on("/cmd", HTTP_GET, manejarCmd);
78:   server.on("/status", HTTP_GET, manejarStatus);
79:   server.begin();
86:   Serial.println("Sensores: FEED=G14, PERM=G27 | Bomba: PUL+=G18, DIR+=G19 (PUL-/DIR- a GND)");
87:   tLoop = tCaudal = millis();
88: }
```
* **Líneas 75-88**: Registro de endpoints REST y puesta en marcha del servidor HTTP.

```cpp
90: void loop() {
91:   server.handleClient();
92:   uint32_t t = millis();
93: 
94:   // --- WiFi: si se cayó o nunca conectó, reintenta cada 30 s en segundo plano ---
95:   static uint32_t tWiFi = 0;
96:   if (t - tWiFi >= 30000 && WiFi.status() != WL_CONNECTED) {
97:     tWiFi = t;
98:     Serial.println("[WIFI] Reintentando conexión con router...");
99:     WiFi.begin(SSID_STA, PASS_STA);
100:   }
```
* **Líneas 90-100**: Atiende peticiones HTTP de los clientes y gestiona la reconexión Wi-Fi automática cada 30 segundos sin interrumpir la operación de la planta.

```cpp
102:   // --- Bomba: rampa cada 50 ms ---
103:   float dt = (t - tLoop) / 1000.0f;
104:   if (dt >= 0.05f) {
105:     tLoop = t;
106:     bomba.tick(dt);
107:     volBomba_L += (bomba.caudalTeorico_mLmin() / 60000.0f) * dt;
108:   }
```
* **Líneas 102-108**: Bucle de control cinemático a $20\text{ Hz}$ ($50\text{ ms}$). Calcula la aceleración suave del motor e integra el volumen teórico bombeado.

```cpp
110:   // --- Caudalímetros: cada 1 s ---
111:   if (t - tCaudal >= 1000) {
112:     float dtc = (t - tCaudal) / 1000.0f;
113:     tCaudal = t;
114:     bool empuja = bomba.caudalTeorico_mLmin() > 150.0f;   // > 0.15 L/min
115:     sensorFeed.actualizar(dtc, empuja);
116:     sensorPerm.actualizar(dtc, empuja);
```
* **Líneas 110-116**: Bucle de instrumentación periódica a $1\text{ Hz}$ ($1000\text{ ms}$). Actualiza los contadores de frecuencia, caudales instantáneos y volúmenes acumulados.

```cpp
118:     float qF = sensorFeed.caudal_mLmin();
119:     float qP = sensorPerm.caudal_mLmin();
120:     float qT = bomba.caudalTeorico_mLmin();
121:     qRet_mLmin   = max(0.0f, qF - qP);
122:     recuperacion = (qF > 20.0f) ? 100.0f * qP / qF : 0.0f;
123:     deltaBomba   = (qT > 20.0f) ? 100.0f * (qF - qT) / qT : 0.0f;
```
* **Líneas 118-123**: Cálculos del balance de materia en la membrana de ultrafiltración:
  * Caudal de Retentado: $Q_{\text{ret}} = \max(0, Q_{\text{feed}} - Q_{\text{perm}})$.
  * Factor de Recuperación: $Y = \frac{Q_{\text{perm}}}{Q_{\text{feed}}} \times 100\%$.
  * Discrepancia Bomba vs Sensor: $\Delta\% = \frac{Q_{\text{feed}} - Q_{\text{teórico}}}{Q_{\text{teórico}}} \times 100\%$.

```cpp
125:     if (sensorFeed.sinSenal()) Serial.println("[ALERTA] FEED sin señal: ¿burbuja de aire o cable suelto?");
126:     if (sensorPerm.sinSenal()) Serial.println("[ALERTA] PERM sin señal: ¿burbuja de aire o cable suelto?");
128:     Serial.printf("[%lus] FEED %5.1f Hz %6.1f | PERM %6.1f | RET %6.1f | Y %4.1f%% | Bomba %3.0f RPM teor %6.1f D%+5.1f%%\n",
129:                   t / 1000, sensorFeed.frecuencia_Hz(), qF, qP, qRet_mLmin,
130:                   recuperacion, bomba.rpmActual(), qT, deltaBomba);
131:   }
132: }
```
* **Líneas 125-131**: Supervisión de diagnósticos y emisión periódica de la línea de datos tabulada hacia la consola serie.

---

## 4.5. Módulo 5: `index_html.h`

El archivo `index_html.h` almacena la interfaz de usuario completa (HTML5 + CSS3 + JS) embebida en memoria Flash mediante `PROGMEM`:

1. **Estilos CSS Modernos y Alertas Visuales**:
   * `.al`: Cartel de advertencia rojo crítico si el caudal de alimentación medido supera los $600\text{ mL/min}$ (límite físico de rotura de fibras capilares del FX100).
   * `.al2`: Cartel de advertencia ámbar si la consigna de velocidad es $\ge 36\text{ RPM}$ ($~600\text{ mL/min}$), recordando a los alumnos que solo deben operar en ese régimen durante ensayos con agua y retentado abierto.
2. **Controles Operativos y Presets**:
   * Slider de rango expandido: $20$ a $100\text{ RPM}$ con paso de $1\text{ RPM}$.
   * Botonera rápida de acceso directo: `[20]`, `[25]`, `[36]`, `[50]`, `[80]`.
   * Botones de maniobra: Arranque, Parada e Inversión Segura (Filtración / Retrolavado).
3. **Módulo Datalogger y Exportación CSV**:
   * El script JavaScript mantiene en memoria local un arreglo dinámico `log = []`.
   * Cada 2 segundos durante la marcha, almacena una tupla con: `Hora`, `Sentido`, `RPM`, `Q_Teorico`, `Q_Feed`, `Q_Perm`, `Q_Ret`, `Vol_Feed`, `Vol_Perm`, `Y%`.
   * Al hacer clic en el botón azul **"📥 CSV"**, el navegador compila los datos en formato compatible con Excel (`data:text/csv;charset=utf-8`) y dispara la descarga automática del archivo con la fecha del día.

---

# 5. Auditoría del Código: Detección de "Cosas Raras" y Correcciones de Ingeniería

Durante el análisis exhaustivo línea por línea, se auditaron y corrigieron cuatro puntos críticos:

| Aspecto Auditado | Estado Anterior ("Cosa Rara") | Corrección Implementada | Justificación de Ingeniería |
| :--- | :--- | :--- | :--- |
| **1. Bug de Inversión de Giro (`bomba.h`)** | `setRPM()` pisaba `_objetivo` durante el frenado de inversión | **`if (_invirtiendo) _rpmGuardada = r; else _objetivo = r;`** | Si el usuario cambiaba la consigna mientras desaceleraba para invertir, `_actual` nunca bajaba de 0.1 RPM, bloqueando el estado `_invirtiendo = true` permanentemente. Resuelto guardando la consigna en `_rpmGuardada`. |
| **2. Compromiso Filtro Digital (`config.h`)** | Techo fijo o corte arbitrario | **`12000 µs` en Fase 1 (banco), opción `4500 µs` post-P5** | `12000 µs` otorga máxima inmunidad contra el hum de 108 Hz en ensayos nominales ($\le 36\text{ RPM}$). `4500 µs` habilita medir hasta $100\text{ RPM}$ ($1540\text{ mL/min}$) una vez validado el hardware limpio. |
| **3. Integrador de Volumen (`caudalimetro.h`)** | `_vol += n/(K*60)` sumaba incluso pulsos falsos descartados | **Movido dentro del bloque `else` de `Q_MAX_FISICO_MLMIN`** | Si una ráfaga supera los $6000\text{ mL/min}$ y es descartada como ruido eléctrico, esos pulsos falsos no contaminan el totalizador acumulado de volumen en Litros. |
| **4. Buffer de Telemetría JSON (`subhito2_2_v2.ino`)** | `char j[420]` en `manejarStatus()` | **`char j[460]`** | Al agregar el campo `"rpm_al": 36.0` para la alarma en la web, la cadena JSON superaba los 410 bytes, quedando peligrosamente al borde del desbordamiento de buffer (*buffer overflow*). |
| **5. Configuración del Modo GPIO (`caudalimetro.h`)** | `pinMode(_pin, INPUT_PULLUP)` | **`pinMode(_pin, INPUT)`** | Como el front-end de la Placa 2 ya provee una resistencia externa de $4.7\text{ k}\Omega$ a $3.3\text{V}$, desactivar el pull-up interno garantiza una conmutación a $0.0\text{ V}$ nítida sin corrientes parásitas. |
| **6. Bug Hermano y Edge Case de Inversión (`bomba.h`)** | Al pulsar `START` tras un `STOP` durante inversión, la bomba quedaba en $0\text{ RPM}$ | **Fallback a `RPM_INICIO` en `arrancar()` + guarda en `toggleSentido()`** | Cierra completamente la máquina de estados. Si se canceló una inversión con STOP, el próximo START restaura la consigna real (o RPM_INICIO como salvaguarda) sin quedar "encendida y muerta". |

---

# 6. Protocolo Experimental de Pruebas de Laboratorio para el Lunes

Sigan estrictamente esta secuencia en el laboratorio:

```
[Paso 1: Medición Multímetro] ──► [Paso 2: Prueba de Soplido] ──► [Paso 3: Motor en Seco 20/50/80 RPM] ──► [Paso 4: Bombeo con Probeta] ──► [Paso 5: Sub-tests P8a/P8b]
Continuidad Masas (GND=0Ω)       Giro de turbina con aire        Criterio P5 (Ver Tabla de Hum)             Validación volumétrica            Validación Máquina Estados
Bornes amarillos P14/P27 = 3.3V   Display marca Hz y mL/min       Badges "SIN SEÑAL" a los 5s                y descarga CSV para Excel         Blindaje de Inversión
```

### Paso 1: Verificación Eléctrica en Frío (Multímetro)
1. **Continuidad de Masas**: Con el multímetro en modo "Beep", tocar un borne GND de Placa 1 y un borne GND de Placa 2 $\rightarrow$ Debe pitar con resistencia $0\,\Omega$.
2. **Voltaje en Reposo**: Energizar el ESP32 por USB. Con voltímetro en DC, medir entre GND y los cables amarillos P14 y P27 $\rightarrow$ Debe medir exactamente **$3.3\text{ V}$** (confirma que el pull-up externo a 3.3V está funcionando y no hay sobretensión).

### Paso 2: Prueba de Soplido (Motor Apagado)
1. Abrir el dashboard en el celular o PC (`http://bomba.local`).
2. Soplar suavemente por el caudalímetro FEED y luego por PERMEADO.
3. **Resultado esperado**: La frecuencia en Hz y el caudal en mL/min deben subir fluidamente y volver a cero al detenerse el soplido. Esto valida que la turbina conmuta sólidamente a $0.0\text{ V}$.

### Paso 3: Prueba P5 — Ensayo de Oro Antirruido en Seco (20, 50 y 80 RPM)
Con las mangueras vacías (sin agua), operar la bomba en tres regímenes sucesivos para mapear el comportamiento frente a EMI:

#### Criterio de Diagnóstico para la Prueba P5:

| Resultado en Seco (80 RPM) | Diagnóstico Físico | Causa Raíz | Acción Inmediata |
| :---: | :---: | :---: | :---: |
| **$0.0\text{ Hz}$ clavado en ambos** | **¡ÉXITO TOTAL!** El hum electromagnético quedó por debajo del umbral lógico gracias al filtro RC. | Front-End Placa 2 ($4.7\text{ k}\Omega + 100\text{ nF}$) funcionando a la perfección. | ✅ Cambiar `FILTRO_RUIDO_US` a `4500` en `config.h` y desbloquear todo el rango hasta 100 RPM. |
| **$\sim 270\text{ a }550\text{ mL/min}$ estables** | El hum de $108\text{ Hz}$ sigue vivo. El filtro de $12\text{ ms}$ deja pasar 1 de cada 2 pulsos ($54\text{ Hz}$). | Ruido físico en la bornera: revisar contacto de resistencias $4.7\text{ k}\Omega$ o capacitor $100\text{ nF}$. | El arreglo es de hardware: revisar cableado en Placa 2, polaridad del capacitor electrolítico de filtro y masa común. |
| **Valores erráticos saltando** | Transitorios sueltos de conmutación inductiva. | Picos de acoplamiento capacitivo entre cables de motor y sensores. | Separar físicamente los cables del motor de los cables amarillos de señal. |

> 💡 **Verificación adicional gratuita en P5**: Como la bomba empuja más de $150\text{ mL/min}$ teóricos sin recibir pulsos reales, **a los 5 segundos exactos deben encenderse los carteles rojos `SIN SEÑAL` en ambos sensores**. Que se enciendan confirma que el algoritmo de supervisión y diagnóstico de fallas opera a la perfección.

### Paso 4: Caracterización con Agua y Descarga Libre (Probeta Graduada)
1. Colocar agua en el recipiente de alimentación y la descarga libre a la probeta de $1000\text{ mL}$.
2. Presionar **"Reset L"** en la web.
3. Operar la bomba a **$36\text{ RPM}$** durante exactamente **$60\text{ segundos}$**.
4. Comparar el volumen de la probeta frente al volumen registrado en la web y hacer clic en **"📥 CSV"** para exportar la planilla oficial de calibración para la tesis.

### Paso 5: Prueba P8 — Validación Dinámica de la Máquina de Estados (Sub-tests P8a y P8b)
Para certificar que la lógica de inversión y detención no posee estados huérfanos:
1. **Sub-test P8a (Cambio de consigna durante frenado)**:
   * Arrancar la bomba a $50\text{ RPM}$.
   * Presionar `DIR` $\rightarrow$ la bomba inicia el frenado.
   * Mientras frena, mover el slider a $30\text{ RPM}$.
   * **Resultado esperado**: La bomba desacelera hasta $0\text{ RPM}$, invierte el sentido de giro físico y acelera suavemente hasta alcanzar exactamente las nuevas $30\text{ RPM}$ (sin trabarse ni ignorar la consigna).
2. **Sub-test P8b (Cancelación y rearme de inversión)**:
   * Con la bomba en marcha a $50\text{ RPM}$, presionar `DIR`.
   * Mientras frena, presionar inmediatamente `STOP` $\rightarrow$ la bomba se detiene por completo.
   * Presionar nuevamente `DIR` y luego `START`.
   * **Resultado esperado**: La bomba arranca y acelera limpiamente a su consigna de trabajo (sin quedarse parada en $0\text{ RPM}$ con el cartel "EN MARCHA").
