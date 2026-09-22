/* ==============================================================================
 * PROYECTO DE TESIS DE INGENIERÍA INDUSTRIAL - PLANTA DE ULTRAFILTRACIÓN FX100
 * SUBHITO 2.1: LECTURA, MONITOREO Y CALIBRACIÓN DE CAUDALÍMETROS YF-S401
 * ==============================================================================
 * 
 * OBJETIVO PEDAGÓGICO Y OPERATIVO:
 * 1. Medir con precisión de microflujo los pulsos generados por los sensores de efecto Hall.
 * 2. Calcular en tiempo real la frecuencia (Hz), caudal instantáneo (L/min y mL/min) y volumen (L).
 * 3. Incorporar control opcional de la bomba peristáltica MBP-2000 (DM860) mediante comandos
 *    por Monitor Serie para verificar la concordancia entre flujo bombeado y flujo medido.
 * 
 * CONEXIÓN FÍSICA EN EL SHIELD ESP32 (38 PINES):
 * ┌────────────────────────┬────────────────────────────────┬──────────────────────────┐
 * │ SENSOR / ACTUADOR      │ CABLE / BORNE DISPOSITIVO      │ BORNE SHIELD ESP32       │
 * ├────────────────────────┼────────────────────────────────┼──────────────────────────┤
 * │ Caudalímetro 1 (Q_feed)│ Cable Rojo (VCC)               │ Borne [ 5V ] / [ VIN ]   │
 * │ (Feed: Bomba a Filtro) │ Cable Negro (GND)              │ Borne [ GND ]            │
 * │ (Montaje Vertical Asc.)│ Cable Amarillo (Señal Pulsos)  │ Borne [ P14 ] (GPIO 14)  │
 * ├────────────────────────┼────────────────────────────────┼──────────────────────────┤
 * │ Caudalímetro 2 (Q_perm)│ Cable Rojo (VCC)               │ Borne [ 5V ] / [ VIN ]   │
 * │ (Permeado / Filtrado)  │ Cable Negro (GND)              │ Borne [ GND ]            │
 * │ (Montaje Horizontal)   │ Cable Amarillo (Señal Pulsos)  │ Borne [ P27 ] (GPIO 27)  │
 * ├────────────────────────┼────────────────────────────────┼──────────────────────────┤
 * │ Driver DM860 (Bomba)   │ PUL+ y DIR+ (Puenteados)       │ Borne [ VIN ] (5V Ánodo) │
 * │                        │ PUL- (Señal Paso)              │ Borne [ P18 ] (GPIO 18)  │
 * │                        │ DIR- (Dirección)               │ Borne [ P19 ] (GPIO 19)  │
 * └────────────────────────┴────────────────────────────────┴──────────────────────────┘
 * 
 * FACTOR DE CONVERSIÓN YF-S401:
 * - Ecuación del fabricante: Frecuencia (Hz) = 98 * Caudal (L/min)
 * - Pulsos por Litro: K = 98 * 60 = 5880 pulsos / Litro
 * - Caudal (L/min) = Frecuencia (Hz) / 98.0
 * - Caudal (mL/min) = Caudal (L/min) * 1000.0
 * ============================================================================== */

#include <Arduino.h>

// ==============================================================================
// 1. ASIGNACIÓN DE PINES
// ==============================================================================
const uint8_t PIN_CAUDAL_1 = 14; // Caudalímetro 1 (Borne P14)
const uint8_t PIN_CAUDAL_2 = 27; // Caudalímetro 2 (Borne P27)

// Pines de la bomba peristáltica (opcionales para ensayo en banco)
const uint8_t PIN_PUL = 18; // Borne P18 -> DM860 PUL-
const uint8_t PIN_DIR = 19; // Borne P19 -> DM860 DIR-
const uint8_t CANAL_LEDC = 0; // Canal PWM del ESP32

// ==============================================================================
// 2. PARÁMETROS MECÁNICOS Y DE CALIBRACIÓN
// ==============================================================================
// Factor K del caudalímetro YF-S401 (Hz por L/min)
const float FACTOR_K_YFS401 = 98.0f; 

// Parámetros de la bomba MBP-2000 (DM860 a 1600 pulsos/rev)
const uint16_t PULSOS_POR_REV = 1600;
const float ML_POR_VUELTA = 4.2f; // mL expulsados por cada vuelta del cabezal

// ==============================================================================
// 3. VARIABLES DE INTERRUPCIÓN (VOLATILE PARA SEGURIDAD EN HILOS)
// ==============================================================================
volatile unsigned long conteoPulsos1 = 0;
volatile unsigned long conteoPulsos2 = 0;

// Rutinas de Servicio de Interrupción (ISR) en memoria ultra-rápida IRAM
void IRAM_ATTR isrCaudalimetro1() {
  conteoPulsos1++;
}

void IRAM_ATTR isrCaudalimetro2() {
  conteoPulsos2++;
}

// ==============================================================================
// 4. VARIABLES DE TELEMETRÍA Y CONTROL
// ==============================================================================
unsigned long pulsosAcumulados1 = 0;
unsigned long pulsosAcumulados2 = 0;

float volumenTotal1_L = 0.0f;
float volumenTotal2_L = 0.0f;

unsigned long t_ultimo_calculo_ms = 0;

// Estado de la bomba auxiliar de prueba
bool bombaActiva = false;
float rpm_actual = 0.0f;

// ==============================================================================
// 5. FUNCIONES AUXILIARES DE CONTROL DE LA BOMBA
// ==============================================================================
void fijarVelocidadBomba(float rpm) {
  if (rpm <= 0.0f) {
    ledcWrite(CANAL_LEDC, 0); // Detener pulsos PWM
    bombaActiva = false;
    rpm_actual = 0.0f;
    Serial.println("\n>>> [BOMBA] Detenida (0 RPM).");
  } else {
    // Cálculo de frecuencia: Freq = (RPM / 60) * 1600
    float freqHz = (rpm / 60.0f) * (float)PULSOS_POR_REV;
    ledcSetup(CANAL_LEDC, freqHz, 8); // Resolución 8 bits
    ledcWrite(CANAL_LEDC, 128);       // 50% ciclo de trabajo (onda cuadrada simétrica)
    bombaActiva = true;
    rpm_actual = rpm;
    float caudalTeorico = (rpm * ML_POR_VUELTA) / 1000.0f;
    Serial.printf("\n>>> [BOMBA] Marcha a %.0f RPM | Frecuencia PWM: %.1f Hz | Caudal Teórico: %.3f L/min (%.0f mL/min)\n",
                  rpm, freqHz, caudalTeorico, caudalTeorico * 1000.0f);
  }
}

void procesarComandoSerie(char cmd) {
  switch (cmd) {
    case '0':
      fijarVelocidadBomba(0.0f);
      break;
    case '1':
      fijarVelocidadBomba(80.0f);
      break;
    case '2':
      fijarVelocidadBomba(100.0f);
      break;
    case '3':
      fijarVelocidadBomba(120.0f);
      break;
    case '4':
      fijarVelocidadBomba(140.0f);
      break;
    case 'r':
    case 'R':
      volumenTotal1_L = 0.0f;
      volumenTotal2_L = 0.0f;
      pulsosAcumulados1 = 0;
      pulsosAcumulados2 = 0;
      Serial.println("\n>>> [RESET] Contadores de volumen y pulsos restablecidos a CERO.");
      break;
    case '?':
    case 'h':
    case 'H':
      Serial.println("\n================ MENÚ DE COMANDOS POR TECLADO ================");
      Serial.println("  '0' : Detener Bomba (0 RPM)");
      Serial.println("  '1' : Arrancar Bomba a 80 RPM  (Teórico: 0.336 L/min)");
      Serial.println("  '2' : Arrancar Bomba a 100 RPM (Teórico: 0.420 L/min)");
      Serial.println("  '3' : Arrancar Bomba a 120 RPM (Teórico: 0.504 L/min)");
      Serial.println("  '4' : Arrancar Bomba a 140 RPM (Teórico: 0.588 L/min)");
      Serial.println("  'r' : Resetear contadores de volumen acumulado");
      Serial.println("  '?' : Mostrar esta ayuda");
      Serial.println("==============================================================");
      break;
    default:
      // Ignorar saltos de línea y retorno de carro
      break;
  }
}

// ==============================================================================
// 6. SETUP: INICIALIZACIÓN DE PUERTO SERIE, PINES E INTERRUPCIONES
// ==============================================================================
void setup() {
  Serial.begin(115200);
  delay(1000); // Pausa para permitir apertura estable del monitor serie

  Serial.println("\n==================================================================");
  Serial.println("  PLANTA PILOTO DE ULTRAFILTRACIÓN INDUSTRIAL - SUBHITO 2.1      ");
  Serial.println("  TEST Y MONITOREO DE CAUDALÍMETROS DE MICROFLUJO (YF-S401)      ");
  Serial.println("==================================================================");

  // Configuración de Pines de los Caudalímetros con PULLUP interno
  pinMode(PIN_CAUDAL_1, INPUT_PULLUP);
  pinMode(PIN_CAUDAL_2, INPUT_PULLUP);

  // Asignar interrupciones por flanco de subida (RISING)
  attachInterrupt(digitalPinToInterrupt(PIN_CAUDAL_1), isrCaudalimetro1, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_CAUDAL_2), isrCaudalimetro2, RISING);
  Serial.println("[OK] Interrupciones de hardware asignadas en GPIO 14 y GPIO 27.");

  // Configuración de pines de la bomba para ensayo dinámico
  pinMode(PIN_DIR, OUTPUT_OPEN_DRAIN);
  digitalWrite(PIN_DIR, LOW); // Sentido horario por defecto
  ledcSetup(CANAL_LEDC, 1000, 8);
  ledcAttachPin(PIN_PUL, CANAL_LEDC);
  ledcWrite(CANAL_LEDC, 0); // Bomba inicialmente apagada
  Serial.println("[OK] Driver DM860 configurado (PUL: GPIO 18, DIR: GPIO 19).");

  Serial.println("\n[INSTRUCCIÓN] Escribe '?' y pulsa Enter para ver comandos de la bomba.");
  Serial.println("Comenzando adquisición continua cada 1 segundo...\n");

  t_ultimo_calculo_ms = millis();
}

// ==============================================================================
// 7. LOOP PRINCIPAL: CÁLCULO DE CAUDAL CADA 1000 ms
// ==============================================================================
void loop() {
  // 1. Atender comandos del usuario por el Monitor Serie
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    procesarComandoSerie(c);
  }

  // 2. Muestreo y cálculo periódico de caudales
  unsigned long t_ahora = millis();
  if (t_ahora - t_ultimo_calculo_ms >= 1000) {
    float dt = (t_ahora - t_ultimo_calculo_ms) / 1000.0f;
    t_ultimo_calculo_ms = t_ahora;

    // Sección Crítica: Capturar pulsos y reiniciar acumulador atómicamente
    noInterrupts();
    unsigned long p1 = conteoPulsos1;
    unsigned long p2 = conteoPulsos2;
    conteoPulsos1 = 0;
    conteoPulsos2 = 0;
    interrupts();

    // Actualizar pulsos acumulados totales
    pulsosAcumulados1 += p1;
    pulsosAcumulados2 += p2;

    // Frecuencias instantáneas en Hertz (pulsos por segundo)
    float f1_Hz = (float)p1 / dt;
    float f2_Hz = (float)p2 / dt;

    // Caudales en Litros por minuto y mililitros por minuto
    // Ecuación YF-S401: Q (L/min) = F (Hz) / 98.0
    float q1_Lmin = f1_Hz / FACTOR_K_YFS401;
    float q2_Lmin = f2_Hz / FACTOR_K_YFS401;

    float q1_mLmin = q1_Lmin * 1000.0f;
    float q2_mLmin = q2_Lmin * 1000.0f;

    // Integración de volumen en Litros: V = sum(Q_Lmin * dt / 60)
    volumenTotal1_L += (q1_Lmin / 60.0f) * dt;
    volumenTotal2_L += (q2_Lmin / 60.0f) * dt;

    // Presentación clara en el Monitor Serie
    Serial.printf("[t: %4lus] ", t_ahora / 1000);

    // Reporte Caudalímetro 1 (Feed / Alimentación - GPIO 14)
    Serial.printf("| FEED (P14): %5.1f Hz -> %5.1f mL/min (%5.3f L/min) | Vol_F: %5.3f L ",
                  f1_Hz, q1_mLmin, q1_Lmin, volumenTotal1_L);

    // Si Caudalímetro 2 (Permeado - GPIO 27) tiene pulsos o actividad
    if (f2_Hz > 0.0f || volumenTotal2_L > 0.0f) {
      float qRet_calc_Lmin = (q1_Lmin >= q2_Lmin) ? (q1_Lmin - q2_Lmin) : 0.0f;
      float rec_porc = (q1_Lmin > 0.02f) ? ((q2_Lmin / q1_Lmin) * 100.0f) : 0.0f;

      Serial.printf("| PERM (P27): %5.1f mL/min | RET(calc): %5.1f mL/min | Recov: %4.1f%% ",
                    q2_mLmin, qRet_calc_Lmin * 1000.0f, rec_porc);
    }

    // Contraste con Bomba si está encendida
    if (bombaActiva) {
      float qBomba_teorico_Lmin = (rpm_actual * ML_POR_VUELTA) / 1000.0f;
      float dif_Lmin = q1_Lmin - qBomba_teorico_Lmin;
      float error_porc = 0.0f;
      if (qBomba_teorico_Lmin > 0.0f) {
        error_porc = (dif_Lmin / qBomba_teorico_Lmin) * 100.0f;
      }
      Serial.printf("| [Bomba %.0f RPM | Teór: %5.3f L/m | Δ: %+5.1f%%]",
                    rpm_actual, qBomba_teorico_Lmin, error_porc);
    }

    Serial.println();
  }
}
