# 🟢 FASE 1: ACCIONAMIENTO Y COAGULACIÓN-SEDIMENTACIÓN
## 🎯 HITO 1: Control de Accionamiento de Potencia (NEMA 34 + DM860 + ESP32)

---

### 1.1. Objetivo de Ingeniería

Lograr el **control preciso, continuo y libre de resonancias** del motor paso a paso bipolar **NEMA 34** ($4.0\text{ Nm}$, corriente nominal hasta $6.0\text{ A}$) acoplado mecánicamente al cabezal de la bomba peristáltica **MBP-2000**. 

El sistema debe permitir:
1. Fijar con precisión milimétrica la velocidad de rotación en el rango de **$0\text{ a }120\text{ RPM}$** mediante comandos por puerto serie o variables de control.
2. Implementar una **rampa de aceleración y desaceleración suave** ($35\text{ RPM/s}$) que evite la pérdida de sincronismo (pérdida de pasos) debida a la inercia del rotor y la resistencia viscoelástica del tubo de silicona/PharMed.
3. Invertir el sentido de giro de manera controlada para permitir los dos modos hidrodinámicos fundamentales:
   * **Sentido Horario (CW)**: Filtración continua hacia la membrana capilar Fresenius FX100.
   * **Sentido Antihorario (CCW)**: Ciclo de Retrolavado (*Backwash*) para desprender la torta de filtración.
4. Garantizar la **estabilidad térmica del motor y del driver**, evitando el sobrecalentamiento en reposo mediante el algoritmo de reducción de corriente por hardware (*Standstill Current*).

---

### 1.2. Componentes Necesarios y Especificaciones Técnicas

| Componente | Especificaciones de Ingeniería | Rol en el Sistema |
| :--- | :--- | :--- |
| **Microcontrolador ESP32** | NodeMCU DevKit V1 (38 pines, 3.3V lógico, 240 MHz). | Generador determinístico de tren de pulsos y procesador de comandos. |
| **Driver Digital DM860** | Leadshine DM860 (DSP digital, $18-80\text{ VAC}$ / $24-110\text{ VDC}$, corriente pico hasta $7.2\text{ A}$). | Etapa de potencia MOSFET con aislamiento optoacoplado de alta velocidad. |
| **Motor Paso a Paso NEMA 34** | Bipolar, 2 fases, 4 cables, torque estático $4.0\text{ Nm}$, $6.0\text{ A/fase}$, ángulo de paso nativo $1.8^\circ$ ($200\text{ pasos/rev}$). | Impulsor mecánico de los rodillos del cabezal de bombeo peristáltico. |
| **Transformador de Potencia** | Primario $220\text{ VAC}$, Secundario $24\text{ VAC}$ ($150\text{ VA}$ a $200\text{ VA}$). | Fuente de energía para la etapa de potencia del motor (rectificado interno a bus de $\approx 34\text{ VDC}$). |
| **Bomba Peristáltica MBP-2000** | Cabezal con 3 rodillos, manguera elastomérica de silicona/PharMed BPT. | Desplazamiento positivo sin contacto de fluido ($4.2\text{ mL/revolución}$). |
| **Conectores y Borneras** | Cables de cobre flexibles de $1.5\text{ mm}^2$ (potencia) y cables Dupont (lógica). | Interconexión robusta sin caídas de tensión ni falsos contactos. |

---

### 1.3. Esquema de Cableado Pin a Pin Exhaustivo

El cableado se divide estrictamente en dos secciones: **Lógica de Control (Baja Tensión 3.3V)** y **Potencia (Media Tensión / Corriente)**.

```
       ESP32 DevKit V1 (38 Pines)                     DRIVER LEADSHINE DM860
     ┌────────────────────────────┐                 ┌─────────────────────────────┐
     │           GPIO 18 (D18) ───┼────────────────►│ PUL+ (PULSE / STEP)         │
     │           GPIO 19 (D19) ───┼────────────────►│ DIR+ (DIRECCIÓN CW/CCW)     │
     │           GPIO 21 (D21) ───┼───────────(Opc)►│ ENA+ (ENABLE / HABILITACIÓN)│
     │                     GND ───┼─┬──────────────►│ PUL- (Masa de Pulsos)       │
     │                            │ ├──────────────►│ DIR- (Masa de Dirección)    │
     │                            │ └─────────(Opc)►│ ENA- (Masa de Enable)       │
     └────────────────────────────┘                 └─────────────────────────────┘
                                                    ┌─────────────────────────────┐
        TRANSFORMADOR 24 VAC                        │ AC / VCC  (Entrada Poder)   │
        (150 VA ➔ Bus interno 34V DC) ─────────────►│ AC / GND  (Entrada Poder)   │
                                                    ├─────────────────────────────┤
        MOTOR NEMA 34 (6A)                          │ A+ (Cable Fase A1)          │
        (Bobinado Bipolar 4 Cables)   ─────────────►│ A- (Cable Fase A2)          │
                                                    │ B+ (Cable Fase B1)          │
                                                    │ B- (Cable Fase B2)          │
                                                    └─────────────────────────────┘
```

#### 🔍 Detalle Crítico de Conexión de Cada Borne:

1. **Borne `PUL+` y `PUL-` (Tren de Pulsos de Paso)**:
   * `PUL+` va directo al pin **`GPIO 18`** del ESP32.
   * `PUL-` va a la masa **`GND`** del ESP32.
   * *Funcionamiento*: Cada flanco de subida de $3.3\text{V}$ enciende el optoacoplador interno y avanza el motor exactamente un micropaso.
2. **Borne `DIR+` y `DIR-` (Sentido de Rotación)**:
   * `DIR+` va directo al pin **`GPIO 19`** del ESP32.
   * `DIR-` va a la masa **`GND`** del ESP32.
   * *Funcionamiento*: Con `HIGH` ($3.3\text{V}$), el motor gira en sentido Horario. Con `LOW` ($0\text{V}$), gira en sentido Antihorario.
3. **Borne `ENA+` y `ENA-` (Habilitación / Torque)**:
   * **Recomendación de Planta**: Dejar **DESCONECTADOS (al aire)**. 
   * *Razón Técnica*: El driver DM860 viene configurado de fábrica para que con `ENA` desconectado (o a nivel bajo) el driver esté **SIEMPRE HABILITADO** con torque de retención. Si se conecta a `GPIO 21`, poner el pin en `HIGH` apagaría el driver dejando el eje libre (suelto).
4. **Bornes de Potencia `AC / AC`**:
   * Los 2 cables de salida de $24\text{ VAC}$ del transformador se conectan directamente a estos bornes. 
   * No importa la polaridad porque el DM860 posee un puente rectificador interno con banco de capacitores electrolíticos que eleva y filtra la tensión a:
     $$V_{\text{bus DC}} = V_{\text{RMS}} \times \sqrt{2} = 24\text{V} \times 1.4142 \approx 33.94\text{ VDC}$$
5. **Bornes del Motor `A+, A-, B+, B-`**:
   * Fase A: Los dos cables del primer bobinado se conectan en `A+` y `A-`.
   * Fase B: Los dos cables del segundo bobinado se conectan en `B+` y `B-`.
   * *Identificación en taller*: Si juntas dos cables del motor y el eje se pone duro para girar con la mano, pertenecen a la misma fase.

---

### 1.4. Fundamento Matemático (Conversión de Frecuencia de Pulsos a RPM)

El microcontrolador no comanda "RPM" directamente, sino que sintetiza una frecuencia de onda cuadrada $f$ cuyos periodos dictan la velocidad angular del rotor.

#### Ecuación Fundamental de Frecuencia:
Dado que el driver DM860 está configurado en **$1600\text{ pulsos por revolución}$** (lo que equivale a 8 micropasos por cada paso nativo de $1.8^\circ$):

$$f_{\text{pulsos}} (\text{Hz}) = \frac{\text{RPM} \times \text{PPR}}{60} = \frac{\text{RPM} \times 1600}{60} = \mathbf{\text{RPM} \times 26.6667\text{ Hz}}$$

El período temporal $T_{\text{periodo}}$ entre pulsos sucesivos es:

$$T_{\text{periodo}} = \frac{1}{f_{\text{pulsos}}} \quad \implies \quad T_{\text{semiciclo}} (\mu\text{s}) = \frac{10^6}{2 \times f_{\text{pulsos}}} = \frac{18750}{\text{RPM}}$$

#### 📊 Tabla de Conversión para Ensayos de Planta:

| Velocidad Deseada | Frecuencia de Pulsos ($f$) | Período Total ($T$) | Tiempo en HIGH / LOW ($T/2$) | Caudal Estimado MBP-2000 |
| :---: | :---: | :---: | :---: | :---: |
| **$10\text{ RPM}$** | $266.67\text{ Hz}$ | $3750\,\mu\text{s}$ | $1875\,\mu\text{s}$ | $0.042\text{ L/min}$ |
| **$30\text{ RPM}$** | $800.00\text{ Hz}$ | $1250\,\mu\text{s}$ | $625\,\mu\text{s}$ | $0.126\text{ L/min}$ |
| **$60\text{ RPM}$** | $1600.00\text{ Hz}$ | $625\,\mu\text{s}$ | $312.5\,\mu\text{s}$ | $0.252\text{ L/min}$ |
| **$100\text{ RPM}$** | $2666.67\text{ Hz}$ | $375\,\mu\text{s}$ | $187.5\,\mu\text{s}$ | $0.420\text{ L/min}$ |
| **$120\text{ RPM}$** | $3200.00\text{ Hz}$ | $312.5\,\mu\text{s}$ | $156.25\,\mu\text{s}$ | $0.504\text{ L/min}$ (Máx. FX100) |

---

### 1.5. Configuración Detallada de los DIP Switches en el DM860

En el lateral del driver hay 8 micro-interruptores (*DIP Switches*). Esta configuración es **el factor decisivo para que el motor no hierva y trabaje silencioso**:

```
 ┌───────────────────────────────────────────────────────────────────────────────────┐
 │                  TABLA DE CONFIGURACIÓN DIP SWITCHES (LEADSHINE DM860)            │
 ├───────────┬───────────┬───────────┬───────────────────────────────────────────────┤
 │ Switch    │ Posición  │ Parámetro │ Razón Técnica de Ingeniería Industrial        │
 ├───────────┼───────────┼───────────┼───────────────────────────────────────────────┤
 │ **SW1**   │ **OFF**   │ Corriente │ Fija la corriente nominal en ~4.0A - 5.0A     │
 │ **SW2**   │ **ON**    │ RMS /     │ (Pico ~6.0A). Proporciona el torque total     │
 │ **SW3**   │ **ON**    │ Peak      │ de 4 Nm sin saturar magnéticamente el núcleo. │
 ├───────────┼───────────┼───────────┼───────────────────────────────────────────────┤
 │ **SW4**   │ ❗**OFF**❗│ Standstill│ **50% de Corriente en Reposo (OBLIGATORIO)**.  │
 │           │           │ Current   │ Al detener la bomba, el driver reduce a la    │
 │           │           │           │ mitad la disipación Joule ($P = I^2 R$).     │
 │           │           │           │ **Elimina por completo el sobrecalentamiento**│
 ├───────────┼───────────┼───────────┼───────────────────────────────────────────────┤
 │ **SW5**   │ **ON**    │           │                                               │
 │ **SW6**   │ **OFF**   │ Resolución│ **1600 pulsos por revolución (8 micropasos)**.│
 │ **SW7**   │ **ON**    │ Micropasos│ Suprime resonancias acústicas y vibraciones,  │
 │ **SW8**   │ **ON**    │           │ entregando flujo continuo sin turbulencias.   │
 └───────────┴───────────┴───────────┴───────────────────────────────────────────────┘
```

---

### 1.6. Código Modular de Control (`Hito1_ControlMotor.ino`)

A diferencia de un código básico bloqueante con `delayMicroseconds` (que se traba si la CPU atiende el puerto serie o el Wi-Fi), este código utiliza una **máquina de estados determinística y el periférico de silicio LEDC Timer** del ESP32. 

Consumo de CPU: **0%**. La generación de pulsos no depende del bucle `loop()` y nunca pierde un paso.

```cpp
/* ==============================================================================
 * PROYECTO DE TESIS DE INGENIERÍA INDUSTRIAL - PLANTA DE ULTRAFILTRACIÓN FX100
 * HITO 1: CONTROL DE ACCIONAMIENTO DE POTENCIA (NEMA 34 + DM860 + ESP32)
 * ============================================================================== */

#include <Arduino.h>

// 1. DEFINICIÓN DE PINES FÍSICOS
const uint8_t PIN_PUL = 18; // Señal STEP (Pulsos de paso)
const uint8_t PIN_DIR = 19; // Señal DIR (Sentido de giro)
const uint8_t PIN_ENA = 21; // Señal ENA (Opcional - Driver habilitado en LOW)

// 2. CONSTANTES DE CALIBRACIÓN MECATRÓNICA
const uint16_t PULSOS_POR_REV = 1600;   // 1600 pulsos = 1 vuelta completa
const float ACELERACION_RPM_SEG = 35.0; // Rampa suave: 35 RPM por cada segundo

// 3. VARIABLES DE ESTADO DEL MOTOR
bool motorHabilitado = false;
bool sentidoHorario  = true; // true = Horario (Filtración), false = Antihorario (Backwash)
float rpm_objetivo   = 0.0;
float rpm_actual     = 0.0;
unsigned long t_ultimo_calculo_ms = 0;

// 4. FUNCIÓN GENERADORA DE FRECUENCIA POR SILICIO (HARDWARE LEDC)
void actualizarVelocidadHardware(float rpm) {
  if (rpm > 0.5 && motorHabilitado) {
    // Ecuación: f (Hz) = (RPM * 1600) / 60
    float frecuencia_hz = (rpm * (float)PULSOS_POR_REV) / 60.0f;
    ledcWriteTone(PIN_PUL, frecuencia_hz); // Generación pura en silicio por timer
  } else {
    ledcWriteTone(PIN_PUL, 0); // Frecuencia 0 (Motor detenido)
    digitalWrite(PIN_PUL, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_ENA, OUTPUT);

  // Configurar pin ENA en LOW (Habilitado permanente)
  digitalWrite(PIN_ENA, LOW);
  digitalWrite(PIN_DIR, sentidoHorario ? HIGH : LOW);

  // Inicializar periférico de hardware LEDC en GPIO 18
  ledcAttach(PIN_PUL, 1000, 8);
  ledcWriteTone(PIN_PUL, 0);

  Serial.println("\n========================================================");
  Serial.println("  HITO 1: CONTROL DE POTENCIA NEMA 34 + DRIVER DM860    ");
  Serial.println("  Planta Piloto de Ultrafiltración Industrial FX100     ");
  Serial.println("========================================================");
  Serial.println("Configuración actual:");
  Serial.println(" - Pines: PUL = GPIO 18 | DIR = GPIO 19 | ENA = GPIO 21");
  Serial.println(" - Resolución: 1600 pulsos/rev (Micropasos = 8)");
  Serial.println(" - Rampa de Aceleración: 35.0 RPM/s");
  Serial.println("\nComandos disponibles en el Monitor Serie:");
  Serial.println("  'R10'   -> Girar a 10 RPM");
  Serial.println("  'R30'   -> Girar a 30 RPM");
  Serial.println("  'R60'   -> Girar a 60 RPM");
  Serial.println("  'R100'  -> Girar a 100 RPM");
  Serial.println("  'DIR'   -> Invertir Sentido de Giro (Horario / Antihorario)");
  Serial.println("  'STOP'  -> Detener Motor con rampa suave");
  Serial.println("  'STATUS'-> Ver estado actual de telemetría");
  Serial.println("========================================================\n");
}

void loop() {
  // ----------------------------------------------------------------------------
  // A. PARSER DE COMANDOS DEL MONITOR SERIE
  // ----------------------------------------------------------------------------
  if (Serial.available() > 0) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();

    if (comando.startsWith("R") || comando.startsWith("r")) {
      float r = comando.substring(1).toFloat();
      if (r >= 0.0f && r <= 130.0f) {
        rpm_objetivo = r;
        motorHabilitado = (r > 0.0f);
        Serial.printf("[COMANDO] Velocidad fijada a %.1f RPM. Acelerando...\n", rpm_objetivo);
      } else {
        Serial.println("❌ Error: Rango de velocidad permitido: 0 a 120 RPM.");
      }
    } 
    else if (comando.equalsIgnoreCase("DIR")) {
      sentidoHorario = !sentidoHorario;
      digitalWrite(PIN_DIR, sentidoHorario ? HIGH : LOW);
      Serial.printf("[COMANDO] Sentido invertido a: %s\n", 
                    sentidoHorario ? "HORARIO (Filtración hacia FX100)" : "ANTIHORARIO (Retrolavado)");
    } 
    else if (comando.equalsIgnoreCase("STOP")) {
      rpm_objetivo = 0.0f;
      motorHabilitado = false;
      Serial.println("[COMANDO] Desacelerando motor a 0 RPM...");
    } 
    else if (comando.equalsIgnoreCase("STATUS")) {
      Serial.printf("[ESTADO] RPM Actual: %.1f | RPM Objetivo: %.1f | Frecuencia: %u Hz | Sentido: %s\n",
                    rpm_actual, rpm_objetivo, (unsigned int)((rpm_actual * 1600) / 60),
                    sentidoHorario ? "HORARIO" : "ANTIHORARIO");
    }
  }

  // ----------------------------------------------------------------------------
  // B. GESTIÓN TEMPORAL DE LA RAMPA DE ACELERACIÓN SUAVE (Cada 20 ms)
  // ----------------------------------------------------------------------------
  unsigned long t_ahora = millis();
  float dt = (t_ahora - t_ultimo_calculo_ms) / 1000.0f;

  if (dt >= 0.02f) { // 50 Hz de tasa de refresco
    t_ultimo_calculo_ms = t_ahora;

    if (motorHabilitado) {
      if (rpm_actual < rpm_objetivo) {
        rpm_actual += ACELERACION_RPM_SEG * dt;
        if (rpm_actual > rpm_objetivo) rpm_actual = rpm_objetivo;
        actualizarVelocidadHardware(rpm_actual);
      } else if (rpm_actual > rpm_objetivo) {
        rpm_actual -= ACELERACION_RPM_SEG * dt;
        if (rpm_actual < rpm_objetivo) rpm_actual = rpm_objetivo;
        actualizarVelocidadHardware(rpm_actual);
      }
    } else {
      if (rpm_actual > 0.0f) {
        rpm_actual -= (ACELERACION_RPM_SEG * 1.5f) * dt; // Desaceleración 50% más rápida
        if (rpm_actual < 0.0f) rpm_actual = 0.0f;
        actualizarVelocidadHardware(rpm_actual);
      }
    }
  }
}
```

---

### 1.7. Procedimiento Paso a Paso de Puesta en Marcha en el Banco

Seguir estrictamente esta secuencia antes de energizar la planta:

1. **Inspección con Energía Apagada**:
   * Verificar que el transformador esté desenchufado de $220\text{V}$.
   * Verificar la posición física de los 8 microswitches del driver DM860: **SW4 en `OFF`** (obligatorio).
2. **Conexión de la Lógica de Control**:
   * `GPIO 18` del ESP32 al borne `PUL+` del DM860.
   * `GPIO 19` del ESP32 al borne `DIR+` del DM860.
   * `GND` del ESP32 puenteado a `PUL-` y `DIR-` del DM860.
   * Dejar `ENA+` y `ENA-` sin conectar.
3. **Conexión del Bobinado del Motor**:
   * Conectar los cables de la Fase A en `A+` y `A-`.
   * Conectar los cables de la Fase B en `B+` y `B-`.
4. **Conexión de la Alimentación de Potencia**:
   * Conectar los dos cables secundarios de $24\text{ VAC}$ del transformador a los bornes `AC / AC` del DM860.
5. **Carga del Firmware (Elegir según la necesidad)**:
   * 🌟 **Opción Principal y Definitiva (Recomendada para la Tesis)**:  
     Subir el sketch [`bomba/bomba.ino`](./03_Firmware_Control_Bomba/bomba/bomba.ino) desde Arduino IDE.  
     *Incluye Control Inalámbrico Wi-Fi, Servidor Web local (Dashboard táctil en `http://bomba.local` o AP `192.168.4.1`), cálculo de caudal en L/min y litros totales, y actualizaciones OTA.*
   * 🧪 **Opción de Diagnóstico Rápido en Banco (Solo Serial)**:  
     Subir el sketch [`Hito1_ControlMotor/Hito1_ControlMotor.ino`](./03_Firmware_Control_Bomba/Hito1_ControlMotor/Hito1_ControlMotor.ino).  
     *Código mínimo sin Wi-Fi que solo responde por el Monitor Serie a 115200 baudios para verificar giro y pulsos por cable USB.*
6. **Encendido y Secuencia de Prueba**:
   * Enchufar el transformador a $220\text{V}$ (el LED verde del DM860 se encenderá fijo).
   * Si cargaste `bomba.ino`: Conéctate por Wi-Fi y controla la velocidad desde la interfaz táctil.
   * Si cargaste `Hito1_ControlMotor.ino`: Abre el Monitor Serie a 115200 baudios y prueba con comandos `R30`, `DIR`, `R60`, `STOP`.

---

### 1.8. Criterio de Aprobación del Hito 1

El Hito 1 se considera **OFICIALMENTE APROBADO** para el informe de tesis cuando se verifiquen los siguientes 5 puntos en el laboratorio:

- [ ] **Giro Suave y Silencioso**: El motor gira en $10, 30, 60 \text{ y } 100\text{ RPM}$ con emisión sonora mínima y sin vibración de resonancia mecánica en el banco.
- [ ] **Rampa Dinámica Libre de Pérdida de Pasos**: Al pasar bruscamente de 0 a 100 RPM, el motor acelera en forma continua sin cabeceos ni silbidos de pérdida de pasos.
- [ ] **Inversión de Sentido Fiable**: El cambio entre modo Filtración (CW) y Retrolavado (CCW) se ejecuta de forma inmediata y limpia.
- [ ] **Estabilidad Térmica Comprobada**: Tras operar durante $20\text{ minutos}$ continuos a $60\text{ RPM}$ y luego estar en reposo $10\text{ minutos}$, la carcasa del motor NEMA 34 permanece tibia ($< 45^\circ\text{C}$), comprobando que el switch SW4 en `OFF` protege las bobinas.
- [ ] **Torque de Arrastre Efectivo**: El motor vence con total facilidad la resistencia del tubo peristáltico elastomérico comprimido por los rodillos de la bomba MBP-2000.

---

### 📂 Estructura y Navegación de Subcarpetas de este Hito:

* 📁 **[`01_Hardware_y_Cableado/`](./01_Hardware_y_Cableado/)**:
  * 🔌 **[`Guia_Conexionado_Fisico_DM860.md`](./01_Hardware_y_Cableado/Guia_Conexionado_Fisico_DM860.md)**: Manual de taller de conexión pin a pin al driver.
  * ⚡ **[`Guia_Alimentacion_y_Masa_Comun.md`](./01_Hardware_y_Cableado/Guia_Alimentacion_y_Masa_Comun.md)**: Esquema de circuitos AC/DC, masa común y calibración del LM2596 a 5.00V.
  * 🚰 **[`Guia_Ferreteria_e_Hidraulica_Base.md`](./01_Hardware_y_Cableado/Guia_Ferreteria_e_Hidraulica_Base.md)**: Mangueras de silicona, racores G1/4" y espigas.
  * 📦 **[`Inventario_Consolidado.md`](./01_Hardware_y_Cableado/Inventario_Consolidado.md)**: Listado de componentes disponibles y faltantes.
* 📁 **[`02_Montaje_y_Optimizacion_Motor/`](./02_Montaje_y_Optimizacion_Motor/)**:
  * ⚙️ **[`GUIA_OPTIMIZACION_MOTOR_NEMA34_Y_DIP_SWITCHES.md`](./02_Montaje_y_Optimizacion_Motor/GUIA_OPTIMIZACION_MOTOR_NEMA34_Y_DIP_SWITCHES.md)**: Conexión de 8 cables en Bipolar Serie (3A) según hoja técnica oficial de CNC Insumos S.R.L.
  * 📷 `IMG_20251128_112538.jpg` e `IMG_20251128_112640.jpg`: Fotografías originales del fabricante.
* 📁 **[`03_Firmware_Control_Bomba/`](./03_Firmware_Control_Bomba/)**:
  * 💻 **[`bomba/bomba.ino`](./03_Firmware_Control_Bomba/bomba/bomba.ino)**: Firmware oficial de la planta con **Control Inalámbrico Wi-Fi**, Dashboard Web responsivo táctil, cálculo de caudal en L/min y ArduinoOTA.
  * 📖 **[`Guia_Montaje_Bomba_y_Bornera_ESP32.md`](./03_Firmware_Control_Bomba/Guia_Montaje_Bomba_y_Bornera_ESP32.md)**: Manual ilustrado de fijación a tornillo en el shield de borneras.
  * 💻 **[`Hito1_ControlMotor/Hito1_ControlMotor.ino`](./03_Firmware_Control_Bomba/Hito1_ControlMotor/Hito1_ControlMotor.ino)**: Firmware modular de prueba en banco por Monitor Serie (115200 baudios).
* 📁 **[`04_Simulador_Interactivo/`](./04_Simulador_Interactivo/)**:
  * 🎮 **[`simulador_bomba.html`](./04_Simulador_Interactivo/simulador_bomba.html)**: Simulador gráfico interactivo ejecutable en cualquier navegador web.
  * 📄 **[`README_Simulador.md`](./04_Simulador_Interactivo/README_Simulador.md)**: Guía de uso, entregable y lista de verificación del simulador.
