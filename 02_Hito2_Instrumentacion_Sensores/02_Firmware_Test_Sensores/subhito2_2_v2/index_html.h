#pragma once
const char INDEX_HTML[] PROGMEM = R"html(<!DOCTYPE html>
<html lang="es"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Planta UF</title><style>
body{font-family:system-ui,sans-serif;background:#0b1020;color:#e2e8f0;margin:0;padding:14px;display:flex;justify-content:center}
.wrap{max-width:460px;width:100%}
.card{background:#141c30;border:1px solid #263450;border-radius:14px;padding:16px;margin-bottom:12px}
h1{font-size:15px;color:#38bdf8;margin:0 0 4px;text-align:center}
.sub{font-size:11px;color:#8296b3;text-align:center;margin:0 0 12px}
.disp{text-align:center;background:#0a0f1e;border-radius:10px;padding:10px;margin-bottom:10px}
.rpm{font-size:44px;font-weight:800;color:#38bdf8;line-height:1}
u{display:block;font-size:10px;color:#8296b3;letter-spacing:2px;text-decoration:none}
.pill{display:inline-block;padding:3px 12px;border-radius:99px;font-size:10px;font-weight:700;margin-top:6px}
.off{background:#2a3448;color:#aab7cc}.on{background:rgba(16,185,129,.25);color:#34d399}.inv{background:rgba(245,158,11,.25);color:#fbbf24}
.al{display:none;background:rgba(239,68,68,.15);border:1px solid #ef4444;color:#fca5a5;border-radius:8px;padding:7px;font-size:11px;text-align:center;margin-bottom:10px}
.vis{display:block}
.row{display:flex;justify-content:space-between;font-size:11px;color:#8296b3;margin-bottom:4px}
input[type=range]{width:100%;accent-color:#38bdf8}
.grid{display:grid;grid-template-columns:repeat(5,1fr);gap:5px;margin:10px 0}
button{border:none;border-radius:8px;padding:8px 4px;font-weight:700;font-size:11px;cursor:pointer;background:#22304e;color:#e2e8f0}
.btns{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.go{background:#10b981;color:#04301c}.no{background:#ef4444;color:#3d0808}
.dir{grid-column:span 2;background:#1c2a45}
.sens{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.s{background:#0a0f1e;border-radius:10px;padding:10px}
.s h3{font-size:10px;margin:0 0 4px}
.s .v{font-size:22px;font-weight:800}
.s .d{font-size:10px;color:#8296b3}
.badge{display:none;color:#ef4444;font-size:10px}
.badge.vis{display:inline}
.bal{background:#0a0f1e;border-radius:10px;padding:10px;margin-top:10px;font-size:11px}
.bal div{display:flex;justify-content:space-between;padding:3px 0}
.lbl{color:#8296b3}.val{font-weight:700}
footer{text-align:center;font-size:10px;color:#8296b3;padding:6px}
</style></head><body><div class="wrap">

<div class="card">
<h1>BOMBA PERISTÁLTICA MBP-2000</h1>
<p class="sub">Rango 72–140 RPM • Membrana FX100 ≤ 0.60 L/min</p>
<div class="disp"><div class="rpm" id="rpm">0.0</div><u>RPM INSTANTÁNEA</u>
<div class="pill off" id="pil">DETENIDA</div></div>
<div class="al" id="al">⚠ Caudal cercano al límite de membrana (0.60 L/min)</div>
<div class="row"><span>Consigna: <b id="lc" style="color:#38bdf8">72</b> RPM</span></div>
<input type="range" id="sl" min="72" max="140" step="1" value="72">
<div class="grid">
<button onclick="setR(72)">72</button><button onclick="setR(80)">80</button><button onclick="setR(100)">100</button>
<button onclick="setR(120)">120</button><button onclick="setR(140)">140</button></div>
<div class="btns">
<button class="go" onclick="cmd('START')">▶ ARRANCAR</button>
<button class="no" onclick="cmd('STOP')">⏹ PARAR</button>
<button class="dir" id="bd" onclick="cmd('DIR')">🔄 HORARIO (FILTRACIÓN)</button></div>
</div>

<div class="card">
<div class="row"><b style="font-size:12px">🌊 CAUDALÍMETROS</b>
<button style="font-size:10px;padding:4px 8px" onclick="cmd('RESET_VOL')">Reset L</button></div>
<div class="sens">
<div class="s"><h3 style="color:#0ea5e9">FEED <span class="badge" id="bf">SIN SEÑAL</span></h3>
<div class="v" id="vf" style="color:#0ea5e9">0.0</div>
<div class="d" id="ff">0.0 Hz</div><div class="d" id="lf">0.000 L</div></div>
<div class="s"><h3 style="color:#a855f7">PERMEADO <span class="badge" id="bp">SIN SEÑAL</span></h3>
<div class="v" id="vp" style="color:#a855f7">0.0</div>
<div class="d" id="fp">0.0 Hz</div><div class="d" id="lp">0.000 L</div></div>
</div>
<div class="bal">
<div><span class="lbl">Retentado (Feed−Perm):</span><span class="val" id="qr" style="color:#f59e0b">0.0 mL/min</span></div>
<div><span class="lbl">Recuperación (Y):</span><span class="val" id="rv" style="color:#38bdf8">0.0 %</span></div>
<div><span class="lbl">Teórico bomba:</span><span class="val" id="qb">0.0 mL/min</span></div>
<div><span class="lbl">Δ Bomba vs Feed:</span><span class="val" id="db">0.0 %</span></div>
</div>
</div>

<footer>ESP32: <b id="ip">...</b> • http://bomba.local</footer>
</div>
<script>
const $=id=>document.getElementById(id);
function cmd(a){fetch('/cmd?act='+a)}
function setR(v){$('sl').value=v;$('lc').innerText=v;fetch('/set?rpm='+v)}
 $('sl').oninput=e=>$('lc').innerText=e.target.value;
 $('sl').onchange=e=>setR(e.target.value);
setInterval(()=>fetch('/status').then(r=>r.json()).then(d=>{
 $('rpm').innerText=d.rpm.toFixed(1);$('ip').innerText=d.ip;
 $('pil').className='pill '+(d.inv?'inv':d.on?'on':'off');
 $('pil').innerText=d.inv?'INVIRTIENDO':d.on?'EN MARCHA':'DETENIDA';
 $('bd').innerText=d.dir?'🔄 HORARIO (FILTRACIÓN)':'🔄 ANTIHORARIO (RETROLAVADO)';
 $('al').classList.toggle('vis',d.q_feed>600);
 $('bf').classList.toggle('vis',!d.feed_ok);
 $('bp').classList.toggle('vis',!d.perm_ok);
 $('vf').innerText=d.q_feed.toFixed(1);$('ff').innerText=d.f_feed.toFixed(1)+' Hz';
 $('lf').innerText=d.vol_feed.toFixed(3)+' L';
 $('vp').innerText=d.q_perm.toFixed(1);$('fp').innerText=d.f_perm.toFixed(1)+' Hz';
 $('lp').innerText=d.vol_perm.toFixed(3)+' L';
 $('qr').innerText=d.q_ret.toFixed(1)+' mL/min';$('rv').innerText=d.recov.toFixed(1)+' %';
 $('qb').innerText=d.pump_ml.toFixed(1)+' mL/min';
 $('db').innerText=(d.delta>0?'+':'')+d.delta.toFixed(1)+' %';
}).catch(()=>{}),500);
</script></body></html>)html";