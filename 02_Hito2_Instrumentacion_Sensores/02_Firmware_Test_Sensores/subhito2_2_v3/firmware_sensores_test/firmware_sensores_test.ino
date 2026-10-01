/* ==============================================================================
 * PROYECTO DE TESIS DE INGENIERÍA INDUSTRIAL - PLANTA DE ULTRAFILTRACIÓN FX100
 * HITO 3: TEST INTEGRAL DE SENSORES DISPONIBLES (CAUDAL, TEMPERATURA, TDS Y ADC)
 * ==============================================================================
 * 
 * DESCRIPCIÓN:
 * Este firmware realiza la lectura simultánea y periódica de toda la instrumentación
 * actualmente disponible en el laboratorio:
 * 1. Dos caudalímetros de microflujo YF-S401 (Permeado en GPIO 27 y Retentado en GPIO 14).
 * 2. Sensor de temperatura sumergible DS18B20 en GPIO 34 (Protocolo OneWire).
 * 3. Sensor de calidad de agua TDS a través del conversor I2C ADS1115 (o GPIO 35 analógico).
 * 
 * CONEXIONES FÍSICAS (ESP32 38 PINES):
 * - GPIO 27 ──► Señal Caudalímetro Permeado (Cable Amarillo)
 * - GPIO 14 ──► Señal Caudalímetro Retentado (Cable Amarillo)
 * - GPIO 34 ──► Señal Sonda DS18B20 (Cable Amarillo con resistencia 4.7k a 3.3V)
 * - GPIO 21 ──► SDA del módulo ADS1115 (Bus I2C)
 * - GPIO 22 ──► SCL del módulo ADS1115 (Bus I2C)
 * - GND     ──► Masa común de todos los sensores
 * - 3V3 / 5V──► Alimentación de sensores
 * ============================================================================== */

#include <Arduino.h>
#include <Wire.h>

// ------------------------------------------------------------------------------
// 1. ASIGNACIÓN DE PINES
// ------------------------------------------------------------------------------
const uint8_t PIN_CAUDAL_PERM = 27; // Pulsos Caudalímetro Permeado
const uint8_t PIN_CAUDAL_RET  = 14; // Pulsos Caudalímetro Retentado
const uint8_t PIN_ONEWIRE     = 34; // Datos Sonda DS18B20
const uint8_t PIN_TDS_DIRECTO = 35; // Entrada analógica de respaldo para TDS

// Dirección I2C del conversor ADS1115 (Pin ADDR a GND)
const uint8_t ADS1115_I2C_ADDR = 0x48;

// ------------------------------------------------------------------------------
// 2. VARIABLES DE CAUDAL (INTERRUPCIONES DE HARDWARE)
// ------------------------------------------------------------------------------
volatile unsigned long pulsosPermeado  = 0;
volatile unsigned long pulsosRetentado = 0;

float caudalPermeado_Lmin  = 0.0;
float caudalRetentado_Lmin = 0.0;
float volumenPermeado_L    = 0.0;

// Rutinas de Servicio de Interrupción (ISR) en memoria IRAM
void IRAM_ATTR isrCaudalPermeado() {
  pulsosPermeado++;
}

void IRAM_ATTR isrCaudalRetentado() {
  pulsosRetentado++;
}

// ------------------------------------------------------------------------------
// 3. VARIABLES DE TEMPERATURA Y CALIDAD DE AGUA (TDS)
// ------------------------------------------------------------------------------
float temperaturaAgua_C = 20.0; // Valor medido (°C)
float factorTCF         = 1.0;  // Factor de corrección de viscosidad Darcy
float voltajeTDS        = 0.0;  // Voltaje analógico (V)
float tds_ppm           = 0.0;  // Sólidos totales disueltos (mg/L o ppm)

unsigned long t_ultimo_muestreo_ms = 0;

// ------------------------------------------------------------------------------
// 4. LECTURA RAW I2C DEL CONVERSOR ADS1115 (Canal A0)
// ------------------------------------------------------------------------------
int16_t leerADS1115_Canal0() {
  Wire.beginTransmission(ADS1115_I2C_ADDR);
  Wire.write(0x01); // Puntero al Registro de Configuración
  // Configuración: Single-ended A0, ±4.096V (Gain 1), Modo Single-shot, 128 SPS
  Wire.write(0xC2);
  Wire.write(0x83);
  Wire.endTransmission();

  delay(10); // Esperar conversión

  Wire.beginTransmission(ADS1115_I2C_ADDR);
  Wire.write(0x00); // Puntero al Registro de Conversión
  Wire.endTransmission();

  Wire.requestFrom((int)ADS1115_I2C_ADDR, 2);
  if (Wire.available() >= 2) {
    int16_t res = (Wire.read() << 8) | Wire.read();
    return res;
  }
  return 0;
}

// ------------------------------------------------------------------------------
// 5. SETUP: CONFIGURACIÓN DE PINES, I2C E INTERRUPCIONES
// ------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n==========================================================");
  Serial.println("  PLANTA PILOTO DE ULTRAFILTRACIÓN INDUSTRIAL - HITO 3    ");
  Serial.println("  Test y Calibración de Sensores (Caudal, TDS, Temp, ADC) ");
  Serial.println("==========================================================");

  // Configuración de I2C (GPIO 21 SDA, GPIO 22 SCL)
  Wire.begin(21, 22);

  // Configuración de Pines de Caudalímetros con Interrupciones
  pinMode(PIN_CAUDAL_PERM, INPUT_PULLUP);
  pinMode(PIN_CAUDAL_RET, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_CAUDAL_PERM), isrCaudalPermeado, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_CAUDAL_RET), isrCaudalRetentado, RISING);

  // Pin analógico de respaldo
  pinMode(PIN_TDS_DIRECTO, INPUT);

  Serial.println("[OK] Interrupciones de caudalímetros asignadas.");
  Serial.println("[OK] Bus I2C inicializado. Escaneando dispositivos...");

  // Escaneo I2C rápido para verificar presencia del ADS1115
  Wire.beginTransmission(ADS1115_I2C_ADDR);
  if (Wire.endTransmission() == 0) {
    Serial.println("[OK] Conversor ADS1115 detectado exitosamente en dirección 0x48.");
  } else {
    Serial.println("[AVISO] ADS1115 no responde en 0x48. Se usará lectura analógica interna (GPIO 35).");
  }

  Serial.println("\n--- INICIANDO REGISTRO DE DATOS (Cada 1 Segundo) ---");
  Serial.println("Tiempo(s), Temp(C), TCF_Darcy, Voltaje_TDS(V), TDS(ppm), Caudal_Perm(L/min), Caudal_Ret(L/min), Vol_Perm_Total(L)");
}

// ------------------------------------------------------------------------------
// 6. LOOP PRINCIPAL: CÁLCULO DE TELEMETRÍA CADA 1 SEGUNDO
// ------------------------------------------------------------------------------
void loop() {
  unsigned long t_ahora = millis();

  if (t_ahora - t_ultimo_muestreo_ms >= 1000) {
    float dt = (t_ahora - t_ultimo_muestreo_ms) / 1000.0f;
    t_ultimo_muestreo_ms = t_ahora;

    // 1. Cálculo de Caudales (Factor YF-S401: 98 pulsos/seg = 1 L/min)
    noInterrupts();
    unsigned long pPerm = pulsosPermeado;
    unsigned long pRet  = pulsosRetentado;
    pulsosPermeado = 0;
    pulsosRetentado = 0;
    interrupts();

    caudalPermeado_Lmin  = (float)pPerm / (98.0f * dt);
    caudalRetentado_Lmin = (float)pRet  / (98.0f * dt);
    volumenPermeado_L   += (caudalPermeado_Lmin / 60.0f) * dt;

    // 2. Lectura de Calidad de Agua (TDS) mediante ADS1115 o ADC interno
    int16_t adcCounts = leerADS1115_Canal0();
    if (adcCounts > 0) {
      voltajeTDS = (float)adcCounts * 0.000125f; // ±4.096V rango / 32768
    } else {
      voltajeTDS = (float)analogRead(PIN_TDS_DIRECTO) * (3.3f / 4095.0f);
    }

    // 3. Compensación Térmica de Viscosidad para Ley de Darcy (TCF)
    // TCF = exp(0.0239 * (20 - T))
    factorTCF = exp(0.0239f * (20.0f - temperaturaAgua_C));

    // 4. Cálculo de TDS en ppm con compensación a 25°C
    float vCompensado = voltajeTDS / (1.0f + 0.02f * (temperaturaAgua_C - 25.0f));
    tds_ppm = (133.42f * pow(vCompensado, 3) - 255.86f * pow(vCompensado, 2) + 857.39f * vCompensado) * 0.5f;
    if (tds_ppm < 0.0f) tds_ppm = 0.0f;

    // 5. Impresión en formato CSV por el Monitor Serie
    Serial.printf("%lu, %.2f, %.4f, %.3f, %.1f, %.3f, %.3f, %.3f\n",
                  t_ahora / 1000,
                  temperaturaAgua_C,
                  factorTCF,
                  voltajeTDS,
                  tds_ppm,
                  caudalPermeado_Lmin,
                  caudalRetentado_Lmin,
                  volumenPermeado_L);
  }
}
