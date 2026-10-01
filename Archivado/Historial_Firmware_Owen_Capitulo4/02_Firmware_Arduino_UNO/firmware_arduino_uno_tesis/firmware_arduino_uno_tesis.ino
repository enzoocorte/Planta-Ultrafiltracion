/*
 * =========================================================================================
 * UNIVERSIDAD NACIONAL DE SALTA (UNSa) - FACULTAD DE INGENIERÍA
 * Proyecto Final de Grado: Módulo Piloto de Ultrafiltración FX100 - MBP-2000
 * Control Cinemático Suave de Motor NEMA 34 con Driver Industrial DM860
 * 
 * VERSIÓN DEFINITIVA OPTIMIZADA: ANTI-CALENTAMIENTO, ANTI-TIRONES Y FLUJO CONTINUO
 * Plataforma: Arduino Uno (ATmega328P - 16 MHz)
 * =========================================================================================
 * 
 * CONFIGURACIÓN EXACTA SEGÚN FOTO DEL DRIVER DM860:
 *   - Corriente de Fase: 2.57 A RMS / 3.08 A Pico -> SW1: OFF, SW2: ON,  SW3: ON
 *   - Reposo Térmico:    Half Current al 50%       -> SW4: OFF (Hacia ARRIBA)
 *   - Micropasos:        1600 pulsos/rev (1/8)     -> SW5: ON,  SW6: OFF, SW7: ON, SW8: ON
 * 
 * CONEXIÓN ELÉCTRICA (Ánodo Común a +5V nativo de Arduino Uno):
 *   - Borne 5V Arduino Uno  -----> Borne PUL+ y DIR+ del DM860 (Ánodo Común 5V)
 *   - Pin D9 Arduino Uno    -----> Borne PUL- del DM860 (Pulsos activos en nivel BAJO)
 *   - Pin D8 Arduino Uno    -----> Borne DIR- del DM860 (HIGH: Horario/Filt, LOW: Antihorario/Retro)
 *   - Borne GND Arduino Uno -----> GND de alimentación común de control
 *   - Borne ENA- / ENA+     -----> DESCONECTADOS (El DM860 gestiona el 50% de reposo por SW4)
 *   - Pin D13 Arduino Uno   -----> LED indicador integrado (Enciende durante marcha)
 *   - Pin A0 Arduino Uno    -----> Potenciómetro opcional de velocidad analógica (Modo 'M')
 * 
 * CARACTERÍSTICAS CINEMÁTICAS ANTI-TIRONES:
 *   1. Rampa continua trapezoidal no bloqueante (Aceleración y frenado progresivo Slew-Rate).
 *   2. Inversión inteligente de 3 fases: Frena a 0 RPM -> Pausa inercial (150 ms) -> Conmuta DIR -> Acelera.
 *   3. Buffer serie protegido: Telemetría compacta (<40 bytes) para no congelar la CPU ni el tren de pulsos.
 * =========================================================================================
 */

// ==========================================
// ASIGNACIÓN DE PINES
// ==========================================
const uint8_t PIN_PUL = 9;   // Señal de pulsos hacia PUL- (Activo en LOW)
const uint8_t PIN_DIR = 8;   // Sentido hacia DIR- (HIGH: Horario, LOW: Antihorario)
const uint8_t PIN_LED = 13;  // LED indicador integrado en Arduino Uno
const uint8_t PIN_POT = A0;  // Potenciómetro analógico opcional

// ==========================================
// PARÁMETROS CINEMÁTICOS DE LA TESIS
// ==========================================
const unsigned int DELAY_MIN_US       = 250;   // Velocidad máxima nominal (~125 RPM)
const unsigned int DELAY_MAX_US       = 6000;  // Velocidad mínima (~6.2 RPM)
const unsigned int DELAY_ARRANQUE_US  = 4500;  // Velocidad suave de arranque/corte (~8.2 RPM)
const unsigned int DELAY_CRUCERO_US   = 3000;  // Velocidad nominal de ultrafiltración (~12.3 RPM)
const unsigned int PASO_AJUSTE_US     = 150;   // Salto de ajuste fino por comando (+ / -)
const unsigned int ANCHO_PULSO_US     = 25;    // Ancho óptimo de pulso en LOW para DM860 (min 2.5 us)
const unsigned int PASOS_POR_REV      = 1600;  // Resolución física configurada en el DM860

// Parámetros de rampa suave
const unsigned int RAMPA_SALTO_US      = 12;   // Variación de microsegundos por ciclo de rampa
const unsigned long RAMPA_INTERVALO_MS = 6;    // Actualización de rampa cada 6 ms

// ==========================================
// MÁQUINA DE ESTADOS CINEMÁTICA (100% ASÍNCRONA)
// ==========================================
enum EstadoBomba {
  DETENIDA,
  ARRANCANDO,
  MARCHA_REGIMEN,
  FRENANDO,
  FRENANDO_PARA_INVERTIR,
  PAUSA_INERCIA_INVERSION
};

EstadoBomba estadoActual = DETENIDA;

// Variables dinámicas de velocidad y pulsos
unsigned int delayObjetivo = DELAY_CRUCERO_US;
unsigned int delayActual   = DELAY_ARRANQUE_US;

bool sentidoGiroDeseado    = HIGH; // HIGH = Filtración (CW), LOW = Retrolavado (CCW)
bool sentidoGiroHardware   = HIGH;
bool estadoPinPulso        = HIGH; // Reposo en HIGH (Ánodo común)

unsigned long tiempoUltimoPulso     = 0;
unsigned long tiempoUltimaRampa     = 0;
unsigned long tiempoPausaInversion   = 0;
unsigned long tiempoUltimaTelemetria = 0;
unsigned long tiempoInicioMarcha    = 0;
unsigned long tiempoAcumuladoSeg    = 0;

bool modoPotenciometro = false; // Desactivado por defecto (se activa con 'M')
int lecturaPotAnterior = -1;

// Prototipos
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

// ==========================================
// SETUP
// ==========================================
void setup() {
  pinMode(PIN_PUL, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_LED, OUTPUT);

  // Estado seguro de reposo (Ánodo Común: PUL en nivel ALTO)
  digitalWrite(PIN_PUL, HIGH);
  digitalWrite(PIN_DIR, sentidoGiroHardware);
  digitalWrite(PIN_LED, LOW);

  Serial.begin(115200);
  while (!Serial && millis() < 1500);

  Serial.println(F("\n=========================================================="));
  Serial.println(F(" UNSa - MODULO ULTRAFILTRACION FX100 / MBP-2000"));
  Serial.println(F(" Firmware Arduino Uno - Control Cinemático Suave (v2.1)"));
  Serial.println(F(" Driver DM860: 2.57A RMS | 1600 p/rev | Half Current SW4"));
  Serial.println(F("=========================================================="));
  imprimirMenuAyuda();
  imprimirReporteCompleto();
}

// ==========================================
// LOOP PRINCIPAL (100% NO BLOQUEANTE)
// ==========================================
void loop() {
  // 1. Recepción de comandos serie
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c != '\r' && c != '\n') {
      procesarComando(c);
    }
  }

  // 2. Control analógico opcional por potenciómetro
  if (modoPotenciometro && (estadoActual == ARRANCANDO || estadoActual == MARCHA_REGIMEN)) {
    leerPotenciometro();
  }

  // 3. Suavizado cinemático continuo de aceleración / desaceleración
  actualizarRampaCinematica();

  // 4. Generación de pulsos para el motor
  if (estadoActual != DETENIDA && estadoActual != PAUSA_INERCIA_INVERSION) {
    generarPulsosPaso();
  }

  // 5. Telemetría compacta periódica (solo 35 bytes para no saturar el buffer UART de 64 bytes)
  if (estadoActual == MARCHA_REGIMEN && (millis() - tiempoUltimaTelemetria >= 3000)) {
    tiempoUltimaTelemetria = millis();
    imprimirTelemetriaCompacta();
  }
}

// ==========================================
// RAMPA TRAPEZOIDAL DE VELOCIDAD SUAVE
// ==========================================
void actualizarRampaCinematica() {
  unsigned long ahora = millis();
  if (ahora - tiempoUltimaRampa < RAMPA_INTERVALO_MS) return;
  tiempoUltimaRampa = ahora;

  switch (estadoActual) {
    case ARRANCANDO:
      // Acelera suavemente reduciendo el delay hasta el objetivo
      if (delayActual > delayObjetivo) {
        if (delayActual >= delayObjetivo + RAMPA_SALTO_US) {
          delayActual -= RAMPA_SALTO_US;
        } else {
          delayActual = delayObjetivo;
        }
      } else if (delayActual < delayObjetivo) {
        if (delayActual + RAMPA_SALTO_US <= delayObjetivo) {
          delayActual += RAMPA_SALTO_US;
        } else {
          delayActual = delayObjetivo;
        }
      } else {
        estadoActual = MARCHA_REGIMEN;
      }
      break;

    case MARCHA_REGIMEN:
      // Ajuste suave ante cambios de velocidad con '+' o '-' en plena marcha
      if (delayActual > delayObjetivo) {
        delayActual = (delayActual >= delayObjetivo + RAMPA_SALTO_US) ? (delayActual - RAMPA_SALTO_US) : delayObjetivo;
      } else if (delayActual < delayObjetivo) {
        delayActual = (delayActual + RAMPA_SALTO_US <= delayObjetivo) ? (delayActual + RAMPA_SALTO_US) : delayObjetivo;
      }
      break;

    case FRENANDO:
      // Desacelera suavemente aumentando el delay hasta la velocidad mínima de corte
      if (delayActual < DELAY_ARRANQUE_US) {
        delayActual += RAMPA_SALTO_US;
      } else {
        // Detención total consumada
        estadoActual = DETENIDA;
        digitalWrite(PIN_PUL, HIGH);
        digitalWrite(PIN_LED, LOW);
        tiempoAcumuladoSeg += (millis() - tiempoInicioMarcha) / 1000;
        Serial.println(F(">> [PARADA]: Motor detenido suavemente (Reposo Half Current activo)."));
      }
      break;

    case FRENANDO_PARA_INVERTIR:
      // Desacelera suavemente antes de cambiar la dirección física
      if (delayActual < DELAY_ARRANQUE_US) {
        delayActual += RAMPA_SALTO_US;
      } else {
        // Alcanzó velocidad mínima: corta pulsos e inicia pausa inercial
        digitalWrite(PIN_PUL, HIGH);
        tiempoPausaInversion = millis();
        estadoActual = PAUSA_INERCIA_INVERSION;
      }
      break;

    case PAUSA_INERCIA_INVERSION:
      // Pausa de amortiguación de 150 ms para disipar la torsión elástica de la manguera y del rotor
      if (millis() - tiempoPausaInversion >= 150) {
        sentidoGiroHardware = sentidoGiroDeseado;
        digitalWrite(PIN_DIR, sentidoGiroHardware);
        delayActual = DELAY_ARRANQUE_US;
        tiempoInicioMarcha = millis();
        estadoActual = ARRANCANDO;
        Serial.println(sentidoGiroHardware == HIGH ? 
          F(">> [INVERSION]: Giro suave iniciado en FILTRACION DIRECTA (CW)") : 
          F(">> [INVERSION]: Giro suave iniciado en RETROLAVADO (CCW)"));
      }
      break;

    case DETENIDA:
      break;
  }
}

// ==========================================
// GENERADOR DE PULSOS ASÍNCRONO NO BLOQUEANTE
// ==========================================
void generarPulsosPaso() {
  unsigned long t = micros();

  if (estadoPinPulso == HIGH) {
    if (t - tiempoUltimoPulso >= delayActual) {
      digitalWrite(PIN_PUL, LOW); // Flanco de bajada activo (Ánodo común)
      estadoPinPulso = LOW;
      tiempoUltimoPulso = t;
    }
  } else {
    if (t - tiempoUltimoPulso >= ANCHO_PULSO_US) {
      digitalWrite(PIN_PUL, HIGH); // Retorno a nivel alto de reposo
      estadoPinPulso = HIGH;
      tiempoUltimoPulso = t;
    }
  }
}

// ==========================================
// PROCESAMIENTO DE COMANDOS UNIFICADOS
// ==========================================
void procesarComando(char c) {
  switch (c) {
    case '1': // Puesta en marcha con Soft-Start
      if (estadoActual == DETENIDA || estadoActual == FRENANDO) {
        digitalWrite(PIN_DIR, sentidoGiroHardware);
        digitalWrite(PIN_LED, HIGH);
        
        delayActual = DELAY_ARRANQUE_US; // Inicia a velocidad baja y suave
        tiempoInicioMarcha = millis();
        tiempoUltimoPulso = micros();
        estadoActual = ARRANCANDO;
        Serial.println(F("\n>> [MARCHA]: Arranque progresivo activado (Soft-Start)..."));
      }
      break;

    case '0': // Parada suave controlada con Soft-Stop
      if (estadoActual != DETENIDA && estadoActual != FRENANDO) {
        estadoActual = FRENANDO;
        Serial.println(F("\n>> [FRENADO]: Desaceleración suave en curso (Soft-Stop)..."));
      }
      break;

    case 'D': // Inversión protegida de sentido de giro
    case 'd':
      sentidoGiroDeseado = !sentidoGiroDeseado;
      if (estadoActual == DETENIDA) {
        sentidoGiroHardware = sentidoGiroDeseado;
        digitalWrite(PIN_DIR, sentidoGiroHardware);
        Serial.println(sentidoGiroHardware == HIGH ? 
          F(">> [MODO PREVIO]: FILTRACION DIRECTA (CW)") : 
          F(">> [MODO PREVIO]: RETROLAVADO (CCW)"));
      } else {
        // Si está en movimiento: Frena progresivamente antes de invertir
        Serial.println(F("\n>> [INVERSION]: Desacelerando suavemente para proteger acople y manguera..."));
        estadoActual = FRENANDO_PARA_INVERTIR;
      }
      break;

    case '+': // Subir velocidad gradualmente (+RPM)
    case 'V':
    case 'v':
      if (modoPotenciometro) {
        Serial.println(F(">> [AVISO]: Desactiva modo potenciómetro ('M') para usar teclado."));
        break;
      }
      if (delayObjetivo > DELAY_MIN_US) {
        if (delayObjetivo >= (DELAY_MIN_US + PASO_AJUSTE_US)) {
          delayObjetivo -= PASO_AJUSTE_US;
        } else {
          delayObjetivo = DELAY_MIN_US;
        }
      }
      Serial.print(F(">> [VELOCIDAD +]: "));
      imprimirTelemetriaCompacta();
      break;

    case '-': // Bajar velocidad gradualmente (-RPM)
      if (modoPotenciometro) {
        Serial.println(F(">> [AVISO]: Desactiva modo potenciómetro ('M') para usar teclado."));
        break;
      }
      if (delayObjetivo < DELAY_MAX_US) {
        delayObjetivo += PASO_AJUSTE_US;
        if (delayObjetivo > DELAY_MAX_US) delayObjetivo = DELAY_MAX_US;
      }
      Serial.print(F(">> [VELOCIDAD -]: "));
      imprimirTelemetriaCompacta();
      break;

    case 'M': // Alternar Potenciómetro en A0 vs Comandos Serie
    case 'm':
      modoPotenciometro = !modoPotenciometro;
      Serial.print(F(">> [CONTROL]: "));
      Serial.println(modoPotenciometro ? F("POTENCIOMETRO EN PIN A0 ACTIVADO") : F("CONTROL POR TECLADO (+/-)"));
      break;

    case 'S': // Reporte completo bajo demanda
    case 's':
      imprimirReporteCompleto();
      break;

    case 'H':
    case 'h':
    case '?':
      imprimirMenuAyuda();
      break;

    default:
      break;
  }
}

// ==========================================
// CONTROL ANALÓGICO POR POTENCIÓMETRO (A0)
// ==========================================
void leerPotenciometro() {
  static unsigned long ultimaLectura = 0;
  if (millis() - ultimaLectura < 80) return;
  ultimaLectura = millis();

  int val = analogRead(PIN_POT);
  if (abs(val - lecturaPotAnterior) > 10) {
    lecturaPotAnterior = val;
    delayObjetivo = map(val, 0, 1023, DELAY_MAX_US, DELAY_MIN_US);
  }
}

// ==========================================
// CÁLCULOS CINEMÁTICOS Y DE CAUDAL (TESIS)
// ==========================================
float calcularFrecuenciaHz(unsigned int d) {
  return 1000000.0 / (float)(d + ANCHO_PULSO_US);
}

float calcularRPM(unsigned int d) {
  return (calcularFrecuenciaHz(d) / (float)PASOS_POR_REV) * 60.0;
}

float calcularCaudalLPM(unsigned int d) {
  // Ecuación de calibración empírica validada de la tesis:
  // Q (L/min) = -0.029 * (delayUs / 10) + 3.858
  float q = -0.029 * ((float)d / 10.0) + 3.858;
  return (q < 0.0) ? 0.0 : q;
}

// ==========================================
// TELEMETRÍA ULTRA-COMPACTA (NO BLOQUEANTE)
// ==========================================
void imprimirTelemetriaCompacta() {
  // Cadena corta de ~35 caracteres: nunca llena el buffer serie de 64 bytes
  Serial.print(F("["));
  Serial.print(calcularRPM(delayActual), 1);
  Serial.print(F(" RPM | "));
  Serial.print(calcularCaudalLPM(delayActual), 2);
  Serial.print(F(" L/min | "));
  Serial.print(sentidoGiroHardware == HIGH ? F("CW") : F("CCW"));
  Serial.println(F("]"));
}

void imprimirReporteCompleto() {
  unsigned long tiempo = tiempoAcumuladoSeg;
  if (estadoActual != DETENIDA) {
    tiempo += (millis() - tiempoInicioMarcha) / 1000;
  }

  Serial.println(F("----------------------------------------------------------"));
  Serial.print(F(" ESTADO MOTOR : "));
  switch (estadoActual) {
    case DETENIDA:                Serial.println(F("DETENIDO (Standstill 50% Corriente)")); break;
    case ARRANCANDO:               Serial.println(F("ARRANCANDO (Rampa Soft-Start)")); break;
    case MARCHA_REGIMEN:           Serial.println(F("REGIMEN ESTABLE CONTINUO")); break;
    case FRENANDO:                 Serial.println(F("FRENANDO (Rampa Soft-Stop)")); break;
    case FRENANDO_PARA_INVERTIR:   Serial.println(F("DESACELERANDO PARA INVERTIR")); break;
    case PAUSA_INERCIA_INVERSION:  Serial.println(F("PAUSA INERCIAL AMORTIGUADA")); break;
  }
  Serial.print(F(" MODO FLUJO   : "));
  Serial.println(sentidoGiroHardware == HIGH ? F("FILTRACION DIRECTA (CW - Horario)") : F("RETROLAVADO (CCW - Antihorario)"));
  Serial.print(F(" VELOCIDAD    : "));
  Serial.print(calcularRPM(delayActual), 2);
  Serial.print(F(" RPM (Frec: "));
  Serial.print(calcularFrecuenciaHz(delayActual), 1);
  Serial.println(F(" Hz)"));
  Serial.print(F(" CAUDAL EST.  : "));
  Serial.print(calcularCaudalLPM(delayActual), 2);
  Serial.println(F(" L/min"));
  Serial.print(F(" RETARDO US   : "));
  Serial.print(delayActual);
  Serial.println(F(" us"));
  Serial.print(F(" TIEMPO MARCHA: "));
  Serial.print(tiempo / 60);
  Serial.print(F(" min "));
  Serial.print(tiempo % 60);
  Serial.println(F(" seg"));
  Serial.println(F("----------------------------------------------------------"));
}

void imprimirMenuAyuda() {
  Serial.println(F("\n--- COMANDOS SERIE (115200 BAUDIOS) ---"));
  Serial.println(F(" '1' : Puesta en marcha suave (Soft-Start progresivo)"));
  Serial.println(F(" '0' : Parada suave (Soft-Stop sin sacudidas)"));
  Serial.println(F(" 'D' : Invertir sentido (Frena -> Pausa inercial -> Gira -> Acelera)"));
  Serial.println(F(" '+' : Aumentar RPM con rampa suave (-150 us)"));
  Serial.println(F(" '-' : Disminuir RPM con rampa suave (+150 us)"));
  Serial.println(F(" 'M' : Alternar Potenciómetro en Pin A0 / Teclado (+/-)"));
  Serial.println(F(" 'S' : Imprimir telemetría completa detallada"));
  Serial.println(F(" 'H' : Ver este menú de ayuda"));
  Serial.println(F("--------------------------------------\n"));
}
