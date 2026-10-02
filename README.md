# 💧 PLANTA PILOTO DE ULTRAFILTRACIÓN INDUSTRIAL (FX100) & REACTOR DE COAGULACIÓN-SEDIMENTACIÓN
## Proyecto de Tesis de Grado en Ingeniería Industrial / Química — Automatización, Control IoT & Modelado de Membranas
**Tesistas de Grado:** Antonella Guitián & Owen Cañizares  
**Codirector de Tesis:** Ing. Enzo *(Investigador Doctoral — Tesis Doctoral en Procesos de Separación por Membranas)*  
**Asesor de Automatización e Instrumentación:** Antigravity AI  
**Ubicación de Montaje:** Laboratorio de Ingeniería — Salta, Argentina  
**Última Actualización:** Septiembre 2026  

---

## 🌐 1. Acceso Rápido al Servidor Web SCADA del ESP32

Para operar la planta piloto, ver los caudalímetros en vivo, arrancar la bomba y exportar los datos a Excel, el ESP32 levanta una red Wi-Fi propia y simultáneamente se enlaza a la red del laboratorio:

```
                  ┌───────────────────────────────────────────────┐
                  │             CÓMO CONECTARSE AL SCADA          │
                  └───────────────────────────────────────────────┘
                                          │
                  ┌───────────────────────┴───────────────────────┐
                  ▼                                               ▼
     [ MODO 1: Wi-Fi Propio Directo ]               [ MODO 2: Router de Laboratorio ]
     (Ideal en banco sin router)                    (Notebook conectada al Box804)
     • Red Wi-Fi: Bomba_Peristaltica_UF             • Red Wi-Fi: Box804
     • Clave: plantapiloto2                         • Clave: plantapiloto2
     • En tu navegador ingresa a:                   • En tu navegador ingresa a:
       👉 http://192.168.4.1                          👉 http://bomba.local
```

> [!TIP]
> **Consejo para celulares**: Si te conectas a la red `Bomba_Peristaltica_UF` desde el teléfono móvil y la página no abre, desactiva momentáneamente los **Datos Móviles (4G/5G)** para que el teléfono no intente buscar la IP en internet.

---

## 🧠 2. ¿Por qué usamos Git y Control de Versiones en esta Tesis?
*(Sección de lectura obligatoria para Antonella y Owen)*

Muchas veces surge la pregunta: *¿Por qué no guardamos simplemente carpetas como `codigo_final_v2_este_si.ino` en un pendrive?*  
En un proyecto de ingeniería industrial y aplicada, **el control de versiones con Git no es una herramienta para programadores informáticos; es la Bitácora de Laboratorio Digital e Inalterable**.

### Las 4 razones fundamentales de ingeniería:
1. **Trazabilidad Científica y Defensa de Tesis**:  
   Ante el jurado evaluador, ustedes no están presentando un proyecto escolar que "funciona por casualidad". Git registra con fecha y hora exacta cada hipótesis, cada ensayo, cada fallo de ruido y la justificación matemática de cómo se solucionó.
2. **Registro de Fallos y Diagnóstico Reproducible**:  
   En la experimentación de laboratorio, si un ensayo funcionaba el lunes y el miércoles deja de funcionar, con Git se compara exactamente qué línea de código, qué resistencia o qué parámetro mecánico cambió en el camino.
3. **Seguridad Absoluta (Cero Pérdida de Trabajo)**:  
   Si una modificación de prueba desestabiliza el motor o rompe la lógica de la bomba, volver al estado anterior 100% estable toma un solo segundo con `git checkout`, sin temor a haber destruido horas de trabajo.
4. **Trabajo Colaborativo en Paralelo**:  
   Permite que Enzo supervise y corrija la arquitectura, mientras Antonella y Owen cargan datos de calibración o actualizan manuales, sin pisarse ni duplicar archivos.

---

## 📜 3. Bitácora de Commits Clave y Solución de Problemas (Changelog de Ingeniería)

Esta tabla resume la evolución cronológica del sistema, los desafíos encontrados en el banco de pruebas y las soluciones implementadas:

| Fecha | Commit | Desafío de Ingeniería Encontrado | Solución Técnica Implementada en el Repositorio |
| :---: | :---: | :--- | :--- |
| **01/10/2026** | *(Actual)* | **Hoja de Ruta Técnica Hito 2.3 (FreeRTOS/ADS1115) e Hito 5 (Darcy/Factorial)**: Consolidación de la auditoría cruzada (ChatGPT 6 Astra & GLM 5.3) para la instrumentación de presiones y ensayos de membrana. | Creación de [`HOJA_DE_RUTA_DESARROLLO_Y_ENSAYOS.md`](./HOJA_DE_RUTA_DESARROLLO_Y_ENSAYOS.md): Elección unánime de ADS1115 Single-Ended (A0-A3), alerta crítica de compra para transductores 0-1 bar (evita error del 48% a TMP de trabajo), desacople FreeRTOS dual-core mediante Seqlock atómico y diseño experimental factorial $3^2$ (11 ensayos con 3 puntos centrales, cálculo de flujo crítico $J_c$, compresibilidad coloidal $\alpha$ y protocolo de limpieza con NaOCl). |
| **01/10/2026** | `bfaee32` | **Auditoría Cruzada Round 2 y Firmware Blindado v4**: Rampa ampliada a 100 RPM para diseño factorial, blindaje atómico y modelo físico de transporte. | Desarrollo de [`subhito2_2_v4/`](./02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v4/): Rango hasta 100 RPM ($\approx 1360\text{ mL/min}$) con alarma clínica en 44 RPM, período recíproco en caudalímetros (<0.1% a 5.5 Hz) inmune a desbordes de `micros()`, sección crítica por instancia (`portMUX_TYPE`), módulo Darcy (`darcy.h`) con ecuación de Vogel y corrección TCF, y cero fragmentación de memoria heap. |
| **01/10/2026** | `336a537` | **Integración de Calibración de Owen, Auto-Calibración y Persistencia Flash NVS**: Owen calibró caudalímetros en probeta a 50 RPM ($680\text{ mL/min}$) obteniendo $K_{\text{alim}} = 154.62$, $K_{\text{perm}} = 55.00$ y cilindrada real de $13.6\text{ mL/rev}$, incorporando almacenamiento no-volátil en Flash NVS (`Preferences.h`), datalogger multi-sesión y optimización de filtro a 2000 µs. | Se integró formalmente la rama de Owen a `main`. Se aplicó ajuste de seguridad preventiva en `config.h`: `RPM_INICIO = 25.0f` ($\approx 340\text{ mL/min}$) para resguardar la membrana Fresenius FX100 ($600\text{ mL/min}$ / $36\text{ RPM}$). Se consolidó [`firmware_planta/`](./02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/firmware_planta/) como versión oficial activa y se integró [`Capitulo_4_Programacion_Firmware/`](./Capitulo_4_Programacion_Firmware/) con simulaciones LTspice y bitácoras. |
| **30/09/2026** | `0629e00` | **Ruido de 81 Hz (835.5 mL/min) en Reposo e Inversión de Canales**: Con bomba detenida, Feed registraba 81.9 Hz constante por acoplamiento de zumbido de 50 Hz/100 Hz que saturaba el filtro ISR de 12 ms ($1/0.0122\text{ s} \approx 81.9\text{ Hz}$). Además, los datasets del CSV revelaron $Q_{\text{perm}} \gg Q_{\text{feed}}$ ($627\text{ mL/min}$ en P27 vs $0\text{ mL/min}$ en P14). | Se documentó formalmente el modelado matemático para la tesis en [Diagnostico_y_Resolucion_Problemas_Instrumentacion.md](./02_Hito2_Instrumentacion_Sensores/01_Guias_Montaje_y_Calibracion/Diagnostico_y_Resolucion_Problemas_Instrumentacion.md). Se activó `INPUT_PULLUP` interno como doble barrera en firmware, se corrigió el cruce de cables amarillos de señal entre borneras y se protocolizó el filtro RC ($4.7\text{ k}\Omega + 100\text{ nF}$) y masa común rígida ($0.0\ \Omega$). |
| **29/09/2026** | `32a1994` | **Torque Ripple y Ruido en Bajas RPM**: Al operar a $20-35\text{ RPM}$ con manguera de $12\text{ mm}$, el NEMA 34 vibraba por pasos discretos a 1600 pulsos/rev. | Se actualizó `config.h` a **3200 pulsos/rev** (16 micropasos en Leadshine DM860: `SW5=OFF, SW6=OFF, SW7=ON, SW8=ON`), logrando un giro ultrasuave y silencioso. |
| **28/09/2026** | `d731cdb`<br>`f6ec04f` | **Ruido EMI en Caudalímetros YF-S401**: El chopper de conmutación del motor (3A inductivos) inducía pulsos falsos en los pines con pull-up interno débil ($45\text{ k}\Omega$). | Diseño del **Módulo Front-End** con la segunda bornera ZS-1057: resistencias de pull-up externas de **$4.7\text{ k}\Omega$ a 3.3V** + filtro pasabajos RC con capacitor cerámico de **$100\text{ nF}$**. |
| **28/09/2026** | `3047a8e`<br>`3f3013a` | **Bloqueo en Inversión de Sentido**: Si el operador presionaba STOP durante la rampa de frenado a 0 RPM para invertir giro, la consigna quedaba huérfana en 0 RPM. | Corrección de la máquina de estados finitos (FSM) de la bomba: fallback seguro a `RPM_INICIO` y totalizador de volumen desacoplado. |
| **25/09/2026** | `5bfc067`<br>`ed5e8d6` | **Límite de Caudal de la Membrana FX100 ($600\text{ mL/min}$)**: La manguera de $12\text{ mm}$ desplaza $15.4\text{ mL/vuelta}$. A más de 36 RPM se supera el caudal seguro del filtro capilar. | Adaptación de la rampa SCADA a rango $20 - 42\text{ RPM}$, incorporación de alerta visual de caudal crítico y exportador CSV para planillas Excel. |
| **24/09/2026** | `81949db`<br>`4f47370` | **Sentido de Giro Invertido y Error de Unidades**: Al cablear el DM860 en Cátodo Común, el pin DIR arrancaba en sentido antihorario (retrolavado) y el caudal dividía por 60 dos veces. | Inversión lógica de `PIN_DIR` para arranque en Filtración horaria por defecto y corrección analítica de la ecuación de flujo ($Q = F / K$). |
| **23/09/2026** | `a0db29c` | **Código Monolítico Inmantenible**: El firmware inicial tenía más de 800 líneas en un solo archivo `.ino`, dificultando el aislamiento de errores. | Refactorización completa en arquitectura modular C++ orientada a objetos: `bomba.cpp`, `caudalimetro.cpp`, `config.h` e `index_html.h`. |

---

## 🗺️ 4. Hoja de Ruta Maestra: Transición a Hito 2.3 (FreeRTOS & ADS1115) e Hito 5 (Darcy & Factorial)

> 📘 **Documento Maestro de Ingeniería:** Para consultar el desglose exhaustivo de ecuaciones, especificaciones de compras de sensores, arquitectura determinista y protocolos de laboratorio, consultar:  
> 👉 **[`HOJA_DE_RUTA_DESARROLLO_Y_ENSAYOS.md`](./HOJA_DE_RUTA_DESARROLLO_Y_ENSAYOS.md)**

### Síntesis de Directrices Técnicas y Decisiones de Auditoría IA:

```
                                  MAPA DE RUTA TÉCNICA
  
      [ SUBHITO 2.2 V3 ] ──► [ SUBHITO 2.2 V4 ] ──► [ HITO 2.3 FREERTOS ] ──► [ HITO 5 ENSAYOS ]
      • Calibrado probeta    • Rampa 100 RPM        • Core 0: Control 50ms     • Línea base Rm diaria
      • 50 y 72 RPM          • Darcy embebido       • Core 1: Web SCADA        • Flujo crítico Jc
      • NVS Flash            • Período recíproco    • ADS1115 Single-Ended     • Matriz 3x3 (11 corridas)
      • En banco físico      • Cero fragmentación   • Transductores 0-1 bar    • Compresibilidad torta
```

1. **Seguridad de Proceso y Desplazamiento Positivo (Dictamen Opus):**
   * **Protección Mecánica Calibrada a 0.70 bar:** A 4.5 Nm de torque, el motor revienta mangueras o fibras si se estrangula la aguja. Es obligatorio instalar una **válvula de alivio mecánica calibrada a $\approx 0.70\text{ bar}$** (contrastada con manómetro) y un **hongo de parada de emergencia (E-Stop)** cortando potencia al DM860. *(Se descartó calibrar a 1.0 bar porque la TMP alcanzaría 0.95 bar y destruiría la membrana).*
   * **Techo Dinámico e Histéresis (`_techo`):** La bomba opera autolimitada a **44 RPM**. Solo se desbloquea hasta 100 RPM si hay transductores con telemetría viva ($<1.5\text{ s}$); tras una pérdida de señal, baja a 44 RPM de inmediato y requiere 5 a 10 s continuos para rearmar.
   * **Parada Dura Inmediata (`paradaDura()`):** Disparo en $<50\text{ ms}$ si $P_1 \ge \mathbf{0.60\text{ bar}}$ (antes de saturar el sensor en 1.0 bar) o si $\text{TMP} \ge \mathbf{0.45\text{ bar}}$. El E-Stop queda enclavado de forma persistente y vacía la cola de comandos con `xQueueReset()`. Inversión de giro bloqueada con membrana conectada.

2. **Instrumentación de Presión y ADS1115 (Hito 2.3):**
   * **Modo Single-Ended Unánime:** Canales A0 ($P_{\text{alim}}$), A1 ($P_{\text{ret}}$), A2 ($P_{\text{perm}}$) y A3 (Conductividad TDS) usando palabra de control `(4 + ch) << 12` (`0xC383, 0xD383, 0xE383, 0xF383`). Permite calcular $\text{TMP} = \frac{P_1 + P_2}{2} - P_3$, supervisar la caída luminal $\Delta P = P_1 - P_2$ y vigilar cavitación en succión.
   * **Alerta Crítica de Compras:** Comprar transductores ratiométricos ($0.5 - 4.5\text{ V}$) de **0 a 1.0 bar (o máximo 0 a 1.2 bar)** con rosca $G 1/4"$. Evitar transductores estándar de 6 o 12 bar, cuyo error de fondo de escala ($\pm 60\text{ a }120\text{ mbar}$) falsearía la TMP de ensayo ($0.10 - 0.40\text{ bar}$) con errores relativos de hasta el 48%.
   * **Acondicionamiento Seguro 5V $\rightarrow$ 3.3V:** Divisor resistivo de precisión ($10\text{ k}\Omega / 20\text{ k}\Omega$ al $1\%$ o $0.1\%$) con capacitor de $100\text{ nF}$ para proteger el ADS1115 y suprimir ruido eléctrico.
   * **Cota Geodésica y Tara Hidrostática en NVS:** Montar los tres transductores a la misma altura física (10 cm de desnivel = 10 mbar = 20% de error) y registrar la tara a cero con líneas llenas en memoria Flash permanente.
   * **Driver I2C No Bloqueante (250 SPS):** Muestreo en round-robin capturando $P_{1,\text{pico}}$ (pulsaciones de rodillos a 1.25–5 Hz), timeout de bus de $50\text{ ms}$, rutina de recuperación de 9 pulsos de reloj en SCL y dead-man a los $1.5\text{ s}$.

3. **Arquitectura Concurrente FreeRTOS (Hito 2.3 / Hito 4):**
   * **Core 1 (`tareaControl`, Prioridad 19):** Lazo de control determinista a **50 ms (20 Hz)** mediante `vTaskDelayUntil()`. Prioridad 19 por encima del stack Wi-Fi lwIP para garantizar cero jitter. Control cinemático de bomba, FSM de presiones y **evaluación ininterrumpida de sobrepresión en cada ciclo de 50 ms (NUNCA a 1 Hz)**.
   * **Core 0 (`tareaWeb`, Prioridad 2):** Servidor HTTP `WebServer`, Wi-Fi SoftAP y datalogging NVS. Prohibidas las escrituras en Flash durante ensayos activos para no congelar la caché.
   * **Sincronización Lock-Free:** Snapshot plano (`SnapshotPlanta_t` $\le 300\text{ B}$) copiado bajo spinlock liviano `portMUX` (~1 µs). Banderas atómicas directas (`std::atomic<bool>`) para STOP y EMERGENCIA independientes de la cola de consignas.
   * **Metrología de Caudal y Permeado a Bajo Flujo:** El sensor YF-S401 opera por debajo de su rango útil ($<300\text{ mL/min}$) en permeado; se establece el uso de una **balanza de precisión en laboratorio como referencia primaria**, mientras el firmware calcula $\Delta\text{vol}/\Delta t$ en ventanas de 10 a 30 s con calibración poligonal $K(f)$.

4. **Protocolo Experimental de Modelado Darcy y Ensuciamiento (Hito 5):**
   * **Fase 0 (Línea Base Diaria $R_{m,0}$):** Ensayo previo obligatorio con agua destilada (5 escalones de TMP a $\ge 60\text{ RPM}$) para desacoplar el envejecimiento de la membrana. Criterios: $R^2 \ge 0.985$ e intercepto compatible con cero. $K_{\text{UF}} = 54750\text{ mL/(h}\cdot\text{bar)}$.
   * **Fase 1 (Flujo Crítico $J_c$):** Determinación por método escalonado (*flux-step*) ascendente y descendente con **recirculación obligatoria del permeado al tanque** para evitar la concentración artificial de la alimentación. La histéresis cuantifica el ensuciamiento irreversible.
   * **Fase 2 (Diseño Factorial $3^2$ Factible):** Caudal (40, 70, 95 RPM — *desplazado desde 25 RPM para evitar recuperación $>100\%$*) $\times$ TMP (0.10, 0.25, 0.40 bar) con **3 réplicas en el punto central (70 RPM, 0.25 bar)** sumando **12 corridas en total** analizadas por Metodología de Superficie de Respuesta (RSM).
   * **Fase 3 (Compresibilidad Coloidal y Limpieza):** Resistencia específica de torta $\alpha = \alpha_0 (\Delta P)^s$ mediante balances gravimétricos ($0.45\ \mu\text{m}$, $105^\circ\text{C}$). Enjuague físico rápido (*forward flush* + *backwash* hidrostático suave con depósito elevado $\le 1.5\text{ m} \approx 0.15\text{ bar}$), con criterio $R_{m,\text{post}} \le 1.05 R_{m,0}$ ($FRR \ge 95\%$), y regeneración química con $\text{NaOCl}$ ($100-200\text{ ppm}$, pH 10) con registro acumulado de dosis ($\text{ppm}\cdot\text{h}$).

---

## 🗂️ 5. Estructura del Repositorio y Guía de Navegación de Archivos

> 🧭 **Guía de Organización del Equipo:** Para conocer en detalle la distribución de archivos, responsabilidades y protocolos de Git entre Enzo, Owen y Antonella, consultar:  
> 👉 **[`ORGANIZACION_PROYECTO_Y_EQUIPO.md`](./ORGANIZACION_PROYECTO_Y_EQUIPO.md)**

```text
SistemaUF/
├── 📄 HOJA_DE_RUTA_DESARROLLO_Y_ENSAYOS.md   ◄── Hoja de ruta técnica de Hito 2.3 e Hito 5
├── 📄 ORGANIZACION_PROYECTO_Y_EQUIPO.md      ◄── Protocolo de trabajo en equipo y ramas Git
├── 📁 00_General_y_P_ID_Planta/              ◄── Planos ISA 5.1 y especificaciones membrana FX100
├── 📁 01_Hito1_Control_Accionamiento_NEMA34_DM860/ ◄── Motor, driver Leadshine, cabezal MBP-2000
├── 📁 02_Hito2_Instrumentacion_Sensores/      ◄── Instrumentación, acondicionamiento RC y firmwares
│   ├── 📁 01_Guias_Montaje_y_Calibracion/    ◄── Diagnóstico de ruido 81 Hz, LTspice y planos HTML
│   ├── 📁 02_Firmware_Test_Sensores/
│   │   ├── 📁 subhito2_2_v3/                 ◄── Firmware en operación en el banco de pruebas
│   │   └── 📁 subhito2_2_v4/                 ◄── Versión blindada con Darcy, 100 RPM y auditoría IA
│   └── 📁 Datos/                             ◄── Planillas de calibración en probeta y CSVs
├── 📁 03_Hito3_Reactor_Sedimentador_Agitador/ ◄── Coagulación-floculación Jar Test con Opuntia
├── 📁 04_Hito4_Integracion_Automatizacion_IoT/◄── Concurrencia FreeRTOS y enclavamientos
├── 📁 05_Hito5_Ensayos_Membrana_VidaUtil/    ◄── Protocolo experimental Darcy, Jc y matriz factorial
└── 📁 Archivado/                             ◄── Historial y códigos preliminares de respaldo
```

### Detalle de Carpetas:

#### 📐 `00_General_y_P_ID_Planta/`
* **Contenido**: Planos maestros de instrumentación y tuberías bajo norma ISA 5.1, bocetos originales de ingeniería, dimensionamiento de tanques, el [diagrama_pid_interactivo.html](./00_General_y_P_ID_Planta/diagrama_pid_interactivo.html) y las especificaciones oficiales de la membrana [ParametrosFiltroFX100.txt](./00_General_y_P_ID_Planta/ParametrosFiltroFX100.txt).
* **¿Para qué leerlo?**: Para entender el flujo global del agua, la ubicación de las válvulas, tomas de presión y balances de materia de toda la planta.

#### ⚡ `01_Hito1_Control_Accionamiento_NEMA34_DM860/`
* **Contenido**: Toda la ingeniería de impulsión: motor NEMA 34, driver Leadshine DM860, cabezal MBP-2000, cálculo de torque y el [Inventario_Consolidado.md](./01_Hito1_Control_Accionamiento_NEMA34_DM860/01_Hardware_y_Cableado/Inventario_Consolidado.md).
* **¿Para qué leerlo?**: Para consultar qué componentes tenemos en el laboratorio, qué ferretería falta comprar en Salta, y cómo se calculan las rampas de aceleración.

#### 📊 `02_Hito2_Instrumentacion_Sensores/` *(¡Carpeta en Operación y Calibración Actual!)*
* **Contenido**: 
  - 🌟 **[subhito2_2_v3 / EN_USO_firmware_planta/](./02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v3/EN_USO_firmware_planta/)**: **Firmware Oficial en Operación Física (Flasheado en ESP32)**. Calibración en probeta a 50 y 72 RPM, NVS Flash permanente (`Preferences.h`), filtro anti-ruido a 2000 µs, datalogger de 600 muestras y Web SCADA en SoftAP puro (`192.168.4.1`).
  - 🚀 **[subhito2_2_v4 / EN_USO_firmware_planta/](./02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v4/EN_USO_firmware_planta/)**: **Firmware Blindado (Hardened)**. Período recíproco con blindaje de rollover en `micros()`, sección crítica por instancia, rango hasta 100 RPM para diseño factorial, módulo físico de Darcy embebido (`darcy.h`) y cero fragmentación de memoria heap.
  - 🤖 **[subhito2_2_v4 / AuditoriaIA/](./02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v4/AuditoriaIA/)**: Dictámenes de auditoría externa de ChatGPT 6 Astra, GLM 5.3 y Claude Sonnet 5 sobre FreeRTOS, ADS1115, seguridad física y diseño factorial.
  - ⚡ **[scripts_compilacion_rapida/](./02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v3/scripts_compilacion_rapida/)**: Scripts `.bat` para compilar con PlatformIO y subir binarios al ESP32 en 3 segundos.
  - 🩺 **[Diagnostico_y_Resolucion_Problemas_Instrumentacion.md](./02_Hito2_Instrumentacion_Sensores/01_Guias_Montaje_y_Calibracion/Diagnostico_y_Resolucion_Problemas_Instrumentacion.md)**: **Documento para tesis**. Análisis matemático y físico de la saturación a 81 Hz (835.5 mL/min), solución al zumbido de 50 Hz/100 Hz, filtro pasabajos RC y resolución del cruce de canales.
  - 📈 **[Simulaciones_Filtro_RC/](./02_Hito2_Instrumentacion_Sensores/01_Guias_Montaje_y_Calibracion/Simulaciones_Filtro_RC/)**: Simulación en **LTspice** (`simulacion_filtro_caudalimetro.asc`), script de modelado en Python y curvas de atenuación de ruido.
  - 🖥️ **[Esquemas_Conexionado_HTML/](./02_Hito2_Instrumentacion_Sensores/01_Guias_Montaje_y_Calibracion/Esquemas_Conexionado_HTML/)**: Colección de planos interactivos SVG (borneras ZS-1057, protoboard, capacitor de desacoplo y nodo pull-up).
  - 📝 **[Bitacoras_Calibracion/](./02_Hito2_Instrumentacion_Sensores/01_Guias_Montaje_y_Calibracion/Bitacoras_Calibracion/)**: Registro cronológico de jornadas de Owen, ensayos en probeta y matriz de fallas resueltas.
  - 📁 **[Datos/](./02_Hito2_Instrumentacion_Sensores/Datos/)**: Planilla oficial de calibración de probeta a 50/72 RPM (`.xlsx`) y datasets de telemetría cruda en banco (`.csv`).
* **¿Para qué leerlo?**: Es la guía práctica para el trabajo diario en el laboratorio, cableado de la segunda bornera, calibración de instrumentos y análisis de fallas para la redacción de la tesis.

#### 🌪️ `03_Hito3_Reactor_Sedimentador_Agitador/`
* **Contenido**: Pretratamiento por coagulación-floculación (Jar Test), control del motor de agitación con driver L298N, cálculo del gradiente de velocidad ($G$), paleta normalizada y boya de nivel en acero inoxidable.
* **¿Para qué leerlo?**: Para preparar los ensayos de dosificación química y acondicionamiento del agua turbia antes de pasar a la membrana.

#### 🌐 `04_Hito4_Integracion_Automatizacion_IoT/`
* **Contenido**: Automatización centralizada en FreeRTOS, enclavamientos de seguridad por Presión Transmembrana ($\text{TMP} \le 0.50\text{ bar}$) y corte por marcha en seco.
* **¿Para qué leerlo?**: Para entender cómo el ESP32 coordina todos los subsistemas de forma autónoma sin intervención humana.

#### 🧪 `05_Hito5_Ensayos_Membrana_VidaUtil/`
* **Contenido**: Modelado fenomenológico del cartucho Fresenius FX100, Ley de Darcy modificada por temperatura (TCF), determinación de flujo crítico $J_c$, matriz factorial $3^2$ (11 ensayos con réplicas en punto central), mecanismos de ensuciamiento (*cake layer*, *pore blocking*) y planillas modelo para la tesis.
* **¿Para qué leerlo?**: Para redactar los capítulos de resultados experimentales, curvas de permeabilidad y conclusiones de la tesis de grado.

#### 📦 `Archivado/`
* **Contenido**: Códigos preliminares de prueba, librerías intermedias y documentación obsoleta.
* **¿Para qué sirve?**: Historial de respaldo; no debe utilizarse en las pruebas activas de laboratorio.

---

## 📌 6. Asignación Rápida de Pines del ESP32

| Pin ESP32 | Función Operativa | Tipo de Señal | Conexión Física de Destino |
| :---: | :--- | :--- | :--- |
| **GPIO 18** | Pulsos STEP Bomba (LEDC) | Salida Digital | Driver Leadshine DM860 (`PUL+` / Cátodo Común) |
| **GPIO 19** | Dirección de Giro (CW/CCW) | Salida Digital | Driver Leadshine DM860 (`DIR+` / Cátodo Común) |
| **GPIO 14** | Caudalímetro FEED (Entrada) | Interrupción IRAM | Placa 2 Front-End (Borne P14 filtrado con RC) |
| **GPIO 27** | Caudalímetro PERMEADO | Interrupción IRAM | Placa 2 Front-End (Borne P27 filtrado con RC) |
| **GPIO 4** | Sensor Temperatura DS18B20 | Bus 1-Wire Digital | Placa 2 Front-End (Borne P4 con Pull-Up $4.7\text{ k}\Omega$) |
| **GPIO 32** | Boya de Nivel Inox (Corte) | Entrada Digital Pull-Up | Placa 2 Front-End (Borne P32 con capacitor 100nF) |
| **GPIO 21** | I2C SDA (Datos) | Bus I2C a 400 kHz | Conversor ADC 16-Bit ADS1115 (Pin SDA) |
| **GPIO 22** | I2C SCL (Reloj) | Bus I2C a 400 kHz | Conversor ADC 16-Bit ADS1115 (Pin SCL) |
| **GPIO 16/17** | Sentido Agitador Jar Test | Salidas Digitales | Driver L298N (Pines IN1 e IN2) |
| **GPIO 5** | Velocidad Agitador (PWM) | Salida PWM 1 kHz | Driver L298N (Pin ENA) |

---

> 📖 **Nota Metodológica**: Cada hito posee su propio archivo `README.md` interno donde se profundiza en las ecuaciones físico-químicas, diagramas de flujo y rutinas de calibración específicas.

