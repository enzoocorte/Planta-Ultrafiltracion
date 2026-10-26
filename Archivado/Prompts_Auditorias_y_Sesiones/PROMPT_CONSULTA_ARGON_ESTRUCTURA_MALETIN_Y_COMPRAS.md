# 📋 PROMPT MAESTRO PARA ARGON (AUDITORÍA TÉCNICA GLOBAL, SKID 45x45 Y COMPRAS)
## Proyecto: Planta Piloto de Ultrafiltración FX100 & Reactor Sedimentador (UNSa 2026)
**Destinatario:** Argon (AI Senior Technical Peer Reviewer — Mecatrónica, Hidráulica y Procesos)  
**Remitente:** Ing. Enzo Corte (Codirector de Tesis — Investigador Doctoral en Membranas) & Antigravity (Ingeniero Líder)  
**Repositorio GitHub Oficial:** `https://github.com/enzoocorte/Planta-Ultrafiltracion` (Rama: `main`)  
**Fecha:** Octubre 2026  
**Objetivo Crítico:** Veredicto técnico final sobre la lista de compras, distribución espacial del maletín cúbico (45x45x45 cm), arquitectura eléctrica y mecanizado en acero inoxidable AISI 304 para cerrar la fabricación en 15 días (Meta: puesta a punto a mediados de noviembre).

---

Hola Argon. Te convocamos para actuar como **auditor técnico senior independiente** (especialista en ingeniería mecánica, hidráulica, procesos de separación por membranas y control mecatrónico) para la etapa de cierre constructivo de una **Planta Piloto de Tratamiento de Agua para Consumo Humano** desarrollada como proyecto de tesis de grado en la Universidad Nacional de Salta (UNSa), Argentina.

Como iniciamos esta conversación desde cero, a continuación te presentamos el **contexto integral del proyecto**, la **arquitectura física y de control definida**, el **repositorio de referencia** y los **4 ejes concretos sobre los que requerimos tu dictamen técnico**.

---

## 📌 1. Contexto Global del Proyecto y Arquitectura de la Planta

### 1.1 Objetivo del Proceso y Membrana
La planta trata agua turbia sintética ($\approx 400 - 500\text{ NTU}$) mediante un pretratamiento de coagulación-floculación con biopolímeros naturales extraídos de cladodios de *Opuntia ficus-indica* (penca de tuna) en un reactor decantador, acoplado a un módulo de **Ultrafiltración Capilar de Fibra Hueca (Fresenius CorDiax FX100 de Helixone®)** de flujo *Inside-Out* ($A_{\text{membrana}} = 1.8\text{ m}^2$, $TMP_{\max} = 0.60\text{ bar}$ operativo / $0.70\text{ bar}$ de alivio mecánico).

### 1.2 Repositorio de Código y Documentación Técnica
Todo el proyecto está versionado de forma abierta en:  
👉 **`https://github.com/enzoocorte/Planta-Ultrafiltracion`** (Rama `main`)

Documentos clave para tu referencia en el repositorio:
* `README.md`: Resumen ejecutivo y avance real sincerado hito por hito.
* `00_General_y_P_ID_Planta/01_Estructura_y_Montaje_Filtro/DOCUMENTO_MAESTRO_ESTRUCTURA_FILTRO.md`: Memoria estructural DFM/DFA de la planta en 3 módulos.
* `01_Hito1_Control_Accionamiento_NEMA34_DM860/`: Control cinematográfico de la bomba peristáltica MBP-2000 con motor NEMA 34 ($4.5\text{ Nm}$) y driver Leadshine DM860.
* `02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v6/EN_USO_firmware_planta/`: Firmware oficial activo en ESP32 (V6.4) con cálculo en tiempo real de la Ley de Darcy (`darcy.h`), factor TCF de Vogel, filtro de *blanking* (1200 µs) para caudalímetro YF-S401 y Web SCADA táctil (`192.168.4.1`).
* `03_Hito3_Reactor_Sedimentador_Agitador/`: Especificaciones del reactor cónico de 25L, protocolos de Jar Test y dimensionamiento del agitador.

---

## 📐 2. División Modular de la Planta (DFM / DFA)

La planta se divide en **tres (3) módulos físicos interconectables**:

```
 ┌──────────────────────────┐      ┌──────────────────────────┐      ┌──────────────────────────┐
 │        MÓDULO 1          │      │        MÓDULO 2          │      │        MÓDULO 3          │
 │  Reactor Sedimentador    │      │  Maletín Skid Modular    │      │  Carro Rodante y Bidón   │
 │        (25 Litros)       │═════►│   (450 x 450 x 450 mm)   │═════►│       de Permeado        │
 │  Acero Inox AISI 304     │ Succ.│  Bomba + FX100 + SCADA   │Perm. │  Estructura a 70 cm alt. │
 │  Fondo Cónico 60° + Tapa │1/2"  │  Manómetros P1/P2 Bourdon│1/4"  │  Bidón PEAD 20-25 L      │
 └──────────────────────────┘      └──────────────────────────┘      └──────────────────────────┘
       ▲                                │ Retentado 1/4"                    ▲
       └────────────────────────────────┴───────────────────────────────────┘
```

1. **Módulo 1 — Reactor Sedimentador Cónico (Hito 3):**  
   * Cuba cónica de **$25\text{ Litros útiles}$ construida en chapa de acero inoxidable AISI 304 (espesor 1.2 a 1.5 mm)** con fondo cónico de $60^\circ$ y válvula esférica inferior para purga de barros de bentonita.
   * Tapa metálica desmontable con asas.
   * Agitador: Motorreductor 12V DC (Sinfín-Corona 5840-31ZY de 100 RPM / 25 kg·cm nominal o motor de limpiaparabrisas automotriz con masa aislada).
   * Tren mecánico: Chumacera bridada autoalineante **KFL08 (8 mm)** + acoplamiento elástico spider 8 mm + placa sándwich de Grilón (120x120 mm) montada con **perforación excéntrica (desplazada 2.5 a 3.5 cm del centro)** para romper la rotación en bloque y anular el vórtice sin soldar deflectores. Paleta a 5–8 cm sobre el inicio del cono.
   * Boya de seguridad de nivel de acero inoxidable contra marcha en seco.

2. **Módulo 2 — Maletín Skid Modular de Filtración y Control (Hito 0, 1, 2 y 4):**  
   * Estructura cúbica de **$450 \times 450 \times 450\text{ mm}$** con marco de tubo cuadrado 20x20 mm y base de multilaminado fenólico marino de 18 mm.
   * Aloja la bomba peristáltica MBP-2000 (NEMA 34 con 4 silentblocks M5), cartucho dializador FX100 en posición vertical (abrazaderas acolchadas de 2"), manómetros Bourdon con baño de glicerina $P_1$ y $P_2$ (0-1 bar) en tees de 1/4", caudalímetro YF-S401 (1/4"), válvula de aguja de 1/4" para control fino de TMP y gabinete plástico estanco IP65 para la electrónica.

3. **Módulo 3 — Carro de Permeado y Soporte (Hito 0):**  
   * Estructura móvil rodante con 4 ruedas de 50 mm (2 con freno total) que aloja en su base el bidón de permeado (20-25 L) y soporta encima el maletín de filtración a una altura de trabajo ergonómica (70 a 115 cm).

---

## ⏱️ 3. El Desafío de Cronograma: Meta Innegociable a Mediados de Noviembre

* **Plazo de Montaje:** Próximos **15 a 20 días** (finales de octubre a mediados de noviembre de 2026).
* **Entregable al 15 de Noviembre:** Planta 100% fabricada, cableada, estanca (probada a 0.70 bar sin fugas) y bombeando agua en circuito cerrado.
* **Diciembre 2026:** Reservado exclusivamente para la ejecución de la **Matriz Factorial $3^2$ (13 corridas de ultrafiltración)**, cálculo de resistencias de Darcy ($R_m, R_c$), modelado cinético de ensuciamiento de Hermia ($n = 2, 1.5, 1, 0$), autopsia de fibras por Microscopía Electrónica de Barrido (SEM/EDS), balance energético SEC y defensa de la tesis.

Cualquier desvío en compras o complicación innecesaria en el taller arriesga la graduación de los tesistas. Por ello, **postergamos provisionalmente la celda de carga electrónica con HX711** para el pesaje de permeado (ya que el caudal de permeado se mide de forma precisa y validada con el caudalímetro YF-S401 y probeta graduada en el Firmware V6.4).

---

## ❓ 4. Preguntas Concretas para tu Dictamen Técnico

Requerimos tu criterio de ingeniería sobre los siguientes **4 ejes clave**:

### Eje 1: Distribución Espacial y Caras del Maletín Skid (450 x 450 x 450 mm)
Para el maletín cúbico, Enzo propone la siguiente distribución de caras y flujos:
* **Frente:** Puerta abisagrada de acrílico o policarbonato transparente para inspección visual directa del dializador FX100, lectura de los manómetros Bourdon $P_1$ y $P_2$, calibración de la válvula de aguja y supervisión de los rodillos de la bomba.
* **Caras Laterales / Trasera:** Chapa perforada de hierro o aluminio (1.2 a 1.5 mm de espesor) en 2 de sus caras para ventilación cruzada natural del transformador y drivers.
* **Cara de Conexiones (Panel pasamuros ciego):** Una chapa sólida donde se montan agrupados los pasamuros hidráulicos y conectores eléctricos.
* **Ruta de Flujos Gravitacionales:**
  * **Salida de Permeado:** En la base o lateral inferior (1/4"), cayendo por gravedad en línea recta hacia la tapa del bidón de permeado ubicado en el carro inferior.
  * **Succión (1/2") y Retorno de Retentado (1/4"):** En la cara superior o lateral alto, comunicando con el sedimentador cónico elevado.
* **Preguntas para Argon:**
  1. ¿Consideras óptima esta distribución de caras (2 perforadas + 1 transparente + 1 panel pasamuros)?
  2. ¿Qué recomendaciones nos das para la **segregación estanca IP65 interna** (gabinete plástico para ESP32, DM860, BTS7960 y fuentes)? En caso de una fuga accidental a 0.7 bar en la zona de mangueras, ¿cómo garantizamos cero riesgo eléctrico?

---

### Eje 2: Arquitectura de Alimentación Eléctrica (AC y DC)
Disponemos físicamente en laboratorio de un **transformador electromagnético de 220V a 24V AC**.
* El driver Leadshine DM860 acepta alimentación de corriente alterna directa en sus bornes `AC / AC` (rango 18–80 VAC), por lo que **el transformador de 24 VAC existente alimentará directamente al DM860 y al motor NEMA 34**, ahorrando la compra de una fuente conmutada de 24V.
* Para el resto de los componentes (circuito DC):
  * **Driver Agitador (BTS7960 43A):** Se alimentará con una fuente conmutada de **12V DC (8 a 10A)** para soportar la corriente de régimen ($1.5 - 3\text{ A}$) y los picos de arranque del motorreductor.
  * **ESP32 y Lógica de Sensores:** Módulo reductor Step-Down LM2596 calibrado a $5.00\text{V DC}$ (conectado a la línea de 12V) ingresando por el pin `VIN` del ESP32.
  * **Control del Agitador:** El driver BTS7960 va alojado dentro del maletín y se conecta al sedimentador mediante un cable taller de 4 hilos con conector circular industrial **GX16-4** (2 hilos motor PWM + 2 hilos boya de nivel con pull-up de 2.2 kΩ y capacitor de 100 nF en el ESP32).
* **Preguntas para Argon:**
  1. ¿Avalas este esquema de alimentación híbrido (24 VAC para DM860 + 12V 10A DC para BTS7960/ESP32)?
  2. ¿Qué precauciones de filtrado de ruido o supresión de transitorios inductivos (varistor MOV, diodos flyback o choques de ferrita) recomiendas para evitar que el ESP32 sufra reinicios por EMI al arrancar el NEMA 34 o el agitador?

---

### Eje 3: Mecanizado y Estanqueidad en la Cuba de Acero Inoxidable AISI 304
El tanque sedimentador cónico es de **chapa de acero inoxidable AISI 304 de 1.2 a 1.5 mm de espesor**.
Debemos realizar dos perforaciones pasamuros en la chapa:
1. **Toma de Succión hacia la bomba (1/2"):** Ubicada en la pared cilíndrica recta, **5 a 8 cm por encima de la línea de soldadura donde nace el cono de 60°** (para tomar agua clarificada sin tocar el manto de lodos).
2. **Retorno de Retentado (1/4"):** Ubicada en la parte superior de la chapa cilíndrica, con un tubo interno de teflón o manguera adherida a la pared para que el fluido caiga deslizándose sin salpicar ni generar espuma.
*(Nota: Las sondas DS18B20 y TDS ingresarán por la tapa superior mediante prensaestopas PG7 y PG9, evitando perforar la chapa bajo el nivel de agua).*
* **Preguntas para Argon:**
  1. ¿Qué herramienta de mecanizado recomiendas para hacer estos agujeros limpios en chapa fina de inoxidable AISI 304 en taller: **mecha escalonada cónica de cobalto (*step drill HSS-Co*)** o **broca copa bimetálica con dientes de cobalto**? ¿Qué parámetros de velocidad (RPM) y lubricante de corte debemos usar para no destemplar la herramienta ni endurecer el inox?
  2. Para lograr estanqueidad absoluta a largo plazo en chapa de 1.2–1.5 mm: ¿qué tipo de pasamuros recomiendas (pasamuros roscado de latón o acero inoxidable con tuerca exterior y arandelas planas con doble junta de EPDM o silicona sanitaria)? ¿Recomiendas algún sellador anaeróbico o pasta teflón en las roscas?

---

### Eje 4: Veredicto sobre la Lista de Compras Consolidada (Mostrador en Salta)
Para emitir la orden de compra inmediata a los proveedores locales de Salta Capital (Bulonera Salta, Rodamientos Salta, sanitarios e insumos industriales) y plataformas online:

**1. Estructura y Maletín:**
* 1x Placa multilaminado fenólico marino 18 mm (cortada a 450x450 mm).
* 4 metros de tubo estructural cuadrado 20x20 mm (chapa 1.2 mm).
* Chapa perforada de hierro o aluminio 1.2–1.5 mm (para 2 caras de 45x45 cm).
* 1x Placa de acrílico o policarbonato transparente de 3–4 mm (para puerta frontal de 45x45 cm) con 2 bisagras y traba.
* 4x Tacos antivibratorios silentblock de goma M5 (macho-hembra) para el NEMA 34.
* 4x Regatones / patas de goma de 25–30 mm de base.
* 1x Manija metálica rebatible de maletín industrial.
* 2x Abrazaderas acolchadas con goma tipo clamps de 2" (para el dializador FX100).

**2. Mecánica del Sedimentador:**
* 1x Chumacera con brida KFL08 (agujero Ø 8 mm) en acero/fundición de zinc con prisioneros.
* 1x Acoplamiento elástico flexible tipo spider (8 mm a 8 mm, o al eje del motor).
* 1x Placa de Grilón/Nylon de 120 x 120 mm (espesor 8–10 mm).
* 4x Separadores / espárragos M6 x 50 mm para torreta del motor.
* Tornillería en acero inoxidable AISI 304 (M5 y M6 con tuercas autofrenantes).

**3. Hidráulica y Cañerías (1/2" y 1/4"):**
* 1x Pasamuros roscado de 1/2" (latón o acero inox) con contratuerca y juntas de EPDM/silicona (para succión sedimentador).
* 1x Pasamuros plástico de 1/2" con canilla plástica o metálica de 1/2" (para descarga de bidón de permeado).
* 3x Pasamuros de 1/4" (1 para retorno en sedimentador, 2 para pasamuros del panel del maletín).
* 2x Bujes de reducción de bronce de 1/2" Macho a 1/4" Hembra.
* 2x Tee de bronce de 1/4" Hembra (para intercalar manómetros P1 y P2 en línea).
* 1x Válvula de aguja de 1/4" Hembra-Hembra (para control de contrapresión / TMP).
* 4x Espigas para manguera rosca 1/4" Macho (para manguera de 6 mm int.).
* 2x Espigas para manguera rosca 1/2" Macho (para manguera de 12 mm int.).
* 3 metros de manguera atóxica de silicona o PVC cristal de 1/4" (Ø int. 6 mm).
* 2 metros de manguera atóxica para bomba peristáltica de 1/2" (Ø int. 12 mm).
* 10x Abrazaderas cremallera metálicas miniatura (para 1/4" y 1/2").
* 2x Rollos de cinta teflón de alta densidad.

**4. Electricidad y Gabinete Estanco:**
* 1x Gabinete estanco plástico IP65 (~200 x 150 x 100 mm) para alojar ESP32, DM860, BTS7960 y LM2596 dentro del maletín.
* 1x Fuente conmutada 12V DC (8 a 10A).
* 1x Driver MOSFET BTS7960 (43A).
* 1x Juego conector circular industrial GX16 de 4 pines (chasis hembra + aéreo macho).
* 3 metros de cable tipo taller envainado flexible 4 x 1.0 mm².
* 1x Entrada interlock 220V de chasis con portafusible y tecla luminosa bipolar.
* 2x Prensaestopas plásticos PG7 y PG9 (para sonda DS18B20 y electrodo TDS en la tapa).

* **Pregunta final para Argon:** ¿Detectas algún punto ciego, pieza faltante o incompatibilidad de roscas/medidas en esta lista de compras para armar la planta en los próximos 15 días? ¿Avalas la viabilidad del montaje para alcanzar la meta de mediados de noviembre?

---
Agradecemos tu revisión crítica y tus recomendaciones prácticas para iniciar las compras y el mecanizado sin demoras.
