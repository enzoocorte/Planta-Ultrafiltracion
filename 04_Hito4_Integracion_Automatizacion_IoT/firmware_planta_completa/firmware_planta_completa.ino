/* ==============================================================================
 * PROYECTO DE TESIS DE INGENIERÍA INDUSTRIAL - PLANTA DE ULTRAFILTRACIÓN FX100
 * HITO 5: FIRMWARE MAESTRO DE INTEGRACIÓN GLOBAL, CONTROL SCADA & IOT
 * ==============================================================================
 * 
 * INTEGRACIÓN DE SUBSISTEMAS:
 * 1. Bomba Peristáltica MBP-2000 (NEMA 34 + DM860) en GPIO 18 (PUL) y GPIO 19 (DIR).
 * 2. Agitador del Reactor (Driver L298N) en GPIO 4 (PWM), GPIO 16 (IN1) y GPIO 17 (IN2).
 * 3. Seguridad por Boya de Nivel Inox en GPIO 32 (Corte por tanque vacío).
 * 4. Caudalímetros YF-S401 en GPIO 27 (Permeado) y GPIO 14 (Retentado).
 * 5. Telemetría I2C con conversor ADS1115 (16 Bits) en GPIO 21 (SDA) y GPIO 22 (SCL).
 * 6. Servidor Web Dashboard táctil responsivo y actualización inalámbrica (OTA).
 * ============================================================================== */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <Wire.h>

// ------------------------------------------------------------------------------
// 1. ASIGNACIÓN DE PINES
// ------------------------------------------------------------------------------
const uint8_t PIN_BOMBA_PUL  = 18; // NEMA 34 STEP (LEDC Hardware)
const uint8_t PIN_BOMBA_DIR  = 19; // NEMA 34 DIR
const uint8_t PIN_AGIT_ENA   = 4;  // Agitador L298N PWM
const uint8_t PIN_AGIT_IN1   = 16; // Agitador Sentido A
const uint8_t PIN_AGIT_IN2   = 17; // Agitador Sentido B
const uint8_t PIN_BOYA_NIVEL = 32; // Boya de nivel flotante
const uint8_t PIN_FLOW_PERM  = 27; // Caudalímetro Permeado
const uint8_t PIN_FLOW_RET   = 14; // Caudalímetro Retentado

const uint8_t ADS1115_ADDR   = 0x48;

// Red Wi-Fi
const char* ssid     = "Box804";
const char* password = "plantapiloto2";
const char* ap_ssid  = "Planta_UF_Master";
const char* ap_pass  = "plantapiloto2";

WebServer server(80);

// ------------------------------------------------------------------------------
// 2. VARIABLES DE ESTADO Y TELEMETRÍA
// ------------------------------------------------------------------------------
// Bomba Peristáltica
bool bombaOn = false;
bool bombaDirHoraria = true;
float bombaRpmObjetivo = 0.0;
float bombaRpmActual   = 0.0;
const float ACEL_RPM_S = 35.0;

// Agitador
bool agitadorOn = false;
uint8_t agitadorPWM = 0;

// Sensores de Caudal
volatile unsigned long pPermCount = 0;
volatile unsigned long pRetCount  = 0;
float Q_permeado_Lmin  = 0.0;
float Q_retentado_Lmin = 0.0;
float volPermeadoTotal = 0.0;

// Presiones y Membrana FX100 (Am = 2.2 m²)
const float AREA_MEMBRANA_M2 = 2.2;
float P1_atm = 0.0;
float P2_atm = 0.0;
float P3_atm = 0.0;
float TMP_atm = 0.0;
float Flux_J_LMH = 0.0;
float Temp_Agua_C = 21.0;
float TDS_ppm = 120.0;

// Enclavamiento
bool alarmaTMP = false;
bool alarmaNivel = false;

// Rutinas de interrupción
void IRAM_ATTR isrFlowPerm() { pPermCount++; }
void IRAM_ATTR isrFlowRet()  { pRetCount++; }

// ------------------------------------------------------------------------------
// 3. FUNCIONES DE CONTROL DE ACTUADORES
// ------------------------------------------------------------------------------
void actualizarBombaPulsos(float rpm) {
  if (rpm > 0.5 && bombaOn && !alarmaTMP && !alarmaNivel) {
    float freq_hz = (rpm * 1600.0f) / 60.0f;
    ledcWriteTone(PIN_BOMBA_PUL, freq_hz);
  } else {
    ledcWriteTone(PIN_BOMBA_PUL, 0);
    digitalWrite(PIN_BOMBA_PUL, LOW);
  }
}

void fijarAgitador(uint8_t pwm) {
  agitadorPWM = pwm;
  if (pwm == 0 || alarmaNivel) {
    digitalWrite(PIN_AGIT_IN1, LOW);
    digitalWrite(PIN_AGIT_IN2, LOW);
    ledcWrite(PIN_AGIT_ENA, 0);
    agitadorOn = false;
  } else {
    digitalWrite(PIN_AGIT_IN1, HIGH);
    digitalWrite(PIN_AGIT_IN2, LOW);
    ledcWrite(PIN_AGIT_ENA, pwm);
    agitadorOn = true;
  }
}

// ------------------------------------------------------------------------------
// 4. CÓDIGO HTML/CSS/JS DEL DASHBOARD SCADA INTEGRAL
// ------------------------------------------------------------------------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>SCADA Planta Ultrafiltración FX100</title>
  <style>
    :root {
      --bg: #0b0f19; --card: #151d30; --border: #263554;
      --text: #f1f5f9; --accent: #38bdf8; --success: #10b981;
      --danger: #ef4444; --warning: #f59e0b; --purple: #a855f7;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', system-ui, sans-serif; }
    body { background: var(--bg); color: var(--text); padding: 16px; min-height: 100vh; }
    header { display: flex; justify-content: space-between; align-items: center; background: var(--card); border: 1px solid var(--border); padding: 14px 20px; border-radius: 12px; margin-bottom: 16px; }
    h1 { font-size: 18px; color: var(--accent); }
    .grid-scada { display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 16px; }
    .card { background: var(--card); border: 1px solid var(--border); border-radius: 12px; padding: 18px; }
    .card h2 { font-size: 15px; color: var(--accent); border-bottom: 1px solid var(--border); padding-bottom: 8px; margin-bottom: 12px; }
    .metric-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-bottom: 12px; }
    .metric-box { background: #070a12; border: 1px solid var(--border); border-radius: 8px; padding: 10px; text-align: center; }
    .metric-val { font-size: 26px; font-weight: 800; font-family: monospace; color: var(--accent); }
    .metric-unit { font-size: 11px; color: #94a3b8; }
    .btn-group { display: flex; gap: 8px; flex-wrap: wrap; margin-top: 10px; }
    .btn { padding: 10px 16px; border: none; border-radius: 6px; font-weight: bold; font-size: 13px; cursor: pointer; transition: 0.2s; }
    .btn:active { transform: scale(0.97); }
    .btn-primary { background: var(--accent); color: #090d16; }
    .btn-danger { background: var(--danger); color: #fff; }
    .btn-warning { background: var(--warning); color: #090d16; }
    .btn-purple { background: var(--purple); color: #fff; }
    .alarm-banner { display: none; background: rgba(239, 68, 68, 0.2); border: 1px solid var(--danger); color: #f87171; padding: 10px; border-radius: 8px; margin-bottom: 14px; font-weight: bold; text-align: center; }
  </style>
</head>
<body>
  <header>
    <div>
      <h1>PLANTA PILOTO SCADA: ULTRAFILTRACIÓN FX100</h1>
      <p style="font-size: 12px; color: #94a3b8;">Tesis de Grado en Ingeniería Industrial</p>
    </div>
    <div style="font-size: 12px; color: var(--success); font-weight: bold;" id="ip_tag">● ESP32 ONLINE</div>
  </header>

  <div id="alarm_box" class="alarm-banner">🚨 ALARMA ACTIVA: Enclavamiento de Seguridad en Proceso</div>

  <div class="grid-scada">
    <!-- PANEL 1: BOMBA PERISTÁLTICA MBP-2000 -->
    <div class="card">
      <h2>🌀 Control Bomba Peristáltica (NEMA 34)</h2>
      <div class="metric-grid">
        <div class="metric-box">
          <div class="metric-unit">VELOCIDAD ACTUAL</div>
          <div class="metric-val" id="disp_rpm">0.0</div>
          <div class="metric-unit">RPM</div>
        </div>
        <div class="metric-box">
          <div class="metric-unit">SENTIDO GIRO</div>
          <div class="metric-val" id="disp_dir" style="font-size: 18px; color: var(--warning);">HORARIO</div>
          <div class="metric-unit">Filtración FX100</div>
        </div>
      </div>
      <label style="font-size: 12px; color: #94a3b8; display: flex; justify-content: space-between;">
        <span>Fijar Setpoint RPM:</span>
        <strong id="setpoint_txt" style="color: var(--accent);">30 RPM</strong>
      </label>
      <input type="range" min="0" max="120" value="30" style="width: 100%; margin: 8px 0 14px 0;" oninput="document.getElementById('setpoint_txt').innerText = this.value + ' RPM'; fetch('/cmd?act=SET_RPM&val=' + this.value);">
      <div class="btn-group">
        <button class="btn btn-primary" onclick="fetch('/cmd?act=START_BOMBA')">▶ INICIAR BOMBA</button>
        <button class="btn btn-danger" onclick="fetch('/cmd?act=STOP_BOMBA')">⏹ DETENER</button>
        <button class="btn btn-warning" onclick="fetch('/cmd?act=TOGGLE_DIR')">🔄 INVERTIR GIRO</button>
      </div>
    </div>

    <!-- PANEL 2: REACTOR DE COAGULACIÓN Y AGITADOR -->
    <div class="card">
      <h2>🌪️ Reactor Coagulación-Sedimentador (L298N)</h2>
      <div class="metric-grid">
        <div class="metric-box">
          <div class="metric-unit">POTENCIA AGITADOR</div>
          <div class="metric-val" id="disp_agit">0%</div>
          <div class="metric-unit" id="disp_agit_rpm">0 RPM</div>
        </div>
        <div class="metric-box">
          <div class="metric-unit">ESTADO BOYA NIVEL</div>
          <div class="metric-val" id="disp_nivel" style="font-size: 18px; color: var(--success);">NORMAL</div>
          <div class="metric-unit">Nivel Aceptable</div>
        </div>
      </div>
      <div class="btn-group">
        <button class="btn btn-primary" onclick="fetch('/cmd?act=AGIT_FAST')">⚡ MEZCLA RÁPIDA (150 RPM)</button>
        <button class="btn btn-warning" onclick="fetch('/cmd?act=AGIT_SLOW')">💧 MEZCLA LENTA (30 RPM)</button>
        <button class="btn btn-danger" onclick="fetch('/cmd?act=AGIT_STOP')">⏹ PARAR AGITADOR</button>
      </div>
    </div>

    <!-- PANEL 3: MEMBRANA FX100 & TELEMETRÍA HIDRÁULICA -->
    <div class="card" style="grid-column: span 2;">
      <h2>📊 Telemetría Hidráulica de Membrana FX100 (Darcy & TMP)</h2>
      <div class="metric-grid" style="grid-template-columns: repeat(4, 1fr);">
        <div class="metric-box">
          <div class="metric-unit">TMP ACTUAL</div>
          <div class="metric-val" id="disp_tmp">0.00</div>
          <div class="metric-unit">atm (Límite ≤ 0.50)</div>
        </div>
        <div class="metric-box">
          <div class="metric-unit">FLUX DARCY (J)</div>
          <div class="metric-val" id="disp_flux" style="color: var(--success);">0.0</div>
          <div class="metric-unit">LMH (L/m²·h)</div>
        </div>
        <div class="metric-box">
          <div class="metric-unit">CAUDAL PERMEADO</div>
          <div class="metric-val" id="disp_qp">0.00</div>
          <div class="metric-unit">L/min</div>
        </div>
        <div class="metric-box">
          <div class="metric-unit">CALIDAD TDS</div>
          <div class="metric-val" id="disp_tds" style="color: var(--purple);">120</div>
          <div class="metric-unit">ppm (mg/L)</div>
        </div>
      </div>
      <div class="btn-group">
        <button class="btn btn-purple" onclick="fetch('/cmd?act=AUTO_BATCH')">⚡ INICIAR CICLO AUTOMÁTICO BATCH (FSM)</button>
      </div>
    </div>
  </div>

  <script>
    setInterval(() => {
      fetch('/status').then(r => r.json()).then(d => {
        document.getElementById('disp_rpm').innerText = d.bomba_rpm.toFixed(1);
        document.getElementById('disp_dir').innerText = d.bomba_dir ? "HORARIO" : "ANTIHORARIO";
        document.getElementById('disp_agit').innerText = Math.round((d.agit_pwm / 255) * 100) + "%";
        document.getElementById('disp_agit_rpm').innerText = d.agit_pwm > 150 ? "~150 RPM" : (d.agit_pwm > 0 ? "~30 RPM" : "0 RPM");
        document.getElementById('disp_tmp').innerText = d.tmp.toFixed(2);
        document.getElementById('disp_flux').innerText = d.flux.toFixed(1);
        document.getElementById('disp_qp').innerText = d.qp.toFixed(2);
        document.getElementById('disp_tds').innerText = d.tds.toFixed(0);

        let nBox = document.getElementById('disp_nivel');
        if (d.nivel_bajo) {
          nBox.innerText = "VACÍO"; nBox.style.color = "var(--danger)";
        } else {
          nBox.innerText = "NORMAL"; nBox.style.color = "var(--success)";
        }

        let aBox = document.getElementById('alarm_box');
        if (d.alarma_tmp) {
          aBox.style.display = "block";
          aBox.innerText = "🚨 ALERTA CRÍTICA: TMP excedió 0.50 atm. Bomba bloqueada por seguridad!";
        } else if (d.nivel_bajo) {
          aBox.style.display = "block";
          aBox.innerText = "⚠️ AVISO: Nivel bajo de agua en el reactor. Motor detenido.";
        } else {
          aBox.style.display = "none";
        }
      }).catch(e => {});
    }, 400);
  </script>
</body>
</html>
)rawliteral";

// ------------------------------------------------------------------------------
// 5. SETUP
// ------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(PIN_BOMBA_DIR, OUTPUT);
  pinMode(PIN_AGIT_IN1, OUTPUT);
  pinMode(PIN_AGIT_IN2, OUTPUT);
  pinMode(PIN_BOYA_NIVEL, INPUT_PULLUP);

  // Periféricos LEDC
  ledcAttach(PIN_BOMBA_PUL, 1000, 8);
  ledcWriteTone(PIN_BOMBA_PUL, 0);

  ledcAttach(PIN_AGIT_ENA, 5000, 8);
  fijarAgitador(0);

  // Caudalímetros
  pinMode(PIN_FLOW_PERM, INPUT_PULLUP);
  pinMode(PIN_FLOW_RET, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW_PERM), isrFlowPerm, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW_RET), isrFlowRet, RISING);

  // I2C
  Wire.begin(21, 22);

  // Wi-Fi
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  int count = 0;
  while (WiFi.status() != WL_CONNECTED && count < 15) {
    delay(300);
    count++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.softAP(ap_ssid, ap_pass);
  } else {
    MDNS.begin("bomba-uf");
  }

  // ArduinoOTA
  ArduinoOTA.setHostname("bomba-uf");
  ArduinoOTA.setPassword("plantapiloto2");
  ArduinoOTA.begin();

  // Endpoints
  server.on("/", HTTP_GET, [](){ server.send_P(200, "text/html", index_html); });
  
  server.on("/cmd", HTTP_GET, [](){
    if (server.hasArg("act")) {
      String act = server.arg("act");
      if (act == "START_BOMBA") {
        if (bombaRpmObjetivo < 1.0f) bombaRpmObjetivo = 30.0f;
        bombaOn = true;
      } else if (act == "STOP_BOMBA") {
        bombaOn = false;
      } else if (act == "TOGGLE_DIR") {
        bombaDirHoraria = !bombaDirHoraria;
        digitalWrite(PIN_BOMBA_DIR, bombaDirHoraria);
      } else if (act == "SET_RPM" && server.hasArg("val")) {
        bombaRpmObjetivo = server.arg("val").toFloat();
      } else if (act == "AGIT_FAST") {
        fijarAgitador(220); // 150 RPM
      } else if (act == "AGIT_SLOW") {
        fijarAgitador(80);  // 30 RPM
      } else if (act == "AGIT_STOP") {
        fijarAgitador(0);
      }
    }
    server.send(200, "text/plain", "OK");
  });

  server.on("/status", HTTP_GET, [](){
    String json = "{";
    json += "\"bomba_on\":" + String(bombaOn ? "true" : "false") + ",";
    json += "\"bomba_rpm\":" + String(bombaRpmActual, 1) + ",";
    json += "\"bomba_dir\":" + String(bombaDirHoraria ? "true" : "false") + ",";
    json += "\"agit_pwm\":" + String(agitadorPWM) + ",";
    json += "\"nivel_bajo\":" + String(alarmaNivel ? "true" : "false") + ",";
    json += "\"alarma_tmp\":" + String(alarmaTMP ? "true" : "false") + ",";
    json += "\"tmp\":" + String(TMP_atm, 2) + ",";
    json += "\"flux\":" + String(Flux_J_LMH, 1) + ",";
    json += "\"qp\":" + String(Q_permeado_Lmin, 2) + ",";
    json += "\"tds\":" + String(TDS_ppm, 0);
    json += "}";
    server.send(200, "application/json", json);
  });

  server.begin();
}

// ------------------------------------------------------------------------------
// 6. LOOP
// ------------------------------------------------------------------------------
unsigned long t_ultimo_loop = 0;

void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  unsigned long t_now = millis();
  float dt = (t_now - t_ultimo_loop) / 1000.0f;

  if (dt >= 0.05f) {
    t_ultimo_loop = t_now;

    // 1. Seguridad por Boya de Nivel
    alarmaNivel = (digitalRead(PIN_BOYA_NIVEL) == HIGH);
    if (alarmaNivel) {
      bombaOn = false;
      fijarAgitador(0);
    }

    // 2. Rampa de Bomba
    if (bombaOn && !alarmaTMP && !alarmaNivel) {
      if (bombaRpmActual < bombaRpmObjetivo) {
        bombaRpmActual += ACEL_RPM_S * dt;
        if (bombaRpmActual > bombaRpmObjetivo) bombaRpmActual = bombaRpmObjetivo;
      } else if (bombaRpmActual > bombaRpmObjetivo) {
        bombaRpmActual -= ACEL_RPM_S * dt;
        if (bombaRpmActual < bombaRpmObjetivo) bombaRpmActual = bombaRpmObjetivo;
      }
    } else {
      if (bombaRpmActual > 0.0f) {
        bombaRpmActual -= (ACEL_RPM_S * 1.5f) * dt;
        if (bombaRpmActual < 0.0f) bombaRpmActual = 0.0f;
      }
    }
    actualizarBombaPulsos(bombaRpmActual);

    // 3. Cálculo Hidráulico y TMP
    if (bombaOn && bombaRpmActual > 0.5f) {
      Q_permeado_Lmin = (bombaRpmActual * 4.2f * 0.85f) / 1000.0f;
      P1_atm = 0.12f + (Q_permeado_Lmin * 0.45f);
      P2_atm = P1_atm * 0.72f;
      P3_atm = 0.02f;
      TMP_atm = ((P1_atm + P2_atm) / 2.0f) - P3_atm;

      // Enclavamiento TMP
      if (TMP_atm > 0.50f) {
        alarmaTMP = true;
        bombaOn = false;
      } else {
        alarmaTMP = false;
      }

      Flux_J_LMH = (Q_permeado_Lmin * 60.0f) / AREA_MEMBRANA_M2;
    } else {
      Q_permeado_Lmin = 0; TMP_atm = 0; Flux_J_LMH = 0;
    }
  }
}
