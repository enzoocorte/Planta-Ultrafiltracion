/* ==============================================================================
 * PROYECTO DE TESIS DE INGENIERÍA INDUSTRIAL - PLANTA DE ULTRAFILTRACIÓN FX100
 * HITO 1: MONTAJE Y ACCIONAMIENTO DE BOMBA PERISTÁLTICA MBP-2000 (NEMA 34 + DM860)
 * CONTROL INALÁMBRICO WI-FI + SERVIDOR WEB TÁCTIL + ARDUINO OTA + CONSOLA SERIE
 * ==============================================================================
 * 
 * ESQUEMA DE CONEXIÓN CON EL SHIELD DE BORNERAS DEL ESP32 (38 PINES):
 * - Borne [ GPIO 18 / D18 ] ──► Borne PUL+ del Driver DM860 (Pulsos STEP)
 * - Borne [ GPIO 19 / D19 ] ──► Borne DIR+ del Driver DM860 (Sentido DIR)
 * - Borne [ GND ]           ──► Bornes PUL- y DIR- puenteados (Cátodo Común)
 * - Borne [ VIN ]           ──► 5.00V DC regulados desde el Step-Down LM2596
 * - Borne [ GND ]           ──► 0V (Masa) de la fuente
 * 
 * MOTOR NEMA 34 (8 CABLES) EN BIPOLAR SERIE (3A):
 * - Amarillo + Azul unidos entre sí y aislados.
 * - Naranja + Marrón unidos entre sí y aislados.
 * - Rojo a A+, Negro a A-, Blanco a B+, Verde a B-.
 * ============================================================================== */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

// ------------------------------------------------------------------------------
// 1. CREDENCIALES WI-FI (Planta Piloto)
// ------------------------------------------------------------------------------
const char* ssid_red     = "Box804";
const char* password_red = "plantapiloto2";

// Red Wi-Fi autónoma (si no hay router en el laboratorio)
const char* ap_ssid      = "Bomba_Peristaltica_UF";
const char* ap_pass      = "plantapiloto2";

WebServer server(80);

// ------------------------------------------------------------------------------
// 2. ASIGNACIÓN DE PINES EN EL SHIELD DE BORNERAS DEL ESP32
// ------------------------------------------------------------------------------
const uint8_t PIN_PUL = 18; // Borne D18 -> Paso (STEP)
const uint8_t PIN_DIR = 19; // Borne D19 -> Dirección (DIR)

// Driver DM860 configurado en 1600 pulsos/rev (8 micropasos)
const uint16_t PULSOS_POR_REV = 1600; 

// Calibración volumétrica del cabezal peristáltico MBP-2000
// Cada vuelta desplaza exactamente 4.2 mL
const float ML_POR_REVOLUCION = 4.2; 

// ------------------------------------------------------------------------------
// 3. VARIABLES DE ESTADO Y CONTROL
// ------------------------------------------------------------------------------
bool bombaEnMarcha = false;
bool sentidoHorario = true; // true = Filtración hacia FX100, false = Retrolavado

float rpm_objetivo = 0.0;
float rpm_actual   = 0.0;
const float ACELERACION_RPM_SEG = 35.0; // Rampa suave: 35 RPM por segundo

float caudal_actual_Lmin = 0.0;
float volumen_total_L    = 0.0;

unsigned long t_ultimo_loop_ms = 0;

// ------------------------------------------------------------------------------
// 4. GENERADOR DE PULSOS POR SILICIO (HARDWARE LEDC TIMER) - 0% CARGA DE CPU
// ------------------------------------------------------------------------------
void fijarFrecuenciaMotor(float rpm) {
  if (rpm > 0.5 && bombaEnMarcha) {
    // Ecuación Fundamental: f (Hz) = (RPM * 1600) / 60
    float frecuencia_hz = (rpm * (float)PULSOS_POR_REV) / 60.0f;
    ledcWriteTone(PIN_PUL, frecuencia_hz); // Generación directa por timer de silicio
  } else {
    ledcWriteTone(PIN_PUL, 0);             // Frecuencia cero (detenido)
    digitalWrite(PIN_PUL, LOW);
  }
}

// ------------------------------------------------------------------------------
// 5. INTERFAZ WEB RESPONSIVA (DASHBOARD SCADA TÁCTIL)
// ------------------------------------------------------------------------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Bomba MBP-2000 | Ultrafiltración</title>
  <style>
    :root {
      --bg: #090d16; --card: #131b2e; --border: #223150;
      --text: #f8fafc; --muted: #94a3b8; --accent: #38bdf8;
      --success: #10b981; --danger: #ef4444; --warning: #f59e0b;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', system-ui, sans-serif; }
    body { background: var(--bg); color: var(--text); padding: 16px; min-height: 100vh; display: flex; justify-content: center; align-items: center; }
    .card { background: var(--card); border: 1px solid var(--border); border-radius: 16px; padding: 22px; width: 100%; max-width: 460px; box-shadow: 0 12px 30px rgba(0,0,0,0.6); }
    h1 { font-size: 19px; color: var(--accent); text-align: center; margin-bottom: 2px; }
    p.sub { font-size: 11px; color: var(--muted); text-align: center; margin-bottom: 16px; }
    .display { background: #060911; border: 1px solid var(--border); border-radius: 12px; padding: 14px; text-align: center; margin-bottom: 16px; }
    .rpm-num { font-size: 46px; font-weight: 800; font-family: monospace; color: var(--accent); line-height: 1; margin: 4px 0; }
    .metric-sub { font-size: 12px; color: var(--muted); }
    .caudal-box { display: flex; justify-content: space-around; background: #0c1220; border-radius: 8px; padding: 10px; margin-top: 10px; border: 1px solid #1a2742; }
    .caudal-item span { font-size: 10px; color: var(--muted); display: block; }
    .caudal-item strong { font-size: 15px; color: var(--success); font-family: monospace; }
    .pill { display: inline-block; padding: 4px 12px; border-radius: 20px; font-size: 11px; font-weight: bold; margin-top: 8px; }
    .pill-on { background: rgba(16, 185, 129, 0.2); color: var(--success); border: 1px solid var(--success); }
    .pill-off { background: rgba(239, 68, 68, 0.2); color: var(--danger); border: 1px solid var(--danger); }
    .slider-container { margin: 16px 0; }
    .slider-header { display: flex; justify-content: space-between; font-size: 12px; margin-bottom: 6px; }
    input[type=range] { width: 100%; height: 8px; border-radius: 4px; background: #223150; accent-color: var(--accent); cursor: pointer; }
    .preset-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 6px; margin-bottom: 14px; }
    .btn-preset { background: #1a253c; border: 1px solid var(--border); color: var(--text); padding: 8px 0; border-radius: 6px; font-size: 11px; font-weight: bold; cursor: pointer; }
    .btn-preset:active { background: var(--accent); color: #000; }
    .action-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-bottom: 12px; }
    .btn { padding: 12px; border: none; border-radius: 8px; font-size: 13px; font-weight: bold; cursor: pointer; transition: 0.15s; }
    .btn:active { transform: scale(0.97); }
    .btn-start { background: var(--accent); color: #060911; }
    .btn-stop { background: var(--danger); color: #fff; }
    .btn-dir { background: var(--warning); color: #060911; grid-column: span 2; }
    .footer { font-size: 10px; color: #64748b; text-align: center; margin-top: 12px; line-height: 1.4; }
  </style>
</head>
<body>
  <div class="card">
    <h1>BOMBA PERISTÁLTICA MBP-2000</h1>
    <p class="sub">Planta Piloto UF FX100 • Control Wi-Fi ESP32</p>

    <div class="display">
      <div class="rpm-num" id="disp_rpm">0.0</div>
      <div class="metric-sub">RPM (Frecuencia: <span id="disp_freq">0</span> Hz)</div>
      <div id="disp_pill" class="pill pill-off">MOTOR DETENIDO</div>
      <div style="font-size: 11px; color: var(--muted); margin-top: 6px;" id="disp_dir">Giro: Horario (Filtración FX100)</div>

      <div class="caudal-box">
        <div class="caudal-item">
          <span>CAUDAL TEÓRICO</span>
          <strong id="disp_flow">0.00 L/min</strong>
        </div>
        <div class="caudal-item">
          <span>VOLUMEN BOMBEO</span>
          <strong id="disp_vol" style="color: var(--accent);">0.00 L</strong>
        </div>
      </div>
    </div>

    <div class="slider-container">
      <div class="slider-header">
        <span>Fijar Velocidad:</span>
        <strong id="txt_slider" style="color: var(--accent);">30 RPM</strong>
      </div>
      <input type="range" id="slider" min="0" max="120" value="30" oninput="ajustarSlider(this.value)">
    </div>

    <div class="preset-grid">
      <button class="btn-preset" onclick="fijarRPM(10)">10 RPM</button>
      <button class="btn-preset" onclick="fijarRPM(30)">30 RPM</button>
      <button class="btn-preset" onclick="fijarRPM(60)">60 RPM</button>
      <button class="btn-preset" onclick="fijarRPM(100)">100 RPM</button>
    </div>

    <div class="action-grid">
      <button class="btn btn-start" onclick="enviarAccion('START')">▶ INICIAR</button>
      <button class="btn btn-stop" onclick="enviarAccion('STOP')">⏹ DETENER</button>
      <button class="btn btn-dir" onclick="enviarAccion('TOGGLE_DIR')">🔄 INVERTIR SENTIDO</button>
    </div>

    <div class="footer">
      ESP32 Bornera: D18 (PUL), D19 (DIR) • DM860 (1600 P/R)<br>
      IP: <span id="disp_ip">Conectando...</span> • OTA Inalámbrico Activo
    </div>
  </div>

  <script>
    function ajustarSlider(v) {
      document.getElementById('txt_slider').innerText = v + " RPM";
      fetch('/set?rpm=' + v);
    }
    function fijarRPM(v) {
      document.getElementById('slider').value = v;
      document.getElementById('txt_slider').innerText = v + " RPM";
      fetch('/set?rpm=' + v);
      fetch('/cmd?act=START');
    }
    function enviarAccion(act) {
      fetch('/cmd?act=' + act);
    }

    setInterval(() => {
      fetch('/status').then(r => r.json()).then(d => {
        document.getElementById('disp_rpm').innerText = d.rpm.toFixed(1);
        document.getElementById('disp_freq').innerText = Math.round((d.rpm * 1600) / 60);
        document.getElementById('disp_flow').innerText = d.flow.toFixed(3) + " L/min";
        document.getElementById('disp_vol').innerText = d.vol.toFixed(2) + " L";
        document.getElementById('disp_ip').innerText = d.ip;

        let p = document.getElementById('disp_pill');
        if (d.on) {
          p.className = "pill pill-on";
          p.innerText = "BOMBA EN MARCHA";
        } else {
          p.className = "pill pill-off";
          p.innerText = "MOTOR DETENIDO (REPOSO FRÍO)";
        }
        document.getElementById('disp_dir').innerText = d.dir ? "Giro: Horario (Filtración FX100)" : "Giro: Antihorario (Retrolavado)";
      }).catch(e => {});
    }, 350);
  </script>
</body>
</html>
)rawliteral";

// ------------------------------------------------------------------------------
// 6. SETUP: INICIALIZACIÓN
// ------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(PIN_DIR, OUTPUT);
  digitalWrite(PIN_DIR, sentidoHorario ? HIGH : LOW);

  // Inicializar temporizador LEDC por hardware en D18
  ledcAttach(PIN_PUL, 1000, 8);
  ledcWriteTone(PIN_PUL, 0);

  Serial.println("\n========================================================");
  Serial.println("  PLANTA PILOTO DE ULTRAFILTRACIÓN INDUSTRIAL FX100     ");
  Serial.println("  HITO 1: ACCIONAMIENTO DE BOMBA CON BORNERA & WI-FI    ");
  Serial.println("========================================================");

  // Conexión Wi-Fi Modo Híbrido (Cliente + Punto de Acceso)
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid_red, password_red);
  Serial.printf("Conectando a red Wi-Fi: '%s' ...", ssid_red);

  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 18) {
    delay(300);
    Serial.print(".");
    intentos++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[OK] Conectado a Wi-Fi exitosamente.");
    Serial.printf("Acceso Web por IP: http://%s\n", WiFi.localIP().toString().c_str());
    if (MDNS.begin("bomba")) {
      Serial.println("Acceso Web por Dominio: http://bomba.local");
    }
  } else {
    Serial.println("\n[AVISO] No se encontró la red Box804. Creando Red Propia...");
    WiFi.softAP(ap_ssid, ap_pass);
    Serial.printf("Conéctate al Wi-Fi del ESP32: '%s' (Clave: '%s')\n", ap_ssid, ap_pass);
    Serial.printf("Abre en tu navegador: http://%s\n", WiFi.softAPIP().toString().c_str());
  }

  // Servicio de Actualización Inalámbrica (ArduinoOTA)
  ArduinoOTA.setHostname("bomba-uf");
  ArduinoOTA.setPassword("plantapiloto2");
  ArduinoOTA.begin();

  // Endpoints del Servidor Web
  server.on("/", HTTP_GET, [](){
    server.send_P(200, "text/html", index_html);
  });

  server.on("/set", HTTP_GET, [](){
    if (server.hasArg("rpm")) {
      float r = server.arg("rpm").toFloat();
      if (r >= 0.0f && r <= 130.0f) {
        rpm_objetivo = r;
      }
    }
    server.send(200, "text/plain", "OK");
  });

  server.on("/cmd", HTTP_GET, [](){
    if (server.hasArg("act")) {
      String act = server.arg("act");
      if (act == "START") {
        if (rpm_objetivo < 1.0f) rpm_objetivo = 30.0f;
        bombaEnMarcha = true;
        Serial.printf("[WEB] Bomba INICIADA a %.1f RPM.\n", rpm_objetivo);
      } else if (act == "STOP") {
        bombaEnMarcha = false;
        Serial.println("[WEB] Bomba DETENIDA.");
      } else if (act == "TOGGLE_DIR") {
        sentidoHorario = !sentidoHorario;
        digitalWrite(PIN_DIR, sentidoHorario ? HIGH : LOW);
        Serial.printf("[WEB] Sentido cambiado a: %s\n", sentidoHorario ? "Horario (Filtración)" : "Antihorario (Retrolavado)");
      }
    }
    server.send(200, "text/plain", "OK");
  });

  server.on("/status", HTTP_GET, [](){
    String ipActual = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    String json = "{";
    json += "\"on\":" + String(bombaEnMarcha ? "true" : "false") + ",";
    json += "\"rpm\":" + String(rpm_actual, 1) + ",";
    json += "\"dir\":" + String(sentidoHorario ? "true" : "false") + ",";
    json += "\"flow\":" + String(caudal_actual_Lmin, 3) + ",";
    json += "\"vol\":" + String(volumen_total_L, 2) + ",";
    json += "\"ip\":\"" + ipActual + "\"";
    json += "}";
    server.send(200, "application/json", json);
  });

  server.begin();
  Serial.println("[OK] Servidor Web activo en el puerto 80.");
  Serial.println("Comandos por Monitor Serie: 'R10', 'R30', 'R60', 'R100', 'DIR', 'STOP'");
}

// ------------------------------------------------------------------------------
// 7. LOOP: RAMPA, TELEMETRÍA, WEB Y PARSER SERIE
// ------------------------------------------------------------------------------
void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  unsigned long t_ahora = millis();
  float dt = (t_ahora - t_ultimo_loop_ms) / 1000.0f;

  if (dt >= 0.02f) { // Refresco cada 20 ms
    t_ultimo_loop_ms = t_ahora;

    // Aceleración y desaceleración suave
    if (bombaEnMarcha) {
      if (rpm_actual < rpm_objetivo) {
        rpm_actual += ACELERACION_RPM_SEG * dt;
        if (rpm_actual > rpm_objetivo) rpm_actual = rpm_objetivo;
      } else if (rpm_actual > rpm_objetivo) {
        rpm_actual -= ACELERACION_RPM_SEG * dt;
        if (rpm_actual < rpm_objetivo) rpm_actual = rpm_objetivo;
      }
    } else {
      if (rpm_actual > 0.0f) {
        rpm_actual -= (ACELERACION_RPM_SEG * 1.5f) * dt;
        if (rpm_actual < 0.0f) rpm_actual = 0.0f;
      }
    }

    fijarFrecuenciaMotor(rpm_actual);

    // Integración de Caudal y Volumen
    caudal_actual_Lmin = (rpm_actual * ML_POR_REVOLUCION) / 1000.0f;
    if (bombaEnMarcha && rpm_actual > 0.1f) {
      volumen_total_L += (caudal_actual_Lmin / 60.0f) * dt;
    }
  }

  // Parser por consola serie
  if (Serial.available()) {
    String c = Serial.readStringUntil('\n');
    c.trim();
    if (c.startsWith("R") || c.startsWith("r")) {
      float r = c.substring(1).toFloat();
      if (r >= 0.0f && r <= 130.0f) {
        rpm_objetivo = r;
        bombaEnMarcha = (r > 0.0f);
        Serial.printf("[SERIAL] Setpoint: %.1f RPM\n", rpm_objetivo);
      }
    } else if (c.equalsIgnoreCase("DIR")) {
      sentidoHorario = !sentidoHorario;
      digitalWrite(PIN_DIR, sentidoHorario ? HIGH : LOW);
      Serial.printf("[SERIAL] Sentido: %s\n", sentidoHorario ? "Horario" : "Antihorario");
    } else if (c.equalsIgnoreCase("STOP")) {
      bombaEnMarcha = false;
      Serial.println("[SERIAL] Bomba Detenida");
    }
  }
}
