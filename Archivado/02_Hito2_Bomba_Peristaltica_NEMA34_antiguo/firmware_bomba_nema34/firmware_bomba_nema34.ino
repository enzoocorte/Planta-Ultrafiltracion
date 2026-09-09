/* ==============================================================================
 * PROYECTO DE TESIS DE INGENIERÍA INDUSTRIAL - PLANTA DE ULTRAFILTRACIÓN FX100
 * HITO 2: ACCIONAMIENTO DE PRECISIÓN DE BOMBA PERISTÁLTICA (NEMA 34 + DM860)
 * ==============================================================================
 * 
 * DESCRIPCIÓN TÉCNICA:
 * Este firmware comanda un motor paso a paso NEMA 34 acoplado a una bomba 
 * peristáltica MBP-2000 mediante un driver industrial Leadshine DM860.
 * 
 * CARACTERÍSTICAS:
 * 1. Generación de pulsos STEP por Silicio Hardware (LEDC Timer) a 0% de uso de CPU.
 * 2. Rampa de aceleración/desaceleración suave (35 RPM/s) para evitar pérdida de pasos.
 * 3. Servidor Web responsivo embebido (Dashboard táctil para celular y PC).
 * 4. Control interactivo por consola serie (115200 baudios).
 * 5. Actualización inalámbrica de código por Wi-Fi (ArduinoOTA).
 * 
 * CONEXIONADO FÍSICO AL ESP32 (38 PINES):
 * - GPIO 18 (D18) ──► PUL+ (Paso / STEP del DM860)
 * - GPIO 19 (D19) ──► DIR+ (Sentido de Giro del DM860)
 * - GND           ──► PUL- y DIR- puenteados (Cátodo Común a masa)
 * - ENA+ / ENA-   ──► DESCONECTADOS (Driver habilitado permanentemente)
 * ============================================================================== */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

// ------------------------------------------------------------------------------
// 1. CREDENCIALES DE RED WI-FI (Planta Piloto)
// ------------------------------------------------------------------------------
const char* ssid     = "Box804";
const char* password = "plantapiloto2";

// Red de respaldo (Hotspot autónomo si no hay router disponible)
const char* ap_ssid  = "ESP32_Bomba_UF";
const char* ap_pass  = "plantapiloto2";

WebServer server(80);

// ------------------------------------------------------------------------------
// 2. ASIGNACIÓN DE PINES FÍSICOS AL DRIVER DM860
// ------------------------------------------------------------------------------
const uint8_t PIN_PUL = 18; // GPIO 18 -> Pulso STEP
const uint8_t PIN_DIR = 19; // GPIO 19 -> Dirección DIR

// Resolución del driver DM860 (DIP switches en 1600 pulsos por revolución)
const uint16_t PULSOS_POR_REV = 1600; 

// ------------------------------------------------------------------------------
// 3. VARIABLES DE ESTADO Y CONTROL DINÁMICO
// ------------------------------------------------------------------------------
bool bombaEncendida = false;
bool direccionHoraria = true; // true = Filtración hacia FX100, false = Retrolavado

float rpm_objetivo = 0.0;     // Velocidad deseada fijada por el usuario (0 a 120 RPM)
float rpm_actual   = 0.0;     // Velocidad instantánea en rampa
const float ACELERACION_RPM_SEG = 35.0; // Rampa: sube/baja 35 RPM por segundo

unsigned long t_ultima_rampa_ms = 0;

// ------------------------------------------------------------------------------
// 4. FUNCIÓN PARA GENERAR PULSOS POR HARDWARE LEDC TIMER
// ------------------------------------------------------------------------------
void actualizarFrecuenciaMotor(float rpm) {
  if (rpm > 0.5 && bombaEncendida) {
    // Ecuación de Frecuencia: f (Hz) = (RPM * 1600 pulsos/rev) / 60 seg
    float freq_hz = (rpm * (float)PULSOS_POR_REV) / 60.0f;
    ledcWriteTone(PIN_PUL, freq_hz); // Síntesis de onda cuadrada en silicio
  } else {
    ledcWriteTone(PIN_PUL, 0);       // Detiene los pulsos
    digitalWrite(PIN_PUL, LOW);
  }
}

// ------------------------------------------------------------------------------
// 5. CÓDIGO HTML/CSS/JS DEL DASHBOARD WEB EMBEBIDO
// ------------------------------------------------------------------------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Control Bomba MBP-2000 (NEMA 34)</title>
  <style>
    :root {
      --bg: #0b0f19; --card: #151d30; --border: #263554;
      --text: #f1f5f9; --accent: #38bdf8; --success: #10b981;
      --danger: #ef4444; --warning: #f59e0b;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', system-ui, sans-serif; }
    body { background: var(--bg); color: var(--text); padding: 20px; display: flex; justify-content: center; align-items: center; min-height: 100vh; }
    .card { background: var(--card); border: 1px solid var(--border); border-radius: 16px; padding: 24px; width: 100%; max-width: 480px; box-shadow: 0 10px 25px rgba(0,0,0,0.5); }
    h1 { font-size: 20px; color: var(--accent); margin-bottom: 4px; text-align: center; }
    p.sub { font-size: 12px; color: #94a3b8; text-align: center; margin-bottom: 20px; }
    .display-box { background: #070a12; border: 1px solid var(--border); border-radius: 12px; padding: 16px; text-align: center; margin-bottom: 18px; }
    .rpm-val { font-size: 48px; font-weight: 800; font-family: monospace; color: var(--accent); }
    .rpm-unit { font-size: 13px; color: #94a3b8; }
    .status-badge { display: inline-block; padding: 4px 12px; border-radius: 20px; font-size: 12px; font-weight: bold; margin-top: 8px; }
    .status-on { background: rgba(16, 185, 129, 0.2); color: var(--success); border: 1px solid var(--success); }
    .status-off { background: rgba(239, 68, 68, 0.2); color: var(--danger); border: 1px solid var(--danger); }
    .slider-box { margin-bottom: 20px; }
    .slider-label { display: flex; justify-content: space-between; font-size: 13px; margin-bottom: 8px; }
    input[type=range] { width: 100%; height: 8px; border-radius: 4px; background: #263554; accent-color: var(--accent); cursor: pointer; }
    .btn-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-bottom: 12px; }
    .btn { padding: 14px; border: none; border-radius: 8px; font-size: 14px; font-weight: bold; cursor: pointer; transition: 0.2s; }
    .btn:active { transform: scale(0.97); }
    .btn-start { background: var(--accent); color: #090d16; }
    .btn-stop { background: var(--danger); color: #fff; }
    .btn-dir { background: var(--warning); color: #090d16; grid-column: span 2; }
    .info-footer { font-size: 11px; color: #64748b; text-align: center; margin-top: 14px; line-height: 1.4; }
  </style>
</head>
<body>
  <div class="card">
    <h1>PLANTA DE ULTRAFILTRACIÓN FX100</h1>
    <p class="sub">Hito 2: Accionamiento Bomba Peristáltica (NEMA 34)</p>

    <div class="display-box">
      <div class="rpm-val" id="rpm_val">0.0</div>
      <div class="rpm-unit">RPM (Frecuencia: <span id="freq_val">0</span> Hz)</div>
      <div id="status_pill" class="status-badge status-off">MOTOR DETENIDO</div>
      <div style="font-size: 12px; color: #94a3b8; margin-top: 6px;" id="dir_txt">Giro: Horario (Filtración hacia FX100)</div>
    </div>

    <div class="slider-box">
      <div class="slider-label">
        <span>Fijar Velocidad de Bomba:</span>
        <strong id="slider_txt" style="color: var(--accent);">30 RPM</strong>
      </div>
      <input type="range" id="rpm_slider" min="0" max="120" value="30" oninput="actualizarSlider(this.value)">
    </div>

    <div class="btn-grid">
      <button class="btn btn-start" onclick="enviarComando('START')">▶ INICIAR</button>
      <button class="btn btn-stop" onclick="enviarComando('STOP')">⏹ DETENER</button>
      <button class="btn btn-dir" onclick="enviarComando('TOGGLE_DIR')">🔄 INVERTIR SENTIDO DE GIRO</button>
    </div>

    <div class="info-footer">
      ESP32 Hardware LEDC Timer (GPIO 18 / 19) • Driver DM860 (1600 P/R)<br>
      IP: <span id="ip_txt">Conectando...</span> • OTA Habilitado
    </div>
  </div>

  <script>
    function actualizarSlider(val) {
      document.getElementById('slider_txt').innerText = val + " RPM";
      fetch('/set?rpm=' + val);
    }
    function enviarComando(accion) {
      fetch('/cmd?action=' + accion);
    }

    setInterval(() => {
      fetch('/status').then(r => r.json()).then(d => {
        document.getElementById('rpm_val').innerText = d.rpm.toFixed(1);
        document.getElementById('freq_val').innerText = Math.round((d.rpm * 1600) / 60);
        document.getElementById('ip_txt').innerText = d.ip;
        
        let pill = document.getElementById('status_pill');
        if (d.on) {
          pill.className = "status-badge status-on";
          pill.innerText = "BOMBA EN MARCHA";
        } else {
          pill.className = "status-badge status-off";
          pill.innerText = "MOTOR DETENIDO";
        }
        document.getElementById('dir_txt').innerText = "Giro: " + (d.dir ? "Horario (Filtración hacia FX100)" : "Antihorario (Retrolavado)");
      }).catch(e => {});
    }, 350);
  </script>
</body>
</html>
)rawliteral";

// ------------------------------------------------------------------------------
// 6. SETUP: INICIALIZACIÓN DE HARDWARE Y SERVICIOS
// ------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_DIR, OUTPUT);
  digitalWrite(PIN_DIR, direccionHoraria);

  // Inicializar temporizador LEDC por hardware en PIN_PUL (GPIO 18)
  ledcAttach(PIN_PUL, 1000, 8);
  ledcWriteTone(PIN_PUL, 0); // Frecuencia 0 al arrancar

  Serial.println("\n========================================================");
  Serial.println("  PLANTA DE ULTRAFILTRACIÓN INDUSTRIAL - HITO 2         ");
  Serial.println("  Control de Bomba NEMA 34 (GPIO 18 PUL, GPIO 19 DIR)  ");
  Serial.println("========================================================");

  // Conexión Wi-Fi Modo Híbrido (Cliente STA + Punto de Acceso AP)
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  Serial.printf("Conectando a la red Wi-Fi: '%s' ...", ssid);

  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(350);
    Serial.print(".");
    intentos++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[OK] Wi-Fi Conectado exitosamente.");
    Serial.printf("Dirección IP: http://%s\n", WiFi.localIP().toString().c_str());
    
    if (MDNS.begin("bomba-uf")) {
      Serial.println("Acceso por Dominio mDNS: http://bomba-uf.local");
    }
  } else {
    Serial.println("\n[AVISO] No se pudo conectar a la red Box804.");
    Serial.println("Iniciando Red Wi-Fi Propia (Hotspot de Respaldo)...");
    WiFi.softAP(ap_ssid, ap_pass);
    Serial.printf("Conéctate al Wi-Fi: '%s' (Clave: '%s')\n", ap_ssid, ap_pass);
    Serial.printf("Abre en tu navegador: http://%s\n", WiFi.softAPIP().toString().c_str());
  }

  // Configuración del servicio ArduinoOTA (Carga Inalámbrica por Wi-Fi)
  ArduinoOTA.setHostname("bomba-uf");
  ArduinoOTA.setPassword("plantapiloto2");

  ArduinoOTA.onStart([]() {
    bombaEncendida = false;
    actualizarFrecuenciaMotor(0.0);
    Serial.println("\n[OTA] Iniciando reprogramación de firmware por Wi-Fi...");
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\n[OTA] Actualización completada con éxito. Reiniciando...");
  });

  ArduinoOTA.begin();
  Serial.println("[OK] Servicio ArduinoOTA iniciado.");

  // Endpoints del Servidor Web
  server.on("/", HTTP_GET, [](){
    server.send_P(200, "text/html", index_html);
  });

  server.on("/set", HTTP_GET, [](){
    if (server.hasArg("rpm")) {
      float r = server.arg("rpm").toFloat();
      if (r >= 0.0f && r <= 150.0f) {
        rpm_objetivo = r;
        if (r > 0.0f && !bombaEncendida) {
          bombaEncendida = true;
        }
      }
    }
    server.send(200, "text/plain", "OK");
  });

  server.on("/cmd", HTTP_GET, [](){
    if (server.hasArg("action")) {
      String act = server.arg("action");
      if (act == "START") {
        if (rpm_objetivo < 1.0f) rpm_objetivo = 30.0f;
        bombaEncendida = true;
        Serial.printf("[COMANDO] Bomba INICIADA. Acelerando a %.1f RPM...\n", rpm_objetivo);
      } else if (act == "STOP") {
        bombaEncendida = false;
        Serial.println("[COMANDO] Bomba DETENIDA. Desacelerando a 0 RPM...");
      } else if (act == "TOGGLE_DIR") {
        direccionHoraria = !direccionHoraria;
        digitalWrite(PIN_DIR, direccionHoraria);
        Serial.printf("[COMANDO] Sentido cambiado a: %s\n", direccionHoraria ? "Horario (Filtración)" : "Antihorario (Retrolavado)");
      }
    }
    server.send(200, "text/plain", "OK");
  });

  server.on("/status", HTTP_GET, [](){
    String ipStr = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    String json = "{";
    json += "\"on\":" + String(bombaEncendida ? "true" : "false") + ",";
    json += "\"rpm\":" + String(rpm_actual, 1) + ",";
    json += "\"setpoint\":" + String(rpm_objetivo, 1) + ",";
    json += "\"dir\":" + String(direccionHoraria ? "true" : "false") + ",";
    json += "\"ip\":\"" + ipStr + "\"";
    json += "}";
    server.send(200, "application/json", json);
  });

  server.begin();
  Serial.println("[OK] Servidor Web iniciado en el puerto 80.");
  Serial.println("\nComandos disponibles en el Monitor Serie:");
  Serial.println("  'S' -> Iniciar Bomba");
  Serial.println("  'X' -> Detener Bomba");
  Serial.println("  'R' -> Invertir Sentido de Giro");
  Serial.println("  'V30', 'V60', 'V100' -> Fijar RPM");
}

// ------------------------------------------------------------------------------
// 7. LOOP PRINCIPAL: GESTIÓN DE RAMPA, WEB, SERIAL Y OTA
// ------------------------------------------------------------------------------
void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  unsigned long t_ahora = millis();
  float dt = (t_ahora - t_ultima_rampa_ms) / 1000.0f;

  if (dt >= 0.02f) { // Actualizar rampa suave cada 20 ms
    t_ultima_rampa_ms = t_ahora;

    if (bombaEncendida) {
      if (rpm_actual < rpm_objetivo) {
        rpm_actual += ACELERACION_RPM_SEG * dt;
        if (rpm_actual > rpm_objetivo) rpm_actual = rpm_objetivo;
        actualizarFrecuenciaMotor(rpm_actual);
      } else if (rpm_actual > rpm_objetivo) {
        rpm_actual -= ACELERACION_RPM_SEG * dt;
        if (rpm_actual < rpm_objetivo) rpm_actual = rpm_objetivo;
        actualizarFrecuenciaMotor(rpm_actual);
      }
    } else {
      if (rpm_actual > 0.0f) {
        rpm_actual -= (ACELERACION_RPM_SEG * 1.5f) * dt; // Desacelera un 50% más rápido
        if (rpm_actual < 0.0f) rpm_actual = 0.0f;
        actualizarFrecuenciaMotor(rpm_actual);
      }
    }
  }

  // ----------------------------------------------------------------------------
  // PARSER DE COMANDOS POR MONITOR SERIE (Para pruebas en mesa)
  // ----------------------------------------------------------------------------
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.equalsIgnoreCase("S")) {
      if (rpm_objetivo < 1.0f) rpm_objetivo = 30.0f;
      bombaEncendida = true;
      Serial.printf("[SERIAL] ▶ Bomba Iniciada a %.1f RPM.\n", rpm_objetivo);
    } else if (cmd.equalsIgnoreCase("X")) {
      bombaEncendida = false;
      Serial.println("[SERIAL] ⏹ Bomba Detenida.");
    } else if (cmd.equalsIgnoreCase("R")) {
      direccionHoraria = !direccionHoraria;
      digitalWrite(PIN_DIR, direccionHoraria);
      Serial.printf("[SERIAL] 🔄 Sentido de Giro: %s\n", direccionHoraria ? "Horario" : "Antihorario");
    } else if (cmd.startsWith("V") || cmd.startsWith("v")) {
      float r = cmd.substring(1).toFloat();
      if (r >= 0.0f && r <= 150.0f) {
        rpm_objetivo = r;
        if (r > 0.0f) bombaEncendida = true;
        Serial.printf("[SERIAL] ⚡ Velocidad fijada a %.1f RPM.\n", rpm_objetivo);
      }
    }
  }
}
