# 🧠 PROMPT MAESTRO DE AUDITORÍA TÉCNICA EXTERNA — RONDA 3 (CIERRE DE ARQUITECTURA)
**Destinatario:** Claude 5.5 Opus (Auditor Técnico y Científico Senior en Sistemas Embebidos y Procesos de Separación por Membrana)  
**Proyecto:** Planta Piloto de Ultrafiltración por Flujo Cruzado & Reactor de Coagulación-Sedimentación (Tesis de Grado y Tesis Doctoral)  
**Institución:** Universidad Nacional de Salta (UNSa), Facultad de Ingeniería — Salta, Argentina  
**Equipo:** Antonella Guitián & Owen Cañizares (Tesistas de Grado), Ing. Enzo (Codirector / Investigador Doctoral), Antigravity AI (Asistente de Arquitectura)  

---

## 🎯 INSTRUCCIONES DE CONTEXTO PARA CLAUDE 5.5 OPUS

Actúa como un **Ingeniero Principal de Sistemas Embebidos de Tiempo Real (FreeRTOS / ESP32)** y simultáneamente como un **Catedrático e Investigador Senior en Procesos de Separación por Membranas (Ultrafiltración Capilar / Fenómenos de Transporte de Darcy)**.

Estamos en el **cierre de la fase de diseño y auditoría técnica** de una planta piloto de ultrafiltración industrial construida para una tesis de grado y ensayos de doctorado. En las Rondas 1 y 2 participaron ChatGPT 6 Astra, GLM 5.3 y Claude Sonnet 5. Sus observaciones permitieron corregir errores conceptuales graves y blindar el sistema. 

Ahora recurrimos a **Claude 5.5 Opus** como la **autoridad técnica definitiva** para:
1. Validar el consenso alcanzado y evaluar si quedan **puntos ciegos (*blind spots*)**, riesgos ocultos de hardware o inconsistencias físicas/matemáticas.
2. Auditar el firmware templado (`v4`), la arquitectura concurrente FreeRTOS propuesta (`planta_rt`) y el algoritmo del caudalímetro recíproco.
3. Evaluar la rigurosidad científica y factibilidad del protocolo experimental de Darcy, flujo crítico ($J_c$) y la matriz factorial de ensayos con membrana capilar Fresenius FX100.
4. Emitir un **Dictamen de Certificación Final** con recomendaciones concretas previas a la puesta en marcha de los ensayos de laboratorio.

---

## 🏭 1. FICHA TÉCNICA DEL HARDWARE Y BANCO DE ENSAYOS

```
                                 ESQUEMA DEL BANCO PILOTO
  
   [Tanque Alimentación] ──► [Bomba Peristáltica] ──► [Transductor P1] ──► [Membrana FX100] ──┬─► [Transductor P2] ──► [Válvula Aguja] ──► [Retorno]
      (Agua Turbia /            (NEMA 34 + DM860)       (0 - 1.0 bar)         (Lumen 13500 fib) │    (0 - 1.0 bar)      (Regula TMP)
       Sobrenadante)                   │                                                        │
                                [Válvula Alivio]                                                └─► [Carcasa Permeado] ──► [Transductor P3] ──► [YF-S401] ──► [Permeado]
                                (1.0 - 1.2 bar)                                                                             (0 - 1.0 bar)       (K=55.0)
```

1. **Impulsión y Potencia:**
   - **Motor:** Paso a paso NEMA 34 (Torque: **$4.5\text{ Nm}$**, corriente nominal: $4.0\text{ A}$).
   - **Driver:** Leadshine DM860 configurado a **3200 pulsos/rev** (16 micropasos en Cátodo Común). Frecuencias de pulso: $800\text{ Hz}$ (15 RPM) a $5333\text{ Hz}$ (100 RPM).
   - **Cabezal Peristáltico:** MBP-2000 con 3 rodillos, manguera de silicona de $12\text{ mm}$ de diámetro interno.
   - **Desplazamiento Volumétrico Real:** **$13.6\text{ mL/vuelta}$** (calibrado gravimétricamente con probeta a 50 y 72 RPM).
   - **Rango Operativo:** $15.0\text{ a }100.0\text{ RPM}$ ($\approx 200\text{ a }1360\text{ mL/min}$).
   - **Alarma Clínica de Membrana:** $44.0\text{ RPM}$ ($\approx 600\text{ mL/min}$ — límite estándar para sangre). La operación hasta 100 RPM está reservada para agua en el diseño factorial de la tesis.

2. **Instrumentación y Acondicionamiento (ESP32 NodeMCU-32S, 240 MHz):**
   - **Caudalímetros:** Turbinas de microflujo Efecto Hall YF-S401 en alimentación ($K_{\text{alim}} = 154.62\text{ pul/L}$) y permeado ($K_{\text{perm}} = 55.00\text{ pul/L}$).
   - **Front-End RC:** Bornera intermedia con resistencias pull-up externas de **$4.7\text{ k}\Omega$ a 3.3V** + filtro pasabajos RC con capacitor cerámico de **$100\text{ nF}$** ($f_c \approx 338\text{ Hz}$), eliminando ruidos de conmutación chopper y acoples de 50/100 Hz.
   - **Conversor ADC:** ADS1115 de 16 bits en bus I2C (`0x48`, SDA: GPIO 21, SCL: GPIO 22 a 400 kHz).
   - **Transductores de Presión:** Tres transductores piezoeléctricos ratiométricos ($0.5 - 4.5\text{ V}$) con rango especificado de **0 a 1.0 bar (0 a 100 kPa)** con rosca $G 1/4"$. Divisores resistivos de precisión ($10\text{ k}\Omega / 20\text{ k}\Omega$ al $0.1\%$) para escalar $0.5 - 4.5\text{ V} \rightarrow 0.33 - 3.00\text{ V}$ antes del ADS1115.
   - **Temperatura:** Sonda sumergible DS18B20 OneWire en GPIO 4 con pull-up de $4.7\text{ k}\Omega$.
   - **Seguridad Mecánica:** Boya de nivel en acero inoxidable (GPIO 32, pull-up + 100 nF).

3. **Membrana de Ultrafiltración Capilar:**
   - **Modelo:** Fresenius Helixone FX100 (Polisulfona / PVP modificada).
   - **Área Efectiva ($A_m$):** **$2.2\text{ m}^2$**.
   - **Geometría Capilar:** $N \approx 13500\text{ fibras}$ huecas, diámetro interno $d_i = 185\ \mu\text{m}$ ($r_i = 92.5\ \mu\text{m}$), espesor de pared $\delta = 35\ \mu\text{m}$, longitud efectiva $L \approx 0.28\text{ m}$.
   - **Coeficiente de Ultrafiltración Nominal ($K_{\text{UF}}$):** $73\text{ mL}/(\text{h}\cdot\text{mmHg}) \approx 5475\text{ mL}/(\text{h}\cdot\text{bar})$.
   - **Presión Máxima Recomendada:** $\text{TMP}_{\text{máx}} = 0.50\text{ bar}$ ($50\text{ kPa} \approx 375\text{ mmHg}$).

---

## 📜 2. EVOLUCIÓN HISTÓRICA Y CONSENSOS DE LAS RONDAS 1 Y 2

### 2.1. De la Versión en Banco (v3) a la Versión Blindada (v4)
* **`subhito2_2_v3` (En Operación Actual):** Validó en laboratorio la auto-calibración en marcha con probeta, persistencia Flash NVS (`Preferences.h`), Web SCADA en SoftAP puro (`192.168.4.1`) y filtrado anti-ruido a $2000\ \mu\text{s}$.
* **`subhito2_2_v4` (Hardened con Aportes de Auditoría):**
  - **Cinemática de Bomba:** Resuelto el bug de bloqueo en inversión (`_rpmGuardada`), rampa S-Curve, validación de finitud (`std::isfinite`), desaceleración controlada a 0 RPM y corte de pulsos antes de conmutar `PIN_DIR`.
  - **Caudalímetros por Período Recíproco:** Sustitución de la cuantización por conteo de pulsos por medición recíproca con protección atómica por instancia (`portMUX_TYPE`), resolución $< 0.1\%$ a 5.5 Hz, y sustracción modular de 32 bits inmune al desborde de `micros()`.
  - **Módulo Darcy Embebido (`darcy.h`):** Ecuación de Vogel para $\mu(T)$, factor de corrección de temperatura TCF, cálculo en vivo de flujo $J$ y $J_{20}$ en LMH, acumulación para regresión de $R_m$.
  - **Cero Fragmentación de Heap:** Eliminación de objetos `String` dinámicos en endpoints JSON mediante buffers estáticos de stack (`snprintf`) y streaming HTTP por fragmentos (`CONTENT_LENGTH_UNKNOWN`).

### 2.2. Consensos Técnicos y Directrices Críticas de la Ronda 2 (Astra, GLM 5.3 y Claude Sonnet 5)
1. **Seguridad Física ante Desplazamiento Positivo:** A $4.5\text{ Nm}$, una bomba peristáltica no se detiene por contrapresión: ocluir la línea reventaría capilares y tubos. Se acordó la instalación obligatoria de una **válvula de alivio mecánica ($1.0 - 1.2\text{ bar}$)** en la impulsión y un **hongo de parada de emergencia (E-Stop)** que corte la etapa de potencia del DM860.
2. **Doble Mecanismo de Parada en Software:** `detener()` con rampa suave ($45\text{ RPM/s}$) para uso normal y `paradaDura()` instantánea ($< 50\text{ ms}$, corte de pulsos sin rampa) para sobrepresión ($P_1 > 1.0\text{ bar}$ o $\text{TMP} > 0.45\text{ bar}$) o pulsador de emergencia.
3. **Techo Dinámico de RPM (`_techo`):** Autolimitación a 44 RPM a menos que los transductores de presión estén instalados, calibrados y con telemetría viva ($< 1.5\text{ s}$).
4. **Veredicto Unánime ADS1115 Single-Ended:** Canales A0 ($P_{\text{alim}}$), A1 ($P_{\text{ret}}$), A2 ($P_{\text{perm}}$) y A3 (TDS) con palabra de configuración `(4 + ch) << 12` (`0xC383, 0xD383, 0xE383, 0xF383`). Permite evaluar $\Delta P_{\text{lumen}} = P_1 - P_2$ y cavitación en succión.
5. **Alerta de Compras de Sensores:** Obligatorio transductores de **0 a 1.0 bar** (o máx. 1.2 bar). Sensores comerciales de 6 o 12 bar introducen un error absoluto de $\pm 60\text{ a }120\text{ mbar}$ ($\pm 1\%$ FS), falseando la TMP de ensayo ($0.10 - 0.40\text{ bar}$) con errores relativos inaceptables del 24% al 48%.
6. **Cota Geodésica y Tara Hidrostática:** Los 3 transductores a la misma altura física ($10\text{ cm}$ de diferencia = $10\text{ mbar} \approx 20\%$ de error en TMP baja). Tara a cero guardada en NVS con bomba parada y circuito lleno de agua.
7. **FreeRTOS Dual-Core Determinista:** `tareaControl` en Core 1 a **Prioridad 19** (por encima de lwIP Wi-Fi de prioridad 18), lazo a 50 ms fijos con `vTaskDelayUntil()`. Servidor Web en Core 0 (prioridad 2). Snapshot plano (`SnapshotPlanta_t` $\le 300\text{ B}$) copiado en $\approx 1\ \mu\text{s}$ con spinlock liviano `portMUX`. Paradas (STOP/ESTOP) por banderas atómicas booleanas desacopladas de la cola de consignas.
8. **Algoritmo de Caudalímetro Mejorado:** Referencia temporal persistente entre ventanas (`_tRef`), cota física continua en ausencia de pulsos ($f \le 10^6 / \Delta t_{\text{sin\_pulso}}$) y alarma de falla por falta de pulsos exclusiva para alimentación (en permeado el caudal cero es legítimo).
9. **Clarificación Mecánica de Retrolavado:** Invertir la bomba solo invierte el flujo luminal o vacía la línea. Un retrolavado real (*backwash*) a través de los microporos capilares requiere inyectar agua pura desde el puerto de permeado a baja presión ($\le 0.2\text{ bar}$).

---

## 💻 3. CÓDIGO Y ESTRUCTURAS PROPUESTAS PARA REVISIÓN

### 3.1. Caudalímetro con Conteo Recíproco Continuo (`caudalimetro.cpp`)
```cpp
// Variables miembro:
volatile uint32_t _pulsos = 0, _t_primero = 0, _t_ultimo = 0;
uint32_t _tRef = 0;
bool     _refValida = false;
uint32_t _timeoutUs = 2000000; // 2s para alimentación, 5s para permeado
float    _f = 0.0f, _q = 0.0f, _vol = 0.0f;
portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;

// ISR con filtro de 1500 us (soporta hasta 1360 mL/min ~210 Hz con margen 3x)
void IRAM_ATTR Caudalimetro::isrPuente(void* arg) {
    auto* c = static_cast<Caudalimetro*>(arg);
    const uint32_t t = micros();
    if ((uint32_t)(t - c->_t_ultimo) < FILTRO_RUIDO_US) return;
    portENTER_CRITICAL_ISR(&c->_mux);
    if (c->_pulsos == 0) c->_t_primero = t;
    c->_t_ultimo = t;
    c->_pulsos++;
    portEXIT_CRITICAL_ISR(&c->_mux);
}

// Bucle de actualización (1 Hz)
void Caudalimetro::actualizar(bool bombaEmpuja, bool esPermeado) {
    uint32_t n, tPrim, tUlt;
    portENTER_CRITICAL(&_mux);
    n = _pulsos; tPrim = _t_primero; tUlt = _t_ultimo; _pulsos = 0;
    portEXIT_CRITICAL(&_mux);
    const uint32_t ahora = micros();

    if (n > 0) {
        // Sustracciones modulares exactas a través del wrap de 32 bits de micros()
        const uint32_t periodos = _refValida ? n : (n > 1 ? n - 1 : 1);
        const uint32_t span     = _refValida ? (tUlt - _tRef) : (tUlt - tPrim);
        bool valido = true;
        if (periodos > 0 && span > 0) {
            const float f = (float)periodos * 1.0e6f / (float)span;
            if (f * 1000.0f / _k <= Q_MAX_FISICO_MLMIN) _f = f;
            else valido = false;
        }
        if (valido) _vol += (float)n / (_k * 60.0f);
        _tRef = tUlt; _refValida = true;
    } else if (_refValida) {
        const uint32_t sinPulso = ahora - _tRef;
        if (sinPulso > _timeoutUs) { 
            _f = 0.0f; _refValida = false; 
        } else if (_f > 0.0f) {
            // Cota física continua: f no puede ser mayor a 1 / tiempo_sin_flanco
            const float cota = 1.0e6f / (float)sinPulso;
            if (cota < _f) _f = cota;
        }
    } else {
        _f = 0.0f;
    }

    const float q = _f * 1000.0f / _k;
    _q += 0.5f * (q - _q); // Filtro EMA liviano

    // Alarma de falta de pulsos exclusiva para alimentación
    if (!esPermeado) {
        if (n > 0) { _segSinPulso = 0; _fallo = false; }
        else if (bombaEmpuja) { if (++_segSinPulso >= 5) _fallo = true; }
        else { _segSinPulso = 0; _fallo = false; }
    } else {
        _fallo = false; // Permeado en 0 es una condición de proceso normal
    }
}
```

### 3.2. Módulo de Darcy Embebido (`darcy.h`)
```cpp
// Ecuación de Vogel para viscosidad dinámica del agua (Pa*s)
inline float viscosidadAguaVogel(float tempC) {
    float T_kelvin = tempC + 273.15f;
    if (T_kelvin < 273.15f) T_kelvin = 273.15f;
    float exponente = 247.8f / (T_kelvin - 140.0f);
    return 0.00002414f * powf(10.0f, exponente);
}

// Factor de Corrección Térmica TCF respecto a 20 °C
inline float factorTCF(float tempC) {
    const float mu20 = 0.001002f; // Pa*s a 20 °C
    float muT = viscosidadAguaVogel(tempC);
    return muT / mu20;
}

// Cálculo instantáneo de flujo y resistencias
struct DarcyResult {
    float J_LMH;       // Flujo volumétrico real (L / m^2 * h)
    float J20_LMH;     // Flujo normalizado a 20 °C
    float R_total_m1;  // Resistencia hidráulica total (m^-1)
    float R_foul_m1;   // Resistencia de colmatación (R_total - Rm)
    bool valido;
};

inline DarcyResult calcularDarcy(float qPerm_mLmin, float tmp_bar, float tempC, float Rm_m1, bool sensorTempOk) {
    DarcyResult res = {0};
    if (tmp_bar < 0.01f || qPerm_mLmin <= 0.0f || !sensorTempOk) {
        res.valido = false;
        return res;
    }
    float Q_Lh = (qPerm_mLmin * 60.0f) / 1000.0f;
    res.J_LMH = Q_Lh / AREA_MEMBRANA_M2; // 2.2 m^2
    res.J20_LMH = res.J_LMH * factorTCF(tempC);

    float tmp_Pa = tmp_bar * 100000.0f;
    float J_ms = (res.J_LMH / 1000.0f) / 3600.0f;
    float mu = viscosidadAguaVogel(tempC);

    if (J_ms > 1e-9f) {
        res.R_total_m1 = tmp_Pa / (mu * J_ms);
        res.R_foul_m1 = (res.R_total_m1 > Rm_m1) ? (res.R_total_m1 - Rm_m1) : 0.0f;
        res.valido = true;
    }
    return res;
}
```

### 3.3. Arquitectura FreeRTOS Determinista Propuesta (`planta_rt.h / cpp`)
```cpp
// Estructuras de sincronización POD (Plain Old Data)
struct SnapshotPlanta_t {
    float rpmActual, rpmObjetivo;
    float qFeed_mLmin, qPerm_mLmin;
    float p1_bar, p2_bar, p3_bar, p1_pico_bar, tmp_bar;
    float temp_C, tds_ppm;
    float J_LMH, J20_LMH, R_total, R_foul;
    uint32_t volFeed_mL, volPerm_mL;
    uint32_t ts_presion_ms;
    uint8_t flagsEstado; // bit 0: marcha, bit 1: alarma_tmp, bit 2: boya, bit 3: i2c_ok
    uint32_t t_ms;
};
static_assert(std::is_trivially_copyable<SnapshotPlanta_t>::value, "Snapshot debe ser POD");
static_assert(sizeof(SnapshotPlanta_t) <= 320, "Snapshot excede tamaño seguro para spinlock");

// Banderas atómicas y colas
static std::atomic<bool> g_stop{false}, g_estop{false};
static QueueHandle_t     g_colaComandos;
static SnapshotPlanta_t  g_snapshot{};
static portMUX_TYPE      g_snapMux = portMUX_INITIALIZER_UNLOCKED;

// Tarea de Control (Core 1, Prioridad 19 - Determinismo de 50 ms)
void tareaControl(void* pv) {
    TickType_t t0 = xTaskGetTickCount();
    uint32_t ciclo = 0;
    SnapshotPlanta_t sLocal{};

    for (;;) {
        vTaskDelayUntil(&t0, pdMS_TO_TICKS(50)); // 20 Hz estricto

        // 1. Evaluación inmediata de banderas atómicas de parada
        if (g_estop.load(std::memory_order_relaxed)) {
            g_estop.store(false);
            bomba.paradaDura(); // Corte instantáneo de pulsos PUL
        }
        if (g_stop.load(std::memory_order_relaxed)) {
            g_stop.store(false);
            bomba.detener(); // Frenado suave por rampa
        }

        // 2. Procesamiento de consignas desde la cola
        ComandoOperador_t cmd;
        while (xQueueReceive(g_colaComandos, &cmd, 0) == pdTRUE) {
            switch (cmd.tipo) {
                case CMD_ARRANCAR: bomba.arrancar(); break;
                case CMD_SET_RPM:  bomba.setRPM(cmd.valor); break;
                case CMD_INVERTIR: bomba.toggleSentido(); break;
            }
        }

        // 3. Cinemática del motor y FSM
        bomba.tick(0.050f);

        // 4. Lazo de 1 Hz para instrumentación y Darcy
        if (++ciclo % 20 == 0) {
            sensorFeed.actualizar(bomba.enMarcha(), false);
            sensorPerm.actualizar(bomba.enMarcha(), true);
            // Lectura FSM ADS1115 y cálculo de Darcy
            // Enclavamiento: si TMP > 0.45 bar -> bomba.paradaDura()
        }

        // 5. Publicación atómica de Snapshot (copia en ~1 us)
        portENTER_CRITICAL(&g_snapMux);
        g_snapshot = sLocal;
        portEXIT_CRITICAL(&g_snapMux);
    }
}

// Tarea Web / Red (Core 0, Prioridad 2)
void tareaWeb(void* pv) {
    server.begin();
    for (;;) {
        server.handleClient();
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}
```

---

## 🔬 4. PROTOCOLO EXPERIMENTAL DE ULTRAFILTRACIÓN (HITO 5)

### 4.1. Espacio Operativo No Rectangular $(Q, \text{TMP})$
El cartucho Fresenius FX100 cuenta con 13500 capilares de $185\ \mu\text{m}$ ID.
* A caudal máximo ($1360\text{ mL/min} \approx 100\text{ RPM}$), la caída de presión axial por fricción es:
  $$\Delta P_{\text{lumen}} = \frac{128 \cdot \mu \cdot L \cdot Q_{\text{feed}}}{\pi \cdot N \cdot d_i^4} \approx 0.16\text{ bar}$$
  Por lo tanto, la $\text{TMP}$ mínima físicamente alcanzable con permeado a presión atmosférica ($P_3 = 0$) es $\text{TMP}_{\text{mín}} \approx \frac{\Delta P_{\text{lumen}}}{2} \approx 0.08\text{ bar}$. Una consigna inferior a 0.08 bar generaría backfiltración (permeado entrando al retentado).
* A caudal bajo ($200\text{ mL/min} \approx 15\text{ RPM}$), una TMP de $0.40\text{ bar}$ exigiría un caudal de permeado teórico de $365\text{ mL/min}$, superando el 100% de la alimentación.

### 4.2. Fases Experimentales de Tesis
* **Fase 0 (Línea Base Diaria de Membrana Limpia $R_{m,0}$):** 5 escalones de TMP ($0.05, 0.10, 0.15, 0.20, 0.25\text{ bar}$) con agua destilada fresca. Regresión lineal $J_{20}$ vs TMP. Requisito: $R^2 \ge 0.985$. Desacopla el envejecimiento de la membrana.
* **Fase 1 (Flujo Crítico $J_c$ por Método Escalonado con Histéresis):**
  - Rampa de TMP ascendente ($0.05 \rightarrow 0.40\text{ bar}$, pasos de $0.05\text{ bar}$ cada 15 min) y descendente ($0.40 \rightarrow 0.05\text{ bar}$) a cizallamiento constante ($\dot{\gamma}_w$ hasta $2700\text{ s}^{-1}$).
  - Punto de inflexión en $dJ/d(\text{TMP})$ define $J_c$. El área de histéresis cuantifica el ensuciamiento hidráulicamente irreversible.
  - Validación de la ley de potencia: $J_c \propto (\dot{\gamma}_w)^n$ (distinguiendo difusión browniana $n \approx 0.33$ de difusión por corte $n \ge 1.0$).
* **Fase 2 (Diseño Factorial $3^2$ y Superficie de Respuesta):**
  - Factores: Caudal (25, 60, 95 RPM) $\times$ TMP (0.10, 0.25, 0.40 bar).
  - 9 tratamientos aleatorizados + **3 réplicas en el punto central (60 RPM, 0.25 bar)** para estimar la varianza residual pura y evaluar curvatura (11 corridas de 45-60 min).
* **Fase 3 (Compresibilidad Coloidal de Torta y Limpieza):**
  - Resistencia específica de torta: $R_{\text{torta}} = \alpha \cdot \frac{M_s}{A_m}$. Masa $M_s$ por balance de materia gravimétrico (filtración $0.45\ \mu\text{m}$ y secado a $105^\circ\text{C}$).
  - Modelo de compresibilidad: $\alpha = \alpha_0 (\Delta P)^s$, determinando el exponente $s$ mediante regresión logarítmica.
  - Limpieza entre ensayos: *Forward flush* a 95 RPM con retentado abierto + *backwash* suave por permeado ($\le 0.2\text{ bar}$). Criterio: $R_{m,\text{post}} \le 1.05 R_{m,0}$ ($FRR \ge 95\%$). Si falla, limpieza química con NaOCl ($100-200\text{ ppm}$, pH 10).

---

## ❓ 5. PREGUNTAS Y EJES DE AUDITORÍA PARA CLAUDE 5.5 OPUS

Te solicitamos una evaluación exhaustiva estructurada en los siguientes 5 ejes:

### EJE 1: Metrología de Caudal y Conteo Recíproco Continuo
1. En el algoritmo propuesto (`actualizar()`), ¿presenta la combinación de resta modular, referencia persistente `_tRef` y cota física continua $f \le 10^6/\Delta t_{\text{sin\_pulso}}$ algún *corner case* o vulnerabilidad oculta ante transitorios bruscos de parada o arranque?
2. La pulsación de los 3 rodillos peristálticos genera una modulación de flujo periódica a $1.25 - 5\text{ Hz}$. En una ventana de integración de 1 s, ¿cómo afecta este rizo a la estimación de $Q$ y de las presiones, y cuál es la estrategia matemática óptima (promediado sincrónico, filtro IIR/EMA con tau específico o amortiguador de pulsación) para no introducir sesgos en el cálculo de Darcy?
3. Sabiendo que $K_{\text{alim}} = 154.62$ y $K_{\text{perm}} = 55.00$ difieren fuertemente del valor nominal (98 pul/L), ¿recomiendas mantener factores K lineales fijos o calibrar una curva poligonal $K(f)$ por tramos mediante balance gravimétrico para el permeado a bajo flujo?

### EJE 2: Arquitectura FreeRTOS y Concurrencia Dual-Core
1. **Prioridad y Asignación de Núcleos:** En el ESP32, `setup()` corre en Core 1. ¿Es la asignación propuesta (`tareaControl` en Core 1 a Prioridad 19, `tareaWeb` en Core 0 a Prioridad 2) verdaderamente inmune al tráfico Wi-Fi y a las tareas del sistema lwIP? ¿Existe algún riesgo de inanición (*starvation*) del IDLE task o del watchdog de FreeRTOS si `tareaControl` o `tareaWeb` monopolizan tiempo?
2. **Sincronización:** ¿Es el spinlock `portMUX` sobre el struct POD $\le 300\text{ B}$ la solución más limpia y robusta, o tiene ventajas prácticas implementar un Seqlock con atómicos (`std::atomic<uint32_t> g_seq`) en la arquitectura Xtensa dual-core del ESP32?
3. **Escrituras a Memoria Flash:** Durante la ejecución de un ensayo, si se almacena la tara de presión o la calibración en Flash NVS (`Preferences.h`) o si se registran datos en LittleFS, se suspende la ejecución desde Flash en ambos núcleos durante el borrado de sector. ¿Qué impacto real tiene esto sobre la generación de pulsos LEDC (hardware) y sobre las interrupciones ISR de los caudalímetros? ¿Qué medidas preventivas debemos implementar?

### EJE 3: Seguridad Físico-Mecánica de la Bomba Peristáltica
1. Respecto al torque de $4.5\text{ Nm}$ del NEMA 34: ¿Consideras suficiente la combinación de válvula de alivio mecánica ($1.0 - 1.2\text{ bar}$) + hongo de emergencia en DM860 + `paradaDura()` en firmware, o recomendarías interponer un presostato electromecánico cableado al pin ENA del driver?
2. El techo dinámico de RPM (`_techo = 44 RPM` en ausencia de transductores válidos y frescos $<1.5\text{ s}$): ¿Presenta algún riesgo de transición brusca o desestabilización cinemática si la telemetría de presión oscila en el umbral de timeout?
3. En la inversión de sentido de giro: se estableció una desaceleración hasta 0 RPM y una pausa estática de $300 - 500\text{ ms}$ antes de alternar `PIN_DIR`. ¿Es este retardo suficiente para garantizar la disipación completa de la inercia del NEMA 34 antes de acelerar en sentido opuesto?

### EJE 4: Rigor Científico del Protocolo de Darcy y Diseño Factorial (Hito 5)
1. **Espacio Operativo No Rectangular:** Dado que a 1360 mL/min la TMP no puede ser menor a 0.08 bar y a 200 mL/min no puede ser mayor a 0.20 bar sin secar el lumen, ¿es estadísticamente válido y publicable ejecutar una Matriz Factorial $3^2$ ajustada por Metodología de Superficie de Respuesta (RSM), o sugieres restringir el diseño a un rango rectangular más estrecho?
2. **Determinación de Flujo Crítico ($J_c$):** ¿Es el método escalonado de TMP con análisis de histéresis el más adecuado para esta escala capilar, o recomiendas operar a flujo permeado constante mediante estrangulamiento de la línea de filtrado?
3. **Mecánica de Limpieza:** Ratificando que invertir la bomba no retrolava a través de la membrana, ¿cuál es el protocolo hidráulico más seguro para retrolavar el cartucho FX100 en el laboratorio sin dañar el potting ni colapsar las fibras (máxima contrapresión recomendada en el compartimento de permeado)? ¿Es adecuada la limpieza química con NaOCl a $100-200\text{ ppm}$ a pH 10 para remover coloides de *Opuntia* sin deteriorar la polisulfona?

### EJE 5: Dictamen de Cierre y "Red Flags"
1. ¿Identificas algún **punto ciego crítico (*fatal flaw*)** en el diseño presentado que pondría en riesgo la integridad de los equipos, la seguridad de los estudiantes en el laboratorio o la validez de los datos para la tesis doctoral?
2. ¿Cuál es tu lista priorizada de acciones inmediatas antes de encender la planta para la campaña experimental de ultrafiltración?

---

> 🎯 **Formato de Respuesta Solicitado:**  
> Por favor responde con el máximo nivel de rigor técnico y analítico, organizando tu dictamen en:  
> 1. **Evaluación Ejecutiva y Certificación de Arquitectura**  
> 2. **Análisis de los Ejes 1 al 4 (con justificación matemática, física y de código)**  
> 3. **Identificación de Puntos Ciegos y Red Flags (Eje 5)**  
> 4. **Checklist Final de Aceptación para Antonella, Owen y Enzo**
