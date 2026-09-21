/* ==============================================================================
 * PROYECTO DE TESIS DE INGENIERÍA INDUSTRIAL - PLANTA DE ULTRAFILTRACIÓN FX100
 * HITO 1: ACCIONAMIENTO Y CONTROL DE BOMBA PERISTÁLTICA MBP-2000
 * MOTOR PASO A PASO NEMA 34 (4.5 Nm) + DRIVER LEADSHINE DM860
 * ==============================================================================
 * CONEXIÓN FÍSICA PROBADA EN BANCO (ÁNODO COMÚN A 5V):
 * - Shield Borne [ VIN ] (5V) ──► Borne [ PUL+ ] y [ DIR+ ] del DM860 (Puenteados)
 * - Shield Borne [ P18 ]      ──► Borne [ PUL- ] del DM860 (Pulsos de paso)
 * - Shield Borne [ P19 ]      ──► Borne [ DIR- ] del DM860 (Sentido de giro)
 * - Bornes [ ENA+ / ENA- ]    ──► VACÍOS (Driver habilitado permanentemente)
 * ============================================================================== */

#include <WiFi.h>
#include <WebServer.h>

// ==============================================================================
// 1. CONFIGURACIÓN DE PINES Y PARÁMETROS MECÁNICOS
// ==============================================================================
const uint8_t PIN_PUL = 18; // Borne P18 -> Señal de pulsos (STEP)
const uint8_t PIN_DIR = 19; // Borne P19 -> Señal de dirección (DIR)

// DM860 configurado en 1600 pulsos por revolución
const uint16_t PULSOS_POR_REV = 1600; 

// Calibración volumétrica del cabezal peristáltico MBP-2000 (4.2 mL por vuelta)
const float ML_POR_VUELTA = 4.2f;

// Límite máximo admisible de caudal por diseño de la membrana de ultrafiltración FX100
const float LIMITE_CAUDAL_MAX_LMIN = 0.60f;

// Rampa de aceleración: 40 RPM por segundo para transiciones suaves y dinámicas
const float ACELERACION_RPM_SEG = 40.0f;

// ==============================================================================
// 2. CREDENCIALES WI-FI
// ==============================================================================
const char* ssid_router = "Box804";
const char* pass_router = "plantapiloto2";

const char* ssid_ap     = "Bomba_Peristaltica_UF";
const char* pass_ap     = "plantapiloto2";

WebServer server(80);

// ==============================================================================
// 3. VARIABLES DE ESTADO Y CONTROL
// ==============================================================================
bool bombaEnMarcha = false;
bool sentidoHorario = true; // true = Filtración hacia FX100, false = Retrolavado

// Maniobra segura de inversión de sentido
bool invirtiendoSentido = false;
bool nuevoSentidoDeseado = true;
float rpm_guardada_inversion = 80.0f;

float rpm_objetivo = 80.0f; // Rango operativo óptimo por defecto: 80 RPM
float rpm_actual   = 0.0f;  // Velocidad instantánea que sigue la rampa

float caudal_Lmin = 0.0f;   // Caudal instantáneo en Litros/minuto
float volumen_L   = 0.0f;   // Volumen total acumulado en Litros

uint32_t frecuencia_hz_actual = 0;
unsigned long t_ultimo_loop_ms = 0;

// ==============================================================================
// 4. CONTROL DE HARDWARE: PULSOS Y DIRECCIÓN (ÁNODO COMÚN OPEN-DRAIN)
// ==============================================================================
void fijarSentidoFisico(bool horario) {
  sentidoHorario = horario;
  // En Ánodo Común con Open-Drain:
  // - LOW conecta a 0V -> circula corriente -> Optoacoplador ON
  // - HIGH pone el pin en Alta Impedancia (Hi-Z) -> 0 corriente -> Optoacoplador OFF
  digitalWrite(PIN_DIR, sentidoHorario ? LOW : HIGH);
}

void actualizarPulsosMotor(float rpm, bool marcha) {
  if (marcha && rpm >= 5.0f) {
    uint32_t f = (uint32_t)((rpm * (float)PULSOS_POR_REV) / 60.0f);
    if (f != frecuencia_hz_actual) {
      ledcChangeFrequency(PIN_PUL, f, 10);
      ledcWrite(PIN_PUL, 512); // 50% ciclo de trabajo simétrico
      frecuencia_hz_actual = f;
    }
  } else {
    if (frecuencia_hz_actual != 0) {
      ledcWrite(PIN_PUL, 1023); // Pin en HIGH continuo -> Optoacoplador apagado
      frecuencia_hz_actual = 0;
    }
  }
}

// ==============================================================================
// 5. INTERFAZ GRÁFICA WEB SCADA (CON ALERTA VISUAL DE 0.6 L/MIN)
// ==============================================================================
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
    .card { background: var(--card); border: 1px solid var(--border); border-radius: 16px; padding: 22px; width: 100%; max-width: 450px; box-shadow: 0 12px 30px rgba(0,0,0,0.6); }
    h1 { font-size: 19px; color: var(--accent); text-align: center; margin-bottom: 2px; }
    p.sub { font-size: 11px; color: var(--muted); text-align: center; margin-bottom: 16px; }
    .display { background: #060911; border: 1px solid var(--border); border-radius: 12px; padding: 14px; text-align: center; margin-bottom: 16px; }
    .rpm-val { font-size: 52px; font-weight: 800; line-height: 1; color: var(--accent); font-variant-numeric: tabular-nums; }
    .rpm-unit { font-size: 14px; color: var(--muted); letter-spacing: 2px; }
    .grid-info { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-top: 12px; }
    .info-box { background: rgba(255,255,255,0.03); border: 1px solid rgba(255,255,255,0.05); border-radius: 8px; padding: 8px; font-size: 11px; }
    .info-lbl { color: var(--muted); display: block; margin-bottom: 2px; }
    .info-num { font-size: 15px; font-weight: 700; color: #fff; }
    .pill { display: inline-block; padding: 5px 12px; border-radius: 20px; font-size: 11px; font-weight: 700; margin-top: 10px; }
    .pill-off { background: rgba(148,163,184,0.15); color: var(--muted); border: 1px solid rgba(148,163,184,0.3); }
    .pill-on { background: rgba(16,185,129,0.2); color: var(--success); border: 1px solid var(--success); }
    .pill-invert { background: rgba(245,158,11,0.2); color: var(--warning); border: 1px solid var(--warning); }
    
    /* Alerta de Caudal Crítico */
    .alerta-box { display: none; background: rgba(239,68,68,0.15); border: 1px solid var(--danger); color: #fca5a5; border-radius: 8px; padding: 10px; font-size: 12px; font-weight: 700; text-align: center; margin-bottom: 14px; }
    .alerta-box.visible { display: block; animation: pulso 1s infinite alternate; }
    @keyframes pulso { from { opacity: 0.8; } to { opacity: 1; } }

    .ctrl-group { margin-bottom: 16px; }
    .slider-lbl { display: flex; justify-content: space-between; font-size: 12px; color: var(--muted); margin-bottom: 6px; }
    input[type=range] { width: 100%; height: 6px; border-radius: 3px; background: #223150; accent-color: var(--accent); outline: none; }
    .preset-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 6px; margin-bottom: 16px; }
    .btn-preset { background: #1a243b; border: 1px solid var(--border); color: #e2e8f0; padding: 8px 4px; border-radius: 8px; font-weight: 600; font-size: 12px; cursor: pointer; }
    .action-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-bottom: 8px; }
    .btn { padding: 12px; border: none; border-radius: 10px; font-weight: 700; font-size: 13px; cursor: pointer; }
    .btn-start { background: var(--success); color: #052e16; }
    .btn-stop { background: var(--danger); color: #450a0a; }
    .btn-dir { grid-column: span 2; background: #1e293b; border: 1px solid var(--border); color: #cbd5e1; }
    .footer { text-align: center; font-size: 10px; color: var(--muted); margin-top: 14px; border-top: 1px solid rgba(255,255,255,0.06); padding-top: 10px; }
  </style>
</head>
<body>
  <div class="card">
    <h1>PLANTA ULTRAFILTRACIÓN</h1>
    <p class="sub">Hito 1: Control Bomba Peristáltica MBP-2000</p>
    
    <div id="box_alerta" class="alerta-box">
      ⚠️ ¡PRECAUCIÓN: CAUDAL > 0.60 L/min! SUPERA LÍMITE DE LA MEMBRANA FX100
    </div>

    <div class="display">
      <div class="rpm-val" id="disp_rpm">0.0</div>
      <div class="rpm-unit">REVOLUCIONES POR MINUTO</div>
      
      <div class="grid-info">
        <div class="info-box">
          <span class="info-lbl">CAUDAL ESTIMADO</span>
          <span class="info-num" id="disp_flow">0.000 L/min</span>
        </div>
        <div class="info-box">
          <span class="info-lbl">VOLUMEN TOTAL</span>
          <span class="info-num" id="disp_vol">0.00 L</span>
        </div>
      </div>
      
      <div class="pill pill-off" id="disp_pill">MOTOR DETENIDO (REPOSO FRÍO)</div>
      <div style="font-size: 11px; color: var(--muted); margin-top: 6px;" id="disp_dir">Giro: Horario (Filtración FX100)</div>
    </div>

    <div class="ctrl-group">
      <div class="slider-lbl">
        <span>Consigna de Velocidad:</span>
        <strong id="txt_slider" style="color: var(--accent);">80 RPM</strong>
      </div>
      <input type="range" id="slider" min="60" max="200" value="80" oninput="moverSlider(this.value)">
    </div>

    <div class="preset-grid">
      <button class="btn-preset" onclick="fijarRPM(60)">60 RPM</button>
      <button class="btn-preset" onclick="fijarRPM(80)">80 RPM</button>
      <button class="btn-preset" onclick="fijarRPM(100)">100 RPM</button>
      <button class="btn-preset" onclick="fijarRPM(140)">140 RPM</button>
    </div>

    <div class="action-grid">
      <button class="btn btn-start" onclick="enviarAccion('START')">▶ INICIAR</button>
      <button class="btn btn-stop" onclick="enviarAccion('STOP')">⏹ DETENER</button>
      <button class="btn btn-dir" onclick="enviarAccion('TOGGLE_DIR')">🔄 INVERTIR SENTIDO</button>
    </div>

    <div class="footer">
      ESP32: P18 (PUL-), P19 (DIR-), VIN (5V) • DM860 (1600 P/R)<br>
      IP Conexión: <span id="disp_ip">...</span>
    </div>
  </div>

  <script>
    function moverSlider(v) {
      document.getElementById('txt_slider').innerText = v + " RPM";
      fetch('/set?rpm=' + v);
    }
    function fijarRPM(v) {
      document.getElementById('slider').value = v;
      document.getElementById('txt_slider').innerText = v + " RPM";
      fetch('/set?rpm=' + v);
    }
    function enviarAccion(act) {
      fetch('/cmd?act=' + act);
    }

    let solicitando = false;
    setInterval(() => {
      if (solicitando) return;
      solicitando = true;
      fetch('/status')
        .then(r => r.json())
        .then(d => {
          document.getElementById('disp_rpm').innerText = d.rpm.toFixed(1);
          document.getElementById('disp_flow').innerText = d.flow.toFixed(3) + " L/min";
          document.getElementById('disp_vol').innerText = d.vol.toFixed(2) + " L";
          document.getElementById('disp_ip').innerText = d.ip;
          
          let alerta = document.getElementById('box_alerta');
          let dispFlow = document.getElementById('disp_flow');
          if (d.flow > 0.60) {
            alerta.className = "alerta-box visible";
            dispFlow.style.color = "var(--danger)";
          } else if (d.flow > 0.55) {
            alerta.className = "alerta-box visible";
            alerta.innerText = "⚠️ PRECAUCIÓN: PRÓXIMO AL LÍMITE DEL FILTRO FX100 (0.60 L/min)";
            dispFlow.style.color = "var(--warning)";
          } else {
            alerta.className = "alerta-box";
            dispFlow.style.color = "#fff";
          }

          let p = document.getElementById('disp_pill');
          if (d.inv) {
            p.className = "pill pill-invert";
            p.innerText = "INVIRTIENDO GIRO (RAMPA SEGURA)...";
          } else if (d.on) {
            p.className = "pill pill-on";
            p.innerText = "BOMBA EN MARCHA";
          } else {
            p.className = "pill pill-off";
            p.innerText = "MOTOR DETENIDO (REPOSO FRÍO)";
          }
          document.getElementById('disp_dir').innerText = d.dir ? "Giro: Horario (Filtración FX100)" : "Giro: Antihorario (Retrolavado)";
        })
        .catch(e => {})
        .finally(() => { solicitando = false; });
    }, 800);
  </script>
</body>
</html>
)rawliteral";

// ==============================================================================
// 6. SETUP: INICIALIZACIÓN DE HARDWARE Y RED
// ==============================================================================
void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println("\n========================================================");
  Serial.println("  PLANTA PILOTO DE ULTRAFILTRACIÓN INDUSTRIAL FX100     ");
  Serial.println("  HITO 1: CONTROL DEFINITIVO DE BOMBA PERISTÁLTICA      ");
  Serial.println("========================================================");

  // Configuración de pin de dirección en modo Open-Drain (drenador abierto)
  pinMode(PIN_DIR, OUTPUT_OPEN_DRAIN);
  fijarSentidoFisico(sentidoHorario);

  // Inicialización de LEDC por hardware en P18 (10 bits de resolución)
  ledcAttach(PIN_PUL, 800, 10);
  ledcWrite(PIN_PUL, 1023); // Nivel ALTO -> Optoacoplador apagado en reposo
  frecuencia_hz_actual = 0;

  // Limpieza previa del stack Wi-Fi para evitar cuelgues de NVS
  WiFi.persistent(false);
  WiFi.disconnect(true, true);
  delay(100);

  // Conexión Wi-Fi limpia (Modo Cliente STA primero, fallback a AP Propio)
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_router, pass_router);
  Serial.printf("Intentando conectar a Wi-Fi: '%s' ...", ssid_router);

  unsigned long t_inicio_wifi = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t_inicio_wifi < 7000)) {
    delay(300);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[OK] Conectado exitosamente al router.");
    Serial.printf("Panel Web disponible en: http://%s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[AVISO] No se encontró el router. Creando Red Wi-Fi Propia (AP)...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid_ap, pass_ap);
    Serial.printf("Conéctate al Wi-Fi: '%s' (Clave: '%s')\n", ssid_ap, pass_ap);
    Serial.printf("Panel Web disponible en: http://%s\n", WiFi.softAPIP().toString().c_str());
  }

  // Rutas del Servidor Web
  server.on("/", HTTP_GET, [](){
    server.send_P(200, "text/html", index_html);
  });

  server.on("/set", HTTP_GET, [](){
    if (server.hasArg("rpm")) {
      float r = server.arg("rpm").toFloat();
      if (r >= 50.0f && r <= 220.0f) {
        rpm_objetivo = r;
      }
    }
    server.send(200, "text/plain", "OK");
  });

  server.on("/cmd", HTTP_GET, [](){
    if (server.hasArg("act")) {
      String act = server.arg("act");
      if (act == "START") {
        if (rpm_objetivo < 60.0f) rpm_objetivo = 80.0f;
        bombaEnMarcha = true;
        Serial.printf("[WEB] Bomba INICIADA a consigna de %.1f RPM\n", rpm_objetivo);
      } else if (act == "STOP") {
        bombaEnMarcha = false;
        invirtiendoSentido = false;
        Serial.println("[WEB] Bomba DETENIDA");
      } else if (act == "TOGGLE_DIR") {
        if (!bombaEnMarcha || rpm_actual < 5.0f) {
          // Si está detenida, cambia inmediatamente
          fijarSentidoFisico(!sentidoHorario);
          Serial.printf("[WEB] Sentido cambiado a: %s\n", sentidoHorario ? "Horario (Filtración)" : "Antihorario (Retrolavado)");
        } else {
          // Maniobra suave: desacelerar a 0, invertir y volver a acelerar
          invirtiendoSentido = true;
          nuevoSentidoDeseado = !sentidoHorario;
          rpm_guardada_inversion = rpm_objetivo;
          rpm_objetivo = 0.0f; // Rampa de frenado
          Serial.println("[WEB] Desacelerando suavemente a 0 RPM para inversión segura...");
        }
      }
    }
    server.send(200, "text/plain", "OK");
  });

  server.on("/status", HTTP_GET, [](){
    String ipActual = (WiFi.getMode() == WIFI_STA) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    String json = "{";
    json += "\"on\":" + String(bombaEnMarcha ? "true" : "false") + ",";
    json += "\"inv\":" + String(invirtiendoSentido ? "true" : "false") + ",";
    json += "\"rpm\":" + String(rpm_actual, 1) + ",";
    json += "\"dir\":" + String(sentidoHorario ? "true" : "false") + ",";
    json += "\"flow\":" + String(caudal_Lmin, 3) + ",";
    json += "\"vol\":" + String(volumen_L, 2) + ",";
    json += "\"ip\":\"" + ipActual + "\"";
    json += "}";
    server.send(200, "application/json", json);
  });

  server.begin();
  Serial.println("[OK] Servidor Web listo.");
}

// ==============================================================================
// 7. LOOP PRINCIPAL (RAMPA SUAVE, MANIOBRA DE INVERSIÓN Y SERVIDOR WEB)
// ==============================================================================
void loop() {
  server.handleClient();

  unsigned long t_actual = millis();
  float dt = (t_actual - t_ultimo_loop_ms) / 1000.0f;

  if (dt >= 0.05f) { // Actualización cada 50 ms (20 Hz)
    t_ultimo_loop_ms = t_actual;

    // Rampa suave de aceleración y desaceleración
    if (bombaEnMarcha) {
      if (rpm_actual < rpm_objetivo) {
        rpm_actual += ACELERACION_RPM_SEG * dt;
        if (rpm_actual > rpm_objetivo) rpm_actual = rpm_objetivo;
      } else if (rpm_actual > rpm_objetivo) {
        rpm_actual -= ACELERACION_RPM_SEG * dt;
        if (rpm_actual < rpm_objetivo) rpm_actual = rpm_objetivo;
      }

      // Si estábamos en maniobra de inversión y llegamos a 0 RPM:
      if (invirtiendoSentido && rpm_actual <= 0.1f) {
        fijarSentidoFisico(nuevoSentidoDeseado);
        invirtiendoSentido = false;
        rpm_objetivo = rpm_guardada_inversion; // Volver a acelerar en el nuevo sentido
        Serial.printf("[SISTEMA] Inversión completada. Acelerando en sentido: %s\n", sentidoHorario ? "Horario" : "Antihorario");
      }
    } else {
      if (rpm_actual > 0.0f) {
        rpm_actual -= (ACELERACION_RPM_SEG * 1.5f) * dt;
        if (rpm_actual < 0.0f) rpm_actual = 0.0f;
      }
    }

    // Actualizar pulsos de silicio por hardware
    actualizarPulsosMotor(rpm_actual, bombaEnMarcha);

    // Integración de Caudal y Volumen
    caudal_Lmin = (rpm_actual * ML_POR_VUELTA) / 1000.0f;
    if (bombaEnMarcha && rpm_actual > 0.1f) {
      volumen_L += (caudal_Lmin / 60.0f) * dt;
    }
  }

  // Parser por Monitor Serie (para control de banco directo)
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.equalsIgnoreCase("START")) {
      bombaEnMarcha = true;
      Serial.println("[SERIE] Bomba Iniciada");
    } else if (cmd.equalsIgnoreCase("STOP")) {
      bombaEnMarcha = false;
      invirtiendoSentido = false;
      Serial.println("[SERIE] Bomba Detenida");
    } else if (cmd.equalsIgnoreCase("DIR")) {
      if (!bombaEnMarcha || rpm_actual < 5.0f) {
        fijarSentidoFisico(!sentidoHorario);
      } else {
        invirtiendoSentido = true;
        nuevoSentidoDeseado = !sentidoHorario;
        rpm_guardada_inversion = rpm_objetivo;
        rpm_objetivo = 0.0f;
      }
    } else if (cmd.startsWith("R") || cmd.startsWith("r")) {
      float r = cmd.substring(1).toFloat();
      if (r >= 50.0f && r <= 220.0f) {
        rpm_objetivo = r;
        Serial.printf("[SERIE] Consigna RPM: %.1f\n", rpm_objetivo);
      }
    }
  }
}
