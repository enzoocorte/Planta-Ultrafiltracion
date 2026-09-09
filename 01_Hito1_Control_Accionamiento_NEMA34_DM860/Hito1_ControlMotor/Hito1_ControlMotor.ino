/* ==============================================================================
 * PROYECTO DE TESIS DE INGENIERÍA INDUSTRIAL - PLANTA DE ULTRAFILTRACIÓN FX100
 * FASE 1: ACCIONAMIENTO Y COAGULACIÓN-SEDIMENTACIÓN
 * 🎯 HITO 1: CONTROL DE ACCIONAMIENTO DE POTENCIA (NEMA 34 + DM860 + ESP32)
 * ==============================================================================
 * 
 * DESCRIPCIÓN:
 * Firmware oficial del Hito 1 para el control de velocidad, sentido de giro y rampa
 * de aceleración suave del motor paso a paso NEMA 34 mediante el driver Leadshine DM860.
 * 
 * ESQUEMA DE CABLEADO (ESP32 38 PINES):
 * - GPIO 18 (D18) ──► PUL+ del Driver DM860 (Pulsos STEP)
 * - GPIO 19 (D19) ──► DIR+ del Driver DM860 (Sentido DIR)
 * - GPIO 21 (D21) ──► ENA+ del Driver DM860 (Opcional - Driver habilitado en LOW)
 * - GND           ──► PUL- y DIR- puenteados (Cátodo Común a 3.3V)
 * ============================================================================== */

#include <Arduino.h>

// ------------------------------------------------------------------------------
// 1. DEFINICIÓN DE PINES FÍSICOS
// ------------------------------------------------------------------------------
const uint8_t PIN_PUL = 18; // Señal STEP (Pulsos de paso)
const uint8_t PIN_DIR = 19; // Señal DIR (Sentido de giro)
const uint8_t PIN_ENA = 21; // Señal ENA (Opcional - Driver habilitado en LOW)

// ------------------------------------------------------------------------------
// 2. CONSTANTES DE CALIBRACIÓN MECATRÓNICA
// ------------------------------------------------------------------------------
const uint16_t PULSOS_POR_REV   = 1600; // 1600 pulsos = 1 vuelta completa en DM860
const float ACELERACION_RPM_SEG = 35.0; // Rampa suave: acelera 35 RPM por segundo

// ------------------------------------------------------------------------------
// 3. VARIABLES DINÁMICAS DE ESTADO DEL MOTOR
// ------------------------------------------------------------------------------
bool motorHabilitado = false;
bool sentidoHorario  = true; // true = Horario (Filtración), false = Antihorario (Backwash)

float rpm_objetivo   = 0.0;
float rpm_actual     = 0.0;
unsigned long t_ultimo_calculo_ms = 0;

// ------------------------------------------------------------------------------
// 4. GENERADOR DETERMINÍSTICO DE PULSOS POR HARDWARE LEDC (0% Carga CPU)
// ------------------------------------------------------------------------------
void actualizarVelocidadHardware(float rpm) {
  if (rpm > 0.5 && motorHabilitado) {
    // Ecuación Fundamental: f (Hz) = (RPM * 1600 pulsos) / 60 seg
    float frecuencia_hz = (rpm * (float)PULSOS_POR_REV) / 60.0f;
    ledcWriteTone(PIN_PUL, frecuencia_hz); // Generación pura en silicio por timer
  } else {
    ledcWriteTone(PIN_PUL, 0);             // Frecuencia 0 (Motor detenido)
    digitalWrite(PIN_PUL, LOW);
  }
}

// ------------------------------------------------------------------------------
// 5. SETUP: INICIALIZACIÓN DE HARDWARE Y SERVICIOS
// ------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_ENA, OUTPUT);

  // Driver DM860 habilitado en nivel bajo (LOW)
  digitalWrite(PIN_ENA, LOW);
  digitalWrite(PIN_DIR, sentidoHorario ? HIGH : LOW);

  // Inicializar periférico de hardware LEDC Timer en GPIO 18 (PUL)
  ledcAttach(PIN_PUL, 1000, 8);
  ledcWriteTone(PIN_PUL, 0);

  Serial.println("\n========================================================");
  Serial.println("  FASE 1: ACCIONAMIENTO Y COAGULACIÓN-SEDIMENTACIÓN     ");
  Serial.println("  HITO 1: CONTROL DE POTENCIA NEMA 34 + DRIVER DM860    ");
  Serial.println("========================================================");
  Serial.println("Configuración actual:");
  Serial.println(" - Pines: PUL = GPIO 18 | DIR = GPIO 19 | ENA = GPIO 21");
  Serial.println(" - Resolución: 1600 pulsos/rev (Micropasos = 8)");
  Serial.println(" - Rampa de Aceleración: 35.0 RPM/s");
  Serial.println("\nComandos disponibles en el Monitor Serie:");
  Serial.println("  'R10'    -> Girar a 10 RPM (Flujo bajo)");
  Serial.println("  'R30'    -> Girar a 30 RPM (Flujo nominal 126 mL/min)");
  Serial.println("  'R60'    -> Girar a 60 RPM (Flujo estándar 252 mL/min)");
  Serial.println("  'R100'   -> Girar a 100 RPM (Flujo alto 420 mL/min)");
  Serial.println("  'DIR'    -> Invertir Sentido de Giro (Horario / Antihorario)");
  Serial.println("  'STOP'   -> Detener Motor con rampa suave");
  Serial.println("  'STATUS' -> Ver estado instantáneo del sistema");
  Serial.println("========================================================\n");
}

// ------------------------------------------------------------------------------
// 6. LOOP PRINCIPAL: PARSER SERIE Y RAMPA DINÁMICA
// ------------------------------------------------------------------------------
void loop() {
  // ----------------------------------------------------------------------------
  // A. PARSER DE COMANDOS POR PUERTO SERIE (115200 Baudios)
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
                    sentidoHorario ? "HORARIO (Filtración)" : "ANTIHORARIO (Retrolavado)");
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
