# ⚙️ GUÍA DE INGENIERÍA: DIMENSIONAMIENTO DE TORQUE Y MOTORREDUCTOR DEL AGITADOR
## Reactor Sedimentador Cónico de 25 Litros — Hito 3
**Proyecto:** Planta Piloto de Ultrafiltración FX100 & Reactor de Coagulación-Sedimentación  
**Tesistas:** Antonella Guitián & Owen Cañizares  
**Codirector:** Ing. Enzo  
**Fecha:** Octubre 2026  

---

## 🎯 1. Objetivo Técnico
Calcular rigurosamente las fuerzas hidrodinámicas, el par resistente de arranque y la potencia mecánica requerida para mezclar un volumen real de **$25\text{ Litros}$** de agua con turbidez (arcillas/coloides y biocoagulante *Opuntia ficus-indica*) en el reactor cónico, estableciendo las especificaciones de torque, potencia y reducción para seleccionar y adquirir el **motorreductor de $12\text{V DC}$ definitivo** que reemplace al actuador insuficiente actual.

---

## 📐 2. Parámetros Geométricos y Físicos del Sistema (25 Litros)

```
                       ESQUEMA MECÁNICO DEL AGITADOR (25 L)
                       ====================================

                     [ MOTORREDUCTOR 12V DC (25-40W) ]
                     • n_max = 100 RPM
                     • Torque: 15 - 25 kg·cm nominal
                                    │
                     [ Acople Flexible 6mm - 6mm ]
                                    │
             ========== [ BUJE DE BRONCE / GUÍA EN TAPA ] ==========
            │                                                      │
            │           Eje de Acero Inoxidable (Ø 6 mm)           │
            │           Longitud sumergida: ~35 - 40 cm            │
            │                                                      │
            │ ~~~~~~~ Nivel de Fluido (25 Litros / 25 kg) ~~~~~~~~ │
            │                                                      │
            │           [ PALETA PLANA PESADA DE ACERO ]           │
            │           • Diámetro barrido: D = 140 mm (0.14 m)    │
            │           • Alto de paleta: w = 50 mm (0.05 m)       │
            \                                                      /
             \            FONDO CÓNICO DECANTADOR                 /
              \                                                  /
               \=============== [ PURGA DE LODOS ] =============/
```

### Tabla de Variables de Entrada:
* **Volumen Útil de Fluido ($V$):** $25\text{ Litros} = 0.025\text{ m}^3$ ($\approx 25\text{ kg}$ de masa líquida).
* **Densidad de la Suspensión ($\rho$):** $1050\text{ kg/m}^3$ (agua con carga de bentonita/arcilla coloidal).
* **Viscosidad Dinámica ($\mu$):** $1.05 \times 10^{-3}\text{ Pa}\cdot\text{s}$ (a $20^\circ\text{C}$).
* **Velocidad de Giro Máxima ($N_{\max}$):** $100\text{ RPM} = \frac{100}{60} = 1.67\text{ rev/s}$.
* **Velocidad Angular ($\omega$):** $\omega = \frac{2\pi \times 100}{60} = 10.47\text{ rad/s}$.
* **Diámetro del Impulsor/Paleta ($D$):** $140\text{ mm} = 0.14\text{ m}$ (radio $R = 0.07\text{ m}$).
* **Altura de la Paleta ($w$):** $50\text{ mm} = 0.05\text{ m}$.
* **Área Frontal de Arrastre ($A$):** $A = D \times w = 0.14\text{ m} \times 0.05\text{ m} = 0.007\text{ m}^2$.

---

## 🔬 3. Cálculo Hidrodinámico de Fuerzas y Torque

### 3.1. Régimen de Flujo (Número de Reynolds del Impulsor)
$$Re = \frac{\rho \cdot N \cdot D^2}{\mu} = \frac{1050 \times 1.67 \times (0.14)^2}{1.05 \times 10^{-3}} \approx \frac{34.37}{0.00105} \approx 32700$$

Al ser $Re > 10000$, el régimen es **plenamente turbulento**, lo que significa que la resistencia al giro es dominada por las fuerzas de inercia y arrastre de presión sobre la paleta.

### 3.2. Fuerza de Arrastre Hidrodinámico ($F_D$)
Para una paleta plana rectangular girando perpendicularmente al fluido, el coeficiente de arrastre es $C_D \approx 1.4$.
La velocidad tangencial promedio en el centro de presión de la pala ($r_m \approx 0.7 \cdot R = 0.049\text{ m}$) es:
$$v_m = \omega \cdot r_m = 10.47\text{ rad/s} \times 0.049\text{ m} \approx 0.513\text{ m/s}$$

$$F_D = \frac{1}{2} C_D \cdot \rho \cdot v_m^2 \cdot A = \frac{1}{2} \times 1.4 \times 1050 \times (0.513)^2 \times 0.007 \approx 1.36\text{ N}$$

### 3.3. Torque Hidrodinámico Dinámico ($T_{\text{hidro}}$)
$$T_{\text{hidro}} = F_D \cdot r_m = 1.36\text{ N} \times 0.049\text{ m} \approx 0.067\text{ Nm} \approx 0.68\text{ kg}\cdot\text{cm}$$

---

## ⚠️ 4. Diagnóstico de Falla: ¿Por qué se Clavó el Motorreductor Anterior?

En el cálculo teórico continuo, $0.68\text{ kg}\cdot\text{cm}$ parece un valor bajo. Sin embargo, en el banco físico intervienen **cuatro cargas mecánicas parásitas dominantes** que frenan motores subdimensionados:

1. **Inercia de Despegue de 25 kg de Fluido (*Breakaway / Starting Torque*):**
   Acelerar desde reposo ($0\text{ RPM}$) una masa estática de $25\text{ kg}$ de agua viscosa con sólidos requiere entre **$5\text{ y }8\text{ veces}$** el torque de régimen continuo:
   $$T_{\text{arranque fluido}} \approx 3.5 \text{ a } 5.5\text{ kg}\cdot\text{cm}$$

2. **Peso Propio de la Paleta y Fricción del Eje Largo:**
   El eje de acero inoxidable de $35\text{ cm}$ más la paleta metálica pesada transmiten carga axial y momentos flectores sobre el buje de la tapa. El rozamiento en seco o con leve desalineación agrega:
   $$T_{\text{fricción mecánica}} \approx 3.0 \text{ a } 6.0\text{ kg}\cdot\text{cm}$$

3. **Sedimento Coloidal en el Fondo Cónico:**
   Al formarse los flóculos densos de bentonita y *Opuntia*, el lodo decantado aumenta la viscosidad aparente en la zona baja del cono, generando un arrastre viscoso adicional.

4. **Caída de Tensión Interna del Driver L298N:**
   El integrado L298N opera con transistores bipolares Darlington que presentan una caída de tensión interna de $\approx 1.8\text{ a }2.0\text{ V}$. Con una fuente de $12\text{V}$, al motor solo le llegan **$10.0\text{V}$**, reduciendo su torque disponible en un **$30\%$**.

---

## 📊 5. Parámetros de Selección del Nuevo Motorreductor

Aplicando un factor de servicio de ingeniería para agitación pesada ($FS = 4.0$):

| Parámetro Requerido | Valor de Diseño Mínimo | Valor Recomendado |
| :--- | :---: | :---: |
| **Velocidad Máxima a 12V** | $100\text{ RPM}$ | $100 - 150\text{ RPM}$ (regulable por PWM) |
| **Torque Nominal de Trabajo** | **$12\text{ kg}\cdot\text{cm}$ ($1.2\text{ Nm}$)** | **$18 - 25\text{ kg}\cdot\text{cm}$ ($1.8 - 2.5\text{ Nm}$)** |
| **Torque de Bloqueo / Parada (*Stall*)** | **$\ge 35\text{ kg}\cdot\text{cm}$ ($3.5\text{ Nm}$)** | **$\ge 50 - 70\text{ kg}\cdot\text{cm}$ ($5.0 - 7.0\text{ Nm}$)** |
| **Potencia Mecánica de Salida** | $12\text{ W}$ | $20 - 30\text{ W}$ |
| **Potencia Eléctrica del Motor** | $25\text{ W}$ | $30 - 45\text{ W}$ (Consumo: $2.0 - 3.5\text{ A}$ a $12\text{V}$) |
| **Diámetro del Eje de Salida** | $6\text{ mm}$ (con rebaje en D) | $8\text{ mm}$ o chavetero |

---

## 🛒 6. Opciones Concretas de Motorreductores en el Mercado (Argentina)

Para comprar en Mercado Libre o distribuidores de automatización en Salta/Argentina:

### 🥇 Opción 1: Motorreductor Sinfín-Corona 12V Tipo 5840-31ZY o GW600 (⭐ LA MEJOR OPCIÓN)
* **Descripción:** Motor DC acoplado a una caja reductora ortogonal de tornillo sinfín y engranajes de bronce/acero templado con salida a $90^\circ$.
* **Velocidad:** Elegir versión de **$100\text{ RPM}$ o $160\text{ RPM}$**.
* **Torque Nominal:** **$20\text{ a }35\text{ kg}\cdot\text{cm}$**.
* **Torque de Bloqueo:** **$> 70\text{ kg}\cdot\text{cm}$**.
* **Ventajas para el Sedimentador:**
  1. **Torque demoledor:** Mueve los 25 litros sin pestañear, incluso arrancando cargado de bentonita.
  2. **Irreversible (Autoblocante):** El peso de la paleta y el agua no pueden hacer girar el motor hacia atrás.
  3. **Eje robusto:** Eje de salida de $8\text{ mm}$ roscado o con rebaje plano D.

### 🥈 Opción 2: Motorreductor Cilíndrico de 37 mm Reforzado Tipo JGB37-550 o JGB37-555 (12V)
* **Descripción:** Caja reductora planetaria o de cascada cilíndrica de metal de $37\text{ mm}$ con motor tamaño 550 o 555.
* **Velocidad:** **$100\text{ RPM}$ o $120\text{ RPM}$**.
* **Torque Nominal:** **$12\text{ a }18\text{ kg}\cdot\text{cm}$**.
* **Torque de Bloqueo:** **$35\text{ a }45\text{ kg}\cdot\text{cm}$**.
* **Ventajas:** Diseño recto, compacto, eje de $6\text{ mm}$ con rebaje en D, fácil de montar verticalmente sobre la torreta de la tapa.

### 🥉 Opción 3: Motorreductor de Limpiaparabrisas o Alzacristales Automotriz 12V (Solución Rápida de Taller)
* **Descripción:** Motores auxiliares de automotor (Bosch / Valeo / universal).
* **Características:** Operan a $12\text{V}$, tienen caja reductora sinfín-corona integrada, giran a $\approx 60 - 90\text{ RPM}$ y entregan más de **$30\text{ kg}\cdot\text{cm}$** de par. Son prácticamente indestructibles para mover 25 L.

---

## ⚡ 7. Conexión Eléctrica y Optimización del Driver L298N

Si el nuevo motor consume entre $1.5\text{ A}$ y $3.0\text{ A}$ en los picos de arranque, el módulo L298N tradicional (que tiene límite de 2A por puente individual) debe conectarse en **PARALELO** para duplicar su capacidad de corriente a **$3.5\text{ A}$ continuos / $4.0\text{ A}$ pico**:

```
      CONEXIÓN EN PARALELO DEL L298N PARA MAYOR POTENCIA (3.5A)
      ==========================================================

      ESP32 DevKit V1                        DRIVER PUENTE H L298N
    ┌─────────────────┐                     ┌────────────────────────┐
    │ GPIO 4 (PWM) ───┼────────────────────►│ ENA y ENB (Puenteados)  │
    │ GPIO 16 (DIR) ──┼───┬────────────────►│ IN1                    │
    │                 │   └────────────────►│ IN4                    │
    │ GPIO 17 (DIR) ──┼───┬────────────────►│ IN2                    │
    │                 │   └────────────────►│ IN3                    │
    │             GND ┼────────────────────►│ GND (Masa Común)       │
    └─────────────────┘                     │                        │
                                            │ OUT1 ──┬───────────────┼──► Cable Motor (+)
                                            │ OUT4 ──┘               │
                                            │ OUT2 ──┬───────────────┼──► Cable Motor (-)
                                            │ OUT3 ──┘               │
                                            └────────────────────────┘
```

> [!TIP]
> **Estrategia de Arranque en Software (Anti-Stall):**  
> Para arrancar la paleta pesada en los 25 litros de agua, en el firmware no se debe aplicar una velocidad baja directa. Se debe enviar un pulso de **PWM = 255 (100%) durante 300 ms** para romper la inercia de despegue y luego reducir inmediatamente al PWM deseado (p. ej. PWM = 90 para mezcla lenta de 30 RPM o PWM = 200 para 100 RPM).
