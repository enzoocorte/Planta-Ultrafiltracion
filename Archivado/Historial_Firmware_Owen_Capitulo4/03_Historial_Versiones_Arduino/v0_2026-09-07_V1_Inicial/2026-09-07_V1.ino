/*
 * =========================================================================================
 * UNIVERSIDAD NACIONAL DE SALTA (UNSa) - FACULTAD DE INGENIERÍA
 * Escuela de Ingeniería Industrial - Proyecto Final de Grado
 * 
 * Proyecto: Módulo Automatizado de Ultrafiltración para Potabilización de Agua
 *           con Membranas de Hemodiálisis Reutilizadas (Fresenius FX100 Classic)
 * 
 * ARCHIVO DE FIRMWARE: 2026-09-07_V1.ino
 * FECHA DE CREACIÓN:  07 de Septiembre de 2026
 * VERSIÓN:            V1 (Versión 1 - Firmware Base Arduino Uno Consolidado)
 * CONTROLADOR:        Arduino Uno R3 (Microcontrolador Microchip ATmega328P @ 16 MHz)
 * ACTUADOR:           Bomba Peristáltica MBP-2000 (3 rodillos, manguera silicona 18/12 mm)
 * MOTOR:              Paso a Paso NEMA 34 (4.5 Nm, 8 hilos, conexión Bipolar Serie)
 * DRIVER INDUSTRIAL:  Leadshine DM860 (Alimentación 24 VDC / 5 A)
 * =========================================================================================
 * 
 * AJUSTES DE MICROSWITCHES EN EL DRIVER DM860:
 *   - Corriente de Fase: 2.57 A RMS (3.08 A Pico) -> SW1: OFF, SW2: ON,  SW3: ON
 *   - Reposo Térmico:    Half Current al 50%       -> SW4: OFF (Protección térmica de bobinas)
 *   - Resolución Pasos:  1600 pulsos/rev (1/8)     -> SW5: ON,  SW6: OFF, SW7: ON, SW8: ON
 * 
 * DIAGRAMA DE CONEXIÓN ELÉCTRICA (Topología Ánodo Común a +5V nativo):
 *   - Arduino Uno Pin 5V   -----> Driver DM860 [PUL+] y [DIR+] (Línea de Ánodo Común +5V)
 *   - Arduino Uno Pin D9   -----> Driver DM860 [PUL-] (Tren de pulsos, activo en nivel BAJO)
 *   - Arduino Uno Pin D8   -----> Driver DM860 [DIR-] (Sentido de giro: HIGH = CW, LOW = CCW)
 *   - Arduino Uno Pin GND  -----> Borne GND de masa común de señales
 *   - Driver DM860 [ENA±]  -----> Desconectados al aire (Habilitado permanente por hardware)
 *   - Arduino Uno Pin D13  -----> LED Testigo Integrado (Enciende durante marcha activa)
 *   - Arduino Uno Pin A0   -----> Potenciómetro analógico opcional de 10k (Modo 'M')
 * 
 * VENTAJAS CINEMÁTICAS IMPLEMENTADAS:
 *   1. Máquina de Estados Asíncrona (FSM): Bucle continuo con micros() y millis() sin delay().
 *   2. Rampa Trapezoidal Slew-Rate: Aceleración y desaceleración progresiva anti-tirones.
 *   3. Inversión Segura de Marcha: Frenado suave a 0 RPM -> Pausa inercial (150 ms) -> Inversión.
 *   4. Protección de Memoria SRAM: Macros F() en todos los literales para no saturar los 2 KB de RAM.
 * =========================================================================================
 */

#include <Arduino.h>

// ============================================================================
// ASIGNACIÓN DE PINES DE CONTROL
// ============================================================================
const uint8_t PIN_PUL = 9;   // Salida de pulsos de paso (hacia PUL- del DM860)
const uint8_t PIN_DIR = 8;   // Salida de dirección (hacia DIR- del DM860)
const uint8_t PIN_LED = 13;  // LED indicador integrado de marcha
const uint8_t PIN_POT = A0;  // Entrada analógica para potenciómetro (opcional)

// ============================================================================
// LÍMITES CINEMÁTICOS Y PARÁMETROS DEL SISTEMA
// ============================================================================
const unsigned int DELAY_MIN_US       = 250;   // Límite de velocidad máxima (~125 RPM)
const unsigned int DELAY_MAX_US       = 6000;  // Límite de velocidad mínima (~6.2 RPM)
const unsigned int DELAY_ARRANQUE_US  = 4500;  // Consigna inicial de despegue seguro (~8.2 RPM)
const unsigned int DELAY_CRUCERO_US   = 3000;  // Velocidad nominal de ultrafiltración (~12.3 RPM)
const unsigned int PASO_AJUSTE_US     = 150;   // Salto de ajuste fino por comandos '+' y '-'
const unsigned int ANCHO_PULSO_US     = 25;    // Ancho mínimo de pulso en nivel BAJO para DM860 (>=2.5 us)
const unsigned int PASOS_POR_REV      = 1600;  // Configuración de micropasos en el DM860 (1/8)

// Parámetros de rampa cinemática anti-inercial
const unsigned int RAMPA_SALTO_US      = 15;   // Variación de microsegundos por ciclo de rampa
const unsigned long RAMPA_INTERVALO_MS = 6;    // Periodo de actualización de rampa en ms

// ============================================================================
// MÁQUINA DE ESTADOS FINITOS (FSM)
// ============================================================================
enum EstadoBomba {
  DETENIDA,
  ARRANCANDO,
  MARCHA_REGIMEN,
  FRENANDO,
  FRENANDO_PARA_INVERTIR,
  PAUSA_INERCIA_INVERSION
};

EstadoBomba estadoActual = DETENIDA;

// Variables de consigna y velocidad dinámica
unsigned int delayObjetivo = DELAY_CRUCERO_US;
unsigned int delayActual   = DELAY_ARRANQUE_US;

bool sentidoGiroDeseado    = HIGH; // HIGH = Horario / Filtración, LOW = Antihorario / Retrolavado
bool sentidoGiroHardware   = HIGH;
bool estadoPinPulso        = HIGH; // Reposo en ALTO (Ánodo Común a +5V)

// Temporizadores de precisión
unsigned long tiempoUltimoPulso     = 0;
unsigned long tiempoUltimaRampa     = 0;
unsigned long tiempoPausaInversion   = 0;
unsigned long tiempoUltimaTelemetria = 0;
unsigned long tiempoInicioMarcha    = 0;
unsigned long tiempoAcumuladoSeg    = 0;

// Modo potenciómetro analógico
bool modoPotenciometro = false;
int lecturaPotAnterior = -1;

// ============================================================================
// PROTOTIPOS DE FUNCIONES
// ============================================================================
void procesarComando(char c);
void actualizarRampaCinematica();
void generarPulsosPaso();
void leerPotenciometro();
void imprimirTelemetriaCompacta();
void imprimirReporteCompleto();
void imprimirMenuAyuda();
float calcularRPM(unsigned int d);
float calcularCaudalLPM(unsigned int d);
float calcularFrecuenciaHz(unsigned int d);

// ============================================================================
// CONFIGURACIÓN INICIAL (SETUP)
// ============================================================================
void setup() {
  pinMode(PIN_PUL, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_LED, OUTPUT);

  // Estado eléctrico seguro en reposo (Ánodo común: reposo en HIGH)
  digitalWrite(PIN_PUL, HIGH);
  digitalWrite(PIN_DIR, sentidoGiroHardware);
  digitalWrite(PIN_LED, LOW);

  Serial.begin(115200);
  while (!Serial && millis() < 1500);

  Serial.println(F("\n=========================================================="));
  Serial.println(F(" UNSa - FACULTAD DE INGENIERIA - INGENIERIA INDUSTRIAL"));
  Serial.println(F(" Proyecto: Modulo de Ultrafiltracion FX100 - MBP-2000"));
  Serial.println(F(" Firmware: 2026-09-07_V1.ino (Plataforma Arduino Uno)"));
  Serial.println(F(" Driver DM860: 2.57A RMS | 1600 p/rev | Half Current SW4"));
  Serial.println(F(" Topologia: Anodo Comun +5V | Rampa Anti-Tirones"));
  Serial.println(F("=========================================================="));
  
  imprimirMenuAyuda();
  imprimirReporteCompleto();
}

// ============================================================================
// BUCLE PRINCIPAL (LOOP ASÍNCRONO)
// ============================================================================
void loop() {
  // 1. Lectura no bloqueante de comandos desde UART
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c != '\r' && c != '\n') {
      procesarComando(c);
    }
  }

  // 2. Control analógico por potenciómetro si está habilitado
  if (modoPotenciometro && (estadoActual == ARRANCANDO || estadoActual == MARCHA_REGIMEN)) {
    leerPotenciometro();
  }

  // 3. Actualización de rampa de aceleración / desaceleración
  actualizarRampaCinematica();

  // 4. Generación de pulsos de micropaso si la bomba está activa
  if (estadoActual != DETENIDA && estadoActual != PAUSA_INERCIA_INVERSION) {
    generarPulsosPaso();
  }

  // 5. Telemetría periódica ligera (cada 3 segundos durante marcha)
  if (estadoActual == MARCHA_REGIMEN && (millis() - tiempoUltimaTelemetria >= 3000)) {
    tiempoUltimaTelemetria = millis();
    imprimirTelemetriaCompacta();
  }
}

// ============================================================================
// GESTOR DE COMANDOS SERIE
// ============================================================================
void procesarComando(char c) {
  switch (c) {
    case '1': // Arranque en sentido HORARIO (Filtración)
      sentidoGiroDeseado = HIGH;
      if (estadoActual == DETENIDA) {
        sentidoGiroHardware = HIGH;
        digitalWrite(PIN_DIR, HIGH);
        delayActual = DELAY_ARRANQUE_US;
        estadoActual = ARRANCANDO;
        tiempoInicioMarcha = millis();
        digitalWrite(PIN_LED, HIGH);
        Serial.println(F("[OK] Marcha iniciada en sentido HORARIO (Filtracion)."));
      } else if (!sentidoGiroHardware) {
        Serial.println(F("[AVISO] Solicitud de inversion: Iniciando frenado suave..."));
        estadoActual = FRENANDO_PARA_INVERTIR;
      } else {
        Serial.println(F("[INFO] Ya se encuentra en marcha Horaria."));
      }
      break;

    case '2': // Arranque en sentido ANTIHORARIO (Retrolavado)
      sentidoGiroDeseado = LOW;
      if (estadoActual == DETENIDA) {
        sentidoGiroHardware = LOW;
        digitalWrite(PIN_DIR, LOW);
        delayActual = DELAY_ARRANQUE_US;
        estadoActual = ARRANCANDO;
        tiempoInicioMarcha = millis();
        digitalWrite(PIN_LED, HIGH);
        Serial.println(F("[OK] Marcha iniciada en sentido ANTIHORARIO (Retrolavado)."));
      } else if (sentidoGiroHardware) {
        Serial.println(F("[AVISO] Solicitud de inversion: Iniciando frenado suave..."));
        estadoActual = FRENANDO_PARA_INVERTIR;
      } else {
        Serial.println(F("[INFO] Ya se encuentra en marcha Antihoraria."));
      }
      break;

    case '0': // Parada suave con rampa de deceleración
      if (estadoActual != DETENIDA && estadoActual != FRENANDO) {
        Serial.println(F("[OK] Detencion solicitada: Desacelerando de forma suave..."));
        estadoActual = FRENANDO;
      } else {
        Serial.println(F("[INFO] La bomba ya se encuentra detenida."));
      }
      break;

    case '+': // Incrementar velocidad (reducir delay)
    case 'V':
    case 'v':
      if (delayObjetivo > DELAY_MIN_US + PASO_AJUSTE_US) {
        delayObjetivo -= PASO_AJUSTE_US;
      } else {
        delayObjetivo = DELAY_MIN_US;
        Serial.println(F("[MAX] Alcanzado limite maximo de velocidad."));
      }
      Serial.print(F("[VEL+] Nuevo Delay Objetivo: "));
      Serial.print(delayObjetivo);
      Serial.print(F(" us (~"));
      Serial.print(calcularRPM(delayObjetivo), 1);
      Serial.println(F(" RPM)"));
      break;

    case '-': // Reducir velocidad (aumentar delay)
    case 'F':
    case 'f':
      if (delayObjetivo < DELAY_MAX_US - PASO_AJUSTE_US) {
        delayObjetivo += PASO_AJUSTE_US;
      } else {
        delayObjetivo = DELAY_MAX_US;
        Serial.println(F("[MIN] Alcanzado limite minimo de velocidad."));
      }
      Serial.print(F("[VEL-] Nuevo Delay Objetivo: "));
      Serial.print(delayObjetivo);
      Serial.print(F(" us (~"));
      Serial.print(calcularRPM(delayObjetivo), 1);
      Serial.println(F(" RPM)"));
      break;

    case 'A': // Preset 20 RPM (Filtración Base Estable)
    case 'a':
      delayObjetivo = 3000;
      Serial.println(F("[PRESET A] Seleccionado: 20 RPM (~3.0 L/min)"));
      break;

    case 'B': // Preset 50 RPM (Media Carga)
    case 'b':
      delayObjetivo = 1200;
      Serial.println(F("[PRESET B] Seleccionado: 50 RPM (~3.5 L/min)"));
      break;

    case 'C': // Preset 100 RPM (Retrolavado / Caudal Alto)
    case 'c':
      delayObjetivo = 575;
      Serial.println(F("[PRESET C] Seleccionado: 100 RPM (~3.7 L/min)"));
      break;

    case 'D': // Preset 125 RPM (Límite Máximo Continuo)
    case 'd':
      delayObjetivo = 460;
      Serial.println(F("[PRESET D] Seleccionado: 125 RPM (~3.75 L/min)"));
      break;

    case 'M': // Alternar modo potenciómetro
    case 'm':
      modoPotenciometro = !modoPotenciometro;
      Serial.print(F("[MODO] Control por Potenciometro en A0: "));
      Serial.println(modoPotenciometro ? F("ACTIVADO") : F("DESACTIVADO"));
      break;

    case '?': // Reporte completo de diagnóstico
      imprimirReporteCompleto();
      break;

    case 'H': // Menú de ayuda
    case 'h':
      imprimirMenuAyuda();
      break;

    default:
      Serial.print(F("[ERR] Comando desconocido: '"));
      Serial.print(c);
      Serial.println(F("'. Envie 'H' para ver menu."));
      break;
  }
}

// ============================================================================
// RAMPA CINEMÁTICA ASÍNCRONA (ACELERACIÓN / DESACELERACIÓN SUAVE)
// ============================================================================
void actualizarRampaCinematica() {
  unsigned long t = millis();
  if (t - tiempoUltimaRampa < RAMPA_INTERVALO_MS) {
    return;
  }
  tiempoUltimaRampa = t;

  switch (estadoActual) {
    case ARRANCANDO:
      if (delayActual > delayObjetivo) {
        if (delayActual - delayObjetivo > RAMPA_SALTO_US) {
          delayActual -= RAMPA_SALTO_US;
        } else {
          delayActual = delayObjetivo;
          estadoActual = MARCHA_REGIMEN;
          Serial.println(F("[RAMPA] Velocidad de regimen alcanzada de forma estable."));
        }
      } else if (delayActual < delayObjetivo) {
        if (delayObjetivo - delayActual > RAMPA_SALTO_US) {
          delayActual += RAMPA_SALTO_US;
        } else {
          delayActual = delayObjetivo;
          estadoActual = MARCHA_REGIMEN;
          Serial.println(F("[RAMPA] Velocidad de regimen alcanzada de forma estable."));
        }
      } else {
        estadoActual = MARCHA_REGIMEN;
      }
      break;

    case MARCHA_REGIMEN:
      if (delayActual > delayObjetivo) {
        if (delayActual - delayObjetivo > RAMPA_SALTO_US) {
          delayActual -= RAMPA_SALTO_US;
        } else {
          delayActual = delayObjetivo;
        }
      } else if (delayActual < delayObjetivo) {
        if (delayObjetivo - delayActual > RAMPA_SALTO_US) {
          delayActual += RAMPA_SALTO_US;
        } else {
          delayActual = delayObjetivo;
        }
      }
      break;

    case FRENANDO:
      if (delayActual < DELAY_ARRANQUE_US) {
        delayActual += (RAMPA_SALTO_US * 2); // Frenado controlado progresivo
      } else {
        estadoActual = DETENIDA;
        digitalWrite(PIN_PUL, HIGH); // Reposo seguro
        digitalWrite(PIN_LED, LOW);
        tiempoAcumuladoSeg += (millis() - tiempoInicioMarcha) / 1000;
        Serial.println(F("[RAMPA] Bomba detenida suavemente sin tirones mecanicos."));
      }
      break;

    case FRENANDO_PARA_INVERTIR:
      if (delayActual < DELAY_ARRANQUE_US) {
        delayActual += (RAMPA_SALTO_US * 2);
      } else {
        estadoActual = PAUSA_INERCIA_INVERSION;
        digitalWrite(PIN_PUL, HIGH);
        tiempoPausaInversion = millis();
        Serial.println(F("[INVERSION] Detencion completa. Esperando 150 ms para amortiguar inercia..."));
      }
      break;

    case PAUSA_INERCIA_INVERSION:
      if (millis() - tiempoPausaInversion >= 150) {
        sentidoGiroHardware = sentidoGiroDeseado;
        digitalWrite(PIN_DIR, sentidoGiroHardware);
        delayActual = DELAY_ARRANQUE_US;
        estadoActual = ARRANCANDO;
        Serial.print(F("[INVERSION] Giro conmutado a: "));
        Serial.println(sentidoGiroHardware ? F("HORARIO (Filtracion)") : F("ANTIHORARIO (Retrolavado)"));
        Serial.println(F("[INVERSION] Acelerando suavemente en nuevo sentido..."));
      }
      break;

    case DETENIDA:
    default:
      break;
  }
}

// ============================================================================
// GENERACIÓN DE PULSOS MICROPASO EN TIEMPO REAL
// ============================================================================
void generarPulsosPaso() {
  unsigned long t = micros();

  if (estadoPinPulso == HIGH) {
    if ((unsigned long)(t - tiempoUltimoPulso) >= delayActual) {
      digitalWrite(PIN_PUL, LOW); // Flanco activo en nivel bajo (Ánodo común)
      estadoPinPulso = LOW;
      tiempoUltimoPulso = t;
    }
  } else {
    if ((unsigned long)(t - tiempoUltimoPulso) >= ANCHO_PULSO_US) {
      digitalWrite(PIN_PUL, HIGH); // Retorno a reposo
      estadoPinPulso = HIGH;
      tiempoUltimoPulso = t;
    }
  }
}

// ============================================================================
// LECTURA DE POTENCIÓMETRO ANALÓGICO
// ============================================================================
void leerPotenciometro() {
  int lectura = analogRead(PIN_POT);
  if (lecturaPotAnterior == -1 || abs(lectura - lecturaPotAnterior) > 15) {
    lecturaPotAnterior = lectura;
    unsigned int nuevoDelay = map(lectura, 0, 1023, DELAY_MAX_US, DELAY_MIN_US);
    delayObjetivo = nuevoDelay;
  }
}

// ============================================================================
// MODELOS MATEMÁTICOS DE CONVERSIÓN
// ============================================================================
float calcularFrecuenciaHz(unsigned int d) {
  if (d == 0) return 0.0;
  return 1000000.0 / (float)(d + ANCHO_PULSO_US);
}

float calcularRPM(unsigned int d) {
  float f = calcularFrecuenciaHz(d);
  return (f * 60.0) / (float)PASOS_POR_REV;
}

float calcularCaudalLPM(unsigned int d) {
  // Modelo experimental de ajuste: Q = -0.029 * (delay / 10) + 3.858
  float q = -0.029 * ((float)d / 10.0) + 3.858;
  if (q < 0.0) q = 0.0;
  return q;
}

// ============================================================================
// FUNCIONES DE TELEMETRÍA Y REPORTES
// ============================================================================
void imprimirTelemetriaCompacta() {
  Serial.print(F("[TELEM] Est:"));
  Serial.print(estadoActual == MARCHA_REGIMEN ? F("REGIMEN") : F("TRANS"));
  Serial.print(F(" | Dir:"));
  Serial.print(sentidoGiroHardware ? F("CW") : F("CCW"));
  Serial.print(F(" | Delay:"));
  Serial.print(delayActual);
  Serial.print(F("us | RPM:"));
  Serial.print(calcularRPM(delayActual), 1);
  Serial.print(F(" | Q~"));
  Serial.print(calcularCaudalLPM(delayActual), 2);
  Serial.println(F(" L/min"));
}

void imprimirReporteCompleto() {
  Serial.println(F("\n----------------- REPORTE DE ESTADO DEL SISTEMA -----------------"));
  Serial.print(F(" Estado Operativo:       "));
  switch (estadoActual) {
    case DETENIDA:                Serial.println(F("DETENIDA (Reposo)")); break;
    case ARRANCANDO:               Serial.println(F("ARRANCANDO (Rampa de subida)")); break;
    case MARCHA_REGIMEN:          Serial.println(F("MARCHA EN REGIMEN")); break;
    case FRENANDO:                Serial.println(F("FRENANDO (Rampa de parada)")); break;
    case FRENANDO_PARA_INVERTIR:  Serial.println(F("FRENANDO PARA INVERTIR")); break;
    case PAUSA_INERCIA_INVERSION: Serial.println(F("PAUSA INERCIAL DE GIRO")); break;
  }
  Serial.print(F(" Sentido de Giro:        "));
  Serial.println(sentidoGiroHardware ? F("HORARIO (Filtracion Normal)") : F("ANTIHORARIO (Retrolavado)"));
  Serial.print(F(" Delay Actual / Obj:     "));
  Serial.print(delayActual);
  Serial.print(F(" us / "));
  Serial.print(delayObjetivo);
  Serial.println(F(" us"));
  Serial.print(F(" Frecuencia de Pulsos:   "));
  Serial.print(calcularFrecuenciaHz(delayActual), 1);
  Serial.println(F(" Hz"));
  Serial.print(F(" Velocidad Angular:      "));
  Serial.print(calcularRPM(delayActual), 1);
  Serial.println(F(" RPM"));
  Serial.print(F(" Caudal Estimado:        "));
  Serial.print(calcularCaudalLPM(delayActual), 2);
  Serial.println(F(" L/min"));
  Serial.print(F(" Modo Potenciometro:     "));
  Serial.println(modoPotenciometro ? F("HABILITADO (A0)") : F("DESHABILITADO"));
  Serial.println(F("-----------------------------------------------------------------\n"));
}

void imprimirMenuAyuda() {
  Serial.println(F("\n--- COMANDOS DISPONIBLES POR CONSOLA SERIAL (115200 BAUD) ---"));
  Serial.println(F("  '1' : Arrancar en sentido HORARIO (Filtracion)"));
  Serial.println(F("  '2' : Arrancar en sentido ANTIHORARIO (Retrolavado)"));
  Serial.println(F("  '0' : Detener suavemente con rampa de frenado"));
  Serial.println(F("  '+' : Aumentar velocidad (acelerar)"));
  Serial.println(F("  '-' : Disminuir velocidad (desacelerar)"));
  Serial.println(F("  'A' : Preset 20 RPM  (Filtracion Base - Delay 3000 us)"));
  Serial.println(F("  'B' : Preset 50 RPM  (Media Carga      - Delay 1200 us)"));
  Serial.println(F("  'C' : Preset 100 RPM (Retrolavado      - Delay 575 us)"));
  Serial.println(F("  'D' : Preset 125 RPM (Velocidad Maxima - Delay 460 us)"));
  Serial.println(F("  'M' : Alternar control por potenciometro analogico (A0)"));
  Serial.println(F("  '?' : Imprimir reporte completo de variables"));
  Serial.println(F("  'H' : Imprimir este menu de ayuda"));
  Serial.println(F("------------------------------------------------------------\n"));
}
