#include "index_html.h"

// ==============================================================================
// DEFINICIÓN DEL CONTENIDO HTML / CSS / JS (PROGMEM) — TESIS UNSa 2026
// Panel SCADA HMI v6.6:
// 1. Pestaña Operador (Panel General Mobile-First que encaja en pantalla de celular):
//    - Caudalímetros Alimentación y Permeado en 2 columnas compactas.
//    - Cronómetro de ensayo, Temperatura y PTM en tiempo real.
//    - Dial circular táctil (Knob) interactivo con contador digital de RPM arriba.
//    - Botonera reactiva de alto contraste con feedback cromático.
// 2. Pestaña Administrador:
//    - Calibración de Factores K y Cilindrada con asistente de probeta.
//    - Configuración de RPM de inicio y Modo de parada (Rampa suave vs Seca).
//    - Dimer de fluidez de arranque (0% a 100%) con ajuste instantáneo valor a valor.
//    - Asignación de límites de caudal, temperatura y PTM con parada automática.
//    - Gestión de ensayos y descarga a Excel/CSV con marcas de tiempo.
//    - Guía de switches del driver Leadshine DM860.
// 3. Pestaña Gráficos en el Tiempo:
//    - Gráficas en tiempo real de Caudales, Presión Transmembrana y Temperatura.
// ==============================================================================

const char INDEX_HTML[] PROGMEM = R"html(<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Planta UF FX100 — SCADA HMI</title>
  <style>
    :root {
      --bg: #070b16;
      --card: #0f172a;
      --card-alt: #131d35;
      --border: #1e293b;
      --border-focus: #38bdf8;
      --text: #f8fafc;
      --muted: #94a3b8;
      --primary: #38bdf8;
      --alim: #0284c7;
      --perm: #10b981;
      --ret: #f59e0b;
      --danger: #ef4444;
      --success: #16a34a;
      --excel: #107c41;
      --radius: 10px;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; user-select: none; -webkit-tap-highlight-color: transparent; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      background: var(--bg);
      color: var(--text);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      overflow-x: hidden;
    }
    
    /* NAV TABS */
    .nav-bar {
      position: sticky;
      top: 0;
      z-index: 100;
      display: flex;
      background: #090e1c;
      border-bottom: 1px solid var(--border);
      padding: 6px 10px;
      gap: 6px;
    }
    .tab-btn {
      flex: 1;
      padding: 8px 4px;
      font-size: 13px;
      font-weight: 700;
      border-radius: 8px;
      border: 1px solid transparent;
      background: #111827;
      color: var(--muted);
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 5px;
      transition: all 0.2s;
    }
    .tab-btn.active {
      background: #1e293b;
      color: #fff;
      border-color: var(--primary);
      box-shadow: 0 0 12px rgba(56, 189, 248, 0.3);
    }

    /* CONTAINERS */
    .view { display: none; padding: 8px 10px 20px; max-width: 520px; width: 100%; margin: 0 auto; }
    .view.active { display: block; }
    
    /* TOAST ALERT */
    #toast {
      position: fixed;
      top: 50px;
      left: 50%;
      transform: translateX(-50%);
      background: #1e1b4b;
      color: #fff;
      padding: 8px 16px;
      border-radius: 8px;
      border: 1px solid #818cf8;
      box-shadow: 0 4px 20px rgba(0,0,0,0.6);
      font-size: 12px;
      font-weight: 700;
      display: none;
      z-index: 200;
      text-align: center;
      max-width: 90%;
    }

    /* COMMON CARD STYLES */
    .card {
      background: var(--card);
      border: 1px solid var(--border);
      border-radius: var(--radius);
      padding: 10px;
      margin-bottom: 8px;
    }
    .card-title {
      font-size: 11px;
      font-weight: 800;
      text-transform: uppercase;
      letter-spacing: 0.5px;
      color: var(--muted);
      margin-bottom: 6px;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }

    /* HEADER & STATUS */
    .header-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      margin-bottom: 8px;
    }
    .brand { font-size: 14px; font-weight: 900; color: #fff; display: flex; align-items: center; gap: 5px; }
    .brand span { color: var(--primary); }
    .badge {
      padding: 3px 8px;
      border-radius: 12px;
      font-size: 10px;
      font-weight: 800;
      letter-spacing: 0.5px;
    }
    .badge-off { background: #334155; color: #cbd5e1; }
    .badge-on { background: #166534; color: #4ade80; box-shadow: 0 0 8px rgba(74, 222, 128, 0.4); }
    .badge-alert { background: #7f1d1d; color: #f87171; animation: pulseAlert 1.2s infinite; }
    @keyframes pulseAlert { 0%, 100% { opacity: 1; } 50% { opacity: 0.5; } }

    /* TELEMETRIA FLOW (2 COLS) */
    .flow-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 6px;
      margin-bottom: 8px;
    }
    .flow-box {
      background: var(--card-alt);
      border: 1px solid var(--border);
      border-radius: var(--radius);
      padding: 8px;
      text-align: center;
    }
    .flow-lbl { font-size: 10px; font-weight: 800; text-transform: uppercase; color: var(--muted); }
    .flow-val { font-size: 24px; font-weight: 900; line-height: 1.1; margin: 2px 0; }
    .flow-sub { font-size: 10px; color: var(--muted); }

    /* TELEMETRIA BAR (3 COLS) */
    .tele-bar {
      display: flex;
      justify-content: space-around;
      background: #090e1a;
      border: 1px solid var(--border);
      border-radius: 8px;
      padding: 6px 4px;
      margin-bottom: 8px;
      font-size: 11px;
    }
    .tele-item { text-align: center; }
    .tele-item span { color: var(--muted); font-size: 10px; display: block; }
    .tele-item b { font-size: 12px; color: #fff; }

    /* RPM HERO & DIAL */
    .rpm-container {
      background: var(--card);
      border: 1px solid var(--border);
      border-radius: var(--radius);
      padding: 8px 10px 10px;
      text-align: center;
      margin-bottom: 8px;
    }
    .rpm-display-box {
      margin-bottom: 4px;
    }
    .rpm-big {
      font-size: 34px;
      font-weight: 900;
      color: #fff;
      line-height: 1;
      letter-spacing: -1px;
    }
    .rpm-big small { font-size: 14px; color: var(--primary); font-weight: 700; margin-left: 2px; }
    .rpm-sub { font-size: 11px; color: var(--muted); margin-top: 2px; }

    /* SVG DIAL KNOB */
    .dial-wrap {
      position: relative;
      width: 170px;
      height: 155px;
      margin: 0 auto;
      touch-action: none;
      cursor: grab;
    }
    .dial-wrap:active { cursor: grabbing; }
    .dial-svg { width: 100%; height: 100%; overflow: visible; }
    .dial-track { fill: none; stroke: #1e293b; stroke-width: 12; stroke-linecap: round; }
    .dial-prog { fill: none; stroke: url(#dialGrad); stroke-width: 12; stroke-linecap: round; transition: stroke-dashoffset 0.05s linear; }
    .dial-center-btn {
      position: absolute;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -45%);
      display: flex;
      align-items: center;
      gap: 12px;
    }
    .step-btn {
      width: 32px;
      height: 32px;
      border-radius: 50%;
      background: #1e293b;
      border: 1px solid #334155;
      color: #fff;
      font-size: 16px;
      font-weight: 900;
      display: flex;
      align-items: center;
      justify-content: center;
      cursor: pointer;
      transition: all 0.15s;
    }
    .step-btn:active { background: var(--primary); transform: scale(0.92); }

    /* RPM PRESETS ROW */
    .presets-row {
      display: flex;
      justify-content: space-between;
      gap: 4px;
      margin: 8px 0;
    }
    .preset-pill {
      flex: 1;
      padding: 6px 2px;
      font-size: 11px;
      font-weight: 800;
      border-radius: 6px;
      border: 1px solid #334155;
      background: #162032;
      color: var(--muted);
      cursor: pointer;
      transition: all 0.15s;
    }
    .preset-pill.active {
      background: #2563eb;
      color: #fff;
      border-color: #60a5fa;
      box-shadow: 0 0 10px rgba(37, 99, 235, 0.6);
      font-weight: 900;
    }

    /* ACTION BUTTONS (ARRANCAR / PARAR / DIR) */
    .action-row {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 6px;
      margin-bottom: 6px;
    }
    .btn-act {
      padding: 12px 6px;
      border-radius: 8px;
      font-size: 13px;
      font-weight: 900;
      letter-spacing: 0.5px;
      border: none;
      cursor: pointer;
      transition: all 0.15s;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 6px;
    }
    .btn-act:active { transform: scale(0.97); }
    .btn-start { background: #166534; color: #bbf7d0; border: 1px solid #22c55e; }
    .btn-start.running { background: #16a34a; color: #fff; box-shadow: 0 0 15px rgba(22, 163, 74, 0.7); }
    .btn-stop { background: #7f1d1d; color: #fecaca; border: 1px solid #ef4444; }
    .btn-stop.stopped { background: #dc2626; color: #fff; box-shadow: 0 0 15px rgba(220, 38, 38, 0.7); }
    .btn-dir {
      width: 100%;
      padding: 9px;
      font-size: 12px;
      font-weight: 800;
      border-radius: 8px;
      border: 1px solid #0284c7;
      background: #032b43;
      color: #38bdf8;
      cursor: pointer;
      margin-bottom: 6px;
    }
    .btn-dir.inv { border-color: #d97706; background: #3d2302; color: #fbbf24; }

    /* ADMIN FORMS & INPUTS */
    .form-group {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 6px 0;
      border-bottom: 1px solid #172238;
      font-size: 12px;
    }
    .form-group:last-child { border-bottom: none; }
    .form-group label { color: #cbd5e1; flex: 1; }
    .form-group input[type="number"], .form-group select {
      background: #090e1a;
      border: 1px solid #334155;
      border-radius: 6px;
      color: #fff;
      padding: 5px 8px;
      font-size: 12px;
      font-weight: 700;
      text-align: right;
      width: 90px;
    }
    .form-group input[type="range"] {
      flex: 1;
      accent-color: var(--primary);
      cursor: pointer;
      margin: 0 8px;
    }
    .btn-save {
      width: 100%;
      padding: 10px;
      font-size: 12px;
      font-weight: 800;
      border-radius: 8px;
      border: none;
      background: #2563eb;
      color: #fff;
      cursor: pointer;
      margin-top: 8px;
      box-shadow: 0 0 10px rgba(37, 99, 235, 0.4);
    }
    .btn-excel {
      background: var(--excel);
      color: #fff;
      border: none;
      padding: 10px;
      border-radius: 8px;
      font-size: 12px;
      font-weight: 800;
      cursor: pointer;
      width: 100%;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 6px;
      margin-top: 6px;
    }

    /* SEGMENTED TOGGLE BUTTONS */
    .seg-group {
      display: flex;
      border: 1px solid #334155;
      border-radius: 6px;
      overflow: hidden;
    }
    .seg-btn {
      padding: 5px 10px;
      font-size: 11px;
      font-weight: 700;
      background: #090e1a;
      color: var(--muted);
      border: none;
      cursor: pointer;
    }
    .seg-btn.active { background: #2563eb; color: #fff; }

    /* CANVAS CHARTS */
    .chart-box {
      background: #040711;
      border: 1px solid var(--border);
      border-radius: 8px;
      padding: 6px;
      margin-bottom: 8px;
    }
    canvas { width: 100%; height: 160px; display: block; }
    
    /* TABLE ENSAYOS */
    .table-box {
      max-height: 200px;
      overflow-y: auto;
      border: 1px solid var(--border);
      border-radius: 6px;
      margin-top: 6px;
    }
    table { width: 100%; border-collapse: collapse; font-size: 10px; text-align: center; }
    th { background: #131c31; color: var(--muted); padding: 5px 2px; position: sticky; top: 0; }
    td { padding: 5px 2px; border-bottom: 1px solid #172238; }

    /* FOOTER */
    .footer { text-align: center; font-size: 10px; color: #475569; margin-top: 10px; }
  </style>
</head>
<body>

  <!-- TOAST NOTIFICATION -->
  <div id="toast">Notificación</div>

  <!-- NAVEGACIÓN SUPERIOR (TABS) -->
  <nav class="nav-bar">
    <button class="tab-btn active" id="btn_tab_op" onclick="switchTab('op')">📱 Operación</button>
    <button class="tab-btn" id="btn_tab_adm" onclick="switchTab('adm')">⚙️ Administrador</button>
    <button class="tab-btn" id="btn_tab_gra" onclick="switchTab('gra')">📈 Gráficos</button>
  </nav>

  <!-- ========================================================================= -->
  <!-- 1. VISTA OPERADOR (PANEL GENERAL COMPACTO - FITS ON MOBILE)               -->
  <!-- ========================================================================= -->
  <div class="view active" id="tab_op">

    <!-- HEADER STATUS BAR -->
    <div class="header-row">
      <div class="brand">💧 PLANTA <span>UF FX100</span></div>
      <div style="display:flex; align-items:center; gap:6px;">
        <span class="badge badge-off" id="badge_estado">DETENIDA</span>
      </div>
    </div>

    <!-- CAUDALÍMETROS EN 2 COLUMNAS COMPACTAS -->
    <div class="flow-grid">
      <div class="flow-box" style="border-top: 3px solid var(--alim);">
        <div class="flow-lbl" style="color:var(--primary)">Q ALIMENTACIÓN</div>
        <div class="flow-val" style="color:#38bdf8" id="op_qa">0.0</div>
        <div class="flow-sub">mL/min • <span id="op_fa">0.0</span> Hz</div>
        <div class="flow-sub" style="margin-top:2px;">Vol: <b id="op_va" style="color:#cbd5e1">0.00</b> L</div>
      </div>
      <div class="flow-box" style="border-top: 3px solid var(--perm);">
        <div class="flow-lbl" style="color:#34d399">Q PERMEADO</div>
        <div class="flow-val" style="color:#34d399" id="op_qp">0.0</div>
        <div class="flow-sub">mL/min • <span id="op_recov">0.0</span>% Y</div>
        <div class="flow-sub" style="margin-top:2px;">Vol: <b id="op_vp" style="color:#cbd5e1">0.00</b> L</div>
      </div>
    </div>

    <!-- TELEMETRÍA EN LÍNEA (TIEMPO, TEMP, PTM) -->
    <div class="tele-bar">
      <div class="tele-item">
        <span>⏱️ TIEMPO</span>
        <b id="op_timer" style="color:#f8fafc">00:00:00</b>
      </div>
      <div class="tele-item">
        <span>🌡️ TEMPERATURA</span>
        <b id="op_temp" style="color:#fbbf24">20.0 °C</b>
      </div>
      <div class="tele-item">
        <span>📊 PTM (TMP)</span>
        <b id="op_tmp" style="color:#38bdf8">0.25 bar</b>
      </div>
      <div class="tele-item">
        <span>💧 RETENTADO</span>
        <b id="op_qret" style="color:#f59e0b">0.0 mL/m</b>
      </div>
    </div>

    <!-- CONTROL DE RPM: CONTADOR ARRIBA + DIAL TÁCTIL (KNOB) -->
    <div class="rpm-container">
      <div class="rpm-display-box">
        <div class="rpm-big"><span id="op_rpm_actual">0.0</span><small>RPM</small></div>
        <div class="rpm-sub">CONSIGNA: <b id="op_rpm_obj" style="color:var(--primary)">50</b> RPM • TEÓRICO: <b id="op_q_teo">677</b> mL/min</div>
      </div>

      <!-- DIAL CIRCULAR SVG TÁCTIL -->
      <div class="dial-wrap" id="dial_wrap">
        <svg class="dial-svg" viewBox="0 0 170 170">
          <defs>
            <linearGradient id="dialGrad" x1="0%" y1="0%" x2="100%" y2="0%">
              <stop offset="0%" stop-color="#0284c7" />
              <stop offset="50%" stop-color="#38bdf8" />
              <stop offset="85%" stop-color="#10b981" />
              <stop offset="100%" stop-color="#f59e0b" />
            </linearGradient>
          </defs>
          <!-- Arco Base -->
          <path class="dial-track" id="dial_track" d="M 33 137 A 62 62 0 1 1 137 137" />
          <!-- Arco Progreso -->
          <path class="dial-prog" id="dial_prog" d="M 33 137 A 62 62 0 1 1 137 137" stroke-dasharray="292" stroke-dashoffset="170" />
          <!-- Cursor Aguja -->
          <circle id="dial_handle" cx="85" cy="23" r="8" fill="#38bdf8" stroke="#ffffff" stroke-width="2" filter="drop-shadow(0 0 6px #38bdf8)" />
        </svg>

        <!-- Botones Pasos [-] y [+] integrados en el dial -->
        <div class="dial-center-btn">
          <button class="step-btn" onclick="stepRPM(-1)" title="-1 RPM">◀</button>
          <div style="font-size:10px; font-weight:800; color:var(--muted)">AJUSTE</div>
          <button class="step-btn" onclick="stepRPM(+1)" title="+1 RPM">▶</button>
        </div>
      </div>

      <!-- PRESETS RÁPIDOS DE RPM -->
      <div class="presets-row">
        <button class="preset-pill" data-rpm="20" onclick="setRPM(20)">20</button>
        <button class="preset-pill" data-rpm="30" onclick="setRPM(30)">30</button>
        <button class="preset-pill" data-rpm="40" onclick="setRPM(40)">40</button>
        <button class="preset-pill active" data-rpm="50" onclick="setRPM(50)">50</button>
        <button class="preset-pill" data-rpm="70" onclick="setRPM(70)">70</button>
        <button class="preset-pill" data-rpm="90" onclick="setRPM(90)">90</button>
      </div>
    </div>

    <!-- BOTONERA DE ACCIÓN INMEDIATA -->
    <div class="action-row">
      <button class="btn-act btn-start" id="btn_start" onclick="cmd('START')">▶ ARRANCAR</button>
      <button class="btn-act btn-stop stopped" id="btn_stop" onclick="cmd('STOP')">⏹ PARAR</button>
    </div>
    <button class="btn-dir" id="btn_dir" onclick="cmd('DIR')">🔄 HORARIO (FILTRACIÓN TANGENCIAL)</button>

    <div class="footer">ESP32 UF UNSa • AP: <b>192.168.4.1</b> • mDNS: <b>bomba.local</b></div>
  </div>

  <!-- ========================================================================= -->
  <!-- 2. VISTA ADMINISTRADOR (CALIBRACIÓN, LIMITES, DIMER, ENSAYOS)            -->
  <!-- ========================================================================= -->
  <div class="view" id="tab_adm">

    <!-- CINEMÁTICA & DIMER DE FLUIDEZ -->
    <div class="card">
      <div class="card-title">⚙️ Cinemática & Dimer de Fluidez</div>
      
      <div class="form-group">
        <label>RPM de Inicio al Arrancar:</label>
        <div style="display:flex; gap:4px;">
          <input type="number" id="adm_rpm_ini" min="15" max="100" value="50">
          <span style="font-size:10px; align-self:center; color:var(--muted)">RPM</span>
        </div>
      </div>

      <div class="form-group">
        <label>Modo de Parada:</label>
        <div class="seg-group">
          <button class="seg-btn active" id="btn_stop_rampa" onclick="setStopMode(false)">🟢 Rampa Suave</button>
          <button class="seg-btn" id="btn_stop_seco" onclick="setStopMode(true)">🔴 Parada Seca</button>
        </div>
      </div>

      <div class="form-group" style="flex-direction:column; align-items:stretch; gap:6px;">
        <div style="display:flex; justify-content:space-between;">
          <label>Dimer Arranque Fluido (Rampa):</label>
          <b style="color:var(--primary)"><span id="dimmer_val_lbl">50</span>% (<span id="dimmer_rate_lbl">12.0</span> RPM/s)</b>
        </div>
        <input type="range" id="dimmer_slider" min="5" max="100" step="5" value="50" oninput="onDimmerChange(this.value)">
        <div style="display:flex; justify-content:space-between; font-size:9px; color:var(--muted)">
          <span>5% (Ultra suave)</span>
          <span>50% (Nominal)</span>
          <span>100% (Rápido)</span>
        </div>
      </div>
    </div>

    <!-- LÍMITES DE SEGURIDAD & ALARMAS -->
    <div class="card">
      <div class="card-title">🛡️ Límites de Seguridad & Enclavamientos</div>
      
      <div class="form-group">
        <label>Caudal Alim. Máximo:</label>
        <div><input type="number" id="adm_q_max" value="1400"> <small>mL/m</small></div>
      </div>
      <div class="form-group">
        <label>Caudal Alim. Mínimo (Sequedad):</label>
        <div><input type="number" id="adm_q_min" value="0"> <small>mL/m</small></div>
      </div>
      <div class="form-group">
        <label>Temperatura Máxima Membrana:</label>
        <div><input type="number" id="adm_temp_max" step="0.5" value="35.0"> <small>°C</small></div>
      </div>
      <div class="form-group">
        <label>Presión Transmembrana (TMP) Máx:</label>
        <div><input type="number" id="adm_tmp_max" step="0.05" value="0.60"> <small>bar</small></div>
      </div>
      <div class="form-group">
        <label>Parada Automática por Límites:</label>
        <input type="checkbox" id="adm_auto_stop" style="width:20px; height:20px; accent-color:var(--primary)">
      </div>
      <button class="btn-save" onclick="guardarLimitesYCinematica()">💾 Aplicar y Guardar Límites en NVS</button>
    </div>

    <!-- CALIBRACIÓN METROLÓGICA -->
    <div class="card">
      <div class="card-title">⚖️ Calibración Metrológica de Sensores</div>
      
      <div class="form-group">
        <label>Factor K Alimentación:</label>
        <div><input type="number" id="adm_ka" step="0.01" value="205.98"> <small>Hz/(L/m)</small></div>
      </div>
      <div class="form-group">
        <label>Factor K Permeado:</label>
        <div><input type="number" id="adm_kp" step="0.01" value="205.98"> <small>Hz/(L/m)</small></div>
      </div>
      <div class="form-group">
        <label>Cilindrada Bomba:</label>
        <div><input type="number" id="adm_ml" step="0.01" value="13.55"> <small>mL/rev</small></div>
      </div>
      <div class="form-group">
        <label>Pulsos / Rev (DM860):</label>
        <div><input type="number" id="adm_pul" value="3200"> <small>p/r</small></div>
      </div>

      <!-- ASISTENTE DE PROBETA -->
      <div style="background:#090e1a; padding:8px; border-radius:6px; margin:8px 0; border:1px solid #1e293b;">
        <div style="font-size:11px; font-weight:700; color:#cbd5e1; margin-bottom:4px;">🧪 Asistente Calibración con Probeta</div>
        <div style="display:flex; gap:6px; margin-bottom:6px;">
          <input type="number" id="cal_vol_ml" placeholder="Vol (mL)" style="flex:1">
          <input type="number" id="cal_tiempo_s" placeholder="Tiempo (s)" style="flex:1">
        </div>
        <button class="step-btn" style="width:100%; border-radius:6px; font-size:11px; height:28px;" onclick="calcularFactorProbeta()">Calcular K Probeta</button>
      </div>

      <div style="display:flex; gap:6px;">
        <button class="btn-save" style="background:#10b981; margin-top:0;" onclick="guardarCalibracion()">💾 Guardar NVS</button>
        <button class="btn-save" style="background:#475569; margin-top:0;" onclick="resetCalibracion()">🔄 Fábrica</button>
      </div>
    </div>

    <!-- GESTIÓN DE ENSAYOS & EXCEL/CSV -->
    <div class="card">
      <div class="card-title">📋 Gestión de Ensayos & Descargas</div>
      
      <div style="display:grid; grid-template-columns:1fr 1fr; gap:6px; margin-bottom:6px;">
        <button class="btn-act" style="background:#0284c7; color:#fff;" onclick="cmd('START_ENSAYO')">▶ Iniciar Ensayo</button>
        <button class="btn-act" style="background:#475569; color:#fff;" onclick="cmd('STOP_ENSAYO')">⏹ Detener Ensayo</button>
      </div>
      <button class="btn-act" style="width:100%; background:#1e293b; color:#38bdf8; border:1px solid #334155; margin-bottom:6px;" onclick="cmd('FIJAR_Q0')">⭐ Fijar Referencia Q0 (Membrana Limpia)</button>

      <button class="btn-excel" onclick="descargarExcel('actual')">📥 Descargar Ensayo Actual (Excel / CSV)</button>
      <button class="btn-excel" style="background:#0284c7" onclick="descargarExcel('all')">📦 Descargar Historial Completo (.CSV)</button>
      <button class="btn-act" style="width:100%; background:#3f1111; color:#f87171; border:1px solid #7f1d1d; margin-top:6px; font-size:11px; padding:6px;" onclick="limpiarRegistros()">🗑️ Limpiar Memoria de Registros</button>

      <!-- TABLA DE ENSAYOS -->
      <div class="table-box">
        <table>
          <thead>
            <tr><th>#ID</th><th>RPM</th><th>Duración</th><th>Merma Alim</th><th>Merma Perm</th><th>CSV</th></tr>
          </thead>
          <tbody id="body_ensayos">
            <tr><td colspan="6" style="color:var(--muted)">Sin ensayos registrados aún.</td></tr>
          </tbody>
        </table>
      </div>
    </div>

    <!-- GUÍA DE SWITCHES DM860 -->
    <div class="card" style="background:#06101e; border-color:#0284c7;">
      <div class="card-title" style="color:#38bdf8">⚙️ Switches Físicos Driver Leadshine DM860</div>
      <p style="font-size:11px; color:#cbd5e1; line-height:1.4; margin-bottom:6px;">
        <b>Corriente (3.14A RMS):</b> SW1=ON, SW2=OFF, SW3=ON<br>
        <b>Reposo (Half Current 50% anti-ruido):</b> SW4=OFF<br>
        <b>Resolución (3200 pulsos/rev):</b> SW5=OFF, SW6=OFF, SW7=ON, SW8=ON
      </p>
    </div>

  </div>

  <!-- ========================================================================= -->
  <!-- 3. VISTA GRÁFICOS (CURVAS EN EL TIEMPO)                                  -->
  <!-- ========================================================================= -->
  <div class="view" id="tab_gra">

    <div class="card">
      <div class="card-title">
        <span>📈 Caudales vs Tiempo</span>
        <span style="color:var(--primary); font-size:10px;">Q_Alim (Azul) • Q_Perm (Verde)</span>
      </div>
      <div class="chart-box">
        <canvas id="chart_q"></canvas>
      </div>
    </div>

    <div class="card">
      <div class="card-title">
        <span>📊 Presión Transmembrana (TMP) y Temperatura</span>
        <span style="color:#fbbf24; font-size:10px;">TMP (Cian) • Temp (Naranja)</span>
      </div>
      <div class="chart-box">
        <canvas id="chart_tmp"></canvas>
      </div>
    </div>

    <button class="btn-excel" onclick="descargarExcel('actual')">📥 Exportar Puntos del Gráfico a Excel / CSV</button>
  </div>

  <!-- ========================================================================= -->
  <!-- JAVASCRIPT HMI ENGINE                                                    -->
  <!-- ========================================================================= -->
  <script>
    const $ = id => document.getElementById(id);
    let pollBusy = false;
    let currentTab = 'op';
    let targetRpm = 50;
    let actualRpm = 0;
    let isRunning = false;
    let modoParadaSeca = false;

    // Buffer histórico de puntos para los gráficos en tiempo real
    const MAX_HIST = 80;
    let histT = [], histQA = [], histQP = [], histTMP = [], histTemp = [];

    // Cambiar Pestañas
    function switchTab(t) {
      currentTab = t;
      ['op', 'adm', 'gra'].forEach(k => {
        $('tab_' + k).classList.toggle('active', k === t);
        $('btn_tab_' + k).classList.toggle('active', k === t);
      });
      if (t === 'gra') renderCharts();
    }

    function showToast(msg, isError = false) {
      const el = $('toast');
      el.innerText = msg;
      el.style.background = isError ? '#7f1d1d' : '#1e1b4b';
      el.style.borderColor = isError ? '#ef4444' : '#818cf8';
      el.style.display = 'block';
      setTimeout(() => { el.style.display = 'none'; }, 3500);
    }

    // =========================================================================
    // DIAL TÁCTIL CIRCULAR (KNOB DE RPM)
    // Rango: 15 a 100 RPM. Ángulo: 135° (min) a 405° (max) -> sweep 270°
    // Longitud total del arco: ~292px
    // =========================================================================
    const TOTAL_ARC = 292;
    const MIN_RPM = 15, MAX_RPM = 100;

    function rpmToAngle(rpm) {
      const norm = (rpm - MIN_RPM) / (MAX_RPM - MIN_RPM);
      return 135 + norm * 270;
    }

    function angleToRpm(deg) {
      let a = deg;
      if (a < 0) a += 360;
      if (a < 90) a += 360; // zona inferior derecha
      if (a < 135) a = 135;
      if (a > 405) a = 405;
      const norm = (a - 135) / 270;
      return Math.round(MIN_RPM + norm * (MAX_RPM - MIN_RPM));
    }

    function updateDialVisual(rpm) {
      const norm = Math.max(0, Math.min(1, (rpm - MIN_RPM) / (MAX_RPM - MIN_RPM)));
      const offset = TOTAL_ARC - norm * TOTAL_ARC;
      $('dial_prog').style.strokeDashoffset = offset;

      // Calcular coordenadas del tirador
      const rad = (rpmToAngle(rpm) * Math.PI) / 180;
      const cx = 85 + 62 * Math.cos(rad);
      const cy = 85 + 62 * Math.sin(rad);
      $('dial_handle').setAttribute('cx', cx.toFixed(1));
      $('dial_handle').setAttribute('cy', cy.toFixed(1));

      $('op_rpm_obj').innerText = rpm;
      const mlRev = parseFloat($('adm_ml')?.value || 13.55);
      $('op_q_teo').innerText = (rpm * mlRev).toFixed(0);

      // Resaltar preset
      document.querySelectorAll('.preset-pill').forEach(btn => {
        const isMatch = Math.abs(parseFloat(btn.dataset.rpm) - rpm) < 0.5;
        btn.classList.toggle('active', isMatch);
      });
    }

    // Control de Pointer Events en el Dial
    let isDraggingDial = false;
    let dialSendTimer = null;

    function handleDialPointer(e) {
      const rect = $('dial_wrap').getBoundingClientRect();
      const cx = rect.left + rect.width / 2;
      const cy = rect.top + rect.height / 2;
      const x = e.clientX - cx;
      const y = e.clientY - cy;
      let deg = (Math.atan2(y, x) * 180) / Math.PI;
      const newRpm = angleToRpm(deg);
      targetRpm = newRpm;
      updateDialVisual(newRpm);

      // Enviar con throttle (120 ms)
      clearTimeout(dialSendTimer);
      dialSendTimer = setTimeout(() => {
        fetch('/set?rpm=' + targetRpm);
      }, 120);
    }

    const dw = $('dial_wrap');
    dw.addEventListener('pointerdown', e => {
      isDraggingDial = true;
      dw.setPointerCapture(e.pointerId);
      handleDialPointer(e);
    });
    dw.addEventListener('pointermove', e => {
      if (isDraggingDial) handleDialPointer(e);
    });
    dw.addEventListener('pointerup', e => {
      if (isDraggingDial) {
        isDraggingDial = false;
        try { dw.releasePointerCapture(e.pointerId); } catch(_) {}
        fetch('/set?rpm=' + targetRpm);
      }
    });

    function setRPM(rpm) {
      targetRpm = Math.max(MIN_RPM, Math.min(MAX_RPM, rpm));
      updateDialVisual(targetRpm);
      fetch('/set?rpm=' + targetRpm);
    }

    function stepRPM(delta) {
      setRPM(targetRpm + delta);
    }

    // =========================================================================
    // COMANDOS DE ACCIÓN Y PETICIONES HTTP
    // =========================================================================
    function cmd(act) {
      fetch('/cmd?act=' + act)
        .then(r => { if (r.ok) showToast('Comando ejecutado: ' + act); });
    }

    // Dimer de fluidez de arranque (0% a 100%)
    let dimmerTimer = null;
    function onDimmerChange(val) {
      $('dimmer_val_lbl').innerText = val;
      const rate = (12.0 * (val / 50.0)).toFixed(1);
      $('dimmer_rate_lbl').innerText = rate;
      clearTimeout(dimmerTimer);
      dimmerTimer = setTimeout(() => {
        fetch('/set_dev?acel_pct=' + val);
      }, 150);
    }

    function setStopMode(isSeco) {
      modoParadaSeca = isSeco;
      $('btn_stop_rampa').classList.toggle('active', !isSeco);
      $('btn_stop_seco').classList.toggle('active', isSeco);
      fetch('/set_dev?stop_seco=' + (isSeco ? '1' : '0'));
    }

    function guardarLimitesYCinematica() {
      const qmax = $('adm_q_max').value;
      const qmin = $('adm_q_min').value;
      const tmax = $('adm_temp_max').value;
      const ptmmax = $('adm_tmp_max').value;
      const rpmini = $('adm_rpm_ini').value;
      const autostop = $('adm_auto_stop').checked ? '1' : '0';
      const url = `/set_dev?save=1&q_max=${qmax}&q_min=${qmin}&temp_max=${tmax}&tmp_max=${ptmmax}&rpm_ini=${rpmini}&auto_stop=${autostop}`;
      fetch(url)
        .then(r => r.json())
        .then(d => { if (d.status === 'ok') showToast('Límites y cinemática guardados en Flash NVS.'); });
    }

    function guardarCalibracion() {
      const ka = $('adm_ka').value;
      const kp = $('adm_kp').value;
      const ml = $('adm_ml').value;
      const pul = $('adm_pul').value;
      const url = `/set_dev?save=1&ka=${ka}&kp=${kp}&ml=${ml}&pul=${pul}`;
      fetch(url)
        .then(r => r.json())
        .then(d => { if (d.status === 'ok') showToast('Calibración guardada en Flash NVS.'); });
    }

    function resetCalibracion() {
      if (!confirm('¿Restaurar factores K y parámetros a los oficiales de laboratorio?')) return;
      fetch('/reset_dev')
        .then(r => r.json())
        .then(() => showToast('Parámetros restaurados a valores oficiales.'));
    }

    function calcularFactorProbeta() {
      const v = parseFloat($('cal_vol_ml').value);
      const t = parseFloat($('cal_tiempo_s').value);
      if (!v || !t || t <= 0) { showToast('Ingresa volumen (mL) y tiempo (s)', true); return; }
      const qReal = (v / t) * 60; // mL/min
      const fActual = parseFloat($('op_fa').innerText);
      if (fActual > 1.0) {
        const nuevoK = (fActual * 1000.0) / qReal;
        $('adm_ka').value = nuevoK.toFixed(2);
        showToast(`Calculado K_Alim: ${nuevoK.toFixed(2)} Hz/(L/min)`);
      } else {
        showToast('Enciende la bomba para recibir pulsos de frecuencia', true);
      }
    }

    function descargarExcel(tipo) {
      window.location.href = '/export_csv?ensayo=' + tipo;
    }

    function limpiarRegistros() {
      if (!confirm('¿Borrar todos los ensayos de la memoria de la planta?')) return;
      fetch('/clear_csv').then(() => showToast('Memoria de registros limpiada.'));
    }

    // =========================================================================
    // DIBUJADO DE GRÁFICOS EN CANVAS (100% OFFLINE)
    // =========================================================================
    function drawLineChart(canvasId, seriesArr, maxValInit) {
      const cv = $(canvasId);
      if (!cv) return;
      const ctx = cv.getContext('2d');
      const w = cv.width = cv.clientWidth;
      const h = cv.height = cv.clientHeight;
      ctx.clearRect(0, 0, w, h);

      // Ejes y grilla
      ctx.strokeStyle = '#172238';
      ctx.lineWidth = 1;
      for (let y = 20; y < h; y += 35) {
        ctx.beginPath(); ctx.moveTo(35, y); ctx.lineTo(w - 10, y); ctx.stroke();
      }

      let maxVal = maxValInit;
      seriesArr.forEach(s => {
        s.data.forEach(v => { if (v > maxVal) maxVal = v; });
      });
      maxVal = maxVal * 1.15 || 10;

      // Etiquetas Y
      ctx.fillStyle = '#64748b';
      ctx.font = '9px monospace';
      ctx.fillText(maxVal.toFixed(0), 4, 18);
      ctx.fillText((maxVal / 2).toFixed(0), 4, h / 2);
      ctx.fillText('0', 16, h - 6);

      // Trazar series
      seriesArr.forEach(s => {
        if (s.data.length < 2) return;
        ctx.strokeStyle = s.color;
        ctx.lineWidth = 2;
        ctx.beginPath();
        const stepX = (w - 50) / (MAX_HIST - 1);
        s.data.forEach((val, idx) => {
          const x = 40 + idx * stepX;
          const y = h - 10 - (val / maxVal) * (h - 30);
          if (idx === 0) ctx.moveTo(x, y);
          else ctx.lineTo(x, y);
        });
        ctx.stroke();
      });
    }

    function renderCharts() {
      drawLineChart('chart_q', [
        { data: histQA, color: '#0284c7' },
        { data: histQP, color: '#10b981' }
      ], 800);

      drawLineChart('chart_tmp', [
        { data: histTMP, color: '#38bdf8' },
        { data: histTemp, color: '#f59e0b' }
      ], 1.0);
    }

    // =========================================================================
    // LOOP DE TELEMETRÍA ASÍNCRONO (/status CADA 600 ms)
    // =========================================================================
    setInterval(async () => {
      if (pollBusy) return;
      pollBusy = true;
      try {
        const res = await fetch('/status');
        const d = await res.json();
        pollBusy = false;

        // Actualizar datos del Operador
        $('op_qa').innerText = d.q_alim.toFixed(1);
        $('op_fa').innerText = d.f_alim.toFixed(1);
        $('op_va').innerText = d.vol_alim.toFixed(2);

        $('op_qp').innerText = d.q_perm.toFixed(1);
        $('op_recov').innerText = d.recov.toFixed(1);
        $('op_vp').innerText = d.vol_perm.toFixed(2);

        $('op_rpm_actual').innerText = d.rpm.toFixed(1);
        $('op_temp').innerText = d.temp.toFixed(1) + ' °C';
        $('op_tmp').innerText = d.tmp.toFixed(2) + ' bar';
        $('op_qret').innerText = d.q_ret.toFixed(1) + ' mL/m';

        // Formato cronómetro
        const sTot = d.t_ensayo_s;
        const hh = String(Math.floor(sTot / 3600)).padStart(2, '0');
        const mm = String(Math.floor((sTot % 3600) / 60)).padStart(2, '0');
        const ss = String(sTot % 60).padStart(2, '0');
        $('op_timer').innerText = `${hh}:${mm}:${ss}`;

        // Badge de estado
        const badge = $('badge_estado');
        if (d.emergencia || d.lim_q_excedido || d.lim_temp_excedido || d.lim_tmp_excedido) {
          badge.className = 'badge badge-alert';
          badge.innerText = d.emergencia ? 'EMERGENCIA' : '⚠️ LÍMITE EXCEDIDO';
        } else if (d.on) {
          badge.className = 'badge badge-on';
          badge.innerText = d.en_regimen ? 'RÉGIMEN ESTABLE' : 'ACELERANDO...';
        } else {
          badge.className = 'badge badge-off';
          badge.innerText = 'DETENIDA';
        }

        // Colores de botones de marcha
        isRunning = d.on;
        $('btn_start').classList.toggle('running', d.on);
        $('btn_stop').classList.toggle('stopped', !d.on);

        // Sentido
        const bDir = $('btn_dir');
        bDir.innerText = d.dir ? '🔄 HORARIO (FILTRACIÓN TANGENCIAL)' : '🔄 ANTIHORARIO (RETROLAVADO)';
        bDir.classList.toggle('inv', !d.dir);

        // Actualizar Dial si no lo estamos arrastrando
        if (!isDraggingDial && Math.abs(targetRpm - d.obj_rpm) > 0.4) {
          targetRpm = Math.round(d.obj_rpm);
          updateDialVisual(targetRpm);
        }

        // Guardar puntos para gráficos
        histQA.push(d.q_alim); if (histQA.length > MAX_HIST) histQA.shift();
        histQP.push(d.q_perm); if (histQP.length > MAX_HIST) histQP.shift();
        histTMP.push(d.tmp);   if (histTMP.length > MAX_HIST) histTMP.shift();
        histTemp.push(d.temp); if (histTemp.length > MAX_HIST) histTemp.shift();

        if (currentTab === 'gra') renderCharts();

        // Actualizar tabla de ensayos si estamos en Admin
        if (currentTab === 'adm' && d.ensayos) {
          const tb = $('body_ensayos');
          if (d.ensayos.length === 0) {
            tb.innerHTML = '<tr><td colspan="6" style="color:var(--muted)">Sin ensayos finalizados aún.</td></tr>';
          } else {
            let trHtml = '';
            d.ensayos.slice().reverse().forEach(e => {
              const mm = String(Math.floor(e.duracion_s / 60)).padStart(2, '0');
              const ss = String(e.duracion_s % 60).padStart(2, '0');
              trHtml += `<tr>
                <td><b>#${e.id}</b></td>
                <td>${e.rpm.toFixed(0)}</td>
                <td>${mm}:${ss}</td>
                <td style="color:#38bdf8">−${e.merma_a.toFixed(1)}%</td>
                <td style="color:#10b981">−${e.merma_p.toFixed(1)}%</td>
                <td><button style="padding:2px 6px; font-size:9px; background:#107c41; color:#fff; border:none; border-radius:4px;" onclick="window.location.href='/export_csv?ensayo=${e.id}'">CSV</button></td>
              </tr>`;
            });
            tb.innerHTML = trHtml;
          }
        }
      } catch (err) {
        pollBusy = false;
      }
    }, 600);

    // Inicialización al cargar la página
    window.addEventListener('DOMContentLoaded', () => {
      updateDialVisual(50);
    });
  </script>
</body>
</html>
)html";
