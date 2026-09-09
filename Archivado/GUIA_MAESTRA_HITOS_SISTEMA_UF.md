# GUÍA MAESTRA DE IMPLEMENTACIÓN POR HITOS: SISTEMA UF INDUSTRIAL V2
## Manual Paso a Paso para Desarrollo, Validación y Documentación de Tesis
### Planta Piloto de Coagulación-Sedimentación y Ultrafiltración (FX100) | ESP32 DevKit V1

---

### 📌 Índice de Hitos del Proyecto

```
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                    PLAN DE HITOS MODULARES                                       │
├────────┬────────────────────────────────────────────────────────┬────────────────────────────────┤
│ Hito   │ Título / Módulo                                        │ Concepto Clave                 │
├────────┼────────────────────────────────────────────────────────┼────────────────────────────────┤
│ Hito 1 │ Control del Motor NEMA 34 con Driver DM860             │ Pulsos STEP/DIR, Frecuencia/RPM│
│ Hito 2 │ Medición de RPM de Paleta Agitadora con Sensor FC-03   │ Interrupciones de Hardware     │
│ Hito 3 │ Máquina de Estados del Reactor de Sedimentación        │ FSM, Temporización No Bloqueo  │
│ Hito 4 │ Lectura de Presión y TMP con Conversor I2C ADS1115     │ ADC 16-bit, Calibración kPa    │
│ Hito 5 │ Medición de Micro-Caudal y Flujo de Membrana (YF-S401) │ Conteo de Pulsos, Ley de Darcy │
│ Hito 6 │ Lazo de Control PID y Protección Estricta del FX100    │ Lazo Cerrado, Alivio TMP < 0.5 │
│ Hito 7 │ FSM Integral de Ultrafiltración (Filtración + Backwash)│ Automatización de Válvulas     │
│ Hito 8 │ Telemetría IoT, Dashboard Web y Exportación de Datos   │ WebSockets, Grafana, CSV Tesis │
└────────┴────────────────────────────────────────────────────────┴────────────────────────────────┘
```

---

## 🟢 FASE 1: ACCIONAMIENTO Y COAGULACIÓN-SEDIMENTACIÓN

---

### 🎯 HITO 1: Control de Accionamiento de Potencia (NEMA 34 + DM860 + ESP32)

#### 1.1. Objetivo
Lograr el control preciso y suave del motor paso a paso **NEMA 34** ($4\text{ Nm}$, $6\text{ A}$) acoplado a la bomba peristáltica **MBP-2000**, fijando las RPM deseadas con rampas de aceleración para evitar pérdida de pasos por inercia.

#### 1.2. Componentes Necesarios
* ESP32 DevKit V1 ($3.3\text{V}$).
* Driver digital DM860 (alimentado a $34\text{ VDC}$ desde el transformador $220\text{V} \rightarrow 24\text{ VAC}$ $150\text{ VA}$).
* Motor NEMA 34 ($6\text{ A}$, 4 cables).
* Cables de conexión dupont / bornera.

#### 1.3. Esquema de Cableado Pin a Pin
```
   ESP32 DevKit V1                       Driver Digital DM860
  ┌────────────────┐                   ┌───────────────────────┐
  │        GPIO 18 ├───────────────────┤ PUL+ (PULSE / STEP)   │
  │        GPIO 19 ├───────────────────┤ DIR+ (DIRECCIÓN)      │
  │        GPIO 21 ├───────────────────┤ ENA+ (ENABLE)         │
  │            GND ├───────────────────┤ PUL-, DIR-, ENA- (GND)│
  └────────────────┘                   └───────────────────────┘
                                       ┌───────────────────────┐
   Transformador 24 VAC                │ VCC / AC+  (24-80 VDC)│
   (150 VA - Bus 34V DC) ──────────────┤ GND / AC-             │
                                       ├───────────────────────┤
   Motor NEMA 34 (6A)                  │ A+ , A-  (Fase A)     │
   (Bobinado Bipolar)   ──────────────┤ B+ , B-  (Fase B)     │
                                       └───────────────────────┘
```

#### 1.4. Fundamento Matemático (Frecuencia de Pulsos a RPM)
Si el driver DM860 está configurado en $1600\text{ pulsos/revolución}$ (micropasos $= 8$):
$$f_{\text{pulsos}} [\text{Hz}] = \frac{\text{RPM} \times \text{PPR}}{60} = \frac{\text{RPM} \times 1600}{60} = \text{RPM} \times 26.667$$
* Ejemplo: Para girar a $60\text{ RPM}$, el ESP32 debe emitir una frecuencia de $1600\text{ Hz}$ ($T_{\text{periodo}} = 625\,\mu\text{s}$).

#### 1.5. Procedimiento Paso a Paso
1. **Configuración de Dip Switches en el DM860**:
   * SW1-SW3: Configurar corriente nominal en $6.0\text{ A}$ (Pico $7.2\text{ A}$).
   * SW4: `OFF` (Standstill Current al $50\%$ para evitar calentamiento del motor en reposo).
   * SW5-SW8: Configurar resolución en $1600\text{ pulsos/rev}$.
2. **Conectar la lógica a $3.3\text{V}$ del ESP32**.
3. **Cargar el código modular del Hito 1 en el ESP32**.
4. **Verificación**: Abrir el monitor serie a 115200 baudios y enviar comandos de velocidad (ej. `RPM:30`, `RPM:60`, `DIR:CW`, `DIR:CCW`, `STOP`).

#### 1.6. Código Modular del Hito 1 (`Hito1_ControlMotor.ino`)
```cpp
// HITO 1: Control de Motor NEMA 34 + DM860 con ESP32
#include <Arduino.h>

const uint8_t PIN_PUL = 18;
const uint8_t PIN_DIR = 19;
const uint8_t PIN_ENA = 21;

const uint16_t PULSOS_POR_REV = 1600; // Configurado en DM860
float rpm_objetivo = 0.0;
bool sentido_horario = true;

// Tarea generadora de pulsos usando Hardware Timer o DelayMicroseconds
void setMotorSpeed(float rpm, bool dir) {
  sentido_horario = dir;
  digitalWrite(PIN_DIR, dir ? HIGH : LOW);
  rpm_objetivo = rpm;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PUL, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_ENA, OUTPUT);

  digitalWrite(PIN_ENA, LOW); // LOW = Habilitar driver en DM860
  setMotorSpeed(0, true);
  Serial.println("--- HITO 1 INICIADO: Control Motor NEMA 34 ---");
  Serial.println("Comandos: 'R30' (30 RPM), 'R60' (60 RPM), 'DIR', 'STOP'");
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.startsWith("R")) {
      float r = cmd.substring(1).toFloat();
      setMotorSpeed(r, sentido_horario);
      Serial.printf("Velocidad fijada: %.1f RPM\n", r);
    } else if (cmd == "DIR") {
      setMotorSpeed(rpm_objetivo, !sentido_horario);
      Serial.printf("Sentido cambiado: %s\n", sentido_horario ? "CW" : "CCW");
    } else if (cmd == "STOP") {
      setMotorSpeed(0, sentido_horario);
      Serial.println("Motor Detenido");
    }
  }

  // Generación de tren de pulsos no bloqueante si RPM > 0
  if (rpm_objetivo > 0.1) {
    float freq_hz = (rpm_objetivo * PULSOS_POR_REV) / 60.0;
    uint32_t delay_us = (uint32_t)(1000000.0 / (2.0 * freq_hz));
    
    digitalWrite(PIN_PUL, HIGH);
    delayMicroseconds(delay_us);
    digitalWrite(PIN_PUL, LOW);
    delayMicroseconds(delay_us);
  }
}
```

#### 1.7. Criterio de Aprobación del Hito 1
* [ ] El motor gira suavemente en $10$, $30$, $60$ y $100\text{ RPM}$ sin vibraciones excesivas ni saltos.
* [ ] El sentido de giro cambia limpiamente sin trabarse.
* [ ] Al enviar `STOP`, el motor se frena y el driver pasa a reposo térmico ($50\%$ corriente).

---

### 🎯 HITO 2: Medición de RPM con Sensor Óptico (`FC-03` / Interrupciones)

#### 2.1. Objetivo
Medir con precisión de laboratorio la velocidad real de rotación ($\text{RPM}$) de la paleta agitadora o del eje de la bomba peristáltica mediante interrupciones de hardware del ESP32.

#### 2.2. Componentes Necesarios
* Módulo Sensor Óptico de Ranura **FC-03** (con comparador LM393).
* Disco ranurado de 20 divisiones acoplado al eje.
* ESP32 DevKit V1.

#### 2.3. Esquema de Cableado
```
   Sensor Óptico FC-03                  ESP32 DevKit V1
  ┌──────────────────┐                ┌────────────────┐
  │              VCC ├────────────────┤ 3.3V (o 5V)    │
  │              GND ├────────────────┤ GND            │
  │     D0 (Digital) ├────────────────┤ GPIO 23        │
  └──────────────────┘                └────────────────┘
```

#### 2.4. Fundamento de Interrupciones de Hardware
Cada vez que una ranura del disco cruza el haz infrarrojo del FC-03, la salida `D0` pasa de `LOW` a `HIGH`. El ESP32 captura este evento por hardware en el pin `GPIO 23` mediante la función de interrupción `attachInterrupt()`.

$$\text{RPM} = \left(\frac{\text{Pulsos contabilizados en } \Delta t\;[\text{seg}]}{\text{N° de ranuras } (20)}\right) \times \left(\frac{60}{\Delta t}\right)$$

#### 2.5. Código Modular del Hito 2 (`Hito2_MedicionRPM.ino`)
```cpp
// HITO 2: Medición de RPM con Sensor Óptico FC-03 e Interrupciones
#include <Arduino.h>

const uint8_t PIN_SENSOR_RPM = 23;
const uint8_t RANURAS_DISCO = 20;

volatile uint32_t contador_pulsos = 0;
uint32_t tiempo_anterior = 0;
float rpm_actual = 0.0;

// Rutina de Servicio de Interrupción (ISR) en IRAM
void IRAM_ATTR isr_contador_pulsos() {
  contador_pulsos++;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_SENSOR_RPM, INPUT_PULLUP);
  
  // Vincular interrupción en flanco de subida
  attachInterrupt(digitalPinToInterrupt(PIN_SENSOR_RPM), isr_contador_pulsos, RISING);
  tiempo_anterior = millis();
  
  Serial.println("--- HITO 2 INICIADO: Medidor de RPM por Interrupciones ---");
}

void loop() {
  uint32_t tiempo_actual = millis();
  
  // Cálculo cada 1 segundo (1000 ms)
  if (tiempo_actual - tiempo_anterior >= 1000) {
    // Deshabilitar interrupciones momentáneamente para lectura atómica
    noInterrupts();
    uint32_t pulsos = contador_pulsos;
    contador_pulsos = 0;
    interrupts();

    float delta_t_seg = (tiempo_actual - tiempo_anterior) / 1000.0;
    rpm_actual = ((float)pulsos / RANURAS_DISCO) * (60.0 / delta_t_seg);
    tiempo_anterior = tiempo_actual;

    Serial.printf("[TELEMETRÍA] Pulsos/seg: %lu | RPM Medidas: %.2f RPM\n", pulsos, rpm_actual);
  }
}
```

#### 2.6. Criterio de Aprobación del Hito 2
* [ ] Girar el eje manualmente o con motor y verificar que las RPM calculadas coincidan con la velocidad real.
* [ ] No existen lecturas espurias ni rebotes fantasma gracias al comparador LM393 del FC-03.

---

### 🎯 HITO 3: Máquina de Estados del Reactor de Sedimentación

#### 3.1. Objetivo
Programar la secuencia automática de tratamiento de agua turbia con coagulantes naturales (*Opuntia ficus-indica* / *Moringa*), transitando por **Mezcla Rápida**, **Mezcla Lenta** y **Sedimentación Estática**.

#### 3.2. Parámetros del Ciclo de Sedimentación
* **Estado 1: MEZCLA RÁPIDA (Coagulación)**: $150\text{ RPM}$ durante $2\text{ minutos}$ ($120\text{ s}$).
* **Estado 2: MEZCLA LENTA (Floculación)**: $30\text{ RPM}$ durante $20\text{ minutos}$ ($1200\text{ s}$).
* **Estado 3: SEDIMENTACIÓN ESTÁTICA**: $0\text{ RPM}$ durante $40\text{ minutos}$ ($2400\text{ s}$).
* **Estado 4: LISTO_PARA_FILTRAR**: Emite señal y pasa el testigo al módulo de ultrafiltración.

#### 3.3. Código Modular del Hito 3 (`Hito3_FSM_Sedimentador.ino`)
```cpp
// HITO 3: Máquina de Estados Finitos (FSM) para Coagulación-Sedimentación
#include <Arduino.h>

enum EstadoSedimentador {
  IDLE,
  MEZCLA_RAPIDA,
  MEZCLA_LENTA,
  SEDIMENTACION_ESTATICA,
  LISTO_PARA_FILTRAR,
  PARADA_EMERGENCIA
};

EstadoSedimentador estado_actual = IDLE;
uint32_t t_inicio_estado = 0;

// Tiempos en segundos (escalables para pruebas rápidas)
const uint32_t T_MEZCLA_RAPIDA_SEG = 120;     // 2 minutos (150 RPM)
const uint32_t T_MEZCLA_LENTA_SEG  = 1200;    // 20 minutos (30 RPM)
const uint32_t T_SEDIMENTACION_SEG = 2400;    // 40 minutos (0 RPM)

const uint8_t PIN_MOTOR_PALETA_PWM = 4; // Control PWM a MOSFET
const uint8_t PIN_LED_ESTADO = 2;

void cambiarEstado(EstadoSedimentador nuevo) {
  estado_actual = nuevo;
  t_inicio_estado = millis();
  
  switch (estado_actual) {
    case IDLE:
      analogWrite(PIN_MOTOR_PALETA_PWM, 0);
      Serial.println("[FSM] -> Estado: IDLE (En espera de inicio)");
      break;
    case MEZCLA_RAPIDA:
      analogWrite(PIN_MOTOR_PALETA_PWM, 255); // 100% PWM (~150 RPM)
      Serial.println("[FSM] -> Estado: MEZCLA RÁPIDA (Dispersión de Coagulante a 150 RPM)");
      break;
    case MEZCLA_LENTA:
      analogWrite(PIN_MOTOR_PALETA_PWM, 60);  // ~25% PWM (~30 RPM)
      Serial.println("[FSM] -> Estado: MEZCLA LENTA (Floculación suave a 30 RPM)");
      break;
    case SEDIMENTACION_ESTATICA:
      analogWrite(PIN_MOTOR_PALETA_PWM, 0);   // Motor apagado
      Serial.println("[FSM] -> Estado: SEDIMENTACIÓN (Decantación gravitacional a 0 RPM)");
      break;
    case LISTO_PARA_FILTRAR:
      analogWrite(PIN_MOTOR_PALETA_PWM, 0);
      Serial.println("[FSM] -> Estado: LISTO PARA FILTRAR (Sobrenadante clarificado)");
      break;
    case PARADA_EMERGENCIA:
      analogWrite(PIN_MOTOR_PALETA_PWM, 0);
      Serial.println("[FSM] -> ALERTA: PARADA DE EMERGENCIA");
      break;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_MOTOR_PALETA_PWM, OUTPUT);
  pinMode(PIN_LED_ESTADO, OUTPUT);
  cambiarEstado(IDLE);
  Serial.println("Comandos: 'START' para iniciar lote, 'EMERGENCY' para abortar");
}

void loop() {
  if (Serial.available()) {
    String c = Serial.readStringUntil('\n');
    c.trim();
    if (c == "START" && estado_actual == IDLE) cambiarEstado(MEZCLA_RAPIDA);
    if (c == "EMERGENCY") cambiarEstado(PARADA_EMERGENCIA);
    if (c == "RESET") cambiarEstado(IDLE);
  }

  uint32_t t_transcurrido_seg = (millis() - t_inicio_estado) / 1000;

  switch (estado_actual) {
    case MEZCLA_RAPIDA:
      if (t_transcurrido_seg >= T_MEZCLA_RAPIDA_SEG) cambiarEstado(MEZCLA_LENTA);
      break;
    case MEZCLA_LENTA:
      if (t_transcurrido_seg >= T_MEZCLA_LENTA_SEG) cambiarEstado(SEDIMENTACION_ESTATICA);
      break;
    case SEDIMENTACION_ESTATICA:
      if (t_transcurrido_seg >= T_SEDIMENTACION_SEG) cambiarEstado(LISTO_PARA_FILTRAR);
      break;
    default:
      break;
  }
}
```

#### 3.4. Criterio de Aprobación del Hito 3
* [ ] La FSM transita automáticamente entre los 3 estados en el tiempo previsto.
* [ ] No hay demoras bloqueantes (`delay()`); el ESP32 sigue respondiendo a comandos de consola o sensores durante todo el ciclo.

---

## 🟡 FASE 2: INSTRUMENTACIÓN Y MEDIDAS ANALÓGICAS

---

### 🎯 HITO 4: Lectura de Presión y Cálculo de TMP con `ADS1115` (16 Bits)

#### 4.1. Objetivo
Leer los 3 transductores de presión ($P_{\text{feed}}$, $P_{\text{concentrado}}$, $P_{\text{permeado}}$) y el sensor de turbidez mediante el convertidor I2C de 16 bits **ADS1115**, calculando la **Presión Transmembrana (TMP)** en tiempo real.

#### 4.2. Esquema I2C
```
   ESP32 DevKit V1                       Módulo I2C ADS1115 (16-Bit)
  ┌────────────────┐                   ┌─────────────────────────────┐
  │        GPIO 22 ├───────────────────┤ SCL                         │
  │        GPIO 21 ├───────────────────┤ SDA                         │
  │           3.3V ├───────────────────┤ VDD                         │
  │            GND ├───────────────────┤ GND / ADDR                  │
  └────────────────┘                   │                             │
   Transductor P1 (Entrada 0-1 bar) ──┤ A0                          │
   Transductor P2 (Concentrado)    ──┤ A1                          │
   Transductor P3 (Permeado)       ──┤ A2                          │
   Sensor Turbidez (0-3000 NTU)    ──┤ A3                          │
                                       └─────────────────────────────┘
```

#### 4.3. Código Modular del Hito 4 (`Hito4_LecturaPresionTMP.ino`)
```cpp
// HITO 4: Lectura de Presión de Precisión y Cálculo de TMP con ADS1115
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;

// Factores de conversión: Transductor 0-1 bar con salida 0.5V a 4.5V
// 0.5V = 0.0 bar (0 kPa) | 4.5V = 1.0 bar (100 kPa)
float voltajeAPresionKpa(float voltios) {
  if (voltios < 0.40) return 0.0; // Desconectado / Ruido
  float p_kpa = ((voltios - 0.50) / (4.50 - 0.50)) * 100.0;
  return (p_kpa < 0.0) ? 0.0 : p_kpa;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA, SCL en ESP32

  if (!ads.begin()) {
    Serial.println("ERROR: No se encontró el módulo ADS1115. Revisar cables I2C.");
    while (1);
  }
  
  // Ganancia 1x: Rango +/- 4.096V (1 bit = 0.125 mV)
  ads.setGain(GAIN_ONE);
  Serial.println("--- HITO 4 INICIADO: Adquisición ADS1115 (16 Bits) ---");
}

void loop() {
  int16_t adc0 = ads.readADC_SingleEnded(0); // P1 Feed
  int16_t adc1 = ads.readADC_SingleEnded(1); // P2 Concentrado
  int16_t adc2 = ads.readADC_SingleEnded(2); // P3 Permeado
  int16_t adc3 = ads.readADC_SingleEnded(3); // Turbidez

  // Conversión a voltios (Multiplicador de 0.125 mV/cuenta)
  float v0 = ads.computeVolts(adc0);
  float v1 = ads.computeVolts(adc1);
  float v2 = ads.computeVolts(adc2);
  float v3 = ads.computeVolts(adc3);

  float p_feed_kpa = voltajeAPresionKpa(v0);
  float p_conc_kpa = voltajeAPresionKpa(v1);
  float p_perm_kpa = voltajeAPresionKpa(v2);

  // Ecuación de Presión Transmembrana (TMP)
  float tmp_kpa = ((p_feed_kpa + p_conc_kpa) / 2.0) - p_perm_kpa;
  float tmp_atm = tmp_kpa / 101.325;

  Serial.println("--------------------------------------------------");
  Serial.printf("P1 (Feed): %.2f kPa | P2 (Conc): %.2f kPa | P3 (Perm): %.2f kPa\n", p_feed_kpa, p_conc_kpa, p_perm_kpa);
  Serial.printf(">>> TMP CALCULADA: %.2f kPa (%.3f atm) <<<\n", tmp_kpa, tmp_atm);

  // Alerta de seguridad del filtro FX100
  if (tmp_atm >= 0.45) {
    Serial.println("⚠️ ALERTA CRÍTICA: TMP SUPERÓ EL 90% DEL LÍMITE DEL FX100 (0.5 atm)!");
  }

  delay(1000);
}
```

#### 4.4. Criterio de Aprobación del Hito 4
* [ ] El ADS1115 responde de forma estable por I2C.
* [ ] Las presiones $P_1, P_2, P_3$ se calculan sin fluctuaciones gracias a los 16 bits de resolución.
* [ ] La fórmula de TMP entrega valores consistentes con la física hidráulica.

---

### 🎯 HITO 5: Medición de Micro-Caudal (`YF-S401`) y Flujo de Membrana ($J$)

#### 5.1. Objetivo
Medir el caudal de permeado $Q$ en $\text{mL/min}$ con el micro-caudalímetro `YF-S401` y calcular el flujo volumétrico normalizado $J$ en $\text{LMH}$ ($A_m = 2.2\text{ m}^2$).

#### 5.2. Código Modular del Hito 5 (`Hito5_MicroCaudal.ino`)
```cpp
// HITO 5: Medición de Micro-Caudal con YF-S401 y Flujo de Membrana J (LMH)
#include <Arduino.h>

const uint8_t PIN_CAUDAL = 27;
const float AREA_MEMBRANA_M2 = 2.2; // Filtro FX100 (2.2 m2)
const float FACTOR_CALIBRACION_PULSOS_L = 5880.0; // YF-S401 (~98 Hz por L/min)

volatile uint32_t pulsos_caudal = 0;
uint32_t t_ant = 0;
float volumen_acumulado_litros = 0.0;

void IRAM_ATTR isr_caudal() {
  pulsos_caudal++;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_CAUDAL, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_CAUDAL), isr_caudal, RISING);
  t_ant = millis();
  Serial.println("--- HITO 5 INICIADO: Medidor de Micro-Caudal y Flujo J ---");
}

void loop() {
  uint32_t t_act = millis();
  if (t_act - t_ant >= 1000) {
    noInterrupts();
    uint32_t p = pulsos_caudal;
    pulsos_caudal = 0;
    interrupts();

    float dt_seg = (t_act - t_ant) / 1000.0;
    t_ant = t_act;

    // Caudal en L/min y mL/min
    float caudal_l_min = (p / FACTOR_CALIBRACION_PULSOS_L) * (60.0 / dt_seg);
    float caudal_ml_min = caudal_l_min * 1000.0;

    // Volumen acumulado
    volumen_acumulado_litros += (p / FACTOR_CALIBRACION_PULSOS_L);

    // Flujo específico J [LMH = L/(m2 * h)]
    float caudal_l_h = caudal_l_min * 60.0;
    float flujo_j_lmh = caudal_l_h / AREA_MEMBRANA_M2;

    Serial.printf("[HIDRÁULICA] Q: %.1f mL/min | Flujo J: %.2f LMH | Vol Total: %.3f L\n", 
                  caudal_ml_min, flujo_j_lmh, volumen_acumulado_litros);
  }
}
```

---

## 🔴 FASE 3: INTEGRACIÓN, CONTROL PID Y PLANTA COMPLETA

---

### 🎯 HITO 6: Lazo de Control PID y Protección Estricta del FX100

#### 6.1. Objetivo
Implementar un controlador **PID** que ajuste automáticamente los RPM del motor NEMA 34 para mantener constante el caudal de permeado ($Q_{\text{set}} = 300\text{ mL/min}$), incorporando el corte de seguridad si $\text{TMP} \ge 0.50\text{ atm}$.

#### 6.2. Criterios de Aceptación del Hito 6
* [ ] Si la viscosidad o resistencia de membrana aumenta, el PID sube suavemente los RPM para mantener el caudal.
* [ ] Si la $\text{TMP}$ roza los $0.48\text{ atm}$, el supervisor desacelera inmediatamente la bomba para no sobrepasar nunca los $0.50\text{ atm}$.

---

### 🎯 HITO 7: Máquina de Estados Integral de Ultrafiltración

#### 7.1. Objetivo
Automatizar las electroválvulas y el sentido de giro de la bomba para ejecutar ciclos de:
1. **Filtración Normal** ($30\text{ min}$)
2. **Forward Flush** ($20\text{ s}$ purga abierta al 100% velocidad)
3. **Backwash** ($45\text{ s}$ inyección inversa con bomba invertida)

---

### 🎯 HITO 8: Telemetría IoT, Dashboard Web y Exportación de Datos para la Tesis

#### 8.1. Objetivo
Desplegar el panel web interactivo en el ESP32 (`http://domotica-esp32.local`) mostrando gráficos en tiempo real de **Caudal ($Q$)**, **$\text{TMP}$**, **Turbidez** y botón para descargar el archivo **`ensayo_filtracion.csv`** listo para abrir en Python/Excel y generar los gráficos del capítulo experimental de la tesis.

---

### 📊 Planilla de Verificación y Bitácora de Tesis

| Hito | Fecha Planificada | Fecha de Aprobación | Firma Director de Tesis | Observaciones Experimentales |
| :--- | :--- | :--- | :--- | :--- |
| **Hito 1** | Pendiente | - | - | - |
| **Hito 2** | Pendiente | - | - | - |
| **Hito 3** | Pendiente | - | - | - |
| **Hito 4** | Pendiente | - | - | - |
| **Hito 5** | Pendiente | - | - | - |
| **Hito 6** | Pendiente | - | - | - |
| **Hito 7** | Pendiente | - | - | - |
| **Hito 8** | Pendiente | - | - | - |

---
*Guía Maestra de Desarrollo Experimental. Sistema de Ultrafiltración V2 - Versión 1.0.0.*
