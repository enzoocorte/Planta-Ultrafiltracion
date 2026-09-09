/* ==============================================================================
 * PROYECTO DE TESIS DE INGENIERÍA INDUSTRIAL - PLANTA DE ULTRAFILTRACIÓN FX100
 * HITO 4: CONTROL DE AGITADOR (L298N) & SEGURIDAD POR BOYA DE NIVEL FLOTANTE
 * ==============================================================================
 * 
 * DESCRIPCIÓN:
 * Este firmware controla el motor DC de la paleta agitadora del sedimentador
 * mediante modulación PWM en el driver L298N y ejecuta la Máquina de Estados (FSM)
 * del ensayo de jarras (Mezcla Rápida -> Mezcla Lenta -> Sedimentación).
 * 
 * SEGURIDAD:
 * Monitorea constantemente la boya de nivel de acero inoxidable (GPIO 32). Si el 
 * nivel de agua cae por debajo del mínimo, apaga el motor para evitar marcha en seco.
 * 
 * CONEXIONES FÍSICAS AL ESP32 (38 PINES):
 * - GPIO 4  ──► ENA del L298N (PWM de Velocidad - Quitar jumper negro)
 * - GPIO 16 ──► IN1 del L298N (Sentido Giro Adelante)
 * - GPIO 17 ──► IN2 del L298N (Sentido Giro Reversa)
 * - GPIO 32 ──► Boya de nivel (Cable 1; el Cable 2 va a GND)
 * - GND     ──► GND común del L298N y ESP32
 * ============================================================================== */

#include <Arduino.h>

// ------------------------------------------------------------------------------
// 1. ASIGNACIÓN DE PINES
// ------------------------------------------------------------------------------
const uint8_t PIN_AGIT_ENA  = 4;  // PWM de control de velocidad
const uint8_t PIN_AGIT_IN1  = 16; // Dirección de giro A
const uint8_t PIN_AGIT_IN2  = 17; // Dirección de giro B
const uint8_t PIN_BOYA_NIVEL = 32; // Sensor de nivel flotante

// Canal PWM LEDC para el motor DC (ESP32 Arduino Core 3.x)
const uint32_t PWM_FREQ = 5000; // 5 kHz (Frecuencia inaudible y suave)
const uint8_t PWM_RES   = 8;    // Resolución de 8 bits (Valores de 0 a 255)

// ------------------------------------------------------------------------------
// 2. MÁQUINA DE ESTADOS FINITOS (FSM) DEL REACTOR
// ------------------------------------------------------------------------------
enum EstadoReactor {
  REPOSO,
  MEZCLA_RAPIDA,
  MEZCLA_LENTA,
  SEDIMENTACION,
  ALARMA_NIVEL_BAJO
};

EstadoReactor estadoActual = REPOSO;
unsigned long t_inicio_estado_ms = 0;

// Tiempos configurables de proceso
const unsigned long T_MEZCLA_RAPIDA_MS = 60000;   // 1 minuto
const unsigned long T_MEZCLA_LENTA_MS  = 900000;  // 15 minutos
const unsigned long T_SEDIMENTACION_MS = 1800000; // 30 minutos

// ------------------------------------------------------------------------------
// 3. FUNCIONES DE CONTROL DEL MOTOR L298N
// ------------------------------------------------------------------------------
void fijarVelocidadAgitador(uint8_t pwmVal, bool sentidoHorario = true) {
  if (pwmVal == 0) {
    digitalWrite(PIN_AGIT_IN1, LOW);
    digitalWrite(PIN_AGIT_IN2, LOW);
    ledcWrite(PIN_AGIT_ENA, 0);
  } else {
    if (sentidoHorario) {
      digitalWrite(PIN_AGIT_IN1, HIGH);
      digitalWrite(PIN_AGIT_IN2, LOW);
    } else {
      digitalWrite(PIN_AGIT_IN1, LOW);
      digitalWrite(PIN_AGIT_IN2, HIGH);
    }
    ledcWrite(PIN_AGIT_ENA, pwmVal);
  }
}

// ------------------------------------------------------------------------------
// 4. SETUP: CONFIGURACIÓN DE PINES Y PWM
// ------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n========================================================");
  Serial.println("  PLANTA PILOTO DE ULTRAFILTRACIÓN INDUSTRIAL - HITO 4   ");
  Serial.println("  Control de Agitador (L298N) y Boya de Nivel Inox       ");
  Serial.println("========================================================");

  pinMode(PIN_AGIT_IN1, OUTPUT);
  pinMode(PIN_AGIT_IN2, OUTPUT);
  pinMode(PIN_BOYA_NIVEL, INPUT_PULLUP);

  // Configurar canal PWM en GPIO 4
  ledcAttach(PIN_AGIT_ENA, PWM_FREQ, PWM_RES);
  fijarVelocidadAgitador(0);

  Serial.println("[OK] Driver L298N inicializado.");
  Serial.println("[OK] Boya de nivel en GPIO 32 configurada.");
  Serial.println("\nComandos disponibles por Monitor Serie:");
  Serial.println("  'AUTO' -> Iniciar ciclo completo automático de Coagulación/Floculación");
  Serial.println("  'FAST' -> Mezcla Rápida manual (PWM 220 / ~150 RPM)");
  Serial.println("  'SLOW' -> Mezcla Lenta manual (PWM 80 / ~30 RPM)");
  Serial.println("  'STOP' -> Detener Agitador");
}

// ------------------------------------------------------------------------------
// 5. LOOP: EJECUCIÓN DE LA FSM Y ENCLAVAMIENTO DE SEGURIDAD
// ------------------------------------------------------------------------------
void loop() {
  // 1. Verificación de Seguridad: Nivel de Agua
  bool nivelBajo = (digitalRead(PIN_BOYA_NIVEL) == HIGH); // Si la boya cae, abre circuito
  
  if (nivelBajo && estadoActual != REPOSO) {
    estadoActual = ALARMA_NIVEL_BAJO;
    fijarVelocidadAgitador(0);
    Serial.println("🚨 [ALARMA] Nivel de agua insuficiente en el reactor. Motor detenido por seguridad.");
  }

  // 2. Transición Temporal de la FSM Automática
  unsigned long t_transcurrido = millis() - t_inicio_estado_ms;

  switch (estadoActual) {
    case REPOSO:
      // Esperando comando del usuario
      break;

    case MEZCLA_RAPIDA:
      fijarVelocidadAgitador(220); // ~150 RPM (Gradiente G ≈ 400 s⁻¹)
      if (t_transcurrido >= T_MEZCLA_RAPIDA_MS) {
        Serial.println("[FSM] Fin de Mezcla Rápida. Iniciando Etapa 2: Mezcla Lenta (Floculación)...");
        estadoActual = MEZCLA_LENTA;
        t_inicio_estado_ms = millis();
      }
      break;

    case MEZCLA_LENTA:
      fijarVelocidadAgitador(80); // ~30 RPM (Gradiente G ≈ 35 s⁻¹)
      if (t_transcurrido >= T_MEZCLA_LENTA_MS) {
        Serial.println("[FSM] Fin de Floculación. Iniciando Etapa 3: Sedimentación Estática (0 RPM)...");
        estadoActual = SEDIMENTACION;
        t_inicio_estado_ms = millis();
        fijarVelocidadAgitador(0);
      }
      break;

    case SEDIMENTACION:
      fijarVelocidadAgitador(0);
      if (t_transcurrido >= T_SEDIMENTACION_MS) {
        Serial.println("✅ [FSM] Ciclo de Coagulación-Sedimentación COMPLETADO. Sobrenadante listo para UF.");
        estadoActual = REPOSO;
      }
      break;

    case ALARMA_NIVEL_BAJO:
      if (!nivelBajo) {
        Serial.println("[OK] Nivel de agua restablecido. Sistema en reposo.");
        estadoActual = REPOSO;
      }
      break;
  }

  // 3. Comandos por Monitor Serie
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.equalsIgnoreCase("AUTO")) {
      if (!nivelBajo) {
        estadoActual = MEZCLA_RAPIDA;
        t_inicio_estado_ms = millis();
        Serial.println("⚡ [FSM] Iniciando Ciclo Automático: Etapa 1 -> Mezcla Rápida (150 RPM)...");
      } else {
        Serial.println("❌ No se puede iniciar: El tanque no tiene nivel de agua.");
      }
    } else if (cmd.equalsIgnoreCase("FAST")) {
      fijarVelocidadAgitador(220);
      Serial.println("[MANUAL] Mezcla Rápida activada (PWM 220).");
    } else if (cmd.equalsIgnoreCase("SLOW")) {
      fijarVelocidadAgitador(80);
      Serial.println("[MANUAL] Mezcla Lenta activada (PWM 80).");
    } else if (cmd.equalsIgnoreCase("STOP")) {
      estadoActual = REPOSO;
      fijarVelocidadAgitador(0);
      Serial.println("[MANUAL] Agitador detenido.");
    }
  }

  delay(50);
}
