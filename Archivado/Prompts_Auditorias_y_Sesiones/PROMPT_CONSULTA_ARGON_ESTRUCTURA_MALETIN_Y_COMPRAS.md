# 📋 PROMPT DE CONSULTA PARA ARGON: AUDITORÍA DE COMPRAS, SKID MODULAR 45x45 Y MECANIZADO INOX
## Proyecto: Planta Piloto de Ultrafiltración UNSa — Meta: Puesta a punto mediados de Noviembre 2026
**Destinatario:** Argon (AI Peer Reviewer)  
**Remitentes:** Ing. Enzo Corte (Codirector), Antigravity (Ingeniero Líder Mecatrónico)  
**Objetivo de la Consulta:** Obtener el veredicto técnico final sobre la lista de compras, distribución espacial del maletín cúbico (45x45x45 cm), arquitectura de alimentación eléctrica y técnicas de perforación en la cuba de acero inoxidable AISI 304 para congelar el diseño y fabricar en 15 días.

---

Hola Argon. Con tu dictamen anterior cerramos con éxito el dimensionamiento del Hito 3 (agitador 25L, motor sinfín 5840-31ZY/limpiaparabrisas, chumacera KFL08, driver BTS7960 y conector GX16-4). 

Ahora necesitamos tu **auditoría final de compras y montaje mecánico** para el **Módulo 2 (Maletín Skid Modular 45x45x45 cm)** y las **conexiones hidráulicas de la planta**, con un objetivo crítico de cronograma: **la planta debe quedar 100% armada, estanca y probada a mediados de noviembre (en ~15 a 20 días)**, para destinar diciembre exclusivamente a los ensayos experimentales de la tesis (matriz $3^2$, Darcy, Hermia, autopsia SEM y OPEX).

Revisamos y corregimos una propuesta preliminar que contenía errores (como asumir erróneamente que el sedimentador era de plástico o que la balanza electrónica HX711 debía entrar ahora). Queremos tu dictamen técnico sobre los siguientes 4 ejes:

---

### 1. Distribución Espacial y Caras del Maletín Skid (450 x 450 x 450 mm)
El núcleo de filtración y control se independiza de los tanques dentro de una estructura cúbica de 45 cm:
* **Base:** Multilaminado fenólico marino de 18 mm barnizado con 4 tacos silentblocks de goma M5 para el motor NEMA 34 y 4 patas de goma inferiores.
* **Frente:** Puerta abisagrada de acrílico/policarbonato transparente para visualización de dializador FX100, lectura visual directa de los manómetros Bourdon de glicerina (P1 y P2), ajuste de la válvula de aguja y monitoreo de la bomba peristáltica MBP-2000.
* **Propuesta de Enzo para los laterales y caras:**
  * **Caras 2 y 3 (Laterales/Trasera):** Chapa perforada de hierro/aluminio (1.2–1.5 mm) para facilitar ventilación cruzada y anclaje modular.
  * **Cara 4 (Panel ciego de conexiones / Bulkhead panel):** Placa sólida donde se montan agrupados los pasamuros de fluidos y conectores eléctricos.
  * **Distribución de flujo gravitacional:**
    * **Salida de Permeado:** En la base o lateral inferior (cae por gravedad hacia el bidón de 20-25 L ubicado en el carro rodante inferior).
    * **Entrada de Succión y Retorno de Retentado:** En la cara superior o lateral alto (comunicando con el sedimentador elevado).
* **Pregunta para Argon:** ¿Consideras óptima esta distribución de caras (2 perforadas + 1 transparente + 1 panel pasamuros)? ¿Qué precauciones recomiendas para la segregación estanca IP65 interna (caja estanca para ESP32, DM860, BTS7960 y fuentes) para garantizar que una fuga accidental a 0.7 bar no alcance la electrónica?

---

### 2. Arquitectura de Alimentación Eléctrica (AC y DC)
Aclaración técnica clave de taller: **Ya contamos con un transformador electromagnético de 220V a 24V AC.**
* El driver Leadshine DM860 acepta alimentación AC directa en sus bornes (18–80 VAC), por lo que **el transformador de 24 VAC existente alimentará directamente al DM860 / NEMA 34**, sin necesidad de comprar una fuente conmutada de 24V.
* Para el resto de la planta (circuito DC):
  * **Agitador (Hito 3):** Driver MOSFET BTS7960 (43A) que alimenta al motorreductor de 12V (requiere fuente conmutada 12V DC de 8–10A para bancar los picos de arranque).
  * **ESP32 y Lógica (Hito 2/4):** Módulo reductor Step-Down LM2596 calibrado a 5.00V (tomado de la línea de 12V) o fuente auxiliar de 5V.
* **Pregunta para Argon:** ¿Apruebas este esquema de alimentación híbrido (24 VAC para DM860 + 12V 10A DC para BTS7960/ESP32)? ¿Sugieres algún filtro adicional (ej. varistor MOV en el primario del trafo AC o choque de ferrita) para blindar al ESP32 contra transitorios inductivos cuando conmuta el NEMA 34 o el agitador?

---

### 3. Mecanizado y Estanqueidad en la Cuba de Acero Inoxidable AISI 304
El sedimentador cónico es de chapa de **acero inoxidable AISI 304 de 1.2 a 1.5 mm de espesor**. Debemos realizar dos perforaciones bajo o sobre el nivel de líquido:
1. **Toma de Succión (1/2"):** Ubicada a 5–8 cm sobre el inicio del cono (zona cilíndrica).
2. **Retorno de Retentado (1/4"):** Ubicada en la parte superior cilíndrica, con tubo interno sumergido contra la pared para evitar salpicaduras y burbujas.
* **Duda de Enzo:** ¿Cómo conviene realizar estos agujeros en la chapa de acero inoxidable en taller? ¿Broca copa bimetálica (con cobalto HSS-Co) o mecha escalonada cónica (*step drill*)? ¿Qué revoluciones y refrigeración recomiendas para no quemar la mecha ni endurecer el inox?
* **Estanqueidad:** ¿Qué tipo de pasamuros recomiendas para chapa fina de inox (pasamuros de latón/bronce con contratuerca y doble junta plana de EPDM/silicona sanitaria, o accesorio de compresión)?

---

### 4. Veredicto sobre la Lista de Compras y Cronograma a Mediados de Noviembre
Eliminamos provisionalmente la celda de carga HX711 para no sumar riesgos de calibración en estas 2 semanas (el caudal de permeado se mide por el caudalímetro YF-S401 y probeta graduada de 1000 mL, ya integrados en Firmware V6.4).
* **Lista de Compras propuesta para mostrador en Salta:**
  1. *Estructura:* Placa fenólica marina 18 mm (45x45 cm), caño estructural 20x20 mm, chapa perforada 1.2 mm, 4 silentblocks M5, 4 patas de goma, 1 manija rebatible, 2 abrazaderas acolchadas de 2" para el FX100.
  2. *Mecánica Sedimentador:* Chumacera bridada KFL08 (8 mm), acople elástico spider (8mm a 8mm), placa de Grilón 120x120 mm (e=8-10 mm), tornillería inox AISI 304.
  3. *Hidráulica:* 1 pasamuros 1/2" inox/latón + 1 pasamuros plástico 1/2" con canilla para permeado + 3 pasamuros 1/4" + 2 bujes 1/2"M a 1/4"H + 2 Tee de bronce 1/4"H (para manómetros Bourdon de glicerina P1 y P2) + 1 válvula de aguja 1/4"H + espigas de 1/2" y 1/4" + mangueras atóxicas de 1/2" y 1/4" + abrazaderas cremallera.
  4. *Electricidad:* Caja estanca IP65 para maletín, fuente conmutada 12V 10A, driver BTS7960 (43A), conector industrial circular GX16 de 4 pines (chasis + aéreo), cable taller 4x1.0 mm², prensaestopas PG7 y PG9 para la tapa del sedimentador.

* **Pregunta final para Argon:** ¿Ves algún faltante hidráulico o mecánico en esta lista para dejar la planta funcionando al 100% sin tener que volver a la ferretería? ¿Avalas la meta de ensamblar todo en 15 días con este plan?

---
Agradecemos tu revisión y visto bueno definitivo para mandar la orden de compra a los tesistas hoy mismo.
