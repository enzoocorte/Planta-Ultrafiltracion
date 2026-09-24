/* ============================================================
 *  PLANTA DE ULTRAFILTRACIÓN — HITO 2.1 (v2.2: cátodo común)
 *  Bomba peristáltica + 2 caudalímetros YF-S401 + SCADA Wi-Fi
 *  ⚠️ Solo compatible con driver cableado en CÁTODO COMÚN
 * ============================================================ */
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "config.h"
#include "caudalimetro.h"
#include "bomba.h"
#include "index_html.h"

Caudalimetro sensorFeed(PIN_SENSOR_FEED, K_FEED, "FEED");
Caudalimetro sensorPerm(PIN_SENSOR_PERM, K_PERM, "PERMEADO");
Bomba bomba;
WebServer server(80);

float qRet_mLmin = 0, recuperacion = 0, deltaBomba = 0, volBomba_L = 0;
uint32_t tLoop = 0, tCaudal = 0;

void manejarRaiz()   { server.send_P(200, "text/html", INDEX_HTML); }
void manejarSet()    { if (server.hasArg("rpm")) bomba.setRPM(server.arg("rpm").toFloat()); server.send(200, "text/plain", "OK"); }

void manejarCmd() {
  String a = server.arg("act");
  if      (a == "START")      bomba.arrancar();
  else if (a == "STOP")       bomba.detener();
  else if (a == "DIR")        bomba.toggleSentido();
  else if (a == "RESET_VOL")  { sensorFeed.resetVolumen(); sensorPerm.resetVolumen(); volBomba_L = 0; }
  server.send(200, "text/plain", "OK");
}

void manejarStatus() {
  char ip[24];
  if (WiFi.status() == WL_CONNECTED) WiFi.localIP().toString().toCharArray(ip, 24);
  else strcpy(ip, "solo AP");
  char j[420];
  snprintf(j, sizeof(j),
    "{\"on\":%d,\"inv\":%d,\"dir\":%d,\"rpm\":%.1f,\"pump_ml\":%.1f,"
    "\"f_feed\":%.1f,\"q_feed\":%.1f,\"vol_feed\":%.3f,\"feed_ok\":%d,"
    "\"f_perm\":%.1f,\"q_perm\":%.1f,\"vol_perm\":%.3f,\"perm_ok\":%d,"
    "\"q_ret\":%.1f,\"recov\":%.1f,\"delta\":%.1f,\"ip\":\"%s\"}",
    bomba.enMarcha(), bomba.invirtiendo(), bomba.sentidoHorario(),
    bomba.rpmActual(), bomba.caudalTeorico_mLmin(),
    sensorFeed.frecuencia_Hz(), sensorFeed.caudal_mLmin(), sensorFeed.volumen_L(), !sensorFeed.sinSenal(),
    sensorPerm.frecuencia_Hz(), sensorPerm.caudal_mLmin(), sensorPerm.volumen_L(), !sensorPerm.sinSenal(),
    qRet_mLmin, recuperacion, deltaBomba, ip);
  server.send(200, "application/json", j);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  sensorFeed.begin();
  sensorPerm.begin();
  bomba.begin();
  Serial.println("\n=== PLANTA UF | Hito 2.1 (v2.2 cátodo común) ===");

  // 1. Conectar a Router primero (15s timeout) para fijar el canal Wi-Fi
  WiFi.mode(WIFI_AP_STA);
  Serial.printf("Conectando a '%s' ", SSID_STA);
  WiFi.begin(SSID_STA, PASS_STA);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? " -> OK" : " -> TIMEOUT (queda AP activo)");

  // 2. Levantar el SoftAP (hereda el canal RF del router si conectó)
  WiFi.softAP(SSID_AP, PASS_AP);
  MDNS.begin("bomba");

  server.on("/", HTTP_GET, manejarRaiz);
  server.on("/set", HTTP_GET, manejarSet);
  server.on("/cmd", HTTP_GET, manejarCmd);
  server.on("/status", HTTP_GET, manejarStatus);
  server.begin();

  Serial.printf("AP:     http://%s  (siempre disponible)\n", WiFi.softAPIP().toString().c_str());
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Router: http://%s  <<< USA ESTA URL en Box804\n", WiFi.localIP().toString().c_str());
    Serial.println("mDNS:   http://bomba.local");
  }
  Serial.println("Sensores: FEED=G14, PERM=G27 | Bomba: PUL+=G18, DIR+=G19 (PUL-/DIR- a GND)");
  tLoop = tCaudal = millis();
}

void loop() {
  server.handleClient();
  uint32_t t = millis();

  // --- WiFi: si se cayó o nunca conectó, reintenta cada 30 s en segundo plano ---
  static uint32_t tWiFi = 0;
  if (t - tWiFi >= 30000 && WiFi.status() != WL_CONNECTED) {
    tWiFi = t;
    Serial.println("[WIFI] Reintentando conexión con router...");
    WiFi.begin(SSID_STA, PASS_STA);
  }

  // --- Bomba: rampa cada 50 ms ---
  float dt = (t - tLoop) / 1000.0f;
  if (dt >= 0.05f) {
    tLoop = t;
    bomba.tick(dt);
    volBomba_L += (bomba.caudalTeorico_mLmin() / 60000.0f) * dt;
  }

  // --- Caudalímetros: cada 1 s ---
  if (t - tCaudal >= 1000) {
    float dtc = (t - tCaudal) / 1000.0f;
    tCaudal = t;
    bool empuja = bomba.caudalTeorico_mLmin() > 150.0f;   // > 0.15 L/min
    sensorFeed.actualizar(dtc, empuja);
    sensorPerm.actualizar(dtc, empuja);

    float qF = sensorFeed.caudal_mLmin();
    float qP = sensorPerm.caudal_mLmin();
    float qT = bomba.caudalTeorico_mLmin();
    qRet_mLmin   = max(0.0f, qF - qP);
    recuperacion = (qF > 20.0f) ? 100.0f * qP / qF : 0.0f;
    deltaBomba   = (qT > 20.0f) ? 100.0f * (qF - qT) / qT : 0.0f;

    if (sensorFeed.sinSenal()) Serial.println("[ALERTA] FEED sin señal: ¿burbuja de aire o cable suelto?");
    if (sensorPerm.sinSenal()) Serial.println("[ALERTA] PERM sin señal: ¿burbuja de aire o cable suelto?");

    Serial.printf("[%lus] FEED %5.1f Hz %6.1f | PERM %6.1f | RET %6.1f | Y %4.1f%% | Bomba %3.0f RPM teor %6.1f D%+5.1f%%\n",
                  t / 1000, sensorFeed.frecuencia_Hz(), qF, qP, qRet_mLmin,
                  recuperacion, bomba.rpmActual(), qT, deltaBomba);
  }
}