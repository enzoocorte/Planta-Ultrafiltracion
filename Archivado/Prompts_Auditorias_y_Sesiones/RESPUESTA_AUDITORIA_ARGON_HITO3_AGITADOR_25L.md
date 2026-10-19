# 📋 DICTAMEN TÉCNICO OFICIAL DE ARGON: AGITADOR Y REACTOR SEDIMENTADOR DE 25L (HITO 3)
## Auditoría Externa de Mecatrónica, Hidrodinámica y Control — Planta Piloto FX100
**Fecha:** 19 de Octubre de 2026  
**Auditor Externo:** Argon (AI Peer Reviewer)  
**Destinatarios:** Ing. Enzo Corte (Codirector), Antonella Guitián & Owen Cañizares (Tesistas UNSa)  
**Referencia del Proyecto:** Hito 3 — Reactor Sedimentador Cónico de 25 Litros

---

## 📌 1. Diagnóstico Inicial y Validación de Alerta de Taller
El diagnóstico del técnico de laboratorio es **100% acertado**. Los micromotorreductores estándar de banco (como la serie N20 o incluso los JGA25-370) tienen trenes de engranajes rectos miniatura cuyos dientes sufren cizallamiento o bloqueo térmico al enfrentar la inercia rotacional de arrancar desde el reposo una paleta en 25 litros de agua con sólidos en suspensión y el aumento de viscosidad aparente que introduce el mucílago de *Opuntia ficus-indica*.

Además, hay un desafío electromecánico crítico en el proceso: **reducir la velocidad de 100 RPM (coagulación) a 20 RPM (floculación) mediante PWM reduce la tensión efectiva al ~20-25%**, lo que desploma el par electromagnético de un motor DC convencional justo cuando debe sostener 20 minutos continuos de agitación lenta.

---

## ⚙️ 2. Selección del Motorreductor: ¿Comercial o Reciclado?

Los valores estimados por Antigravity (**1.5 a 2.5 Nm nominales / 15–25 kg·cm** y **25–40 W**) son una base excelente porque otorgan el margen de torque necesario para operar al 25% de ciclo útil de PWM (20–30 RPM) sin que el eje se detenga (*stall*).

| Opción | Modelo / Tipo | Veredicto Técnico | Ventajas y Puntos Críticos |
| :--- | :--- | :--- | :--- |
| **Comercial (Ideal)** | **Motorreductor Sinfín-Corona tipo 5840-31ZY (12V, 100 RPM)** | **Recomendado #1 (Compra)** | Corona de bronce de alta resistencia, eje de salida de **8 mm** con plano (D-shaft), **auto-bloqueante** (en reposo a 0 RPM la paleta queda fija) y perfil chato (el motor queda horizontal sobre la tapa, bajando el centro de gravedad). |
| **Comercial (Alternativa)** | **Planetario PG45 (45 mm, 12V, 100 RPM)** | **Recomendado #2 (Compra)** | Altísima eficiencia mecánica (>80%) y excelente entrega de par a bajas RPM. Como desventaja, es largo y queda vertical como una "torre" sobre la tapa. *(Nota: descartar los tipo JSX69/JGY370 pequeños porque su torque nominal real a 100 RPM ronda apenas los 2 a 4 kg·cm).* |
| **Reciclado (Ideal)** | **Motor de Limpiaparabrisas Automotriz (12V DC)** | **Recomendado #1 (Taller)** | Indestructible, diseñado para servicio continuo bajo carga pesada. Suelen girar a **50–60 RPM en baja y 80–100 RPM en alta**, entregando entre **30 y 80 kg·cm (3 a 8 Nm)**. Ya traen brida con 3 puntos de anclaje roscados M6. |
| **Reciclado (Alternativa)** | **Motor de Alzacristales (Levanta vidrios 12V)** | **Aceptable con reservas** | Tienen muy buen torque (~30 kg·cm) y perfil ultra plano, pero están diseñados para **ciclo intermitente (S2)**; en 20 minutos continuos de floculación pueden recalentar si no se les adosa un disipador o cooler, y su acople de salida suele ser un engranaje plástico/estriado no estándar. *(Descartar motor de microondas: es AC 220V sincrónico de 5 RPM fijas y ~4W).* |

### ⚠️ Precauciones críticas si usan Motor de Limpiaparabrisas:
1. **Masa aislada vs. Chasis negativo:** En muchos limpiaparabrisas antiguos, el polo negativo está internamente unido a la carcasa metálica del motor. **Verificar con multímetro continuidad entre los cables y el chasis**. Si hay continuidad y montan el motor directo a la tapa de acero inoxidable, pondrán todo el tanque a GND (riesgo de lazos de masa o cortocircuito si conmutan por el lado bajo). Si tiene masa al chasis, el motor debe montarse sobre bujes o arandelas aislantes de nylon/goma.
2. **Conexión de escobillas:** Tienen 3 escobillas (Común, Baja velocidad, Alta velocidad). Conectarlo en el terminal de **Alta Velocidad (~90-100 RPM)** y regular hacia abajo por PWM.
3. **Corriente de arranque (*Inrush current*):** Un motor de limpiaparabrisas consume **1.5 A a 3 A** en régimen libre/carga suave, pero en el instante de arranque (*t = 0*) puede pedir un pico de **12 A a 20 A** durante 100 ms. La fuente de 12V debe poder entregar al menos 10 A (o sumar un capacitor electrolítico grande en la línea de potencia, más una rampa de arranque suave en el código del ESP32).

---

## 🛠️ 3. Montaje Mecánico sobre la Tapa de Acero Inoxidable

Un eje de agitador de **50 a 70 cm de longitud** sumergido en un fluido en movimiento actúa como una viga en voladizo (*cantilever*). Cualquier mínima asimetría hidrodinámica genera fuerzas radiales que, multiplicadas por el brazo de palanca del eje, producen un **momento flector** severo en la punta del motor.

* **¿Acople directo o rodamiento intermedio?**
  **Es indispensable colocar un soporte mecánico y un acople flexible.** Nunca conectar el eje largo de forma rígida y directa al eje del motorreductor; los pequeños rodamientos o bujes de bronce internos de la caja reductora tomarán juego en pocas semanas.
* **Arquitectura de montaje recomendada (de arriba hacia abajo):**
  1. **Torreta o separadores (*standoffs*):** Montar el motor sobre 3 o 4 pilares de 40–50 mm de altura sobre la tapa.
  2. **Acoplamiento elástico (Tipo *Spider / Jaw coupling* o fuelle, ej. 8 mm a 8 mm o 10 mm):** Se ubica en el espacio entre la tapa y el motor. Absorbe cualquier desalineación angular entre el agujero de la tapa y el eje del motor, evitando que el eje "latiguee".
  3. **Rodamiento con brida (Chumacera de pared tipo KFL08 o FL000 en aleación de zinc o inox):** Atornillado directamente sobre la cara superior de la tapa de acero inoxidable. Este rodamiento autoalineante absorbe el 100% de las cargas radiales y el peso axial del eje + paleta, dejando que el motor entregue **únicamente torque puro de torsión**.

### Estanqueidad y Vibraciones sobre la Chapa:
* **Refuerzo de rigidez (Anti-pandeo):** La chapa de las tapas de tanques de 25 L (usualmente de 1.0 a 1.5 mm de espesor) tiende a flexionar y vibrar ("efecto tambor"). Colocar una **placa o disco de refuerzo** de 120x120 mm (puede ser de Grilón/Nylon/Delrin de 8–10 mm de espesor, o planchuela de aluminio/inox de 3 mm) haciendo "sándwich" con la tapa. El Grilón además actúa como amortiguador acústico, aislando eléctrica y mecánicamente el motor del tanque.
* **Estanqueidad (Paso del eje):** Al no ser un reactor presurizado, no se necesita un sello mecánico complejo que reste torque por fricción. Usando la chumacera bridada **KFL08** por encima de la tapa, colocar por el lado interno de la tapa un **retén de labio de nitrilo/silicona (Oil Seal)** o una arandela ajustada de **PTFE (Teflón)** que actúe como escudo contra salpicaduras y condensación ascendente de vapor de agua hacia el rodamiento.

---

## ⚡ 4. Etapa de Potencia y Driver: ¿L298N, BTS7960 o MOSFET?

* **Descartar el L298N de plano:** Incluso poniendo sus canales en paralelo, el L298N utiliza transistores bipolares (BJT) vetustos que provocan una **caída de tensión interna de 2 V a 3.5 V**. Con una fuente de 12 V, al motor solo le llegarán ~9 V (perdiendo el 25% de las RPM máximas y del torque de diseño), y esa potencia perdida se disipa como calor extremo en el disipador. Además, un pico de arranque de motor grande lo quemará instantáneamente.
* **Módulo BTS7960 (IBT-2 de 43 A) — *La opción más robusta "Plug & Play"*:** Aunque es un puente H completo y solo se girará en un sentido, viene en un módulo comercial barato, con disipador macizo integrado, circuito integrado de *gate driver* que acepta **3.3 V lógicos del ESP32 de forma nativa**, protección térmica y sensor de corriente (`IS`). Además, a diferencia de un MOSFET simple, el puente H permite hacer **frenado activo (*Active Braking*)** al terminar el minuto de coagulación a 100 RPM para pasar rápidamente a 20 RPM.
* **MOSFET Canal N (IRLZ44N / AOD4184):** Si se utiliza MOSFET, evitar los módulos rojos con **IRF520** (no son nivel lógico y hierven a 3.3V/5V). Exigen además diodo rápido de rueda libre (*flyback*, mín. 10A como MBR1045) en bornes del motor.

> **Dictamen Potencia:** Utilizar un módulo **BTS7960 (43A)** (un pin PWM fijo en GND y aplicando PWM en el otro). Configurar el periférico `LEDC` del ESP32 a **frecuencia ultrasónica (16 kHz a 20 kHz)** para eliminar el silbido audible (*coil whine*) en el laboratorio durante los 20 minutos de floculación, con **rampa de arranque suave (`soft-start` de 2 a 3 segundos)** en software.

---

## 🌐 5. Arquitectura de Control: ¿1 ESP32 Centralizado vs. 2 ESP32 (ESP-NOW)?

Para una planta piloto de laboratorio donde ambos módulos están a pocos metros de distancia, **la recomendación categórica es: 1 SOLO ESP32 CENTRALIZADO en el maletín vinculado mediante manguera envainada y conector de aviación GX16.**

### Razones:
1. **Confiabilidad del canal Wi-Fi + ESP-NOW:** Al usar SoftAP y ESP-NOW a la vez, ambos comparten la radio de 2.4 GHz y el mismo canal. Saltos de canal provocan pérdida de paquetes y exigen complejas máquinas de estado de reintento.
2. **Alimentación física requerida:** De todas formas hay que tender un cable de 12V al sedimentador si no se coloca una fuente pesada adicional junto al tanque.
3. **Mantenibilidad de Firmware:** Un único código determinista: `Coagulación -> Floculación -> Sedimentación (espera boya) -> Bombeo Peristáltico DM860` sin bugs de sincronización entre placas.

### Cableado Recomendado:
* **Driver BTS7960 dentro del maletín de control**, cerca de la fuente y del ESP32 (señales lógicas de 3.3V cortas y sin interferencias).
* **Manguera umbilical (Cable tipo Taller / TPR 4 x 1.0 mm² o 4 x 1.5 mm² con ficha GX16 de 4 pines):**
  * **Pines 1 y 2 (Potencia):** Salida de 12V PWM hacia el motor del agitador.
  * **Pines 3 y 4 (Señal):** Boya de nivel del sedimentador (contacto seco cerrado a GND, con resistencia *pull-up* física de **2.2 kΩ a 3.3V** y capacitor cerámico de **100 nF a GND** en el pin del ESP32).

---

## 🌊 6. Puntos Ciegos Hidrodinámicos en 25 Litros

### A. Vórtice Central y Rotación en Bloque:
Las paredes lisas de acero inoxidable generan rotación de cuerpo sólido sin gradiente de cizalla relativo.
* **Solución sin soldar el tanque:** Montar el eje del agitador **ligeramente excéntrico ($1/6$ a $1/4$ del radio del tanque)** al perforar la tapa, rompiendo la simetría e impidiendo el vórtice sin necesidad de deflectores, o colocar una canasta/aro portabafles desmontable (4 bafles de ~3 cm).

### B. Cizallamiento de Macroflóculos:
* El mucílago de *Opuntia* produce flóculos frágiles al corte tangencial. La velocidad en punta de pala debe ser $V_{tip} \le 0.3 - 0.4\text{ m/s}$.
* **Regla de dimensionamiento:** Diámetro de paleta $d = 0.35\text{ a }0.45 \cdot D_{\text{tanque}} \approx \mathbf{11 - 14\text{ cm}}$. A 25 RPM, $V_{tip} \approx 0.16\text{ m/s}$ (totalmente seguro).

### C. Altura de la Paleta y Protección del Cono de 60°:
* La paleta debe situarse en el tercio inferior del cilindro recto, **5 a 8 cm por encima de la línea donde nace el cono de 60°**.
* El cono inferior debe conservarse como **zona de calma (*sludge blanket*)** para la compactación de lodos sin resuspensión por turbulencia.

### D. Toma de Agua Clarificada:
* La succión hacia la bomba peristáltica debe ubicarse **3 a 5 cm por encima de la transición cilindro-cono** (o mediante pescante superior/lateral), reservando el vértice inferior del cono **exclusivamente para la purga de lodos pesados**.

---

### 📋 Resumen de Decisiones Adoptadas:
1. **Motor:** Motorreductor 5840-31ZY (12V, 100 RPM, eje 8mm) o Motor de limpiaparabrisas automotriz 12V con carcasa aislada.
2. **Mecánica:** Chumacera KFL08 en tapa + Acople elástico flexible 8mm + Placa sándwich de Grilón anti-vibración + Eje excéntrico.
3. **Electrónica:** Driver BTS7960 (43A) en maletín + PWM ultrasónico a 18 kHz con rampa *soft-start* de 2 s.
4. **Control:** 1 ESP32 centralizado + Manguera umbilical con ficha GX16-4.
5. **Hidráulica:** Paleta de 12–14 cm a 5–8 cm sobre el inicio del cono; salida de agua clarificada lateral/elevada.
