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

Para adquirir en distribuidores locales o plataformas comerciales (Mercado Libre / distribuidores industriales):

### 🥇 Opción 1 Comercial (Compra Recomendada #1): Motorreductor Sinfín-Corona 12V Tipo 5840-31ZY (100 RPM)
* **Descripción:** Motor DC acoplado a reductora ortogonal de tornillo sinfín de acero y corona de bronce de alta resistencia con salida a $90^\circ$.
* **Velocidad Nominal:** **$100\text{ RPM}$** (permite regular por PWM hasta $20 - 30\text{ RPM}$ para floculación).
* **Torque Nominal:** **$20\text{ a }35\text{ kg}\cdot\text{cm}$ ($2.0 - 3.5\text{ Nm}$)**.
* **Torque de Bloqueo (*Stall*):** **$> 70\text{ kg}\cdot\text{cm}$ ($> 7\text{ Nm}$)**.
* **Eje de Salida:** Eje macizo de **$8\text{ mm}$ con plano (D-shaft)**.
* **Ventajas Críticas:**
  1. **Torque demoledor:** Mueve los $25\text{ kg}$ de agua con bentonita y *Opuntia* sin riesgo de atasco.
  2. **Irreversible (Autoblocante):** En reposo ($0\text{ RPM}$), el peso de la paleta y el fluido no pueden girar el motor hacia atrás.
  3. **Perfil chato:** El motor se monta horizontalmente sobre la tapa, bajando el centro de gravedad del reactor.

### 🥈 Opción Reciclada (Solución Ideal de Taller #1): Motor de Limpiaparabrisas Automotriz 12V DC
* **Descripción:** Motores de limpiaparabrisas de automotor (Bosch, Valeo, o universales de desarmadero/taller).
* **Velocidad:** Giran habitualmente a **$50 - 60\text{ RPM}$ (baja)** y **$80 - 100\text{ RPM}$ (alta)**.
* **Torque:** **$30\text{ a }80\text{ kg}\cdot\text{cm}$ ($3.0 - 8.0\text{ Nm}$)**. Prácticamente indestructibles, diseñados para trabajo continuo bajo lluvia y carga pesada.
* **Fijación:** Poseen brida con 3 puntos de anclaje roscados **M6**.
* **⚠️ Precauciones Mandatorias si se utiliza Limpiaparabrisas:**
  1. **Aislamiento de Chasis vs. Masa (GND):** En muchos modelos antiguos, el polo negativo (-) está internamente conectado a la carcasa metálica. **Es obligatorio medir con multímetro continuidad entre los cables y la carcasa**. Si hay continuidad, montar el motor con bujes/arandelas aislantes de nylon o goma para evitar poner toda la cuba de acero inoxidable a GND (evitando cortocircuitos con la conmutación PWM).
  2. **Conexión de Escobillas:** Poseen 3 bornes (Común, Baja y Alta). Conectar el terminal de **Alta Velocidad (~90-100 RPM)** y regular la velocidad hacia abajo mediante modulación PWM.
  3. **Pico de Corriente de Arranque (*Inrush*):** En régimen consume $1.5 - 3.0\text{ A}$, pero en el instante inicial ($t=0$) puede demandar picos de **$12 - 20\text{ A}$**. Requiere fuente de al menos $10\text{ A}$ y rampa de aceleración suave (*soft-start*) en software.

---

## 🛠️ 7. Montaje Mecánico sobre la Tapa de Acero Inoxidable

El eje del agitador de $50 - 70\text{ cm}$ de longitud actúa en el fluido como una viga en voladizo (*cantilever*). Las asimetrías hidrodinámicas generan momentos flectores severos sobre el motor. **Es terminantemente prohibido acoplar el eje rígidamente al eje del motorreductor.**

```
                     ARQUITECTURA DE MONTAJE MECÁNICO EN TAPA
                     ========================================

                 [ MOTORREDUCTOR 12V (5840-31ZY o Limpiaparabrisas) ]
                                          │
                 [ 3 o 4 Separadores / Standoffs M6 (40 - 50 mm) ]
                                          │
                 [ Acoplamiento Elástico Spider / Mandíbula (8mm - 8mm) ]
                                          │
    ═════════════[ Chumacera con Brida KFL08 (Rodamiento Autoalineante) ]═════════════
   │ ┌──────────────────────────────────────────────────────────────────────────┐ │
   │ │            Placa de Refuerzo de Grilón/Nylon (120x120 mm, e = 8-10 mm)    │ │
   │ ├──────────────────────────────────────────────────────────────────────────┤ │
   │ │            Tapa de Chapa de Acero Inoxidable del Sedimentador             │ │
   │ ├──────────────────────────────────────────────────────────────────────────┤ │
   │ │            Retén de Labio (Nitrilo) o Arandela PTFE (Escudo Antisalpicadura)│ │
   │ └──────────────────────────────────────────────────────────────────────────┘ │
    ══════════════════════════════════════════════════════════════════════════════════
                                          │
                                 [ Eje Inox Ø 8 mm ]
                                          │
                     [ Paleta 11 - 14 cm a 5-8 cm sobre Cono 60° ]
```

1. **Torreta con Separadores (*Standoffs*):** El motor se eleva $40 - 50\text{ mm}$ sobre la tapa mediante 3 o 4 pilares roscados M6.
2. **Acoplamiento Elástico (*Spider / Jaw Coupling* de 8mm a 8mm):** Absorbe desalineaciones angulares o radiales, evitando que el eje "latiguee" o sobrecargue los engranajes del motor.
3. **Chumacera Bridada Tipo KFL08 (8 mm):** Atornillada firmemente sobre la cara superior de la tapa. Su rodamiento autoalineante absorbe el **100% de las fuerzas radiales y el peso axial del eje y la paleta**, transmitiendo al motor únicamente esfuerzo de torsión pura.
4. **Placa de Refuerzo Sándwich Anti-Pandeo:** Para evitar que la chapa de la tapa ($1.0 - 1.5\text{ mm}$) vibre ("efecto tambor"), se instala una placa de refuerzo de **Grilón / Nylon de $120 \times 120\text{ mm}$ ($8 - 10\text{ mm}$ de espesor)** que actúa además como amortiguador acústico y aislador eléctrico.
5. **Estanqueidad Antisalpicaduras:** Por debajo de la tapa, se coloca un retén de labio de nitrilo o arandela de PTFE para evitar que el vapor o condensación haciendan al rodamiento KFL08.

---

## ⚡ 8. Etapa de Potencia: Descarte de L298N y Adopción de Driver BTS7960 (43A)

> [!CAUTION]
> **Descarte Definitivo del L298N:**  
> El integrado bipolar L298N provoca una caída de tensión de **$2.0 - 3.5\text{ V}$**. En $12\text{V}$, el motor solo recibe $\approx 9\text{V}$, perdiendo el $25\%$ del torque y velocidad de diseño. Además, su límite térmico ($2\text{A}$ por canal) se destruiría ante los picos de arranque del nuevo motorreductor. Se prohíbe también el uso del módulo rojo **IRF520** (no es de nivel lógico y se recalienta a $3.3\text{V}$).

### Driver Oficial Seleccionado: Módulo BTS7960 (IBT-2 de 43A)
* **Capacidad:** $43\text{ A}$ pico / $20\text{ A}$ continuos con disipador macizo de aluminio integrado.
* **Compatibilidad de Señal:** Entradas lógicas compatibles con **$3.3\text{V}$ nativos del ESP32**.
* **Frenado Activo (*Active Braking*):** Permite frenar dinámicamente de $100\text{ RPM}$ a $20\text{ RPM}$ al concluir el minuto de mezcla rápida sin desbordar el tiempo de floculación.
* **Frecuencia PWM Ultrasónica:** Se configura el periférico `LEDC` del ESP32 a **$18\text{ kHz}$** (rango inaudible para el oído humano), eliminando por completo el silbido (*coil whine*) en el laboratorio durante los 20 minutos de floculación.
* **Rampa Soft-Start por Firmware:** Arranque progresivo de 0 a 100% en **2.5 segundos** para eliminar el pico de corriente (*inrush*) y el golpe mecánico sobre el eje.

---

## 🌐 9. Arquitectura de Control: 1 ESP32 Centralizado + Manguera Umbilical GX16-4

Siguiendo el dictamen de ingeniería, **se adopta un único microcontrolador ESP32 centralizado en el maletín de control**, descartando la arquitectura distribuida por ESP-NOW (evitando congestión de radio 2.4 GHz, pérdidas de paquetes Wi-Fi y desincronización de estados).

```
   MALETÍN DE CONTROL CENTRAL                               REACTOR SEDIMENTADOR (25 L)
 ┌───────────────────────────────────────┐                 ┌─────────────────────────────┐
 │ • ESP32 DevKit V1 (SCADA + WebServer) │                 │ • Motor 12V (5840-31ZY)     │
 │ • Driver BTS7960 (43A)                │                 │ • Chumacera KFL08 en Tapa   │
 │ • Fuente de Potencia 12V / 10A        │                 │ • Boya de Nivel Inoxidable  │
 │                                       │                 │                             │
 │   [ Ficha Chasis Hembra GX16-4 ]      │  MANGUERA 4x1.0 │   [ Ficha Aérea Macho GX16 ]│
 │   • Pin 1: Motor PWM (+) ─────────────┼─────────────────┼──► Borne Motor (+)          │
 │   • Pin 2: Motor PWM (-) ─────────────┼─────────────────┼──► Borne Motor (-)          │
 │   • Pin 3: Boya Señal (GPIO 32) ──────┼─────────────────┼──► Contacto Boya (C1)       │
 │   • Pin 4: GND Filtro ────────────────┼─────────────────┼──► Contacto Boya (C2)       │
 └───────────────────────────────────────┘                 └─────────────────────────────┘
```

* **Filtro de Ruido en Entrada de Boya:** En el maletín, se coloca una resistencia *pull-up* física externa de **$2.2\text{ k}\Omega$ a $3.3\text{V}$** y un capacitor cerámico de **$100\text{ nF}$ a GND** en el pin `GPIO 32` del ESP32 para suprimir cualquier transitorio inducido por el cableado del motor.

---

## 🌊 10. Reglas de Diseño Hidrodinámico para el Reactor de 25 Litros

1. **Supresión de Rotación en Bloque (Anti-Vórtice) sin Soldaduras:**  
   Las paredes lisas de acero inoxidable generan vórtice central y giro en masa del agua. Para evitarlo sin tener que soldar bafles interiores al tanque:
   * **Montaje Excéntrico:** Se perfora la tapa desplazando el centro del eje a una distancia de **$1/6$ a $1/4$ del radio del tanque** (unos $2.5 - 4\text{ cm}$ del centro). La asimetría rompe el vórtice central e incrementa el gradiente $G$ de cizalla.
   * *Alternativa:* Introducir una canasta/aro portabafles desmontable con 4 deflectores verticales de $\approx 3\text{ cm}$ de ancho.
2. **Control de Velocidad Tangencial ($V_{tip}$):**  
   Para no cizallar los macroflóculos frágiles formados por *Opuntia*:
   * Diámetro de la paleta: **$d = 11\text{ a }14\text{ cm}$** ($0.35 - 0.45 \cdot D_{\text{tanque}}$).
   * A $25\text{ RPM}$, la velocidad tangencial en la punta es de apenas **$0.16\text{ m/s}$**, muy por debajo del umbral destructivo de flóculos ($0.35\text{ m/s}$).
3. **Altura de la Paleta y Protección del Cono de 60°:**  
   La paleta debe montarse en el tercio inferior del cilindro recto, **$5\text{ a }8\text{ cm}$ por encima de la línea de soldadura donde comienza el cono**. Esto mantiene el fondo cónico como una **zona quieta de decantación y espesamiento de lodos (*sludge blanket*)**, impidiendo su resuspensión.
4. **Toma de Succión hacia Ultrafiltración:**  
   La extracción de agua clarificada debe realizarse **$3\text{ a }5\text{ cm}$ por encima de la transición cilindro-cono** o mediante un pescante lateral, reservando la válvula del vértice inferior del cono **únicamente para la purga de barros**.

