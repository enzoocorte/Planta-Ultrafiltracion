#include "index_html.h"

// ==============================================================================
// DEFINICIÓN DEL CONTENIDO HTML / CSS / JS (PROGMEM) — TESIS UNSa 2026
// Incluye:
// - Medición en Tiempo Real de ambos caudalímetros con rechazo de ruido EMI.
// - Panel y Gráfico en Vivo de "Merma de Caudal por Formación de la Torta"
//   (Q_Alim vs Q_Alim_0, Q_Perm vs Q_Perm_0, % de Merma y relación J/J0).
// - Modo Híbrido para Permeado: lectura automática del sensor YF-S401 o carga
//   rápida con probeta cuando el caudal cae por debajo del umbral mecánico de giro.
// ==============================================================================

const char INDEX_HTML[] PROGMEM = R"html(<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Planta UF - Monitoreo en Tiempo Real de Torta & SCADA</title>
  <style>
    :root {
      --bg: #0b1020;
      --card: #141c30;
      --card-dev: #171d33;
      --border: #263450;
      --text: #e2e8f0;
      --muted: #8296b3;
      --primary: #38bdf8;
      --alim: #06b6d4;
      --perm: #facc15;
      --ret: #f59e0b;
      --success: #10b981;
      --danger: #ef4444;
      --excel: #107c41;
      --dev: #a855f7;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      background: var(--bg);
      color: var(--text);
      padding: 14px;
      display: flex;
      justify-content: center;
    }
    .wrap { max-width: 560px; width: 100%; display: flex; flex-direction: column; gap: 12px; }
    .card {
      background: var(--card);
      border: 1px solid var(--border);
      border-radius: 14px;
      padding: 16px;
      box-shadow: 0 4px 20px rgba(0,0,0,0.4);
    }
    .card-dev {
      border: 1px solid rgba(168,85,247,0.5);
      background: #15162c;
    }
    h1 { font-size: 16px; color: var(--primary); text-align: center; margin-bottom: 2px; }
    .sub { font-size: 11px; color: var(--muted); text-align: center; margin-bottom: 12px; }
    .disp {
      text-align: center;
      background: #0a0f1e;
      border-radius: 10px;
      padding: 12px;
      margin-bottom: 10px;
      border: 1px solid #1a253c;
    }
    .rpm { font-size: 44px; font-weight: 800; color: var(--primary); line-height: 1; }
    .disp u { display: block; font-size: 10px; color: var(--muted); letter-spacing: 2px; text-decoration: none; margin-top: 4px; }
    .pill {
      display: inline-block;
      padding: 3px 12px;
      border-radius: 99px;
      font-size: 10px;
      font-weight: 700;
      margin-top: 8px;
    }
    .off { background: #2a3448; color: #aab7cc; }
    .on  { background: rgba(16,185,129,.25); color: #34d399; }
    .inv { background: rgba(245,158,11,.25); color: #fbbf24; }
    .al {
      display: none;
      background: rgba(239,68,68,.15);
      border: 1px solid var(--danger);
      color: #fca5a5;
      border-radius: 8px;
      padding: 8px;
      font-size: 11px;
      text-align: center;
      margin-bottom: 10px;
    }
    .vis { display: block !important; }
    .row { display: flex; justify-content: space-between; font-size: 11px; color: var(--muted); margin-bottom: 6px; align-items: center; flex-wrap: wrap; gap: 4px; }
    input[type=range] { width: 100%; accent-color: var(--primary); cursor: pointer; }
    .grid { display: grid; grid-template-columns: repeat(6,1fr); gap: 5px; margin: 10px 0; }
    button {
      border: none;
      border-radius: 8px;
      padding: 8px 4px;
      font-weight: 700;
      font-size: 11px;
      cursor: pointer;
      background: #22304e;
      color: var(--text);
      transition: all 0.2s;
    }
    button:hover { filter: brightness(1.15); }
    button:active { transform: scale(0.96); filter: brightness(1.3); }
    .btns { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }
    .go { background: var(--success); color: #04301c; }
    .no { background: var(--danger); color: #3d0808; }
    .dir { grid-column: span 2; background: #1c2a45; border: 1px solid var(--border); }
    .btn-excel {
      background: var(--excel);
      color: #ffffff;
      padding: 10px;
      font-size: 12px;
      font-weight: 800;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 6px;
      border-radius: 8px;
      text-decoration: none;
      border: none;
      width: 100%;
      cursor: pointer;
    }
    .btn-dev {
      background: var(--dev);
      color: #ffffff;
      padding: 6px 12px;
      font-size: 11px;
      font-weight: 700;
      border-radius: 6px;
    }
    .btn-autocal {
      background: linear-gradient(135deg, #0284c7, #0369a1);
      color: #ffffff;
      padding: 8px;
      font-size: 11px;
      font-weight: 800;
      border-radius: 6px;
      width: 100%;
      margin-top: 6px;
    }

    .sens { display: flex; flex-direction: column; gap: 10px; margin-top: 8px; }
    .s {
      background: #0a0f1e;
      border-radius: 10px;
      padding: 12px 14px;
      border: 1px solid #1a253c;
    }
    .s h3 { font-size: 12px; margin-bottom: 6px; display: flex; justify-content: space-between; align-items: center; }
    .s .val-row { display: flex; justify-content: space-between; align-items: baseline; }
    .s .v { font-size: 28px; font-weight: 800; }
    .s .info-row { display: flex; justify-content: space-between; font-size: 11px; color: var(--muted); margin-top: 4px; }
    .s .diag-row { display: flex; justify-content: space-between; font-size: 10px; color: #64748b; margin-top: 4px; padding-top: 4px; border-top: 1px dashed #1e293b; }
    .badge { display: none; color: var(--danger); font-size: 9px; font-weight: bold; }

    .torta-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-top: 8px; }
    .torta-box {
      background: #0a0f1e;
      border: 1px solid #1e293b;
      border-radius: 10px;
      padding: 10px;
    }
    .torta-title { font-size: 10px; font-weight: 700; color: var(--muted); margin-bottom: 4px; }
    .torta-val { font-size: 20px; font-weight: 800; }
    .torta-sub { font-size: 10px; color: #94a3b8; margin-top: 2px; }

    .bal {
      background: #0a0f1e;
      border-radius: 10px;
      padding: 10px;
      margin-top: 10px;
      font-size: 11px;
      border: 1px solid #1a253c;
    }
    .bal div { display: flex; justify-content: space-between; padding: 3px 0; border-bottom: 1px solid #151f32; }
    .bal div:last-child { border-bottom: none; }
    .lbl { color: var(--muted); }
    .val { font-weight: 700; }

    .input-group { display: flex; flex-direction: column; gap: 4px; margin-bottom: 8px; }
    .input-group label { font-size: 11px; color: #cbd5e1; font-weight: 600; }
    .input-group input, .input-group select {
      background: #0a0f1e;
      border: 1px solid #263450;
      color: #38bdf8;
      padding: 8px;
      border-radius: 6px;
      font-size: 12px;
      font-weight: 700;
    }
    .input-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }

    .table-box {
      margin-top: 10px;
      max-height: 180px;
      overflow-y: auto;
      border: 1px solid #1a253c;
      border-radius: 8px;
      background: #080d1a;
    }
    table { width: 100%; border-collapse: collapse; font-size: 10px; text-align: left; }
    th { background: #131b2e; color: var(--muted); padding: 6px 8px; position: sticky; top: 0; }
    td { padding: 6px 8px; border-bottom: 1px solid #131b2e; color: #e2e8f0; }

    .toast {
      display: none;
      background: #1e1b4b;
      border: 1px solid #a855f7;
      color: #e9d5ff;
      padding: 10px 14px;
      border-radius: 8px;
      font-size: 11px;
      text-align: center;
      margin-bottom: 8px;
    }

    .progress-bar-bg {
      background: #0a0f1e;
      border: 1px solid #293855;
      border-radius: 99px;
      height: 14px;
      overflow: hidden;
      margin-top: 6px;
      position: relative;
    }
    .progress-bar-fill {
      background: linear-gradient(90deg, #38bdf8, #a855f7);
      height: 100%;
      width: 0%;
      transition: width 0.3s;
    }
    .progress-text {
      position: absolute;
      width: 100%;
      text-align: center;
      font-size: 9px;
      line-height: 14px;
      color: #fff;
      font-weight: 800;
      top: 0;
      left: 0;
    }
    footer { text-align: center; font-size: 10px; color: var(--muted); padding: 6px; }
  </style>
</head>
<body>
  <div class="wrap">

    <!-- CARD 1: CONTROL DE BOMBA PERISTÁLTICA -->
    <div class="card">
      <div class="row" style="margin-bottom:4px; gap:6px; flex-wrap:wrap">
        <h1 style="text-align:left; margin-bottom:0">PLANTA UF • UNSa 2026</h1>
        <div style="display:flex; gap:5px; flex-wrap:wrap;">
          <button class="btn-dev" style="background:#0284c7; color:#fff;" onclick="toggleSwitches()">⚙️ Switches DM860</button>
          <button class="btn-dev" onclick="toggleDev()">🛠️ Calibración / Factores K</button>
        </div>
      </div>
      <p class="sub" style="text-align:left">Medición Continua en Tiempo Real • Seguimiento de Formación de Torta</p>

      <div class="disp">
        <div class="rpm" id="rpm">0.0</div>
        <u>RPM INSTANTÁNEA (OBJETIVO: <span id="obj_rpm_lbl" style="color:var(--primary)">50</span> RPM)</u>
        <div class="pill off" id="pil">DETENIDA</div>
      </div>

      <div class="al" id="al">⚠ ALERTA: Caudal Alimentación excede límite</div>

      <div class="row">
        <span>Consigna de Operación:</span>
        <span><b id="lc" style="color:var(--primary)">50</b> RPM</span>
      </div>
      <input type="range" id="sl" min="15" max="100" step="1" value="50">

      <div class="grid">
        <button class="rpm-btn" data-rpm="20" onclick="setR(20)">20 RPM</button>
        <button class="rpm-btn" data-rpm="30" onclick="setR(30)">30 RPM</button>
        <button class="rpm-btn" data-rpm="40" onclick="setR(40)">40 RPM</button>
        <button class="rpm-btn" data-rpm="50" onclick="setR(50)">50 RPM</button>
        <button class="rpm-btn" data-rpm="70" onclick="setR(70)">70 RPM</button>
        <button class="rpm-btn" data-rpm="90" onclick="setR(90)">90 RPM</button>
      </div>

      <div class="btns">
        <button class="go" id="btn_start" onclick="cmd('START')">▶ ARRANCAR</button>
        <button class="no" id="btn_stop" onclick="cmd('STOP')">⏹ PARAR</button>
        <button class="dir" id="bd" onclick="cmd('DIR')">🔄 HORARIO (FILTRACIÓN)</button>
      </div>
    </div>

    <!-- CARD COLAPSABLE: GUÍA OFICIAL DE SWITCHES DEL DRIVER DM860 -->
    <div class="card" id="card_switches" style="display:none; border:1px solid #0284c7; background:#071426;">
      <div class="row" style="margin-bottom:8px">
        <b style="font-size:12px; color:#38bdf8">⚙️ CONFIGURACIÓN OFICIAL DE DIP SWITCHES — DRIVER LEADSHINE DM860</b>
        <button style="padding:2px 8px; font-size:10px; background:#1e293b; color:#94a3b8; border:1px solid #334155" onclick="toggleSwitches()">✖ Cerrar</button>
      </div>
      <p style="font-size:10px; color:var(--muted); margin-bottom:10px;">
        Posición física de los 8 microinterruptores del driver DM860 para Motor Paso a Paso NEMA 34 (4.5 Nm) y Bomba Peristáltica (3200 pulsos/rev).
      </p>

      <div style="display:grid; grid-template-columns: repeat(auto-fit, minmax(140px, 1fr)); gap:8px; margin-bottom:10px;">
        <!-- Bloque 1: SW1-SW3 Corriente -->
        <div style="background:#0b1d36; border:1px solid #1e3a5f; border-radius:8px; padding:8px;">
          <div style="font-size:10px; font-weight:700; color:#38bdf8; margin-bottom:4px">1. CORRIENTE (SW1 - SW3)</div>
          <div style="font-size:12px; font-weight:800; color:#fff">4.9A Pico / 3.5A RMS</div>
          <div style="font-size:9px; color:#94a3b8; margin:3px 0;">(Opción 5.6A: OFF, ON, OFF)</div>
          <div style="display:flex; gap:3px; font-size:9px; font-weight:800; margin-top:6px;">
            <span style="background:#16a34a; color:#fff; padding:2px 5px; border-radius:4px;">SW1: ON</span>
            <span style="background:#dc2626; color:#fff; padding:2px 5px; border-radius:4px;">SW2: OFF</span>
            <span style="background:#dc2626; color:#fff; padding:2px 5px; border-radius:4px;">SW3: OFF</span>
          </div>
        </div>

        <!-- Bloque 2: SW4 Reposo Frío -->
        <div style="background:#06231d; border:1px solid #059669; border-radius:8px; padding:8px;">
          <div style="font-size:10px; font-weight:700; color:#34d399; margin-bottom:4px">2. REPOSO FRÍO (SW4) ⭐</div>
          <div style="font-size:12px; font-weight:800; color:#34d399">HALF CURRENT (50%)</div>
          <div style="font-size:9px; color:#a7f3d0; margin:3px 0;">¡Evita sobrecalentamiento!</div>
          <div style="display:flex; gap:3px; font-size:9px; font-weight:800; margin-top:6px;">
            <span style="background:#16a34a; color:#fff; padding:2px 7px; border-radius:4px; font-size:10px;">SW4: ON (Half)</span>
          </div>
        </div>

        <!-- Bloque 3: SW5-SW8 Micropasos -->
        <div style="background:#0b1d36; border:1px solid #1e3a5f; border-radius:8px; padding:8px;">
          <div style="font-size:10px; font-weight:700; color:#facc15; margin-bottom:4px">3. MICROPASOS (SW5 - SW8)</div>
          <div style="font-size:12px; font-weight:800; color:#fff">3200 Pulsos / Rev (1/16)</div>
          <div style="font-size:9px; color:#94a3b8; margin:3px 0;">Calibrado con cil = 13.55 mL/rev</div>
          <div style="display:flex; gap:3px; font-size:9px; font-weight:800; margin-top:6px; flex-wrap:wrap;">
            <span style="background:#16a34a; color:#fff; padding:2px 4px; border-radius:4px;">SW5: ON</span>
            <span style="background:#dc2626; color:#fff; padding:2px 4px; border-radius:4px;">SW6: OFF</span>
            <span style="background:#16a34a; color:#fff; padding:2px 4px; border-radius:4px;">SW7: ON</span>
            <span style="background:#16a34a; color:#fff; padding:2px 4px; border-radius:4px;">SW8: ON</span>
          </div>
        </div>
      </div>

      <div style="font-size:10px; background:#040d1a; padding:6px 8px; border-radius:6px; border-left:3px solid #38bdf8; color:#94a3b8">
        📌 <b>Posición Física Oficial de Palanquitas en el DM860:</b><br>
        [SW1: <b style="color:#34d399">ON</b>] &nbsp; [SW2: <b style="color:#f87171">OFF</b>] &nbsp; [SW3: <b style="color:#f87171">OFF</b>] &nbsp; [SW4: <b style="color:#34d399">ON</b>] &nbsp; | &nbsp; [SW5: <b style="color:#34d399">ON</b>] &nbsp; [SW6: <b style="color:#f87171">OFF</b>] &nbsp; [SW7: <b style="color:#34d399">ON</b>] &nbsp; [SW8: <b style="color:#34d399">ON</b>]
      </div>
    </div>

    <!-- CARD 2: CAUDALES EN TIEMPO REAL Y FORMACIÓN DE TORTA -->
    <div class="card">
      <div class="row">
        <b style="font-size:12px; color:#fff">🌊 CAUDALES EN TIEMPO REAL (SIN PROBETA)</b>
        <div style="display:flex; gap:5px; align-items:center; flex-wrap:wrap;">
          <button id="btn_solo_alim" style="font-size:10px; padding:4px 8px; background:#1e293b; border:1px solid #475569" onclick="toggleSoloAlim()">🔒 Solo Alim</button>
          <button style="font-size:10px; padding:4px 8px" onclick="cmd('RESET_VOL')">Reset Vol</button>
        </div>
      </div>

      <div class="sens">
        <!-- 1. Sensor Alimentacion -->
        <div class="s" style="border-left: 4px solid var(--alim);">
          <h3 style="color:var(--alim)">
            <span>ALIMENTACIÓN EN TIEMPO REAL (GPIO 14)</span>
            <span class="badge" id="ba">SIN FLUJO</span>
          </h3>
          <div class="val-row">
            <div>
              <span class="v" id="va" style="color:var(--alim)">0.0</span>
              <small style="font-size:12px; font-weight:700; color:var(--muted)">mL/min</small>
            </div>
            <div style="text-align:right">
              <div style="font-size:14px; font-weight:700; color:#e2e8f0" id="la">0.000 L</div>
              <div style="font-size:10px; color:#94a3b8">Inicial Q₀: <b id="qa_ref" style="color:var(--alim)">--</b> mL/min</div>
            </div>
          </div>
          <div class="info-row">
            <span>Frecuencia: <b id="fa" style="color:#e2e8f0">0.0 Hz</b></span>
            <span>Factor K: <b id="lbl_ka" style="color:var(--alim)">205.98</b> Hz/(L/min)</span>
          </div>
          <div class="diag-row">
            <span>Pulsos válidos (1s): <b id="val_a" style="color:#34d399">0</b></span>
            <span>Ruido EMI filtrado (1s): <b id="gl_a" style="color:#94a3b8">0</b></span>
          </div>
        </div>

        <!-- 2. Sensor Permeado + Asistencia para Caudal Ultra-Bajo -->
        <div class="s" style="border-left: 4px solid var(--perm);">
          <h3 style="color:var(--perm)">
            <span>PERMEADO EN TIEMPO REAL (GPIO 27)</span>
            <span class="badge" id="bp">BAJO UMBRAL / SIN FLUJO</span>
          </h3>
          <div class="val-row">
            <div>
              <span class="v" id="vp" style="color:var(--perm)">0.0</span>
              <small style="font-size:12px; font-weight:700; color:var(--muted)">mL/min</small>
            </div>
            <div style="text-align:right">
              <div style="font-size:14px; font-weight:700; color:#e2e8f0" id="lp">0.000 L</div>
              <div style="font-size:10px; color:#94a3b8">Inicial Q₀: <b id="qp_ref" style="color:var(--perm)">--</b> mL/min</div>
            </div>
          </div>
          <div class="info-row">
            <span>Frecuencia: <b id="fp" style="color:#e2e8f0">0.0 Hz</b></span>
            <span>Flujo Darcy: <b id="j_lmh" style="color:var(--perm)">0.00 LMH</b></span>
          </div>
          <div class="diag-row">
            <span>Modo: <b id="lbl_estado_perm" style="color:#34d399">Sensor Tiempo Real</b></span>
            <span>Pulsos: <b id="val_p" style="color:#34d399">0</b> | EMI: <b id="gl_p">0</b></span>
          </div>

          <!-- Asistencia por Probeta SOLO si el caudal de permeado es tan bajo que no mueve la turbina -->
          <div style="margin-top:8px; padding-top:8px; border-top:1px solid #1e293b; display:flex; align-items:center; justify-content:space-between; gap:6px; flex-wrap:wrap;">
            <span style="font-size:10px; color:var(--muted);">¿Caudal &lt; mín. del sensor? Cargar probeta:</span>
            <div style="display:flex; gap:4px; align-items:center;">
              <input type="number" id="inp_qp_man" step="0.5" placeholder="mL/min" style="width:68px; background:#141c30; border:1px solid #334155; color:var(--perm); padding:4px 6px; border-radius:6px; font-size:11px; font-weight:700;">
              <button style="padding:4px 8px; font-size:10px; background:#ca8a04; color:#fff;" onclick="fijarPermManual()">Fijar</button>
              <button id="btn_perm_auto" style="padding:4px 8px; font-size:10px; background:#334155; display:none;" onclick="volverPermAuto()">Auto</button>
            </div>
          </div>
        </div>
      </div>

      <!-- PANEL DE MONITOREO DE FORMACIÓN DE TORTA (MERMA DE CAUDAL EN TIEMPO REAL) -->
      <div style="margin-top:12px; background:#0d1428; border:1px solid #263859; border-radius:10px; padding:12px;">
        <div class="row" style="margin-bottom:6px;">
          <b style="font-size:11px; color:#38bdf8">📉 MERMA DE CAUDAL POR FORMACIÓN DE TORTA (TIEMPO REAL)</b>
          <button style="font-size:10px; padding:3px 8px; background:#0284c7; color:#fff;" onclick="fijarQ0()">📌 Fijar Q₀ Actual (Filtro Limpio)</button>
        </div>

        <div class="torta-grid">
          <div class="torta-box" style="border-top:2px solid var(--alim)">
            <div class="torta-title">MERMA EN ALIMENTACIÓN</div>
            <div class="torta-val" id="merma_a" style="color:var(--alim)">0.0 %</div>
            <div class="torta-sub" id="merma_a_det">Caída vs Q₀: 0.0 mL/min</div>
          </div>
          <div class="torta-box" style="border-top:2px solid var(--perm)">
            <div class="torta-title">MERMA EN PERMEADO (1 − J/J₀)</div>
            <div class="torta-val" id="merma_p" style="color:var(--perm)">0.0 %</div>
            <div class="torta-sub" id="merma_p_det">Relación J/J₀: 100.0 %</div>
          </div>
        </div>

        <!-- Gráfico en Vivo de Caída de Caudal (HTML5 Canvas Offline) -->
        <div style="margin-top:10px;">
          <div class="row" style="font-size:10px; margin-bottom:3px;">
            <span>Curva en Tiempo Real: <b style="color:var(--alim)">━ Q_Alim(t)</b> | <b style="color:var(--perm)">━ Q_Perm(t)</b></span>
            <span id="chart_pts_lbl">0 s</span>
          </div>
          <canvas id="chart_torta" width="490" height="135" style="width:100%; height:135px; background:#070b16; border:1px solid #1a253c; border-radius:6px;"></canvas>
        </div>
      </div>

      <!-- Balance Hidráulico Instantáneo -->
      <div class="bal">
        <div>
          <span class="lbl">Caudal Retentado (Qalim − Qperm):</span>
          <span class="val" id="qr" style="color:var(--ret)">0.0 mL/min</span>
        </div>
        <div>
          <span class="lbl">Tasa de Recuperación (Y% = Qperm/Qalim):</span>
          <span class="val" id="rv" style="color:var(--primary)">0.0 %</span>
        </div>
        <div>
          <span class="lbl">Caudal Teórico Bomba (sin contrapresión):</span>
          <span class="val" id="qb">0.0 mL/min</span>
        </div>
        <div>
          <span class="lbl">Efecto Contrapresión Torta (Δ Alim vs Bomba):</span>
          <span class="val" id="db">0.0 %</span>
        </div>
      </div>
    </div>

    <!-- CARD: PRESIÓN MANOMÉTRICA PRIMARIA & LEY DE DARCY (SUBHITO 2.4) -->
    <div class="card" style="border: 1px solid #10b981; background: #0c1824;">
      <div class="row">
        <b style="font-size:12px; color:#34d399">📊 PRESIÓN MANOMÉTRICA PRIMARIA & DARCY EN TIEMPO REAL</b>
        <span class="pill on" id="pill_darcy" style="margin-top:0">DARCY ACTIVO</span>
      </div>
      <p style="font-size:10px; color:var(--muted); margin-bottom:8px;">
        Ingreso visual de Manómetros Bourdon de Glicerina (P₁, P₂) y Temperatura (T). El microcontrolador calcula en vivo TMP, factor TCF (viscosidad Vogel), flujo J₂₀ y resistencias hidráulicas R_total y R_torta en cada muestra.
      </p>

      <!-- SELECTOR Y MONITOREO DE TEMPERATURA (SENSOR DS18B20 EN D4 vs MANUAL) -->
      <div style="background:#07101b; padding:8px 10px; border-radius:8px; border:1px solid #13273e; margin-bottom:10px; display:flex; justify-content:space-between; align-items:center; flex-wrap:wrap; gap:6px;">
        <div style="display:flex; align-items:center; gap:8px;">
          <span style="font-size:11px; font-weight:700; color:#fff">🌡️ Medición de Temperatura:</span>
          <span class="badge" id="badge_sensor_temp" style="font-size:10px; padding:3px 6px; background:#065f46; color:#34d399;">🟢 SONDA D4 ACTIVA</span>
          <span style="font-size:11px; color:#94a3b8">En vivo: <b id="val_temp_live" style="color:#38bdf8; font-size:13px">20.0</b> °C</span>
        </div>
        <div style="display:flex; gap:4px; align-items:center;">
          <button id="btn_temp_auto" style="padding:4px 8px; font-size:10px; font-weight:700; background:#0284c7; color:#fff; border-radius:5px; border:none;" onclick="setTempModo(true)">⚡ Sonda DS18B20 (Auto)</button>
          <button id="btn_temp_man" style="padding:4px 8px; font-size:10px; font-weight:700; background:#1e293b; color:#94a3b8; border-radius:5px; border:1px solid #334155;" onclick="setTempModo(false)">✏️ Ingreso Manual</button>
        </div>
      </div>

      <div style="display:grid; grid-template-columns: repeat(3, 1fr) auto; gap:6px; align-items:end; margin-bottom:10px; background:#07101b; padding:8px; border-radius:8px; border:1px solid #13273e;">
        <div>
          <label style="font-size:10px; color:var(--alim); font-weight:700; display:block; margin-bottom:2px">P₁ Alim (bar):</label>
          <input type="number" id="inp_p1" step="0.01" min="0" max="6" value="0.30" style="width:100%; background:#141c30; border:1px solid #334155; color:#fff; padding:5px 6px; border-radius:6px; font-size:11px; font-weight:700; text-align:center;">
        </div>
        <div>
          <label style="font-size:10px; color:var(--ret); font-weight:700; display:block; margin-bottom:2px">P₂ Ret (bar):</label>
          <input type="number" id="inp_p2" step="0.01" min="0" max="6" value="0.20" style="width:100%; background:#141c30; border:1px solid #334155; color:#fff; padding:5px 6px; border-radius:6px; font-size:11px; font-weight:700; text-align:center;">
        </div>
        <div>
          <label style="font-size:10px; color:var(--perm); font-weight:700; display:block; margin-bottom:2px">Temp T (°C):</label>
          <input type="number" id="inp_temp" step="0.5" min="5" max="45" value="20.0" style="width:100%; background:#141c30; border:1px solid #334155; color:#fff; padding:5px 6px; border-radius:6px; font-size:11px; font-weight:700; text-align:center;">
        </div>
        <div>
          <button style="padding:6px 10px; font-size:11px; background:#10b981; color:#fff; font-weight:700;" onclick="actualizarPresion()">⚡ Aplicar</button>
        </div>
      </div>

      <div style="display:grid; grid-template-columns: repeat(4, 1fr); gap:6px; margin-bottom:8px;">
        <div style="background:#07101b; border:1px solid #162a42; border-radius:8px; padding:8px; text-align:center;">
          <div style="font-size:9px; color:var(--muted); font-weight:700">TMP EFECTIVA</div>
          <div style="font-size:15px; font-weight:800; color:#38bdf8" id="disp_tmp">0.250</div>
          <div style="font-size:9px; color:var(--muted)">bar</div>
        </div>
        <div style="background:#07101b; border:1px solid #162a42; border-radius:8px; padding:8px; text-align:center;">
          <div style="font-size:9px; color:var(--muted); font-weight:700">FLUJO J₂₀</div>
          <div style="font-size:15px; font-weight:800; color:var(--perm)" id="disp_j20">0.00</div>
          <div style="font-size:9px; color:var(--muted)">LMH (20°C)</div>
        </div>
        <div style="background:#07101b; border:1px solid #162a42; border-radius:8px; padding:8px; text-align:center;">
          <div style="font-size:9px; color:var(--muted); font-weight:700">FACTOR TCF</div>
          <div style="font-size:15px; font-weight:800; color:#cbd5e1" id="disp_tcf">1.000</div>
          <div style="font-size:9px; color:var(--muted)">μ(T)/μ₂₀</div>
        </div>
        <div style="background:#07101b; border:1px solid #162a42; border-radius:8px; padding:8px; text-align:center;">
          <div style="font-size:9px; color:var(--muted); font-weight:700">R_TORTA</div>
          <div style="font-size:12px; font-weight:800; color:#f59e0b" id="disp_rtorta">0.00e0</div>
          <div style="font-size:9px; color:var(--muted)">m⁻¹</div>
        </div>
      </div>

      <div class="row" style="font-size:10px; margin-bottom:0; color:var(--muted)">
        <span>Resistencia Total R_tot: <b id="disp_rtot" style="color:#e2e8f0">0.00e0 m⁻¹</b></span>
        <span>Membrana FX100 Rm: <b style="color:#94a3b8">2.09e13 m⁻¹</b></span>
      </div>
    </div>

    <!-- CARD 3: MODO CALIBRACIÓN / PARÁMETROS GUARDADOS -->
    <div class="card card-dev" id="card_dev" style="display:none">
      <div class="row">
        <b style="font-size:12px; color:var(--dev)">🛠️ PARÁMETROS DE CALIBRACIÓN (GUARDADOS EN FLASH)</b>
        <button style="background:#334155; font-size:10px; padding:2px 8px" onclick="toggleDev()">✕ Cerrar</button>
      </div>
      <div class="toast" id="toast_msg"></div>

      <p style="font-size:10px; color:var(--muted); margin-bottom:8px;">
        Aquí puedes cargar la <b>Cilindrada (mL/rev)</b> y el <b>Factor K [Hz/(L/min)]</b> obtenidos en tu calibración con probeta. Una vez guardados en Flash, el ESP32 mide todos los ensayos en tiempo real sin usar probeta:
      </p>

      <div class="input-grid">
        <div class="input-group">
          <label>Factor K Alimentación [Hz/(L/min)]:</label>
          <div style="display:flex; gap:4px; align-items:center;">
            <button type="button" style="padding:6px 10px; font-size:14px; font-weight:900; background:#334155" onclick="adjVal('inp_ka', -1.0)">−</button>
            <input type="number" id="inp_ka" step="0.01" value="205.98" style="width:100%; text-align:center;">
            <button type="button" style="padding:6px 10px; font-size:14px; font-weight:900; background:#334155" onclick="adjVal('inp_ka', +1.0)">+</button>
          </div>
        </div>
        <div class="input-group">
          <label>Factor K PERMEADO [Hz/(L/min)]:</label>
          <div style="display:flex; gap:4px; align-items:center;">
            <button type="button" style="padding:6px 10px; font-size:14px; font-weight:900; background:#334155" onclick="adjVal('inp_kp', -1.0)">−</button>
            <input type="number" id="inp_kp" step="0.01" value="205.98" style="width:100%; text-align:center;">
            <button type="button" style="padding:6px 10px; font-size:14px; font-weight:900; background:#334155" onclick="adjVal('inp_kp', +1.0)">+</button>
          </div>
        </div>
        <div class="input-group">
          <label>Cilindrada Bomba (mL/rev):</label>
          <input type="number" id="inp_ml" step="0.01" value="13.55">
        </div>
        <div class="input-group">
          <label>Micropasos (Pulsos/Rev):</label>
          <input type="number" id="inp_pul" step="100" value="3200">
        </div>
      </div>

      <div class="btns" style="margin-top:6px; margin-bottom:10px;">
        <button style="background:var(--dev); color:#fff" onclick="guardarParametros(1)">💾 Guardar Fijo en Flash ESP32</button>
        <button style="background:#334155" onclick="guardarParametros(0)">⚡ Aplicar Temporal</button>
      </div>

      <!-- SECCIÓN OPCIONAL: RE-CALIBRACIÓN PUNTUAL -->
      <div style="background:#0e1122; padding:10px; border-radius:10px; border:1px solid #293855;">
        <b style="font-size:11px; color:#facc15">🎯 Asistente de Ajuste Rápido con Probeta (Opcional)</b>
        <div style="display:grid; grid-template-columns:1fr 1fr 1fr; gap:6px; margin-top:6px;">
          <div>
            <label style="font-size:9px; color:var(--muted)">RPM Ensayo</label>
            <input type="number" id="cal_rpm" value="50.0" style="width:100%; background:#0a0f1e; border:1px solid #263450; color:#38bdf8; padding:5px; border-radius:6px; font-size:11px;">
          </div>
          <div>
            <label style="font-size:9px; color:var(--muted)">Q Alim (mL/min)</label>
            <input type="number" id="cal_q_alim" value="720.0" style="width:100%; background:#0a0f1e; border:1px solid #263450; color:var(--alim); padding:5px; border-radius:6px; font-size:11px;">
          </div>
          <div>
            <label style="font-size:9px; color:var(--muted)">Q Perm (0=omitir)</label>
            <input type="number" id="cal_q_perm" value="0.0" style="width:100%; background:#0a0f1e; border:1px solid #263450; color:var(--perm); padding:5px; border-radius:6px; font-size:11px;">
          </div>
        </div>
        <button style="background:#4f46e5; color:#fff; width:100%; padding:7px; font-weight:800; font-size:11px; border-radius:6px; margin-top:6px" onclick="calibrarPorRpmYQ()">
          🚀 Recalcular Factores y Guardar en Flash
        </button>
      </div>
    </div>

    <!-- CARD 4: DATALOGGER Y DESCARGA A EXCEL -->
    <div class="card">
      <div class="row">
        <b style="font-size:12px; color:#fff">📊 REGISTRO DE MERMA DE CAUDAL & EXPORTACIÓN EXCEL (.CSV)</b>
        <button style="font-size:10px; padding:3px 8px; background:#475569" onclick="clearLog()">🗑️ Limpiar</button>
      </div>

      <div style="background:#0a0f1e; border:1px solid #1a253c; border-radius:10px; padding:12px; margin-top:8px">
        <div class="row" style="margin-bottom:6px">
          <span>Ensayo en curso:</span>
          <span><b id="ensayo_actual_lbl" style="color:var(--primary)">Ensayo #1</b> (<span id="t_log">00:00</span> min)</span>
        </div>
        <div class="row" style="margin-bottom:8px">
          <span>Muestras guardadas (cada 10s):</span>
          <span><b id="n_logs" style="color:#f8fafc">0</b> filas (incluye % Merma y J/J₀)</span>
        </div>

        <div class="input-group" style="margin-bottom:8px">
          <label style="color:var(--muted)">Seleccionar Ensayo para Descargar:</label>
          <select id="sel_ensayo" style="background:#131c31; border:1px solid #293855; color:#fff; padding:8px; border-radius:6px; font-size:11px">
            <option value="all">📦 Todos los Ensayos (Histórico Completo)</option>
            <option value="actual">🔴 Ensayo Actual en Vivo</option>
          </select>
        </div>

        <button class="btn-excel" onclick="descargarExcel()">
          📥 Descargar Curva de Merma y Caudales en Excel (.CSV)
        </button>
      </div>

      <div class="row" style="margin-top:10px; margin-bottom:2px">
        <span style="font-weight:700; color:#cbd5e1">Resumen de Ensayos Finalizados:</span>
        <span id="cant_ensayos" style="color:var(--muted)">0 ensayos</span>
      </div>
      <div class="table-box">
        <table id="tabla_ensayos">
          <thead>
            <tr>
              <th>ID</th>
              <th>RPM</th>
              <th>Duración</th>
              <th>Merma Alim</th>
              <th>Merma Perm</th>
              <th>CSV</th>
            </tr>
          </thead>
          <tbody id="body_ensayos">
            <tr><td colspan="6" style="text-align:center; color:var(--muted)">Sin ensayos finalizados aún.</td></tr>
          </tbody>
        </table>
      </div>
    </div>

    <footer>
      ESP32 UF UNSa • <b id="ip">192.168.4.1</b> • <a href="http://bomba.local" style="color:var(--primary);text-decoration:none">http://bomba.local</a>
    </footer>
  </div>

  <script>
    const $ = id => document.getElementById(id);
    let devOpen = false;
    let rawStatus = {};
    let lastStableFa = 0;
    let lastStableFp = 0;
    let soloAlimActivo = false;
    let switchesOpen = false;
    let modoTempAuto = true;

    function toggleSwitches() {
      switchesOpen = !switchesOpen;
      $('card_switches').style.display = switchesOpen ? 'block' : 'none';
      if (switchesOpen) {
        $('card_switches').scrollIntoView({ behavior: 'smooth' });
      }
    }

    function setTempModo(auto) {
      modoTempAuto = auto;
      cmd(auto ? 'TEMP_AUTO_ON' : 'TEMP_AUTO_OFF');
      actualizarEstiloTemp();
      if (auto) {
        showToast("🌡️ Sonda DS18B20 en D4 activada: actualización automática de temperatura en vivo.");
      } else {
        showToast("✏️ Modo manual: escribe la temperatura en °C y haz clic en Aplicar.");
      }
    }

    function actualizarEstiloTemp() {
      const bAuto = $('btn_temp_auto');
      const bMan = $('btn_temp_man');
      if (bAuto && bMan) {
        if (modoTempAuto) {
          bAuto.style.background = '#059669';
          bAuto.style.border = '2px solid #34d399';
          bAuto.style.boxShadow = '0 0 10px rgba(52, 211, 153, 0.6)';
          bAuto.style.color = '#fff';
          bAuto.style.fontWeight = '800';
          bMan.style.background = '#1e293b';
          bMan.style.border = '1px solid #334155';
          bMan.style.boxShadow = 'none';
          bMan.style.color = '#94a3b8';
          bMan.style.fontWeight = 'normal';
        } else {
          bMan.style.background = '#d97706';
          bMan.style.border = '2px solid #fbbf24';
          bMan.style.boxShadow = '0 0 10px rgba(251, 191, 36, 0.6)';
          bMan.style.color = '#fff';
          bMan.style.fontWeight = '800';
          bAuto.style.background = '#1e293b';
          bAuto.style.border = '1px solid #334155';
          bAuto.style.boxShadow = 'none';
          bAuto.style.color = '#94a3b8';
          bAuto.style.fontWeight = 'normal';
        }
      }
      if ($('inp_temp')) {
        $('inp_temp').readOnly = modoTempAuto;
        $('inp_temp').style.opacity = modoTempAuto ? '0.75' : '1.0';
      }
    }

    // Historial en vivo para el gráfico de formación de torta (últimos 120 puntos)
    let histT = [];
    let histQA = [];
    let histQP = [];
    let lastChartPushMs = 0;

    function toggleDev() {
      devOpen = !devOpen;
      $('card_dev').style.display = devOpen ? 'block' : 'none';
    }

    function showToast(msg, isError = false) {
      const t = $('toast_msg');
      t.innerText = msg;
      t.style.borderColor = isError ? '#ef4444' : '#a855f7';
      t.style.background = isError ? '#3f1111' : '#1e1b4b';
      t.style.display = 'block';
      setTimeout(() => { t.style.display = 'none'; }, 4500);
    }

    function updateRpmHighlight(v) {
      document.querySelectorAll('.rpm-btn').forEach(btn => {
        const isMatch = Math.abs(parseFloat(btn.dataset.rpm) - parseFloat(v)) < 0.1;
        btn.style.border = isMatch ? '2px solid #60a5fa' : '1px solid #334155';
        btn.style.background = isMatch ? '#2563eb' : '#1e293b';
        btn.style.boxShadow = isMatch ? '0 0 12px rgba(37, 99, 235, 0.6)' : 'none';
        btn.style.color = isMatch ? '#ffffff' : '#94a3b8';
        btn.style.fontWeight = isMatch ? '900' : 'normal';
      });
    }

    function cmd(act) { fetch('/cmd?act=' + act); }
    function setR(v) {
      $('sl').value = v;
      $('lc').innerText = v;
      $('obj_rpm_lbl').innerText = v;
      updateRpmHighlight(v);
      fetch('/set?rpm=' + v);
    }

    function fijarQ0() {
      cmd('FIJAR_Q0');
      histT = []; histQA = []; histQP = [];
    }

    function fijarPermManual() {
      const v = parseFloat($('inp_qp_man').value);
      if (isNaN(v) || v < 0) {
        alert("Ingresa un caudal de permeado en mL/min (ej. 8.5)");
        return;
      }
      fetch('/set_perm_manual?qp=' + v);
    }

    function volverPermAuto() {
      $('inp_qp_man').value = '';
      cmd('PERM_AUTO');
    }

    function actualizarPresion() {
      const p1 = parseFloat($('inp_p1').value);
      const p2 = parseFloat($('inp_p2').value);
      const temp = parseFloat($('inp_temp').value);
      if (isNaN(p1) || isNaN(p2) || isNaN(temp)) {
        alert("Ingresa valores válidos de presión y temperatura.");
        return;
      }
      fetch(`/set_presion?p1=${p1}&p2=${p2}&temp=${temp}`)
        .then(r => r.json())
        .then(res => {
          if (res.status === 'ok') {
            showToast(`✅ Presión registrada: TMP=${res.tmp} bar | J20=${res.j20} LMH`);
          }
        });
    }

    function calibrarPorRpmYQ() {
      const rpm = parseFloat($('cal_rpm').value);
      const qa = parseFloat($('cal_q_alim').value);
      let qp = parseFloat($('cal_q_perm').value);
      if (isNaN(qp) || qp < 0) qp = 0;

      if (isNaN(rpm) || isNaN(qa) || rpm <= 0 || qa <= 0) {
        alert("Ingresa RPM y Caudal de Alimentación válidos mayores a cero.");
        return;
      }

      let fa = (rawStatus && rawStatus.f_alim > 1.0) ? rawStatus.f_alim : lastStableFa;
      let fp = (rawStatus && rawStatus.f_perm > 0.35) ? rawStatus.f_perm : lastStableFp;

      let url = `/calibrar_rpm_q?rpm=${rpm}&qa=${qa}`;
      if (fa > 1.0) url += `&fa=${fa.toFixed(2)}`;
      if (qp > 0) {
        url += `&qp=${qp}`;
        if (fp > 0.35) url += `&fp=${fp.toFixed(2)}`;
      }

      fetch(url)
        .then(r => r.json())
        .then(res => {
          if (res.status === 'error') {
            showToast("❌ Error: " + (res.msg || "No se pudo calibrar"), true);
            return;
          }
          let msg = `🎯 Guardado: Cilindrada=${res.ml_rev} mL/rev | K_Alim=${res.k_alim}`;
          if (res.kp_actualizado) msg += ` | K_Perm=${res.k_perm}`;
          showToast(msg);
        });
    }

    function guardarParametros(guardarNvs) {
      const ka = parseFloat($('inp_ka').value);
      const kp = parseFloat($('inp_kp').value);
      const ml = parseFloat($('inp_ml').value);
      const pul = parseInt($('inp_pul').value);

      if (isNaN(ka) || isNaN(kp) || isNaN(ml) || isNaN(pul)) {
        alert("Por favor ingresa números válidos.");
        return;
      }

      fetch(`/set_dev?ka=${ka}&kp=${kp}&ml=${ml}&pul=${pul}&save=${guardarNvs}`)
        .then(r => r.json())
        .then(res => {
          if (res.status === 'error') {
            showToast("❌ Error: " + (res.msg || "Parámetros rechazados"), true);
            return;
          }
          showToast(guardarNvs ? "✅ Factores guardados en memoria Flash ESP32 (NVS)!" : "⚡ Parámetros aplicados en caliente (RAM).");
        });
    }

    function descargarExcel() {
      const sel = $('sel_ensayo').value;
      window.location.href = '/export_csv?ensayo=' + sel;
    }

    function descargarEnsayoId(id) {
      window.location.href = '/export_csv?ensayo=' + id;
    }

    function toggleSoloAlim() {
      let nuevo = !soloAlimActivo;
      cmd(nuevo ? 'SOLO_ALIM_ON' : 'SOLO_ALIM_OFF');
    }

    function adjVal(id, delta) {
      let el = $(id);
      let v = parseFloat(el.value) || 0;
      v = Math.max(10.0, Math.round((v + delta) * 10) / 10);
      el.value = v.toFixed(2);
      guardarParametros(0);
    }

    function clearLog() {
      if (confirm("¿Deseas reiniciar el historial de ensayos y el gráfico?")) {
        histT = []; histQA = []; histQP = [];
        fetch('/clear_csv').then(() => {
          $('n_logs').innerText = "0";
          $('t_log').innerText = "00:00";
        });
      }
    }

    function drawChart(qAlimRef, qPermRef) {
      const canvas = $('chart_torta');
      if (!canvas) return;
      const ctx = canvas.getContext('2d');
      const W = canvas.width, H = canvas.height;
      ctx.clearRect(0, 0, W, H);

      // Rejilla de fondo
      ctx.strokeStyle = '#162038';
      ctx.lineWidth = 1;
      for (let i = 1; i <= 3; i++) {
        let y = (H / 4) * i;
        ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(W, y); ctx.stroke();
      }

      if (histQA.length < 2) {
        ctx.fillStyle = '#64748b';
        ctx.font = '11px sans-serif';
        ctx.fillText('Enciende la bomba para graficar la curva de caudal en tiempo real...', 65, H / 2);
        return;
      }

      let maxA = Math.max(100, qAlimRef || 0, ...histQA) * 1.1;
      let maxP = Math.max(20, qPermRef || 0, ...histQP) * 1.2;

      // Línea punteada de referencia Q_Alim_0
      if (qAlimRef > 10) {
        let yRefA = H - (qAlimRef / maxA) * (H - 16) - 8;
        ctx.strokeStyle = 'rgba(6, 182, 212, 0.35)';
        ctx.setLineDash([4, 4]);
        ctx.beginPath(); ctx.moveTo(0, yRefA); ctx.lineTo(W, yRefA); ctx.stroke();
        ctx.setLineDash([]);
      }

      // Curva Q_Alim(t)
      ctx.strokeStyle = '#06b6d4';
      ctx.lineWidth = 2.2;
      ctx.beginPath();
      histQA.forEach((v, i) => {
        let x = (i / (histQA.length - 1)) * (W - 12) + 6;
        let y = H - (v / maxA) * (H - 16) - 8;
        if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
      });
      ctx.stroke();

      // Curva Q_Perm(t)
      ctx.strokeStyle = '#facc15';
      ctx.lineWidth = 2.2;
      ctx.beginPath();
      histQP.forEach((v, i) => {
        let x = (i / (histQP.length - 1)) * (W - 12) + 6;
        let y = H - (v / maxP) * (H - 16) - 8;
        if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
      });
      ctx.stroke();
    }

    $('sl').oninput = e => {
      $('lc').innerText = e.target.value;
      $('obj_rpm_lbl').innerText = e.target.value;
      updateRpmHighlight(e.target.value);
    };
    $('sl').onchange = e => setR(e.target.value);

    let pollBusy = false;
    setInterval(() => {
      if (pollBusy) return;
      pollBusy = true;
      fetch('/status')
        .then(r => r.json())
        .then(d => {
          pollBusy = false;
          rawStatus = d;
          $('rpm').innerText = d.rpm.toFixed(1);
          $('obj_rpm_lbl').innerText = d.obj_rpm.toFixed(0);
          updateRpmHighlight(d.obj_rpm);
          $('ip').innerText = d.ip;
          $('pil').className = 'pill ' + (d.inv ? 'inv' : d.on ? 'on' : 'off');
          $('pil').innerText = d.inv ? 'INVIRTIENDO' : d.on ? '🟢 EN MARCHA' : 'DETENIDA';

          // Cambio dinámico de color en botones de Bomba (Marcha / Parada / Sentido)
          const bStart = $('btn_start');
          const bStop = $('btn_stop');
          if (bStart && bStop) {
            if (d.on) {
              bStart.style.background = '#16a34a';
              bStart.style.border = '2px solid #4ade80';
              bStart.style.boxShadow = '0 0 14px rgba(74, 222, 128, 0.6)';
              bStart.style.color = '#fff';
              bStart.innerText = '🟢 EN MARCHA';
              bStop.style.background = '#3b1212';
              bStop.style.border = '1px solid #7f1d1d';
              bStop.style.boxShadow = 'none';
              bStop.style.color = '#fca5a5';
              bStop.innerText = '⏹ PARAR';
            } else {
              bStart.style.background = '#064e3b';
              bStart.style.border = '1px solid #047857';
              bStart.style.boxShadow = 'none';
              bStart.style.color = '#86efac';
              bStart.innerText = '▶ ARRANCAR';
              bStop.style.background = '#dc2626';
              bStop.style.border = '2px solid #f87171';
              bStop.style.boxShadow = '0 0 12px rgba(220, 38, 38, 0.6)';
              bStop.style.color = '#fff';
              bStop.innerText = '⏹ DETENIDA';
            }
          }

          if ($('bd')) {
            $('bd').innerText = d.dir ? '🔄 HORARIO (FILTRACIÓN)' : '🔄 ANTIHORARIO (RETROLAVADO)';
            $('bd').style.background = d.dir ? '#0284c7' : '#d97706';
            $('bd').style.border = d.dir ? '2px solid #38bdf8' : '2px solid #fbbf24';
            $('bd').style.boxShadow = d.dir ? '0 0 10px rgba(56, 189, 248, 0.4)' : '0 0 10px rgba(251, 191, 36, 0.4)';
            $('bd').style.color = '#fff';
          }

          $('al').classList.toggle('vis', d.cruce || d.q_alim > 1600);
          $('ba').classList.toggle('vis', !d.alim_ok);
          $('bp').classList.toggle('vis', !d.perm_ok);

          soloAlimActivo = !!d.solo_alim;
          const bsa = $('btn_solo_alim');
          if (bsa) {
            if (soloAlimActivo) {
              bsa.innerText = '🔒 Perm Bloqueado (ACTIVO)';
              bsa.style.background = '#0284c7';
              bsa.style.border = '2px solid #38bdf8';
              bsa.style.boxShadow = '0 0 10px rgba(56, 189, 248, 0.5)';
              bsa.style.color = '#fff';
            } else {
              bsa.innerText = '🔒 Solo Alim (OFF)';
              bsa.style.background = '#1e293b';
              bsa.style.border = '1px solid #475569';
              bsa.style.boxShadow = 'none';
              bsa.style.color = '#94a3b8';
            }
          }

          if ($('lbl_estado_perm')) {
            if (soloAlimActivo) {
              $('lbl_estado_perm').innerText = '🔒 Bloqueado (Solo Alim)';
              $('lbl_estado_perm').style.color = '#38bdf8';
            } else if (d.perm_manual) {
              $('lbl_estado_perm').innerText = '🧪 Asistido por Probeta (' + d.q_perm_man.toFixed(1) + ' mL/min)';
              $('lbl_estado_perm').style.color = '#facc15';
            } else {
              $('lbl_estado_perm').innerText = '🟢 Sensor en Tiempo Real';
              $('lbl_estado_perm').style.color = '#34d399';
            }
          }
          $('btn_perm_auto').style.display = d.perm_manual ? 'inline-block' : 'none';

          $('va').innerText = d.q_alim.toFixed(1);
          $('fa').innerText = d.f_alim.toFixed(1) + ' Hz';
          $('la').innerText = d.vol_alim.toFixed(3) + ' L';
          $('vp').innerText = d.q_perm.toFixed(1);
          $('fp').innerText = d.f_perm.toFixed(1) + ' Hz';
          $('lp').innerText = d.vol_perm.toFixed(3) + ' L';

          $('qa_ref').innerText = d.q_alim_ref > 1 ? d.q_alim_ref.toFixed(1) : '--';
          $('qp_ref').innerText = d.q_perm_ref > 0.1 ? d.q_perm_ref.toFixed(1) : '--';

          // Indicadores de Merma de Caudal por Formación de Torta
          $('merma_a').innerText = d.merma_alim.toFixed(1) + ' %';
          let caidaMlA = (d.q_alim_ref > 1 && d.on) ? Math.max(0, d.q_alim_ref - d.q_alim) : 0;
          $('merma_a_det').innerText = `Caída vs Q₀: −${caidaMlA.toFixed(1)} mL/min`;

          $('merma_p').innerText = d.merma_perm.toFixed(1) + ' %';
          $('merma_p_det').innerText = `Flujo Relativo J/J₀: ${d.j_j0.toFixed(1)} %`;

          if ($('val_a')) $('val_a').innerText = d.val_alim || 0;
          if ($('gl_a'))  $('gl_a').innerText  = d.gl_alim || 0;
          if ($('val_p')) $('val_p').innerText = d.val_perm || 0;
          if ($('gl_p'))  $('gl_p').innerText  = d.gl_perm || 0;

          if (d.f_alim && d.f_alim > 1.0) lastStableFa = d.f_alim;
          if (d.f_perm && d.f_perm > 0.35) lastStableFp = d.f_perm;
          if ($('j_lmh')) $('j_lmh').innerText = (d.j_lmh || 0).toFixed(2) + ' LMH';
          $('qr').innerText = d.q_ret.toFixed(1) + ' mL/min';
          $('rv').innerText = d.recov.toFixed(1) + ' %';
          $('qb').innerText = d.pump_ml.toFixed(1) + ' mL/min';
          $('db').innerText = (d.delta > 0 ? '+' : '') + d.delta.toFixed(1) + ' %';

          $('lbl_ka').innerText = d.k_alim.toFixed(2);

          // Indicadores de Presión Manométrica y Darcy en Tiempo Real (Subhito 2.4)
          if ($('disp_tmp')) $('disp_tmp').innerText = (d.tmp !== undefined) ? d.tmp.toFixed(3) : '--';
          if ($('disp_j20')) $('disp_j20').innerText = (d.j20 !== undefined) ? d.j20.toFixed(2) : '--';
          if ($('disp_tcf')) $('disp_tcf').innerText = (d.tcf !== undefined) ? d.tcf.toFixed(3) : '1.000';
          if ($('disp_rtorta')) $('disp_rtorta').innerText = (d.r_torta !== undefined) ? d.r_torta.toExponential(2) : '--';
          if ($('disp_rtot')) $('disp_rtot').innerText = (d.r_tot !== undefined) ? d.r_tot.toExponential(2) + ' m⁻¹' : '--';
          if ($('pill_darcy')) {
            $('pill_darcy').className = 'pill ' + (d.darcy_ok ? 'on' : 'off');
            $('pill_darcy').innerText = d.darcy_ok ? 'DARCY ACTIVO' : 'SIN TMP / PERM';
          }

          // Actualización de temperatura y sensor DS18B20 en GPIO 4
          if (d.temp_auto !== undefined && !$('inp_temp').dataset.manTouched) {
            modoTempAuto = !!d.temp_auto;
            actualizarEstiloTemp();
          }
          if ($('val_temp_live')) {
            $('val_temp_live').innerText = (d.temp !== undefined) ? d.temp.toFixed(1) : '--';
          }
          if ($('badge_sensor_temp')) {
            if (d.ds18b20) {
              $('badge_sensor_temp').innerText = '🟢 SONDA D4 ACTIVA';
              $('badge_sensor_temp').style.background = '#065f46';
              $('badge_sensor_temp').style.color = '#34d399';
            } else {
              $('badge_sensor_temp').innerText = '⚪ SONDA NO DETECTADA';
              $('badge_sensor_temp').style.background = '#3f1d1d';
              $('badge_sensor_temp').style.color = '#f87171';
            }
          }

          if (!document.activeElement || (document.activeElement.id !== 'inp_p1' && document.activeElement.id !== 'inp_p2' && document.activeElement.id !== 'inp_temp')) {
            if ($('inp_p1') && d.p1 !== undefined && !$('inp_p1').dataset.touched) $('inp_p1').value = d.p1.toFixed(2);
            if ($('inp_p2') && d.p2 !== undefined && !$('inp_p2').dataset.touched) $('inp_p2').value = d.p2.toFixed(2);
            if ($('inp_temp') && d.temp !== undefined && (modoTempAuto || !$('inp_temp').dataset.touched)) $('inp_temp').value = d.temp.toFixed(1);
          }

          // Actualizar gráfico en tiempo real cada 2 segundos mientras la bomba está encendida
          let nowMs = Date.now();
          if (d.on && (nowMs - lastChartPushMs >= 2000)) {
            lastChartPushMs = nowMs;
            histT.push(d.t_ensayo_s);
            histQA.push(d.q_alim);
            histQP.push(d.q_perm);
            if (histT.length > 120) {
              histT.shift(); histQA.shift(); histQP.shift();
            }
            $('chart_pts_lbl').innerText = d.t_ensayo_s + ' s en vivo';
          }
          drawChart(d.q_alim_ref, d.q_perm_ref);

          if (!document.activeElement || document.activeElement.tagName !== 'INPUT') {
            $('inp_ka').value = d.k_alim.toFixed(2);
            $('inp_kp').value = d.k_perm.toFixed(2);
            $('inp_ml').value = d.ml_rev.toFixed(2);
            $('inp_pul').value = d.pul_rev;
          }

          $('n_logs').innerText = d.n_logs;
          $('ensayo_actual_lbl').innerText = 'Ensayo #' + d.ensayo_act;

          let segTot = d.t_ensayo_s;
          let mins = Math.floor(segTot / 60);
          let secs = segTot % 60;
          $('t_log').innerText = (mins < 10 ? '0' : '') + mins + ':' + (secs < 10 ? '0' : '') + secs;

          $('cant_ensayos').innerText = d.ensayos.length + ' ensayos';

          const sel = $('sel_ensayo');
          const currentVal = sel.value;
          let optHtml = '<option value="all">📦 Todos los Ensayos (Histórico Completo)</option>';
          optHtml += '<option value="actual">🔴 Ensayo Actual en Vivo (#' + d.ensayo_act + ')</option>';
          d.ensayos.forEach(e => {
            optHtml += `<option value="${e.id}">Ensayo #${e.id}: ${e.rpm.toFixed(0)} RPM (Merma Alim: ${e.merma_a.toFixed(1)}%, Perm: ${e.merma_p.toFixed(1)}%)</option>`;
          });
          if (sel.options.length !== d.ensayos.length + 2) {
            sel.innerHTML = optHtml;
            sel.value = currentVal;
          }

          const tbody = $('body_ensayos');
          if (d.ensayos.length === 0) {
            tbody.innerHTML = '<tr><td colspan="6" style="text-align:center; color:var(--muted)">Sin ensayos finalizados aún.</td></tr>';
          } else {
            let trHtml = '';
            d.ensayos.slice().reverse().forEach(e => {
              let m = Math.floor(e.duracion_s / 60);
              let s = e.duracion_s % 60;
              let timeStr = (m < 10 ? '0' : '') + m + ':' + (s < 10 ? '0' : '') + s;
              trHtml += `<tr>
                <td><b>#${e.id}</b></td>
                <td>${e.rpm.toFixed(0)}</td>
                <td>${timeStr}</td>
                <td style="color:var(--alim)">−${e.merma_a.toFixed(1)}%</td>
                <td style="color:var(--perm)">−${e.merma_p.toFixed(1)}%</td>
                <td><button style="padding:2px 6px; font-size:9px; background:#107c41" onclick="descargarEnsayoId(${e.id})">📥 CSV</button></td>
              </tr>`;
            });
            tbody.innerHTML = trHtml;
          }
        })
        .catch(() => { pollBusy = false; });
    }, 600);
  </script>
</body>
</html>
)html";
