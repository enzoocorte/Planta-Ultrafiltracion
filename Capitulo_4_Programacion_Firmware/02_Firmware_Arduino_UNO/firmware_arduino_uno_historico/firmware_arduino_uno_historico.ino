/*
 * =========================================================================================
 * UNIVERSIDAD NACIONAL DE SALTA (UNSa) - FACULTAD DE INGENIERÍA
 * Escuela de Ingeniería Industrial
 * Proyecto Final de Grado: Módulo de Ultrafiltración FX100 - Bomba Peristáltica MBP-2000
 * 
 * CONTROL CINEMÁTICO SUAVE DE MOTOR NEMA 34 CON DRIVER INDUSTRIAL DM860
 * Adaptación Optimizada para Plataforma: Arduino Uno (ATmega328P - 16 MHz)
 * Rango de Velocidad Ampliado: Hasta 200+ RPM con Rampa de Aceleración y Soft-Stop
 * 
 * Tesistas:  Braian Owen Alexis Cañizares y María Antonella Guitian Mónico
 * Directores: Dr. Ing. Jorge Emilio Almazán e Ing. Enzo Marcelo Corte
 * =========================================================================================
 * 
 * CONFIGURACIÓN DEL DRIVER INDUSTRIAL DM860:
 *   - Resolución:     1600 pulsos/rev (1/8 paso) -> SW5: ON,  SW6: OFF, SW7: ON, SW8: ON
 *   - Reposo Térmico: Half Current al 50%       -> SW4: OFF (Hacia ARRIBA)
 *   - Corriente Fase: ~2.4 A a 3.14 A RMS       -> SW1: OFF, SW2: ON,  SW3: ON
 * 
 * CONEXIÓN ELÉCTRICA EN ÁNODO COMÚN (+5V NATIVO DE ARDUINO UNO):
 *   - Borne 5V Arduino Uno  -----> Bornes PUL+ y DIR+ del DM860 (Ánodo Común 5V)
 *   - Pin D9 Arduino Uno    -----> Borne PUL- del DM860 (Flanco activo en nivel BAJO)
 *   - Pin D8 Arduino Uno    -----> Borne DIR- del DM860 (HIGH = Horario/Filtración, LOW = Antihorario/Retrolavado)
 *   - Pin D13 Arduino Uno   -----> LED testigo integrado en la placa Arduino Uno
 *   - Borne GND Arduino Uno -----> GND común de la electrónica de control
 *   - Bornes ENA+ y ENA-    -----> DESCONECTADOS (Driver habilitado por defecto, reposo por SW4)
 * 
 * CONEXIÓN DEL MOTOR NEMA 34 (4.5 Nm - Bipolar Serie):
 *   - Fase A: Borne A+ (Rojo), Borne A- (Negro), Empalme aislado (Amarillo + Azul)
 *   - Fase B: Borne B+ (Blanco), Borne B- (Verde), Empalme aislado (Naranja + Marrón)
 * 
 * PARÁMETROS CINEMÁTICOS DE VELOCIDAD (con 1600 pulsos/rev):
 *   - 3000 us -> ~12.3 RPM (Arranque con alto par / torque y preset 20 RPM nominal)
 *   - 1200 us -> ~30.8 RPM (Preset 50 RPM según curva de flujo)
 *   -  575 us -> ~64.3 RPM (Preset 100 RPM)
 *   -  137 us ->  200.0 RPM (Régimen de alta velocidad para ensayos dinámicos)
 *   -  100 us -> ~250.0 RPM (Límite máximo seguro del sistema cinemático)
 * =========================================================================================
 */

// ==========================================
// ASIGNACIÓN DE PINES
// ==========================================
const uint8_t PIN_PUL = 9;   // Señal de pulsos hacia PUL- (Activo en LOW)
const uint8_t PIN_DIR = 8;   // Control de dirección hacia DIR- (HIGH: CW, LOW: CCW)
const uint8_t PIN_LED = 13;  // LED indicador integrado en Arduino Uno

// ==========================================
// PARÁMETROS CINEMÁTICOS (HASTA 200+ RPM)
// ==========================================
const unsigned int DELAY_MIN_US       = 100;   // Retardo mínimo (~250 RPM, límite seguro)
const unsigned int DELAY_MAX_US       = 6000;  // Retardo máximo (~6.2 RPM, límite inferior)
const unsigned int DELAY_ARRANQUE_US  = 3000;  // Retardo suave de inicio (~12.3 RPM, alto torque)
const unsigned int PASO_AJUSTE_US     = 100;   // Salto de ajuste fino por comando (+ / - / V)
const unsigned int ANCHO_PULSO_US     = 30;    // Ancho del pulso en LOW (mínimo 2.5 us requerido por DM860)
const unsigned int PASOS_POR_REV      = 1600;  // Micropasos configurados en el DM860

// Parámetros de rampa cinemática suave (Soft-Start y Soft-Stop)
const unsigned int RAMPA_SALTO_US      = 15;   // Variación de microsegundos por ciclo de rampa
const unsigned long RAMPA_INTERVALO_MS = 5;    // Intervalo de actualización de rampa (cada 5 ms)

// ==========================================
// MÁQUINA DE ESTADOS CINEMÁTICA ASÍNCRONA
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
unsigned int delayObjetivo = DELAY_ARRANQUE_US;
unsigned int delayActual   = DELAY_ARRANQUE_US;

bool sentidoGiroDeseado    = HIGH; // HIGH = Filtración (Horario / CW), LOW = Retrolavado (Antihorario / CCW)
bool sentidoGiroHardware   = HIGH;
bool estadoPinPulso        = HIGH; // Reposo en HIGH (Ánodo común a 5V)

unsigned long tiempoUltimoPulso     = 0;
unsigned long tiempoUltimaRampa     = 0;
unsigned long tiempoPausaInversion   = 0;
unsigned long tiempoUltimaTelemetria = 0;
unsigned long tiempoInicioMarcha    = 0;
unsigned long tiempoAcumuladoSeg    = 0;

// Prototipos de funciones
void procesarComando(char c);
void actualizarRampaCinematica();
void generarTrenPulsos();
void fijarDelayConRampa(unsigned int nuevoDelay);
void imprimirTelemetriaCompacta();
void imprimirReporteCompleto();
void imprimirMenuAyuda();
float calcularFrecuenciaHz(unsigned int d);
float calcularRPM(unsigned int d);
float calcularCaudalLPM(unsigned int d);

// ==========================================
// CONFIGURACIÓN INICIAL (SETUP)
// ==========================================
void setup() {
  pinMode(PIN_PUL, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_LED, OUTPUT);

  // Estado seguro inicial (Ánodo Común a 5V: PUL en HIGH mantiene apagado el optoacoplador)
  digitalWrite(PIN_PUL, HIGH);
  digitalWrite(PIN_DIR, sentidoGiroHardware);
  digitalWrite(PIN_LED, LOW);

  Serial.begin(115200);
  while (!Serial && millis() < 1500);

  Serial.println(F("\n=========================================================="));
  Serial.println(F(" UNSa - FACULTAD DE INGENIERIA | TESIS ULTRAFILTRACION"));
  Serial.println(F(" Modulo FX100 - Bomba Peristaltica MBP-2000 - Motor NEMA 34"));
  Serial.println(F(" Driver DM860: 1600 p/rev | Anodo Comun 5V | Hasta 200+ RPM"));
  Serial.println(F("=========================================================="));
  imprimirMenuAyuda();
  imprimirReporteCompleto();
}

// ==========================================
// BUCLE PRINCIPAL (LOOP 100% NO BLOQUEANTE)
// ==========================================
void loop() {
  // 1. Recepción y procesamiento de comandos serie
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c != '\r' && c != '\n') {
      procesarComando(c);
    }
  }

  // 2. Control suave de aceleración y desaceleración (Rampa no bloqueante)
  actualizarRampaCinematica();

  // 3. Generación continua de pulsos para el motor (con reinicio limpio de temporizador)
  if (estadoActual != DETENIDA && estadoActual != PAUSA_INERCIA_INVERSION) {
    generarTrenPulsos();
  }

  // 4. Telemetría compacta periódica cada 2000 ms (protege el buffer serie de 64 bytes)
  if (estadoActual == MARCHA_REGIMEN && (millis() - tiempoUltimaTelemetria >= 2000)) {
    tiempoUltimaTelemetria = millis();
    imprimirTelemetriaCompacta();
  }
}

// ==========================================
// ACTUALIZACIÓN DE LA RAMPA CINEMÁTICA
// ==========================================
void actualizarRampaCinematica() {
  unsigned long ahora = millis();
  if (ahora - tiempoUltimaRampa < RAMPA_INTERVALO_MS) return;
  tiempoUltimaRampa = ahora;

  switch (estadoActual) {
    case ARRANCANDO:
      // Acelera suavemente reduciendo el delay hasta el valor objetivo
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
      // Ajuste suave ante cambios dinámicos de consigna (+, -, presets)
      if (delayActual > delayObjetivo) {
        delayActual = (delayActual >= delayObjetivo + RAMPA_SALTO_US) ? (delayActual - RAMPA_SALTO_US) : delayObjetivo;
      } else if (delayActual < delayObjetivo) {
        delayActual = (delayActual + RAMPA_SALTO_US <= delayObjetivo) ? (delayActual + RAMPA_SALTO_US) : delayObjetivo;
      }
      break;

    case FRENANDO:
      // Desacelera suavemente (Soft-Stop) aumentando el delay hasta velocidad mínima de corte
      // Evita el retroceso elástico del rotor y la manguera de silicona
      if (delayActual < DELAY_ARRANQUE_US) {
        delayActual += RAMPA_SALTO_US;
      } else {
        // Detención total consumada
        estadoActual = DETENIDA;
        digitalWrite(PIN_PUL, HIGH);
        digitalWrite(PIN_LED, LOW);
        tiempoAcumuladoSeg += (millis() - tiempoInicioMarcha) / 1000;
        Serial.println(F(">> [PARADA]: Motor detenido con Soft-Stop (Reposo Half Current activo)."));
      }
      break;

    case FRENANDO_PARA_INVERTIR:
      // Desacelera antes de cambiar la dirección física
      if (delayActual < DELAY_ARRANQUE_US) {
        delayActual += RAMPA_SALTO_US;
      } else {
        digitalWrite(PIN_PUL, HIGH);
        tiempoPausaInversion = millis();
        estadoActual = PAUSA_INERCIA_INVERSION;
      }
      break;

    case PAUSA_INERCIA_INVERSION:
      // Pausa inercial amortiguada de 150 ms para disipar la torsión mecánica
      if (millis() - tiempoPausaInversion >= 150) {
        sentidoGiroHardware = sentidoGiroDeseado;
        digitalWrite(PIN_DIR, sentidoGiroHardware);
        delayActual = DELAY_ARRANQUE_US;
        tiempoInicioMarcha = millis();
        estadoActual = ARRANCANDO;
        Serial.println(sentidoGiroHardware == HIGH ? 
          F(">> [INVERSION]: Giro iniciado en FILTRACION DIRECTA (Horario - CW)") : 
          F(">> [INVERSION]: Giro iniciado en RETROLAVADO (Antihorario - CCW)"));
      }
      break;

    case DETENIDA:
      break;
  }
}

// ==========================================
// GENERADOR DE PULSOS ASÍNCRONO NO BLOQUEANTE
// ==========================================
void generarTrenPulsos() {
  unsigned long t = micros();

  if (estadoPinPulso == HIGH) {
    if ((unsigned long)(t - tiempoUltimoPulso) >= delayActual) {
      digitalWrite(PIN_PUL, LOW); // Flanco de bajada activo (Ánodo común a 5V)
      estadoPinPulso = LOW;
      tiempoUltimoPulso = t;      // Reinicio limpio de contador
    }
  } else {
    if ((unsigned long)(t - tiempoUltimoPulso) >= ANCHO_PULSO_US) {
      digitalWrite(PIN_PUL, HIGH); // Retorno a reposo en nivel alto
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
    case '1': // Puesta en marcha suave (Soft-Start)
      if (estadoActual == DETENIDA || estadoActual == FRENANDO) {
        digitalWrite(PIN_DIR, sentidoGiroHardware);
        digitalWrite(PIN_LED, HIGH);
        
        delayActual = DELAY_ARRANQUE_US;
        tiempoInicioMarcha = millis();
        tiempoUltimoPulso = micros();
        estadoActual = ARRANCANDO;
        Serial.println(F("\n>> [MARCHA]: Arranque progresivo activado (Soft-Start)..."));
      }
      break;

    case '0': // Parada suave controlada (Soft-Stop)
      if (estadoActual != DETENIDA && estadoActual != FRENANDO) {
        estadoActual = FRENANDO;
        Serial.println(F("\n>> [FRENADO]: Desaceleracion suave en curso (Soft-Stop)..."));
      }
      break;

    case 'D': // Inversión controlada de sentido de giro
    case 'd':
      sentidoGiroDeseado = !sentidoGiroDeseado;
      if (estadoActual == DETENIDA) {
        sentidoGiroHardware = sentidoGiroDeseado;
        digitalWrite(PIN_DIR, sentidoGiroHardware);
        Serial.println(sentidoGiroHardware == HIGH ? 
          F(">> [MODO PREVIO]: FILTRACION DIRECTA (Horario - CW)") : 
          F(">> [MODO PREVIO]: RETROLAVADO (Antihorario - CCW)"));
      } else {
        Serial.println(F("\n>> [INVERSION]: Desacelerando para proteger acople y manguera..."));
        estadoActual = FRENANDO_PARA_INVERTIR;
      }
      break;

    case '+': // Subir velocidad gradualmente (+RPM / -Delay)
    case 'V':
    case 'v':
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

    case '-': // Bajar velocidad gradualmente (-RPM / +Delay)
      if (delayObjetivo < DELAY_MAX_US) {
        delayObjetivo += PASO_AJUSTE_US;
        if (delayObjetivo > DELAY_MAX_US) delayObjetivo = DELAY_MAX_US;
      }
      Serial.print(F(">> [VELOCIDAD -]: "));
      imprimirTelemetriaCompacta();
      break;

    // Presets rápidos de velocidad
    case '2': // Preset 20 RPM (delay 3000 us)
      fijarDelayConRampa(3000);
      Serial.println(F(">> [PRESET]: Consigna fijada en 20 RPM (3000 us)"));
      break;

    case '5': // Preset 50 RPM (delay 1200 us)
      fijarDelayConRampa(1200);
      Serial.println(F(">> [PRESET]: Consigna fijada en 50 RPM (1200 us)"));
      break;

    case '9': // Preset 100 RPM (delay 575 us)
      fijarDelayConRampa(575);
      Serial.println(F(">> [PRESET]: Consigna fijada en 100 RPM (575 us)"));
      break;

    case 'X': // Preset 200 RPM (delay 137 us)
    case 'x':
      fijarDelayConRampa(137);
      Serial.println(F(">> [PRESET]: Consigna fijada en 200 RPM (137 us)"));
      break;

    case 'S': // Reporte completo de telemetría bajo demanda
    case 's':
      imprimirReporteCompleto();
      break;

    case 'H': // Menú de comandos disponibles
    case 'h':
    case '?':
      imprimirMenuAyuda();
      break;

    default:
      break;
  }
}

// ==========================================
// FIJAR RETARDO OBJETIVO CON TRANSICIÓN SUAVE
// ==========================================
void fijarDelayConRampa(unsigned int nuevoDelay) {
  if (nuevoDelay < DELAY_MIN_US) nuevoDelay = DELAY_MIN_US;
  if (nuevoDelay > DELAY_MAX_US) nuevoDelay = DELAY_MAX_US;
  delayObjetivo = nuevoDelay;
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
  // Modelo empírico experimental de calibración validado en la tesis:
  // Q (L/min) = -0.029 * (delayUs / 10.0) + 3.858
  float q = -0.029 * ((float)d / 10.0) + 3.858;
  if (q < 0.05) q = 0.05;
  return q;
}

// ==========================================
// TELEMETRÍA COMPACTA (MENOS DE 40 BYTES)
// ==========================================
void imprimirTelemetriaCompacta() {
  Serial.print(F("["));
  Serial.print(calcularRPM(delayActual), 1);
  Serial.print(F(" RPM | "));
  Serial.print(calcularCaudalLPM(delayActual), 2);
  Serial.print(F(" L/min | "));
  Serial.print(calcularFrecuenciaHz(delayActual), 1);
  Serial.print(F(" Hz | "));
  Serial.print(delayActual);
  Serial.print(F(" us | "));
  Serial.print(sentidoGiroHardware == HIGH ? F("CW") : F("CCW"));
  Serial.println(F("]"));
}

// ==========================================
// REPORTE COMPLETO DE ESTADO
// ==========================================
void imprimirReporteCompleto() {
  unsigned long tiempo = tiempoAcumuladoSeg;
  if (estadoActual != DETENIDA) {
    tiempo += (millis() - tiempoInicioMarcha) / 1000;
  }

  Serial.println(F("----------------------------------------------------------"));
  Serial.print(F(" ESTADO MOTOR : "));
  switch (estadoActual) {
    case DETENIDA:                Serial.println(F("DETENIDO (Standstill 50% Corriente SW4)")); break;
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
  Serial.print(F(" us (Objetivo: "));
  Serial.print(delayObjetivo);
  Serial.println(F(" us)"));
  Serial.print(F(" TIEMPO MARCHA: "));
  Serial.print(tiempo / 60);
  Serial.print(F(" min "));
  Serial.print(tiempo % 60);
  Serial.println(F(" seg"));
  Serial.println(F("----------------------------------------------------------"));
}

// ==========================================
// MENÚ DE AYUDA Y COMANDOS SERIE
// ==========================================
void imprimirMenuAyuda() {
  Serial.println(F("\n--- COMANDOS SERIE (115200 BAUDIOS) ---"));
  Serial.println(F(" '1' : Puesta en marcha con rampa suave (Soft-Start)"));
  Serial.println(F(" '0' : Parada controlada sin golpe inercial (Soft-Stop)"));
  Serial.println(F(" 'D' : Invertir sentido (Desacelera -> Pausa -> Invierte -> Acelera)"));
  Serial.println(F(" '+' o 'V' : Subir RPM con rampa (-100 us)"));
  Serial.println(F(" '-'       : Bajar RPM con rampa (+100 us)"));
  Serial.println(F(" '2' : Preset rapido 20 RPM (3000 us)"));
  Serial.println(F(" '5' : Preset rapido 50 RPM (1200 us)"));
  Serial.println(F(" '9' : Preset rapido 100 RPM (575 us)"));
  Serial.println(F(" 'X' : Preset rapido 200 RPM (137 us)"));
  Serial.println(F(" 'S' : Imprimir reporte completo detallado"));
  Serial.println(F(" 'H' o '?' : Mostrar este menu de ayuda"));
  Serial.println(F("--------------------------------------\n"));
}
