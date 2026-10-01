#include "index_html.h"

// ==============================================================================
// DEFINICIÓN DEL CONTENIDO HTML / CSS / JS (PROGMEM)
// Al estar en un archivo .cpp separado, GCC compila este archivo una sola vez
// generando el objeto index_html.cpp.o, acelerando drásticamente la compilación.
// ==============================================================================

const char INDEX_HTML[] PROGMEM = R"html(<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Planta UF - SCADA, Auto-Calibración & Modo Dev</title>
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
      --autocal: #0ea5e9;
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
    .wrap { max-width: 530px; width: 100%; display: flex; flex-direction: column; gap: 12px; }
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
    .rpm { font-size: 46px; font-weight: 800; color: var(--primary); line-height: 1; }
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
    .row { display: flex; justify-content: space-between; font-size: 11px; color: var(--muted); margin-bottom: 6px; align-items: center; }
    input[type=range] { width: 100%; accent-color: var(--primary); cursor: pointer; }
    .grid { display: grid; grid-template-columns: repeat(5,1fr); gap: 6px; margin: 10px 0; }
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
    .btn-excel:hover { background: #0d6535; }
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

    /* Sensores dispuestos uno abajo de otro */
    .sens {
      display: flex;
      flex-direction: column;
      gap: 10px;
      margin-top: 8px;
    }
    .s {
      background: #0a0f1e;
      border-radius: 10px;
      padding: 12px 14px;
      border: 1px solid #1a253c;
    }
    .s h3 { font-size: 12px; margin-bottom: 6px; display: flex; justify-content: space-between; align-items: center; }
    .s .val-row { display: flex; justify-content: space-between; align-items: baseline; }
    .s .v { font-size: 26px; font-weight: 800; }
    .s .info-row { display: flex; justify-content: space-between; font-size: 11px; color: var(--muted); margin-top: 4px; }
    .badge { display: none; color: var(--danger); font-size: 9px; font-weight: bold; }
    
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
    tr:hover { background: #17233d; }

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
      box-shadow: 0 4px 15px rgba(168,85,247,0.3);
    }

    /* Progress bar for auto-cal */
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
      <div class="row" style="margin-bottom:4px">
        <h1 style="text-align:left; margin-bottom:0">PLANTA UF • MBP-2000</h1>
        <button class="btn-dev" onclick="toggleDev()">🛠️ Modo Dev</button>
      </div>
      <p class="sub" style="text-align:left">Rampa Fluida S-Curve • Despegue Suave & Parada Rápida</p>
      
      <div class="disp">
        <div class="rpm" id="rpm">0.0</div>
        <u>RPM INSTANTÁNEA (OBJETIVO: <span id="obj_rpm_lbl" style="color:var(--primary)">50</span> RPM)</u>
        <div class="pill off" id="pil">DETENIDA</div>
      </div>

      <div class="al" id="al">⚠ ALERTA: Caudal Alimentación excede límite (> 1200 mL/min)</div>

      <div class="row">
        <span>Consigna de Operación:</span>
        <span><b id="lc" style="color:var(--primary)">50</b> RPM</span>
      </div>
      <input type="range" id="sl" min="15" max="75" step="1" value="50">
      
      <div class="grid">
        <button onclick="setR(20)">20 RPM</button>
        <button onclick="setR(35)">35 RPM</button>
        <button onclick="setR(50)" style="border: 1px solid var(--primary); font-weight: 900;">50 RPM ⭐</button>
        <button onclick="setR(60)">60 RPM</button>
        <button onclick="setR(72)">72 RPM</button>
      </div>

      <div class="btns">
        <button class="go" onclick="cmd('START')">▶ ARRANCAR</button>
        <button class="no" onclick="cmd('STOP')">⏹ PARAR</button>
        <button class="dir" id="bd" onclick="cmd('DIR')">🔄 HORARIO (FILTRACIÓN)</button>
      </div>
    </div>

    <!-- CARD 2: INSTRUMENTACIÓN Y BALANCE HIDRÁULICO (UNO ABAJO DE OTRO) -->
    <div class="card">
      <div class="row">
        <b style="font-size:12px; color:#fff">🌊 CAUDALÍMETROS YF-S401</b>
        <button style="font-size:10px; padding:3px 8px" onclick="cmd('RESET_VOL')">Reset Volúmenes</button>
      </div>

      <div class="sens">
        <!-- 1. Sensor Alimentacion (Arriba) -->
        <div class="s" style="border-left: 4px solid var(--alim);">
          <h3 style="color:var(--alim)">
            <span>ALIMENTACIÓN (GPIO 14)</span>
            <span class="badge" id="ba">SIN SEÑAL</span>
          </h3>
          <div class="val-row">
            <div>
              <span class="v" id="va" style="color:var(--alim)">0.0</span>
              <small style="font-size:12px; font-weight:700; color:var(--muted)">mL/min</small>
            </div>
            <div style="font-size:14px; font-weight:700; color:#e2e8f0" id="la">0.000 L</div>
          </div>
          <div class="info-row">
            <span>Frecuencia: <b id="fa" style="color:#e2e8f0">0.0 Hz</b></span>
            <span>Factor K: <b id="lbl_ka" style="color:var(--alim)">154.62</b> pulsos/L</span>
          </div>
        </div>

        <!-- 2. Sensor Permeado (Abajo) -->
        <div class="s" style="border-left: 4px solid var(--perm);">
          <h3 style="color:var(--perm)">
            <span>PERMEADO (GPIO 27)</span>
            <span class="badge" id="bp">SIN SEÑAL</span>
          </h3>
          <div class="val-row">
            <div>
              <span class="v" id="vp" style="color:var(--perm)">0.0</span>
              <small style="font-size:12px; font-weight:700; color:var(--muted)">mL/min</small>
            </div>
            <div style="font-size:14px; font-weight:700; color:#e2e8f0" id="lp">0.000 L</div>
          </div>
          <div class="info-row">
            <span>Frecuencia: <b id="fp" style="color:#e2e8f0">0.0 Hz</b></span>
            <span>Factor K: <b id="lbl_kp" style="color:var(--perm)">55.00</b> pulsos/L</span>
          </div>
        </div>
      </div>

      <!-- Balance Hidráulico -->
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
          <span class="lbl">Caudal Teórico Bomba:</span>
          <span class="val" id="qb">0.0 mL/min</span>
        </div>
        <div>
          <span class="lbl">Desviación (Δ Bomba vs Alimentación):</span>
          <span class="val" id="db">0.0 %</span>
        </div>
      </div>
    </div>

    <!-- CARD 3: MODO DESARROLLADOR (CIERRA AL GUARDAR) -->
    <div class="card card-dev" id="card_dev" style="display:none">
      <div class="row">
        <b style="font-size:12px; color:var(--dev)">🛠️ CALIBRACIÓN EN CALIENTE Y AUTO-TUNING</b>
        <button style="background:#334155; font-size:10px; padding:2px 8px" onclick="toggleDev()">✕ Cerrar</button>
      </div>
      <div class="toast" id="toast_msg"></div>

      <!-- SECCIÓN A: AUTO-CALIBRACIÓN INTELIGENTE EN MARCHA -->
      <div style="background:#0e1122; padding:10px; border-radius:8px; border:1px solid #293855; margin-bottom:10px">
        <div class="row" style="margin-bottom:2px">
          <b style="font-size:11px; color:#38bdf8">⚡ AUTO-CALIBRACIÓN EN RÉGIMEN PERMANENTE</b>
          <span id="regimen_badge" style="font-size:9px; color:#aab7cc">Esperando estabilidad...</span>
        </div>
        <p style="font-size:10px; color:var(--muted)">Calcula automáticamente los factores K de ambos sensores durante 15s estables a las RPM actuales.</p>
        
        <div id="autocal_progress_box" style="display:none">
          <div class="progress-bar-bg">
            <div class="progress-bar-fill" id="autocal_fill"></div>
            <span class="progress-text" id="autocal_txt">Muestreando... 0%</span>
          </div>
        </div>

        <div style="display:flex; gap:6px; margin-top:6px">
          <button class="btn-autocal" id="btn_start_autocal" onclick="iniciarAutoCal()">
            ▶ Iniciar Auto-Calibración Automática (15s)
          </button>
          <button id="btn_cancel_autocal" style="display:none; background:var(--danger); color:#fff; font-size:10px; padding:6px" onclick="cancelarAutoCal()">
            ✖ Cancelar
          </button>
        </div>
      </div>

      <!-- SECCIÓN B: CALIBRACIÓN EXACTA POR RPM Y CAUDALES MEDIDOS -->
      <div style="background:#0e1122; padding:10px; border-radius:8px; border:1px solid #293855; margin-bottom:10px">
        <b style="font-size:11px; color:#facc15">🎯 CALIBRAR POR RPM & CAUDAL (PROBETA)</b>
        <p style="font-size:10px; color:var(--muted); margin-bottom:6px">Ingresa las RPM y los caudales reales medidos para recalcular cilindrada y K simultáneamente:</p>
        
        <div class="input-grid" style="grid-template-columns: 1fr 1fr 1fr;">
          <div class="input-group">
            <label style="font-size:10px">RPM Ensayo:</label>
            <input type="number" id="cal_rpm" value="50.0" step="1">
          </div>
          <div class="input-group">
            <label style="font-size:10px">Q Alim (mL/min):</label>
            <input type="number" id="cal_q_alim" value="680.0" step="5">
          </div>
          <div class="input-group">
            <label style="font-size:10px">Q Perm (mL/min):</label>
            <input type="number" id="cal_q_perm" value="100.0" step="5">
          </div>
        </div>

        <button style="background:#4f46e5; color:#fff; width:100%; padding:8px; font-weight:800; margin-top:2px" onclick="calibrarPorRpmYQ()">
          🚀 Calcular y Guardar Calibración en ESP32
        </button>
      </div>

      <!-- SECCIÓN C: EDICIÓN DIRECTA DE PARÁMETROS -->
      <div class="input-grid">
        <div class="input-group">
          <label>Factor K Alimentación:</label>
          <input type="number" id="inp_ka" step="0.01" value="154.62">
        </div>
        <div class="input-group">
          <label>Factor K PERMEADO:</label>
          <input type="number" id="inp_kp" step="0.01" value="55.00">
        </div>
        <div class="input-group">
          <label>Cilindrada Bomba (mL/rev):</label>
          <input type="number" id="inp_ml" step="0.01" value="13.60">
        </div>
        <div class="input-group">
          <label>Micropasos (Pulsos/Rev):</label>
          <input type="number" id="inp_pul" step="100" value="3200">
        </div>
      </div>

      <div class="btns" style="margin-top:6px">
        <button style="background:var(--dev); color:#fff" onclick="guardarParametros(1)">💾 Guardar en Flash ESP32</button>
        <button style="background:#334155" onclick="guardarParametros(0)">⚡ Aplicar Temporal</button>
        <button style="background:#1e293b; grid-column:span 2; border:1px solid #475569" onclick="resetParametros()">🔄 Restablecer Valores de Fábrica</button>
      </div>
    </div>

    <!-- CARD 4: DATALOGGER Y DESCARGA MULTI-SESIÓN A EXCEL -->
    <div class="card">
      <div class="row">
        <b style="font-size:12px; color:#fff">📊 HISTORIAL DE ENSAYOS & EXCEL (.CSV)</b>
        <button style="font-size:10px; padding:3px 8px; background:#475569" onclick="clearLog()">🗑️ Limpiar Todo</button>
      </div>
      
      <div style="background:#0a0f1e; border:1px solid #1a253c; border-radius:10px; padding:12px; margin-top:8px">
        <div class="row" style="margin-bottom:6px">
          <span>Ensayo actual en curso:</span>
          <span><b id="ensayo_actual_lbl" style="color:var(--primary)">Ensayo #1</b> (<span id="t_log">00:00</span> min)</span>
        </div>
        <div class="row" style="margin-bottom:8px">
          <span>Muestras registradas:</span>
          <span><b id="n_logs" style="color:#f8fafc">0</b> muestras guardadas</span>
        </div>

        <div class="input-group" style="margin-bottom:8px">
          <label style="color:var(--muted)">Seleccionar Ensayo para Descargar:</label>
          <select id="sel_ensayo" style="background:#131c31; border:1px solid #293855; color:#fff; padding:8px; border-radius:6px; font-size:11px">
            <option value="all">📦 Todos los Ensayos (Histórico Completo)</option>
            <option value="actual">🔴 Ensayo Actual en Vivo</option>
          </select>
        </div>

        <button class="btn-excel" onclick="descargarExcel()">
          📥 Descargar Ensayo Seleccionado en Excel (.CSV)
        </button>
      </div>

      <!-- Tabla de ensayos registrados al apagar la bomba -->
      <div class="row" style="margin-top:10px; margin-bottom:2px">
        <span style="font-weight:700; color:#cbd5e1">Corridas Registradas al Apagar Bomba:</span>
        <span id="cant_ensayos" style="color:var(--muted)">0 ensayos</span>
      </div>
      <div class="table-box">
        <table id="tabla_ensayos">
          <thead>
            <tr>
              <th>ID</th>
              <th>Consigna</th>
              <th>Duración</th>
              <th>Muestras</th>
              <th>Vol Perm</th>
              <th>Acción</th>
            </tr>
          </thead>
          <tbody id="body_ensayos">
            <tr><td colspan="6" style="text-align:center; color:var(--muted)">No hay ensayos finalizados aún. Arranca y apaga la bomba para registrar.</td></tr>
          </tbody>
        </table>
      </div>
    </div>

    <footer>
      ESP32 UF • <b id="ip">Cargando...</b> • <a href="http://bomba.local" style="color:var(--primary);text-decoration:none">http://bomba.local</a>
    </footer>
  </div>

  <script>
    const $ = id => document.getElementById(id);
    let devOpen = false;
    let rawStatus = {};

    function toggleDev() {
      devOpen = !devOpen;
      $('card_dev').style.display = devOpen ? 'block' : 'none';
    }

    function showToast(msg) {
      const t = $('toast_msg');
      t.innerText = msg;
      t.style.display = 'block';
      setTimeout(() => { t.style.display = 'none'; }, 4000);
    }

    function cmd(act) { fetch('/cmd?act=' + act); }
    function setR(v) {
      $('sl').value = v;
      $('lc').innerText = v;
      $('obj_rpm_lbl').innerText = v;
      fetch('/set?rpm=' + v);
    }

    function iniciarAutoCal() {
      if (!rawStatus.on) {
        alert("Primero enciende la bomba y deja que alcance las RPM consignadas.");
        return;
      }
      fetch('/iniciar_auto_cal')
        .then(r => r.json())
        .then(res => {
          showToast("⚡ Rutina de Auto-Calibración iniciada. Manteniendo 15s de régimen permanente...");
        });
    }

    function cancelarAutoCal() {
      fetch('/cancelar_auto_cal')
        .then(r => r.json())
        .then(() => showToast("Auto-calibración cancelada."));
    }

    function calibrarPorRpmYQ() {
      const rpm = parseFloat($('cal_rpm').value);
      const qa = parseFloat($('cal_q_alim').value);
      const qp = parseFloat($('cal_q_perm').value);

      if (isNaN(rpm) || isNaN(qa) || isNaN(qp) || rpm <= 0 || qa <= 0 || qp <= 0) {
        alert("Ingresa valores numéricos válidos mayores a cero.");
        return;
      }

      fetch(`/calibrar_rpm_q?rpm=${rpm}&qa=${qa}&qp=${qp}`)
        .then(r => r.json())
        .then(res => {
          showToast(`🎯 Calibración completada: Cilindrada=${res.ml_rev} mL/rev | K_Alim=${res.k_alim} | K_Perm=${res.k_perm}`);
          // Cerrar automáticamente el Modo Dev al guardar
          setTimeout(() => {
            devOpen = false;
            $('card_dev').style.display = 'none';
          }, 1200);
        })
        .catch(() => alert("Error al comunicar con ESP32"));
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
          showToast(guardarNvs ? "✅ Factores guardados en memoria Flash ESP32 (NVS)!" : "⚡ Parámetros aplicados en caliente (RAM).");
          // Cerrar automáticamente el Modo Dev al guardar en Flash
          if (guardarNvs) {
            setTimeout(() => {
              devOpen = false;
              $('card_dev').style.display = 'none';
            }, 1000);
          }
        })
        .catch(() => alert("Error al comunicarse con el ESP32"));
    }

    function resetParametros() {
      if (confirm("¿Restablecer factores K y calibración a los valores de fábrica?")) {
        fetch('/reset_dev')
          .then(r => r.json())
          .then(res => {
            showToast("🔄 Factores de fábrica restablecidos con éxito.");
            setTimeout(() => {
              devOpen = false;
              $('card_dev').style.display = 'none';
            }, 1000);
          });
      }
    }

    function descargarExcel() {
      const sel = $('sel_ensayo').value;
      window.location.href = '/export_csv?ensayo=' + sel;
    }

    function descargarEnsayoId(id) {
      window.location.href = '/export_csv?ensayo=' + id;
    }

    function clearLog() {
      if (confirm("¿Deseas reiniciar todo el historial de ensayos y registros?")) {
        fetch('/clear_csv').then(() => {
          $('n_logs').innerText = "0";
          $('t_log').innerText = "00:00";
        });
      }
    }

    $('sl').oninput = e => {
      $('lc').innerText = e.target.value;
      $('obj_rpm_lbl').innerText = e.target.value;
    };
    $('sl').onchange = e => setR(e.target.value);

    // Bucle de telemetría cada 500 ms
    setInterval(() => {
      fetch('/status')
        .then(r => r.json())
        .then(d => {
          rawStatus = d;
          $('rpm').innerText = d.rpm.toFixed(1);
          $('obj_rpm_lbl').innerText = d.obj_rpm.toFixed(0);
          $('ip').innerText = d.ip;
          $('pil').className = 'pill ' + (d.inv ? 'inv' : d.on ? 'on' : 'off');
          $('pil').innerText = d.inv ? 'INVIRTIENDO' : d.on ? 'EN MARCHA' : 'DETENIDA';
          $('bd').innerText = d.dir ? '🔄 HORARIO (FILTRACIÓN)' : '🔄 ANTIHORARIO (RETROLAVADO)';
          $('al').classList.toggle('vis', d.q_alim > 1200);
          $('ba').classList.toggle('vis', !d.alim_ok);
          $('bp').classList.toggle('vis', !d.perm_ok);
          $('va').innerText = d.q_alim.toFixed(1);
          $('fa').innerText = d.f_alim.toFixed(1) + ' Hz';
          $('la').innerText = d.vol_alim.toFixed(3) + ' L';
          $('vp').innerText = d.q_perm.toFixed(1);
          $('fp').innerText = d.f_perm.toFixed(1) + ' Hz';
          $('lp').innerText = d.vol_perm.toFixed(3) + ' L';
          $('qr').innerText = d.q_ret.toFixed(1) + ' mL/min';
          $('rv').innerText = d.recov.toFixed(1) + ' %';
          $('qb').innerText = d.pump_ml.toFixed(1) + ' mL/min';
          $('db').innerText = (d.delta > 0 ? '+' : '') + d.delta.toFixed(1) + ' %';
          
          $('lbl_ka').innerText = d.k_alim.toFixed(2);
          $('lbl_kp').innerText = d.k_perm.toFixed(2);
          
          // Badge de régimen
          $('regimen_badge').innerText = d.en_regimen ? "🟢 Régimen Permanente Estable" : "🟡 Transición / Ajustando";
          $('regimen_badge').style.color = d.en_regimen ? "#34d399" : "#fbbf24";

          // Auto-Calibración en curso
          if (d.auto_cal) {
            $('autocal_progress_box').style.display = 'block';
            $('btn_cancel_autocal').style.display = 'block';
            $('btn_start_autocal').innerText = '⏳ Auto-Calibrando...';
            $('autocal_fill').style.width = d.auto_cal_prog + '%';
            $('autocal_txt').innerText = `Muestreando régimen... ${d.auto_cal_prog}%`;
          } else {
            $('autocal_progress_box').style.display = 'none';
            $('btn_cancel_autocal').style.display = 'none';
            $('btn_start_autocal').innerText = '▶ Iniciar Auto-Calibración Automática (15s)';
            if (d.auto_cal_res && d.auto_cal_res.length > 0) {
              showToast(d.auto_cal_res);
            }
          }

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

          // Renderizar tabla y dropdown de ensayos
          $('cant_ensayos').innerText = d.ensayos.length + ' ensayos';
          
          const sel = $('sel_ensayo');
          const currentVal = sel.value;
          let optHtml = '<option value="all">📦 Todos los Ensayos (Histórico Completo)</option>';
          optHtml += '<option value="actual">🔴 Ensayo Actual en Vivo (#' + d.ensayo_act + ')</option>';
          d.ensayos.forEach(e => {
            optHtml += `<option value="${e.id}">Ensayo #${e.id}: ${e.rpm.toFixed(1)} RPM (${e.muestras} m, ${e.vol_perm.toFixed(3)} L perm)</option>`;
          });
          if (sel.options.length !== d.ensayos.length + 2) {
            sel.innerHTML = optHtml;
            sel.value = currentVal;
          }

          const tbody = $('body_ensayos');
          if (d.ensayos.length === 0) {
            tbody.innerHTML = '<tr><td colspan="6" style="text-align:center; color:var(--muted)">No hay ensayos finalizados aún. Arranca y apaga la bomba para registrar.</td></tr>';
          } else {
            let trHtml = '';
            d.ensayos.slice().reverse().forEach(e => {
              let m = Math.floor(e.duracion_s / 60);
              let s = e.duracion_s % 60;
              let timeStr = (m < 10 ? '0' : '') + m + ':' + (s < 10 ? '0' : '') + s;
              trHtml += `<tr>
                <td><b>#${e.id}</b></td>
                <td>${e.rpm.toFixed(1)} RPM</td>
                <td>${timeStr}</td>
                <td>${e.muestras}</td>
                <td style="color:var(--perm)">${e.vol_perm.toFixed(3)} L</td>
                <td><button style="padding:2px 6px; font-size:9px; background:#107c41" onclick="descargarEnsayoId(${e.id})">📥 CSV</button></td>
              </tr>`;
            });
            tbody.innerHTML = trHtml;
          }
        })
        .catch(() => {});
    }, 500);
  </script>
</body>
</html>
)html";
