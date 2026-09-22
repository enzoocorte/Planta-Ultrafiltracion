# 📐 ARQUITECTURA GENERAL Y DIAGRAMA DE FLUJO DE INSTRUMENTACIÓN (P&ID)
## Planta Piloto de Coagulación-Sedimentación y Ultrafiltración FX100
**Investigación Doctoral / Tesis de Grado**: Ing. Enzo (Codirector) • Antonella Guitián & Owen Cañizares (Tesistas)  
**Ubicación**: `00_General_y_P_ID_Planta/`

---

## 🎯 1. Propósito y Filosofía del Diagrama

Este documento establece la **visión sistémica e integral** de toda la planta piloto. Mientras que los **Hitos 1 al 5** se enfocan en la validación modular y experimental de cada subsistema, este diagrama P&ID (*Piping and Instrumentation Diagram*) representa el **plano maestro permanente** que conecta hidráulica, mecánica y electrónicamente todos los componentes del banco de ensayos.

### 🖼️ Boceto Original de Banco vs Diagrama Formalizado
* **Boceto Original manuscrito en taller**: [`boceto_original_enzo_pid.jpg`](./boceto_original_enzo_pid.jpg)
* **Diagrama Formal ISA 5.1 e Interactivo**: Este documento y el simulador visual complementario [`diagrama_pid_interactivo.html`](./diagrama_pid_interactivo.html).

---

## 🗺️ 2. Diagrama P&ID Formal (Norma ISA 5.1)

### Código de Simbología de Líneas:
* `════════` **Línea Doble / Continua Gruesa**: Tuberías y mangueras hidráulicas de proceso (recorrido de fluidos).
* `- - - - -` **Línea de Trazo Discontinuo**: Señales eléctricas e instrumentación analógica/digital hacia el ESP32.
* `·········` **Línea Punteada**: Buses digitales multiplexados (Bus I2C para ADS1115 y Bus OneWire para DS18B20).

```mermaid
flowchart LR
    %% ESTILOS GENERALES
    classDef tanque fill:#1e293b,stroke:#38bdf8,stroke-width:2px,color:#f8fafc;
    classDef bomba fill:#0f172a,stroke:#10b981,stroke-width:2px,color:#f8fafc;
    classDef filtro fill:#1e1e38,stroke:#a855f7,stroke-width:2px,color:#f8fafc;
    classDef sensor fill:#0f172a,stroke:#f59e0b,stroke-width:1.5px,color:#f8fafc;
    classDef actuador fill:#0f172a,stroke:#ec4899,stroke-width:1.5px,color:#f8fafc;
    classDef esp fill:#020617,stroke:#38bdf8,stroke-width:3px,color:#38bdf8;
    classDef valvula fill:#1e293b,stroke:#64748b,stroke-width:1.5px,color:#f8fafc;

    %% NODO CENTRAL: CEREBRO ESP32
    ESP["💻 ESP32 DevKit V1<br><b>(Cerebro Central SCADA)</b>"]:::esp
    ADS["📟 ADC ADS1115<br><b>(16 Bits I2C)</b>"]:::sensor

    %% =========================================================================
    %% ETAPA 1: SEDIMENTADOR / REACTOR
    %% =========================================================================
    subgraph ETAPA_1["1. REACTOR COAGULADOR - SEDIMENTADOR CÓNICO"]
        direction TB
        TAPA["Tapa Superior Hermética<br>(Entrada Agua Turbia + Coagulante)"]
        NIVEL["🛡️ Boya Inox Nivel Mínimo<br>(Reed Switch)"]:::sensor
        AGIT["🌪️ Paleta Agitadora<br>+ Motor DC 12V"]:::actuador
        DRIVER_L298N["⚡ Driver Puente H<br>L298N (12V)"]:::actuador
        CUERPO_SED["Tanque Reactor Cónico<br><b>(Sedimentación Gravitacional)</b>"]:::tanque
        VALV_PURGA["🔻 Válvula Purga Fondo<br>(Extracción de Lodos)"]:::valvula
        VALV_SOBRENAD["🟢 Válvula de Salida<br>(Toma de Sobrenadante)"]:::valvula

        TAPA --> CUERPO_SED
        CUERPO_SED --> VALV_PURGA
        CUERPO_SED --> VALV_SOBRENAD
    end

    %% =========================================================================
    %% ETAPA 2: PRETRATAMIENTO Y BOMBEO
    %% =========================================================================
    subgraph ETAPA_2["2. PROTECCIÓN MECÁNICA Y ACCIONAMIENTO (HITO 1)"]
        direction TB
        PREFILTRO["🛡️ PREFILTRO Y-STRAINER<br><b>Malla Inox 120 Mesh (125 µm)</b><br>(Vaso transparente lavable)"]:::filtro
        BOMBA["🌀 Bomba Peristáltica<br><b>MBP-2000 (3 Rodillos)</b>"]:::bomba
        DRIVER_DM860["⚡ Driver Leadshine DM860<br>(Ánodo Común 5V)"]:::actuador
        MOTOR_NEMA["⚙️ Motor NEMA 34<br>(4.5 Nm - Bipolar Serie)"]:::actuador

        DRIVER_DM860 ==> MOTOR_NEMA
        MOTOR_NEMA -. Eje Mecánico .-> BOMBA
    end

    %% =========================================================================
    %% ETAPA 3: INSTRUMENTACIÓN DE ENTRADA Y ULTRAFILTRACIÓN
    %% =========================================================================
    subgraph ETAPA_3["3. MEMBRANA DE ULTRAFILTRACIÓN Y SENSORES"]
        direction TB
        P1["📊 Transductor Presión P1<br><b>(Entrada Feed: 0-1.2 bar)</b>"]:::sensor
        Q1["🌊 Caudalímetro Entrada Q1<br><b>(Efecto Hall YF-S401)</b>"]:::sensor
        MEMBRANA["🧪 MEMBRANA FX100<br><b>Filtro Capilar Fresenius</b><br>(1.8 m² Polisulfona / Helixone)"]:::filtro
        
        P2["📊 Transductor Presión P2<br><b>(Retentado / Salida Axial)</b>"]:::sensor
        V_REG["🔴 Válvula de Aguja V_reg<br><b>(Regulación de Contrapresión / TMP)</b>"]:::valvula
    end

    %% =========================================================================
    %% ETAPA 4: PERMEADO Y CALIDAD DE AGUA
    %% =========================================================================
    subgraph ETAPA_4["4. LÍNEA DE PERMEADO Y MONITOREO DE CALIDAD"]
        direction TB
        P3["📊 Transductor Presión P3<br><b>(Salida Permeado: 0-1.2 bar)</b>"]:::sensor
        QP["🌊 Caudalímetro Permeado Qp<br><b>(Efecto Hall YF-S401)</b>"]:::sensor
        TANQUE_PERM["🪣 Tanque de Permeado<br><b>(Agua Clarificada y Filtrada)</b>"]:::tanque
        SENSOR_TDS["🧪 Sonda TDS<br><b>(Conductividad / ppm)</b>"]:::sensor
        SENSOR_TEMP["🌡️ Sonda DS18B20 Inox<br><b>(Temperatura °C - OneWire)</b>"]:::sensor
        GRIFO_MUESTREO["💧 Válvula Muestreo / Drenaje"]:::valvula

        TANQUE_PERM --> GRIFO_MUESTREO
    end

    %% =========================================================================
    %% CONEXIONES HIDRÁULICAS (LÍNEA LLENA GRUESA)
    %% =========================================================================
    VALV_SOBRENAD == "Manguera Succión 1/4" ==> PREFILTRO
    PREFILTRO == "Agua sin partículas >125µm" ==> BOMBA
    BOMBA == "Impulsión Peristáltica" ==> P1
    P1 ==> Q1
    Q1 == "Alimentación Capilar (Feed)" ==> MEMBRANA

    %% RETENTADO (Línea de retorno tangencial)
    MEMBRANA == "Salida Retentado Axial" ==> P2
    P2 ==> V_REG
    V_REG == "Retorno / Recirculación Tangencial" ==> CUERPO_SED

    %% PERMEADO (Salida lateral de la membrana)
    MEMBRANA == "Salida Permeado Radial" ==> P3
    P3 ==> QP
    QP == "Descarga Permeado" ==> TANQUE_PERM

    %% =========================================================================
    %% CONEXIONES ELÉCTRICAS Y DE DATOS AL ESP32 (LÍNEAS PUNTEADAS / DISCONTINUAS)
    %% =========================================================================
    %% ETAPA 1 -> ESP32
    NIVEL -. "GPIO 32 (Directa: Pull-up a GND)" .-> ESP
    ESP -. "GPIO 4 (PWM), 16, 17 (Giro)" .-> DRIVER_L298N
    DRIVER_L298N -. "12V Potencia" .-> AGIT

    %% ETAPA 2 -> ESP32
    ESP -. "VIN (5V), P18 (LEDC Pul), P19 (Open-Drain Dir)" .-> DRIVER_DM860

    %% ETAPA 3 -> ESP32
    Q1 -. "GPIO 14 (Pulsos Interrupción)" .-> ESP
    P1 -. "Canal A1 (Analógica 0.5-4.5V)" .-> ADS
    P2 -. "Canal A2 (Analógica 0.5-4.5V)" .-> ADS

    %% ETAPA 4 -> ESP32
    P3 -. "Canal A3 (Analógica 0.5-4.5V)" .-> ADS
    QP -. "GPIO 27 (Pulsos Interrupción)" .-> ESP
    SENSOR_TDS -. "Canal A0 (Analógica 0-2.3V)" .-> ADS
    SENSOR_TEMP -. "GPIO 34 (OneWire con 4.7kΩ)" .-> ESP

    %% BUS I2C: ADS1115 -> ESP32
    ADS ····· "GPIO 21 (SDA) & GPIO 22 (SCL)" ····· ESP
```

---

## 🔬 3. Evaluación de Ingeniería: Fortalezas y Mejoras Críticas al Boceto

Al analizar tu boceto manuscrito con criterios estrictos de ingeniería de procesos y mecánica de fluidos, la estructura conceptual es excelente. A continuación se detallan las **mejoras de diseño necesarias para que el banco funcione de forma impecable**:

```
┌──────────────────────────────────────┬──────────────────────────────────────┬────────────────────────────────────────────────────────┐
│ Componente / Tramo                   │ Planteo Inicial del Boceto           │ Optimización de Ingeniería Recomendada                 │
├──────────────────────────────────────┼──────────────────────────────────────┼────────────────────────────────────────────────────────┤
│ 1. Salida del Sedimentador           │ 1 sola válvula de descarga general   │ • Válvula de sobrenadante (a 4 cm sobre el cono)       │
│                                      │                                      │ • Válvula de purga en el vértice inferior para lodos   │
├──────────────────────────────────────┼──────────────────────────────────────┼────────────────────────────────────────────────────────┤
│ 2. Prefiltro Sedimentador ➔ Bomba    │ Concepto de protección de manguera   │ • Malla inoxidable lavable tipo Y de 120 mesh (125 µm) │
│                                      │                                      │ • Vaso transparente para inspección visual directa     │
├──────────────────────────────────────┼──────────────────────────────────────┼────────────────────────────────────────────────────────┤
│ 3. Retorno de Membrana al Tanque     │ Retorno directo con sensor P2        │ ⭐ INCORPORAR VÁLVULA DE AGUJA REGULADORA (V_reg)      │
│                                      │                                      │ (Sin ella, el agua no genera contrapresión y no filtra)│
├──────────────────────────────────────┼──────────────────────────────────────┼────────────────────────────────────────────────────────┤
│ 4. Sensores de Presión (P1, P2, P3)  │ Indicados hacia el ESP32             │ Deben pasar por conversor ADC externo ADS1115 (16 bits)│
│                                      │                                      │ (El ADC interno del ESP32 es ruidoso y soporta máx 3.3V│
├──────────────────────────────────────┼──────────────────────────────────────┼────────────────────────────────────────────────────────┤
│ 5. Normalización de Flujo Darcy      │ Caudal de permeado y TDS en tanque   │ Sensor de temperatura DS18B20 sumergido en permeado    │
│                                      │                                      │ (Vital para corregir viscosidad por temperatura a 20°C)│
└──────────────────────────────────────┴──────────────────────────────────────┴────────────────────────────────────────────────────────┘
```

---

## ⚡ 4. Matriz Exhaustiva de Conexionado al ESP32: ¿Directo o Indirecto?

| Sensor / Actuador | Señal Eléctrica | ¿Conexión Directa al ESP32? | Interfaz / Acondicionador | Pin Asignado ESP32 | Función en el Proceso |
| :--- | :--- | :---: | :--- | :--- | :--- |
| **Boya de Nivel Inox** | Digital (On/Off) | **SÍ (Directa)** | Resistencia Pull-up interna | `GPIO 32` | Corte contra marcha en seco de bomba y agitador. |
| **Motor Agitador (Paleta)** | Potencia 12V DC (PWM) | **NO (Indirecta)** | **Driver Puente H L298N** | `GPIO 4` (PWM), `GPIO 16, 17` (Dir) | Gradiente $G$ de Camp-Stein (Mezcla rápida, lenta, reposo). |
| **Bomba Peristáltica MBP-2000**| Pulsos STEP + DIR | **NO (Indirecta)** | **Driver Leadshine DM860** | `GPIO 18` (PUL), `GPIO 19` (DIR) | Accionamiento NEMA 34 (70 a 160 RPM) con Ánodo Común 5V. |
| **Caudalímetro Entrada ($Q_1$)**| Tren de Pulsos (Hall) | **SÍ (Directa)** | Colector abierto con pull-up | `GPIO 14` (Interrupción) | Medición de caudal de alimentación (*Feed*). |
| **Transductor Presión Entrada ($P_1$)**| Analógica $0.5 - 4.5\text{V}$ | **NO (Indirecta)** | **Canal A1 del ADS1115** | Bus I2C (`GPIO 21` / `22`) | Presión de impulsión y cálculo de $\text{TMP}$. |
| **Transductor Presión Retentado ($P_2$)**| Analógica $0.5 - 4.5\text{V}$| **NO (Indirecta)** | **Canal A2 del ADS1115** | Bus I2C (`GPIO 21` / `22`) | Presión de concentrado previo a la válvula de aguja. |
| **Transductor Presión Permeado ($P_3$)** | Analógica $0.5 - 4.5\text{V}$| **NO (Indirecta)** | **Canal A3 del ADS1115** | Bus I2C (`GPIO 21` / `22`) | Contrapresión de permeado y cálculo de $\text{TMP}$. |
| **Caudalímetro Permeado ($Q_p$)**| Tren de Pulsos (Hall) | **SÍ (Directa)** | Colector abierto con pull-up | `GPIO 27` (Interrupción) | Caudal de agua filtrada para cálculo de flujo $J$ (LMH). |
| **Sonda Calidad de Agua (TDS)**| Analógica $0 - 2.3\text{V}$ | **NO (Indirecta)** | **Canal A0 del ADS1115** | Bus I2C (`GPIO 21` / `22`) | Medición de ppm y retención de sólidos disueltos. |
| **Sonda Temperatura (DS18B20)**| Digital (Bus OneWire) | **SÍ (Directa)** | Resistencia externa $4.7\text{ k}\Omega$ a 3.3V | `GPIO 34` | Factor de corrección térmica $TCF$ según modelo Darcy. |

---

## 🚰 5. Los Tres Puntos Clave de Hidráulica Aplicada

### 5.1. ¿Por qué la Válvula de Regulación ($V_{\text{reg}}$) es Manual de Aguja y NO Automática?
* **Decisión de Ingeniería de Membranas**: Se utiliza una **Válvula Manual de Aguja de $1/4''$ en acero inoxidable** con volante micrométrico.
* **¿Por qué manual y no una servoválvula electrónica?**:
  1. **Metodología de Ensayo Científico**: La caracterización hidrodinámica y la determinación de las resistencias de Darcy ($R_m$ y $R_c$) exigen operar a **Presión Transmembrana constante ($\text{TMP}$)** (ej. $0.20, 0.30\text{ ó }0.40\text{ bar}$) para registrar cómo decae el flujo en función del tiempo.
  2. **Estabilidad absoluta sin oscilaciones**: Una válvula automática con lazo PID tiende a oscilar ante pequeñas fluctuaciones de caudal pulsátil de la bomba peristáltica, generando picos de presión que fatigan los capilares. La válvula manual de aguja fija una restricción física milimétrica constante y libre de fallas electrónicas.
  3. **Costo y practicidad**: Una servoválvula industrial para microflujos de líquidos agresivos/coloidales es sumamente costosa. Con la válvula manual, el tesista observa el display web del SCADA y en 3 segundos gira la perilla hasta la TMP deseada.
* **¿Qué sucede si no estuviera?**: El agua tomaría el camino de menor resistencia (el conducto axial interno de los capilares) y retornaría toda al sedimentador, dando un caudal de permeado casi nulo ($Q_p \approx 0$).

### 5.2. El Prefiltro: ¿Por qué tipo "Y" de 120 mesh ($125\,\mu\text{m}$)?
* Los capilares de la membrana FX100 tienen un diámetro interno de **$200\,\mu\text{m}$**.
* El coagulante natural (*Opuntia ficus-indica*) produce flóculos gelatinosos y puede contener micro-fibras vegetales.
* Si una partícula mayor a $150\,\mu\text{m}$ ingresa a la membrana, tapona físicamente la embocadura de una fibra capilar, inutilizándola permanentemente.
* Un prefiltro tipo "Y" lavable de $120\text{ mesh}$ ($125\,\mu\text{m}$) retiene todo cuerpo sólido peligroso sin generar una pérdida de carga excesiva que cavite la succión de la bomba peristáltica.

### 5.3. Altura de la Toma del Sobrenadante en el Sedimentador
* El tanque sedimentador debe tener una geometría cónica o inclinada en la base.
* La **Válvula de Purga de Fondo** se abre periódicamente para evacuar la torta de lodos decantados.
* La **Válvula de Salida a la Bomba** se coloca unos $3\text{ a }5\text{ cm}$ por encima de la cota máxima del lodo compactado. Así, la bomba peristáltica aspira únicamente líquido sobrenadante clarificado, maximizando la vida útil del filtro FX100.

### 5.4. Motor de la Paleta y Driver Puente H L298N: Tensión y Acople Mecánico
* **¿Por qué 12V y no un motor de menor voltaje (5V o 6V)?**:
  1. **Caída de tensión interna del driver L298N**: El chip L298N está construido con transistores bipolares (BJT) que introducen una caída de tensión de saturación de **$V_{CE(sat)} \approx 2.0\text{V} \text{ a } 2.5\text{V}$**. Si se alimenta con **5V**, al motor solo le llegan entre **$2.5\text{V} \text{ y } 3.0\text{V}$**, haciendo que gire casi sin torque y se clave al entrar en contacto con el agua espesa. Con **12V**, al motor le llegan aproximadamente **$9.5\text{V} \text{ a } 10.0\text{V}$**, entregando par suficiente en todo momento.
  2. **Resistencia viscosa de la mezcla**: El agua sintética de bentonita ($\approx 500\text{ NTU}$) y el extracto de *Opuntia ficus-indica* (mucílago biopolimérico viscoso) oponen una resistencia hidrodinámica considerable ($F_D$). Un motorreductor de 12V con caja reductora metálica entrega entre $2\text{ y }5\text{ kg}\cdot\text{cm}$ de torque sostenido sin recalentarse.
  3. **Disponibilidad universal**: Cualquier fuente switching común de $12\text{V DC}$ (1A o 2A, como las de módem/cámaras) o un convertidor Step-Down LM2596 conectado al transformador de la planta alimenta el sistema con total facilidad.
* **¿Cómo se conecta mecánicamente la paleta al motor?**:
  1. **El Motorreductor**: Se utiliza un motorreductor DC de 12V (200 a 300 RPM nominales con eje metálico de $4\text{ mm}$, $6\text{ mm}$ u $8\text{ mm}$ con chaflán plano tipo "D").
  2. **El Acople de Eje**: Se monta un **Acople Rígido de Aluminio para CNC / Impresora 3D** (cilindro de aluminio con prisioneros Allen M3/M4, con orificio de $4\text{ mm} \to 6\text{ mm}$ o $6\text{ mm} \to 8\text{ mm}$).
  3. **El Eje del Agitador**: Una varilla de acero inoxidable AISI 304 de $6\text{ mm}$ u $8\text{ mm}$ (de unos $25\text{ a }35\text{ cm}$ de longitud) se introduce en el otro extremo del acople de aluminio y se aprieta con llave Allen.
  4. **Guía en la Tapa (Evita bamboleo)**: En el centro de la tapa del reactor se encastra un **buje de teflón o un rodamiento 608** (de patín/roller). El eje pasa por el rodamiento, que absorbe las fuerzas radiales y mantiene la varilla perfectamente vertical sin forzar el reductor del motor.
  5. **Fijación de la Paleta**: La paleta (rectángulo de acrílico cortado a láser, chapa fina de acero inoxidable 304 o impresión 3D en PETG de aprox. $7\text{ cm} \times 2.5\text{ cm}$) tiene una perforación central y se fija en el extremo inferior de la varilla mediante **arandela + tuerca + contratuerca inox (o tuerca autofrenante con teflón)**, asegurando que no se afloje jamás durante la rotación.
* **Control Dual en el mismo ESP32**:
  * La **Bomba NEMA 34** utiliza el canal LEDC 0 por hardware en `GPIO 18` (PUL) y `GPIO 19` (DIR) a través del driver DM860.
  * La **Paleta Agitadora** utiliza el canal LEDC 1 (PWM) en `GPIO 4` (D4) y dos pines lógicos `GPIO 16` (D16) y `GPIO 17` (D17) para el sentido de giro a través del driver L298N.
  * Ambos accionamientos operan de manera simultánea, síncrona y sin conflicto de recursos gracias a la arquitectura Dual-Core a 240 MHz del ESP32.


### 5.5. Sensor de Nivel (Boya Inox): ¿Cómo fijar la cota y cómo cablear?
* **¿Cómo colocarlo a la altura deseada?**:
  * **Montaje Lateral (Recomendado)**: Se taladra la pared del reactor a la altura mínima admisible (unos 4 cm sobre el vértice del cono, inmediatamente encima de la válvula de sobrenadante). Se inserta la rosca macho con su junta tórica de silicona (O-ring) y se aprieta la tuerca exterior.
  * **Montaje Suspendido**: Se cuelga una varilla rígida desde la tapa hermética que desciende la boya hasta dicha altura crítica.
* **Principio y Conexión**:
  * La boya tiene **solo 2 cables** y no tiene polaridad.
  * **Cable 1** ──► Borne **`GPIO 32` (D32)** del ESP32.
  * **Cable 2** ──► Borne **`GND`** del ESP32.
  * En el firmware se activa la resistencia pull-up interna (`pinMode(32, INPUT_PULLUP)`). Cuando hay agua, el flotador sube y cierra el circuito a masa (`LOW`). Si el nivel desciende por debajo de la boya, cae por gravedad, el circuito se abre (`HIGH`), y el ESP32 **apaga en el acto la bomba peristáltica y el agitador** (evitando la marcha en seco y la entrada de aire a la membrana).

### 5.6. Conversor ADS1115: ¿Se necesitan varios o alcanza con uno solo?
* **¡CON UN SOLO ADS1115 ALCANZA PARA TODA LA PLANTA!**
* El módulo ADS1115 cuenta con **4 canales analógicos independientes (A0 a A3)** con resolución de 16 bits:
  * **Canal `A0`**: Sonda de Calidad de Agua (**TDS**, salida $0\text{ a }2.3\text{V}$).
  * **Canal `A1`**: Transductor de Presión **$P_1$** (Alimentación / Feed, $0.5\text{ a }4.5\text{V}$).
  * **Canal `A2`**: Transductor de Presión **$P_2$** (Retentado / Salida axial, $0.5\text{ a }4.5\text{V}$).
  * **Canal `A3`**: Transductor de Presión **$P_3$** (Permeado / Salida radial, $0.5\text{ a }4.5\text{V}$).
* **Conexión al ESP32**:
  * Solo requiere 2 cables de datos por el bus I2C:
    * Borne `SDA` del ADS1115 ──► Borne **`GPIO 21` (D21)** del ESP32.
    * Borne `SCL` del ADS1115 ──► Borne **`GPIO 22` (D22)** del ESP32.
    * Borne `VDD` a `5V` (o `3V3`), `GND` a `GND` común, y `ADDR` a `GND` (dirección fija `0x48`).


---

## 🎯 Entregable Concreto de este Módulo General
* **Plano Maestro P&ID y Arquitectura Sistémica de la Planta**: Diagrama normalizado según norma ISA 5.1 que unifica el reactor de coagulación, la bomba MBP-2000, el módulo de ultrafiltración FX100, la instrumentación de 6 variables físicas y la interfaz de control embebida en el microcontrolador ESP32.

---

## 📋 Lista de Verificación (Checklist de Avance del P&ID)
- [x] Diagrama formal Mermaid generado con diferenciación estricta de mangueras (línea llena) y cables de señal al ESP32 (línea punteada).
- [x] Conexión de actuadores y sensores clasificada formalmente entre interfaces directas e indirectas (Puente H L298N, Driver DM860, ADC ADS1115).
- [x] Incorporación justificada de la válvula de aguja de retentado $V_{\text{reg}}$ para presurización transmembrana (TMP).
- [x] Especificación técnica del prefiltro de succión lavable (tipo Y, 120 mesh / $125\,\mu\text{m}$).
- [x] Boceto original de banco archivado en `00_General_y_P_ID_Planta/boceto_original_enzo_pid.jpg`.
- [x] Diagrama interactivo visual generado para consulta en navegador web.
