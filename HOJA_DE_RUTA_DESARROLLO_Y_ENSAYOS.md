# 🗺️ HOJA DE RUTA MAESTRA: DESARROLLO DE FIRMWARE (HITO 2.3), INSTRUMENTACIÓN Y DISEÑO EXPERIMENTAL DE ULTRAFILTRACIÓN (HITO 5)
## Transición Tecnológica: De la Validación en Banco (v4) a la Concurrencia Determinista FreeRTOS, Seguridad Integral y Modelado de Ensuciamiento FX100

**Proyecto:** Planta Piloto de Ultrafiltración por Flujo Cruzado & Reactor de Coagulación-Sedimentación  
**Tesistas de Grado:** Antonella Guitián & Owen Cañizares  
**Codirector de Tesis:** Ing. Enzo *(Investigador Doctoral — Tesis Doctoral en Procesos de Separación por Membranas)*  
**Asesoría Técnica y Metodológica:** Antigravity AI  
**Auditoría Externa Cruzada de IA:** ChatGPT 6 Astra, GLM 5.3 y Claude Sonnet 5 (Ronda 1 y Ronda 2)  
**Ubicación:** Laboratorio de Ingeniería, Universidad Nacional de Salta (UNSa), Argentina  
**Fecha de Consolidación:** Octubre de 2026  

---

## 📌 1. Visión General y Estado Actual del Proyecto

El proyecto ha completado con éxito la fase de instrumentación básica y filtrado de interferencias electromagnéticas (EMI) en el banco de pruebas:
* **Impulsión y Cinemática:** Motor paso a paso NEMA 34 acoplado a cabezal peristáltico de precisión MBP-2000, comandado por driver Leadshine DM860 configurado a **3200 pulsos/rev** (16 micropasos), logrando eliminación total de resonancias y torque ripple.
* **Front-End de Caudal:** Caudalímetros YF-S401 desacoplados mediante bornera intermedia con resistencias pull-up externas de $4.7\text{ k}\Omega$ a 3.3V y filtros pasabajos analógicos RC ($100\text{ nF}$).
* **Firmware `subhito2_2_v3` (En Operación Física):** Calibrado en banco con probeta a 50 y 72 RPM ($K_{\text{alim}} = 154.62$, $K_{\text{perm}} = 55.00$), persistencia en memoria Flash NVS (`Preferences.h`) y Web SCADA en SoftAP puro (`192.168.4.1`).
* **Firmware `subhito2_2_v4` (Versión Blindada / Hardened):** 
  - Ampliación de rango de consigna hasta **100 RPM** ($\approx 1360\text{ mL/min}$) para permitir barridos de flujo y cizallamiento en agua durante el diseño factorial, manteniendo la alarma visual clínica de membrana en **44 RPM** ($600\text{ mL/min}$).
  - Algoritmo de período recíproco de pulsos en caudalímetros con resolución inferior a 0.1% a 5.5 Hz, protección estricta contra overflow de `micros()`, y sección crítica atómica por instancia con `portMUX_TYPE`.
  - Módulo físico de Darcy embebido (`darcy.h`) calculando viscosidad dinámica $\mu(T)$ según la ecuación de Vogel, corrección de temperatura TCF, flujo volumétrico $J$ y $J_{20}$ en $\text{L}/(\text{m}^2\cdot\text{h})$ (LMH) y acumulador de regresión lineal para $R_m$.
  - Cero fragmentación de memoria heap mediante `snprintf` con buffers locales de stack y transmisión HTTP por fragmentos (`CONTENT_LENGTH_UNKNOWN`).

Esta hoja de ruta establece los pasos de ingeniería rigurosos y consensuados con la auditoría externa para completar el **Hito 2.3** (Integración de Presiones, FreeRTOS y Seguridad Física) y el **Hito 5** (Protocolo Experimental de Tesis, Modelado de Darcy, Determinación de Flujo Crítico $J_c$ y Matriz Factorial).

---

## 🛡️ 2. Seguridad de Proceso: Operación con Desplazamiento Positivo y RPM_MAX = 100

> [!CAUTION]
> **Riesgo Físico Mayor y Resolución de Bloqueante:** Una bomba peristáltica acoplada a un NEMA 34 con torque nominal de $4.5\text{ Nm}$ es una **bomba de desplazamiento positivo**.  
> Si la válvula de aguja de retentado se estrangula o se ocluye la línea, la presión hidráulica no se autorregula: se incrementa exponencialmente hasta el límite mecánico del motor, superando ampliamente la resistencia de las mangueras, los conectores Luer-Lock y las fibras capilares de la membrana Fresenius FX100 (límite de rotura/delaminación $\approx 0.67\text{ bar}$). **La seguridad física no puede descansar únicamente en el microcontrolador.**

### 2.1. Protección Mecánica e Independiente del Firmware (Mandatoria)
1. **Válvula de Alivio Mecánica Calibrada a $\approx 0.70\text{ bar}$ (70 kPa):** 
   - *Corrección Crítica de Auditoría Opus:* La propuesta inicial de calibrar el alivio a $1.0 - 1.2\text{ bar}$ fue **vetada**, ya que con $P_1, P_2 \approx 1.0\text{ bar}$ la TMP alcanzaría $\approx 0.95\text{ bar}$, destruyendo los capilares. Además, un sensor de $0-1\text{ bar}$ satura en 1.0 bar, haciendo inútil un disparo de software en ese valor.
   - *Solución:* Instalar en la impulsión una válvula de alivio mecánico calibrada rigurosamente a **$\approx 0.70\text{ bar}$** (contrastada con manómetro patrón). Si la presión sube, el líquido recircula al tanque sin entrar al cartucho.
2. **Presostato Electromecánico Autónomo:** Interponer un presostato tarado a $0.70\text{ bar}$ cableado directamente a los bornes `ENA+ / ENA-` del driver Leadshine DM860 o en serie con su contactor de potencia, cortando el motor ante fallo del microcontrolador.
3. **Hongo de Parada de Emergencia Físico (E-Stop):** Pulsador con retención mecánica accesible al operador, cortando potencia al DM860.

### 2.2. Estrategia de Seguridad en Firmware
1. **Techo Dinámico de RPM (`_techo`):**
   * Por defecto, la bomba opera autolimitada con `_techo = RPM_ALARMA_MEMBRANA` ($44.0\text{ RPM} \approx 600\text{ mL/min}$).
   * El rango de **44 a 100 RPM solo se desbloquea** si se cumplen simultáneamente tres condiciones:
     a) Conversor ADS1115 y transductores de presión detectados y respondiendo en el bus I2C.
     b) Telemetría de presión fresca (antigüedad $< 1.5\text{ s}$).
     c) Histéresis de seguridad: tras una pérdida transitoria de telemetría, el techo baja instantáneamente a 44 RPM, y solo vuelve a habilitar 100 RPM tras **5 a 10 segundos** de lecturas continuas estables.
2. **Doble Tipo de Parada y Umbrales Corregidos:**
   * **Parada por Rampa (`detener()`):** Desacelera a $45\text{ RPM/s}$ en paradas normales.
   * **Parada Dura Inmediata (`paradaDura()`):** Disparada en $< 50\text{ ms}$ (sin rampa) ante:
     - Presión de entrada $P_1 \ge \mathbf{0.60\text{ bar}}$ (antes del fondo de escala del sensor).
     - Presión Transmembrana $\text{TMP} \ge \mathbf{0.45\text{ bar}}$ (antes del límite estructural de $0.50\text{ bar}$).
     - Pulsador de emergencia (E-Stop).
3. **Enclavamiento Persistente de ESTOP y Purga de Cola:**
   - La bandera `g_estop` enclava el estado `_bloqueada = true` de la bomba y **vacía inmediatamente la cola de comandos con `xQueueReset(g_colaComandos)`**. Esto evita el bug crítico detectado por Opus donde un comando `CMD_ARRANCAR` huérfano reiniciaba el motor en el mismo tick.
   - La bomba solo puede volver a arrancar tras un comando manual explícito de rearmado (`CMD_REARMAR`).
4. **Bloqueo Físico de Inversión con Membrana Conectada:**
   - Al invertir el giro, la válvula de alivio quedaría del lado de succión y la membrana podría recibir presión negativa/vacío indeseado. El firmware bloquea la orden de inversión si el sistema se encuentra en modo "Membrana FX100".
5. **Monitoreo de Deriva Cinemática ($Q_{\text{medido}}$ vs $Q_{\text{teórico}}$):**
   - Comparación continua entre $Q_{\text{alim}}$ y el flujo cinemático teórico ($Q = \text{RPM} \times 13.6\text{ mL/rev}$). Desviaciones $> 15\%$ sostenidas por 10 s disparan alarma de pasos perdidos o aire en línea.

---

## 🧭 3. Subhito 2.4: Instrumentación Manométrica Primaria (P1, P2) e Integración Híbrida en SCADA para Cálculo de Darcy en Tiempo Real

```
                    INSTALACIÓN DE MANÓMETROS DE GLICERINA EN PLANTA
   
    Línea de Impulsión                                             Línea de Permeado
    (Salida Bomba MBP-2000)                                        (Salida Filtrado)
           │                                                               │
           ▼                                                               ▼
     [ MANÓMETRO P1 ]                                                [ MANÓMETRO P3 ]
   (Baño de Glicerina)                                              (O Descarga Libre:
     0 a 1.0 / 1.6 bar                                                P3 ≈ 0.0 bar)
           │
           ├───► [ Válvula de Alivio Mecánica a 0.70 bar ] ──► Retorno a Tanque
           │
           ▼
    ┌────────────────────────────────────────────────────────┐
    │       MÓDULO DE ULTRAFILTRACIÓN FRESENIUS FX100        │
    │     (Lumen capilar de Polisulfona/Helixone, 2.2 m²)    │
    └────────────────────────────────────────────────────────┘
           │
           ▼
     [ MANÓMETRO P2 ]
   (Baño de Glicerina)
     0 a 1.0 / 1.6 bar
           │
           ▼
    [ Válvula de Aguja ] ──► Línea de Retentado (Recirculación a Tanque)
```

### 3.1. Decisión Estratégica de Ingeniería: Reemplazo de Transductores Genéricos y ADS1115 por Manómetros con Baño de Glicerina

Tras la adquisición de los **manómetros mecánicos con baño de glicerina** para la planta piloto, se tomó la decisión consensuada de ingeniería de **no adquirir ni montar transductores electrónicos genéricos (0-10 bar + ADS1115)**, fundamentada en sólidos criterios metrológicos y de compatibilidad electromagnética:

1. **Incompatibilidad de Rango Operativo y Error de Cero (*Zero Drift*):**
   * El rango de trabajo de Presión Transmembrana en el Hito 5 con la membrana Fresenius FX100 es de muy baja presión: **$0.10\text{ bar}$, $0.25\text{ bar}$ y $0.40\text{ bar}$** ($100\text{ a }400\text{ mbar}$).
   * Un transductor piezoeléctrico genérico de mercado de $0-10\text{ bar}$ ($0-1\text{ MPa}$) con precisión típica de $\pm 1.5\%$ Fondo de Escala (FS) presenta una incertidumbre absoluta de **$\pm 0.15\text{ bar}$ ($\pm 150\text{ mbar}$)**.
   * A una TMP operativa de $0.25\text{ bar}$, dicho error representa un **sesgo relativo del $60\%$**, haciendo inviable cualquier modelado científico de permeabilidad y resistencias de Darcy.
2. **Inmunidad Total al Ruido Electromagnético (EMI):**
   * El driver Leadshine DM860 conmuta corrientes inductivas de $3.0\text{ A}$ por fase a alta frecuencia sobre el motor NEMA 34.
   * Los cables de señal analógica milivoltílica y el bus I2C hacia el ADS1115 son altamente vulnerables al ruido acoplado, mientras que los manómetros de glicerina son **completamente mecánicos e inmunes a EMI**.
3. **Amortiguación Hidráulica Viscosa de Pulsaciones:**
   * La bomba peristáltica MBP-2000 de 3 rodillos genera pulsaciones hidráulicas a frecuencias de $1.25\text{ a }5\text{ Hz}$.
   * El baño de glicerina de alta viscosidad actúa como un filtro pasabajos mecánico de segundo orden, estabilizando la aguja indicadora y permitiendo una lectura visual directa del valor medio de presión sin oscilaciones erráticas ni fatiga mecánica del tubo Bourdon.

### 3.2. Topología Hidráulica e Instalación Física ($P_1, P_2, P_3$)
* **Manómetro $P_1$ (Alimentación / Feed):** Instalado inmediatamente antes de la entrada al cabezal superior de la membrana FX100, en derivación T posterior a la válvula de alivio calibrada a $0.70\text{ bar}$.
* **Manómetro $P_2$ (Retentado / Concentrado):** Instalado en la salida del cabezal inferior de la membrana, previo a la válvula de aguja de estrangulamiento.
* **Presión de Permeado ($P_3$):** Al descargar el permeado libremente a presión atmosférica o en probeta/tanque receptor abierto, se asume $P_3 = 0.00\text{ bar}$ manométrico (salvo que se instale un manómetro de baja escala si se restringe la salida).
* **Variables Hidráulicas Directas:**
  $$\Delta P_{\text{lumen}} = P_1 - P_2 \quad (\text{Pérdida de carga axial})$$
  $$\text{TMP} = \frac{P_1 + P_2}{2} - P_3 \quad (\text{Presión Transmembrana efectiva})$$

### 3.3. Integración Híbrida Asistida en el Servidor Web SCADA (`http://192.168.4.1`)
Para mantener la automatización del datalogger y el cálculo científico de Darcy en tiempo real sin requerir transductores electrónicos:
1. El operador observa las lecturas estables en los manómetros de glicerina ($P_1$ y $P_2$) y la temperatura del agua ($T\ [^\circ\text{C}]$) en la probeta o termómetro.
2. En la interfaz web del SCADA (`http://192.168.4.1`), el operador ingresa los valores en el panel dedicado **"📊 Presión Manométrica Primaria & Darcy"** y presiona **"Fijar Presiones"**.
3. El microcontrolador ESP32 recibe los valores mediante el endpoint HTTP `/set_presion?p1=...&p2=...&temp=...` y el módulo C++ embebido `darcy.h` calcula de forma instantánea:
   * Presión Transmembrana: $\text{TMP} = \frac{P_1 + P_2}{2} - P_3\ [\text{bar}]$.
   * Viscosidad dinámica del agua según Vogel: $\mu(T) = 0.00002414 \times 10^{\frac{247.8}{T + 273.15 - 140}}\ [\text{Pa}\cdot\text{s}]$.
   * Factor de Corrección Térmica: $\text{TCF}(T) = \frac{\mu(T)}{\mu(20^\circ\text{C})}$.
   * Flujo normalizado a 20°C: $J_{20} = J \cdot \text{TCF}\ [\text{LMH}]$.
   * Resistencia hidráulica total: $R_{\text{total}} = \frac{\text{TMP}\ [\text{Pa}]}{\mu(T) \cdot J_{\text{SI}}}\ [\text{m}^{-1}]$.
   * Resistencia adicional de ensuciamiento/torta: $R_{\text{torta}} = R_{\text{total}} - R_m\ [\text{m}^{-1}]$.
4. **Exportación Continua en Datalogger CSV:** Cada fila registrada en el buffer del ESP32 (cada 10 segundos) incluye automáticamente las columnas calculadas de presiones y resistencias, permitiendo la descarga directa del ensayo completo para su análisis en Excel o Python.

### 3.4. Seguridad de Proceso y Enclavamientos Asistidos
* **Protección Mecánica Autónoma:** La válvula de alivio calibrada a **$0.70\text{ bar}$** y el hongo de parada de emergencia (E-Stop) garantizan la protección física contra rotura de mangueras y sobrepresión de capilares sin depender del software.
* **Alarma Visual y Sonora en SCADA:** Si el operador ingresa un valor de $P_1 \ge 0.60\text{ bar}$ o $\text{TMP} \ge 0.45\text{ bar}$, la interfaz web despliega una advertencia crítica en rojo y emite alerta de seguridad para instruir la apertura inmediata de la válvula de retentado o la detención de la bomba.

---

## ⚡ 4. Hito 2.3 / Hito 4: Arquitectura Concurrente FreeRTOS (Dual-Core Determinista)

### 4.1. Diagnóstico del Cuello de Botella en Core 1
En Arduino-ESP32, la función `setup()` y la tarea `loopTask` corren por defecto en el **Core 1**, mientras que el stack Wi-Fi de lwIP corre a prioridad elevada (18-19).
Si el lazo de control comparte el bucle con `server.handleClient()`, una ráfaga de tráfico HTTP o una descarga de CSV introduce fluctuaciones temporales (*jitter*) de decenas de milisegundos en la máquina de rampa y en la vigilancia de sobrepresión.

### 4.2. Topología de Concurrencia Asimétrica

```
                   ARQUITECTURA DUAL-CORE FREERTOS
  ┌─────────────────────────────────┐   ┌─────────────────────────────────┐
  │   CORE 0: TELEMETRÍA, RED & UI  │   │      CORE 1: CONTROL & FSM      │
  │      (Prioridad 2 / Red)        │   │    TIEMPO REAL (Prioridad 19)   │
  ├─────────────────────────────────┤   ├─────────────────────────────────┤
  │ • Tarea WebServer (Core 0)      │   │ • Tarea Control (Core 1, 50 ms) │
  │ • SoftAP + Conexión Wi-Fi STA   │   │ • vTaskDelayUntil() determinista│
  │ • Endpoints JSON / CSV          │   │ • Rampa cinemática S-Curve      │
  │ • UI SCADA HTML/JS              │   │ • FSM ADS1115 (250 SPS)         │
  │ • Persistencia diferida NVS     │   │ • Detección P1 pico y TMP       │
  │ • Despacho de comandos          │   │ • Enclavamientos de seguridad   │
  └────────────────┬────────────────┘   └────────────────┬────────────────┘
                   │                                     │
                   ├──────── Banderas Atómicas (STOP) ───┤
                   ├──────── Cola de Comandos (Queue) ───┤
                   └──────── Snapshot POD (portMUX) ─────┘
```

#### Asignación de Prioridades y Núcleos:
* **Core 1 — `tareaControl` (Prioridad 19 — Tiempo Real Crítico):**
  - Período estricto de **50 ms (20 Hz)** mediante `vTaskDelayUntil()`.
  - Al asignarse prioridad 19, se ubica por encima del tráfico de red lwIP, garantizando determinismo estricto (< 100 µs de jitter) con un consumo de CPU inferior al $1\%$.
  - Manejo cinemático exclusivo de la instancia `Bomba`.
  - **Enclavamientos en Cada Ciclo (20 Hz):** *Corrección Crítica de Opus:* La evaluación de $P_1 \ge 0.60\text{ bar}$ y $\text{TMP} \ge 0.45\text{ bar}$ se ejecuta **en cada ciclo de 50 ms**, NUNCA en el bucle lento de 1 Hz. Con $4.5\text{ Nm}$ de torque, 1 segundo es letal ante el cierre accidental de una válvula.
  - Muestreo analógico no bloqueante de presiones mediante FSM de ADS1115 (4 canales a 250 SPS $\approx 20\text{ ms}$).
  - Enclavamiento de parada dura inmediata ante sobrepresión o vaciamiento de tanque por boya.
* **Core 0 — `tareaWeb` (Prioridad 2):**
  - Servidor HTTP `WebServer`, resolución DNS y entrega de interfaz gráfica web.
  - Generación de telemetría JSON y descarga de archivos CSV en bloques.
* **Sincronización Thread-Safe sin Bloqueo:**
  1. **Lectura de Estado (Core 1 $\rightarrow$ Core 0):** Estructura plana POD `SnapshotPlanta_t` ($\le 300\text{ bytes}$) copiada bajo spinlock liviano `portMUX_TYPE` (duración de copia $\approx 1\ \mu\text{s}$, atómica, sin asignación dinámica y sin riesgo de inversión de prioridades). El buffer local `sLocal` se asigna por completo antes de publicar.
  2. **Comandos de Marcha/Consigna (Core 0 $\rightarrow$ Core 1):** Despachados mediante cola de mensajes FreeRTOS `xQueueSend(g_cola, &cmd, 0)` (para `ARRANCAR`, `SET_RPM`, `CAMBIAR_MODO`).
  3. **Comandos de Parada de Emergencia (STOP / ESTOP):** Se desacoplan de la cola y se transmiten mediante **banderas atómicas directas** (`std::atomic<bool> g_stop`, `g_estop`), garantizando respuesta instantánea en el siguiente tick del control. Al dispararse `g_estop`, se vacía la cola con `xQueueReset()`.
  4. **Protección de Caché Flash:** Las escrituras en Flash NVS (`Preferences.h`) o LittleFS suspenden la caché en ambos núcleos; por ello, **se prohíbe escribir en Flash durante un ensayo activo**. Los datos se transmiten a la PC por streaming Wi-Fi o se acumulan en RAM / SD.

### 4.3. Metrología de Caudalímetro y Resolución de Permeado a Bajo Flujo
1. **Resta Modular Segura ante Desborde de `micros()`:** El cálculo de tiempo entre flancos utiliza sustracción no signada de 32 bits directa `(t_ult - t_prim)`, la cual es matemáticamente exacta a través del rollover de `micros()` (que ocurre cada 71.6 minutos).
2. **Referencia Persistente entre Ventanas:** Se preserva el último flanco de la ventana anterior como referencia temporal (`_tRef`), contabilizando períodos exactos y evitando descartar pulsos entre ventanas.
3. **Cota Física Continua en Reposo:** Para evitar lecturas fantasmas prolongadas tras la detención de la bomba, si transcurre un tiempo $\Delta t$ sin registrar flancos, el caudal se acota de forma continua según el límite físico superior:
   $$f \le \frac{10^6}{\Delta t_{\text{sin\_pulso}}} \quad [\text{Hz}]$$
4. **Discriminación de Falla:** La alarma de falla por ausencia de pulsos (`sinSenal()`) se aplica **exclusivamente al caudalímetro de alimentación**.
5. **Resolución Crítica de Permeado Fuera de Rango (Alerta Opus):**
   * El sensor YF-S401 tiene un rango útil de fábrica de $0.3\text{ a }6.0\text{ L/min}$ ($300\text{ a }6000\text{ mL/min}$).
   * El flujo permeado de la membrana FX100 opera entre **$45\text{ y }365\text{ mL/min}$**, cayendo por debajo de $50\text{ mL/min}$ a medida que progresa el ensuciamiento. A $10\text{ mL/min}$, la frecuencia es de apenas $0.55\text{ Hz}$ (menos de 1 pulso por segundo), donde la fricción de la turbina introduce no-linealidades severas.
   * **Estrategia Metrológica Dual:**
     a) **Referencia Primaria Gravimétrica:** Utilizar una balanza de precisión de laboratorio con salida serie/USB conectada a la PC de captura como estándar maestro de $J$ en los balances de masa.
     b) **Integración Temporal en Firmware:** Para el YF-S401 de permeado, calcular $Q_{\text{perm}}$ mediante ventanas de integración extendidas ($\Delta \text{vol} / \Delta t$ en $10\text{ a }30\text{ s}$) y aplicar una curva de calibración poligonal $K(f)$ por tramos calibrada con balanza (5 a 8 puntos).

---

## 🧪 5. Hito 5: Protocolo Científico de Modelado Darcy y Diseño Factorial de Ensayos

El Hito 5 constituye el núcleo de la producción científica y de ingeniería para la defensa de la tesis de Antonella y Owen y la validación de modelos en la tesis doctoral de Enzo.

### 5.1. Ecuaciones Rectoras del Transporte en Fibra Hueca
El transporte de solvente a través de la membrana de Polisulfona/PVP Fresenius FX100 se modela mediante la Ley de Darcy extendida:

$$J = \frac{Q_p}{A_m} = \frac{\text{TMP}}{\mu(T) \cdot R_{\text{total}}} = \frac{\text{TMP}}{\mu(T) \cdot (R_m + R_{\text{foul}})}$$

Donde:
* $J$: Flujo permeado volumétrico específico $[\text{L}/(\text{m}^2\cdot\text{h})]$ o $[\text{m}^3/(\text{m}^2\cdot\text{s})]$.
* $Q_p$: Caudal volumétrico medido por el caudalímetro de permeado $[\text{L}/\text{h}]$.
* $A_m$: Área efectiva de transferencia superficial del cartucho FX100 (**$2.2\text{ m}^2$**).
* $K_{\text{UF}}$ Nominal de Membrana Limpia: **$54750\text{ mL/(h}\cdot\text{bar)}$** ($73\text{ mL/(h}\cdot\text{mmHg)} \times 750\text{ mmHg/bar} = 912.5\text{ mL/(min}\cdot\text{bar)}$). Resistencia intrínseca nominal: $R_{m,\text{fab}} \approx 1.4 \times 10^{13}\text{ m}^{-1}$.
* $\mu(T)$: Viscosidad dinámica del agua $[\text{Pa}\cdot\text{s}]$, calculada mediante la ecuación de Vogel:
  $$\mu(T) = 0.00002414 \times 10^{\frac{247.8}{T(K) - 140}}$$
* $J_{20}$: Flujo volumétrico normalizado a $20^\circ\text{C}$ (eliminando variaciones estacionales de temperatura en Salta):
  $$J_{20} = J_T \cdot \text{TCF}(T) = J_T \cdot \frac{\mu(T)}{\mu(20^\circ\text{C})}$$
* $R_m$: Resistencia hidráulica propia de la membrana limpia $[\text{m}^{-1}]$.
* $R_{\text{foul}}$: Resistencia total de ensuciamiento ($R_{\text{torta}} + R_{\text{irrev}}$).
* **Unidades de Factor $K$:** Los factores $K$ se calibran en **$\text{Hz}/(\text{L/min})$** ($K_{\text{alim}} = 154.62$, $K_{\text{perm}} = 55.00$), de modo que $Q\,[\text{mL/min}] = f\,[\text{Hz}] \times 1000 / K$.
* **Esfuerzo de Corte Luminal ($\dot{\gamma}_w$):** En los 13500 capilares paralelos de radio interno $r_i = 92.5\ \mu\text{m}$, la velocidad de corte en pared generada por el flujo cruzado es:
  $$\dot{\gamma}_w = \frac{4 \cdot Q_{\text{feed}}}{\pi \cdot N_{\text{fibras}} \cdot r_i^3} \quad \left[\text{s}^{-1}\right]$$
  Para caudales de $200\text{ a }1360\text{ mL/min}$, $\dot{\gamma}_w$ varía entre **$397\text{ s}^{-1}$ y $2700\text{ s}^{-1}$**, cubriendo el rango hidrodinámico exacto donde la retrodifusión inducida por corte previene la deposición de sólidos.

---

### 5.2. Espacio Operativo Real $(Q, \text{TMP})$ y Restricciones Físicas
> [!NOTE]
> **El espacio experimental $(Q, \text{TMP})$ no es rectangular y requiere recirculación:**  
> 1. A caudales altos ($1360\text{ mL/min}$), la pérdida de carga axial por fricción a lo largo de las fibras capilares ($\Delta P_{\text{lumen}} \approx 0.16\text{ bar}$) fija una $\text{TMP}$ mínima inevitable de $\approx \Delta P/2 \approx 0.08\text{ bar}$.
> 2. A caudales bajos ($25\text{ RPM} \approx 340\text{ mL/min}$), una TMP alta ($0.40\text{ bar}$) requeriría $\approx 365\text{ mL/min}$ de permeado con agua limpia, superando el $100\%$ del caudal de alimentación (infactible). Por ello, el nivel bajo del diseño factorial se desplaza a **$40\text{ RPM}$ ($544\text{ mL/min}$)**.
> 3. **Recirculación Obligatoria del Permeado:** Durante los ensayos a concentración constante, la manguera de permeado debe retornar obligatoriamente al tanque de alimentación. De lo contrario, en 60 minutos se extraerían 10 a 20 Litros, concentrando artificialmente la alimentación y falseando las cinéticas de ensuciamiento.

Por esta razón, la experimentación se estructura en **tres fases secuenciales de alta eficiencia**.

---

### 5.3. Fase 0: Caracterización Inicial y Línea Base Diaria ($R_{m,0}$)
1. Cargar el sistema con agua desionizada o destilada ($< 1\text{ NTU}$, conductividad $< 10\ \mu\text{S/cm}$).
2. Desgasificar el módulo montado verticalmente.
3. Ejecutar 4 o 5 escalones de presión transmembrana:
   $$\text{TMP} = [0.05, \ 0.10, \ 0.15, \ 0.20, \ 0.25]\text{ bar}$$
4. Mantener cada escalón durante $5\text{ minutos}$ y registrar $J_{20}$.
5. Ajustar por regresión lineal: $J_{20} = \frac{1}{\mu_{20} R_m} \cdot \text{TMP}$.
6. **Criterio de Validación:** $R^2 \ge 0.985$. El valor obtenido de $R_{m,0}$ se guarda en Flash NVS y se utiliza como covariable de referencia para todos los ensayos del día.

---

### 5.4. Fase 1: Determinación de Flujo Crítico ($J_c$) por TMP Escalonado
El Flujo Crítico delimita el régimen donde la fuerza convectiva hacia la membrana equilibra la fuerza de arrastre tangencial por cizallamiento.

```
       FLUJO J [LMH]
         ▲
         │                        Régimen Supercrítico
         │                       (Colmatación Irreversible)
         │                         /  • Ensuciamiento por torta compactada
         │                        /
         │     Régimen Subcrítico/   ◄── Punto de Quiebre = FLUJO CRÍTICO (Jc)
         │       (Lineal/Limpio)/
         │                     /
         │                    /  Curva descendente (Histéresis)
         │                   / ◄─────── Permeabilidad residual post-ensuciamiento
         │                  /
         └─────────────────┴────────────────────────► TMP [bar]
```

#### Protocolo de Ensayo Escalonado (Flux-Step Method):
1. Cargar suspensión sintética condicionada (bentonita/caolín) con turbidez conocida (ej. $100\text{ NTU}$).
2. Fijar caudal constante de alimentación (ej. $Q_1 = 340\text{ mL/min}$, $Q_2 = 816\text{ mL/min}$ o $Q_3 = 1292\text{ mL/min}$).
3. **Escalones Ascendentes:** Iniciar en $\text{TMP} = 0.05\text{ bar}$ e incrementar en pasos de $\Delta \text{TMP} = 0.05\text{ bar}$ cada $15\text{ minutos}$ hasta alcanzar $0.40\text{ bar}$.
4. **Escalones Descendentes:** Reducir la TMP en pasos simétricos de regreso a $0.05\text{ bar}$.
5. **Análisis de Histéresis y Mecanismo:**
   * La divergencia entre la curva ascendente y la recta de agua limpia determina el **Flujo Crítico ($J_c$)**.
   * El área de histéresis entre la curva ascendente y descendente cuantifica la magnitud del ensuciamiento hidráulicamente irreversible.
   * **Predicción Científica Falsable:** Comprobar la relación $J_c \propto (\dot{\gamma}_w)^n$. Si $n \approx 0.33$, predomina la difusión browniana; si $n \ge 1.0$, predomina la retrodifusión inducida por esfuerzo de corte sobre flóculos coloidales.

---

### 5.5. Fase 2: Matriz Experimental Factorial Completa ($3^2$) y Vinculación con Pretratamiento (*Opuntia ficus-indica*)
Para optimizar el esfuerzo de laboratorio, evitar esquinas infactibles y preservar las membranas:

* **Factor A: Caudal de Impulsión / Esfuerzo de Corte ($Q_{\text{feed}}$)**
  - Nivel Bajo ($-1$): **$40\text{ RPM}$** ($544\text{ mL/min}$, $\dot{\gamma}_w \approx 1060\text{ s}^{-1}$ — *desplazado desde 25 RPM para evitar recuperación $>100\%$ a alta TMP*).
  - Nivel Medio ($0$): **$70\text{ RPM}$** ($952\text{ mL/min}$, $\dot{\gamma}_w \approx 1850\text{ s}^{-1}$).
  - Nivel Alto ($+1$): **$95\text{ RPM}$** ($1292\text{ mL/min}$, $\dot{\gamma}_w \approx 2560\text{ s}^{-1}$).
* **Factor B: Presión Transmembrana ($\text{TMP}$)**
  - Nivel Bajo ($-1$): $0.10\text{ bar}$ ($100\text{ mbar}$).
  - Nivel Medio ($0$): $0.25\text{ bar}$ ($250\text{ mbar}$).
  - Nivel Alto ($+1$): $0.40\text{ bar}$ ($400\text{ mbar}$).

#### Plan de Ejecución (13 Ensayos Aleatorizados con Recirculación Total):
1. **9 tratamientos de la matriz factorial $3 \times 3$** ejecutados con agua pretratada por coagulación-floculación con *Opuntia ficus-indica* (Hito 3).
2. **3 réplicas en el Punto Central ($70\text{ RPM}$, $\text{TMP} = 0.25\text{ bar}$)** con agua pretratada con *Opuntia*, distribuidas al inicio, mitad y final de la campaña para estimar la varianza residual pura y evaluar curvatura cuadrática mediante Metodología de Superficie de Respuesta (RSM).
3. **1 Corrida Testigo de Control sin Pretratamiento (Agua Cruda Directa):**
   * Ejecutada en las condiciones idénticas del Punto Central (**$70\text{ RPM}$ y $\text{TMP} = 0.25\text{ bar}$**).
   * **Objetivo Científico Fundamental de la Tesis:** Contrastar directamente la corrida testigo contra las 3 réplicas del punto central con *Opuntia* para cuantificar rigurosamente:
     * Reducción absoluta y porcentual de la resistencia de torta: $\Delta R_{\text{torta}} = R_{\text{torta, cruda}} - R_{\text{torta, Opuntia}}$.
     * Atenuación de la pendiente de declinación de flujo adimensional: $\left|\frac{d(J/J_0)}{dt}\right|_{\text{Opuntia}} \ll \left|\frac{d(J/J_0)}{dt}\right|_{\text{cruda}}$.
     * Incremento del Factor de Recuperación de Flujo post-retrolavado ($\text{FRR}$).
* **Condición Operativa Mandatoria:** Manguera de permeado retornando al tanque de alimentación (recirculación total de permeado) para garantizar concentración constante durante los 45 a 60 minutos de cada ensayo.

#### Respuestas Primarias Registradas en CSV y Balanza:
1. Declinación temporal de flujo: $J(t)$ y $J_{20}(t)$ (registrados por balanza de precisión y corroborados por YF-S401).
2. Velocidad de ensuciamiento hidráulico: $\frac{dR_{\text{total}}}{dt}$.
3. Resistencia reversible: $R_{\text{torta}}$ post-ensayo.
4. Resistencia irreversible: $R_{\text{irrev}}$.
5. Factor de Recuperación de Flujo: $FRR = \frac{J_{\text{post-lavado}}}{J_0} \times 100\%$.
6. Remoción de Turbidez: $\eta_{\text{turb}} = \left(1 - \frac{\text{NTU}_{\text{perm}}}{\text{NTU}_{\text{feed}}}\right) \times 100\%$.

---

### 5.6. Fase 3: Modelado de Compresibilidad Coloidal de Torta
La resistencia específica de la torta ($\alpha$) se modela a partir del balance de materia gravimétrico:

$$R_{\text{torta}} = \alpha \cdot \frac{M_s}{A_m}$$

Donde:
* $M_s$: Masa de sólidos depositados sobre la membrana, calculada por balance gravimétrico de SST (muestras filtradas por membranas de $0.45\ \mu\text{m}$ secadas en estufa a $105^\circ\text{C}$).
* Ley de Compresibilidad:
  $$\alpha = \alpha_0 \cdot (\Delta P)^s$$
  - $s$: Índice de compresibilidad ($s \approx 0$ para torta incompresible, $s \in [0.5, \ 1.0]$ para agregados floculados deformables).
  - Mediante la regresión lineal de $\ln(\alpha)$ vs $\ln(\text{TMP})$, se obtiene empíricamente el valor de $s$ para la tesis.

---

### 5.7. Protocolo de Limpieza y Clarificación del Retrolavado

> [!IMPORTANT]
> **Aclaración Mecánica sobre el Retrolavado (Dictamen Opus):**  
> Invertir el sentido de giro de la bomba peristáltica en la línea de impulsión **no efectúa retrolavado a través de la pared capilar**. Solo invierte la dirección del flujo a lo largo del lumen o drena el módulo.  
> Un retrolavado (*backwash*) real requiere introducir líquido limpio desde el puerto exterior de permeado hacia el lumen. El método más seguro y reproducible en laboratorio es utilizar una **columna o depósito hidrostático elevado ($\le 1.5\text{ m} \approx 0.15\text{ bar}$)** con el lumen abierto al drenaje, aplicando pulsos suaves de **$30\text{ a }60\text{ segundos}$**. Esto garantiza que la contrapresión jamás exceda el límite del potting ni colapse las fibras.

#### Ciclo de Limpieza entre Ensayos:
1. **Enjuague Frontal Rápido (*Forward Flush*):** Circular agua limpia a caudal alto ($95\text{ RPM}$) con la válvula de permeado cerrada y la válvula de retentado 100% abierta durante $5\text{ minutos}$ para barrer la torta suelta por cizallamiento superficial.
2. **Retrolavado Hidrostático Suave (*Backwash*):** Conectar el depósito elevado de agua destilada al puerto de permeado ($\le 0.15\text{ bar}$) durante $60\text{ segundos}$ con drenaje luminal abierto.
3. **Comprobación de Aceptación:** Medir $R_{m,\text{post}}$ con agua limpia a $\text{TMP} \ge 0.20\text{ bar}$. Si $R_{m,\text{post}} \le 1.05 \cdot R_{m,0}$ ($FRR \ge 95\%$), el cartucho se declara apto para la siguiente corrida.
4. **Limpieza Química Oxidante (CIP):** Si $FRR < 95\%$ debido a fouling orgánico/coloidal persistente:
   - Preparar solución de Hipoclorito de Sodio ($\text{NaOCl}$) a **$100\text{ a } 200\text{ ppm}$** de cloro libre activo.
   - Ajustar pH a **$9.5 - 10.5$** con $\text{NaOH}$ diluido (evitar degradación ácida de la polisulfona/PVP).
   - Recircular a baja presión durante $20\text{ minutos}$ a temperatura ambiente ($< 35^\circ\text{C}$).
   - **Registro Acumulado de Dosis:** Llevar una bitácora estricta de exposición en $\text{ppm}\cdot\text{h}$ para no exceder la vida útil de la membrana.
   - Para flóculos inorgánicos (aluminio/hierro), realizar un paso separado con **ácido cítrico al 1-2%** (pH 2-3), **NUNCA mezclado con cloro**.
   - Enjuagar exhaustivamente con agua pura hasta alcanzar neutralidad en pH y conductividad TDS basal.

---

### 5.8. Subhito 5.5 — Modelado Cinético de Mecanismos de Fouling (Modelos de Hermia) y Optimización por Superficie de Respuesta (RSM / ANOVA)

Aprovechando las series temporales de flujo $J(t)$ registradas cada $10\text{ segundos}$ en los archivos `.CSV` durante los 60 minutos de cada corrida:

1. **Ajuste de la Ecuación General de Bloqueo de Hermia:**
   $$\frac{d^2 t}{d V^2} = k \left(\frac{d t}{d V}\right)^n \quad \Longleftrightarrow \quad -\frac{dJ}{dt} = K_H \, J \, (A_m J)^{2-n}$$
   Se ajustan por regresión no lineal y linealización los 4 mecanismos clásicos:
   * **Bloqueo Completo de Poros ($n = 2.0$):** Cada partícula coloidal sella completamente la entrada de un poro sin superposición.
     $$\ln\left(\frac{J}{J_0}\right) = -k_b \, t$$
   * **Bloqueo Estándar Interno ($n = 1.5$):** Las partículas penetran en los capilares y se adsorben en las paredes internas de los poros reduciendo su diámetro útil.
     $$\frac{1}{\sqrt{J}} - \frac{1}{\sqrt{J_0}} = k_s \, t$$
   * **Bloqueo Intermedio ($n = 1.0$):** Algunas partículas bloquean poros abiertos mientras otras se depositan sobre partículas ya adsorbidas.
     $$\frac{1}{J} - \frac{1}{J_0} = k_i \, t$$
   * **Filtración por Formación de Torta ($n = 0.0$):** Las partículas coloidales y flóculos forman una capa porosa superficial sobre la membrana.
     $$\frac{1}{J^2} - \frac{1}{J_0^2} = 2 \, k_c \, t$$
2. **Identificación de Transición de Mecanismos:**
   * Comparación de coeficientes de determinación $R^2$ para determinar el mecanismo rector en cada fase temporal (bloqueo de poros inicial vs. crecimiento de torta en régimen).
   * Contraste directo del mecanismo entre la corrida con agua cruda y las corridas con agua floculada con *Opuntia ficus-indica*.
3. **Superficie de Respuesta 3D y Análisis de Varianza (ANOVA):**
   * Ajuste de modelo polinomial de segundo orden para el flujo estacionario $J_{\text{ss}}$, la merma de caudal y $R_{\text{torta}}$:
     $$Y = \beta_0 + \beta_1 (\text{RPM}) + \beta_2 (\text{TMP}) + \beta_{12} (\text{RPM} \cdot \text{TMP}) + \beta_{11} (\text{RPM}^2) + \beta_{22} (\text{TMP}^2) + \varepsilon$$
   * Tabla ANOVA para verificar significancia estadística ($p < 0.05$) de efectos lineales, cuadráticos y de interacción.
   * Determinación de las condiciones operativas óptimas $(\text{RPM}^*, \text{TMP}^*)$ que maximizan el volumen de permeado minimizando la velocidad de ensuciamiento.

---

### 5.9. Subhito 5.6 — Autopsia Morfológica Post-Mortem de las Fibras Capilares Helixone® mediante Microscopía Electrónica de Barrido (SEM / EDS)

> [!CAUTION]
> **Ensayo Destructivo — Ubicación Estricta al Cierre Experimental:**  
> La autopsia de la membrana Fresenius FX100 implica el seccionamiento físico del cartucho de diálisis, la fractura criogénica y la extracción irreversible de fibras capilares. Se ejecuta **estrictamente una vez concluida la totalidad de corridas de la matriz factorial**.

#### Protocolo de Disección y Microscopía Electrónica (SEM / EDS):
1. **Seccionamiento y Muestreo Axial:**
   * Apertura controlada de la carcasa de policarbonato del cartucho FX100 mediante sierra oscilante sin afectar el haz central.
   * Extracción de fascículos de fibras capilares ($185\ \mu\text{m}$ diámetro interno, $35\ \mu\text{m}$ espesor de pared) en 3 posiciones axiales estratégicas:
     - **Zona 1 (Entrada / Feed):** Máximo esfuerzo de corte luminal y primer impacto coloidal.
     - **Zona 2 (Centro del Módulo):** Régimen hidrodinámico completamente desarrollado.
     - **Zona 3 (Salida / Retentado):** Máxima concentración de sólidos y polarización por concentración límite.
2. **Corte Transversal Criogénico (*Cross-Section*):**
   * Congelación de fibras en Nitrógeno Líquido ($-196^\circ\text{C}$) y fractura frágil limpia para preservar la arquitectura porosa asimétrica sin deformación mecánica.
   * Metalizado con oro/platino por pulverización catódica (*sputtering*).
   * **Medición Micrométrica Directa del Espesor de Torta ($\delta_c\ [\mu\text{m}]$):** Medición por micrografía SEM de la capa superficial de torta coloidal sobre la cara interna luminal.
   * **Validación Falso-Científica de Darcy:** Correlación directa entre el espesor observado $\delta_c$ y la resistencia hidráulica $R_{\text{torta}}$ calculada por el modelo de Darcy en el SCADA ($R_{\text{torta}} = \frac{\delta_c}{P_{\text{torta}}}$).
3. **Superficie Luminal Interna (Micrografías Comparativas a 2.000x, 5.000x y 20.000x):**
   * **Fibra Virgen (Control de Fábrica):** Matriz limpia de Polisulfona/PVP, estructura microporosa abierta.
   * **Fibra Ensuciada (Pre-Retrolavado):** Cobertura densa de torta coloidal y agregados floculados de *Opuntia*.
   * **Fibra Regenerada (Post-Retrolavado):** Verificación visual directa de poros desobstruidos y partículas residuales irreversibles que justifican físicamente el valor de $R_{\text{irrev}}$.
4. **Microanálisis Elemental por Dispersión de Energía de Rayos X (EDS):**
   * Mapeo composicional puntual y espectral sobre la superficie de ensuciamiento:
     - Identificación inorgánica: Silicio ($\text{Si}$), Aluminio ($\text{Al}$), Hierro ($\text{Fe}$), Calcio ($\text{Ca}$), Magnesio ($\text{Mg}$) atribuibles a arcillas bentoníticas y sales minerales.
     - Identificación orgánica: Carbono ($\text{C}$) y Oxígeno ($\text{O}$) atribuibles a restos poliméricos del biocoagulante *Opuntia ficus-indica*.

---

## 🏭 6. Hito 6: Evaluación de Indicadores de Ingeniería Industrial (SEC, Costo Operativo y Vida Útil)

El Hito 6 proporciona el cierre ingenieril y económico indispensable para la graduación de Antonella y Owen en Ingeniería Industrial / Química, transformando los datos fenomenológicos en métricas de toma de decisiones productivas.

### 6.1. Subhito 6.1 — Consumo Específico de Energía ($\text{SEC}$ en $\text{kWh/m}^3$)
1. **Medición y Modelado de Potencia Eléctrica:**
   * Medición de potencia de la etapa de potencia DM860 + motor NEMA 34 y del agitador L298N en cada punto operativo $(\text{RPM}, \text{TMP})$.
   * Potencia eléctrica total absorbida: $P_{\text{eléctrica}} = P_{\text{bomba}} + P_{\text{agitador}}\ [\text{kW}]$.
2. **Cálculo del SEC en la Matriz Factorial:**
   $$\text{SEC} = \frac{P_{\text{eléctrica}}\ [\text{kW}]}{Q_p\ [\text{m}^3/\text{h}]} \quad \left[\frac{\text{kWh}}{\text{m}^3}\right]$$
3. **Optimización Energética de Proceso:**
   * Gráfica de curvas de iso-consumo energético específico ($\text{SEC}$ vs $\text{TMP}$ y $\text{RPM}$).
   * Identificación del Punto Óptimo de Compromiso: mínimo gasto energético por metro cúbico producido con máxima retención de turbidez y calidad de agua CAA.

### 6.2. Subhito 6.2 — Estimación de Vida Útil del Módulo y Costo Operativo Unitario
1. **Modelado de Pérdida Irreversible de Permeabilidad y Fin de Vida Útil:**
   * Proyección del incremento acumulativo de resistencia irreversible ciclo a ciclo:
     $$R_{\text{irrev}}(N) = R_{m,0} + N \cdot \Delta R_{\text{irrev}}$$
   * Definición del Fin de Vida Útil del Módulo FX100 cuando el flujo permeado declina un $40\%$ respecto a su valor inicial ($FRR < 60\%$) incluso tras limpieza química CIP profunda.
2. **Frecuencia Óptima de Retrolavado y Limpieza Química (CIP):**
   * Optimización temporal entre tiempo de filtración productiva ($t_f$) y tiempo de parada por lavado ($t_l$) para maximizar la productividad neta de agua tratada.
3. **Estructura de Costos Operativos ($\text{OPEX}$ Unitario):**
   * Costo energético ($\text{SEC} \times \text{Tarifa Eléctrica}$ en $\$AR/\text{kWh}$).
   * Costo de insumos químicos (preparación de *Opuntia ficus-indica*, $\text{NaOCl}$, ácido cítrico).
   * Amortización del módulo capilar Fresenius FX100 (costo de reposición / $\text{m}^3$ totales proyectados).
   * **Indicador de Tesis:** Costo total unitario por metro cúbico de agua potable producida ($\text{USD/m}^3$ y $\$AR/\text{m}^3$), validando la viabilidad tecno-económica de la planta en comunidades rurales aisladas.

---

## 👥 7. Asignación de Roles y Responsabilidades del Equipo

* **Ing. Enzo (Codirector — Tesis Doctoral):**
  - Supervisión de la arquitectura determinista de firmware y el modelo de Darcy embebido.
  - Formulación y ajuste de los modelos de Hermia (bloqueo completo, estándar, intermedio y torta).
  - Coordinación de la sesión de Microscopía Electrónica de Barrido (SEM / EDS) en el centro de microscopía.
  - Validación del análisis de varianza (ANOVA) y optimización tecno-económica del SEC.
* **Owen Cañizares (Tesista de Grado):**
  - Instalación mecánica de los manómetros de glicerina $P_1$ y $P_2$, válvula de aguja y válvula de alivio calibrada a $0.70\text{ bar}$.
  - Operación del firmware V6.4, carga asistida de presiones manométricas en la interfaz web SCADA y verificación de cálculo en vivo de Darcy.
  - Medición eléctrica de potencia y cálculo de curvas de $\text{SEC}\ [\text{kWh/m}^3]$ en el Hito 6.
  - Preparación de muestras de fibras capilares para la fractura criogénica y observación SEM/EDS.
* **Antonella Guitián (Tesista de Grado):**
  - Preparación de suspensiones turbias de ensayo y optimización de dosis de coagulante en Jar Test (Hito 3).
  - Ejecución de la Corrida Testigo sin pretratamiento vs. las 3 réplicas con *Opuntia ficus-indica* en el Punto Central.
  - Medición de turbidez, conductividad TDS y balances gravimétricos ($0.45\ \mu\text{m}$, secado $105^\circ\text{C}$).
  - Estimación de costos operativos unitarios ($\text{OPEX}$), frecuencia de CIP y redacción del capítulo industrial de tesis.

---

## 📅 8. Cronograma Consolidado de Hitos y Tareas Secuenciales

| Semana | Hito / Fase | Actividad Técnica y Experimental | Responsables | Entregable Clave |
| :---: | :---: | :--- | :---: | :--- |
| **Semana 1** | **Subhito 2.4 & SCADA** | Instalación de manómetros de glicerina $P_1, P_2$, válvula de alivio a 0.7 bar, prueba de carga asistida en SCADA. | Owen / Enzo | SCADA calculando Darcy y CSV exportando presiones. |
| **Semana 2** | **Hito 5 (Fase 0 & 1)** | Ensayos de agua limpia CWF (determinación de $R_{m,0}$) y ensayo de flujo crítico escalonado $J_c$. | Todo el equipo | Curva $J_{20}$ vs TMP y punto de quiebre de flujo crítico. |
| **Semana 3** | **Hito 3 ◄► Hito 5** | Corrida Testigo (agua cruda directa) vs. 3 réplicas Punto Central con *Opuntia* (70 RPM, 0.25 bar). | Antonella / Owen | Cuantificación de $\Delta R_{\text{torta}}$ y atenuación de colmatación. |
| **Semana 4** | **Hito 5 (Fase 2)** | Ejecución de la matriz factorial completa $3^2$ (13 corridas totales con recirculación de permeado). | Antonella / Owen | Datalogger CSV completo de las 13 sesiones experimentales. |
| **Semana 5** | **Subhito 5.5 & Hito 6** | Ajuste cinético de Hermia ($n=2, 1.5, 1, 0$), ANOVA/RSM 3D, cálculo de $\text{SEC}\ [\text{kWh/m}^3]$ y OPEX. | Enzo / Antonella / Owen | Superficies de respuesta 3D y evaluación económica. |
| **Semana 6** | **Subhito 5.6 & Cierre** | Autopsia destructiva SEM/EDS de fibras FX100, medición de espesor $\delta_c$ y consolidación final de tesis. | Todo el equipo | Micrografías SEM a 300 DPI, espectros EDS y borrador final. |

---

## 🔗 9. Trazabilidad de Archivos y Enlaces Directos en el Repositorio

* 📐 **Arquitectura General, Estructura DFM y P&ID (Hito 0):** [`00_General_y_P_ID_Planta/README.md`](./00_General_y_P_ID_Planta/README.md)
* 🏗️ **Memoria Maestra de Estructura y Modularidad Mecánica:** [`00_General_y_P_ID_Planta/01_Estructura_y_Montaje_Filtro/DOCUMENTO_MAESTRO_ESTRUCTURA_FILTRO.md`](./00_General_y_P_ID_Planta/01_Estructura_y_Montaje_Filtro/DOCUMENTO_MAESTRO_ESTRUCTURA_FILTRO.md)
* 📐 **Diagrama P&ID ISA 5.1 y Simulador Interactivo:** [`00_General_y_P_ID_Planta/02_Diagrama_PID_y_Filosofia_Control/diagrama_pid_interactivo.html`](./00_General_y_P_ID_Planta/02_Diagrama_PID_y_Filosofia_Control/diagrama_pid_interactivo.html)
* 💻 **Firmware Activo en Operación (V6.4):** [`02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/Subhito2_2_v6/EN_USO_firmware_planta/`](./02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/Subhito2_2_v6/EN_USO_firmware_planta/)
* 🚰 **Guía de Montaje Manómetros de Glicerina:** [`02_Hito2_Instrumentacion_Sensores/01_Guias_Montaje_y_Calibracion/Guia_Subhito2_4_Manometros_Glicerina_y_Darcy_SCADA.md`](./02_Hito2_Instrumentacion_Sensores/01_Guias_Montaje_y_Calibracion/Guia_Subhito2_4_Manometros_Glicerina_y_Darcy_SCADA.md)
* 🌿 **Protocolo de Coagulación con Opuntia (Hito 3):** [`03_Hito3_Reactor_Sedimentador_Agitador/01_Protocolos_y_Teoria_Floculacion/Protocolo_Ensayo_Coagulacion_Floculacion_JarTest.md`](./03_Hito3_Reactor_Sedimentador_Agitador/01_Protocolos_y_Teoria_Floculacion/Protocolo_Ensayo_Coagulacion_Floculacion_JarTest.md)
* 🧪 **Protocolo Completo Darcy y Ensayos (Hito 5):** [`05_Hito5_Ensayos_Membrana_VidaUtil/01_Protocolo_Ensayos_Darcy_y_Fouling/Protocolo_Ensayos_Darcy_y_VidaUtil.md`](./05_Hito5_Ensayos_Membrana_VidaUtil/01_Protocolo_Ensayos_Darcy_y_Fouling/Protocolo_Ensayos_Darcy_y_VidaUtil.md)
* 📈 **Modelado Cinético Hermia y RSM (Subhito 5.5):** [`05_Hito5_Ensayos_Membrana_VidaUtil/03_Modelado_Hermia_y_Optimizacion_RSM/Guia_Modelado_Hermia_y_Superficie_Respuesta.md`](./05_Hito5_Ensayos_Membrana_VidaUtil/03_Modelado_Hermia_y_Optimizacion_RSM/Guia_Modelado_Hermia_y_Superficie_Respuesta.md)
* 🔬 **Protocolo de Autopsia SEM / EDS FX100 (Subhito 5.6):** [`05_Hito5_Ensayos_Membrana_VidaUtil/04_Autopsia_SEM_EDS_Fibras_Helixone/Protocolo_Autopsia_SEM_EDS_Membrana_FX100.md`](./05_Hito5_Ensayos_Membrana_VidaUtil/04_Autopsia_SEM_EDS_Fibras_Helixone/Protocolo_Autopsia_SEM_EDS_Membrana_FX100.md)
* 🏭 **Hito 6: Evaluación Industrial, Consumo Energético y Costos:** [`06_Hito6_Evaluacion_Industrial_Energia_Costos/README.md`](./06_Hito6_Evaluacion_Industrial_Energia_Costos/README.md)
* ⚡ **Cálculo de Consumo Específico SEC (Subhito 6.1):** [`06_Hito6_Evaluacion_Industrial_Energia_Costos/01_Consumo_Energetico_SEC/Guia_Calculo_SEC_Consumo_Especifico.md`](./06_Hito6_Evaluacion_Industrial_Energia_Costos/01_Consumo_Energetico_SEC/Guia_Calculo_SEC_Consumo_Especifico.md)
* 💰 **Vida Útil y Costos Operativos OPEX (Subhito 6.2):** [`06_Hito6_Evaluacion_Industrial_Energia_Costos/02_Costos_y_Vida_Util/Guia_Vida_Util_Frecuencia_CIP_y_Costos_Operativos.md`](./06_Hito6_Evaluacion_Industrial_Energia_Costos/02_Costos_y_Vida_Util/Guia_Vida_Util_Frecuencia_CIP_y_Costos_Operativos.md)

