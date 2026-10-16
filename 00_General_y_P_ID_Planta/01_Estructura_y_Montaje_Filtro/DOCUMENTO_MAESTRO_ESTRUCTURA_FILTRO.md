# DOCUMENTO MAESTRO DE INGENIERÍA: DISEÑO, MODULARIDAD Y FABRICACIÓN DE PLANTA PILOTO DE ULTRAFILTRACIÓN

**Proyecto de Tesis de Grado — Ingeniería Industrial**  
**Objeto:** Estructura Mecánica, Modular y de Proceso para Sistema de Sedimentación y Ultrafiltración  
**Autor:** Tesista de Ingeniería Industrial  
**Revisión:** 1.0 — Documento de Diseño Preliminar y Validación Mecánica  
**Fecha:** Octubre 2026  

---

## 1. RESUMEN EJECUTIVO Y OBJETIVO DEL PROYECTO

El presente documento constituye el diseño conceptual, estructural y metodológico para la fabricación de una **Planta Piloto Modular de Tratamiento de Agua por Ultrafiltración (UF)** acoplada a un pretratamiento de sedimentación y coagulación/floculación.

El sistema se concibe bajo el paradigma de **Diseño para Fabricación y Ensamble (DFM/DFA)**, dividiendo la planta en **tres (3) módulos autónomos e interconectables**:
1. **Módulo 1 — Sedimentador / Coagulador:** Decantador cónico de acero inoxidable con agitación controlada y purga inferior.
2. **Módulo 2 — Caja de Filtración y Control:** Gabinete cúbico portátil tipo maletín (450 × 450 × 450 mm) con bomba peristáltica, prefiltración, membrana UF, sensores e instrumentación ESP32.
3. **Módulo 3 — Torre/Carro de Permeado y Soporte:** Estructura móvil rodante (700 mm de altura) con base para tanque acumulador de permeado (bidón PEAD ~20-25 L) y brazos superiores para encastre y fijación rápida de la caja de filtración.

El objetivo central para la tesis de Ingeniería Industrial es demostrar **viabilidad técnico-económica**, **ergonomía industrial**, **seguridad de operación** (segregación eléctrica/hidráulica) y **modularidad logística** (facilidad de transporte y mantenimiento).

---

## 2. RELEVAMIENTO DE DATOS EXISTENTES Y DIAGNÓSTICO CRÍTICO DE INGENIERÍA

A partir del análisis exhaustivo del documento preliminar y del material fotográfico relevado en laboratorio, se identificaron aspectos críticos que requieren corrección ingenieril antes de pasar a la etapa de manufactura:

### 2.1 Módulo Sedimentador (Tanque Inoxidable + Base de Apoyo)
* **Datos medidos:**
  * Altura total del cuerpo del tanque: **43 cm** (430 mm).
  * Válvula de alimentación/toma lateral: **5 cm** por encima del inicio del cuerpo cilíndrico.
  * Altura de patas de la base existente: **34 cm, 30 cm, 29/30 cm, 33 cm** (irregularidad de hasta 5 cm entre apoyos).
  * Vástago agitador existente: Longitud = **32 cm** (320 mm), Diámetro = **4 mm** (Radio 2 mm) con extremo roscado macho.
  * Tapa: Cuenta con una manija plana soldada al ras del centro y un orificio excéntrico.
* **Diagnóstico Crítico de Ingeniería:**
  1. **Incompatibilidad del motor de lectora CD/DVD:** El intento de utilizar un motor miniatura de bandeja/disco de CD (eje Ø2 mm) es un **error técnico grave**. Dichos micromotores entregan un torque de apenas $0.5 \text{ a } 2 \text{ mN}\cdot\text{m}$ a miles de RPM sin carga, diseñados para mover una lente o girar un disco plástico en el aire. El agua tiene una densidad de $1000 \text{ kg/m}^3$ y una viscosidad que frenará instantáneamente el motor por arrastre hidrodinámico sobre el rotor de 5 álabes, provocando sobrecalentamiento y quema inmediata.
  2. **Fenómeno de agitación en sedimentación/coagulación:** Para procesos fisicoquímicos se requiere:
     * *Coagulación / Mezcla rápida:* 100 – 150 RPM durante 30 a 60 segundos.
     * *Floculación / Mezcla lenta:* 20 – 40 RPM para promover el choque y agregación de flóculos sin romperlos por cizallamiento (*shear stress*).
     * Una hélice tipo ventilador girando a alta velocidad rompería los flóculos e impediría la decantación.
  3. **Esbeltez y vibración del eje Ø4 mm:** Un vástago de 320 mm de longitud y solo 4 mm de diámetro tiene una relación de esbeltez crítica ($L/D = 80$). Al rotar sumergido sin punto de apoyo intermedio, entrará en resonancia y pandeo dinámico (*whirling*), destruyendo cualquier acople rígido y agrandando la perforación de la tapa.
  4. **Nivelación del trípode/soporte:** Las diferencias de altura medidas en las patas (29 a 34 cm) provocan una inclinación que altera la decantación cónica (el lodo no se acumulará simétricamente en el vértice de purga). Se debe instalar un sistema de nivelación regulable.

### 2.2 Módulo de Filtración (Caja 45 × 45 × 45 cm)
* **Datos proyectados:**
  * Dimensiones: **450 mm (ancho) × 450 mm (profundidad) × 450 mm (altura)**.
  * Distribución: 3 laterales de chapa perforada, frente con puerta abisagrada, base y techo de chapa sólida con manija superior ("tipo maletín").
* **Diagnóstico Crítico de Ingeniería:**
  1. **Segregación de Riesgo Eléctrico e Hidráulico (Norma IEC 60529 / OSHA):** En el lateral izquierdo se proyecta la bomba peristáltica junto a la electrónica (driver, ESP32, conexiones), y en el lateral derecho el transformador. En una planta de agua a presión, cualquier microfuga o pulverización puede alcanzar circuitos de 220V/12V.  
     * *Solución mandatoria:* Alojar el ESP32, drivers y fuente en una **caja estanca plástica estandarizada (IP65)** montada sobre el lateral, garantizando barrera física estricta entre fluidos y electricidad.
  2. **Rigidez y Peso:** El uso de tubo estructural 20×20 mm con chapa perforada y maciza de acero al carbono es óptimo en resistencia, pero la caja completamente equipada pesará entre 12 y 16 kg. La manija de transporte superior debe tener refuerzo estructural interno bajo la chapa de techo para evitar deformaciones por flexión al izarla.

### 2.3 Módulo Tanque de Permeado y Soporte (70 × 40 cm)
* **Datos proyectados:**
  * Carro rodante de 700 mm de altura libre y 400 mm de ancho.
  * Dos brazos superiores para recibir la caja de filtro de 45 cm.
  * Chapa trasera sólida y base con ruedas.
* **Diagnóstico Crítico de Ingeniería:**
  1. **Análisis de Centro de Gravedad y Vuelco:** Al colocar la caja de 45×45 cm (~15 kg) a 70 cm del suelo, el centro de gravedad del conjunto queda elevado (~90 cm). Si el bidón inferior de 20-25 L está vacío, el carro podría ser vulnerable a vuelco ante un empuje accidental.
     * *Solución:* La distancia entre ejes de las ruedas debe ser de al menos 420 × 420 mm, y se deben emplear 4 ruedas de 50 mm (2 de ellas con freno total de doble acción: rueda + giro de horquilla).
  2. **Ergonomía Operativa:** Con el carro a 70 cm y la caja de 45 cm encima, la altura de trabajo de la puerta frontal y los controles se ubica entre **70 cm y 115 cm**. Esta altura es **antropométricamente perfecta** para un operario de pie (percentil 5 a 95 según normas IRAM/ISO de ergonomía), evitando flexiones lumbares.

---

## 3. ESPECIFICACIONES TÉCNICAS Y MODIFICACIONES POR MÓDULO

### 3.1 MÓDULO 1: SEDIMENTADOR / COAGULADOR

```
        [ MOTORREDUCTOR 12V DC (30-100 RPM) ]
                       |
            [ Acople Flexible 4mm-6mm ]
                       |
       ===== [ BUJE GUÍA / RETÉN EN TAPA ] =====
      |                                         |
      |   === TAPA INOXIDABLE CON ASAS LAT. === |
      |                                         |
      |~~~~~ Nivel de Líquido ~~~~~~~~~~~~~~~~~~|  <--- Entrada Retrolavado / Alim.
      |                                         |
      |          Eje Inox Ø4-6 mm               |
      |                                         |
      |                                         |  <--- Salida Clarificado a Bomba
      |                                         |       (h = 5 cm sobre cono)
      |          [ PALETA AGITADORA ]           |
      \                                         /
       \                CONO                   /
        \                                     /
         \====== [ VÁLVULA DE PURGA ] =======/
```

#### Modificaciones Mecánicas a Realizar:
1. **Reemplazo del Motor por Motorreductor:**
   * Descartar el motor de CD. Utilizar un **motorreductor DC 12V con caja reductora metálica (tipo JGA25-370 o TT metálico)** con velocidad nominal de **30 a 60 RPM** y torque mínimo de **$1.5 \text{ a } 3.0 \text{ kg}\cdot\text{cm}$**.
   * Costo muy bajo, bajo consumo (< 300 mA), controlable por PWM desde el ESP32 mediante puente H (L298N o MOSFET simple).
2. **Sistema de Guía en Tapa (Cojinete central):**
   * Perforar el centro de la tapa a Ø10 mm.
   * Montar un **pasamuros o portabuje de Delrin/Teflón o buje de bronce fosforoso autolubricado** para estabilizar el vástago de 32 cm. Esto elimina cualquier vibración o cabeceo excéntrico.
3. **Acople Motor-Vástago:**
   * Utilizar un **acople elástico de aluminio tipo mandíbula (Jaw coupling) o acople helicoidal ranurado** de Ø interior de un lado igual al eje del motor (típicamente 4 mm) y del otro lado mecanizado a 4 mm.
   * *Resolución del extremo roscado:* **No es necesario cortar la rosca**. Se puede colocar una tuerca autoblocante M4 como tope axial y fijar el acople elástico con los prisioneros Allen sobre la sección lisa o bien roscar una cupla adaptadora de bronce hembra M4 a eje cilíndrico liso.
4. **Modificación de la Tapa y Manijas:**
   * Desoldar la manija plana central actual.
   * Instalar una **torreta o puente soporte elevado en chapa doblada (perfil U invertido)** fijado a la tapa con tornillos M4 para sostener firmemente el motorreductor alineado con el centro.
   * Soldar **dos manijas tipo puente en los laterales exteriores de la tapa**, permitiendo destapar el tanque de forma rápida y segura sin traccionar del motor ni del eje.
5. **Nivelación de la Base:**
   * En los extremos de las 4 patas metálicas (34, 30, 29, 33 cm), cortar las patas más largas o soldar una planchuela base con una tuerca soldada roscada de **3/8” o M10**.
   * Instalar **4 regatones de nivelación basculantes con base de goma antideslizante**. Esto permite calibrar la horizontalidad perfecta en cualquier piso del laboratorio y amortiguar vibraciones.
6. **Tomas Hidráulicas del Sedimentador:**
   * *Entrada superior (Tapa):* Conexión rápida para manguera de recirculación/retrolavado.
   * *Salida lateral (h = 5 cm desde la unión cilíndro-cono):* Niple roscado de 1/2" soldado o pasamuros para manguera de succión hacia la bomba peristáltica (garantiza tomar agua clarificada sin arrastrar el lodo decantado).
   * *Purga inferior (vértice del cono):* Válvula esférica de 1/2" existente para evacuación y limpieza periódica de sedimentos.

---

### 3.2 MÓDULO 2: CAJA DE FILTRACIÓN Y CONTROL ("MALETÍN")

```
                      450 mm
        +-----------------------------------+  ^
        | [MANIJA REBATIBLE REFORZADA]      |  |
        | [TECHO: CHAPA MACIZA N°18]        |  |
        |===================================|  |
        | LATERAL IZQ.      | FONDO:        |  |
        | (Ventilación)     | - Prefiltro   |  |
        |                   | - Membrana UF |  | 450 mm
        | [Caja Estanca     | - Manómetros  |  |
        |  ESP32/Drivers]   | - Sensores P  |  |
        |                   |               |  |
        | [Bomba            | LATERAL DER:  |  |
        |  Peristáltica]    | - Fuente 12V  |  |
        |                   | - Térmica 220V|  |
        |-------------------+---------------|  |
        | FRENTE: Puerta con visor acrílico |  |
        | BASE: Chapa sólida con encastres  |  v
        +-----------------------------------+
        <-------------- 450 mm ------------->
```

#### Especificaciones Constructivas:
* **Estructura Portante:** Bastidor cúbico de 450 × 450 × 450 mm en **tubo estructural cuadrado de 20 × 20 mm × 1.2 mm** de espesor.
* **Caras Laterales y Ventilación:**
  * **Lateral Izquierdo (Mando & Bombeo):**
    * Chapa perforada de acero SAE 1010 N°18 (agujeros Ø3 a 5 mm paso alternado).
    * Bomba peristáltica montada en panel inferior con salida de cabezal hacia el frente o fondo.
    * Caja estanca plástica IP65 (160 × 120 × 75 mm) que encierra el microcontrolador ESP32, driver de la bomba (A4988 / TB6600 o controlador PWM), relés y bornes de conexión.
  * **Lateral Posterior (Fondo Hidráulico):**
    * Chapa perforada para soporte mediante abrazaderas tipo *clamp* plásticas o metálicas.
    * Montaje del **Prefiltro de sedimentos** (vaso de 10" o 5" con cartucho de polipropileno expandido de 5 µm).
    * Montaje del **Módulo de Membrana de Ultrafiltración (UF)** de fibra hueca tubular o encapsulada.
    * Transductores de presión digital (0-5 V o I2C) y/o manómetros analógicos de glicerina para monitorear la presión transmembrana (TMP).
  * **Lateral Derecho (Alimentación Eléctrica):**
    * Chapa perforada para evacuación térmica de la fuente.
    * Fuente conmutada 220V AC a 12V DC (10A o 15A) montada en carril DIN o soporte aislado.
    * Conector de entrada C14 (interlock) con portafusible integrado e interruptor bipolar luminoso de encendido general.
  * **Cara Frontal (Acceso y Seguridad):**
    * Puerta abisagrada con marco de perfil ángulo 15×15 mm o tubo 15×15 mm.
    * Cierre mediante falleba o traba a presión con imán de neodimio.
    * Panel frontal de acrílico o policarbonato transparente de 3 mm para inspección visual directa del flujo, bomba y filtros sin abrir el gabinete.
  * **Base y Techo:**
    * **Base:** Chapa sólida SAE 1010 N°16 (1.6 mm) plegada con pestañas perimetrales soldadas al bastidor para máxima rigidez y contención de goteos accidentales.
    * **Techo:** Chapa sólida SAE 1010 N°18 (1.2 mm) con **manija central de acero rebatible para baúl** reforzada internamente con una planchuela de distribución de carga de 25 × 3 mm soldada a los travesaños del techo.

---

### 3.3 MÓDULO 3: CARRO / TORRE DE PERMEADO Y SOPORTE

```
              400 mm
        +-------------------+  ^
        | [BRAZO IZQ] [DER] |  |  Brazos de encastre con pernos
        |=====|=======|=====|  |  de fijación rápida (M8 mariposa)
        |     |       |     |  |
        |     |       |     |  |
        |     |       |     |  |
        |  [LATERAL TRASERO |  |  700 mm (Altura libre del carro)
        |   CHAPA SÓLIDA]   |  |
        |                   |  |
        |   [ ALOJAMIENTO   |  |
        |     DEL BIDÓN     |  |
        |      PEAD 20L     |  |
        |     PERMEADO ]    |  |
        |                   |  |
        |===================|  |
        | BASE CHAPA SÓLIDA |  v
        +--[O]---------[O]--+
          Ruedas 50 mm (2 giratorias + 2 con freno)
```

#### Especificaciones Constructivas:
* **Estructura Vertical:** Tubo estructural cuadrado de **20 × 20 × 1.2 mm** (o 25 × 25 mm para mayor rigidez torsional).
* **Dimensiones Generales:**
  * Altura de columnas: **700 mm**.
  * Ancho frontal: **400 mm**.
  * Profundidad de base: **450 mm** (para calzar exactamente bajo la huella de la caja del filtro).
* **Placa Posterior:** Chapa sólida lisa SAE 1010 N°18 (1.2 mm) atornillada o soldada en el fondo. Cumple dos funciones clave:
  1. Actúa como **diafragma estructural rígido** (arriostramiento contra esfuerzos de corte y torsión lateral).
  2. Protege al bidón de permeado contra impactos o suciedad exterior.
* **Bandeja Inferior (Base rodante):**
  * Chapa sólida SAE 1010 N°16 (1.6 mm) plegada en forma de batea con pestaña perimetral hacia arriba (15 mm) para asegurar que el bidón de permeado quede contenido y no deslice durante el transporte.
  * 4 ruedas industriales de Ø50 mm con banda de rodadura de poliuretano/goma (2 fijas traseras y 2 giratorias delanteras con freno de doble acción).
* **Brazos de Encastre y Desmontaje Rápido ("Quick-Release"):**
  * En la parte superior de las columnas (cota 700 mm), se sueldan dos brazos en voladizo hacia el frente en tubo 20×20 mm o perfil ángulo de 1” × 1/8”.
  * La base de la Caja de Filtración (Módulo 2) posee 4 orificios pasantes reforzados que coinciden con 4 pernos guía fijados a los brazos del carro.
  * La fijación se realiza mediante **perillas de ajuste manual o tuercas mariposa M8**. Esto permite separar la caja en menos de 30 segundos sin necesidad de llaves ni herramientas, facilitando su transporte como maletín independiente.

---

## 4. SELECCIÓN Y JUSTIFICACIÓN DE MATERIALES (CRITERIO: RESISTENCIA Y ECONOMÍA)

Para una tesis de Ingeniería Industrial, la selección de materiales debe fundamentarse en una **matriz multicriterio de decisión**, evaluando costo por kilogramo, disponibilidad comercial en plaza, facilidad de mecanizado/soldadura en talleres locales y resistencia a ambientes húmedos:

| Alternativa de Material | Costo Relativo | Resistencia a Corrosión | Facilidad de Manufactura | Veredicto de Ingeniería |
| :--- | :---: | :---: | :---: | :--- |
| **Acero Inoxidable AISI 304** (Completo) | Muy Alto (4.5×) | Excelente (Inerte) | Requiere TIG/MIG argón, consumibles caros | **Descartado para bastidores** (costo excesivo). Solo reservado para partes en contacto directo con agua (tanque sedimentador existente). |
| **Perfilería de Aluminio Ranurado (Tipo Bosch/V-Slot 2020)** | Alto (2.8×) | Excelente | Excelente (sin soldadura, ensamble por tuercas T) | **Excelente para prototipado rápido**, pero los accesorios y escuadras encarecen un 150% el costo final frente al acero. |
| **Acero al Carbono Estructural SAE 1010/1020 + Pintura Epoxi / Electrostática** | **Bajo (1.0×)** | **Muy Buena** (con tratamiento superficial) | **Excelente** (soldadura MIG/MAG convencional, corte sensitiva) | **SELECCIÓN GANADORA**. Es el estándar de la industria metalmecánica por su inmejorable relación costo/resistencia estructural. |

### 4.1 Especificación de la Protección Superficial:
Para evitar la corrosión típica del acero al carbono en contacto con salpicaduras de agua de proceso:
1. Desengrase y fosfatizado químico de los perfiles y chapas.
2. Aplicación de fondo anticorrosivo epoxi al cromo/zinc.
3. Acabado con **pintura en polvo electrostática termoconvertible (poliéster o epoxi-poliéster horneada)**, color Gris Texturado RAL 7035 o Azul Industrial. Brinda una película de 80 a 100 micrones altamente resistente a impactos y agentes químicos.

### 4.2 Selección de Chapas:
* **Laterales y Fondo de Caja (Módulo 2):** Chapa perforada SAE 1010 N°18 (espesor 1.25 mm), perforación redonda de 4 mm al tresbolillo (40% de área abierta). Otorga excelente rigidez, disipación de calor del transformador y bomba, y visibilidad interior.
* **Bases de Apoyo (Caja y Carro):** Chapa lisa SAE 1010 N°16 (espesor 1.6 mm) para soportar las cargas puntuales de los componentes y el bidón lleno de 25 kg.

### 4.3 Componentes Plásticos y Grado Sanitario:
* **Mangueras de Proceso:** Manguera flexible de silicona de grado alimenticio/sanitario (especialmente para el tramo de compresión en la bomba peristáltica) y mangueras de polietileno/poliuretano de 8 mm OD con conexiones rápidas neumáticas/hidráulicas tipo *push-in* de poliacetal o acero inoxidable.
* **Tanque de Permeado:** Bidón existente de **Polietileno de Alta Densidad (PEAD / HDPE)** de 20-25 L. Material inerte, grado alimenticio, resistente químicamente y de costo nulo al ser un insumo ya disponible.

---

## 5. DIAGRAMA DE FLUJO DEL PROCESO E INSTRUMENTACIÓN (P&ID CONCEPTUAL)

El ciclo operativo de la planta piloto se rige por las siguientes etapas:

```
                  [ COAGULANTE / AGUA CRUDA ]
                              |
                              v
                +----------------------------+
                |    MODULO 1: SEDIMENTADOR   | <--- Agitador (30-60 RPM)
                |  - Decantación de flóculos |
                +----------------------------+
                   | (Salida clarificado)   \ (Purga lodos)
                   v                         v [Válvula Esférica]
        +--------------------+
        | BOMBA PERISTÁLTICA | (Caudal regulable vía ESP32)
        +--------------------+
                   |
                   v
        +--------------------+
        | PREFILTRO CARTUCHO | (Sedimentos 5 µm) ---> [Sensor P1]
        +--------------------+
                   |
                   v
        +--------------------+
        | MEMBRANA ULTRAFILTR| ---> [Sensor P2] ---> Presión Transmembrana (TMP = P1 - P2)
        +--------------------+
             |            \
 (Permeado)  v             v (Concentrado / Retrolavado)
    +-----------------+    +--------------------------+
    | MODULO 3:       |    | Retorno a Módulo 1       |
    | TANQUE PERMEADO |    | para ciclo cerrado / lav.|
    | (PEAD 25 L)     |    +--------------------------+
    +-----------------+
```

### Automatización y Control con ESP32:
* **Control de Bomba:** Variación de velocidad mediante PWM y driver paso a paso/DC para mantener un flujo transmembrana constante ($J = Q / A$).
* **Monitoreo de Ensuciamiento (*Fouling*):** Al monitorear la presión de entrada al filtro y la salida, el ESP32 calcula en tiempo real la **Presión Transmembrana (TMP)**. Cuando la TMP supera un umbral crítico programado, el sistema activa una alerta visual/sonora y conmuta una electroválvula para ciclo de retrolavado (*backwash*).

---

## 6. LISTA DE MATERIALES E INSUMOS (BOM - BILL OF MATERIALS)

A continuación se detalla la lista de compras estandarizada con formatos comerciales de venta habitual:

### 6.1 Perfilería y Metalmecánica (Estructura)
| Ítem | Descripción Técnica | Cantidad Requerida | Uso / Módulo | Justificación DFM |
| :---: | :--- | :---: | :--- | :--- |
| **1** | Tubo estructural cuadrado 20 × 20 × 1.2 mm | 2 barras (12 m total) | Bastidor Módulo 2 y Carro Módulo 3 | Perfil estándar de máxima disponibilidad, corte fácil con sensitiva |
| **2** | Chapa perforada SAE 1010 N°18 (Ø4 mm) | 1 recorte (aprox. 0.8 m²) | 3 laterales de la caja de filtro | Ventilación pasiva y peso reducido |
| **3** | Chapa lisa negra SAE 1010 N°16 (1.6 mm) | 1 recorte (aprox. 0.6 m²) | Base de caja y base rodante del carro | Resistencia al peso y rigidez estructural |
| **4** | Chapa lisa negra SAE 1010 N°18 (1.2 mm) | 1 recorte (aprox. 0.5 m²) | Techo de caja y fondo trasero del carro | Plegado liviano para maletín y arriostramiento |
| **5** | Planchuela hierro 1” × 1/8” (25.4 × 3.2 mm) | 1 tramo (1.5 m) | Refuerzos de manija, orejas y brazos | Rigidez para tornillería M8 |
| **6** | Bisagras tipo libro / munición 40 mm | 2 unidades | Puerta frontal de caja | Acceso de servicio |
| **7** | Manija metálica rebatible para baúl | 1 unidad | Techo de caja Módulo 2 | Ergonomía tipo maletín |
| **8** | Regatones de nivelación con vástago M10 y tuercas | 4 conjuntos | Patas del soporte del sedimentador | Corrección de desnivel de 29-34 cm |
| **9** | Ruedas industriales Ø50 mm (2 giratorias c/ freno + 2 fijas) | 4 unidades | Base del carro Módulo 3 | Maniobrabilidad en laboratorio |

### 6.2 Componentes Mecatrónicos y de Agitación
| Ítem | Descripción Técnica | Cantidad | Reemplazo / Función |
| :---: | :--- | :---: | :--- |
| **10** | Motorreductor DC 12V con engranajes metálicos (30-60 RPM) | 1 unidad | **Reemplaza al motor de CD**. Proporciona el torque real para agitar el agua |
| **11** | Acople elástico flexible de mandíbula 4 mm a 4 mm (o 6 mm) | 1 unidad | Vincula el eje del motor con el vástago sin transferir desalineaciones |
| **12** | Buje de teflón/Delrin o bronce autolubricado Ø interior 4 mm | 1 unidad | Centra el vástago en la tapa e impide desgaste por rozamiento |
| **13** | Caja estanca plástica IP65 (160 × 120 × 75 mm) | 1 unidad | Aísla la electrónica (ESP32) de la humedad de las mangueras |
| **14** | Tornillería completa (Bulones M6, M8 mariposas, arandelas grower) | 1 kit | Ensamble desmontable sin soldaduras fijas entre módulos |

---

## 7. RECOMENDACIONES INDUSTRIALES PARA LA DEFENSA DE TESIS

Para destacar el perfil de **Ingeniero Industrial** ante el tribunal evaluador, se sugiere incorporar los siguientes enfoques de gestión y producción en el marco teórico y la memoria descriptiva:

1. **Enfoque de Mantenibilidad y TPM (Total Productive Maintenance):**
   * El desacople rápido de la Caja de Filtro respecto del Carro de Permeado reduce el **MTTR (Mean Time to Repair)** al permitir retirar el módulo a un banco de trabajo sin desmontar toda la instalación.
   * El reemplazo de filtros y mangueras se realiza desde la puerta frontal transparente sin herramientas especiales (SMED aplicado a mantenimiento).
2. **Seguridad e Higiene Industrial (Ergonomía & Riesgo Eléctrico):**
   * Altura de mando a 700-1150 mm conforme a directrices de la ergonomía laboral.
   * Separación física clase I de baja tensión (12V) y red (220V), con toma a tierra unificada para toda la estructura metálica.
3. **Análisis de Costos y Balance Económico:**
   * Comparar el costo de fabricación propia de la estructura en acero SAE 1010 + pintura epoxi (aprox. 35-45 USD en materiales metálicos) frente a la compra de un gabinete comercial importado de acero inoxidable (superior a 250 USD). Demuestra un ahorro del **70-80%** en capex de estructura para la planta piloto.

---

*Fin del Documento Maestro — Aprobado para Fase de Construcción y Renderizado Digital.*
