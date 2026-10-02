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

## 🧭 3. Hito 2.3: Instrumentación de Presión Transmembrana (TMP) y Conversor ADS1115

```
                                  CONEXIÓN SINGLE-ENDED AL ADS1115
   
   +5V ────────┬──────────────┬──────────────┬───────────────────────────────┐
               │              │              │                               │
            [P1: FEED]     [P2: RET.]     [P3: PERM.]                     [Sonda TDS]
          (0.5 - 4.5V)   (0.5 - 4.5V)   (0.5 - 4.5V)                      (0 - 2.3V)
               │              │              │                               │
             [ 10k ]        [ 10k ]        [ 10k ]                           │
               ├── V_div      ├── V_div      ├── V_div                       │
             [ 20k ]        [ 20k ]        [ 20k ]                           │
               │              │              │                               │
   GND ────────┴──────────────┴──────────────┴───────────────────────────────┴──────────
               │              │              │                               │
               ▼              ▼              ▼                               ▼
            ADS1115        ADS1115        ADS1115                         ADS1115
            Canal A0       Canal A1       Canal A2                        Canal A3
         (0.33 - 2.97V) (0.33 - 2.97V) (0.33 - 2.97V)                   (0 - 2.3V)
```

### 3.1. Topología de Conexión: Single-Ended vs. Diferencial (Veredicto Técnico)
Tras la auditoría cruzada con los tres modelos de IA (Astra, GLM 5.3 y Claude), se ratifica de forma **unánime la configuración Single-Ended** utilizando los 4 canales analógicos del ADS1115 (dirección I2C `0x48`):

* **Canal A0:** Presión de Alimentación ($P_1$ o $P_{\text{feed}}$).
* **Canal A1:** Presión de Retentado ($P_2$ o $P_{\text{ret}}$).
* **Canal A2:** Presión de Permeado ($P_3$ o $P_{\text{perm}}$).
* **Canal A3:** Sonda de Calidad de Agua / Conductividad TDS (o riel de 5V para compensación ratiométrica).

#### Justificación Matemática y de Proceso:
1. **Definición de Presión Transmembrana en Módulos Capilares:**
   $$\text{TMP} = \frac{P_1 + P_2}{2} - P_3$$
   Una medición diferencial de hardware solo permite medir pares fijos como $(P_1 - P_2)$. Para obtener la TMP por hardware diferencial se requerirían dos chips ADS1115 separados, ya que la referencia $P_3$ no puede conectarse simultáneamente a dos pares analógicos en el mismo multiplexor interno.
2. **Diagnóstico Hidráulico Esencial:**
   El modo Single-Ended preserva las presiones manométricas independientes respecto a la atmósfera. Esto es indispensable para:
   * Evaluar la pérdida de carga hidráulica longitudinal a lo largo del lumen de la fibra: $\Delta P_{\text{lumen}} = P_1 - P_2$. Si $\Delta P_{\text{lumen}}$ crece rápidamente, diagnostica atascamiento en la entrada del haz capilar.
   * Diagnosticar cavitación o succión negativa en la línea de aspiración ($P_1 < 0$).
   * Medir $P_1$ de pico para la protección estructural de la membrana.
   * Detectar contrapresión anormal en la línea de permeado ($P_3 > 0.05\text{ bar}$).

### 3.2. Especificaciones Críticas para Compras (¡Alerta de Adquisición!)
> [!CAUTION]
> **Riesgo de Pérdida Experimental:** NO adquirir transductores industriales genéricos de **0 a 6 bar** o **0 a 1.2 MPa (12 bar)**.
> * Los sensores comerciales piezoeléctricos tienen un error de precisión típico de $\pm 1.0\%$ a $\pm 2.0\%$ a Fondo de Escala (Full Scale - FS).
> * En un sensor de 6 bar ($6000\text{ mbar}$), un error de $\pm 1\%$ FS representa una incertidumbre absoluta de **$\pm 60\text{ mbar}$ ($\pm 0.06\text{ bar}$)**.
> * En filtración capilar con membrana FX100, la TMP de trabajo se sitúa entre **$0.10\text{ bar}$ y $0.40\text{ bar}$** ($100\text{ a }400\text{ mbar}$).
> * ¡Una incertidumbre de $\pm 60\text{ mbar}$ introduce un **error relativo del 24% al 60%** en la variable de respuesta central de la tesis!

#### Especificación Oficial para el Pedido de Materiales:
* **Rango Manométrico:** **0 a 1.0 bar (0 a 100 kPa / 0 a 14.5 psi)**. Alternativa admisible: **0 a 1.2 bar**.
* **Tipo de Salida:** Ratiométrica lineal $0.5\text{ V}$ a $4.5\text{ V}$ con alimentación de $+5\text{ VDC}$ (o transmisores $4-20\text{ mA}$ con resistencia shunt de $150\ \Omega$ al 0.1%).
* **Conexión Hidráulica:** Rosca macho $G 1/4"$ en acero inoxidable o latón niquelado.
* **Precisión:** $\le \pm 1.0\%$ FS (incertidumbre absoluta $\le \pm 10\text{ mbar}$, equivalente a un error relativo $< 4\%$ a $0.25\text{ bar}$ de TMP).

### 3.3. Interfaz Eléctrica Segura (Adaptación 5V a 3.3V)
Dado que los transductores de 0-1 bar operan con salida ratiométrica de $0.5\text{ V}$ a $4.5\text{ V}$ alimentados con $5\text{ V}$, si el ADS1115 se alimenta con $3.3\text{ V}$, cualquier tensión superior a $V_{DD} + 0.3\text{ V}$ destruirá los diodos de protección ESD de las entradas analógicas.

Se establecen dos alternativas de conexión para el montaje de Owen:
* **Alternativa A (Recomendada por Simplicidad - Divisor Resistivo de Precisión):**
  - Alimentar el ADS1115 a $3.3\text{ V}$ y configurar la ganancia interna PGA en `GAIN_ONE` (rango $0 \text{ a } 4.096\text{ V}$, $0.125\text{ mV/LSB}$).
  - Interponer un divisor de tensión en cada canal analógico ($P_1, P_2, P_3$):
    $$R_{\text{alto}} = 10\text{ k}\Omega \ (\text{tolerancia } 1\% \text{ o } 0.1\%) \quad , \quad R_{\text{bajo}} = 20\text{ k}\Omega \ (\text{tolerancia } 1\% \text{ o } 0.1\%)$$
    Factor de atenuación: $\frac{20}{10 + 20} = \frac{2}{3} \approx 0.6667$.
  - Con esto, la señal de $0.5 - 4.5\text{ V}$ se escala linealmente a **$0.333\text{ V} - 3.000\text{ V}$**, perfectamente dentro del rango seguro del ADS1115 alimentado a $3.3\text{ V}$.
  - Agregar un capacitor cerámico de $10\text{ nF}$ a $100\text{ nF}$ en paralelo con $R_{\text{bajo}}$ para supresión de ruido de conmutación.
* **Alternativa B (Level Shifter I2C):**
  - Alimentar el ADS1115 a $+5\text{ V}$ con ganancia `GAIN_ONE`.
  - Conectar los sensores directamente sin divisores a A0, A1, A2.
  - Interponer un módulo desplazador de nivel bidireccional MOSFET (tipo BSS138 de 4 canales) entre el bus I2C del ESP32 (3.3V) y el ADS1115 (5V).

### 3.4. Montaje Físico y Tara Hidrostática en NVS
* **Cota Geodésica Común:** Los tres transductores deben instalarse rigurosamente a la **misma altura física / cota de referencia**. Una diferencia de altura de apenas $10\text{ cm}$ en la columna de agua genera un error estático de $\approx 10\text{ mbar}$ ($1\text{ kPa}$), lo que representa un $20\%$ de sesgo en una TMP de $0.05\text{ bar}$.
* **Tara Automática de Cero en NVS:** Con la bomba detenida y el circuito hidráulico completamente cebado con agua, se ejecuta una rutina de puesta a cero en la UI que almacena los offsets de reposo en la memoria Flash NVS (`Preferences.h`).

### 3.5. Driver I2C No Bloqueante & Registros ADS1115
Para configurar correctamente las lecturas Single-Ended en el ADS1115:
* **Palabra de Configuración Single-Ended (128 SPS, PGA 4.096V):**
  El campo MUX para Single-Ended respecto a GND es `100 + canal`:
  $$\text{Registro} = 0\text{x}8000 \ | \ ((4 + \text{ch}) \ll 12) \ | \ 0\text{x}0200 \ | \ 0\text{x}0100 \ | \ (\text{DR} \ll 5) \ | \ 0\text{x}0003$$
  - Canal A0 (128 SPS): `0xC383`
  - Canal A1 (128 SPS): `0xD383`
  - Canal A2 (128 SPS): `0xE383`
  - Canal A3 (128 SPS): `0xF383`
* **Muestreo a 250 SPS en Round-Robin:** Conversión de $4\text{ ms}$ por canal, barriendo los 3 canales de presión a $\approx 55\text{ Hz}$. Esto permite registrar no solo el promedio, sino el **valor pico de $P_1$** causado por la pulsación de los 3 rodillos del cabezal peristáltico ($1.25 - 5\text{ Hz}$).
* **Recuperación Automática de Bus I2C Trabado:** Si la línea SDA queda retenida en nivel bajo, el firmware libera los pines y conmuta SCL durante **9 ciclos de reloj** con SDA en alto para forzar la liberación del esclavo, reinicializando luego el bus `Wire`.

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

### 5.5. Fase 2: Matriz Experimental Factorial Completa ($3^2$ con Puntos Centrales)
Para optimizar el esfuerzo de laboratorio, evitar esquinas infactibles y preservar las membranas:

* **Factor A: Caudal de Impulsión / Esfuerzo de Corte ($Q_{\text{feed}}$)**
  - Nivel Bajo ($-1$): **$40\text{ RPM}$** ($544\text{ mL/min}$, $\dot{\gamma}_w \approx 1060\text{ s}^{-1}$ — *desplazado desde 25 RPM para evitar recuperación $>100\%$ a alta TMP*).
  - Nivel Medio ($0$): **$70\text{ RPM}$** ($952\text{ mL/min}$, $\dot{\gamma}_w \approx 1850\text{ s}^{-1}$).
  - Nivel Alto ($+1$): **$95\text{ RPM}$** ($1292\text{ mL/min}$, $\dot{\gamma}_w \approx 2560\text{ s}^{-1}$).
* **Factor B: Presión Transmembrana ($\text{TMP}$)**
  - Nivel Bajo ($-1$): $0.10\text{ bar}$ ($100\text{ mbar}$).
  - Nivel Medio ($0$): $0.25\text{ bar}$ ($250\text{ mbar}$).
  - Nivel Alto ($+1$): $0.40\text{ bar}$ ($400\text{ mbar}$).

#### Plan de Ejecución (12 Ensayos Aleatorizados con Recirculación Total):
* 9 tratamientos de la matriz $3 \times 3$.
* **3 réplicas en el punto central (70 RPM, 0.25 bar)** distribuidas al inicio, mitad y final (Total = **12 corridas**) para estimar el error experimental puro y evaluar curvatura cuadrática mediante Metodología de Superficie de Respuesta (RSM).
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

## 👥 6. Asignación de Roles y Responsabilidades del Equipo

* **Ing. Enzo (Codirector — Tesis Doctoral):**
  - Supervisión de la arquitectura FreeRTOS determinista y seguridad de control en Core 1.
  - Aprobación de especificaciones de compras de transductores 0-1 bar y válvula de alivio mecánica calibrada a 0.70 bar.
  - Modelado avanzado de transporte, regresión no lineal y ANOVA del diseño factorial.
* **Owen Cañizares (Tesista de Grado):**
  - Montaje de transductores de presión y divisores resistivos ($10\text{ k}\Omega / 20\text{ k}\Omega$) en bornera Placa 2.
  - Conexión del bus I2C al ADS1115 y calibración/tara hidrostática en banco con columna de agua.
  - Instalación de la válvula de alivio a 0.7 bar y pulsador de emergencia (E-Stop) sobre la etapa de potencia DM860.
  - Calibración gravimétrica de la curva $K(f)$ del caudalímetro de permeado (5 a 8 puntos con balanza).
  - Compilación y mantenimiento del firmware mediante PlatformIO CLI.
* **Antonella Guitián (Tesista de Grado):**
  - Preparación de suspensiones turbias de ensayo y optimización de dosis de coagulante en Jar Test (Hito 3).
  - Configuración del circuito en recirculación total de permeado para los ensayos a concentración constante.
  - Ejecución de balances gravimétricos en laboratorio (filtración $0.45\ \mu\text{m}$, secado $105^\circ\text{C}$ y pesaje de torta $M_s$).
  - Monitoreo de calidad de agua: turbidez de entrada/salida (CAA Art. 982), conductividad TDS y registro en planillas experimentales.

---

## 📅 7. Cronograma de Hitos y Tareas Secuenciales

| Semana | Hito / Fase | Actividad Técnica | Responsable | Entregable Clave |
| :---: | :---: | :--- | :---: | :--- |
| **Semana 1** | **Seguridad & Compras** | Compra de transductores 0-1 bar e instalación de válvula de alivio a 0.7 bar / E-Stop. | Owen / Enzo | Circuito hidráulico protegido contra sobrepresión. |
| **Semana 1** | **Hito 2.3 (Firmware)** | Implementación FreeRTOS dual-core (Core 1 a prio 19), enclavamientos a 50 ms y FSM ADS1115. | Enzo / Antigravity | Firmware compilado sin warnings en PlatformIO. |
| **Semana 2** | **Hito 2.3 (Banco)** | Calibración de transductores P1, P2, P3 con columna de agua, tara hidrostática y parada dura. | Owen / Antonella | Curvas manométricas y validación de parada en $<50\text{ ms}$. |
| **Semana 2** | **Hito 5 (Fase 0)** | Ensayo de agua limpia CWF, determinación de $R_{m,0}$ y validación $R^2 \ge 0.985$ e intercepto ≈ 0. | Todo el equipo | Curva $J_{20}$ vs TMP y valor basal de resistencia. |
| **Semana 3** | **Hito 5 (Fase 1)** | Ensayo de Flujo Escalonado con histéresis para determinar Flujo Crítico $J_c$ (recirculación total). | Antonella / Owen | Curva $J$ vs TMP con punto de quiebre y ley de corte. |
| **Semana 3-4**| **Hito 5 (Fase 2)** | Ejecución de Matriz Factorial $3^2$ (12 ensayos con 3 puntos centrales y balances gravimétricos). | Antonella / Owen | Planillas de datos completos y balances de masa. |
| **Semana 5** | **Hito 5 (Fase 3)** | Modelado de compresibilidad $\alpha = \alpha_0 (\Delta P)^s$, ajuste de Hermia y redacción final. | Todo el equipo | Gráficas vectoriales a 300 DPI y borrador de tesis. |

---

## 🔗 8. Trazabilidad de Archivos y Enlaces Directos en el Repositorio

* 💻 **Firmware Blindado Activo (v4):** [`02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v4/EN_USO_firmware_planta/`](./02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v4/EN_USO_firmware_planta/)
* 🤖 **Auditorías Externas de IA (Rondas 1, 2 y 3):**
  - **Ronda 3 (Cierre Maestro - Claude 5.5 Opus):** [`02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v4/AuditoriaIA/ronda3_claude_opus.md`](./02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v4/AuditoriaIA/ronda3_claude_opus.md)
  - Astra: [`02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v4/AuditoriaIA/Ronda2_Astra.md`](./02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v4/AuditoriaIA/Ronda2_Astra.md)
  - GLM 5.3: [`02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v4/AuditoriaIA/ronda2_glm5.3.txt`](./02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v4/AuditoriaIA/ronda2_glm5.3.txt)
  - Claude Sonnet 5: [`02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v4/AuditoriaIA/ronda2_claudesonet5.txt`](./02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v4/AuditoriaIA/ronda2_claudesonet5.txt)
* 📋 **Manual de Organización y Protocolo Git del Equipo:** [`ORGANIZACION_PROYECTO_Y_EQUIPO.md`](./ORGANIZACION_PROYECTO_Y_EQUIPO.md)
* 🩺 **Diagnóstico Físico y Filtro RC para Tesis:** [`02_Hito2_Instrumentacion_Sensores/01_Guias_Montaje_y_Calibracion/Diagnostico_y_Resolucion_Problemas_Instrumentacion.md`](./02_Hito2_Instrumentacion_Sensores/01_Guias_Montaje_y_Calibracion/Diagnostico_y_Resolucion_Problemas_Instrumentacion.md)
* 🧪 **Protocolo Completo del Hito 5:** [`05_Hito5_Ensayos_Membrana_VidaUtil/01_Protocolo_Ensayos_Darcy_y_Fouling/Protocolo_Ensayos_Darcy_y_VidaUtil.md`](./05_Hito5_Ensayos_Membrana_VidaUtil/01_Protocolo_Ensayos_Darcy_y_Fouling/Protocolo_Ensayos_Darcy_y_VidaUtil.md)
* 📑 **Planilla Maestra de Datos de Tesis:** [`05_Hito5_Ensayos_Membrana_VidaUtil/02_Planillas_y_Datos_Tesis/plantilla_datos_ensayo_tesis.csv`](./05_Hito5_Ensayos_Membrana_VidaUtil/02_Planillas_y_Datos_Tesis/plantilla_datos_ensayo_tesis.csv)
