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

#### Modificaciones Mecánicas a Realizar (Auditadas por Argon):
1. **Reemplazo del Motor por Motorreductor de Potencia para 25 Litros:**
   * Descartar motores de juguete o subdimensionados (< 5 kg·cm). Para mover una masa líquida de **\text{ Litros}$ (\text{ kg}$)** con paleta de acero a velocidades de hasta **\text{ RPM}$**, se requiere un **motorreductor DC 12V con torque nominal de  \text{ a } 35\text{ kg}\cdot\text{cm}$ (.0 - 3.5\text{ Nm}$)** y potencia de ** \text{ a } 40\text{ W}$** (modelo Sinfín-Corona 5840-31ZY con eje D de 8 mm o motor de limpiaparabrisas automotriz 12V con masa aislada de chasis).
   * Controlable por modulación PWM ultrasónica (\text{ kHz}$) desde el ESP32 mediante driver MOSFET **BTS7960 (43A)** con rampa *soft-start* de 2.5 s alojado en el maletín.
2. **Sistema de Guía en Tapa (Chumacera KFL08 y Montaje Excéntrico Anti-Vórtice):**
   * **Perforación Excéntrica:** Perforar la tapa desmontable a ** - 35\text{ mm}$ desplazada del centro geométrico**. Esto rompe la simetría rotacional del fluido y anula el vórtice central sin necesidad de soldar bafles interiores en la cuba.
   * Montar sobre la tapa una placa sándwich de **Grilón / Nylon de 120 × 120 mm ( - 10\text{ mm}$ de espesor)** que absorbe vibraciones ('efecto tambor') y aísla eléctricamente.
   * Fijar una **chumacera bridada autoalineante tipo KFL08 (para eje Ø 8 mm)** en acero/fundición de zinc que absorbe el 100% de los esfuerzos axiales y radiales de la paleta.
3. **Acople Motor-Eje:**
   * Utilizar un **acoplamiento elástico tipo spider / mandíbula (Jaw coupling)** de 8 mm a 8 mm con elastómero amortiguador central.
   * Elevar el motor sobre 4 separadores/espárragos roscados M6 de 50 mm (torreta abierta).
   * Eje: Varilla redonda calibrada de **Acero Inoxidable AISI 304 Ø 8 mm (tolerancia h9)** de 1 m de largo. Paleta ubicada ** - 8\text{ cm}$ por encima del inicio del fondo cónico de 60°**.
4. **Modificación de la Tapa y Manijas:**
   * Desoldar la manija plana central actual.
   * Soldar o fijar **dos manijas tipo puente en los laterales exteriores de la tapa**, permitiendo destapar el tanque de forma rápida y segura sin traccionar del motor ni del eje.
   * Sondas de instrumentación: Instalar dos prensaestopas plásticos (PG7 para DS18B20 y PG9 para electrodos TDS) en la **tapa superior**, ingresando suspendidas al líquido sin perforar la chapa bajo nivel de agua.
5. **Nivelación de la Base:**
   * En los extremos de las patas metálicas, instalar **4 regatones de nivelación basculantes con base de goma antideslizante (rosca M10)** para calibrar la horizontalidad perfecta en el laboratorio.
6. **Tomas Hidráulicas del Sedimentador (Mecanizado Inox 304):**
   * **Perforación:** Se utiliza **mecha escalonada cónica de Cobalto HSS-Co (M35 al 5%)** a bajas revoluciones (**350 a 400 RPM**) con lubricación constante de aceite de corte pesado (no usar WD-40) y taco de madera de respaldo interno.
   * **Salida de Succión (h = 5 a 8 cm sobre la unión cilíndro-cono):** Pasamuros de bronce o acero inox de 1/2' con contratuerca y **doble junta plana de Silicona Sanitaria / EPDM de 3 mm** (absorbe la curvatura del cilindro) respaldada con arandelas planas de inox. Inmediatamente a la salida se rosca **1x Válvula esférica de 1/2'** para poder desconectar el maletín sin vaciar los 25L.
   * **Retorno de Retentado (Superior):** Pasamuros de 1/4' con cañito interno adherido a la pared para descarga laminar sin espuma.
   * **Purga inferior (vértice del cono):** Válvula esférica de 1/2' de paso total para evacuación y limpieza periódica de barros de bentonita.

---

### 3.2 MÓDULO 2: CAJA DE FILTRACIÓN Y CONTROL ('MALETÍN SKID')

```
                      350 mm
        +-----------------------------------+  ^
        | [MANIJA REBATIBLE REFORZADA]      |  |
        | [TECHO: CHAPA / FENÓLICO MARINO]  |  |
        |===================================|  |
        | FACHADA EXTERIOR  | INTERIOR SECO |  |
        | (Zona Húmeda)     | (Ventilado)   |  | 500 mm
        |                   |               |  | (Altura libre
        | - FX100 Vertical  | - NEMA 34     |  |  para FX100)
        |   (Clamps 2")     | - Trafo 24VAC |  |
        | - Manómetros P1/P2| - Driver DM860|  |
        | - Válvula Aguja   | - Cargador 5V |  |
        | - Caudalím YF-S401| - Caja IP65   |  |
        | - Cabezal MBP2000 |   (ESP32/BTS) |  |
        |-------------------+---------------|  |
        | BASE: Fenólico 18mm + Patas Goma  |  v
        +-----------------------------------+
        <-------------- 350 mm ------------->
```

#### Especificaciones Constructivas y Reingeniería de Fachada Exterior (500 × 350 × 350 mm):
* **Geometría y Dimensiones Optimizadas:** Bastidor vertical de **500 mm (Alto) × 350 mm (Ancho) × 350 mm (Profundidad)**.
  * *Ventaja de los 500 mm de altura:* El cartucho dializador Fresenius FX100 mide **320 mm** de longitud; al sumar los acoples del Hospital Oñativia y los radios de curvatura de las mangueras de 1/4" sin estrangulamiento, se requiere un vano vertical libre mínimo de **420 a 450 mm**. Los 500 mm de altura vertical garantizan un tendido hidráulico rectilíneo, libre de codos forzados y profesional.
  * *Ventaja de la huella compacta (350 × 350 mm):* Reduce el peso muerto del maletín a menos de **11 kg** y minimiza el espacio ocupado en la mesada de laboratorio.
* **Segregación Estanca por Gravedad (Fachada Exterior de Proceso):**
  * **Zona Húmeda / Proceso (Frontal Exterior):** Todos los componentes con fluidos a presión se montan rígidamente en la **cara frontal exterior** sobre una placa de fenólico marino barnizado o chapa protegida:
    * Cartucho dializador **Fresenius FX100** montado verticalmente con 2 abrazaderas acolchadas de goma de 2".
    * Manómetros Bourdon de glicerina P1 y P2 (0–1 bar) montados en Tees de 1/4" orientados al operador.
    * **Válvula de aguja de 1/4"** conectada a la Tee de P2 mediante niple hexagonal 1/4" M-M para control micrométrico de TMP.
    * Caudalímetro YF-S401 fijado con manguera de silicona de 6 mm y precintos plásticos chicos.
    * Cabezal de rodillos de la bomba peristáltica MBP-2000 asomando hacia la fachada exterior (o conectado mediante pasamuros estancos inmediatos para succión y descarga).
    * *Cero riesgo eléctrico:* Cualquier goteo durante el recambio del filtro, retrolavados o purgas cae por gravedad al exterior (sobre una bandeja de goteo de mesada) y **jamás entra en contacto con los circuitos eléctricos**.
  * **Zona Seca / Eléctrica (Interior del Maletín):**
    * Al no contener mangueras presurizadas internas, el interior es un recinto **100% seco y ventilado**.
    * Motor paso a paso NEMA 34 atornillado rígidamente sobre tacos antivibratorios (silentblocks).
    * **Transformador 220V a 24V AC** elevado sobre perfiles U de aluminio, protegido por portafusible aéreo con fusible lento de 6A en el secundario.
    * **Driver Leadshine DM860** fijado directamente contra el panel lateral de chapa perforada para disipación térmica por convección natural.
    * **Alimentación ESP32:** Cargador USB homologado de 5.0V / 2.0A conectado internamente a los 220V (evita el problema de auto-apagado de los powerbanks comerciales).
    * **Caja estanca plástica IP65 chica (150 × 110 × 70 mm):** Alojando exclusivamente el ESP32 NodeMCU-32S, borneras de señal y driver BTS7960 para el agitador, con prensaestopas inferiores y bucle de goteo (*drip loop*).
* **Panel de Conexiones Lateral:**
  * Entrada C14 interlock 220V con fusible y tecla bipolar luminosa.
  * Conector industrial GX16 de 4 pines para conexión remota del motorreductor del sedimentador y la boya de nivel.

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
