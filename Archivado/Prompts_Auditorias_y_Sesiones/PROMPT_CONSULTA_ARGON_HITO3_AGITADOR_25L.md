# 🌪️ CONSULTA TÉCNICA A ARGON: SELECCIÓN Y MONTAJE DEL AGITADOR EN REACTOR SEDIMENTADOR CÓNICO DE 25L

> **Instrucciones para Enzo:**  
> Copia todo el bloque a continuación y pégalo directamente en el chat con **Argon**. Al tener acceso al repositorio de GitHub (`https://github.com/enzoocorte/Planta-Ultrafiltracion`), Argon podrá inspeccionar directamente la memoria de cálculo, las fotos reales de taller y el estado del Hito 3 para darte una respuesta técnica, crítica y constructiva como revisor externo experto.

---

```markdown
# 🏛️ CONSULTA DE INGENIERÍA Y REVISIÓN EXTERNA: AGITADOR PARA REACTOR CÓNICO DE 25 LITROS (HITO 3)

Hola Argon. Te consultamos como **Asesor y Revisor Técnico Externo** de nuestro proyecto de Tesis de Grado en Ingeniería Industrial / Química (Universidad Nacional de Salta, Argentina).

Te pedimos que inspecciones directamente nuestro repositorio oficial en GitHub:  
👉 **https://github.com/enzoocorte/Planta-Ultrafiltracion** (Rama `main`)

Como tienes acceso al repositorio y memoria persistente, no repetiremos todo el contexto histórico, pero te resumimos el problema técnico puntual que necesitamos resolver contigo hoy:

---

## 📌 1. El Cuello de Botella Actual (Hito 3: Reactor Sedimentador)

En el **Hito 3** (`03_Hito3_Reactor_Sedimentador_Agitador/`), tenemos construido y disponible en taller el **tanque reactor sedimentador de 25 Litros útiles** en chapa de acero inoxidable con fondo cónico de $60^\circ$ y tapa metálica desmontable con orificio pasante (puedes ver las fotos reales de la cuba, la base trípode y la tapa en la carpeta `03_Hito3_Reactor_Sedimentador_Agitador/04_Galeria_y_Montaje_Mecanico/` y en `guia_conexion_sedimentador.html`).

**El problema físico:**
Actualmente el agitador no está incorporado porque el motorreductor de 12V que teníamos en banco **carece del torque y potencia necesarios**. Al probarlo con la paleta pesada de acero inoxidable sumergida en fluido, el motor se clava y no puede vencer la inercia de arranque ni la resistencia de los **25 kg de agua con turbidez (bentonita/arcilla ~500 NTU y biopolímero de *Opuntia ficus-indica*)**.

---

## 🎯 2. Requerimientos Operativos del Proceso (Jar Test en 25 Litros)

1. **Volumen Útil:** $25\text{ Litros} = 0.025\text{ m}^3$ ($\approx 25\text{ kg}$ de masa líquida).
2. **Ciclo Automatizado de Jar Test:**
   * **Mezcla Rápida (Coagulación):** Velocidad máxima de **$100\text{ RPM}$** durante $60\text{ segundos}$ para dispersión del coagulante sin generar vórtice violento que desborde el tanque.
   * **Mezcla Lenta (Floculación):** Velocidad de **$30\text{ RPM}$** ($G \approx 25 - 45\text{ s}^{-1}$) durante $15\text{ minutos}$ para favorecer el crecimiento de flóculos sin rotura por cizallamiento (*shear stress*).
   * **Sedimentación Estática:** **$0\text{ RPM}$** (apagado) durante $30\text{ minutos}$.
3. **Electrónica y Control Disponible:**
   * Microcontrolador **ESP32 DevKit V1** (control de velocidad por PWM en `GPIO 4`, sentido de giro en `GPIO 16/17`).
   * Driver disponible en banco: **Puente H L298N** (alimentado con fuente de $12\text{V DC}$).
   * Seguridad: Boya magnética de nivel de acero inoxidable en `GPIO 32` para corte por marcha en seco.

---

## ❓ 3. Preguntas Concretas para tu Dictamen Técnico

Queremos tu criterio técnico independiente sobre cuatro ejes clave:

### A. Selección del Motorreductor: ¿Qué tipo y modelo nos recomiendas?
Antigravity formuló una memoria preliminar en `03_Hito3_Reactor_Sedimentador_Agitador/02_Firmware_Control_Agitador/Guia_Dimensionamiento_Motorreductor_y_Torque_Agitador.md` estimando un torque nominal de $15\text{ a }25\text{ kg}\cdot\text{cm}$ ($1.5 - 2.5\text{ Nm}$) y potencia eléctrica de $25 - 40\text{ W}$.
* Entre las siguientes alternativas, ¿cuál consideras la más viable, robusta y económica para nuestro caso?
  1. **Motorreductor comercial de 12V con reducción Sinfín-Corona (Worm Gear tipo 5840-31ZY o GW600 a 100 RPM).**
  2. **Motorreductor cilíndrico comercial de 12V con caja metálica (tipo JGB37-550 o 555 a 100 RPM).**
  3. **Motor reciclado de descarte automotriz o de taller (limpiaparabrisas de 12V, o motorreductor planetario de taladro atornillador inalámbrico de 12V con batería agotada).**
  4. ¿O considerarías un pequeño motor AC de 220V con caja reductora?

### B. Montaje Mecánico sobre la Tapa Metálica: ¿Cómo diseñarlo correctamente?
Nuestra idea base es **montar el motor directamente sobre la tapa desmontable de acero inoxidable**.
* ¿Qué opinas de esta configuración?
* Al levantar la tapa para limpiar el tanque tras una decantación de lodos, se levantaría también el conjunto motor + eje + paleta. ¿Es ergonómico y conveniente, o recomiendas un puente/soporte estructural independiente anclado a la pestaña del tanque?
* ¿Cómo resolverías el guiado del eje largo (~35-40 cm) en la tapa para evitar vibración, cabeceo excéntrico o desgaste prematuro por rozamiento metal-metal (buje de bronce fosforoso, teflón/Delrin, rodamiento con retén de goma)?
* ¿Qué tipo de acople motor-eje nos recomiendas (acople elástico tipo mandíbula / Oldham / fuelle)?

### C. Driver y Electrónica de Potencia: ¿L298N en paralelo o dar el salto a otro driver?
* Para mover este motorreductor en 25 L (que consumirá entre 1.5A y 3.5A en los picos de arranque), ¿alcanza con conectar los dos canales del L298N en paralelo para duplicar su capacidad a 3.5A?
* ¿O nos sugieres sustituir el L298N por un driver MOSFET más moderno y eficiente como el **BTS7960 (43A)** o un módulo MOSFET unidireccional simple? (Teniendo en cuenta que en un agitador no es imprescindible invertir el giro en marcha normal).

### D. Puntos Ciegos y Recomendaciones Críticas
* ¿Qué consideraciones hidrodinámicas o mecánicas crees que se nos pueden estar escapando al escalar de 5 a 25 litros (deflectores/baffles anti-vórtice, longitud del eje, efecto de la turbulencia sobre el manto de lodos del fondo cónico)?

---

Por favor, revisa el repositorio y danos tu opinión estructurada y directa para que el equipo (Enzo, Antonella, Owen) y Antigravity podamos tomar la decisión final y pasar a la fase de mecanizado y compra.
```
