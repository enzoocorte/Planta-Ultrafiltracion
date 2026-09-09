/*
 * ===================================================================
 * HITO 1: CONTROL DE BOMBA PERISTÁLTICA (NEMA 34 + DM860)
 * ===================================================================
 * Planta Piloto de Ultrafiltración - Tesis de Ingeniería
 * Microcontrolador: ESP32 DevKit V1 (38 pines)
 * Driver: DM860 (1600 pulsos/rev) - Cátodo Común
 * Pines: PUL=25, DIR=26, ENA=27
 * Comunicación: Bluetooth Classic ("Bomba_Filtro_ESP32") + Serial USB 115200
 * ===================================================================
 */

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth no habilitado! En Arduino IDE: Tools > Partition Scheme con soporte BT.
#endif

BluetoothSerial SerialBT;

// DEFINICIÓN DE PINES (Coincide 100% con tu cableado actual)
const uint8_t PIN_PUL = 25;  // Pulso / STEP
const uint8_t PIN_DIR = 26;  // Dirección
const uint8_t PIN_ENA = 27;  // Enable del driver

const uint16_t PULSOS_POR_REV = 1600;

// Variables de Estado y Dinámica
bool bombaEncendida = false;
bool direccionActual = HIGH; // HIGH = Horario (Filtración), LOW = Antihorario (Retrolavado)

float rpm_objetivo = 0.0;
float rpm_actual = 0.0;
const float ACELERACION_RPM_SEG = 35.0; // Rampa: sube/baja 35 RPM por segundo para evitar pérdida de pasos

unsigned long t_ultimo_pulso_us = 0;
unsigned long t_ultima_rampa_ms = 0;
bool estado_pin_pul = LOW;
const unsigned int ANCHO_PULSO_US = 6; // 6 microsegundos de ancho de pulso seguro para DM860

void procesarComandoTexto(String cmd);

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_PUL, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_ENA, OUTPUT);

  // Seguridad: Iniciar con driver deshabilitado (sin corriente a las bobinas del NEMA 34)
  digitalWrite(PIN_ENA, LOW);
  digitalWrite(PIN_PUL, LOW);
  digitalWrite(PIN_DIR, direccionActual);

  // Iniciar Bluetooth Classic
  SerialBT.begin("Bomba_Filtro_ESP32");

  Serial.println("==================================================");
  Serial.println("--- HITO 1: CONTROL NEMA 34 (RPM EXACTAS + RAMPAS) ---");
  Serial.println("Bluetooth listo como: Bomba_Filtro_ESP32");
  Serial.println("Comandos validos:");
  Serial.println("  '1'         -> Encender bomba a velocidad fijada");
  Serial.println("  '0'         -> Apagar bomba con rampa suave");
  Serial.println("  'D' o 'd'   -> Invertir sentido de giro");
  Serial.println("  'RPM 30'    -> Fijar velocidad a 30 RPM exactas");
  Serial.println("  'RPM 60'    -> Fijar velocidad a 60 RPM exactas");
  Serial.println("  '+' / '-'   -> Incrementar / Decrementar 5 RPM");
  Serial.println("==================================================");
}

void loop() {
  // 1. Lectura por Bluetooth
  if (SerialBT.available()) {
    String cmdBT = SerialBT.readStringUntil('\n');
    cmdBT.trim();
    procesarComandoTexto(cmdBT);
  }

  // 2. Lectura por Serial USB (PC)
  if (Serial.available()) {
    String cmdUSB = Serial.readStringUntil('\n');
    cmdUSB.trim();
    procesarComandoTexto(cmdUSB);
  }

  // 3. Generador de Rampa de Aceleración (calculado cada 20 ms)
  unsigned long ahora_ms = millis();
  if (ahora_ms - t_ultima_rampa_ms >= 20) {
    float dt = (ahora_ms - t_ultima_rampa_ms) / 1000.0;
    t_ultima_rampa_ms = ahora_ms;

    if (bombaEncendida) {
      if (rpm_actual < rpm_objetivo) {
        rpm_actual += ACELERACION_RPM_SEG * dt;
        if (rpm_actual > rpm_objetivo) rpm_actual = rpm_objetivo;
      } else if (rpm_actual > rpm_objetivo) {
        rpm_actual -= ACELERACION_RPM_SEG * dt;
        if (rpm_actual < rpm_objetivo) rpm_actual = rpm_objetivo;
      }
    } else {
      // Frenado suave por rampa
      if (rpm_actual > 0.0) {
        rpm_actual -= ACELERACION_RPM_SEG * dt * 1.5;
        if (rpm_actual <= 0.0) {
          rpm_actual = 0.0;
          digitalWrite(PIN_ENA, LOW); // Apaga el driver al detenerse por completo
        }
      }
    }
  }

  // 4. Generación de Pulsos No Bloqueante (control por período)
  if (rpm_actual > 0.5) {
    // Ecuación: T_periodo = (60 / (RPM * 1600)) * 10^6 = 37500 / RPM
    unsigned long intervalo_us = (unsigned long)(37500.0 / rpm_actual);
    unsigned long ahora_us = micros();

    if (estado_pin_pul == LOW) {
      if (ahora_us - t_ultimo_pulso_us >= intervalo_us) {
        digitalWrite(PIN_PUL, HIGH);
        estado_pin_pul = HIGH;
        t_ultimo_pulso_us = ahora_us;
      }
    } else {
      if (ahora_us - t_ultimo_pulso_us >= ANCHO_PULSO_US) {
        digitalWrite(PIN_PUL, LOW);
        estado_pin_pul = LOW;
        t_ultimo_pulso_us = ahora_us;
      }
    }
  }
}

void procesarComandoTexto(String cmd) {
  if (cmd.length() == 0) return;

  if (cmd == "1") {
    if (rpm_objetivo < 1.0) rpm_objetivo = 30.0; // Velocidad de arranque segura: 30 RPM
    digitalWrite(PIN_ENA, HIGH);
    bombaEncendida = true;
    SerialBT.printf("Bomba ON -> Acelerando suavemente a %.1f RPM\n", rpm_objetivo);
    Serial.printf("Bomba ON -> Acelerando suavemente a %.1f RPM\n", rpm_objetivo);
  } 
  else if (cmd == "0") {
    bombaEncendida = false;
    SerialBT.println("Bomba OFF -> Desacelerando con rampa...");
    Serial.println("Bomba OFF -> Desacelerando con rampa...");
  } 
  else if (cmd.equalsIgnoreCase("D")) {
    direccionActual = !direccionActual;
    digitalWrite(PIN_DIR, direccionActual);
    String txt = direccionActual ? "Sentido: HORARIO (Filtracion)" : "Sentido: ANTIHORARIO (Retrolavado)";
    SerialBT.println(txt);
    Serial.println(txt);
  } 
  else if (cmd.startsWith("RPM") || cmd.startsWith("rpm")) {
    float r = cmd.substring(3).toFloat();
    if (r >= 0.0 && r <= 150.0) {
      rpm_objetivo = r;
      if (r > 0.0 && !bombaEncendida) {
        digitalWrite(PIN_ENA, HIGH);
        bombaEncendida = true;
      }
      SerialBT.printf("RPM Objetivo: %.1f RPM\n", rpm_objetivo);
      Serial.printf("RPM Objetivo: %.1f RPM\n", rpm_objetivo);
    } else {
      SerialBT.println("Error: Rango de velocidad permitido de 0 a 150 RPM");
    }
  } 
  else if (cmd == "+") {
    rpm_objetivo += 5.0;
    if (rpm_objetivo > 150.0) rpm_objetivo = 150.0;
    SerialBT.printf("RPM +5 -> %.1f RPM\n", rpm_objetivo);
    Serial.printf("RPM +5 -> %.1f RPM\n", rpm_objetivo);
  } 
  else if (cmd == "-") {
    rpm_objetivo -= 5.0;
    if (rpm_objetivo < 0.0) rpm_objetivo = 0.0;
    SerialBT.printf("RPM -5 -> %.1f RPM\n", rpm_objetivo);
    Serial.printf("RPM -5 -> %.1f RPM\n", rpm_objetivo);
  }
}
