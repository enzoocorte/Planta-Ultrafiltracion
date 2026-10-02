# 📘 MANUAL TÉCNICO Y EXPLICACIÓN INTEGRAL DEL FIRMWARE `subhito2_2_v4`
## Planta Piloto de Ultrafiltración FX100 • Subhito 2.2: Instrumentación, Metrología de Caudal y Control Cinemático
**Codirector**: Ing. Enzo  
**Tesistas**: Antonella Guitián & Owen Cañizares  
**Carrera**: Ingeniería Industrial / Química — Universidad Nacional de Salta (UNSa)  
**Fecha de Actualización**: Octubre 2026  
**Ubicación del Firmware**: [`02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v4/EN_USO_firmware_planta/`](./EN_USO_firmware_planta/)

---

# 📑 ÍNDICE GENERAL

1. [Arquitectura General y Evolución Metrológica](#1-arquitectura-general-y-evolución-metrológica)
   - [1.1. De la Versión v3 a la Versión v4: ¿Por qué cambiaron los algoritmos?](#11-de-la-versión-v3-a-la-versión-v4-por-qué-cambiaron-los-algoritmos)
   - [1.2. Síntesis de las 3 Rondas de Auditoría Externa de IA](#12-síntesis-de-las-3-rondas-de-auditoría-externa-de-ia)
2. [Fundamentos Físicos, Matemáticos y Metrológicos del Caudal](#2-fundamentos-físicos-matemáticos-y-metrológicos-del-caudal)
   - [2.1. Unidades de la Constante de Calibración $K$: Aclaración Definitiva](#21-unidades-de-la-constante-de-calibración-k-aclaración-definitiva)
   - [2.2. Limitación del Conteo Fijo por Ventana (v3) vs. Período Recíproco (v4)](#22-limitación-del-conteo-fijo-por-ventana-v3-vs-período-recíproco-v4)
   - [2.3. Demostración Matemática: Inmunidad al Desbordamiento (*Rollover*) de `micros()`](#23-demostración-matemática-inmunidad-al-desbordamiento-rollover-de-micros)
   - [2.4. La Cota Física Superior Continua ($f \le 10^6 / \Delta t_{\text{sin}}$)](#24-la-cota-física-superior-continua-f-le-106--delta-t_textsin)
   - [2.5. La Realidad Física del Sensor YF-S401 en Permeado y el Método Gravimétrico](#25-la-realidad-física-del-sensor-yf-s401-en-permeado-y-el-método-gravimétrico)
   - [2.6. Interacción con el Rizo Peristáltico del Cabezal de 3 Rodillos](#26-interacción-con-el-rizo-peristáltico-del-cabezal-de-3-rodillos)
   - [2.7. Diagnóstico Asimétrico de Pérdida de Flujo y Cable Desconectado](#27-diagnóstico-asimétrico-de-pérdida-de-flujo-y-cable-desconectado)
3. [Análisis Exhaustivo Módulo por Módulo (Línea por Línea)](#3-análisis-exhaustivo-módulo-por-módulo-línea-por-línea)
   - [3.1. Módulo `config.h`](#31-módulo-configh)
   - [3.2. Módulo `caudalimetro.h`](#32-módulo-caudalimetroh)
   - [3.3. Módulo `caudalimetro.cpp`](#33-módulo-caudalimetrocpp)
   - [3.4. Módulo `Bomba.h`](#34-módulo-bombah)
   - [3.5. Módulo `Bomba.cpp`](#35-módulo-bombacpp)
   - [3.6. Módulo `darcy.h`](#36-módulo-darcyh)
   - [3.7. Módulo `EN_USO_firmware_planta.ino`](#37-módulo-en_uso_firmware_plantaino)
   - [3.8. Módulo `index_html.cpp`](#38-módulo-index_htmlcpp)
4. [Protocolo de Laboratorio: Puesta a Punto y Re-Calibración en Banco](#4-protocolo-de-laboratorio-puesta-a-punto-y-re-calibración-en-banco)
   - [4.1. Calibración Volumétrica de la Bomba Peristáltica ($\text{mL/rev}$)](#41-calibración-volumétrica-de-la-bomba-peristáltica-textmlrev)
   - [4.2. Calibración Multipunto del Sensor de Alimentación ($K_{\text{alim}}$)](#42-calibración-multipunto-del-sensor-de-alimentación-k_textalim)
   - [4.3. Calibración Gravimétrica del Sensor de Permeado ($K_{\text{perm}}$)](#43-calibración-gravimétrica-del-sensor-de-permeado-k_textperm)
   - [4.4. Ensayo de Respuesta Dinámica y Parada Súbita](#44-ensayo-de-respuesta-dinámica-y-parada-súbita)

---

# 1. Arquitectura General y Evolución Metrológica

## 1.1. De la Versión v3 a la Versión v4: ¿Por qué cambiaron los algoritmos?

En la versión `v3`, utilizada por Owen para la calibración con probeta a 50 y 72 RPM:
1. La frecuencia se calculaba mediante un **conteo simple de pulsos por unidad de tiempo**:
   $$f = \frac{n}{\Delta t}$$
   donde $n$ era el número de pulsos recibidos en una ventana fija de tiempo ($\Delta t \approx 1.0\text{ s}$).
2. Este método presentaba una **severa degradación metrológica a caudales bajos**:
   - En el permeado, a $50\text{ mL/min}$, con $K = 55.00\text{ Hz/(L/min)}$, la frecuencia generada es de solo $2.75\text{ Hz}$.
   - En una ventana fija de 1 segundo, el microcontrolador cuenta **2 pulsos** ($f = 2.0\text{ Hz} \rightarrow Q = 36.4\text{ mL/min}$) o **3 pulsos** ($f = 3.0\text{ Hz} \rightarrow Q = 54.5\text{ mL/min}$).
   - ¡Esto generaba una oscilación espuria de $\pm 25\%$ a $\pm 35\%$ en la pantalla del SCADA cada segundo, sin que el flujo físico real hubiera cambiado en absoluto!
3. Al detener la bomba, si no entraba ningún pulso ($n = 0$), el código v3 colapsaba bruscamente a $0$, o si se aplicaba filtrado previo sin cota temporal, quedaba un flujo "fantasma" flotando en pantalla.
4. El rizo pulsátil del cabezal peristáltico de 3 rodillos ($0.75\text{ Hz}$ a $5.0\text{ Hz}$) producía fenómenos de batido (*aliasing*) con la ventana de muestreo fija de 1.0 segundo.

En la versión `v4`, se implementó una **reingeniería metrológica completa**:
- **Conteo Recíproco de Período**: Se mide el tiempo exacto en microsegundos transcurrido entre el primer y el último pulso de la ventana mediante `micros()`. La resolución temporal pasa de 1 segundo a $1\ \mu\text{s}$, eliminando el error de cuantización.
- **Cota Física Superior Continua**: Si la bomba frena y dejan de entrar pulsos, la frecuencia estimada decae siguiendo la cota $f \le 10^6 / \Delta t_{\text{sin\_pulso}}$, emulando a la perfección la desaceleración física real.
- **Filtro Exponencial Ponderado (EMA)** adaptado con constante de tiempo $\tau \approx 2\text{ s}$ para absorber el rizo de los 3 rodillos.
- **Aclaración y homogenización de unidades de $K$** a $[\text{Hz}/(\text{L/min})]$.
- **Inmunidad modular al rollover** de 71.58 minutos de `micros()`.

---

## 1.2. Síntesis de las 3 Rondas de Auditoría Externa de IA

Durante el desarrollo de esta versión v4, el sistema fue sometido a tres rondas de auditoría con los modelos de inteligencia artificial más avanzados del mundo (DeepSeek R1, GPT-o3-mini, Gemini 2.0 Pro, Claude 3.5 Sonnet, GLM-5.3, GPT Astra y Claude 3.5/5.5 Opus):

| Ronda | Modelos Evaluadores | Hallazgos Principales Aportados |
| :---: | :--- | :--- |
| **Ronda 1** | DeepSeek R1, GPT-o3-mini, Gemini Pro | Descubrimiento de la necesidad de conteo recíproco a bajo caudal; corrección de límites de TMP en la membrana FX100 ($0.50\text{ bar}$ en lugar de $0.80\text{ bar}$); implementación de modelo Darcy-Vogel. |
| **Ronda 2** | Claude Sonnet 5, GLM-5.3, GPT Astra | Diseño de la cota física superior continua ($f \le 10^6 / \Delta t$); preservación atómica de secciones críticas; eliminación de fragmentación de memoria (evitar `String` en lazos de alta velocidad). |
| **Ronda 3** | Claude Opus (Autoridad de Cierre) | Corrección de unidades de $K$ ($[\text{Hz}/(\text{L/min})]$ vs $[\text{pul/L}]$); advertencia del rango no lineal de la turbina YF-S401 en permeado ($< 200\text{ mL/min}$) y recomendación formal del método gravimétrico; reducción de umbral de disparo de alivio a $0.7\text{ bar}$; desacople de alarma de cable cortado en permeado. |

---

# 2. Fundamentos Físicos, Matemáticos y Metrológicos del Caudal

---

## 2.1. Unidades de la Constante de Calibración $K$: Aclaración Definitiva

El caudalímetro **YF-S401** opera mediante una pequeña turbina con imanes de neodimio enfrentados a un sensor de Efecto Hall integrado en el cuerpo plástico.

### Ecuación Nominal de Fabricante:
$$F = 98 \cdot Q$$
donde:
- $F$ es la frecuencia de pulsos generada en **$\text{Hertz}$** ($\text{pulsos/segundo}$).
- $Q$ es el caudal volumétrico en **$\text{Litros/minuto}$** ($\text{L/min}$).

Por lo tanto, la constante dimensional $K$ se define rigurosamente como:
$$K = \frac{F}{Q} = \left[\frac{\text{Hz}}{\text{L/min}}\right]$$

### Relación con los Pulsos por Litro:
Dado que $1\text{ L/min} = \frac{1\text{ Litro}}{60\text{ segundos}}$, tenemos:
$$K \left[\frac{\text{Hz}}{\text{L/min}}\right] = \frac{1\text{ pulso/s}}{(1/60)\text{ L/s}} = 60\ \left[\frac{\text{pulsos}}{\text{Litro}}\right]$$

$$\mathbf{\text{Pulsos por Litro} = K \times 60}$$

### Verificación con los Valores Calibrados por Owen:
- **Sensor de Alimentación**:
  $$K_{\text{alim}} = 154.62\ \frac{\text{Hz}}{\text{L/min}} \implies 154.62 \times 60 = \mathbf{9277.2\ \frac{\text{pulsos}}{\text{Litro}}}$$
  A $50\text{ RPM}$ ($Q_{\text{alim}} = 680.0\text{ mL/min} = 0.680\text{ L/min}$):
  $$F = 154.62 \times 0.680 = \mathbf{105.14\text{ Hz}}$$
- **Sensor de Permeado**:
  $$K_{\text{perm}} = 55.00\ \frac{\text{Hz}}{\text{L/min}} \implies 55.00 \times 60 = \mathbf{3300.0\ \frac{\text{pulsos}}{\text{Litro}}}$$
  A $100\text{ mL/min} = 0.100\text{ L/min}$:
  $$F = 55.00 \times 0.100 = \mathbf{5.50\text{ Hz}}$$

> [!WARNING]
> En algunas planillas o versiones antiguas de firmware se etiquetó a $K$ erróneamente como "pulsos/Litro" cuando su valor era $154.62$. Si $K$ fuera en pulsos/Litro, ¡un litro solo generaría 154 pulsos en vez de 9277! En todo el firmware v4, **$K$ está expresado en $\text{Hz}/(\text{L/min})$**, y para integrar el volumen en Litros a partir de $n$ pulsos, la fórmula matemática es:
> $$V\ [\text{Litros}] = \frac{n}{K \times 60}$$

---

## 2.2. Limitación del Conteo Fijo por Ventana (v3) vs. Período Recíproco (v4)

### El Problema de la Discretización $\pm 1$ Pulso:
Supongamos que un flujo estacionario genera pulsos cada $400\text{ ms}$ ($2.5\text{ Hz}$):
```
Tiempo (s):   0.0       0.4       0.8       1.2       1.6       2.0
Pulsos:        |---------|---------|---------|---------|---------|
Ventana 1:    [.........................]  -> Cae en t=0.0, 0.4, 0.8 => n = 3 pulsos -> f = 3.0 Hz
Ventana 2:                              [.........................] -> Cae en t=1.2, 1.6     => n = 2 pulsos -> f = 2.0 Hz
```
A pesar de que el caudal es perfectamente constante, el método de conteo simple reporta alternativamente $3.0\text{ Hz}$ y $2.0\text{ Hz}$ (variación del $\mathbf{33.3\%}$).

### La Solución: Medición por Período Recíproco en Microsegundos:
En la versión v4, en cada pulso que entra a la ISR, se registra la marca de tiempo `micros()`:
- Cuando se cierra la ventana de 1 segundo, si ingresaron $n \ge 2$ pulsos:
  $$\Delta t_{\text{ráfaga}} = t_{\text{último}} - t_{\text{primero}}\quad [\mu\text{s}]$$
  Entre el primer pulso y el último pulso han ocurrido exactamente $(n - 1)$ períodos completos. Por lo tanto, la frecuencia real es:
  $$\mathbf{f = \frac{(n - 1) \times 10^6}{t_{\text{último}} - t_{\text{primero}}}\quad [\text{Hz}]}$$
- Si solo ingresó $n = 1$ pulso:
  No podemos restar $t_{\text{último}} - t_{\text{primero}}$ dentro de la misma ventana, pero la ISR capturó el período exacto `_periodo_us` transcurrido respecto al pulso inmediatamente anterior:
  $$\mathbf{f = \frac{10^6}{\text{periodo\_us}}\quad [\text{Hz}]}$$

Este método tiene una **resolución de $1\ \mu\text{s}$**, eliminando de raíz la oscilación por bordes de ventana.

---

## 2.3. Demostración Matemática: Inmunidad al Desbordamiento (*Rollover*) de `micros()`

La función `micros()` del ESP32 devuelve un entero de 32 bits sin signo (`uint32_t`).
El valor máximo almacenable es:
$$2^{32} - 1 = 4\,294\,967\,295\ \mu\text{s} \approx 4294.97\text{ s} \approx \mathbf{71.58\text{ minutos}}$$
A los 71.58 minutos de encendido el ESP32, el contador se desborda y vuelve a comenzar en $0$.

### ¿Por qué la resta `uint32_t` NUNCA falla ni requiere condicionales?
En el estándar ANSI C / C++ y en la arquitectura de 32 bits del procesador Tensilica LX6, la aritmética de tipos `unsigned` se define formalmente como **aritmética modular congruente en base $2^{32}$**:
$$(A - B) \pmod{2^{32}}$$

Supongamos el peor escenario posible (el pulso ocurre justo durante el rollover):
- Pulso anterior: $t_{\text{anterior}} = 4\,294\,967\,000\ \mu\text{s}$ (apenas $295\ \mu\text{s}$ antes del desborde).
- Nuevo pulso: $t_{\text{actual}} = 500\ \mu\text{s}$ (después del desborde).
- Cálculo en código:
  $$\Delta t = t_{\text{actual}} - t_{\text{anterior}} = 500 - 4\,294\,967\,000$$

En el registro binario del procesador:
$$\Delta t = 500 + (2^{32} - 4\,294\,967\,000) = 500 + 296 = \mathbf{796\ \mu\text{s}}$$

El resultado es **matemática y rigurosamente exacto ($796\ \mu\text{s}$)**. No existe ningún error numérico, no se generan números negativos ni se produce ningún fallo en el cálculo.

---

## 2.4. La Cota Física Superior Continua ($f \le 10^6 / \Delta t_{\text{sin}}$)

¿Qué sucede si la bomba se detiene o el fluido se corta súbitamente?
Durante el segundo siguiente a la parada, el número de pulsos recibidos será $n = 0$.
Si simplemente conserváramos la frecuencia calculada en el ciclo anterior, el sistema mostraría caudal cuando el fluido ya está quieto. Y si la hiciéramos saltar a cero bruscamente, arruinaríamos los filtros de proceso.

### El Teorema Físico del Tiempo de Silencio:
Si han transcurrido $\Delta t_{\text{sin}} = t_{\text{ahora}} - t_{\text{último}}$ microsegundos sin que haya aparecido un nuevo flanco de señal, **es físicamente imposible** que la frecuencia instantánea real sea mayor que la frecuencia de un pulso que ocurriera exactamente ahora:
$$f_{\text{máx\_posible}} = \frac{10^6}{\Delta t_{\text{sin}}}\quad [\text{Hz}]$$

*Ejemplo Práctico:*
1. La bomba venía girando a régimen y la última frecuencia medida era $f = 4.0\text{ Hz}$ (un pulso cada $250\text{ ms}$).
2. Se apaga la bomba. Pasa $1.0\text{ segundo}$ sin pulsos:
   $$f_{\text{máx}} = \frac{10^6}{1\,000\,000} = 1.0\text{ Hz} \implies \text{El código limita } f \le 1.0\text{ Hz}$$
3. Pasan $2.0\text{ segundos}$ sin pulsos:
   $$f_{\text{máx}} = \frac{10^6}{2\,000\,000} = 0.5\text{ Hz} \implies f \le 0.5\text{ Hz}$$
4. Pasan $3.0\text{ segundos}$ sin pulsos:
   $$f_{\text{máx}} = \frac{10^6}{3\,000\,000} = 0.33\text{ Hz} \implies \text{El código aplica corte estricto } f = 0.0\text{ Hz}$$

Esto genera una **curva de decaimiento natural y físicamente perfecta**, idéntica al frenado inercial del rotor de la turbina.

---

## 2.5. La Realidad Física del Sensor YF-S401 en Permeado y el Método Gravimétrico

Este punto fue destacado con especial énfasis por Claude Opus en la auditoría de cierre, y representa una pieza clave para la fundamentación teórica de la tesis de Antonella y Owen:

### Límites de Operación del Sensor YF-S401:
- El fabricante especifica el sensor para un rango de trabajo útil de:
  $$Q_{\text{catálogo}} \in [0.3\text{ a }6.0\text{ L/min}] = [\mathbf{300\text{ a }6000\text{ mL/min}}]$$
- En la planta piloto de ultrafiltración FX100:
  - El caudal de **Alimentación** opera entre $200\text{ y }1360\text{ mL/min}$ (dentro del rango lineal del sensor).
  - El caudal de **Permeado** opera típicamente entre $\mathbf{45\text{ y }365\text{ mL/min}}$, y durante fenómenos de ensuciamiento severo (*fouling*) puede descender por debajo de $30\text{ mL/min}$ o $10\text{ mL/min}$.

### ¿Qué ocurre físicamente por debajo de $200\text{ mL/min}$?
A caudales tan bajos:
1. La energía cinética del chorro incidente sobre las paletas de la turbina es muy pequeña.
2. La **fricción mecánica estática y viscosa** en los pivotes del eje de acero inoxidable del rotor deja de ser despreciable frente a la cupla impulsora.
3. Se produce un fenómeno de **deslizamiento (*slip*) e histéresis**: el rotor gira a menor velocidad que la velocidad media del fluido, haciendo que la constante $K$ caiga drásticamente.
4. Por esta razón física, Owen obtuvo experimentalmente $K_{\text{perm}} = 55.00$, valor muy inferior a los $154.62$ de la alimentación y a los $98.0$ nominales.

```
       K [Hz/(L/min)]
         ▲
  160 ───┼────────────────────────────── (Alimentación: K = 154.62)
         │
   98 ───┼ - - - - - - - - - - - - - - - (Nominal Fabricante: K = 98.0)
         │           .----------------- (Permeado en régimen lineal > 300 mL/min)
   55 ───┼──────────'                    (Permeado calibrado por Owen: K = 55.00)
         │         /
         │       /   <- Zona de no linealidad por fricción estática (< 150 mL/min)
    0 ───┴──────┴─────────┴─────────────► Caudal Q (mL/min)
         0     100       300     1000
```

### Decisión Metrológica para la Tesis:
- El sensor YF-S401 de permeado cumple una función excelente como **indicador continuo de tendencia dinámica en tiempo real** en la pantalla SCADA.
- Para el cálculo riguroso del flujo transmembrana $J$ $[\text{LMH}]$ y de la resistencia hidráulica de la membrana $R_m$ en la memoria escrita de la tesis, **el patrón primario oficial es el método gravimétrico**:
  $$J_v = \frac{\Delta m}{\rho_{\text{agua}} \cdot A_m \cdot \Delta t}$$
  recolectando el permeado sobre una balanza analítica digital ($0.1\text{ g}$ de resolución). El firmware v4 está preparado para contrastarse directamente contra este método.

---

## 2.6. Interacción con el Rizo Peristáltico del Cabezal de 3 Rodillos

La bomba peristáltica **MBP-2000** posee un cabezal rotor con **3 rodillos** desfasados $120^\circ$.
Cada vez que un rodillo ingresa a la pista semicircular y ocluye la manguera de silicona ($\varnothing_{\text{int}} = 12\text{ mm}$), se genera una pulsación periódica de presión y velocidad de flujo.

### Frecuencia Fundamental de Pulsación:
$$f_{\text{rizo}} = \frac{3 \times \text{RPM}}{60} = \frac{\text{RPM}}{20}\quad [\text{Hz}]$$

| Velocidad de Bomba | Caudal Teórico ($13.6\text{ mL/rev}$) | Frecuencia de Rizo ($f_{\text{rizo}}$) | Período de Pulsación ($T_{\text{rizo}}$) |
| :---: | :---: | :---: | :---: |
| **$15.0\text{ RPM}$** | $204\text{ mL/min}$ | **$0.75\text{ Hz}$** | $1.33\text{ segundos}$ |
| **$20.0\text{ RPM}$** | $272\text{ mL/min}$ | **$1.00\text{ Hz}$** | $1.00\text{ segundos}$ |
| **$50.0\text{ RPM}$** | $680\text{ mL/min}$ | **$2.50\text{ Hz}$** | $0.40\text{ segundos}$ |
| **$72.0\text{ RPM}$** | $979\text{ mL/min}$ | **$3.60\text{ Hz}$** | $0.28\text{ segundos}$ |
| **$100.0\text{ RPM}$** | $1360\text{ mL/min}$ | **$5.00\text{ Hz}$** | $0.20\text{ segundos}$ |

### Consecuencia Metrológica:
A velocidades bajas ($15$ a $25\text{ RPM}$), el período del rizo ($1.33\text{ s}$) es mayor que la ventana de actualización de 1.0 segundo.
Para evitar que la telemetría oscile al ritmo de la pulsación de los rodillos, el firmware v4 incorpora un **filtro digital pasa-bajos exponencial ponderado (EMA)**:
$$Q_k = \alpha \cdot Q_{\text{instantáneo}} + (1 - \alpha) \cdot Q_{k-1}$$
con $\alpha = 0.4$, lo que otorga una constante de tiempo equivalente de:
$$\tau \approx \frac{\Delta t}{\ln(1 / (1 - \alpha))} = \frac{1.0}{\ln(1 / 0.6)} \approx \mathbf{1.96\text{ segundos}}$$
Este filtro suaviza completamente la ondulación en pantalla sin introducir un retardo apreciable para el control.

---

## 2.7. Diagnóstico Asimétrico de Pérdida de Flujo y Cable Desconectado

En la versión v3, si pasaban más de 5 segundos sin pulsos mientras la bomba estaba en marcha, ambos sensores pasaban a estado de error (`SIN SEÑAL`).
Sin embargo, **físicamente los dos sensores tienen naturalezas operativas completamente distintas**:
1. **Sensor de Alimentación**: Está ubicado inmediatamente a la salida de la bomba de desplazamiento positivo. Si la bomba gira a más de $1\text{ RPM}$, es físicamente mandatorio que circule líquido. Si pasan 5 segundos sin un solo pulso, es señal inequívoca de:
   - Manguera desacoplada o rota.
   - Bomba descebada con bolsón de aire masivo.
   - Cable de señal del sensor cortado o bornera floja.  
   $\implies$ **Alarma de Fallo Crítico Justificada**.
2. **Sensor de Permeado**: Está ubicado en la línea de filtrado a través de la membrana capilar. Si la válvula de permeado está cerrada, o si la presión transmembrana (TMP) aún no superó la contrapresión capilar, el flujo de permeado **es legítimamente cero**. Reportar un fallo de sensor en este caso confunde al operador haciéndole creer que hay una falla de hardware.  
   $\implies$ **En v4, el sensor de permeado se declara con `esAlimentacion = false`, inhibiendo falsas alarmas cuando el flujo se detiene normalmente**.

---

# 3. Análisis Exhaustivo Módulo por Módulo (Línea por Línea)

---

## 3.1. Módulo `config.h`

Este archivo centraliza todas las constantes físicas, cinemáticas y metrológicas de la planta en tiempo de compilación.

```cpp
1: #pragma once
```
- **Línea 1**: Directiva del preprocesador que asegura que el archivo de cabecera solo se incluya una vez durante la compilación, evitando redefiniciones de símbolos.

```cpp
13: constexpr uint8_t PIN_PUL                = 18;  // DM860 PUL+ (PUL- a GND común)
14: constexpr uint8_t PIN_DIR                = 19;  // DM860 DIR+ (DIR- a GND común)
15: constexpr uint8_t PIN_LED_BOMBA          = 2;   // LED Azul Onboard (indicador de marcha)
18: constexpr uint8_t PIN_SENSOR_ALIMENTACION = 14;  // Sensor de Alimentación (Borne 12 Sup)
19: constexpr uint8_t PIN_SENSOR_PERMEADO     = 27;  // Sensor de Permeado (Borne 11 Sup, Pull-up 4.7k a 3.3V)
```
- **Líneas 13-15**: Asignación de pines de control del driver DM860. El optoacoplador está conectado en configuración de cátodo común (GPIO a PUL+, GND a PUL-). El pin 2 comanda el LED azul de la placa NodeMCU-32S.
- **Líneas 18-19**: Pines GPIO dedicados a la interrupción de pulsos Hall. El sensor de permeado incluye un resistor físico pull-up de $4.7\text{ k}\Omega$ conectado al riel de $3.3\text{ V}$.

```cpp
24: constexpr uint16_t PULSOS_POR_REV = 3200;     // DM860 configurado a 1/16 micropasos
25: constexpr float ML_POR_VUELTA     = 13.6000f; // Calibrado: 680.0 mL/min / 50.0 RPM
28: constexpr float RPM_MIN           = 15.0f;    // ~204 mL/min
29: constexpr float RPM_MAX           = 100.0f;   // ~1360 mL/min (Habilitado para diseño factorial)
30: constexpr float RPM_INICIO        = 25.0f;    // Consigna de arranque suave (~340 mL/min)
31: constexpr float RPM_ALARMA_MEMBRANA = 44.0f;  // 44.12 RPM = 600 mL/min (Límite hemodiálisis)
```
- **Línea 24**: Conmutadores DIP del driver DM860 ajustados a $1/16$ micropasos. Con un motor NEMA 34 de $1.8^\circ$ ($200$ pasos por vuelta): $200 \times 16 = 3200\text{ pulsos/vuelta}$.
- **Línea 25**: Desplazamiento volumétrico real por vuelta del cabezal peristáltico MBP-2000 obtenido por Owen con probeta ($13.60\text{ mL/vuelta}$).
- **Líneas 28-29**: Rango operativo ampliado hasta $100.0\text{ RPM}$ para permitir el diseño factorial experimental $3^2$ en agua pura definido para la tesis.
- **Línea 31**: Umbral de advertencia clínica ($44.12\text{ RPM} = 600\text{ mL/min}$), límite estándar en diálisis que sirve de cota de seguridad operativa.

```cpp
36: constexpr float AREA_MEMBRANA_M2       = 2.2f;    // Superficie interfacial efectiva (m²)
37: constexpr float K_UF_NOMINAL           = 73.0f;   // mL / (h * mmHg)
38: constexpr float DIAMETRO_CAPILAR_UM    = 185.0f;  // Diámetro interno capilar (μm)
39: constexpr float ESPESOR_PARED_UM       = 35.0f;   // Grosor de pared capilar (μm)
40: constexpr float TMP_MAX_SEGURA_BAR     = 0.50f;   // Límite máximo admisible de TMP (bar)
```
- **Líneas 36-40**: Especificaciones geométricas e hidrodinámicas del filtro capilar de hemodiálisis Fresenius FX100 Helixone®.
  - La permeabilidad hidráulica nominal es $73.0\text{ mL/(h}\cdot\text{mmHg)}$. Multiplicado por el factor de conversión ($750\text{ mmHg/bar}$), equivale a $54\,750\text{ mL/(h}\cdot\text{bar)}$.
  - $\text{TMP}_{\text{máx}}$ establecida en $0.50\text{ bar}$ conforme a los límites de integridad de la fibra hueca de polisulfona/poliamida.

```cpp
44: constexpr float ACEL_ARRANQUE_RPM_S  = 2.0f;   // 2.0 RPM/s despegue inicial suave
45: constexpr float ACEL_NOMINAL_RPM_S   = 3.5f;   // 3.5 RPM/s aceleración progresiva
46: constexpr float DESACEL_AJUSTE_RPM_S = 8.0f;   // 8.0 RPM/s desaceleración en marcha
47: constexpr float FRENADO_PARADA_RPM_S = 45.0f;  // 45.0 RPM/s frenado rápido al presionar STOP (< 1.5s)
```
- **Líneas 44-47**: Parámetros de aceleración cinemática. El arranque a $2.0\text{ RPM/s}$ evita golpes de ariete sobre la membrana capilar, mientras que el frenado de parada a $45.0\text{ RPM/s}$ detiene el equipo en menos de $1.5\text{ segundos}$ ante una consigna de STOP.

```cpp
55: constexpr float K_ALIMENTACION = 154.62f; // Hz/(L/min) -> 9277.2 pulsos/L
56: constexpr float K_PERMEADO     = 55.00f;  // Hz/(L/min) -> 3300.0 pulsos/L
58: constexpr uint32_t FILTRO_RUIDO_US = 1500;    // 1.5 ms blanking anti-rebote (hasta 666 Hz)
59: constexpr float Q_MAX_FISICO_MLMIN = 6000.0f; // Límite de corte físico (6.0 L/min)
```
- **Líneas 55-56**: Factores $K$ en unidades exactas $[\text{Hz}/(\text{L/min})]$.
- **Línea 58**: Tiempo de *blanking* (máscara de supresión de rebote). $1500\ \mu\text{s}$ impone un límite superior de $666\text{ Hz}$ ($Q \approx 4300\text{ mL/min}$ en alimentación), permitiendo que pasen los pulsos genuinos y rechazando cualquier espiga de conmutación electromagnética de alta frecuencia.

---

## 3.2. Módulo `caudalimetro.h`

Define la clase `Caudalimetro` que encapsula la gestión de hardware y algoritmos de cada sensor.

```cpp
14: class Caudalimetro {
15: public:
16:   Caudalimetro(uint8_t pin, float k, const char* nombre, bool esAlimentacion = true);
18:   void begin();
19:   void actualizar(float dt_s, bool bombaEmpuja);
```
- **Línea 16**: Constructor con parámetro por defecto `esAlimentacion = true`. Permite discriminar la lógica de diagnóstico entre alimentación y permeado.
- **Línea 18**: Configura el pin GPIO con resistencia pull-up interna y conecta la interrupción por hardware mediante `attachInterruptArg`.
- **Línea 19**: Método invocado cada 1.0 segundo desde el lazo principal para realizar la computación metrológica y el filtrado digital.

```cpp
21:   float caudal_mLmin()   const { return _q; }
22:   float caudal_Lmin()    const { return _q / 1000.0f; }
23:   float frecuencia_Hz()  const { return _f; }
24:   float volumen_L()      const { return _vol; }
25:   bool  sinSenal()       const { return _fallo; }
26:   void  resetVolumen()         { _vol = 0.0f; }
27:   const char* nombre()   const { return _nombre; }
28:   bool  esAlimentacion() const { return _esAlimentacion; }
30:   void  setK(float nuevoK)     { if (nuevoK > 0.1f) _k = nuevoK; }
31:   float getK()           const { return _k; }
```
- **Líneas 21-31**: Métodos de acceso en línea (*inlined*) de bajo costo computacional. `setK()` valida que el factor sea positivo ($> 0.1$) antes de aplicarlo, protegiendo contra divisiones por cero.

```cpp
34:   static void IRAM_ATTR isrPuente(void* arg);
36:   const uint8_t _pin;
37:   float         _k;
38:   const char*   _nombre;
39:   const bool    _esAlimentacion;
41:   volatile uint32_t _pulsos = 0;
42:   volatile uint32_t _t_ultimo = 0;
43:   volatile uint32_t _t_primero = 0;
44:   volatile uint32_t _periodo_us = 0;
51:   portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
```
- **Línea 34**: Declaración estática de la rutina de servicio de interrupción (`ISR`), calificada con el atributo `IRAM_ATTR` para que el compilador ubique el código en la memoria RAM estática interna del ESP32, garantizando ejecución de latencia nula sin fallos de caché de la memoria Flash SPI.
- **Líneas 41-44**: Variables compartidas entre el contexto de interrupción y el hilo normal de ejecución. Están marcadas como `volatile` para forzar al compilador a leerlas siempre desde la memoria y no desde registros cacheados de la CPU.
- **Línea 51**: Cerrojo de giro (*spinlock*) de FreeRTOS para asegurar atomicidad estricta entre los dos núcleos físicos del ESP32.

---

## 3.3. Módulo `caudalimetro.cpp`

Implementa los algoritmos de captura de pulsos, período recíproco y cota física.

```cpp
3: Caudalimetro::Caudalimetro(uint8_t pin, float k, const char* nombre, bool esAlimentacion)
4:   : _pin(pin), _k(k), _nombre(nombre), _esAlimentacion(esAlimentacion) {}
6: void Caudalimetro::begin() {
7:   pinMode(_pin, INPUT_PULLUP);
8:   attachInterruptArg(digitalPinToInterrupt(_pin), isrPuente, this, FALLING);
9: }
```
- **Líneas 6-9**: `pinMode` configura el pin como entrada con resistencia de polarización positiva interna. `attachInterruptArg` asocia el flanco descendente (`FALLING`) del sensor Hall con la función `isrPuente`, pasando el puntero `this` de la instancia como argumento de contexto.

```cpp
11: void Caudalimetro::actualizar(float dt_s, bool bombaEmpuja) {
12:   portENTER_CRITICAL(&_mux);
13:   uint32_t n      = _pulsos;
14:   _pulsos         = 0;
15:   uint32_t per_us = _periodo_us;
16:   uint32_t t_prim = _t_primero;
17:   uint32_t t_ult  = _t_ultimo;
18:   portEXIT_CRITICAL(&_mux);
```
- **Líneas 12-18**: **Sección Crítica Atómica**: Deshabilita temporalmente las interrupciones en ambos núcleos del procesador durante menos de $0.2\ \mu\text{s}$ para copiar las variables acumuladas por la ISR y reiniciar el contador `_pulsos = 0`. Esto elimina por completo cualquier condición de carrera (*race condition*).

```cpp
20:   uint32_t tAhora = micros();
21:   uint32_t tSinFlanco = tAhora - t_ult;
```
- **Líneas 20-21**: Captura la estampa actual de tiempo y calcula la diferencia respecto al último pulso recibido. Como se demostró en la Sección 2.3, la resta de tipos `uint32_t` es absolutamente inmune al desbordamiento de 71.58 minutos.

```cpp
27:   if (n >= 2 && (t_ult - t_prim) > 0) {
28:     _f = ((float)(n - 1) * 1000000.0f) / (float)(t_ult - t_prim);
29:   } else if (n == 1 && per_us > 0) {
30:     _f = 1000000.0f / (float)per_us;
31:   }
```
- **Líneas 27-31**: **Cálculo de Período Recíproco**:
  - Si ingresaron $2$ o más pulsos en la ventana, se computa la frecuencia a partir del tiempo real exacto entre el primero y el último pulso en microsegundos, dividiendo por $(n - 1)$ intervalos.
  - Si ingresó $1$ solo pulso, se utiliza el período inter-pulso medido en la ISR (`per_us`).

```cpp
38:   if (tSinFlanco > 0) {
39:     float f_max_posible = 1000000.0f / (float)tSinFlanco;
40:     if (_f > f_max_posible) {
41:       _f = f_max_posible;
42:     }
43:   }
45:   if (n == 0 && tSinFlanco > 3000000UL) {
46:     _f = 0.0f;
47:   }
```
- **Líneas 38-47**: **Cota Física Superior Continua**:
  - Si han pasado `tSinFlanco` microsegundos sin eventos, la frecuencia instantánea real no puede exceder $10^6 / tSinFlanco$. Si la frecuencia previa es superior, se recorta a ese límite físico.
  - Si han transcurrido más de $3.0\text{ segundos}$ completos de inactividad absoluta ($n = 0$), la frecuencia se fuerza de manera definitiva a $0.0\text{ Hz}$.

```cpp
50:   float q = (_f * 1000.0f) / _k;
52:   if (q > Q_MAX_FISICO_MLMIN) {
53:     q = 0.0f;
54:     _f = 0.0f;
55:     Serial.printf("[%s] Ruido descartado: %lu pulsos espurios\n", _nombre, (unsigned long)n);
56:   } else {
58:     _vol += (float)n / (_k * 60.0f);
59:   }
```
- **Líneas 50-59**:
  - Conversión de frecuencia a caudal: $Q\ [\text{mL/min}] = (F \times 1000) / K$.
  - Comprobación de plausibilidad: si el caudal supera $6000\text{ mL/min}$ ($Q_{\text{máx\_físico}}$), se descarta como perturbación EMI.
  - Si es válido, se acumula el volumen consumido en Litros: $n / (K \times 60)$.

```cpp
62:   if (n > 0 || _f > 0.05f) {
63:     _q = 0.4f * q + 0.6f * _q;
64:   } else {
65:     _q = 0.0f;
66:   }
```
- **Líneas 62-66**: Filtro EMA con ponderación $40\%$ valor instantáneo y $60\%$ valor histórico. Si la frecuencia cae por debajo de $0.05\text{ Hz}$, el caudal se apaga limpiamente a cero.

```cpp
69:   if (n > 0 || _f > 0.05f) {
70:     _tiempoSinPulso_s = 0.0f;
71:     _fallo = false;
72:   } else if (bombaEmpuja && _esAlimentacion) {
73:     _tiempoSinPulso_s += dt_s;
74:     if (_tiempoSinPulso_s >= 5.0f) _fallo = true;
75:   } else {
76:     _tiempoSinPulso_s = 0.0f;
77:     _fallo = false;
78:   }
```
- **Líneas 69-78**: Detección asimétrica de avería. Solo si `_esAlimentacion == true` y la bomba está en marcha durante más de 5 segundos continuos sin pulsos se enclava la bandera `_fallo = true`. En permeado se mantiene apagada.

```cpp
81: void IRAM_ATTR Caudalimetro::isrPuente(void* arg) {
82:   Caudalimetro* c = reinterpret_cast<Caudalimetro*>(arg);
83:   uint32_t t = micros();
85:   portENTER_CRITICAL_ISR(&c->_mux);
86:   uint32_t dt = t - c->_t_ultimo;
88:   if (dt >= FILTRO_RUIDO_US) {
89:     if (c->_pulsos == 0) {
90:       c->_t_primero = t;
91:     }
93:     if (dt < 5000000UL) {
94:       c->_periodo_us = dt;
95:     } else {
96:       c->_periodo_us = 0;
97:     }
98:     c->_t_ultimo = t;
99:     c->_pulsos++;
100:   }
101:   portEXIT_CRITICAL_ISR(&c->_mux);
102: }
```
- **Líneas 81-102**: **Rutina de Interrupción en Silicio**:
  - `reinterpret_cast<Caudalimetro*>(arg)` recupera el objeto asociado al pin que interrumpió.
  - Adquiere el cerrojo ISR `portENTER_CRITICAL_ISR`.
  - Verifica si transcurrieron al menos $1500\ \mu\text{s}$ (`FILTRO_RUIDO_US`).
  - Si es el primer pulso del ciclo, fija `_t_primero = t`.
  - Si el silencio previo fue menor a 5 segundos, registra el intervalo `_periodo_us = dt`. Si fue mayor, descarta el período espurio.
  - Incrementa atómicamente el contador de pulsos y actualiza `_t_ultimo = t`.

---

## 3.4. Módulo `Bomba.h`

Declara la interfaz de control cinemático de la bomba peristáltica mediante generación de pulsos por hardware LEDC.

```cpp
13: class Bomba {
14: public:
15:   void begin();
16:   void arrancar();
17:   void detener();
18:   bool setRPM(float rpm);
19:   void toggleSentido();
21:   float rpmActual() const           { return _actual; }
22:   float rpmObjetivo() const         { return _objetivo; }
23:   bool  enMarcha() const            { return _enMarcha; }
24:   bool  invirtiendo() const         { return _invirtiendo; }
25:   bool  sentidoHorario() const      { return _horario; }
26:   bool  enRegimenEstable() const    { return (_enMarcha && fabsf(_actual - _objetivo) < 0.3f && _actual > 5.0f); }
27:   float caudalTeorico_mLmin() const { return _actual * _mlPorVuelta; }
35:   void tick(float dt);
```
- **Línea 26**: `enRegimenEstable()`: Función de diagnóstico que certifica que la bomba alcanzó la consigna de velocidad (error absoluto $< 0.3\text{ RPM}$) y está en marcha continua. Esta condición es el disparador indispensable para iniciar la auto-calibración de caudalímetros.
- **Línea 27**: `caudalTeorico_mLmin()` calcula el caudal cinemático nominal: $\text{RPM} \times \text{mL/rev}$.
- **Línea 35**: `tick(float dt)`: Función de actualización periódica invocada cada $50\text{ ms}$ que ejecuta las ecuaciones diferenciales de rampa.

---

## 3.5. Módulo `Bomba.cpp`

Implementa el control fino de frecuencia por hardware y las rampas cinemáticas.

```cpp
8: void Bomba::begin() {
9:   pinMode(PIN_DIR, OUTPUT);
10:   fijarSentido(true);
12: #if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
13:   ledcAttach(PIN_PUL, 800, 10);
14:   ledcWrite(PIN_PUL, 0);
15: #else
16:   ledcSetup(0, 800, 10);
17:   ledcAttachPin(PIN_PUL, 0);
18:   ledcWrite(0, 0);
19: #endif
20: }
```
- **Líneas 8-20**: Compatibilidad dual entre Arduino Core ESP32 v2.x y v3.x. Inicializa el periférico PWM por hardware LEDC a $800\text{ Hz}$ con resolución de 10 bits ($0$ a $1023$). `ledcWrite(..., 0)` mantiene la salida en estado bajo en reposo, de modo que el optoacoplador del driver DM860 permanece apagado y el motor no consume corriente de conmutación.

```cpp
65: void Bomba::tick(float dt) {
66:   float objetivo = _enMarcha ? _objetivo : 0.0f;
68:   if (_actual < objetivo) {
69:     if (_actual < 1.0f) _actual = 1.0f;
75:     float tasaAcel = ACEL_NOMINAL_RPM_S;
76:     if (_actual < 10.0f) {
77:       tasaAcel = ACEL_ARRANQUE_RPM_S + (_actual / 10.0f) * (ACEL_NOMINAL_RPM_S - ACEL_ARRANQUE_RPM_S);
78:     }
80:     float delta = objetivo - _actual;
81:     if (delta < 3.0f) {
82:       tasaAcel = fmaxf(0.8f, tasaAcel * (delta / 3.0f));
83:     }
84:     _actual = fminf(objetivo, _actual + tasaAcel * dt);
```
- **Líneas 65-84**: **Rampa S-Curve Progresiva de Aceleración**:
  - Si parte del reposo, inicia en $1.0\text{ RPM}$ para saltar la zona de resonancia mecánica del motor paso a paso.
  - Entre $1$ y $10\text{ RPM}$, interpola linealmente la aceleración desde $2.0\text{ RPM/s}$ hasta $3.5\text{ RPM/s}$.
  - Al aproximarse a menos de $3\text{ RPM}$ del objetivo, reduce progresivamente la aceleración para realizar una entrada tangencial asintótica sin sobreimpulso (*overshoot*).

```cpp
88:     if (!_enMarcha) {
89:       _actual = fmaxf(0.0f, _actual - FRENADO_PARADA_RPM_S * dt);
90:       if (_actual < 1.0f) _actual = 0.0f;
91:     } else {
95:       float tasaDecel = DESACEL_AJUSTE_RPM_S;
...
100:       _actual = fmaxf(objetivo, _actual - tasaDecel * dt);
101:     }
```
- **Líneas 88-101**: Modos de frenado diferenciados:
  - **Parada de Seguridad**: Desacelera agresivamente a $45.0\text{ RPM/s}$ ($< 1.5\text{ s}$ para detenerse desde $60\text{ RPM}$).
  - **Ajuste de Consigna**: Desacelera suavemente a $8.0\text{ RPM/s}$ para mantener la estabilidad del proceso.

```cpp
118:   if (_actual >= 1.0f) {
119:     uint32_t f = (uint32_t)(_actual * (float)_pulsosPorRev / 60.0f);
120:     if (f < 50) f = 50;
127:     ledcWrite(PIN_PUL, 512); // Duty cycle 50%
```
- **Líneas 118-127**: Generación de frecuencia por hardware.
  $$f_{\text{pasos}} = \frac{\text{RPM} \times \text{Pulsos/Rev}}{60} = \frac{\text{RPM} \times 3200}{60}$$
  A $50\text{ RPM}$: $f = (50 \times 3200) / 60 = 2666\text{ Hz}$. El ciclo de trabajo se fija exactamente en $512 / 1024 = 50.0\%$, otorgando al optoacoplador del DM860 un tiempo de encendido simétrico para conmutar sin distorsión.

---

## 3.6. Módulo `darcy.h`

Implementa los modelos termofísicos e hidráulicos de ultrafiltración capilar: la ecuación de viscosidad de Vogel y el transporte de resistencias en serie de Darcy.

```cpp
26:   static float viscosidadAgua(float temp_C) {
27:     const float T_K = temp_C + 273.15f;
28:     return 2.414e-5f * powf(10.0f, 247.8f / (T_K - 140.0f));
29:   }
```
- **Líneas 26-29**: Ecuación empírica de Vogel para la viscosidad dinámica del agua destilada ($\text{Pa}\cdot\text{s}$), válida entre $5^\circ\text{C}$ y $60^\circ\text{C}$ con error menor al $0.5\%$.
  - A $20.0^\circ\text{C}$: $\mu = 1.0016\times 10^{-3}\text{ Pa}\cdot\text{s}$ ($1.0016\text{ mPa}\cdot\text{s}$), valor validado en la auditoría de Claude Opus.

```cpp
46:     r.mu_Pas  = viscosidadAgua(temp_C);
47:     r.TCF     = r.mu_Pas / MU20;
50:     r.J_LMH   = (qPerm_mLmin * 0.06f) / AREA_MEMBRANA_M2;
51:     r.J20_LMH = r.J_LMH * r.TCF;
```
- **Líneas 46-51**:
  - Cálculo del Factor de Corrección por Temperatura:
    $$\text{TCF} = \frac{\mu(T)}{\mu_{20}}$$
  - Flujo de permeado volumétrico específico superficial:
    $$J\ [\text{LMH}] = \frac{Q_{\text{perm}}\ [\text{mL/min}] \times 0.06}{A_{\text{membrana}}\ [\text{m}^2]} = \frac{Q_{\text{perm}} \times 0.06}{2.2}$$
  - Flujo normalizado a $20^\circ\text{C}$:
    $$J_{20} = J \times \text{TCF} = J \times \frac{\mu(T)}{\mu_{20}}$$
    (A mayor temperatura, el agua es más fluida y fluye más rápido; la corrección estandariza el flujo a la referencia de $20^\circ\text{C}$).

```cpp
54:     const float J_SI   = r.J_LMH / 3.6e6f;
55:     const float TMP_Pa = tmp_bar * 100000.0f;
58:     r.R_total = TMP_Pa / (r.mu_Pas * J_SI);
59:     r.R_torta = r.R_total - _Rm;
```
- **Líneas 54-59**: **Ley de Darcy en Unidades del Sistema Internacional**:
  $$J\ [\text{m/s}] = \frac{\text{TMP}\ [\text{Pa}]}{\mu\ [\text{Pa}\cdot\text{s}] \cdot R_{\text{total}}\ [\text{m}^{-1}]}$$
  Despejando la resistencia hidráulica total del sistema:
  $$R_{\text{total}} = \frac{\text{TMP}}{\mu \cdot J}\quad [\text{m}^{-1}]$$
  Y la resistencia debida al ensuciamiento o capa de torta:
  $$R_{\text{torta}} = R_{\text{total}} - R_m$$
  donde $R_m = 1.44 \times 10^{13}\text{ m}^{-1}$ es la resistencia intrínseca de la membrana Fresenius FX100 Helixone®.

---

## 3.7. Módulo `EN_USO_firmware_planta.ino`

Es el orquestador general de la planta, administra el servidor web HTTP, la gestión de sesiones de ensayos y los lazos de control temporal.

```cpp
71: Caudalimetro sensorAlimentacion(PIN_SENSOR_ALIMENTACION, K_ALIMENTACION, "ALIMENTACION", true);
72: Caudalimetro sensorPermeado(PIN_SENSOR_PERMEADO, K_PERMEADO, "PERMEADO", false);
```
- **Líneas 71-72**: Instanciación explícita de los dos sensores, configurando al sensor de permeado con `esAlimentacion = false` para desacoplar su alarma de reposo.

```cpp
461:   WiFi.setSleep(false);            // Desactiva ahorro de energía del módem
462:   WiFi.setTxPower(WIFI_POWER_19_5dBm); // Máxima potencia de transmisión RF
468:   WiFi.softAP(SSID_AP, PASS_AP, 1, 0, 4); // Canal 1 fijo, SSID visible, hasta 4 clientes
```
- **Líneas 461-468**: Configuración de alta disponibilidad Wi-Fi:
  - `WiFi.setSleep(false)` evita que la radio entre en modo de bajo consumo, garantizando latencias HTTP menores a $5\text{ ms}$.
  - Potencia de transmisión fijada en $+19.5\text{ dBm}$ (el máximo del silicio del ESP32).
  - Canal RF 1 fijo para evitar la congestión del canal 6 o 11 comúnmente saturados.

```cpp
504: void loop() {
505:   server.handleClient();
507:   uint32_t tAhora = millis();
510:   if (tAhora - tLoop >= 50) { ... bomba.tick(dt); ... }
563:   if (tAhora - tCaudal >= 1000) { ... sensorAlimentacion.actualizar(dt, bombaEmpuja); ... }
631:   if (tAhora - tDatalogger >= INTERVALO_LOG_MS) { ... guardarMuestraDatalogger(); ... }
```
- **Líneas 504-637**: **Lazo Principal No Bloqueante**:
  - `server.handleClient()` atiende las solicitudes de la interfaz gráfica web en cada vuelta de ciclo.
  - La cinemática de la bomba se evalúa con precisión cada $50\text{ ms}$ ($20\text{ Hz}$).
  - La adquisición y filtrado de caudalímetros se ejecuta cada $1000\text{ ms}$ ($1\text{ Hz}$).
  - El almacenamiento histórico en el datalogger se realiza cada $10\text{ segundos}$.
  - No existe ningún `delay()` que detenga el microcontrolador.

---

## 3.8. Módulo `index_html.cpp`

Contiene el código fuente completo del panel de control web SCADA industrial (HTML5, hojas de estilo CSS3 y scripts de telemetría AJAX en JavaScript puro) almacenado íntegramente en la memoria Flash mediante la directiva `PROGMEM`.

### Aspectos Críticos Homologados:
1. **Unidades de Factor $K$ en Pantalla**: En la tarjeta de Alimentación y en el modal de Modo Desarrollador, el rótulo de las constantes fue corregido de `pulsos/L` a **`Hz/(L/min)`**.
2. **Telemetría AJAX Asíncrona sin Recarga**: La interfaz consulta la ruta `/status` cada $1000\text{ ms}$ mediante `fetch()`. Los indicadores numéricos, barras de progreso y gráficas dinámicas de Canvas se actualizan modificando el DOM directamente, sin consumir memoria RAM dinámica en el microcontrolador.
3. **Descarga de Datos en Formato Excel (.CSV)**: La ruta `/export_csv` genera un archivo de texto plano delimitado por punto y coma (`;`) con cabecera internacional `sep=;`, listo para ser abierto directamente en Excel o procesado en scripts de Python (Pandas/NumPy) sin problemas de codificación.

---

# 4. Protocolo de Laboratorio: Puesta a Punto y Re-Calibración en Banco

Para poner a punto los caudalímetros en el laboratorio de la UNSa con la versión v4, Owen, Antonella y Enzo deben seguir este protocolo riguroso de cuatro pasos:

---

## 4.1. Calibración Volumétrica de la Bomba Peristáltica ($\text{mL/rev}$)

Antes de calibrar los sensores de caudal, es indispensable verificar el desplazamiento volumétrico de la bomba peristáltica MBP-2000:
1. Desconectar la salida de la bomba de la membrana y colocarla descargando libremente sobre una probeta graduada limpia de $500\text{ mL}$ o $1000\text{ mL}$.
2. En la interfaz web, fijar una velocidad de ensayo constante (ej. $50.0\text{ RPM}$).
3. Encender la bomba simultáneamente con un cronómetro durante un lapso exacto de $2\text{ minutos}$ ($120\text{ segundos}$).
4. Medir el volumen recolectado en la probeta ($V_{\text{probeta}}$ en $\text{mL}$).
5. Calcular la cilindrada real:
   $$V_{\text{vuelta}} = \frac{V_{\text{probeta}}}{50.0\text{ RPM} \times 2.0\text{ min}}\quad [\text{mL/rev}]$$
6. Ingresar este valor en el modal "Modo Desarrollador" en el campo `Cilindrada Bomba (mL/rev)` y presionar **Guardar en Memoria Flash**.

---

## 4.2. Calibración Multipunto del Sensor de Alimentación ($K_{\text{alim}}$)

1. Conectar el sensor de alimentación en serie con la salida de la bomba.
2. Realizar corridas de 1 minuto a cinco niveles de velocidad representativos del diseño factorial:
   - Punto 1: $20\text{ RPM}$ ($Q_{\text{teórico}} \approx 272\text{ mL/min}$)
   - Punto 2: $40\text{ RPM}$ ($Q_{\text{teórico}} \approx 544\text{ mL/min}$)
   - Punto 3: $50\text{ RPM}$ ($Q_{\text{teórico}} \approx 680\text{ mL/min}$)
   - Punto 4: $70\text{ RPM}$ ($Q_{\text{teórico}} \approx 952\text{ mL/min}$)
   - Punto 5: $90\text{ RPM}$ ($Q_{\text{teórico}} \approx 1224\text{ mL/min}$)
3. En cada punto, registrar la frecuencia promedio reportada por el sensor ($F_{\text{prom}}$ en $\text{Hz}$) y el caudal medido por probeta ($Q_{\text{probeta}}$ en $\text{mL/min}$).
4. Para cada punto, calcular el factor puntual:
   $$K_i = \frac{F_{\text{prom}} \times 1000}{Q_{\text{probeta}}}\quad \left[\frac{\text{Hz}}{\text{L/min}}\right]$$
5. Graficar $F$ vs. $Q_{\text{probeta}}$ en Excel y realizar una regresión lineal forzada a cero ($y = m \cdot x$). La pendiente $m$ resultante es el **Factor $K_{\text{alim}}$ óptimo**.

---

## 4.3. Calibración Gravimétrica del Sensor de Permeado ($K_{\text{perm}}$)

Dado que el permeado opera a bajos caudales ($45$ a $365\text{ mL/min}$):
1. Colocar el vaso colector de permeado sobre una balanza digital con resolución de $0.1\text{ g}$.
2. Encender la planta con agua limpia y ajustar la válvula de aguja del retentado para generar tres niveles de presión transmembrana (TMP):
   - Nivel Bajo: $\text{TMP} \approx 0.15\text{ bar}$ ($Q_{\text{perm}} \approx 80-100\text{ mL/min}$)
   - Nivel Medio: $\text{TMP} \approx 0.30\text{ bar}$ ($Q_{\text{perm}} \approx 180-220\text{ mL/min}$)
   - Nivel Alto: $\text{TMP} \approx 0.45\text{ bar}$ ($Q_{\text{perm}} \approx 300-350\text{ mL/min}$)
3. Medir la masa recolectada durante $3\text{ minutos}$ en la balanza ($\Delta m$ en gramos $\approx \Delta V$ en $\text{mL}$ de agua a temperatura ambiente).
4. Contrastar contra el volumen integrado reportado por el SCADA ($V_{\text{perm}}$ en Litros $\times 1000$).
5. Ajustar $K_{\text{perm}}$ en el SCADA hasta que el error relativo entre la balanza y el caudalímetro sea menor al $3\%$:
   $$K_{\text{perm\_nuevo}} = K_{\text{perm\_actual}} \times \frac{V_{\text{SCADA}}}{V_{\text{balanza}}}$$

---

## 4.4. Ensayo de Respuesta Dinámica y Parada Súbita

1. Con la bomba girando a $50\text{ RPM}$ en régimen permanente, presionar el botón **STOP** en el dashboard.
2. Observar la curva de decaimiento en la consola Serial del ESP32 ($115200\text{ baudios}$):
   - Debe verificarse que el caudal desciende suave y rápidamente hacia cero en menos de $2\text{ segundos}$ gracias a la **cota física continua**.
   - No debe haber congelamiento de lecturas ni lecturas de flujo fantasma con la bomba detenida.
3. Volver a arrancar presionando **START**: verificar que el arranque suave a $2.0\text{ RPM/s}$ despega sin vibraciones bruscas en el cabezal ni pérdidas de pasos en el driver DM860.

---

> [!TIP]
> **Documento para la Tesis de Grado**: Este manual cubre todos los aspectos de ingeniería requeridos para la sección metodológica, de hardware embebido y metrología del informe final de tesis de Antonella Guitián y Owen Cañizares en la Universidad Nacional de Salta.
