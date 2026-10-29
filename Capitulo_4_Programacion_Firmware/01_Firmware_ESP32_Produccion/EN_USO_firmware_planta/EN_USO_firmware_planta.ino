/* ==============================================================================
 * PLANTA PILOTO DE ULTRAFILTRACIÓN — TESIS INGENIERÍA INDUSTRIAL (UNSa 2026)
 * Tesistas: Antonella Guitián & Owen Cañizares | Codirector: Ing. Enzo Corte
 * Firmware de Control, Medición Continua en Tiempo Real y Monitoreo de Torta
 *
 * FUNCIONES CLAVE PARA EL SEGUIMIENTO DE FORMACIÓN DE TORTA (FOULING):
 * 1. Medición en Tiempo Real Continua (sin probeta):
 *    - Calcula segundo a segundo Q_Alim(t) y Q_Perm(t) con filtro anti-EMI.
 * 2. Monitoreo de Merma de Caudal por Formación de Torta:
 *    - Captura automáticamente el caudal inicial limpio (Q_Alim_0 y Q_Perm_0) al
 *      alcanzar el régimen estable (y permite fijarlo manualmente con un botón).
 *    - Calcula en vivo el porcentaje de caída/merma de Alimentación (% Merma Alim)
 *      por incremento de contrapresión, el porcentaje de merma de Permeado
 *      (% Merma Perm) y el Flujo Normalizado J/J0 (%) a medida que crece la torta.
 * 3. Modo Híbrido en Permeado (Sensor en Tiempo Real + Asistencia por Probeta):
 *    - Si en el tramo final de ensuciamiento el caudal de permeado cae por debajo
 *      del umbral mecánico de giro de la turbina YF-S401 (< 10 mL/min), el operador
 *      puede ingresar un valor puntual medido con probeta en vivo sin detener el
 *      ensayo para que quede registrado en las curvas y en el archivo Excel (.CSV).
 * 4. Sonda Térmica Digital 1-Wire DS18B20 en GPIO 4 (D4):
 *    - Lectura nativa 1-Wire no bloqueante (sin librerías externas) que actualiza
 *      automáticamente temp_agua_C en tiempo real si la sonda está conectada en D4.
 * ============================================================================== */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "config.h"
#include "caudalimetro.h"
#include "registro_ensayos.h"
#include "Bomba.h"
#include "darcy.h"
#include "index_html.h"

DNSServer dnsServer;
constexpr uint16_t DNS_PORT = 53;

// ------------------------------------------------------------------------------
// DRIVER NATIVO 1-WIRE NO BLOQUEANTE PARA SONDA DS18B20 EN GPIO 4 (D4)
// Permite compilar directo en Arduino IDE sin instalar librerías externas.
// ------------------------------------------------------------------------------
namespace DS18B20_Native {
  static portMUX_TYPE owMux = portMUX_INITIALIZER_UNLOCKED;

  // CRC-8 Dallas/Maxim (polinomio x^8 + x^5 + x^4 + 1, 0x8C reflejado)
  static uint8_t crc8(const uint8_t* d, uint8_t n) {
    uint8_t crc = 0;
    while (n--) {
      uint8_t b = *d++;
      for (uint8_t i = 8; i; i--) {
        uint8_t mix = (crc ^ b) & 1;
        crc >>= 1;
        if (mix) crc ^= 0x8C;
        b >>= 1;
      }
    }
    return crc;
  }

  static bool reset() {
    pinMode(PIN_SENSOR_TEMP_DS18B20, OUTPUT);
    digitalWrite(PIN_SENSOR_TEMP_DS18B20, LOW);
    delayMicroseconds(480);
    pinMode(PIN_SENSOR_TEMP_DS18B20, INPUT_PULLUP);
    delayMicroseconds(70);
    bool presence = (digitalRead(PIN_SENSOR_TEMP_DS18B20) == LOW);
    delayMicroseconds(410);
    return presence;
  }

  static void writeBit(uint8_t bit) {
    portENTER_CRITICAL(&owMux);
    pinMode(PIN_SENSOR_TEMP_DS18B20, OUTPUT);
    digitalWrite(PIN_SENSOR_TEMP_DS18B20, LOW);
    if (bit) {
      delayMicroseconds(6);
      pinMode(PIN_SENSOR_TEMP_DS18B20, INPUT_PULLUP);
      delayMicroseconds(64);
    } else {
      delayMicroseconds(60);
      pinMode(PIN_SENSOR_TEMP_DS18B20, INPUT_PULLUP);
      delayMicroseconds(10);
    }
    portEXIT_CRITICAL(&owMux);
  }

  static uint8_t readBit() {
    portENTER_CRITICAL(&owMux);
    pinMode(PIN_SENSOR_TEMP_DS18B20, OUTPUT);
    digitalWrite(PIN_SENSOR_TEMP_DS18B20, LOW);
    delayMicroseconds(3);
    pinMode(PIN_SENSOR_TEMP_DS18B20, INPUT_PULLUP);
    delayMicroseconds(10);
    uint8_t r = digitalRead(PIN_SENSOR_TEMP_DS18B20);
    delayMicroseconds(53);
    portEXIT_CRITICAL(&owMux);
    return r;
  }

  static void writeByte(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
      writeBit(data & 0x01);
      data >>= 1;
    }
  }

  static uint8_t readByte() {
    uint8_t r = 0;
    for (uint8_t i = 0; i < 8; i++) {
      r >>= 1;
      if (readBit()) r |= 0x80;
    }
    return r;
  }

  static bool iniciarConversion() {
    if (!reset()) return false;
    writeByte(0xCC); // Skip ROM
    writeByte(0x44); // Convert T
    return true;
  }

  static bool leerTemperatura(float& tempOut) {
    if (!reset()) return false;
    writeByte(0xCC); // Skip ROM
    writeByte(0xBE); // Read Scratchpad
    uint8_t sp[9];
    for (uint8_t i = 0; i < 9; i++) sp[i] = readByte();
    if (crc8(sp, 8) != sp[8]) return false; // P1-2: Descarta lecturas corruptas por ruido o ISRs
    int16_t raw = (int16_t)((sp[1] << 8) | sp[0]);
    float t = (float)raw / 16.0f;
    // Ignorar valor de power-on reset (85.0 °C exactos) o fuera de rango físico
    if (t < 1.0f || t > 65.0f || fabsf(t - 85.0f) < 0.01f) {
      return false;
    }
    tempOut = t;
    return true;
  }
}

// ------------------------------------------------------------------------------
// ESTRUCTURAS DE DATOS PARA EL DATALOGGER Y MONITOREO DE TORTA
// ------------------------------------------------------------------------------
struct RegistroCalibracion {
  uint16_t id_ensayo;
  bool     en_regimen;     // Empaquetado con id_ensayo y perm_manual (4 bytes exactos)
  bool     perm_manual;    // false = Sensor YF-S401 en tiempo real, true = Asistido por probeta
  uint32_t t_relativo_s;
  float    rpm;
  float    q_bomba;
  float    f_alim;
  float    q_alim;
  float    q_alim_ref;     // Caudal inicial de referencia con filtro limpio (Q_Alim_0)
  float    merma_alim_pct; // % de merma de alimentación por contrapresión de la torta
  float    vol_alim;
  float    f_perm;
  float    q_perm;
  float    q_perm_ref;     // Caudal inicial de permeado con filtro limpio (Q_Perm_0)
  float    merma_perm_pct; // % de merma de permeado por ensuciamiento (1 - J/J0)
  float    j_sobre_j0_pct; // Flujo normalizado J(t)/J0 en %
  float    vol_perm;
  float    q_ret;
  float    recov;
  float    delta;
  float    k_alim;
  float    k_perm;
  float    j_lmh;
  // Subhito 2.4: Presión manométrica primaria y modelo de Darcy en tiempo real
  float    p1_bar;
  float    p2_bar;
  float    tmp_bar;
  float    temp_c;
  float    j20_lmh;
  float    tcf;
  float    r_total;
  float    r_torta;
};

struct EnsayoInfo {
  uint16_t id;
  float    rpm_consigna;
  uint32_t t_inicio_ms;
  uint32_t duracion_s;
  uint16_t muestras;
  float    vol_alim;
  float    vol_perm;
  float    merma_alim_final;
  float    merma_perm_final;
};

// Buffer circular en Heap dinámico (300 registros = 50 minutos a 1 muestra cada 10 s)
// Asignado en setup() para liberar completamente el segmento estático .dram0.bss del ESP32
RegistroCalibracion* bufferLog = nullptr;
size_t bufferHead = 0;
size_t bufferCount = 0;

EnsayoInfo listaEnsayos[MAX_ENSAYOS];
size_t numEnsayos = 0;
uint16_t ensayoActualId = 1;

// Variables de estado del ensayo actual
uint32_t tInicioEnsayo_ms = 0;
float volAlimInicioEnsayo = 0.0f;
float volPermInicioEnsayo = 0.0f;
bool  bombaEnMarchaAnterior = false;

// Variables de Seguimiento de Formación de Torta (Merma en Tiempo Real)
float   qAlimRef_mLmin   = 0.0f;  // Q_Alim_0 (filtro limpio a las RPM actuales)
float   qPermRef_mLmin   = 0.0f;  // Q_Perm_0 (filtro limpio a las RPM actuales)
float   mermaAlim_pct    = 0.0f;  // % caída de Q_Alim respecto a Q_Alim_0
float   mermaPerm_pct    = 0.0f;  // % caída de Q_Perm respecto a Q_Perm_0
float   jSobreJ0_pct     = 100.0f;// % Flujo relativo J(t)/J0
uint8_t segEstablesRef   = 0;     // Contador para fijar Q0 automáticamente tras 5s en régimen

// Soporte para lectura híbrida de Permeado (Sensor en vivo + Asistencia manual a muy bajo caudal)
bool    usarPermManual   = false;
float   qPermManual_mLmin = 0.0f;
double  volPermManualAcum_L = 0.0;

// Variables de Auto-Calibración en Marcha
bool    autoCalibrando = false;
uint8_t autoCalMuestras = 0;
float   autoCalSumFrecAlim = 0.0f;
float   autoCalSumFrecPerm = 0.0f;
String  autoCalMensaje = "";

// Instanciación de componentes principales
Caudalimetro sensorAlimentacion(SENSOR_ALIM_CFG, "ALIMENTACION", true);
Caudalimetro sensorPermeado(SENSOR_PERM_CFG, "PERMEADO", false);
RegistroEnsayos registroEnsayos;
Bomba bomba;
WebServer server(80);
Preferences prefs;
static char ipStr[16] = "192.168.4.1";

// Variables hidráulicas calculadas cada 1 segundo
float qAlimEfectivo_mLmin = 0.0f;
float qPermEfectivo_mLmin = 0.0f;
float qRet_mLmin = 0.0f;
float recuperacion = 0.0f;
float deltaBomba = 0.0f;
float jLMH_actual = 0.0f;
bool  flagCruceSensores = false;

ModeloDarcy modeloDarcy;
ResultadoDarcy resultadoDarcy;

// Variables de Presión Primaria Manométrica y Modelo de Darcy (Subhito 2.4)
float p1_manometro_bar  = 0.30f; // Manómetro Bourdon P1 (Alimentación)
float p2_manometro_bar  = 0.20f; // Manómetro Bourdon P2 (Retentado)
float p3_manometro_bar  = 0.00f; // Contrapresión Permeado / Columna hidrostática
float temp_agua_C       = 20.0f; // Temperatura de proceso (°C) — actualizada por DS18B20 en D4 o Web SCADA
float tmp_efectiva_bar  = 0.25f; // TMP = ((P1 + P2) / 2) - P3
bool  ds18b20_activo    = false;
bool  modoTempAuto      = true;  // true = Medición continua con sonda DS18B20 en D4; false = Manual

uint32_t tLoop = 0;
uint32_t tCaudal = 0;
uint32_t tDatalogger = 0;

// Parámetros y Límites del Modo Administrador
float rpmInicioCustom     = RPM_INICIO; // Consigna inicial al arrancar (50 RPM)
bool  modoParadaSeca      = false;      // false = Rampa suave, true = Parada instantánea
float dimmerArranquePct   = 50.0f;      // 0 a 100% de fluidez/aceleración de arranque
float qAlimMax_mLmin      = 1400.0f;    // Límite caudalímetro alimentación máx
float qAlimMin_mLmin      = 0.0f;       // Límite inferior sequedad (0 = off)
float qPermMax_mLmin      = 800.0f;     // Límite caudalímetro permeado máx
float tempMax_C           = 35.0f;      // Límite temperatura máxima (membrana FX100)
float tmpMax_bar          = 0.60f;      // Límite PTM máxima
bool  autoParadaLimites   = false;      // Parada preventiva automática si se sobrepasan límites

// Parámetros y Monitoreo de Mantenimiento Preventivo
float    horasManguera             = 0.0f;   // Horas de marcha acumuladas de la manguera peristáltica
uint32_t segundosMangueraAcum      = 0;      // Acumulador de segundos para el horímetro
bool     guardianStandby           = true;   // Desplazamiento periódico de rodillos en reposo para evitar aplastamiento
uint32_t tUltimaActividadBomba_ms  = 0;      // Timestamp para reposo del Guardián Standby
constexpr float HORAS_MAX_MANGUERA = 150.0f; // Umbral de fatiga y fatiga superficial recomendada

// ------------------------------------------------------------------------------
// GESTIÓN DE PREFERENCES (MEMORIA FLASH NVS CON AUTO-SANACIÓN)
// ------------------------------------------------------------------------------
void guardarParametrosNVS(float ka, float kp, float ml, uint16_t pul,
                          float rpmIni, bool stopSeco, float acelPct,
                          float qMax, float qMin, float tMax, float ptmMax, bool autoStop) {
  prefs.putFloat("k_alim", ka);
  prefs.putFloat("k_perm", kp);
  prefs.putFloat("ml_rev", ml);
  prefs.putUShort("pul_rev", pul);
  prefs.putFloat("rpm_ini", rpmIni);
  prefs.putBool("stop_seco", stopSeco);
  prefs.putFloat("acel_pct", acelPct);
  prefs.putFloat("q_max", qMax);
  prefs.putFloat("q_min", qMin);
  prefs.putFloat("temp_max", tMax);
  prefs.putFloat("tmp_max", ptmMax);
  prefs.putBool("auto_stop", autoStop);
  Serial.println("[NVS] Todos los parámetros y límites guardados en memoria Flash con éxito.");
}

void guardarParametrosNVS(float ka, float kp, float ml, uint16_t pul) {
  guardarParametrosNVS(ka, kp, ml, pul, rpmInicioCustom, modoParadaSeca, dimmerArranquePct,
                       qAlimMax_mLmin, qAlimMin_mLmin, tempMax_C, tmpMax_bar, autoParadaLimites);
}

void cargarParametrosNVS() {
  prefs.begin("planta_uf", false);
  uint8_t ver = prefs.getUChar("cal_ver", 0);
  float ka = prefs.getFloat("k_alim", K_ALIMENTACION);
  float kp = prefs.getFloat("k_perm", K_PERMEADO);
  float ml = prefs.getFloat("ml_rev", ML_POR_VUELTA);
  uint16_t pul = prefs.getUShort("pul_rev", PULSOS_POR_REV);

  rpmInicioCustom   = prefs.getFloat("rpm_ini", RPM_INICIO);
  modoParadaSeca    = prefs.getBool("stop_seco", false);
  dimmerArranquePct = prefs.getFloat("acel_pct", 50.0f);
  qAlimMax_mLmin    = prefs.getFloat("q_max", 1400.0f);
  qAlimMin_mLmin    = prefs.getFloat("q_min", 0.0f);
  qPermMax_mLmin    = prefs.getFloat("q_perm_max", 800.0f);
  tempMax_C         = prefs.getFloat("temp_max", 35.0f);
  tmpMax_bar        = prefs.getFloat("tmp_max", 0.60f);
  autoParadaLimites = prefs.getBool("auto_stop", false);

  horasManguera     = prefs.getFloat("hrs_mang", 0.0f);
  if (!std::isfinite(horasManguera) || horasManguera < 0.0f) horasManguera = 0.0f;
  guardianStandby   = prefs.getBool("guard_sb", true);
  tUltimaActividadBomba_ms = millis();

  bool actualizarNvs = (ver != VERSION_CALIBRACION);
  if (!std::isfinite(ka) || ka < 50.0f || ka > 500.0f || actualizarNvs) { ka = K_ALIMENTACION; actualizarNvs = true; }
  if (!std::isfinite(kp) || kp < 50.0f || kp > 500.0f || actualizarNvs) { kp = K_PERMEADO;     actualizarNvs = true; }
  if (!std::isfinite(ml) || ml < 2.0f  || ml > 40.0f  || actualizarNvs) { ml = ML_POR_VUELTA;  actualizarNvs = true; }
  if (pul < 200 || pul > 25600)                                         { pul = PULSOS_POR_REV; actualizarNvs = true; }

  if (actualizarNvs) {
    Serial.println("[NVS] Aplicando calibracion oficial Ensayos 2 y 3 (K=205.98 Hz/(L/min), Cilindrada=13.55 mL/rev)...");
    prefs.putUChar("cal_ver", VERSION_CALIBRACION);
    guardarParametrosNVS(ka, kp, ml, pul, rpmInicioCustom, modoParadaSeca, dimmerArranquePct,
                         qAlimMax_mLmin, qAlimMin_mLmin, tempMax_C, tmpMax_bar, autoParadaLimites);
  }

  sensorAlimentacion.setK(ka, fabsf(ka - K_ALIMENTACION) < 0.05f);
  sensorPermeado.setK(kp, false);
  bomba.setMlPorVuelta(ml);
  bomba.setPulsosPorRev(pul);
  bomba.setRpmInicio(rpmInicioCustom);
  bomba.setModoParadaSeca(modoParadaSeca);
  bomba.setSuavidadArranquePct(dimmerArranquePct);

  Serial.printf("\n[NVS] Parametros activos: K_Alim=%.2f | K_Perm=%.2f | mL/rev=%.4f | Pul/Rev=%u | RPM_Ini=%.1f\n",
                ka, kp, ml, pul, rpmInicioCustom);
}

double volumenPermeadoTotal_L() {
  return sensorPermeado.volumen_L() + volPermManualAcum_L;
}

bool bombaGirandoReal() {
  return bomba.enMarcha();
}

float rpmRealConfirmada() {
  return bombaGirandoReal() ? bomba.rpmActual() : 0.0f;
}

float caudalTeoricoConfirmado_mLmin() {
  return bombaGirandoReal() ? bomba.caudalTeorico_mLmin() : 0.0f;
}

// ------------------------------------------------------------------------------
// FUNCIÓN PARA GUARDAR MUESTRA EN EL DATALOGGER (CADA 10 SEGUNDOS)
// ------------------------------------------------------------------------------
void guardarMuestraDatalogger() {
  if (!bufferLog) return;
  if (tInicioEnsayo_ms == 0) {
    tInicioEnsayo_ms = millis();
  }

  uint32_t t_rel_s = (millis() - tInicioEnsayo_ms) / 1000;

  RegistroCalibracion reg;
  reg.id_ensayo      = ensayoActualId;
  reg.t_relativo_s   = t_rel_s;
  reg.rpm            = rpmRealConfirmada();
  reg.en_regimen     = bomba.enRegimenEstable();
  reg.q_bomba        = caudalTeoricoConfirmado_mLmin();
  reg.f_alim         = sensorAlimentacion.frecuencia_Hz();
  reg.q_alim         = qAlimEfectivo_mLmin;
  reg.q_alim_ref     = qAlimRef_mLmin;
  reg.merma_alim_pct = mermaAlim_pct;
  reg.vol_alim       = sensorAlimentacion.volumen_L();
  reg.f_perm         = sensorPermeado.frecuencia_Hz();
  reg.q_perm         = qPermEfectivo_mLmin;
  reg.q_perm_ref     = qPermRef_mLmin;
  reg.merma_perm_pct = mermaPerm_pct;
  reg.j_sobre_j0_pct = jSobreJ0_pct;
  reg.perm_manual    = usarPermManual;
  reg.vol_perm       = (float)volumenPermeadoTotal_L();
  reg.q_ret          = qRet_mLmin;
  reg.recov          = recuperacion;
  reg.delta          = deltaBomba;
  reg.k_alim         = sensorAlimentacion.getK();
  reg.k_perm         = sensorPermeado.getK();
  reg.j_lmh          = jLMH_actual;
  reg.p1_bar         = p1_manometro_bar;
  reg.p2_bar         = p2_manometro_bar;
  reg.tmp_bar        = tmp_efectiva_bar;
  reg.temp_c         = temp_agua_C;
  reg.j20_lmh        = resultadoDarcy.valido ? resultadoDarcy.J20_LMH : jLMH_actual;
  reg.tcf            = resultadoDarcy.valido ? resultadoDarcy.TCF : 1.0f;
  reg.r_total        = resultadoDarcy.valido ? resultadoDarcy.R_total : 0.0f;
  reg.r_torta        = resultadoDarcy.valido ? resultadoDarcy.R_adicional : 0.0f;

  bufferLog[bufferHead] = reg;
  bufferHead = (bufferHead + 1) % MAX_REGISTROS;
  if (bufferCount < MAX_REGISTROS) {
    bufferCount++;
  }

  Serial.printf("[LOG #%u][Ensayo %u] t=%us | RPM=%.1f | Q_Alim=%.1f | Q_Perm=%.1f | TMP=%.2f bar | T=%.1f C | J20=%.2f LMH | R_torta=%.2e m^-1\n",
                (unsigned int)bufferCount, ensayoActualId, t_rel_s, reg.rpm,
                reg.q_alim, reg.q_perm, reg.tmp_bar, reg.temp_c, reg.j20_lmh, reg.r_torta);
}

// ------------------------------------------------------------------------------
// GESTIÓN AUTOMÁTICA DE SESIONES Y ENSAYOS
// ------------------------------------------------------------------------------
void finalizarEnsayoActual() {
  if (!bufferLog) return;
  uint32_t duracion_s = (tInicioEnsayo_ms > 0) ? ((millis() - tInicioEnsayo_ms) / 1000) : 0;
  uint16_t muestrasEnsayo = 0;
  for (size_t i = 0; i < bufferCount; i++) {
    if (bufferLog[i].id_ensayo == ensayoActualId) muestrasEnsayo++;
  }

  if (muestrasEnsayo == 0 && duracion_s >= 5) {
    guardarMuestraDatalogger();
    muestrasEnsayo = 1;
  }

  if (muestrasEnsayo > 0) {
    if (numEnsayos >= MAX_ENSAYOS) {
      for (size_t i = 0; i < MAX_ENSAYOS - 1; i++) {
        listaEnsayos[i] = listaEnsayos[i + 1];
      }
      numEnsayos = MAX_ENSAYOS - 1;
    }

    listaEnsayos[numEnsayos].id               = ensayoActualId;
    listaEnsayos[numEnsayos].rpm_consigna     = bomba.rpmObjetivo();
    listaEnsayos[numEnsayos].t_inicio_ms      = tInicioEnsayo_ms;
    listaEnsayos[numEnsayos].duracion_s       = duracion_s;
    listaEnsayos[numEnsayos].muestras         = muestrasEnsayo;
    listaEnsayos[numEnsayos].vol_alim         = sensorAlimentacion.volumen_L() - volAlimInicioEnsayo;
    listaEnsayos[numEnsayos].vol_perm         = (float)volumenPermeadoTotal_L() - volPermInicioEnsayo;
    listaEnsayos[numEnsayos].merma_alim_final = mermaAlim_pct;
    listaEnsayos[numEnsayos].merma_perm_final = mermaPerm_pct;
    numEnsayos++;

    Serial.printf("\n<<< [ENSAYO #%u FINALIZADO] Consigna: %.1f RPM | Duracion: %us | Merma Alim: %.1f%% | Merma Perm: %.1f%% >>>\n\n",
                  ensayoActualId, bomba.rpmObjetivo(), duracion_s, mermaAlim_pct, mermaPerm_pct);
  }
}

void reiniciarReferenciasTorta() {
  qAlimRef_mLmin = 0.0f;
  qPermRef_mLmin = 0.0f;
  mermaAlim_pct  = 0.0f;
  mermaPerm_pct  = 0.0f;
  jSobreJ0_pct   = 100.0f;
  segEstablesRef = 0;
}

void fijarReferenciasTortaActuales() {
  if (qAlimEfectivo_mLmin > 10.0f) {
    qAlimRef_mLmin = qAlimEfectivo_mLmin;
  } else {
    qAlimRef_mLmin = bomba.caudalTeorico_mLmin();
  }
  if (qPermEfectivo_mLmin > 0.5f) {
    qPermRef_mLmin = qPermEfectivo_mLmin;
  }
  mermaAlim_pct = 0.0f;
  mermaPerm_pct = 0.0f;
  jSobreJ0_pct  = 100.0f;
  Serial.printf("[TORTA] Referencia inicial Q0 fijada: Q_Alim_0 = %.1f mL/min | Q_Perm_0 = %.1f mL/min\n",
                qAlimRef_mLmin, qPermRef_mLmin);
}

void iniciarNuevoEnsayo(float consignaRpm) {
  uint16_t muestrasPrevias = 0;
  if (bufferLog) {
    for (size_t i = 0; i < bufferCount; i++) {
      if (bufferLog[i].id_ensayo == ensayoActualId) muestrasPrevias++;
    }
  }
  if (muestrasPrevias > 0) {
    ensayoActualId++;
  }
  tInicioEnsayo_ms = millis();
  volAlimInicioEnsayo = sensorAlimentacion.volumen_L();
  volPermInicioEnsayo = (float)volumenPermeadoTotal_L();
  reiniciarReferenciasTorta();
  tDatalogger = millis();
  guardarMuestraDatalogger();
  Serial.printf("\n>>> [ENSAYO #%u INICIADO] Consigna: %.1f RPM <<<\n", ensayoActualId, consignaRpm);
}

// ------------------------------------------------------------------------------
// MANEJADORES DE RUTAS DEL SERVIDOR WEB
// ------------------------------------------------------------------------------
void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleCaptivePortal() {
  String host = server.hostHeader();
  if (host.indexOf("192.168.4.1") >= 0 || host.indexOf("bomba.local") >= 0) {
    handleRoot();
  } else {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
  }
}

void handleStatus() {
  uint32_t t_act_s = (tInicioEnsayo_ms > 0) ? ((millis() - tInicioEnsayo_ms) / 1000) : 0;
  uint8_t prog = (uint8_t)((autoCalMuestras * 100) / MUESTRAS_AUTO_CAL);
  if (prog > 100) prog = 100;

  char buf[2560];
  bool sobreQMax = (qAlimEfectivo_mLmin > qAlimMax_mLmin);
  bool subQMin   = (qAlimMin_mLmin > 5.0f && bomba.enMarcha() && bomba.enRegimenEstable() && qAlimEfectivo_mLmin < qAlimMin_mLmin);
  bool sobreTemp = (temp_agua_C > tempMax_C);
  bool sobreTMP  = (tmp_efectiva_bar > tmpMax_bar);

  int n = snprintf(buf, sizeof(buf),
    "{"
    "\"rpm\":%.1f,\"obj_rpm\":%.1f,\"on\":%s,\"inv\":%s,\"dir\":%s,\"en_regimen\":%s,\"emergencia\":%s,"
    "\"ip\":\"%s\",\"alim_ok\":%s,\"perm_ok\":%s,\"f_alim\":%.2f,\"q_alim\":%.1f,\"vol_alim\":%.4f,"
    "\"f_perm\":%.2f,\"q_perm\":%.1f,\"vol_perm\":%.4f,\"q_ret\":%.1f,\"recov\":%.2f,"
    "\"q_alim_ref\":%.1f,\"merma_alim\":%.1f,\"q_perm_ref\":%.1f,\"merma_perm\":%.1f,\"j_j0\":%.1f,"
    "\"perm_manual\":%s,\"q_perm_man\":%.1f,"
    "\"p1\":%.3f,\"p2\":%.3f,\"p3\":%.3f,\"tmp\":%.3f,\"temp\":%.1f,"
    "\"ds18b20\":%s,\"temp_auto\":%s,"
    "\"j20\":%.2f,\"tcf\":%.3f,\"r_tot\":%.3e,\"r_torta\":%.3e,\"darcy_ok\":%s,"
    "\"flan_alim\":%lu,\"val_alim\":%lu,\"gl_alim\":%lu,"
    "\"flan_perm\":%lu,\"val_perm\":%lu,\"gl_perm\":%lu,"
    "\"modo_seco\":%s,\"solo_alim\":%s,\"ruido_seco\":%s,"
    "\"pump_ml\":%.1f,\"delta\":%.2f,\"j_lmh\":%.2f,\"cruce\":%s,"
    "\"k_alim\":%.2f,\"k_perm\":%.2f,\"qmin_alim\":%.2f,\"qmin_perm\":%.2f,\"ml_rev\":%.4f,\"pul_rev\":%u,"
    "\"rpm_ini\":%.1f,\"stop_seco\":%s,\"acel_pct\":%.1f,"
    "\"q_max\":%.1f,\"q_min\":%.1f,\"temp_max\":%.1f,\"tmp_max\":%.2f,\"auto_stop\":%s,"
    "\"lim_q_excedido\":%s,\"lim_temp_excedido\":%s,\"lim_tmp_excedido\":%s,"
    "\"auto_cal\":%s,\"auto_cal_prog\":%u,\"auto_cal_res\":\"%s\","
    "\"mant_modo\":%d,\"mant_txt\":\"%s\",\"mant_t_rest\":%lu,\"guardian_sb\":%s,\"hrs_mang\":%.1f,\"max_hrs_mang\":%.1f,"
    "\"n_logs\":%u,\"ensayo_act\":%u,\"t_ensayo_s\":%lu,"
    "\"heap\":%u,\"maxblk\":%u,"
    "\"ensayos\":[",
    rpmRealConfirmada(), bomba.rpmObjetivo(),
    bomba.enMarcha() ? "true" : "false",
    bomba.invirtiendo() ? "true" : "false",
    bomba.sentidoHorario() ? "true" : "false",
    bomba.enRegimenEstable() ? "true" : "false",
    bomba.enEmergencia() ? "true" : "false",
    ipStr,
    !sensorAlimentacion.sinSenal() ? "true" : "false",
    (!sensorPermeado.sinSenal() || usarPermManual) ? "true" : "false",
    sensorAlimentacion.frecuencia_Hz(), qAlimEfectivo_mLmin, sensorAlimentacion.volumen_L(),
    sensorPermeado.frecuencia_Hz(), qPermEfectivo_mLmin, volumenPermeadoTotal_L(),
    qRet_mLmin, recuperacion,
    qAlimRef_mLmin, mermaAlim_pct, qPermRef_mLmin, mermaPerm_pct, jSobreJ0_pct,
    usarPermManual ? "true" : "false", qPermManual_mLmin,
    p1_manometro_bar, p2_manometro_bar, p3_manometro_bar, tmp_efectiva_bar, temp_agua_C,
    ds18b20_activo ? "true" : "false", modoTempAuto ? "true" : "false",
    resultadoDarcy.valido ? resultadoDarcy.J20_LMH : jLMH_actual,
    resultadoDarcy.valido ? resultadoDarcy.TCF : 1.0f,
    resultadoDarcy.valido ? resultadoDarcy.R_total : 0.0f,
    resultadoDarcy.valido ? resultadoDarcy.R_adicional : 0.0f,
    resultadoDarcy.valido ? "true" : "false",
    (unsigned long)sensorAlimentacion.flancosBrutos(), (unsigned long)sensorAlimentacion.pulsosValidos(), (unsigned long)sensorAlimentacion.glitchesVentana(),
    (unsigned long)sensorPermeado.flancosBrutos(), (unsigned long)sensorPermeado.pulsosValidos(), (unsigned long)sensorPermeado.glitchesVentana(),
    sensorPermeado.modoSeco() ? "true" : "false",
    sensorPermeado.bloqueado() ? "true" : "false",
    sensorPermeado.ruidoDetectadoEnSeco() ? "true" : "false",
    caudalTeoricoConfirmado_mLmin(), deltaBomba, jLMH_actual, flagCruceSensores ? "true" : "false",
    sensorAlimentacion.getK(), sensorPermeado.getK(),
    sensorAlimentacion.getQMinDetectable(), sensorPermeado.getQMinDetectable(),
    bomba.getMlPorVuelta(), bomba.getPulsosPorRev(),
    bomba.getRpmInicio(),
    bomba.getModoParadaSeca() ? "true" : "false",
    bomba.getSuavidadArranquePct(),
    qAlimMax_mLmin, qAlimMin_mLmin, tempMax_C, tmpMax_bar,
    autoParadaLimites ? "true" : "false",
    (sobreQMax || subQMin) ? "true" : "false",
    sobreTemp ? "true" : "false",
    sobreTMP ? "true" : "false",
    autoCalibrando ? "true" : "false", prog, autoCalMensaje.c_str(),
    (int)bomba.modoMantenimiento(), bomba.estadoMantenimientoStr(), (unsigned long)bomba.tiempoRestanteVaiven_s(),
    guardianStandby ? "true" : "false", horasManguera, HORAS_MAX_MANGUERA,
    (unsigned)bufferCount, (unsigned)ensayoActualId, (unsigned long)t_act_s,
    ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)
  );

  if (!autoCalibrando && autoCalMensaje.length() > 0) {
    autoCalMensaje = "";
  }

  if (n < 0 || n >= (int)sizeof(buf)) {
    Serial.println(F("[STATUS] Alerta: JSON supero limite de buffer"));
    n = sizeof(buf) - 1;
  }

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  server.sendContent(buf, n);

  char item[160];
  for (size_t i = 0; i < numEnsayos; i++) {
    int itemLen = snprintf(item, sizeof(item),
      "%s{\"id\":%u,\"rpm\":%.1f,\"duracion_s\":%lu,\"muestras\":%u,\"vol_alim\":%.4f,\"vol_perm\":%.4f,\"merma_a\":%.1f,\"merma_p\":%.1f}",
      (i > 0) ? "," : "",
      listaEnsayos[i].id, listaEnsayos[i].rpm_consigna,
      (unsigned long)listaEnsayos[i].duracion_s,
      listaEnsayos[i].muestras,
      listaEnsayos[i].vol_alim, listaEnsayos[i].vol_perm,
      listaEnsayos[i].merma_alim_final, listaEnsayos[i].merma_perm_final
    );
    if (itemLen >= (int)sizeof(item)) itemLen = sizeof(item) - 1;
    server.sendContent(item, itemLen);
  }

  server.sendContent("]}");
  server.sendContent("");
}

void handleCmd() {
  if (!server.hasArg("act")) {
    server.send(400, "text/plain", "Falta argumento act");
    return;
  }
  String act = server.arg("act");
  if (act == "START") {
    bomba.arrancar();
  } else if (act == "STOP") {
    bomba.detener();
    if (autoCalibrando) {
      autoCalibrando = false;
      autoCalMensaje = "Auto-calibracion cancelada al apagar bomba.";
    }
  } else if (act == "EMERGENCY") {
    bomba.paradaEmergencia();
  } else if (act == "REARM") {
    bomba.rearmarEmergencia();
  } else if (act == "MANT_VAIVEN_START") {
    bomba.iniciarVaiven(60);
    tUltimaActividadBomba_ms = millis();
  } else if (act == "MANT_VAIVEN_STOP") {
    bomba.detenerMantenimiento();
    tUltimaActividadBomba_ms = millis();
  } else if (act == "MANT_JOG_FWD") {
    bomba.iniciarJog(true);
    tUltimaActividadBomba_ms = millis();
  } else if (act == "MANT_JOG_REV") {
    bomba.iniciarJog(false);
    tUltimaActividadBomba_ms = millis();
  } else if (act == "MANT_JOG_STOP") {
    bomba.detenerJog();
    tUltimaActividadBomba_ms = millis();
  } else if (act == "GUARDIAN_ON") {
    guardianStandby = true;
    prefs.putBool("guard_sb", true);
  } else if (act == "GUARDIAN_OFF") {
    guardianStandby = false;
    prefs.putBool("guard_sb", false);
  } else if (act == "RESET_HORAS_MANGUERA") {
    horasManguera = 0.0f;
    segundosMangueraAcum = 0;
    prefs.putFloat("hrs_mang", 0.0f);
  } else if (act == "MODO_SECO_ON") {
    sensorPermeado.setModoSeco(true);
    sensorAlimentacion.setModoSeco(true);
  } else if (act == "MODO_SECO_OFF") {
    sensorPermeado.setModoSeco(false);
    sensorAlimentacion.setModoSeco(false);
  } else if (act == "SOLO_ALIM_ON") {
    sensorPermeado.setBloqueado(true);
    usarPermManual = false;
    qPermManual_mLmin = 0.0f;
  } else if (act == "SOLO_ALIM_OFF") {
    sensorPermeado.setBloqueado(false);
  } else if (act == "FIJAR_Q0") {
    fijarReferenciasTortaActuales();
  } else if (act == "PERM_AUTO") {
    usarPermManual = false;
    qPermManual_mLmin = 0.0f;
  } else if (act == "LIMPIAR_ALARMA_SECO") {
    sensorPermeado.limpiarAlarmaSeco();
    sensorAlimentacion.limpiarAlarmaSeco();
  } else if (act == "DIR") {
    bomba.toggleSentido();
  } else if (act == "RESET_VOL") {
    sensorAlimentacion.resetVolumen();
    sensorPermeado.resetVolumen();
    volPermManualAcum_L = 0.0;
    volAlimInicioEnsayo = 0.0f;
    volPermInicioEnsayo = 0.0f;
  } else if (act == "TEMP_AUTO_ON") {
    modoTempAuto = true;
  } else if (act == "TEMP_AUTO_OFF") {
    modoTempAuto = false;
  } else if (act == "START_ENSAYO") {
    ensayoActualId++;
    tInicioEnsayo_ms = millis();
    volAlimInicioEnsayo = sensorAlimentacion.volumen_L();
    volPermInicioEnsayo = volumenPermeadoTotal_L();
    fijarReferenciasTortaActuales();
    Serial.printf("[ENSAYO] Ensayo #%u INICIADO (Cronómetro a cero, Q0 fijado).\n", ensayoActualId);
  } else if (act == "STOP_ENSAYO") {
    tInicioEnsayo_ms = 0;
    Serial.printf("[ENSAYO] Ensayo #%u FINALIZADO.\n", ensayoActualId);
  }
  server.send(200, "text/plain", "OK");
}

// Endpoint para ingresar caudal de permeado medido con probeta cuando es menor al mínimo del YF-S401
void handleSetPermManual() {
  if (!server.hasArg("qp")) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Falta parametro qp\"}");
    return;
  }
  float qp = server.arg("qp").toFloat();
  if (!std::isfinite(qp) || qp < 0.0f || qp > 500.0f) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Caudal manual fuera de rango (0-500 mL/min)\"}");
    return;
  }
  if (qp == 0.0f) {
    usarPermManual = false;
    qPermManual_mLmin = 0.0f;
  } else {
    usarPermManual = true;
    qPermManual_mLmin = qp;
    if (qPermRef_mLmin <= 0.5f) {
      qPermRef_mLmin = qp; // Si no había referencia Q_Perm_0 previa, adopta la primera lectura
    }
  }
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// Endpoint para ingresar lecturas manométricas y temperatura (Subhito 2.4 - Ley de Darcy)
void handleSetPresion() {
  if (server.hasArg("p1")) {
    float p1 = server.arg("p1").toFloat();
    if (std::isfinite(p1) && p1 >= 0.0f && p1 <= 6.0f) {
      p1_manometro_bar = p1;
    }
  }
  if (server.hasArg("p2")) {
    float p2 = server.arg("p2").toFloat();
    if (std::isfinite(p2) && p2 >= 0.0f && p2 <= 6.0f) {
      p2_manometro_bar = p2;
    }
  }
  if (server.hasArg("p3")) {
    float p3 = server.arg("p3").toFloat();
    if (std::isfinite(p3) && p3 >= 0.0f && p3 <= 2.0f) {
      p3_manometro_bar = p3;
    }
  }
  if (server.hasArg("temp")) {
    float t = server.arg("temp").toFloat();
    if (std::isfinite(t) && t >= 2.0f && t <= 50.0f) {
      temp_agua_C = t;
      modoTempAuto = false; // Si se ingresa manual, se desactiva el seguimiento auto
    }
  }

  tmp_efectiva_bar = fmaxf(0.001f, ((p1_manometro_bar + p2_manometro_bar) / 2.0f) - p3_manometro_bar);
  resultadoDarcy = modeloDarcy.calcular(qPermEfectivo_mLmin, tmp_efectiva_bar, temp_agua_C);

  char json[288];
  snprintf(json, sizeof(json),
    "{\"status\":\"ok\",\"p1\":%.3f,\"p2\":%.3f,\"p3\":%.3f,\"tmp\":%.3f,\"temp\":%.1f,"
    "\"j20\":%.2f,\"tcf\":%.3f,\"r_total\":%.3e,\"r_torta\":%.3e,\"darcy_ok\":%s}",
    p1_manometro_bar, p2_manometro_bar, p3_manometro_bar, tmp_efectiva_bar, temp_agua_C,
    resultadoDarcy.valido ? resultadoDarcy.J20_LMH : jLMH_actual,
    resultadoDarcy.valido ? resultadoDarcy.TCF : 1.0f,
    resultadoDarcy.valido ? resultadoDarcy.R_total : 0.0f,
    resultadoDarcy.valido ? resultadoDarcy.R_adicional : 0.0f,
    resultadoDarcy.valido ? "true" : "false");

  server.send(200, "application/json", json);
}

void handleSetRPM() {
  if (server.hasArg("rpm")) {
    float nuevoRpm = server.arg("rpm").toFloat();
    if (!std::isfinite(nuevoRpm) || nuevoRpm < RPM_MIN || nuevoRpm > RPM_MAX) {
      server.send(400, "text/plain", "ERROR: RPM fuera de rango");
      return;
    }

    if (bomba.setRPM(nuevoRpm)) {
      reiniciarReferenciasTorta();
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "ERROR al ajustar RPM");
    }
  } else {
    server.send(400, "text/plain", "Falta argumento rpm");
  }
}

void handleSetDev() {
  float ka = -1.0f, kp = -1.0f, ml = -1.0f, qmina = -1.0f, qminp = -1.0f;
  int32_t pul = -1;

  if (server.hasArg("ka")) {
    ka = server.arg("ka").toFloat();
    if (!std::isfinite(ka) || ka < 10.0f || ka > 1000.0f) {
      server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Factor K Alimentacion fuera de rango (10-1000)\"}");
      return;
    }
  }
  if (server.hasArg("kp")) {
    kp = server.arg("kp").toFloat();
    if (!std::isfinite(kp) || kp < 10.0f || kp > 1000.0f) {
      server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Factor K Permeado fuera de rango (10-1000)\"}");
      return;
    }
  }
  if (server.hasArg("ml")) {
    ml = server.arg("ml").toFloat();
    if (!std::isfinite(ml) || ml < 1.0f || ml > 50.0f) {
      server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Cilindrada fuera de rango (1-50 mL/rev)\"}");
      return;
    }
  }
  if (server.hasArg("pul")) {
    pul = server.arg("pul").toInt();
    if (pul < 200 || pul > 25600) {
      server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Pulsos/rev fuera de rango (200-25600)\"}");
      return;
    }
  }
  if (server.hasArg("qmina")) {
    qmina = server.arg("qmina").toFloat();
    if (!std::isfinite(qmina) || qmina < 0.0f || qmina > 100.0f) {
      server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Qmin detectable alimentacion invalido\"}");
      return;
    }
  }
  if (server.hasArg("qminp")) {
    qminp = server.arg("qminp").toFloat();
    if (!std::isfinite(qminp) || qminp < 0.0f || qminp > 100.0f) {
      server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Qmin detectable permeado invalido\"}");
      return;
    }
  }

  if (ka > 0.0f && fabsf(ka - sensorAlimentacion.getK()) > 0.005f) sensorAlimentacion.setK(ka);
  if (kp > 0.0f && fabsf(kp - sensorPermeado.getK()) > 0.005f)     sensorPermeado.setK(kp);
  if (ml > 0.0f) bomba.setMlPorVuelta(ml);
  if (pul > 0)   bomba.setPulsosPorRev((uint16_t)pul);
  if (qmina >= 0.0f) sensorAlimentacion.setQMinDetectable(qmina);
  if (qminp >= 0.0f) sensorPermeado.setQMinDetectable(qminp);

  // Parámetros de Administrador (Cinemática, Dimer y Límites)
  if (server.hasArg("rpm_ini")) {
    float r = server.arg("rpm_ini").toFloat();
    if (r >= RPM_MIN && r <= RPM_MAX) {
      rpmInicioCustom = r;
      bomba.setRpmInicio(r);
    }
  }
  if (server.hasArg("stop_seco")) {
    modoParadaSeca = (server.arg("stop_seco") == "1");
    bomba.setModoParadaSeca(modoParadaSeca);
  }
  if (server.hasArg("acel_pct")) {
    float a = server.arg("acel_pct").toFloat();
    if (a >= 5.0f && a <= 100.0f) {
      dimmerArranquePct = a;
      bomba.setSuavidadArranquePct(a);
    }
  }
  if (server.hasArg("q_max")) {
    float qm = server.arg("q_max").toFloat();
    if (qm > 100.0f && qm <= 3000.0f) qAlimMax_mLmin = qm;
  }
  if (server.hasArg("q_min")) {
    float qmin = server.arg("q_min").toFloat();
    if (qmin >= 0.0f && qmin <= 1000.0f) qAlimMin_mLmin = qmin;
  }
  if (server.hasArg("temp_max")) {
    float tm = server.arg("temp_max").toFloat();
    if (tm >= 20.0f && tm <= 60.0f) tempMax_C = tm;
  }
  if (server.hasArg("tmp_max")) {
    float pm = server.arg("tmp_max").toFloat();
    if (pm >= 0.1f && pm <= 2.0f) tmpMax_bar = pm;
  }
  if (server.hasArg("auto_stop")) {
    autoParadaLimites = (server.arg("auto_stop") == "1");
  }

  bool saveNvs = (server.hasArg("save") && server.arg("save") == "1");
  if (saveNvs) {
    guardarParametrosNVS(sensorAlimentacion.getK(), sensorPermeado.getK(),
                         bomba.getMlPorVuelta(), bomba.getPulsosPorRev(),
                         rpmInicioCustom, modoParadaSeca, dimmerArranquePct,
                         qAlimMax_mLmin, qAlimMin_mLmin, tempMax_C, tmpMax_bar, autoParadaLimites);
  }

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleIniciarAutoCal() {
  if (!bomba.enMarcha()) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Enciende la bomba primero\"}");
    return;
  }
  autoCalibrando = true;
  autoCalMuestras = 0;
  autoCalSumFrecAlim = 0.0f;
  autoCalSumFrecPerm = 0.0f;
  autoCalMensaje = "";
  server.send(200, "application/json", "{\"status\":\"iniciada\"}");
}

void handleCancelarAutoCal() {
  autoCalibrando = false;
  autoCalMensaje = "Auto-calibracion cancelada.";
  server.send(200, "application/json", "{\"status\":\"cancelada\"}");
}

void handleCalibrarRpmQ() {
  if (!server.hasArg("rpm") || !server.hasArg("qa")) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Faltan argumentos (requiere rpm y qa)\"}");
    return;
  }

  float rpm = server.arg("rpm").toFloat();
  float qa  = server.arg("qa").toFloat();
  float qp  = server.hasArg("qp") ? server.arg("qp").toFloat() : 0.0f;

  if (rpm <= 0.0f || qa <= 0.0f || !std::isfinite(rpm) || !std::isfinite(qa) || !std::isfinite(qp) || qp < 0.0f) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Valores numericos invalidos\"}");
    return;
  }

  float nuevaCilindrada = qa / rpm;
  if (nuevaCilindrada >= 1.0f && nuevaCilindrada <= 50.0f) {
    bomba.setMlPorVuelta(nuevaCilindrada);
  }

  float fa = server.hasArg("fa") ? server.arg("fa").toFloat() : sensorAlimentacion.frecuencia_Hz();
  float fp = server.hasArg("fp") ? server.arg("fp").toFloat() : sensorPermeado.frecuencia_Hz();

  bool kaOk = false;
  bool kpOk = false;

  if (fa > 1.0f) {
    float nuevoKa = (fa * 1000.0f) / qa;
    if (nuevoKa >= 20.0f && nuevoKa <= 600.0f) {
      kaOk = sensorAlimentacion.setK(nuevoKa);
    }
  }
  if (qp > 0.0f && fp > 0.35f && !sensorPermeado.bloqueado()) {
    float nuevoKp = (fp * 1000.0f) / qp;
    if (nuevoKp >= 20.0f && nuevoKp <= 600.0f) {
      kpOk = sensorPermeado.setK(nuevoKp);
    }
  }

  guardarParametrosNVS(sensorAlimentacion.getK(), sensorPermeado.getK(),
                       bomba.getMlPorVuelta(), bomba.getPulsosPorRev());

  char json[192];
  snprintf(json, sizeof(json),
    "{\"status\":\"ok\",\"ml_rev\":%.4f,\"k_alim\":%.2f,\"k_perm\":%.2f,\"ka_actualizado\":%s,\"kp_actualizado\":%s}",
    bomba.getMlPorVuelta(), sensorAlimentacion.getK(), sensorPermeado.getK(),
    kaOk ? "true" : "false", kpOk ? "true" : "false");

  server.send(200, "application/json", json);
}

void handleResetDev() {
  sensorAlimentacion.setK(K_ALIMENTACION, true);
  sensorPermeado.setK(K_PERMEADO, false);
  bomba.setMlPorVuelta(ML_POR_VUELTA);
  bomba.setPulsosPorRev(PULSOS_POR_REV);
  rpmInicioCustom   = RPM_INICIO;
  modoParadaSeca    = false;
  dimmerArranquePct = 50.0f;
  qAlimMax_mLmin    = 1400.0f;
  qAlimMin_mLmin    = 0.0f;
  tempMax_C         = 35.0f;
  tmpMax_bar        = 0.60f;
  autoParadaLimites = false;

  bomba.setRpmInicio(RPM_INICIO);
  bomba.setModoParadaSeca(false);
  bomba.setSuavidadArranquePct(50.0f);

  guardarParametrosNVS(K_ALIMENTACION, K_PERMEADO, ML_POR_VUELTA, PULSOS_POR_REV,
                       RPM_INICIO, false, 50.0f, 1400.0f, 0.0f, 35.0f, 0.60f, false);
  server.send(200, "application/json", "{\"status\":\"reset_ok\"}");
}

// Exportación a Excel (.CSV) con columnas de Merma por Formación de Torta
void handleExportCSV() {
  String targetEnsayo = server.hasArg("ensayo") ? server.arg("ensayo") : "all";

  String filename = "PlantaUF_MonitoreoTorta_";
  if (targetEnsayo == "all") {
    filename += "HistoricoCompleto.csv";
  } else if (targetEnsayo == "actual") {
    filename += "Ensayo_" + String(ensayoActualId) + "_EnVivo.csv";
  } else {
    filename += "Ensayo_" + targetEnsayo + ".csv";
  }

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader("Content-Type", "text/csv; charset=UTF-8");
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
  server.sendHeader("Connection", "close");
  server.send(200, "text/csv; charset=UTF-8", "");

  server.sendContent("\xEF\xBB\xBFsep=;\n"
                     "PLANTA DE ULTRAFILTRACION FX100 - MONITOREO EN TIEMPO REAL DE MERMA DE CAUDAL Y TORTA (UNSa 2026)\n"
                     "Ensayo_ID;Tiempo_s;Tiempo_MinSec;RPM_Bomba;Estable_1_0;Q_Bomba_Teorico_mLmin;"
                     "Frec_Alim_Hz;Q_Alim_mLmin;Q_Alim_Inicial_mLmin;Merma_Alim_Torta_Pct;Vol_Alim_L;"
                     "Modo_Perm;Frec_Perm_Hz;Q_Perm_mLmin;Q_Perm_Inicial_mLmin;Merma_Perm_Torta_Pct;Flujo_Relativo_J_J0_Pct;Vol_Perm_L;"
                     "Q_Retentado_mLmin;Recuperacion_Y_Pct;Desviacion_Bomba_Pct;K_Alim;K_Perm;J_LMH;"
                     "P1_Alim_bar;P2_Ret_bar;TMP_bar;Temp_C;J20_LMH;TCF;R_Total_m1;R_Torta_m1\n");

  uint16_t filtroId = 0;
  if (targetEnsayo == "actual") {
    filtroId = ensayoActualId;
  } else if (targetEnsayo != "all") {
    filtroId = (uint16_t)targetEnsayo.toInt();
  }

  if (!bufferLog) {
    server.sendContent("ERROR: Buffer no inicializado.\n");
    return;
  }

  char fila[450];
  char blk[1400];
  size_t used = 0;

  for (size_t i = 0; i < bufferCount; i++) {
    size_t idx = (bufferHead + MAX_REGISTROS - bufferCount + i) % MAX_REGISTROS;
    const RegistroCalibracion& r = bufferLog[idx];

    if (filtroId > 0 && r.id_ensayo != filtroId) {
      continue;
    }

    uint32_t mins = r.t_relativo_s / 60;
    uint32_t secs = r.t_relativo_s % 60;

    int len = snprintf(fila, sizeof(fila),
      "%u;%lu;%02lu:%02lu;%.1f;%d;%.1f;"
      "%.2f;%.1f;%.1f;%.2f;%.4f;"
      "%s;%.2f;%.1f;%.1f;%.2f;%.2f;%.4f;"
      "%.1f;%.2f;%.2f;%.2f;%.2f;%.2f;"
      "%.3f;%.3f;%.3f;%.1f;%.2f;%.3f;%.4e;%.4e\n",
      (unsigned)r.id_ensayo, (unsigned long)r.t_relativo_s, (unsigned long)mins, (unsigned long)secs,
      r.rpm, r.en_regimen ? 1 : 0, r.q_bomba,
      r.f_alim, r.q_alim, r.q_alim_ref, r.merma_alim_pct, r.vol_alim,
      r.perm_manual ? "PROBETA_MANUAL" : "SENSOR_TIEMPO_REAL",
      r.f_perm, r.q_perm, r.q_perm_ref, r.merma_perm_pct, r.j_sobre_j0_pct, r.vol_perm,
      r.q_ret, r.recov, r.delta, r.k_alim, r.k_perm, r.j_lmh,
      r.p1_bar, r.p2_bar, r.tmp_bar, r.temp_c, r.j20_lmh, r.tcf, r.r_total, r.r_torta
    );

    if (len > 0) {
      if ((size_t)len >= sizeof(fila)) len = sizeof(fila) - 1;
      if (used + len > sizeof(blk)) {
        server.sendContent(blk, used);
        used = 0;
      }
      memcpy(blk + used, fila, len);
      used += len;
    }

    if ((i & 15) == 0) yield();
  }

  if (used > 0) {
    server.sendContent(blk, used);
    used = 0;
  }

  server.sendContent("");
}

void handleClearCSV() {
  bufferHead = 0;
  bufferCount = 0;
  numEnsayos = 0;
  ensayoActualId = 1;
  tInicioEnsayo_ms = bomba.enMarcha() ? millis() : 0;
  volAlimInicioEnsayo = sensorAlimentacion.volumen_L();
  volPermInicioEnsayo = (float)volumenPermeadoTotal_L();
  reiniciarReferenciasTorta();
  registroEnsayos.limpiar();
  server.send(200, "text/plain", "LOGS_CLEARED");
}

class ServerPrintAdapter : public Print {
public:
  WebServer& _s;
  char _buf[1024];
  size_t _idx = 0;

  ServerPrintAdapter(WebServer& s) : _s(s) {}

  size_t write(uint8_t c) override {
    _buf[_idx++] = (char)c;
    if (_idx >= sizeof(_buf) - 1) {
      _buf[_idx] = '\0';
      _s.sendContent(_buf, _idx);
      _idx = 0;
    }
    return 1;
  }

  void flush() {
    if (_idx > 0) {
      _buf[_idx] = '\0';
      _s.sendContent(_buf, _idx);
      _idx = 0;
    }
  }
};

void handleExportMetrologia() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader("Content-Type", "text/csv; charset=UTF-8");
  server.sendHeader("Content-Disposition", "attachment; filename=\"PlantaUF_Metrologia_Completa.csv\"");
  server.sendHeader("Connection", "close");
  server.send(200, "text/csv; charset=UTF-8", "");
  ServerPrintAdapter adapter(server);
  registroEnsayos.exportarCSV(adapter);
  adapter.flush();
  server.sendContent("");
}

// ------------------------------------------------------------------------------
// SETUP DEL MICROCONTROLADOR ESP32
// ------------------------------------------------------------------------------
void setup() {
  // P0-1: Driver deshabilitado y líneas de control seguras desde el primer instante físico
  pinMode(PIN_ENA, OUTPUT);
  digitalWrite(PIN_ENA, LOW);  // Driver listo (habilitado para arrancar)
  pinMode(PIN_PUL, OUTPUT);
  digitalWrite(PIN_PUL, LOW);  // Pin de pulsos amarrado a GND sin oscilación ni glitches
  pinMode(PIN_DIR, OUTPUT);
  digitalWrite(PIN_DIR, LOW);  // Dirección predeterminada (Horario / Filtración)

  Serial.begin(115200);
  delay(500);

  Serial.println("\n==================================================");
  Serial.println(" PLANTA PILOTO DE ULTRAFILTRACIÓN FX100 — UNSa   ");
  Serial.println(" Monitoreo en Tiempo Real de Formación de Torta   ");
  Serial.println("==================================================");

  pinMode(PIN_LED_BOMBA, OUTPUT);
  digitalWrite(PIN_LED_BOMBA, LOW);

  // Asignación segura del buffer de datalogger en Heap (elimina 74.4 KB de .dram0.bss)
  bufferLog = new (std::nothrow) RegistroCalibracion[MAX_REGISTROS]{};
  if (bufferLog) {
    Serial.printf("[DATALOGGER] Buffer circular de %u registros asignado en Heap (%u bytes).\n",
                  (unsigned)MAX_REGISTROS, (unsigned)(MAX_REGISTROS * sizeof(RegistroCalibracion)));
  } else {
    Serial.println("[DATALOGGER] ERROR: No se pudo asignar memoria para bufferLog.");
  }

  // Inicializar sonda térmica DS18B20 en GPIO 4 (D4) con pull-up externo de 4.7 kOhm a 3.3V
  pinMode(PIN_SENSOR_TEMP_DS18B20, INPUT_PULLUP);
  if (DS18B20_Native::iniciarConversion()) {
    ds18b20_activo = true;
    Serial.printf("[DS18B20] Sonda termica 1-Wire detectada en GPIO %u (D4).\n", PIN_SENSOR_TEMP_DS18B20);
  } else {
    Serial.printf("[DS18B20] Sin respuesta en GPIO %u (D4). Usando temperatura manual por defecto (%.1f C).\n",
                  PIN_SENSOR_TEMP_DS18B20, temp_agua_C);
  }

  const bool inicioAlim = sensorAlimentacion.begin();
  const bool inicioPerm = sensorPermeado.begin();
  if (!inicioAlim || !inicioPerm) {
    Serial.println("⚠️ [ERROR CRÍTICO] Fallo en la inicialización de caudalímetros");
  }
  bomba.begin();
  registroEnsayos.begin();

  cargarParametrosNVS();

  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(SSID_AP, PASS_AP, 1, 0, 4);
  strlcpy(ipStr, WiFi.softAPIP().toString().c_str(), sizeof(ipStr));

  Serial.println("[WIFI] Punto de Acceso Estable Creado:");
  Serial.printf("       SSID: %s | Pass: %s\n", SSID_AP, PASS_AP);
  Serial.printf("       IP AP: http://%s\n", WiFi.softAPIP().toString().c_str());

  if (MDNS.begin("bomba")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("[mDNS] Servidor publicado en: http://bomba.local");
  }

  dnsServer.start(DNS_PORT, "*", local_IP);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/cmd", HTTP_GET, handleCmd);
  server.on("/set", HTTP_GET, handleSetRPM);
  server.on("/set_perm_manual", HTTP_GET, handleSetPermManual);
  server.on("/set_presion", HTTP_GET, handleSetPresion);
  server.on("/set_dev", HTTP_GET, handleSetDev);
  server.on("/reset_dev", HTTP_GET, handleResetDev);
  server.on("/iniciar_auto_cal", HTTP_GET, handleIniciarAutoCal);
  server.on("/cancelar_auto_cal", HTTP_GET, handleCancelarAutoCal);
  server.on("/calibrar_rpm_q", HTTP_GET, handleCalibrarRpmQ);
  server.on("/export_csv", HTTP_GET, handleExportCSV);
  server.on("/export_metrologia", HTTP_GET, handleExportMetrologia);
  server.on("/clear_csv", HTTP_GET, handleClearCSV);

  server.on("/generate_204", HTTP_GET, handleCaptivePortal);
  server.on("/gen_204", HTTP_GET, handleCaptivePortal);
  server.on("/ncsi.txt", HTTP_GET, handleCaptivePortal);
  server.on("/connecttest.txt", HTTP_GET, handleCaptivePortal);
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptivePortal);
  server.onNotFound(handleCaptivePortal);

  server.begin();
  Serial.println("[HTTP] Servidor Web SCADA iniciado con exito en puerto 80.\n");

  tLoop = millis();
  tCaudal = millis();
  tDatalogger = millis();
  tInicioEnsayo_ms = 0;
}

// ------------------------------------------------------------------------------
// BUCLE PRINCIPAL (LOOP NO BLOQUEANTE)
// ------------------------------------------------------------------------------
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  uint32_t tAhora = millis();

  // 1. Control cinemático de la bomba cada 50 ms
  if (tAhora - tLoop >= 50) {
    float dt = (tAhora - tLoop) / 1000.0f;
    tLoop = tAhora;
    bomba.tick(dt);

    digitalWrite(PIN_LED_BOMBA, (bomba.rpmActual() > 0.5f) ? HIGH : LOW);

    bool enMarcha = bomba.enMarcha();

    if (enMarcha && !bombaEnMarchaAnterior && !bomba.enMantenimiento()) {
      iniciarNuevoEnsayo(bomba.rpmObjetivo());
      registroEnsayos.iniciar(sensorAlimentacion, sensorPermeado, bomba.rpmActual());
    }

    if (!enMarcha && bombaEnMarchaAnterior && !bomba.enMantenimiento()) {
      finalizarEnsayoActual();
      registroEnsayos.finalizar(sensorAlimentacion, sensorPermeado, bomba.rpmActual());
    }

    bombaEnMarchaAnterior = enMarcha;
  }

  // 2. Adquisición en tiempo real y cálculo de merma por torta cada 1000 ms (1 segundo)
  if (tAhora - tCaudal >= 1000) {
    tCaudal = tAhora;

    // Actualización de horímetro y standby de manguera
    if (bomba.rpmActual() > 1.0f) {
      tUltimaActividadBomba_ms = millis();
      segundosMangueraAcum++;
      if (segundosMangueraAcum >= 60) {
        segundosMangueraAcum = 0;
        horasManguera += (1.0f / 60.0f);
        static uint8_t minGuardar = 0;
        if (++minGuardar >= 10) {
          minGuardar = 0;
          prefs.putFloat("hrs_mang", horasManguera);
        }
      }
    } else if (guardianStandby && !bomba.enEmergencia() && !bomba.enMantenimiento() && !bomba.enMarcha()) {
      if (millis() - tUltimaActividadBomba_ms >= 7200000UL) { // 2 horas de reposo
        tUltimaActividadBomba_ms = millis();
        bomba.ejecutarGuardianShift();
        Serial.println(F("[MANTENIMIENTO] Guardián Standby: Rotación preventiva de 120° ejecutada para aliviar compresión estática."));
      }
    }

    // Lectura asíncrona no bloqueante de la sonda DS18B20 en GPIO 4 (D4) con filtro anti-parpadeo
    static uint8_t fallosConsecutivosDS18 = 0;
    float tLeida = 0.0f;
    if (DS18B20_Native::leerTemperatura(tLeida)) {
      ds18b20_activo = true;
      fallosConsecutivosDS18 = 0;
      if (modoTempAuto) {
        temp_agua_C = tLeida;
      }
    } else {
      if (++fallosConsecutivosDS18 >= 3) {
        ds18b20_activo = false;
      }
    }
    DS18B20_Native::iniciarConversion(); // Inicia la conversión para el próximo segundo

    bool bombaEmpuja = (bomba.rpmActual() > 1.0f);
    float qBomba = bomba.caudalTeorico_mLmin();

    // Captura atómica única por segundo
    const auto ma = sensorAlimentacion.capturar(bombaEmpuja);
    const auto mp = sensorPermeado.capturar(bombaEmpuja);
    registroEnsayos.tick(ma, mp, bomba.rpmActual());

    qAlimEfectivo_mLmin = sensorAlimentacion.caudal_mLmin();
    float qPermSensor   = sensorPermeado.caudal_mLmin();

    if (usarPermManual && bombaEmpuja && !sensorPermeado.bloqueado()) {
      qPermEfectivo_mLmin = qPermManual_mLmin;
      volPermManualAcum_L += (double)qPermManual_mLmin / 60000.0;
    } else {
      qPermEfectivo_mLmin = qPermSensor;
    }

    // Captura automática de Caudales Iniciales (Q_Alim_0 y Q_Perm_0) tras 5 segundos en Régimen Estable
    if (bomba.enRegimenEstable()) {
      if (segEstablesRef < 10) segEstablesRef++;
      if (segEstablesRef >= 5) {
        if (qAlimRef_mLmin <= 1.0f && qAlimEfectivo_mLmin > 20.0f) {
          qAlimRef_mLmin = qAlimEfectivo_mLmin;
          Serial.printf("[TORTA] Q_Alim_0 inicial capturado automaticamente: %.1f mL/min\n", qAlimRef_mLmin);
        }
        if (qPermRef_mLmin <= 0.1f && qPermEfectivo_mLmin > 1.0f) {
          qPermRef_mLmin = qPermEfectivo_mLmin;
          Serial.printf("[TORTA] Q_Perm_0 inicial capturado automaticamente: %.1f mL/min\n", qPermRef_mLmin);
        }
      }
    } else {
      segEstablesRef = 0;
    }

    // Cálculo en tiempo real de la Merma de Caudal por Formación de Torta
    if (bombaEmpuja && qAlimRef_mLmin > 10.0f) {
      float caidaA = ((qAlimRef_mLmin - qAlimEfectivo_mLmin) / qAlimRef_mLmin) * 100.0f;
      mermaAlim_pct = fmaxf(0.0f, caidaA);
    } else {
      mermaAlim_pct = 0.0f;
    }

    if (bombaEmpuja && qPermRef_mLmin > 0.5f) {
      float caidaP = ((qPermRef_mLmin - qPermEfectivo_mLmin) / qPermRef_mLmin) * 100.0f;
      mermaPerm_pct = fmaxf(0.0f, caidaP);
      jSobreJ0_pct  = constrain((qPermEfectivo_mLmin / qPermRef_mLmin) * 100.0f, 0.0f, 150.0f);
    } else {
      mermaPerm_pct = 0.0f;
      jSobreJ0_pct  = 100.0f;
    }

    flagCruceSensores = (qPermEfectivo_mLmin > qAlimEfectivo_mLmin + 50.0f) && (bomba.rpmActual() > 5.0f);

    // Balance Hidráulico Tangencial y Flujo Darcy en tiempo real
    qRet_mLmin   = fmaxf(0.0f, qAlimEfectivo_mLmin - qPermEfectivo_mLmin);
    recuperacion = (qAlimEfectivo_mLmin > 50.0f) ? ((qPermEfectivo_mLmin / qAlimEfectivo_mLmin) * 100.0f) : 0.0f;
    jLMH_actual  = (qPermEfectivo_mLmin * 0.06f) / AREA_MEMBRANA_M2;
    resultadoDarcy = modeloDarcy.calcular(qPermEfectivo_mLmin, tmp_efectiva_bar, temp_agua_C);
    deltaBomba   = (qBomba > 1.0f) ? (((qAlimEfectivo_mLmin - qBomba) / qBomba) * 100.0f) : 0.0f;

    // Máquina de estados de Auto-Calibración en régimen permanente
    if (autoCalibrando) {
      if (bomba.enRegimenEstable()) {
        autoCalSumFrecAlim += sensorAlimentacion.frecuencia_Hz();
        autoCalSumFrecPerm += sensorPermeado.frecuencia_Hz();
        autoCalMuestras++;

        if (autoCalMuestras >= MUESTRAS_AUTO_CAL) {
          float fPromAlim = autoCalSumFrecAlim / (float)MUESTRAS_AUTO_CAL;
          float qRefAlim  = bomba.rpmActual() * bomba.getMlPorVuelta();

          if (qRefAlim > 10.0f && fPromAlim > 1.0f) {
            float nuevoKa = (fPromAlim * 1000.0f) / qRefAlim;
            sensorAlimentacion.setK(nuevoKa);
          }

          float kpActual = sensorPermeado.getK();
          guardarParametrosNVS(sensorAlimentacion.getK(), kpActual,
                               bomba.getMlPorVuelta(), bomba.getPulsosPorRev(),
                               rpmInicioCustom, modoParadaSeca, dimmerArranquePct,
                               qAlimMax_mLmin, qAlimMin_mLmin, tempMax_C, tmpMax_bar, autoParadaLimites);

          autoCalibrando = false;
          autoCalMensaje = "✅ Auto-Calibracion OK: K_Alim=" + String(sensorAlimentacion.getK(), 2) + " Hz/(L/min)";
        }
      }
    }

    // Verificación de enclavamiento por límites de seguridad configurados en Modo Administrador
    bool sobreQMax = (qAlimEfectivo_mLmin > qAlimMax_mLmin);
    bool subQMin   = (qAlimMin_mLmin > 5.0f && bomba.enMarcha() && bomba.enRegimenEstable() && qAlimEfectivo_mLmin < qAlimMin_mLmin);
    bool sobreTemp = (temp_agua_C > tempMax_C);
    bool sobreTMP  = (tmp_efectiva_bar > tmpMax_bar);
    if (autoParadaLimites && bomba.enMarcha() && (sobreQMax || subQMin || sobreTemp || sobreTMP)) {
      bomba.detener();
      Serial.println(F("[SEGURIDAD] Parada preventiva automática activada por límites de proceso (Q, Temp o TMP)."));
    }

    Serial.printf("[TELEMETRIA] RPM:%4.1f | Q_Alim:%5.1f mL/min | Q_Perm:%5.1f mL/min | T:%4.1f C | TMP:%.2f bar | J20:%4.2f LMH | R_torta:%.2e m^-1\n",
                  bomba.rpmActual(), qAlimEfectivo_mLmin,
                  qPermEfectivo_mLmin, temp_agua_C, tmp_efectiva_bar,
                  resultadoDarcy.valido ? resultadoDarcy.J20_LMH : jLMH_actual,
                  resultadoDarcy.valido ? resultadoDarcy.R_adicional : 0.0f);
  }

  // 3. Muestreo del Datalogger cada 10 segundos (SOLO cuando la bomba gira realmente y hay flujo)
  if (tAhora - tDatalogger >= INTERVALO_LOG_MS) {
    tDatalogger = tAhora;
    if (bombaGirandoReal()) {
      guardarMuestraDatalogger();
    }
  }

  delay(2);
}
