/*
 * ===================================================================
 * HITO 1: CONTROL DE BOMBA NEMA 34 CON DASHBOARD WEB + ACTUALIZACIÓN OTA
 * ARQUITECTURA POR HARDWARE TIMER LEDC (PULSOS DETERMINÍSTICOS EN SILICIO)
 * ===================================================================
 * Planta Piloto de Ultrafiltración - Tesis de Ingeniería Industrial
 * Microcontrolador: ESP32 DevKit V1 (30/38 pines)
 * Temporizador: Hardware LEDC Timer (0% CPU, pulsos puros por silicio)
 * Driver: Leadshine DM860 (1600 pulsos/rev) - Conexión Cátodo Común
 * Pines Físicos: PUL=GPIO 25, DIR=GPIO 26, ENA=GPIO 27
 * Red Wi-Fi: "Box804" / Clave: "plantapiloto2"
 * ===================================================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

// 1. CREDENCIALES DE RED WI-FI (Laboratorio / Planta)
const char* ssid = "Box804";
const char* password = "plantapiloto2";

// Red de Respaldo propia (Hotspot si no hay Wi-Fi disponible)
const char* ap_ssid = "ESP32-Bomba-UF";
const char* ap_pass = "12345678";

WebServer server(80);

// 2. ASIGNACIÓN DE PINES FÍSICOS AL DRIVER DM860
// Configuración: ENA desconectado (habilitado permanente), PUL=GPIO 18, DIR=GPIO 19
const uint8_t PIN_PUL = 18; // Pulso / Paso (STEP) -> GPIO 18 (D18)
const uint8_t PIN_DIR = 19; // Dirección (DIR)       -> GPIO 19 (D19)

const uint16_t PULSOS_POR_REV = 1600; // DM860 en 1600 pulsos/rev

// 3. VARIABLES DE ESTADO Y CONTROL
bool bombaEncendida = false;
bool direccionHoraria = true; // true = Horario (Filtración), false = Antihorario (Retrolavado)

float rpm_objetivo = 0.0;
float rpm_actual = 0.0;
const float ACELERACION_RPM_SEG = 35.0; // Rampa: 35 RPM por segundo

unsigned long t_ultima_rampa_ms = 0;

// Función para actualizar la frecuencia del temporizador de hardware LEDC
void actualizarFrecuenciaMotor(float rpm) {
  if (rpm > 0.5 && bombaEncendida) {
    // Ecuación: Frecuencia (Hz) = (RPM * 1600 pulsos/rev) / 60 segundos
    float freq_hz = (rpm * (float)PULSOS_POR_REV) / 60.0f;
    ledcWriteTone(PIN_PUL, freq_hz);
  } else {
    ledcWriteTone(PIN_PUL, 0); // Apaga los pulsos
    digitalWrite(PIN_PUL, LOW);
  }
}

// 4. INTERFAZ WEB HTML5 EMBEBIDA EN MEMORIA FLASH
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Planta Piloto UF - Hito 1</title>
  <style>
    body { font-family: 'Segoe UI', system-ui, -apple-system, sans-serif; background: #0f172a; color: #f8fafc; text-align: center; margin: 0; padding: 20px; }
    .card { background: #1e293b; border-radius: 16px; max-width: 480px; margin: auto; padding: 28px; box-shadow: 0 10px 25px rgba(0,0,0,0.5); border: 1px solid #334155; }
    h1 { color: #38bdf8; font-size: 20px; margin-bottom: 4px; letter-spacing: 0.5px; }
    h3 { color: #94a3b8; font-size: 13px; margin-top: 0; font-weight: normal; }
    .display-box { background: #090d16; border-radius: 12px; padding: 20px; margin: 20px 0; border: 1px solid #1e293b; }
    .val-box { font-size: 52px; font-weight: 800; color: #10b981; font-family: monospace; }
    .unit { font-size: 18px; color: #64748b; }
    .status-badge { display: inline-block; padding: 6px 14px; border-radius: 20px; font-size: 12px; font-weight: bold; margin-top: 8px; }
    .status-on { background: #065f46; color: #34d399; }
    .status-off { background: #450a0a; color: #f87171; }
    .btn { background: #38bdf8; color: #0f172a; border: none; padding: 14px 20px; font-size: 15px; font-weight: bold; border-radius: 8px; cursor: pointer; margin: 6px; width: 45%; transition: 0.2s; }
    .btn:active { transform: scale(0.96); }
    .btn-stop { background: #ef4444; color: #fff; }
    .btn-dir { background: #f59e0b; color: #0f172a; width: 94%; }
    .slider-box { margin: 25px 0; text-align: left; background: #090d16; padding: 16px; border-radius: 8px; border: 1px solid #334155; }
    .slider-box label { font-size: 14px; color: #94a3b8; font-weight: 600; }
    input[type=range] { width: 100%; height: 8px; border-radius: 5px; background: #334155; outline: none; margin-top: 12px; accent-color: #38bdf8; }
    .info-footer { font-size: 11px; color: #64748b; margin-top: 18px; line-height: 1.4; }
  </style>
</head>
<body>
  <div class="card">
    <h1>PLANTA DE ULTRAFILTRACIÓN</h1>
    <h3>HITO 1: BOMBA PERISTÁLTICA MBP-2000 (NEMA 34)</h3>
    
    <div class="display-box">
      <div class="val-box"><span id="rpm_val">0.0</span> <span class="unit">RPM</span></div>
      <div id="status_pill" class="status-badge status-off">MOTOR DETENIDO</div>
      <div id="dir_txt" style="font-size: 13px; color: #94a3b8; margin-top: 10px;">Giro: Horario (Filtración)</div>
    </div>

    <div class="slider-box">
      <label>Velocidad Objetivo: <span id="target_val" style="color:#38bdf8; font-weight: bold;">30</span> RPM</label>
      <input type="range" min="0" max="120" value="30" id="rpm_slider" oninput="updateSlider(this.value)" onchange="sendRPM(this.value)">
    </div>

    <div>
      <button class="btn" onclick="sendCommand('START')">▶ INICIAR</button>
      <button class="btn btn-stop" onclick="sendCommand('STOP')">⏹ DETENER</button>
    </div>
    <br>
    <div>
      <button class="btn btn-dir" onclick="sendCommand('TOGGLE_DIR')">🔄 INVERTIR GIRO (Filtración / Backwash)</button>
    </div>

    <div class="info-footer">
      Control Determinístico por Hardware LEDC Timer<br>
      OTA Habilitada: Carga inalámbrica activa
    </div>
  </div>

  <script>
    function updateSlider(val) {
      document.getElementById('target_val').innerText = val;
    }
    function sendRPM(val) {
      fetch('/set?rpm=' + val);
    }
    function sendCommand(cmd) {
      fetch('/cmd?action=' + cmd);
    }

    setInterval(function() {
      fetch('/status').then(r => r.json()).then(d => {
        document.getElementById('rpm_val').innerText = d.rpm.toFixed(1);
        let pill = document.getElementById('status_pill');
        if (d.on) {
          pill.className = "status-badge status-on";
          pill.innerText = "BOMBA EN MARCHA";
        } else {
          pill.className = "status-badge status-off";
          pill.innerText = "MOTOR DETENIDO";
        }
        document.getElementById('dir_txt').innerText = "Giro: " + (d.dir ? "Horario (Filtración hacia FX100)" : "Antihorario (Retrolavado)");
      }).catch(err => {});
    }, 400);
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_DIR, OUTPUT);
  digitalWrite(PIN_DIR, direccionHoraria);

  // Inicializar temporizador LEDC por hardware en PIN_PUL (GPIO 18)
  ledcAttach(PIN_PUL, 1000, 8);
  ledcWriteTone(PIN_PUL, 0); // Frecuencia 0 al arrancar

  Serial.println("\n==================================================");
  Serial.println("  PLANTA DE ULTRAFILTRACIÓN INDUSTRIAL - HITO 1   ");
  Serial.println("  Control por Hardware LEDC Timer (PUL=18, DIR=19)");
  Serial.println("==================================================");

  // Conexión Wi-Fi Modo Híbrido
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  Serial.printf("Conectando a la red Wi-Fi: '%s' ...", ssid);

  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(400);
    Serial.print(".");
    intentos++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[OK] Wi-Fi Conectado con éxito.");
    Serial.printf("IP Asignada: http://%s\n", WiFi.localIP().toString().c_str());
    
    if (MDNS.begin("bomba-uf")) {
      Serial.println("Acceso por dominio mDNS: http://bomba-uf.local");
    }
  } else {
    Serial.println("\n[AVISO] No se pudo conectar a la red Box804.");
    Serial.println("Levantando Red Wi-Fi Propia (Hotspot de Respaldo)...");
    WiFi.softAP(ap_ssid, ap_pass);
    Serial.printf("Conéctate al Wi-Fi: '%s' (Clave: '%s')\n", ap_ssid, ap_pass);
    Serial.printf("Abre en tu navegador: http://%s\n", WiFi.softAPIP().toString().c_str());
  }

  // Configuración OTA
  ArduinoOTA.setHostname("bomba-uf");
  ArduinoOTA.setPassword("plantapiloto2");

  ArduinoOTA.onStart([]() {
    bombaEncendida = false;
    actualizarFrecuenciaMotor(0.0);
    Serial.println("\n[OTA] Iniciando carga de nuevo firmware por Wi-Fi...");
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\n[OTA] Actualización completada con éxito. Reiniciando...");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA ERROR %u]\n", error);
  });

  ArduinoOTA.begin();
  Serial.println("[OK] Servicio ArduinoOTA iniciado.");

  // Rutas del Servidor Web
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
      } else if (act == "STOP") {
        bombaEncendida = false;
      } else if (act == "TOGGLE_DIR") {
        direccionHoraria = !direccionHoraria;
        digitalWrite(PIN_DIR, direccionHoraria);
      }
    }
    server.send(200, "text/plain", "OK");
  });

  server.on("/status", HTTP_GET, [](){
    String json = "{\"rpm\":" + String(rpm_actual, 1) + 
                  ",\"on\":" + String(bombaEncendida ? "true" : "false") + 
                  ",\"dir\":" + String(direccionHoraria ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });

  server.begin();
  Serial.println("[OK] Servidor Web iniciado.");
  Serial.println("--------------------------------------------------");
}

void loop() {
  // 1. Atender solicitudes del Servidor Web y OTA
  server.handleClient();
  ArduinoOTA.handle();

  // 2. Rampa de Aceleración Suave (calculada cada 20 ms)
  unsigned long ahora_ms = millis();
  if (ahora_ms - t_ultima_rampa_ms >= 20) {
    float dt = (ahora_ms - t_ultima_rampa_ms) / 1000.0f;
    t_ultima_rampa_ms = ahora_ms;

    if (bombaEncendida) {
      if (rpm_actual < rpm_objetivo) {
        rpm_actual += ACELERACION_RPM_SEG * dt;
        if (rpm_actual > rpm_objetivo) rpm_actual = rpm_objetivo;
      } else if (rpm_actual > rpm_objetivo) {
        rpm_actual -= ACELERACION_RPM_SEG * dt;
        if (rpm_actual < rpm_objetivo) rpm_actual = rpm_objetivo;
      }
      actualizarFrecuenciaMotor(rpm_actual);
    } else {
      if (rpm_actual > 0.0f) {
        rpm_actual -= ACELERACION_RPM_SEG * dt * 2.0f;
        if (rpm_actual <= 0.0f) {
          rpm_actual = 0.0f;
        }
        actualizarFrecuenciaMotor(rpm_actual);
      }
    }
  }
}
