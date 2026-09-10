# 🔌 GUÍA MAESTRA DE CONEXIONADO FÍSICO: DRIVER LEADSHINE DM860 & MOTOR NEMA 34

Esta guía técnica describe el conexionado de taller paso a paso para el **Hito 1**, garantizando que el conexionado eléctrico entre el Transformador, el Driver DM860, el Motor NEMA 34 y el ESP32 sea 100% seguro y libre de ruidos electromagnéticos.

---

## 🧭 1. Distribución Fisiológica de los Bornes del Driver DM860

El driver digital Leadshine DM860 cuenta con dos bloques de borneras independientes:

```
                  VISTA FRONTAL DEL DRIVER LEADSHINE DM860
                  ========================================

     ┌───────────────────────────────────────────────────────────┐
     │  [LED VERDE: PWR]  [LED ROJO: ALARM]                      │
     │                                                           │
     │  ┌── BORNERA SUPERIOR: SEÑALES LÓGICAS (ESP32 - 3.3V) ──┐ │
     │  │  [ PUL+ ] ──► Cable a GPIO 18 del ESP32              │ │
     │  │  [ PUL- ] ──┐                                        │ │
     │  │  [ DIR+ ] ──┼► Cable a GPIO 19 del ESP32             │ │
     │  │  [ DIR- ] ──┴► Puenteados a GND Común del ESP32      │ │
     │  │  [ ENA+ ] ──► (DESCONECTADO / AIRE)                  │ │
     │  │  [ ENA- ] ──► (DESCONECTADO / AIRE)                  │ │
     │  └──────────────────────────────────────────────────────┘ │
     │                                                           │
     │  ┌── SELECTOR DIP SWITCHES (SW1 a SW8) ─────────────────┐ │
     │  │  SW1: OFF | SW2: ON  | SW3: ON  (Corriente ~4.2A RMS)│ │
     │  │  SW4: OFF ❗ (50% Standstill Current en Reposo) ❗   │ │
     │  │  SW5: ON  | SW6: OFF | SW7: ON  | SW8: ON (1600 P/R)│ │
     │  └──────────────────────────────────────────────────────┘ │
     │                                                           │
     │  ┌── BORNERA INFERIOR: POTENCIA Y BOBINAS DEL MOTOR ────┐ │
     │  │  [  AC  ] ──┐ Entrada 24 VAC desde Transformador      │ │
     │  │  [  AC  ] ──┘ (Rectifica internamente a ~34V DC)     │ │
     │  │  [  A+  ] ──┐ Fase A del Bobinado NEMA 34            │ │
     │  │  [  A-  ] ──┘ (Par de cables Fase 1)                 │ │
     │  │  [  B+  ] ──┐ Fase B del Bobinado NEMA 34            │ │
     │  │  [  B-  ] ──┘ (Par de cables Fase 2)                 │ │
     │  └──────────────────────────────────────────────────────┘ │
     └───────────────────────────────────────────────────────────┘
```

---

## ⚡ 2. Circuito de Potencia (Transformador 220V ➔ 24 VAC)

### ¿Por qué se alimenta con AC directo?
* El driver Leadshine DM860 fue diseñado para recibir tanto corriente continua ($24\text{ a }110\text{ VDC}$) como corriente alterna ($18\text{ a }80\text{ VAC}$).
* Posee internamente un puente rectificador de onda completa de grado industrial y capacitores de filtrado de alta capacidad.
* Al conectar los **$24\text{ VAC}$** del secundario del transformador ($150\text{ VA}$), la tensión se rectifica y filtra elevándose al valor pico:
  $$V_{\text{bus}} = V_{\text{RMS}} \times \sqrt{2} = 24\text{ V} \times 1.4142 \approx \mathbf{33.94\text{ VDC}}$$
* **Ventaja clave**: Tensión óptima para vencer la inductancia de las bobinas del NEMA 34 a altas revoluciones, garantizando el torque máximo de $4.0\text{ Nm}$.

### Conexión:
1. Conectar los dos cables del secundario de $24\text{ VAC}$ del transformador a los bornes marcados como `AC` y `AC` (o `VCC` y `GND`).
2. **Polaridad**: Al ser corriente alterna, no importa cuál cable va a cuál borne.
3. **Calibre de cable**: Utilizar cable de cobre de al menos $1.5\text{ mm}^2$ de sección.

---

## 🌀 3. Identificación y Conexión de las Fases del Motor NEMA 34 (8 Cables - Modelo FX3.162)

Según el plano oficial del fabricante (**CNC Insumos S.R.L.**), el motor posee **8 cables** agrupados en 4 bobinados (2 bobinados por fase).

### ⭐ Conexión Bipolar SERIE (Consumo 3.0A - Recomendada para Bomba Peristáltica):
Entrega el torque máximo de **$4.5\text{ Nm}$** con solo **$3.0\text{ A}$** de corriente, evitando calentar el motor y relajando el transformador:
* **Fase A**:
  * Cable **ROJO** ──► Borne **`A+`** del DM860
  * Cable **AMARILLO** + Cable **AZUL** ──► **Unir entre sí y aislar con cinta** (NO van al driver)
  * Cable **NEGRO** ──► Borne **`A-`** del DM860
* **Fase B**:
  * Cable **BLANCO** ──► Borne **`B+`** del DM860
  * Cable **NARANJA** + Cable **MARRÓN** ──► **Unir entre sí y aislar con cinta** (NO van al driver)
  * Cable **VERDE** ──► Borne **`B-`** del DM860

### Conexión Alternativa Bipolar PARALELO (Consumo 6.0A):
Para altas velocidades (>600 RPM):
* **Rojo + Azul** (unidos) ──► Borne **`A+`**
* **Amarillo + Negro** (unidos) ──► Borne **`A-`**
* **Blanco + Marrón** (unidos) ──► Borne **`B+`**
* **Naranja + Verde** (unidos) ──► Borne **`B-`**

> *Para el detalle completo con tablas de inductancia y resistencia, consultar [Montaje/GUIA_OPTIMIZACION_MOTOR_NEMA34_Y_DIP_SWITCHES.md](./Montaje/GUIA_OPTIMIZACION_MOTOR_NEMA34_Y_DIP_SWITCHES.md).*

---

## 💻 4. Conexión de Control Lógico con el ESP32 (Cátodo Común a 3.3V)

### Por qué Cátodo Común y NO Ánodo Común:
* Los optoacopladores del DM860 tienen un diodo infrarrojo interno con caída de tensión $V_F \approx 1.2\text{V}$ y resistencia limitadora de $270\,\Omega$.
* Si conectamos en Ánodo Común a $+5\text{V}$ y el ESP32 envía $3.3\text{V}$, queda un voltaje residual de $5\text{V} - 3.3\text{V} = 1.7\text{V} > 1.2\text{V}$, haciendo que el optoacoplador conduzca permanentemente y deforme la señal.
* **En Cátodo Común a GND**:
  * Cuando el pin del ESP32 manda `3.3V` (`HIGH`), la corriente es $\approx \frac{3.3\text{V} - 1.2\text{V}}{270\,\Omega} = 7.8\text{ mA}$ (encendido perfecto del optoacoplador).
  * Cuando el pin del ESP32 manda `0.0V` (`LOW`), la corriente es $0.0\text{ mA}$ (apagado absoluto).

### Esquema Pin a Pin:
* `GPIO 18` del ESP32 ──────────► Borne **`PUL+`**
* `GPIO 19` del ESP32 ──────────► Borne **`DIR+`**
* `GND` del ESP32     ────────┬─► Borne **`PUL-`**
                              └─► Borne **`DIR-`**
* Bornes **`ENA+`** y **`ENA-`** ──► **DEJAR DESCONECTADOS (Al aire)**.

---

## 🛡️ 5. Los DIP Switches y la Prevención Térmica

```
  SW1   SW2   SW3   SW4   SW5   SW6   SW7   SW8
 [OFF]  [ON]  [ON] [OFF]  [ON]  [OFF] [ON]  [ON]
 └───────────────┘ └───┘ └─────────────────────┘
     Corriente      50%         1600 Pulsos/Rev
    ~4.2A Peak    Reposo          (8 micropasos)
```

* **El Interruptor Crítico es SW4**:
  * En posición **`OFF`**, apenas la bomba se detiene, el driver DM860 recorta la corriente suministrada al $50\%$. Como la disipación térmica sigue la ley cuadrática de Joule ($P = I^2 R$), **el calor generado cae un $75\%$**.
  * Esto permite que el motor opere durante horas en el laboratorio manteniéndose en temperatura ambiente ($\approx 28-35^\circ\text{C}$).

---

## 🎯 Entregable Concreto de esta Guía
* **Circuito de potencia y control del DM860 100% cableado**: Conexión de alimentación AC de 24V, aislamiento de bobinados en serie del motor NEMA 34 y señales lógicas en Cátodo Común a 3.3V desde el ESP32.

---

## 📋 Lista de Verificación (Checklist de Avance)
- [ ] Transformador de 24 VAC desconectado de la red de 220V durante el proceso de cableado.
- [ ] Conexión de los 2 cables secundarios de 24 VAC a los bornes `AC / AC` del DM860 con cable taller $\ge 1.5\text{ mm}^2$.
- [ ] Fases del motor NEMA 34 conectadas en Bipolar Serie:
  - [ ] Cable Amarillo y Cable Azul empalmados entre sí y aislados.
  - [ ] Cable Naranja y Cable Marrón empalmados entre sí y aislados.
  - [ ] Cable Rojo a `A+` y Cable Negro a `A-`.
  - [ ] Cable Blanco a `B+` y Cable Verde a `B-`.
- [ ] Conexión de señales lógicas al ESP32:
  - [ ] `GPIO 18` conectado a `PUL+`.
  - [ ] `GPIO 19` conectado a `DIR+`.
  - [ ] `GND` del ESP32 puenteado a `PUL-` y `DIR-`.
  - [ ] `ENA+` y `ENA-` dejados al aire (desconectados).
- [ ] Verificación visual de los DIP switches: **SW4 verificado en `OFF`**.
- [ ] Encendido seguro: LED verde encendido en el DM860 sin alarma roja.
