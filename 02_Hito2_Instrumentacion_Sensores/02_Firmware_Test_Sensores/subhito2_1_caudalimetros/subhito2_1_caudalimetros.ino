/* ==============================================================================
 * PROYECTO DE TESIS DE INGENIERÍA INDUSTRIAL - PLANTA DE ULTRAFILTRACIÓN FX100
 * SUBHITO 2.1: SISTEMA INTEGRADO DE CONTROL WI-FI Y CAUDALÍMETROS DE MICROFLUJO
 * ==============================================================================
 * 
 * ARQUITECTURA DEL SISTEMA:
 * 1. Control de Bomba Peristáltica MBP-2000 (NEMA 34 + Leadshine DM860):
 *    - Rampa suave de aceleración/desaceleración (40 RPM/s) en rango seguro (70-160 RPM).
 *    - Maniobra segura de inversión de sentido con parada previa a 0 RPM.
 *    - Alerta de caudal crítico si supera 0.60 L/min.
 * 2. Instrumentación de Caudalímetros de Efecto Hall YF-S401:
 *    - Caudalímetro 1 (Feed / Entrada): GPIO 14 (Borne P14), montaje vertical ascendente.
 *    - Caudalímetro 2 (Permeado): GPIO 27 (Borne P27), montaje horizontal.
 *    - Interrupciones por hardware en memoria ultrarrápida IRAM.
 *    - Cálculo en tiempo real de Frecuencia (Hz), mL/min, L/min y Volumen acumulado (L).
 * 3. Balance de Masa y Rendimiento de la Membrana Fresenius FX100:
 *    - Q_retentado (calculado) = Q_feed - Q_permeado
 *    - Tasa de Recuperación: Y (%) = (Q_permeado / Q_feed) * 100%
 *    - Contraste dinámico: Discrepancia Δ% entre bomba volumétrica y caudalímetro de entrada.
 * 4. Interfaz Gráfica Web SCADA por Wi-Fi (Modo Router Box804 + Modo AP Propio):
 *    - Conéctate al Wi-Fi: "Bomba_Peristaltica_UF" (Clave: "plantapiloto2")
 *    - Abre el navegador en: http://192.168.4.1
 * 5. Consola Serie simultánea a 115200 baudios con telemetría en vivo.
 * ============================================================================== */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// ==============================================================================
// 1. CONFIGURACIÓN DE PINES Y PARÁMETROS MECÁNICOS
// ==============================================================================
// Driver Leadshine DM860
const uint8_t PIN_PUL = 18; // Borne P18 -> DM860 PUL-
const uint8_t PIN_DIR = 19; // Borne P19 -> DM860 DIR-

// Caudalímetros de Microflujo YF-S401
const uint8_t PIN_CAUDAL_FEED = 14; // Borne P14 -> Sensor 1 (Feed / Vertical)
const uint8_t PIN_CAUDAL_PERM = 27; // Borne P27 -> Sensor 2 (Permeado / Horizontal)

// Parámetros de la Bomba Peristáltica MBP-2000
const uint16_t PULSOS_POR_REV = 1600; // DM860 a 1600 pulsos/rev
const float ML_POR_VUELTA = 4.2f;     // mL expulsados por cada giro del cabezal
const float LIMITE_CAUDAL_MAX_LMIN = 0.60f; // Límite de seguridad de la membrana FX100
const float ACELERACION_RPM_SEG = 40.0f;    // Rampa suave: 40 RPM por segundo

// Factor K del caudalímetro YF-S401 (Hz por L/min)
// Ecuación fabricante: F (Hz) = 98 * Q (L/min) -> 5880 pulsos por Litro
const float FACTOR_K_YFS401 = 98.0f;

// ==============================================================================
// 2. CREDENCIALES WI-FI Y SERVIDOR WEB
// ==============================================================================
const char* ssid_router = "Box804";
const char* pass_router = "plantapiloto2";

const char* ssid_ap     = "Bomba_Peristaltica_UF";
const char* pass_ap     = "plantapiloto2";

WebServer server(80);

// ==============================================================================
// 3. VARIABLES DE ESTADO Y CONTROL DE LA BOMBA
// ==============================================================================
bool bombaEnMarcha = false;
bool sentidoHorario = true; // true = Filtración hacia FX100, false = Retrolavado

bool invirtiendoSentido = false;
bool nuevoSentidoDeseado = true;
float rpm_guardada_inversion = 80.0f;

float rpm_objetivo = 80.0f; // Consigna operativa por defecto
float rpm_actual   = 0.0f;  // Velocidad instantánea que sigue la rampa

float caudalBomba_Lmin = 0.0f;
float volumenBomba_L   = 0.0f;

uint32_t frecuencia_hz_actual = 0;
unsigned long t_ultimo_loop_ms = 0;

// ==============================================================================
// 4. VARIABLES DE CAUDALÍMETROS E INTERRUPCIONES CON FILTRO ANTI-RUIDO
// ==============================================================================
// A caudal máximo admisible de la planta (0.60 L/min), la frecuencia real es de ~59 Hz (periodo de ~17 ms).
// Cualquier pulso que ocurra con un intervalo menor a 2000 microsegundos (equivale a >500 Hz o >5.1 L/min)
// es 100% ruido eléctrico inducido por el motor NEMA 34 o el driver DM860.
const unsigned long FILTRO_RUIDO_MIN_US = 2000; // 2 milisegundos de tiempo muerto mínimo

volatile unsigned long conteoPulsosFeed = 0;
volatile unsigned long conteoPulsosPerm = 0;
volatile unsigned long t_ultimo_pulso_feed_us = 0;
volatile unsigned long t_ultimo_pulso_perm_us = 0;

void IRAM_ATTR isrCaudalFeed() {
  unsigned long t_ahora_us = micros();
  if (t_ahora_us - t_ultimo_pulso_feed_us >= FILTRO_RUIDO_MIN_US) {
    conteoPulsosFeed++;
    t_ultimo_pulso_feed_us = t_ahora_us;
  }
}

void IRAM_ATTR isrCaudalPerm() {
  unsigned long t_ahora_us = micros();
  if (t_ahora_us - t_ultimo_pulso_perm_us >= FILTRO_RUIDO_MIN_US) {
    conteoPulsosPerm++;
    t_ultimo_pulso_perm_us = t_ahora_us;
  }
}

// Variables calculadas periódicamente (cada 1 segundo)
float freqFeed_Hz = 0.0f;
float qFeed_Lmin  = 0.0f;
float qFeed_mLmin = 0.0f;
float volFeed_L   = 0.0f;

float freqPerm_Hz = 0.0f;
float qPerm_Lmin  = 0.0f;
float qPerm_mLmin = 0.0f;
float volPerm_L   = 0.0f;

float qRet_calc_mLmin = 0.0f;
float recuperacion_porc = 0.0f;
float discrepanciaBomba_porc = 0.0f;

unsigned long t_ultimo_muestreo_caudal_ms = 0;

// ==============================================================================
// 5. CONTROL DE HARDWARE: PULSOS Y DIRECCIÓN (ÁNODO COMÚN OPEN-DRAIN)
// ==============================================================================
void fijarSentidoFisico(bool horario) {
  sentidoHorario = horario;
  // Ánodo Común en Open-Drain:
  // LOW conduce a 0V -> Optoacoplador ON
  // HIGH pone el pin en Alta Impedancia (Hi-Z) -> Optoacoplador OFF
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
// 6. INTERFAZ GRÁFICA WEB SCADA (BOMBA + CAUDALÍMETROS HITO 2)
// ==============================================================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Planta UF | Hito 2: Control y Caudal</title>
  <style>
    :root {
      --bg: #090d16; --card: #131b2e; --border: #223150;
      --text: #f8fafc; --muted: #94a3b8; --accent: #38bdf8;
      --feed: #0ea5e9; --perm: #a855f7; --ret: #f59e0b;
      --success: #10b981; --danger: #ef4444; --warning: #f59e0b;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', system-ui, sans-serif; }
    body { background: var(--bg); color: var(--text); padding: 14px; min-height: 100vh; display: flex; justify-content: center; }
    .container { width: 100%; max-width: 480px; }
    .card { background: var(--card); border: 1px solid var(--border); border-radius: 16px; padding: 18px; margin-bottom: 14px; box-shadow: 0 10px 25px rgba(0,0,0,0.5); }
    h1 { font-size: 18px; color: var(--accent); text-align: center; margin-bottom: 2px; }
    p.sub { font-size: 11px; color: var(--muted); text-align: center; margin-bottom: 14px; }
    .sec-title { font-size: 12px; font-weight: 700; color: var(--muted); text-transform: uppercase; letter-spacing: 1px; margin-bottom: 10px; display: flex; align-items: center; justify-content: space-between; }
    
    /* Display Bomba */
    .display { background: #060911; border: 1px solid var(--border); border-radius: 12px; padding: 12px; text-align: center; margin-bottom: 14px; }
    .rpm-val { font-size: 46px; font-weight: 800; line-height: 1; color: var(--accent); font-variant-numeric: tabular-nums; }
    .rpm-unit { font-size: 12px; color: var(--muted); letter-spacing: 2px; }
    .pill { display: inline-block; padding: 4px 10px; border-radius: 20px; font-size: 10px; font-weight: 700; margin-top: 8px; }
    .pill-off { background: rgba(148,163,184,0.15); color: var(--muted); border: 1px solid rgba(148,163,184,0.3); }
    .pill-on { background: rgba(16,185,129,0.2); color: var(--success); border: 1px solid var(--success); }
    .pill-invert { background: rgba(245,158,11,0.2); color: var(--warning); border: 1px solid var(--warning); }
    
    /* Alerta Caudal */
    .alerta-box { display: none; background: rgba(239,68,68,0.15); border: 1px solid var(--danger); color: #fca5a5; border-radius: 8px; padding: 8px; font-size: 11px; font-weight: 700; text-align: center; margin-bottom: 12px; }
    .alerta-box.visible { display: block; animation: pulso 1s infinite alternate; }
    @keyframes pulso { from { opacity: 0.8; } to { opacity: 1; } }

    /* Controles Bomba */
    .ctrl-group { margin-bottom: 12px; }
    .slider-lbl { display: flex; justify-content: space-between; font-size: 11px; color: var(--muted); margin-bottom: 5px; }
    input[type=range] { width: 100%; height: 6px; border-radius: 3px; background: #223150; accent-color: var(--accent); outline: none; }
    .preset-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 6px; margin-bottom: 12px; }
    .btn-preset { background: #1a243b; border: 1px solid var(--border); color: #e2e8f0; padding: 7px 2px; border-radius: 8px; font-weight: 600; font-size: 11px; cursor: pointer; }
    .action-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-bottom: 6px; }
    .btn { padding: 11px; border: none; border-radius: 10px; font-weight: 700; font-size: 12px; cursor: pointer; }
    .btn-start { background: var(--success); color: #052e16; }
    .btn-stop { background: var(--danger); color: #450a0a; }
    .btn-dir { grid-column: span 2; background: #1e293b; border: 1px solid var(--border); color: #cbd5e1; }
    .btn-reset { background: transparent; border: 1px solid var(--border); color: var(--muted); font-size: 10px; padding: 4px 8px; border-radius: 6px; cursor: pointer; }

    /* Grid Caudalímetros */
    .tele-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-bottom: 10px; }
    .tele-card { background: #0b1120; border: 1px solid var(--border); border-radius: 10px; padding: 10px; }
    .tele-head { font-size: 10px; font-weight: 700; text-transform: uppercase; margin-bottom: 4px; display: flex; justify-content: space-between; }
    .tele-val { font-size: 20px; font-weight: 800; line-height: 1.1; font-variant-numeric: tabular-nums; }
    .tele-sub { font-size: 10px; color: var(--muted); margin-top: 3px; }
    
    /* Balance Membrana */
    .balance-card { background: #070c17; border: 1px solid #1e293b; border-radius: 10px; padding: 10px; margin-top: 8px; }
    .bal-row { display: flex; justify-content: space-between; font-size: 11px; padding: 3px 0; border-bottom: 1px solid rgba(255,255,255,0.04); }
    .bal-row:last-child { border-bottom: none; }
    .bal-lbl { color: var(--muted); }
    .bal-val { font-weight: 700; font-variant-numeric: tabular-nums; }

    .footer { text-align: center; font-size: 10px; color: var(--muted); margin-top: 10px; padding-bottom: 10px; }
  </style>
</head>
<body>
  <div class="container">
    
    <!-- TARJETA 1: CONTROL BOMBA MBP-2000 -->
    <div class="card">
      <h1>BOMBA PERISTÁLTICA MBP-2000</h1>
      <p class="sub">Planta Piloto FX100 • Control Wi-Fi y Rampa Segura</p>

      <div class="display">
        <div class="rpm-val" id="rpmVal">0.0</div>
        <div class="rpm-unit">RPM INSTANTÁNEA</div>
        <div id="pillEstado" class="pill pill-off">DETENIDA</div>
      </div>

      <div id="alertaCaudal" class="alerta-box">
        ⚠️ ADVERTENCIA: Caudal próximo al límite de membrana (0.60 L/min)
      </div>

      <div class="ctrl-group">
        <div class="slider-lbl">
          <span>Consigna: <strong id="lblConsigna" style="color:var(--accent);">80</strong> RPM</span>
          <span>Rango: 70 - 160 RPM</span>
        </div>
        <input type="range" id="sliderRpm" min="70" max="160" step="1" value="80">
      </div>

      <div class="preset-grid">
        <button class="btn-preset" onclick="setRpm(80)">80 RPM</button>
        <button class="btn-preset" onclick="setRpm(100)">100 RPM</button>
        <button class="btn-preset" onclick="setRpm(120)">120 RPM</button>
        <button class="btn-preset" onclick="setRpm(140)">140 RPM</button>
      </div>

      <div class="action-grid">
        <button class="btn btn-start" onclick="enviarComando('START')">▶ ARRANCAR</button>
        <button class="btn btn-stop" onclick="enviarComando('STOP')">⏹ PARAR</button>
        <button class="btn btn-dir" id="btnDir" onclick="enviarComando('TOGGLE_DIR')">🔄 SENTIDO: HORARIO (FILTRACIÓN)</button>
      </div>
    </div>

    <!-- TARJETA 2: TELEMETRÍA CAUDALÍMETROS (SUBHITO 2.1) -->
    <div class="card">
      <div class="sec-title">
        <span>🌊 Telemetría de Caudal (YF-S401)</span>
        <button class="btn-reset" onclick="enviarComando('RESET_VOL')">Reset Litros</button>
      </div>

      <div class="tele-grid">
        <!-- Sensor Feed -->
        <div class="tele-card" style="border-left: 3px solid var(--feed);">
          <div class="tele-head" style="color:var(--feed);">
            <span>FEED (Entrada P14)</span>
            <span id="fFeed">0.0 Hz</span>
          </div>
          <div class="tele-val" id="qFeed" style="color:var(--feed);">0.0</div>
          <div class="tele-sub">mL/min (<span id="qFeedL">0.000</span> L/m)</div>
          <div class="tele-sub" style="margin-top:4px;">Vol: <strong id="volFeed">0.000</strong> L</div>
        </div>

        <!-- Sensor Permeado -->
        <div class="tele-card" style="border-left: 3px solid var(--perm);">
          <div class="tele-head" style="color:var(--perm);">
            <span>PERMEADO (P27)</span>
            <span id="fPerm">0.0 Hz</span>
          </div>
          <div class="tele-val" id="qPerm" style="color:var(--perm);">0.0</div>
          <div class="tele-sub">mL/min (<span id="qPermL">0.000</span> L/m)</div>
          <div class="tele-sub" style="margin-top:4px;">Vol: <strong id="volPerm">0.000</strong> L</div>
        </div>
      </div>

      <!-- Balance de Masa y Rendimiento -->
      <div class="balance-card">
        <div class="bal-row">
          <span class="bal-lbl">Retentado Estimado (Feed - Perm):</span>
          <span class="bal-val" id="qRet" style="color:var(--ret);">0.0 mL/min</span>
        </div>
        <div class="bal-row">
          <span class="bal-lbl">Tasa de Recuperación Membrana (Y):</span>
          <span class="bal-val" id="recov" style="color:#38bdf8;">0.0 %</span>
        </div>
        <div class="bal-row">
          <span class="bal-lbl">Caudal Teórico Bomba:</span>
          <span class="bal-val" id="qBombaTeor">0.0 mL/min</span>
        </div>
        <div class="bal-row">
          <span class="bal-lbl">Discrepancia Bomba vs Feed (Δ%):</span>
          <span class="bal-val" id="deltaBomba">0.0 %</span>
        </div>
      </div>
    </div>

    <div class="footer">
      ESP32 IP: <strong id="lblIp">...</strong> • Proyecto Ultrafiltración Industrial
    </div>
  </div>

  <script>
    const slider = document.getElementById('sliderRpm');
    const lblConsigna = document.getElementById('lblConsigna');

    slider.addEventListener('input', (e) => {
      lblConsigna.innerText = e.target.value;
    });

    slider.addEventListener('change', (e) => {
      setRpm(e.target.value);
    });

    function setRpm(val) {
      slider.value = val;
      lblConsigna.innerText = val;
      fetch('/set?rpm=' + val);
    }

    function enviarComando(act) {
      fetch('/cmd?act=' + act);
    }

    // Actualización de Telemetría cada 500 ms
    setInterval(() => {
      fetch('/status')
        .then(r => r.json())
        .then(d => {
          // 1. Datos Bomba
          document.getElementById('rpmVal').innerText = d.rpm.toFixed(1);
          document.getElementById('lblIp').innerText = d.ip;

          const pill = document.getElementById('pillEstado');
          if (d.inv) {
            pill.className = 'pill pill-invert';
            pill.innerText = 'INVIRTIENDO...';
          } else if (d.on) {
            pill.className = 'pill pill-on';
            pill.innerText = 'EN MARCHA';
          } else {
            pill.className = 'pill pill-off';
            pill.innerText = 'DETENIDA';
          }

          const btnDir = document.getElementById('btnDir');
          btnDir.innerText = d.dir ? '🔄 SENTIDO: HORARIO (FILTRACIÓN)' : '🔄 SENTIDO: ANTIHORARIO (RETROLAVADO)';

          // Alerta Caudal > 0.60 L/min
          const alerta = document.getElementById('alertaCaudal');
          if (d.q_feed_l > 0.58 || d.flow_pump > 0.58) {
            alerta.classList.add('visible');
          } else {
            alerta.classList.remove('visible');
          }

          // 2. Datos Caudalímetros (Hito 2)
          document.getElementById('fFeed').innerText = d.f_feed.toFixed(1) + ' Hz';
          document.getElementById('qFeed').innerText = d.q_feed_ml.toFixed(1);
          document.getElementById('qFeedL').innerText = d.q_feed_l.toFixed(3);
          document.getElementById('volFeed').innerText = d.vol_feed.toFixed(3);

          document.getElementById('fPerm').innerText = d.f_perm.toFixed(1) + ' Hz';
          document.getElementById('qPerm').innerText = d.q_perm_ml.toFixed(1);
          document.getElementById('qPermL').innerText = d.q_perm_l.toFixed(3);
          document.getElementById('volPerm').innerText = d.vol_perm.toFixed(3);

          // 3. Balance de Masa
          document.getElementById('qRet').innerText = d.q_ret_ml.toFixed(1) + ' mL/min';
          document.getElementById('recov').innerText = d.recov.toFixed(1) + ' %';
          document.getElementById('qBombaTeor').innerText = (d.flow_pump * 1000.0).toFixed(1) + ' mL/min';
          
          const deltaElem = document.getElementById('deltaBomba');
          deltaElem.innerText = (d.delta_bomba > 0 ? '+' : '') + d.delta_bomba.toFixed(1) + ' %';
          if (Math.abs(d.delta_bomba) < 5.0) {
            deltaElem.style.color = 'var(--success)';
          } else if (Math.abs(d.delta_bomba) < 15.0) {
            deltaElem.style.color = 'var(--warning)';
          } else {
            deltaElem.style.color = 'var(--danger)';
          }
        })
        .catch(err => console.log('Error fetch:', err));
    }, 500);
  </script>
</body>
</html>
)rawliteral";

// ==============================================================================
// 7. SETUP: INICIALIZACIÓN DE PUERTO SERIE, PINES, WI-FI Y SERVIDOR
// ==============================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n==================================================================");
  Serial.println("  PLANTA PILOTO DE ULTRAFILTRACIÓN INDUSTRIAL - SUBHITO 2.1      ");
  Serial.println("  CONTROL WI-FI SCADA + CAUDALÍMETROS DE MICROFLUJO (YF-S401)    ");
  Serial.println("==================================================================");

  // Configuración de Pines de los Caudalímetros con PULLUP interno
  pinMode(PIN_CAUDAL_FEED, INPUT_PULLUP);
  pinMode(PIN_CAUDAL_PERM, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_CAUDAL_FEED), isrCaudalFeed, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_CAUDAL_PERM), isrCaudalPerm, FALLING);
  Serial.println("[OK] Interrupciones asignadas (flanco de bajada FALLING): FEED en P14 y PERMEADO en P27.");

  // Configuración de pines del driver DM860 (Bomba NEMA 34)
  pinMode(PIN_DIR, OUTPUT_OPEN_DRAIN);
  fijarSentidoFisico(sentidoHorario);

  ledcAttach(PIN_PUL, 800, 10);
  ledcWrite(PIN_PUL, 1023); // En reposo: nivel ALTO (optoacoplador DM860 apagado)
  frecuencia_hz_actual = 0;
  Serial.println("[OK] Driver DM860 inicializado por hardware en P18 y P19.");

  // Inicialización de Pila Wi-Fi DUAL (AP y STA simultáneos)
  WiFi.persistent(false);
  WiFi.disconnect(true, true);
  delay(100);

  // Modo Dual: Red Propia AP + Cliente de Router STA
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid_ap, pass_ap);
  Serial.printf("\n[OK] Red Wi-Fi Propia activada: '%s' (Clave: '%s')\n", ssid_ap, pass_ap);
  Serial.printf(">>> Acceso Directo por AP: http://%s\n", WiFi.softAPIP().toString().c_str());

  WiFi.begin(ssid_router, pass_router);
  Serial.printf("Conectando también a router: '%s' ...", ssid_router);

  unsigned long t_inicio_wifi = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t_inicio_wifi < 6000)) {
    delay(300);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[OK] Conectado exitosamente al router!");
    Serial.printf(">>> Acceso por Router: http://%s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[AVISO] No se pudo enlazar al router Box804. El AP 'Bomba_Peristaltica_UF' sigue 100% activo.");
  }

  // Servicio mDNS (permite ingresar como http://bomba.local)
  if (MDNS.begin("bomba")) {
    Serial.println("[OK] Servicio mDNS activo: http://bomba.local");
  }

  // Rutas del Servidor Web
  server.on("/", HTTP_GET, [](){
    server.send_P(200, "text/html", index_html);
  });

  server.on("/set", HTTP_GET, [](){
    if (server.hasArg("rpm")) {
      float r = server.arg("rpm").toFloat();
      if (r >= 70.0f && r <= 160.0f) {
        rpm_objetivo = r;
      }
    }
    server.send(200, "text/plain", "OK");
  });

  server.on("/cmd", HTTP_GET, [](){
    if (server.hasArg("act")) {
      String act = server.arg("act");
      if (act == "START") {
        if (rpm_objetivo < 70.0f) rpm_objetivo = 80.0f;
        if (rpm_objetivo > 160.0f) rpm_objetivo = 160.0f;
        bombaEnMarcha = true;
        Serial.printf("[WEB] Bomba INICIADA a consigna de %.1f RPM\n", rpm_objetivo);
      } else if (act == "STOP") {
        bombaEnMarcha = false;
        invirtiendoSentido = false;
        Serial.println("[WEB] Bomba DETENIDA");
      } else if (act == "TOGGLE_DIR") {
        if (!bombaEnMarcha || rpm_actual < 5.0f) {
          fijarSentidoFisico(!sentidoHorario);
          Serial.printf("[WEB] Sentido cambiado a: %s\n", sentidoHorario ? "Horario (Filtración)" : "Antihorario (Retrolavado)");
        } else {
          invirtiendoSentido = true;
          nuevoSentidoDeseado = !sentidoHorario;
          rpm_guardada_inversion = rpm_objetivo;
          rpm_objetivo = 0.0f; // Rampa de frenado a 0 RPM
          Serial.println("[WEB] Desacelerando suavemente a 0 RPM para inversión segura...");
        }
      } else if (act == "RESET_VOL") {
        volFeed_L = 0.0f;
        volPerm_L = 0.0f;
        volumenBomba_L = 0.0f;
        Serial.println("[WEB] Contadores de volumen reseteados a CERO.");
      }
    }
    server.send(200, "text/plain", "OK");
  });

  server.on("/status", HTTP_GET, [](){
    String ipRouter = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "Buscando...";
    String ipAP = WiFi.softAPIP().toString();
    String json = "{";
    json += "\"on\":" + String(bombaEnMarcha ? "true" : "false") + ",";
    json += "\"inv\":" + String(invirtiendoSentido ? "true" : "false") + ",";
    json += "\"rpm\":" + String(rpm_actual, 1) + ",";
    json += "\"dir\":" + String(sentidoHorario ? "true" : "false") + ",";
    json += "\"flow_pump\":" + String(caudalBomba_Lmin, 3) + ",";
    json += "\"vol_pump\":" + String(volumenBomba_L, 3) + ",";
    json += "\"f_feed\":" + String(freqFeed_Hz, 1) + ",";
    json += "\"q_feed_ml\":" + String(qFeed_mLmin, 1) + ",";
    json += "\"q_feed_l\":" + String(qFeed_Lmin, 3) + ",";
    json += "\"vol_feed\":" + String(volFeed_L, 3) + ",";
    json += "\"f_perm\":" + String(freqPerm_Hz, 1) + ",";
    json += "\"q_perm_ml\":" + String(qPerm_mLmin, 1) + ",";
    json += "\"q_perm_l\":" + String(qPerm_Lmin, 3) + ",";
    json += "\"vol_perm\":" + String(volPerm_L, 3) + ",";
    json += "\"q_ret_ml\":" + String(qRet_calc_mLmin, 1) + ",";
    json += "\"recov\":" + String(recuperacion_porc, 1) + ",";
    json += "\"delta_bomba\":" + String(discrepanciaBomba_porc, 1) + ",";
    json += "\"ip\":\"" + ipRouter + "\",";
    json += "\"ip_ap\":\"" + ipAP + "\"";
    json += "}";
    server.send(200, "application/json", json);
  });

  server.begin();
  Serial.println("[OK] Servidor Web SCADA listo.");

  t_ultimo_loop_ms = millis();
  t_ultimo_muestreo_caudal_ms = millis();
}

// ==============================================================================
// 8. LOOP PRINCIPAL (RAMPA BOMBA + MUESTREO CAUDALÍMETROS + SERVIDOR WEB)
// ==============================================================================
void loop() {
  server.handleClient();

  unsigned long t_actual = millis();

  // 1. Control Dinámico de la Bomba (Cada 50 ms / 20 Hz)
  float dt_bomba = (t_actual - t_ultimo_loop_ms) / 1000.0f;
  if (dt_bomba >= 0.05f) {
    t_ultimo_loop_ms = t_actual;

    if (bombaEnMarcha) {
      if (rpm_actual < rpm_objetivo) {
        rpm_actual += ACELERACION_RPM_SEG * dt_bomba;
        if (rpm_actual > rpm_objetivo) rpm_actual = rpm_objetivo;
      } else if (rpm_actual > rpm_objetivo) {
        rpm_actual -= ACELERACION_RPM_SEG * dt_bomba;
        if (rpm_actual < rpm_objetivo) rpm_actual = rpm_objetivo;
      }

      // Si estábamos invirtiendo y llegamos a 0 RPM
      if (invirtiendoSentido && rpm_actual <= 0.1f) {
        fijarSentidoFisico(nuevoSentidoDeseado);
        invirtiendoSentido = false;
        rpm_objetivo = rpm_guardada_inversion; // Volver a acelerar al sentido opuesto
        Serial.printf("[SISTEMA] Sentido invertido a: %s. Acelerando...\n",
                      sentidoHorario ? "Horario" : "Antihorario");
      }
    } else {
      // Desaceleración suave al frenar
      if (rpm_actual > 0.0f) {
        rpm_actual -= ACELERACION_RPM_SEG * dt_bomba;
        if (rpm_actual < 0.0f) rpm_actual = 0.0f;
      }
    }

    // Actualizar pulsos de hardware DM860
    actualizarPulsosMotor(rpm_actual, (bombaEnMarcha || rpm_actual > 0.0f));

    // Caudal teórico de la bomba peristáltica
    caudalBomba_Lmin = (rpm_actual * ML_POR_VUELTA) / 1000.0f;
    volumenBomba_L  += (caudalBomba_Lmin / 60.0f) * dt_bomba;
  }

  // 2. Muestreo de Caudalímetros YF-S401 (Cada 1000 ms / 1 Hz)
  if (t_actual - t_ultimo_muestreo_caudal_ms >= 1000) {
    float dt_caudal = (t_actual - t_ultimo_muestreo_caudal_ms) / 1000.0f;
    t_ultimo_muestreo_caudal_ms = t_actual;

    // Sección Crítica: Capturar pulsos y reiniciar acumulador atómicamente
    noInterrupts();
    unsigned long pFeed = conteoPulsosFeed;
    unsigned long pPerm = conteoPulsosPerm;
    conteoPulsosFeed = 0;
    conteoPulsosPerm = 0;
    interrupts();

    // Frecuencias en Hertz
    freqFeed_Hz = (float)pFeed / dt_caudal;
    freqPerm_Hz = (float)pPerm / dt_caudal;

    // Caudales instantáneos
    // Factor K YF-S401: Q (L/min) = F (Hz) / 98.0
    qFeed_Lmin  = freqFeed_Hz / FACTOR_K_YFS401;
    qFeed_mLmin = qFeed_Lmin * 1000.0f;

    qPerm_Lmin  = freqPerm_Hz / FACTOR_K_YFS401;
    qPerm_mLmin = qPerm_Lmin * 1000.0f;

    // Integración de volumen en Litros
    volFeed_L += (qFeed_Lmin / 60.0f) * dt_caudal;
    volPerm_L += (qPerm_Lmin / 60.0f) * dt_caudal;

    // Balance de Masa
    qRet_calc_mLmin = (qFeed_mLmin >= qPerm_mLmin) ? (qFeed_mLmin - qPerm_mLmin) : 0.0f;
    recuperacion_porc = (qFeed_mLmin > 20.0f) ? ((qPerm_mLmin / qFeed_mLmin) * 100.0f) : 0.0f;

    // Discrepancia con Bomba
    float qBomba_mLmin = caudalBomba_Lmin * 1000.0f;
    if (qBomba_mLmin > 20.0f) {
      discrepanciaBomba_porc = ((qFeed_mLmin - qBomba_mLmin) / qBomba_mLmin) * 100.0f;
    } else {
      discrepanciaBomba_porc = 0.0f;
    }

    // Telemetría periódica por Monitor Serie
    Serial.printf("[t: %4lus] FEED: %5.1f Hz | %5.1f mL/m | PERM: %5.1f mL/m | RET(calc): %5.1f mL/m | Y: %4.1f%% | Bomba: %4.0f RPM (Teór: %5.1f mL/m | Δ: %+5.1f%%)\n",
                  t_actual / 1000,
                  freqFeed_Hz, qFeed_mLmin,
                  qPerm_mLmin,
                  qRet_calc_mLmin,
                  recuperacion_porc,
                  rpm_actual, qBomba_mLmin, discrepanciaBomba_porc);
  }
}
