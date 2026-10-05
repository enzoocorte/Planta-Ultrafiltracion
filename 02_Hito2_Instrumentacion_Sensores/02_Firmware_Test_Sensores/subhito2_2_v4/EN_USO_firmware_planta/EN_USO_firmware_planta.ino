/* ==============================================================================
 * PLANTA PILOTO DE ULTRAFILTRACIÓN — TESIS INGENIERÍA INDUSTRIAL (UNSa 2026)
 * Firmware de Control, Adquisición, Modo Desarrollador, Auto-Calibración y Datalogger
 * Arquitectura C++ Optimizada y Wi-Fi SoftAP de Alta Estabilidad (Anti-Desconexión)
 * ============================================================================== */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "config.h"
#include "caudalimetro.h"
#include "Bomba.h"
#include "darcy.h"
#include "index_html.h"

// Servidor DNS para Portal Cautivo Anti-Desconexión en Android / iOS / Windows
DNSServer dnsServer;
constexpr uint16_t DNS_PORT = 53;

// ------------------------------------------------------------------------------
// ESTRUCTURAS DE DATOS PARA EL DATALOGGER Y GESTIÓN DE ENSAYOS
// ------------------------------------------------------------------------------
struct RegistroCalibracion {
  uint8_t  id_ensayo;
  uint32_t t_relativo_s;
  float    rpm;
  float    q_bomba;
  float    f_alim;
  float    q_alim;
  float    vol_alim;
  float    f_perm;
  float    q_perm;
  float    vol_perm;
  float    q_ret;
  float    recov;
  float    delta;
  float    k_alim;
  float    k_perm;
  float    j_lmh;
};

struct EnsayoInfo {
  uint8_t  id;
  float    rpm_consigna;
  uint32_t t_inicio_ms;
  uint32_t duracion_s;
  uint16_t muestras;
  float    vol_alim;
  float    vol_perm;
};

// Buffers de almacenamiento en RAM
RegistroCalibracion bufferLog[MAX_REGISTROS];
size_t numRegistros = 0;

EnsayoInfo listaEnsayos[MAX_ENSAYOS];
size_t numEnsayos = 0;
uint8_t ensayoActualId = 1;

// Variables de estado del ensayo actual
uint32_t tInicioEnsayo_ms = 0;
float volAlimInicioEnsayo = 0.0f;
float volPermInicioEnsayo = 0.0f;
bool  bombaEnMarchaAnterior = false;

// Variables de Auto-Calibración en Marcha
bool    autoCalibrando = false;
uint8_t autoCalMuestras = 0;
float   autoCalSumFrecAlim = 0.0f;
float   autoCalSumFrecPerm = 0.0f;
String  autoCalMensaje = "";

// Instanciación de componentes (Alimentación con alarma de corte; Permeado sin alarma en reposo)
Caudalimetro sensorAlimentacion(PIN_SENSOR_ALIMENTACION, K_ALIMENTACION, "ALIMENTACION", true);
Caudalimetro sensorPermeado(PIN_SENSOR_PERMEADO, K_PERMEADO, "PERMEADO", false);
Bomba bomba;
WebServer server(80);
Preferences prefs;

// Variables hidráulicas y de control
float qRet_mLmin = 0.0f;
float recuperacion = 0.0f;
float deltaBomba = 0.0f;
float jLMH_actual = 0.0f;
bool  flagCruceSensores = false;

ModeloDarcy modeloDarcy;
ResultadoDarcy resultadoDarcy;

uint32_t tLoop = 0;
uint32_t tCaudal = 0;
uint32_t tDatalogger = 0;

// ------------------------------------------------------------------------------
// GESTIÓN DE PREFERENCES (MEMORIA FLASH NVS)
// ------------------------------------------------------------------------------
void cargarParametrosNVS() {
  prefs.begin("planta_uf", false);
  float ka = prefs.getFloat("k_alim", K_ALIMENTACION);
  float kp = prefs.getFloat("k_perm", K_PERMEADO);
  float ml = prefs.getFloat("ml_rev", ML_POR_VUELTA);
  uint16_t pul = prefs.getUShort("pul_rev", PULSOS_POR_REV);

  sensorAlimentacion.setK(ka);
  sensorPermeado.setK(kp);
  bomba.setMlPorVuelta(ml);
  bomba.setPulsosPorRev(pul);

  Serial.printf("\n[NVS] Parametros cargados: K_Alim=%.2f | K_Perm=%.2f | mL/rev=%.4f | Pul/Rev=%u\n",
                ka, kp, ml, pul);
}

void guardarParametrosNVS(float ka, float kp, float ml, uint16_t pul) {
  prefs.putFloat("k_alim", ka);
  prefs.putFloat("k_perm", kp);
  prefs.putFloat("ml_rev", ml);
  prefs.putUShort("pul_rev", pul);
  Serial.println("[NVS] Parametros guardados en memoria Flash con exito.");
}

// ------------------------------------------------------------------------------
// FUNCIÓN PARA GUARDAR MUESTRA EN EL DATALOGGER (CADA 10 SEGUNDOS)
// ------------------------------------------------------------------------------
void guardarMuestraDatalogger() {
  if (tInicioEnsayo_ms == 0) {
    tInicioEnsayo_ms = millis();
  }

  uint32_t t_rel_s = (millis() - tInicioEnsayo_ms) / 1000;

  // Desplazamiento FIFO circular si el buffer se llena
  if (numRegistros >= MAX_REGISTROS) {
    for (size_t i = 0; i < MAX_REGISTROS - 1; i++) {
      bufferLog[i] = bufferLog[i + 1];
    }
    numRegistros = MAX_REGISTROS - 1;
  }

  RegistroCalibracion reg;
  reg.id_ensayo    = ensayoActualId;
  reg.t_relativo_s = t_rel_s;
  reg.rpm          = bomba.rpmActual();
  reg.q_bomba      = bomba.caudalTeorico_mLmin();
  reg.f_alim       = sensorAlimentacion.frecuencia_Hz();
  reg.q_alim       = sensorAlimentacion.caudal_mLmin();
  reg.vol_alim     = sensorAlimentacion.volumen_L();
  reg.f_perm       = sensorPermeado.frecuencia_Hz();
  reg.q_perm       = sensorPermeado.caudal_mLmin();
  reg.vol_perm     = sensorPermeado.volumen_L();
  reg.q_ret        = qRet_mLmin;
  reg.recov        = recuperacion;
  reg.delta        = deltaBomba;
  reg.k_alim       = sensorAlimentacion.getK();
  reg.k_perm       = sensorPermeado.getK();
  reg.j_lmh        = (reg.q_perm * 0.06f) / AREA_MEMBRANA_M2;

  bufferLog[numRegistros++] = reg;

  Serial.printf("[LOG #%u][Ensayo %u] t=%us | RPM=%.1f | Q_Alim=%.1f mL/min | Q_Perm=%.1f mL/min | J=%.2f LMH | Y=%.1f%%\n",
                (unsigned int)numRegistros, ensayoActualId, t_rel_s, reg.rpm, reg.q_alim, reg.q_perm, reg.j_lmh, reg.recov);
}

// ------------------------------------------------------------------------------
// MANEJADORES DE RUTAS DEL SERVIDOR WEB
// ------------------------------------------------------------------------------
void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleStatus() {
  uint32_t t_act_s = (tInicioEnsayo_ms > 0) ? ((millis() - tInicioEnsayo_ms) / 1000) : 0;
  uint8_t prog = (uint8_t)((autoCalMuestras * 100) / MUESTRAS_AUTO_CAL);
  if (prog > 100) prog = 100;

  float qAlim = sensorAlimentacion.caudal_mLmin();
  float qPerm = sensorPermeado.caudal_mLmin();
  jLMH_actual = (qPerm * 0.06f) / AREA_MEMBRANA_M2;

  char buf[768];
  int n = snprintf(buf, sizeof(buf),
    "{"
    "\"rpm\":%.1f,\"obj_rpm\":%.1f,\"on\":%s,\"inv\":%s,\"dir\":%s,\"en_regimen\":%s,"
    "\"ip\":\"%s\",\"alim_ok\":%s,\"perm_ok\":%s,\"f_alim\":%.2f,\"q_alim\":%.1f,\"vol_alim\":%.4f,"
    "\"f_perm\":%.2f,\"q_perm\":%.1f,\"vol_perm\":%.4f,\"q_ret\":%.1f,\"recov\":%.2f,"
    "\"pump_ml\":%.1f,\"delta\":%.2f,\"j_lmh\":%.2f,\"cruce\":%s,"
    "\"k_alim\":%.2f,\"k_perm\":%.2f,\"ml_rev\":%.4f,\"pul_rev\":%u,"
    "\"auto_cal\":%s,\"auto_cal_prog\":%u,\"auto_cal_res\":\"%s\","
    "\"n_logs\":%u,\"ensayo_act\":%u,\"t_ensayo_s\":%lu,"
    "\"heap\":%u,\"maxblk\":%u,"
    "\"ensayos\":[",
    bomba.rpmActual(), bomba.rpmObjetivo(),
    bomba.enMarcha() ? "true" : "false",
    bomba.invirtiendo() ? "true" : "false",
    bomba.sentidoHorario() ? "true" : "false",
    bomba.enRegimenEstable() ? "true" : "false",
    WiFi.softAPIP().toString().c_str(),
    !sensorAlimentacion.sinSenal() ? "true" : "false",
    !sensorPermeado.sinSenal() ? "true" : "false",
    sensorAlimentacion.frecuencia_Hz(), qAlim, sensorAlimentacion.volumen_L(),
    sensorPermeado.frecuencia_Hz(), qPerm, sensorPermeado.volumen_L(),
    qRet_mLmin, recuperacion,
    bomba.caudalTeorico_mLmin(), deltaBomba, jLMH_actual, flagCruceSensores ? "true" : "false",
    sensorAlimentacion.getK(), sensorPermeado.getK(),
    bomba.getMlPorVuelta(), bomba.getPulsosPorRev(),
    autoCalibrando ? "true" : "false", prog, autoCalMensaje.c_str(),
    (unsigned)numRegistros, (unsigned)ensayoActualId, (unsigned long)t_act_s,
    ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)
  );

  if (n >= (int)sizeof(buf)) {
    n = sizeof(buf) - 1;
    Serial.println("⚠️ [WARN] handleStatus buf[768] truncado");
  }

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  server.sendContent(buf, n);

  char item[128];
  for (size_t i = 0; i < numEnsayos; i++) {
    int itemLen = snprintf(item, sizeof(item),
      "%s{\"id\":%u,\"rpm\":%.1f,\"duracion_s\":%lu,\"muestras\":%u,\"vol_alim\":%.4f,\"vol_perm\":%.4f}",
      (i > 0) ? "," : "",
      listaEnsayos[i].id, listaEnsayos[i].rpm_consigna,
      (unsigned long)listaEnsayos[i].duracion_s,
      listaEnsayos[i].muestras,
      listaEnsayos[i].vol_alim, listaEnsayos[i].vol_perm
    );
    if (itemLen >= (int)sizeof(item)) itemLen = sizeof(item) - 1;
    server.sendContent(item, itemLen);
  }

  server.sendContent("]}");
  server.sendContent(""); // Cierra el streaming chunked
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
  } else if (act == "DIR") {
    bomba.toggleSentido();
  } else if (act == "RESET_VOL") {
    sensorAlimentacion.resetVolumen();
    sensorPermeado.resetVolumen();
  }
  server.send(200, "text/plain", "OK");
}

void handleSetRPM() {
  if (server.hasArg("rpm")) {
    float rpm = server.arg("rpm").toFloat();
    if (bomba.setRPM(rpm)) {
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "ERROR: RPM fuera de rango o invalido");
    }
  } else {
    server.send(400, "text/plain", "Falta argumento rpm");
  }
}

// Configuración en Modo Desarrollador
void handleSetDev() {
  if (server.hasArg("ka")) {
    float ka = server.arg("ka").toFloat();
    sensorAlimentacion.setK(ka);
  }
  if (server.hasArg("kp")) {
    float kp = server.arg("kp").toFloat();
    sensorPermeado.setK(kp);
  }
  if (server.hasArg("ml")) {
    float ml = server.arg("ml").toFloat();
    bomba.setMlPorVuelta(ml);
  }
  if (server.hasArg("pul")) {
    uint16_t pul = (uint16_t)server.arg("pul").toInt();
    bomba.setPulsosPorRev(pul);
  }

  bool saveNvs = (server.hasArg("save") && server.arg("save") == "1");
  if (saveNvs) {
    guardarParametrosNVS(sensorAlimentacion.getK(), sensorPermeado.getK(),
                         bomba.getMlPorVuelta(), bomba.getPulsosPorRev());
  }

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// Iniciar Auto-Calibración Inteligente en Régimen Permanente
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

// Calibrador Completo por RPM y Caudal de Probeta
void handleCalibrarRpmQ() {
  if (!server.hasArg("rpm") || !server.hasArg("qa")) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Faltan argumentos (requiere rpm y qa)\"}");
    return;
  }

  float rpm = server.arg("rpm").toFloat();
  float qa  = server.arg("qa").toFloat();
  float qp  = server.hasArg("qp") ? server.arg("qp").toFloat() : 0.0f;

  if (rpm <= 0.0f || qa <= 0.0f) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"RPM y Caudal de Alimentacion deben ser > 0\"}");
    return;
  }

  // 1. Cilindrada real de la bomba
  float nuevaCilindrada = qa / rpm;
  bomba.setMlPorVuelta(nuevaCilindrada);

  // 2. Factores K basados en frecuencia actual
  float fa = sensorAlimentacion.frecuencia_Hz();
  float fp = sensorPermeado.frecuencia_Hz();

  if (fa > 1.0f) {
    float nuevoKa = (fa * 1000.0f) / qa;
    sensorAlimentacion.setK(nuevoKa);
  }
  if (qp > 0.0f && fp > 0.3f) {
    float nuevoKp = (fp * 1000.0f) / qp;
    sensorPermeado.setK(nuevoKp);
  }

  guardarParametrosNVS(sensorAlimentacion.getK(), sensorPermeado.getK(),
                       bomba.getMlPorVuelta(), bomba.getPulsosPorRev());

  String json = "{";
  json += "\"status\":\"ok\",";
  json += "\"ml_rev\":" + String(bomba.getMlPorVuelta(), 4) + ",";
  json += "\"k_alim\":" + String(sensorAlimentacion.getK(), 2) + ",";
  json += "\"k_perm\":" + String(sensorPermeado.getK(), 2);
  json += "}";

  server.send(200, "application/json", json);
}

// Restablecer parámetros de fábrica
void handleResetDev() {
  sensorAlimentacion.setK(K_ALIMENTACION);
  sensorPermeado.setK(K_PERMEADO);
  bomba.setMlPorVuelta(ML_POR_VUELTA);
  bomba.setPulsosPorRev(PULSOS_POR_REV);
  
  guardarParametrosNVS(K_ALIMENTACION, K_PERMEADO, ML_POR_VUELTA, PULSOS_POR_REV);
  server.send(200, "application/json", "{\"status\":\"reset_ok\"}");
}

// Exportación a Excel (.CSV) con filtrado por Ensayo o Histórico Completo
void handleExportCSV() {
  String targetEnsayo = server.hasArg("ensayo") ? server.arg("ensayo") : "all";
  
  String filename = "PlantaUF_Calibracion_";
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
  
  // Encabezado CSV
  server.sendContent("sep=;\n"
                     "PLANTA DE ULTRAFILTRACION FX100 - REGISTRO DE ENSAYOS Y CALIBRACION\n"
                     "Ensayo_ID;Tiempo_s;Tiempo_MinSec;RPM_Bomba;Q_Bomba_Teorico_mLmin;Frec_Alimentacion_Hz;Q_Alimentacion_mLmin;Vol_Alimentacion_L;Frec_PERMEADO_Hz;Q_PERMEADO_mLmin;Vol_PERMEADO_L;Q_Retentado_mLmin;Recuperacion_Y_Pct;Desviacion_Bomba_Alim_Pct;K_Alim;K_Perm;J_LMH\n");

  uint8_t filtroId = 0;
  if (targetEnsayo == "actual") {
    filtroId = ensayoActualId;
  } else if (targetEnsayo != "all") {
    filtroId = (uint8_t)targetEnsayo.toInt();
  }

  char fila[200];
  for (size_t i = 0; i < numRegistros; i++) {
    const RegistroCalibracion& r = bufferLog[i];
    
    if (filtroId > 0 && r.id_ensayo != filtroId) {
      continue;
    }

    uint32_t mins = r.t_relativo_s / 60;
    uint32_t secs = r.t_relativo_s % 60;

    int len = snprintf(fila, sizeof(fila),
      "%u;%lu;%02u:%02u;%.1f;%.1f;%.2f;%.1f;%.4f;%.2f;%.1f;%.4f;%.1f;%.2f;%.2f;%.2f;%.2f;%.2f\n",
      r.id_ensayo, (unsigned long)r.t_relativo_s, mins, secs,
      r.rpm, r.q_bomba, r.f_alim, r.q_alim, r.vol_alim,
      r.f_perm, r.q_perm, r.vol_perm, r.q_ret, r.recov,
      r.delta, r.k_alim, r.k_perm, r.j_lmh
    );

    server.sendContent(fila, len);
    if ((i & 31) == 0) yield();
  }

  server.sendContent(""); // Cierra el streaming chunked
}

void handleClearCSV() {
  numRegistros = 0;
  numEnsayos = 0;
  ensayoActualId = 1;
  tInicioEnsayo_ms = millis();
  server.send(200, "text/plain", "LOGS_CLEARED");
}

// ------------------------------------------------------------------------------
// SETUP DEL MICROCONTROLADOR ESP32
// ------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n==================================================");
  Serial.println(" PLANTA PILOTO DE ULTRAFILTRACIÓN FX100 — UNSa   ");
  Serial.println(" Rampa S-Curve, Auto-Calibracion & Datalogger     ");
  Serial.println("==================================================");

  // 1. Cargar parámetros de calibración desde NVS Flash
  cargarParametrosNVS();

  // 2. Inicialización de Hardware
  pinMode(PIN_LED_BOMBA, OUTPUT);
  digitalWrite(PIN_LED_BOMBA, LOW);

  sensorAlimentacion.begin();
  sensorPermeado.begin();
  bomba.begin();

  // 3. Configuración Wi-Fi Robusta (AP Dedicado Anti-Desconexión)
  WiFi.disconnect(true);           // Limpiar estados previos
  delay(100);
  WiFi.mode(WIFI_AP);              // Modo AP Puro (evita escaneos STA que botan clientes)
  WiFi.setSleep(false);            // CRÍTICO: Desactiva ahorro de energía del módem
  WiFi.setTxPower(WIFI_POWER_19_5dBm); // Máxima potencia de transmisión RF

  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(SSID_AP, PASS_AP, 1, 0, 4); // Canal 1 fijo, SSID visible, hasta 4 clientes

  Serial.println("[WIFI] Punto de Acceso Estable Creado:");
  Serial.printf("       SSID: %s | Pass: %s\n", SSID_AP, PASS_AP);
  Serial.printf("       IP AP: http://%s\n", WiFi.softAPIP().toString().c_str());

  if (MDNS.begin("bomba")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("[mDNS] Servidor publicado en: http://bomba.local");
  }

  // 4. Servidor DNS para Portal Cautivo Anti-Desconexión
  // ¿POR QUÉ SE HACE ESTO?
  // Sistemas operativos modernos (Android, iOS, Windows) envían consultas DNS ocultas
  // (p. ej. connectivitycheck.gstatic.com, msftconnecttest.com) para verificar si la red tiene Internet.
  // Al no haber conexión externa, el teléfono/PC asume que el Wi-Fi está roto y se desconecta solo.
  // Este servidor DNS responde a cualquier dominio ("*") con la IP local 192.168.4.1.
  // El sistema operativo reconoce la red como "Portal Cautivo" (estilo hotel/aeropuerto),
  // ANULA el descarte automático y mantiene la conexión Wi-Fi fija y permanente al SCADA.
  dnsServer.start(DNS_PORT, "*", local_IP);
  Serial.println("[DNS] Servidor DNS Captive Portal activo en puerto 53 (Anti-Desconexion)");

  // 5. Enrutamiento del Servidor Web
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/cmd", HTTP_GET, handleCmd);
  server.on("/set", HTTP_GET, handleSetRPM);
  server.on("/set_dev", HTTP_GET, handleSetDev);
  server.on("/reset_dev", HTTP_GET, handleResetDev);
  server.on("/iniciar_auto_cal", HTTP_GET, handleIniciarAutoCal);
  server.on("/cancelar_auto_cal", HTTP_GET, handleCancelarAutoCal);
  server.on("/calibrar_rpm_q", HTTP_GET, handleCalibrarRpmQ);
  server.on("/export_csv", HTTP_GET, handleExportCSV);
  server.on("/clear_csv", HTTP_GET, handleClearCSV);

  // Rutas de sondeo de conectividad de sistemas operativos para Portal Cautivo
  server.on("/generate_204", HTTP_GET, handleRoot);        // Android Captive Portal Check
  server.on("/gen_204", HTTP_GET, handleRoot);             // Android alternativo
  server.on("/ncsi.txt", HTTP_GET, handleRoot);            // Windows Network Connectivity Status
  server.on("/connecttest.txt", HTTP_GET, handleRoot);     // Windows alternativo
  server.on("/hotspot-detect.html", HTTP_GET, handleRoot); // Apple iOS / macOS
  server.onNotFound(handleRoot);                           // Cualquier otra URL no registrada abre el SCADA

  server.begin();
  Serial.println("[HTTP] Servidor Web SCADA iniciado con exito en puerto 80.\n");

  tLoop = millis();
  tCaudal = millis();
  tDatalogger = millis();
  tInicioEnsayo_ms = millis();
}

// ------------------------------------------------------------------------------
// BUCLE PRINCIPAL (LOOP NO BLOQUEANTE)
// ------------------------------------------------------------------------------
void loop() {
  dnsServer.processNextRequest(); // Atiende consultas DNS en < 2 us sin bloquear la ejecucion
  server.handleClient();

  uint32_t tAhora = millis();

  // 1. Control cinemático de la bomba cada 50 ms (Rampa S-Curve Progresiva)
  if (tAhora - tLoop >= 50) {
    float dt = (tAhora - tLoop) / 1000.0f;
    tLoop = tAhora;
    bomba.tick(dt);

    // Testigo LED onboard
    digitalWrite(PIN_LED_BOMBA, (bomba.rpmActual() > 0.5f) ? HIGH : LOW);

    // Detección de flancos de la bomba para registro de ensayos
    bool enMarcha = bomba.enMarcha();
    
    // Flanco de subida: Iniciar sesión de ensayo
    if (enMarcha && !bombaEnMarchaAnterior) {
      uint16_t muestrasPrevias = 0;
      for (size_t i = 0; i < numRegistros; i++) {
        if (bufferLog[i].id_ensayo == ensayoActualId) muestrasPrevias++;
      }
      if (muestrasPrevias > 0) {
        ensayoActualId++;
      }
      tInicioEnsayo_ms = millis();
      volAlimInicioEnsayo = sensorAlimentacion.volumen_L();
      volPermInicioEnsayo = sensorPermeado.volumen_L();
      Serial.printf("\n>>> [ENSAYO #%u INICIADO] Consigna: %.1f RPM <<<\n", ensayoActualId, bomba.rpmObjetivo());
    }
    
    // Flanco de bajada: Finalizar sesión y registrar
    if (!enMarcha && bombaEnMarchaAnterior) {
      uint32_t duracion_s = (millis() - tInicioEnsayo_ms) / 1000;
      uint16_t muestrasEnsayo = 0;
      for (size_t i = 0; i < numRegistros; i++) {
        if (bufferLog[i].id_ensayo == ensayoActualId) muestrasEnsayo++;
      }

      // Si duró al menos 1s o no hubo muestras periódicas aún, registrar muestra de cierre
      if (muestrasEnsayo == 0) {
        guardarMuestraDatalogger();
        muestrasEnsayo = 1;
      }

      if (numEnsayos < MAX_ENSAYOS) {
        listaEnsayos[numEnsayos].id           = ensayoActualId;
        listaEnsayos[numEnsayos].rpm_consigna = bomba.rpmObjetivo();
        listaEnsayos[numEnsayos].t_inicio_ms  = tInicioEnsayo_ms;
        listaEnsayos[numEnsayos].duracion_s   = duracion_s;
        listaEnsayos[numEnsayos].muestras     = muestrasEnsayo;
        listaEnsayos[numEnsayos].vol_alim     = sensorAlimentacion.volumen_L() - volAlimInicioEnsayo;
        listaEnsayos[numEnsayos].vol_perm     = sensorPermeado.volumen_L() - volPermInicioEnsayo;
        numEnsayos++;

        Serial.printf("\n<<< [ENSAYO #%u FINALIZADO Y REGISTRADO] Duracion: %us | Muestras: %u | Vol Perm: %.3f L >>>\n\n",
                      ensayoActualId, duracion_s, muestrasEnsayo, sensorPermeado.volumen_L() - volPermInicioEnsayo);
      }
    }

    bombaEnMarchaAnterior = enMarcha;
  }

  // 2. Adquisición y cálculo de caudales cada 1000 ms (1 segundo)
  if (tAhora - tCaudal >= 1000) {
    float dt = (tAhora - tCaudal) / 1000.0f;
    tCaudal = tAhora;

    bool bombaEmpuja = (bomba.rpmActual() > 1.0f);
    sensorAlimentacion.actualizar(dt, bombaEmpuja);
    sensorPermeado.actualizar(dt, bombaEmpuja);

    float qAlim = sensorAlimentacion.caudal_mLmin();
    float qPerm = sensorPermeado.caudal_mLmin();

    // Sanity-check de cruce de sensores (el permeado no puede exceder físicamente al caudal de alimentación)
    flagCruceSensores = (qPerm > qAlim + 50.0f) && (bomba.rpmActual() > 5.0f);
    if (flagCruceSensores) {
      Serial.printf("⚠️ [ALERTA] Cruce de cables o sensor invertido: Q_Perm (%.1f mL/min) > Q_Alim (%.1f mL/min)\n", qPerm, qAlim);
    }

    // Balance Hidráulico Tangencial y Flujo Darcy en tiempo real
    qRet_mLmin   = fmaxf(0.0f, qAlim - qPerm);
    recuperacion = (qAlim > 50.0f) ? ((qPerm / qAlim) * 100.0f) : 0.0f;
    jLMH_actual  = (qPerm * 0.06f) / AREA_MEMBRANA_M2;

    float qBomba = bomba.caudalTeorico_mLmin();
    deltaBomba   = (qBomba > 1.0f) ? (((qAlim - qBomba) / qBomba) * 100.0f) : 0.0f;

    // Máquina de estados de Auto-Calibración en régimen permanente
    if (autoCalibrando) {
      if (bomba.enRegimenEstable()) {
        autoCalSumFrecAlim += sensorAlimentacion.frecuencia_Hz();
        autoCalSumFrecPerm += sensorPermeado.frecuencia_Hz();
        autoCalMuestras++;

        Serial.printf("[AUTO-CAL] Muestra %u/%u | F_Alim=%.2f Hz | F_Perm=%.2f Hz\n",
                      autoCalMuestras, MUESTRAS_AUTO_CAL, sensorAlimentacion.frecuencia_Hz(), sensorPermeado.frecuencia_Hz());

        if (autoCalMuestras >= MUESTRAS_AUTO_CAL) {
          float fPromAlim = autoCalSumFrecAlim / (float)MUESTRAS_AUTO_CAL;
          float fPromPerm = autoCalSumFrecPerm / (float)MUESTRAS_AUTO_CAL;
          float qRefAlim  = bomba.rpmActual() * bomba.getMlPorVuelta();

          if (qRefAlim > 10.0f && fPromAlim > 1.0f) {
            float nuevoKa = (fPromAlim * 1000.0f) / qRefAlim;
            sensorAlimentacion.setK(nuevoKa);
            Serial.printf("[AUTO-CAL] K_Alim ajustado: %.2f Hz/(L/min)\n", nuevoKa);
          }
          // NOTA METROLÓGICA (Auditoría Ronda 4): Permeado depende de Darcy y ensuciamiento,
          // no de RPM de la bomba. NUNCA se auto-calibra con target ficticio. Se calibra con balanza gravimétrica.
          float kpActual = sensorPermeado.getK();

          guardarParametrosNVS(sensorAlimentacion.getK(), kpActual,
                               bomba.getMlPorVuelta(), bomba.getPulsosPorRev());

          autoCalibrando = false;
          autoCalMensaje = "✅ Auto-Calibracion OK: K_Alim=" + String(sensorAlimentacion.getK(), 2) + " (K_Perm intacto=" + String(kpActual, 2) + ")";
          Serial.printf("\n>>> %s <<<\n\n", autoCalMensaje.c_str());
        }
      } else {
        Serial.println("[AUTO-CAL] Esperando estabilizacion de RPM...");
      }
    }

    // Telemetría periódica por Serial
    Serial.printf("[TELEMETRIA] RPM: %4.1f | Q_Alim: %5.1f mL/min | Q_Perm: %5.1f mL/min | J: %4.2f LMH | Q_Ret: %5.1f mL/min | Y: %4.1f%% | V_Perm: %.3f L\n",
                  bomba.rpmActual(), qAlim, qPerm, jLMH_actual, qRet_mLmin, recuperacion, sensorPermeado.volumen_L());
  }

  // 3. Muestreo del Datalogger cada 10 segundos (SOLO mientras la bomba está en marcha)
  // Al presionar STOP, se corta el registro para evitar muestras espurias por flujo residual
  if (tAhora - tDatalogger >= INTERVALO_LOG_MS) {
    tDatalogger = tAhora;
    if (bomba.enMarcha()) {
      guardarMuestraDatalogger();
    }
  }
}
