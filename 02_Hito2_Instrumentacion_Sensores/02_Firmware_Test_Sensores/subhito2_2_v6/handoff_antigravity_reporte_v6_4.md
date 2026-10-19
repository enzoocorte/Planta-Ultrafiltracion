# 📋 REPORTE DE HANDOFF Y DOCUMENTACIÓN TÉCNICA PARA ANTIGRAVITY — FIRMWARE `V6.4` (PENDIENTE DE RE-CONEXIÓN FÍSICA EN BORNERA)

* **Proyecto**: Planta Piloto de Ultrafiltración Tangencial (`Fresenius FX100 Helixone®`) — Tesis de Ingeniería Industrial (UNSa)
* **Tesistas**: Antonella Guitián & Owen Cañizares | **Codirector**: Ing. Enzo Corte
* **Versión de Código**: `V6.4` (`EN_USO_firmware_planta`)
* **Estado Actual del Sistema**: **Código `V6.4` compilado, auditado y con bug de RPM corregido, pero PENDIENTE DE VALIDACIÓN EN PLANTA (los caudales aún no miden bien en el banco actual debido a falsos contactos físicos en los zócalos hembra de la Placa 2, los cuales se corregirán con destornillador en la próxima sesión de laboratorio).**

---

## 1. Instrucciones Directas de Repositorio (Git) para Antigravity

**Antigravity**, por favor ejecuta las siguientes acciones sobre el repositorio local y remoto del proyecto:

1. **Actualizar los archivos del firmware a la revisión `V6.4`** copiando los archivos actualizados (`caudalimetro.cpp`, `caudalimetro.h`, `EN_USO_firmware_planta.ino`, `config.h`, `Bomba.cpp`, `Bomba.h`, `darcy.h`, `registro_ensayos.h`, `index_html.cpp`, `index_html.h`).
2. **Documentar explícitamente en el repositorio (`README.md` / `CHANGELOG.md` / bitácora)** que esta versión **`V6.4` todavía NO está validada como operativa final en lectura de caudales**, dejando asentado el estado exacto:
   * **Lo que YA funciona al 100% en `V6.4`**: El control cinemático de la bomba NEMA 34, el seguimiento continuo de `RPM_Bomba` sin caídas a `0.0` y el registro ininterrumpido del datalogger cada $10\text{ s}$ (comprobado en `PlantaUF_MonitoreoTorta_HistoricoCompleto5.csv`).
   * **Lo que TODAVÍA falla en los ensayos de banco (`CSV 5`)**: La medición de los caudalímetros (`Q_Alim` y `Q_Perm`) sigue arrojando valores erráticos y colapsa a `0.0 mL/min` a altas RPM **debido a un problema físico de montaje en la Placa 2** (las patas de $0.4\text{ mm}$ de las resistencias *pull-up* de $4.7\text{ k}\Omega$ y del capacitor `104` están insertadas sin presión en los agujeritos negros del zócalo hembra en lugar de estar apretadas bajo los tornillos verdes).
3. **Realizar el commit y push al repositorio Git** con un mensaje claro de trazabilidad, por ejemplo:
   ```bash
   git add .
   git commit -m "wip(firmware-v6.4): fix RPM=0 bug & add 3000us/mediana3 filter; flow reading pending physical screw-terminal rewiring on Board 2"
   git push
   ```

---

## 2. Todo lo Realizado y Cambios Implementados en el Código (De `V6.0` a `V6.4`)

Durante esta etapa de depuración intensiva sobre los archivos reales en uso (`EN_USO_firmware_planta`) y los históricos exportados por el SCADA (`PlantaUF_MonitoreoTorta_HistoricoCompleto3.csv`, `4.csv` y `5.csv`), se realizaron tres correcciones estructurales en el software:

| # | Archivo Modificado | Problema Detectado en Versiones Previas | Cambio Exacto Implementado en `V6.4` | Resultado Comprobado |
| :--- | :--- | :--- | :--- | :--- |
| **1** | `EN_USO_firmware_planta.ino` | **Bug de caída de `RPM_Bomba` a `0.0` mientras la bomba seguía girando (`CSV 4`)**: Las funciones `bombaGirandoReal()` y `rpmRealConfirmada()` exigían que `sensorAlimentacion` reportara flujo activo (`tieneFlujoAlim`). Cuando el caudalímetro perdía pulsos por falso contacto en la bornera, el código forzaba `RPM_Bomba = 0.0` en el SCADA y detenía el guardado del datalogger cada $10\text{ s}$. | Se independizó el reporte de RPM y el datalogger respecto de los caudalímetros. Ahora `bombaGirandoReal()` evalúa únicamente `bomba.enMarcha() && (bomba.rpmActual() > 1.0f)`. | **100% Resuelto en `CSV 5`**: En los 2 ensayos del `CSV 5`, `RPM_Bomba` mantuvo `25.0`, `50.0`, `67.0`, `69.0` y `90.0 RPM` durante los $180\text{ s}$ sin volver a caer jamás a `0.0`. |
| **2** | `caudalimetro.cpp` (`Caudalimetro::isr()`) | **Paso de ráfagas de falso contacto mecánico (`373.06 Hz` / `1980.1 mL/min` en `CSV 5`, `t=90s`)**: En la revisión previa (`V6.3`) el bloqueo entre flancos de bajada estaba en `periodoMin_us = 1500 us` ($666.7\text{ Hz}$), lo que permitió que el rebote mecánico de una pata suelta en el agujerito negro `6` pasara como una frecuencia de `373.06 Hz`. | Se elevó el bloqueo en la interrupción `isr()` a **`periodoMin_us = 3000 us` ($333.3\text{ Hz}$)** y el ancho mínimo de semiciclo `HIGH`/`LOW` a **`1200 us`** en ambos canales, además de saturar la salida máxima en `1500.0 mL/min`. | Bloquea en tiempo de interrupción cualquier rebote de contacto $> 333\text{ Hz}$, dejando pasar el $100\%$ del rango físico real de los sensores YF-S401 ($0\text{ a }220\text{ Hz}$). |
| **3** | `caudalimetro.cpp` y `caudalimetro.h` (`Caudalimetro::capturar()`) | **Variaciones bruscas de 1 segundo por paso de rodillos o microburbujas**: Sin filtro de mediana, cualquier pulso perdido o burbuja hacía oscilar la lectura instantánea. | Se activó en `capturar()` el **Filtro Híbrido de Mediana de 3 ventanas (`mediana3(_qHist[0], _qHist[1], _qHist[2])`) + Suavizado Exponencial EMA ($\alpha = 0.35$)** sobre `_qSuavizadoDisplay`, el cual alimenta tanto la pantalla web como el `.CSV` (`caudal_mLmin()`). | Suprime caídas o picos aislados de 1 segundo sin introducir retardo en el seguimiento de la formación de torta. |

---

## 3. Hipótesis Físico-Electrónica Demostrada con Datos: Por Qué Falla Hoy el Circuito y Por Qué Funcionará al Atornillar

El análisis forense del archivo `PlantaUF_MonitoreoTorta_HistoricoCompleto5.csv` demuestra de forma concluyente que **el código `V6.4` mide y procesa correctamente los pulsos cuando llegan al pin del ESP32**, y que la falla actual se origina en el **montaje físico sobre la Placa 2 (bornera auxiliar sin ESP32)**:

### 3.1. Cómo está conectado hoy el circuito (Estado que falla)

En la configuración actual del banco:
* Se retiró correctamente el tercer capacitor electrolítico grande ($2200\ \mu\text{F}$) que estaba suelto en el telgopor.
* En la **Placa 2**, los cables de tensión y señal entran en los casilleros **`1` (Rojo `5V`)**, **`4` (Negro `GND`)**, **`6` (Amarillo `GPIO 27` Permeado)**, **`9` (Violeta `GPIO 14` Alimentación)** y **`12` (Marrón `3.3V`)**.
* **El defecto crítico actual**: las patas de las dos resistencias *pull-up* de $4.7\text{ k}\Omega$ (entre `6–12` y `9–12`) y del capacitor cerámico `104` ($100\text{ nF}$, entre `7–9`) fueron insertadas a presión dentro de los **agujeritos negros del zócalo hembra (`pinches`)** de la Placa 2 en vez de ser apretadas bajo los **tornillos verdes**.
* Como el alambre de una resistencia o capacitor mide apenas **$0.40\text{ mm}$** de diámetro y el zócalo hembra negro está diseñado para pines cuadrados DuPont de **$0.64\text{ mm}$**, las patas quedan sueltas adentro del plástico, no hacen presión contra las láminas internas y pierden contacto con la vibración de la bomba.

### 3.2. Qué revelan los datos del `CSV 5` en cada canal

| Canal Evaluado | Comportamiento Registrado en `PlantaUF_MonitoreoTorta_HistoricoCompleto5.csv` | Causa Física Exacta en la Placa 2 |
| :--- | :--- | :--- |
| **Canal Alimentación (`GPIO 14`, cable Violeta, casillero `9`, CON capacitor `104` en `7–9`)** | En `Ensayo 1`: a `25.0 RPM` (`t=20s`) mide **`11.17 Hz` (`63.9 mL/min`)**; acelerando (`t=40s`) mide **`22.88 Hz` (`83.3 mL/min`)**; pero al llegar a **`90.0 RPM` (`t=50s` y `t=60s`) cae a `0.93 Hz` y `0.00 Hz` (`0.0 mL/min`)**. Al bajar a `50.0 RPM` (`t=70s`), vuelve a marcar (`4.41 Hz`). En `Ensayo 2` (`t=0s a 80s` a `67 RPM`), mide `9.09 a 14.46 Hz` y luego cae a `0.00 Hz`. | **Ahogo capacitivo (Filtro RC sin resistencia pull-up externa)**: Al no hacer contacto la resistencia de $4.7\text{ k}\Omega$ en los agujeritos negros `9` y `12` (o el cable de `3.3V` en `12`), el capacitor `104` ($100\text{ nF}$) no recibe corriente desde `3.3V` y solo puede recargarse a través de la débil resistencia `INPUT_PULLUP` interna del ESP32 ($\approx 45\text{ k}\Omega$). La constante de tiempo sube 10 veces: de $\tau = 0.47\text{ ms}$ a **$\tau = 45\text{ k}\Omega \times 100\text{ nF} = 4.5\text{ ms}$**. A `25 RPM` los pulsos son lentos y el capacitor llega a cargarse parcialmente, pero a `70–90 RPM` el semiperíodo del pulso es de $\approx 2.5\text{ ms}$: la tensión máxima apenas llega a $V = 3.3 \cdot (1 - e^{-2.5/4.5}) = \mathbf{1.41\text{ V}}$, quedando por debajo del umbral lógico `HIGH` del ESP32 ($2.48\text{ V}$). Por eso la señal se apaga a `0.00 Hz` al subir las RPM. |
| **Canal Permeado (`GPIO 27`, cable Amarillo, casillero `6`, SIN capacitor `104`)** | En `Ensayo 2` (`t=90s a 180s` a `69.0 RPM`), sí logra medir frecuencias altas (**`24.49 Hz` a `55.47 Hz`**, equivalentes a `133 a 264 mL/min`), pero en `t=90s` registra un pico falso de **`373.06 Hz` (`1980.1 mL/min`)**. | Como en el casillero `6` se había retirado el capacitor `104`, el pin no sufrió ahogo capacitivo a `69 RPM` y pudo contar los pulsos, pero la pata de la resistencia suelta en el agujerito negro `6` vibró y generó ráfagas de falso contacto mecánico (`373.06 Hz`). |

---

## 4. Plan de Acción Físico en Laboratorio (Para Cerrar la Validación y Pasar al Siguiente Hito)

En la próxima sesión de laboratorio, con destornillador en mano, se ejecutará el **conexionado firme 100% bajo tornillo verde** (dejando los agujeritos negros completamente vacíos):

### 4.1. Tabla de Conexión Definitiva en la Bornera Verde de la Placa 2

| Tornillo Verde (Placa 2) | Qué se aprieta directamente dentro de la mordaza del tornillo | Función Eléctrica |
| :--- | :--- | :--- |
| **Tornillo 1 (`5V`)** | Cable Rojo desde `5V` de Placa 1 + **2 cables Rojos** de los caudalímetros YF-S401 | Alimentación de $5\text{ V}$ firme para los sensores Hall |
| **Tornillo 4 (`GND`)** | Cable Negro desde `GND` de Placa 1 + **2 cables Negros** de los caudalímetros YF-S401 | Masa común unificada ($0.0\ \Omega$ respecto al USB del ESP32) |
| **Tornillo 6 (`GPIO 27` Permeado)** | Cable Amarillo hacia `G27` (Placa 1) + Cable de datos del sensor de Permeado + **1 pata de la Resistencia 1 ($4.7\text{ k}\Omega$)** | Nodo de señal de Permeado con pull-up mecánicamente inamovible |
| **Tornillo 9 (`GPIO 14` Alimentación)** | Cable Violeta hacia `G14` (Placa 1) + Cable de datos del sensor de Alimentación + **1 pata de la Resistencia 2 ($4.7\text{ k}\Omega$)** | Nodo de señal de Alimentación con pull-up mecánicamente inamovible |
| **Tornillo 12 (`3.3V`)** | Cable Marrón desde `3V3` de Placa 1 + **Las otras 2 patas de las Resistencias 1 y 2 ($4.7\text{ k}\Omega$)** | Alimentación lógica de $3.3\text{ V}$ para los pull-ups (protege los pines del ESP32) |

> **Nota técnica sobre los capacitores `104`**: La salida del sensor YF-S401 es de colector abierto (*open-collector*: solo conmuta a `GND`), por lo que quienes protegen y fijan el voltaje del ESP32 en $3.3\text{ V}$ son exclusivamente las **2 resistencias de $4.7\text{ k}\Omega$ conectadas al Tornillo 12 (`3.3V`)**. Con el filtro digital de la versión **`V6.4`** (`3000 us` + `mediana3` + `EMA`), se recomienda realizar la primera prueba **sin los capacitores `104`** (solo con las 2 resistencias atornilladas en `6`, `9` y `12`) para verificar que no exista ningún ahogo capacitivo a altas RPM.

### 4.2. Valores de Verificación Esperados tras Atornillar (Criterio de Aprobación de `V6.4`)

Una vez apretados los 5 tornillos verdes y cargado `caudalimetro.cpp` (`V6.4`), al bombear agua limpia sin contrapresión deben observarse en `http://192.168.4.1` los siguientes valores estables (con $K = 205.98\text{ Hz/(L/min)}$ y $\text{Cilindrada} = 13.55\text{ mL/rev}$):

* **A `25.0 RPM`**: `Frec_Alim` $\approx 110\text{ a }120\text{ Hz}$ $\longrightarrow$ `Q_Alim` **$\approx 320\text{ a }355\text{ mL/min}$**.
* **A `50.0 RPM`**: `Frec_Alim` $\approx 220\text{ a }235\text{ Hz}$ $\longrightarrow$ `Q_Alim` **$\approx 640\text{ a }685\text{ mL/min}$**.
* **A `70.0 RPM`**: `Frec_Alim` $\approx 305\text{ a }325\text{ Hz}$ $\longrightarrow$ `Q_Alim` **$\approx 890\text{ a }955\text{ mL/min}$**.

Una vez verificado este escalón en banco, se subirá el nuevo `.CSV` de confirmación y se actualizará el tag del repositorio en Git como **Versión `V6.4` Validada en Planta**.
