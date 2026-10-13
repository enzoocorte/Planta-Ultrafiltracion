# GUÍA DE ENSAYOS EXPERIMENTALES EN BANCO — SUBHITO 2.2 (FIRMWARE V5.2 DEFINITIVO)

**Planta Piloto de Ultrafiltración Tangencial — Membrana Fresenius FX100 (Helixone®)**  
* **Carrera:** Ingeniería Industrial — Facultad de Ingeniería, Universidad Nacional de Salta (UNSa 2026)  
* **Tesistas:** Antonella Guitián & Owen Cañizares  
* **Codirección:** Ing. Enzo Corte  
* **Ubicación:** Laboratorio de Operaciones Unitarias / Domótica — UNSa, Salta  
* **Versión de Firmware:** `V5.2 Definitivo` (Núcleo Metrológico Certificado & Erradicación de Caudal Fantasma)  
* **Hardware Congelado:** Filtro RC Pasivo ($R = 4.7\text{ k}\Omega$, $C = 200\text{ nF}$) + Algoritmo Geométrico por Software  

---

## 1. OBJETIVO DEL PROTOCOLO DE ENSAYO

1. **Certificación del Cero Hidráulico y Anti-Ruido:** Demostrar formalmente que con el motor paso a paso NEMA 34 girando a altas revoluciones sin líquido en la línea de permeado, la medición de caudal marca estrictamente **0.0 mL/min**, erradicando el caudal fantasma histórico de $\approx 145\text{ mL/min}$.
2. **Aprovechamiento de la Curva de Alimentación Validada:** No perder tiempo repitiendo el aforo completo de alimentación ($20\text{ a }90\text{ RPM}$ ya caracterizados con probeta de $1000\text{ mL}$ y $R^2 = 0.985$). Realizar únicamente un control rápido a **50 RPM** para verificar estabilidad.
3. **Caracterización Experimental del Caudal de Permeado:** Medir con precisión volumétrica o gravimétrica el bajo flujo de permeado ($10\text{ a }100\text{ mL/min}$) mediante probeta fina de $100\text{ mL}$ o balanza digital tarada a $0.0\text{ g}$.
4. **Calibración Dinámica del Factor K en Vivo:** Ajustar el factor $K_{\text{perm}}$ directamente desde los controles táctiles `[ − ]` y `[ + ]` del SCADA o la consola `/set_dev`, guardándolo en la memoria Flash NVS del microcontrolador sin recompilar.
5. **Validación del Registro Datalogger (Doble Exportación):** Constatar que los ensayos queden perfectamente registrados desde $t = 0\text{ s}$ tanto en formato de proceso estándar (`/export_csv`) como en el registro metrológico completo de 29 columnas (`/export_metrologia`).

---

## 2. DATOS HISTÓRICOS CONSOLIDADOS (ALIMENTACIÓN — ENSAYOS 2 Y 3)

Estos datos volumétricos ya fueron medidos y certificados con probeta de $1000\text{ mL}$ por Antonella, Owen y Enzo. **Forman parte del cuerpo de la tesis y sirven como línea de base de calibración:**

| RPM Consigna | Tiempo Ensayo (min) | Volumen en Probeta (mL) | Caudal Real $Q$ (mL/min) | Cilindrada Real (mL/rev) | Frecuencia SCADA (Hz) |
| :---: | :---: | :---: | :---: | :---: | :---: |
| **20** | 1.0 | 280 | **280** | 14.00 | 60 |
| **30** | 1.0 | 430 | **430** | 14.33 | 100 |
| **40** | 1.0 | 597 | **597** | 14.93 | 130 |
| **50** | 1.0 | 720 | **720** | 14.40 | 150 |
| **60** | 1.0 | 820 | **820** | 13.67 | 160 |
| **70** | 1.0 | 915 | **915** | 13.07 | 180 |
| **80** | 0.5 | 480 | **960** | 12.00 | 190 |
| **90** | 0.5 | 540 | **1080** | 12.00 | 200 |

* **Cilindrada media experimental:** $13.55\text{ mL/rev}$ (configurada en $13.60\text{ mL/rev}$ nominal en `config.h`).
* **Factor $K_{\text{alim}}$ experimental por regresión:** $172.72\text{ Hz / (L/min)}$ ($R^2 = 0.985$).

---

## 3. MATRIZ DE DIAGNÓSTICO FÍSICO DE 4 CONDICIONES (DISCRIMINACIÓN DE RUIDO)

Para garantizar rigor académico y descartar cualquier fuente de acoplamiento electromagnético (EMI) de la etapa de potencia (48V) sobre el sensor Hall de permeado, se ejecuta la siguiente matriz antes de iniciar el bombeo con fluidos:

| N° | Condición Física del Sistema | Estado Motor / Driver DM860 | Línea de Permeado | Criterio de Aceptación (Monitor Serial / SCADA) | Diagnóstico Académico |
| :---: | :--- | :--- | :---: | :--- | :--- |
| **1** | **Reposo Absoluto** | Driver 48V APAGADO. Motor apagado. | Seca / Vacía | $Q_{\text{perm}} = 0.0\text{ mL/min}$<br>`V:0, R:0, F:0` | Confirma nivel lógico base sin ruido de fondo ambiente. |
| **2** | **Driver Energizado en Reposo** | Driver 48V ENCENDIDO.<br>Torque de retención (sin pulsos STEP). | Seca / Vacía | $Q_{\text{perm}} = 0.0\text{ mL/min}$<br>`V:0, R:0, F:0` | Descarta inducción parásita por ripple de la fuente conmutada o chopper del driver. |
| **3** | **Giro de Motor en Seco** (Barrido 20, 50 y 80 RPM) | Driver 48V ENCENDIDO.<br>Motor girando a régimen. | Seca / Vacía | $Q_{\text{perm}} = 0.0\text{ mL/min}$<br>`V:0`, `val_perm = 0`<br>`ruido_seco = false` | **Certificación clave de V5.2:** Descarta pulsos por tren de conmutación STEP/DIR. Si hay glitches, el firmware los aísla sin registrar flujo. |
| **4** | **Pull-up Pasivo de Control** (Prueba facultativa) | Desconectar cable de señal del sensor en bornera (pull-up $4.7\text{ k}\Omega$ activo). | N/A | Cero pulsos absolutos a 90 RPM. | Demuestra que el cableado apantallado no actúa como antena receptora. |

---

## 4. PROCEDIMIENTO OPERATIVO PASO A PASO EN EL LABORATORIO

### PASO 1: Puesta en Marcha y Portal Cautivo Automático
1. Conectar el ESP32 a la PC por cable USB.
2. Abrir el Monitor Serial de Arduino IDE a **115200 baudios**.
3. Encender la fuente de 48V del motor.
4. Conectar el celular o la tablet al Wi-Fi de la planta:
   * **SSID:** `Bomba_Peristaltica_UF`
   * **Contraseña:** `plantapiloto2`
5. **Verificación de Red:**
   * El celular abrirá **automáticamente** el SCADA web en pantalla completa gracias al servidor DNS en puerto 53 y redirección HTTP 302.
   * Si se accede desde una laptop o navegador manual, ingresar a: `http://192.168.4.1` (o `http://bomba.local`).

---

### PASO 2: Certificación del Modo Seco (Cero Hidráulico con Botón SCADA)
1. Con los caudalímetros vacíos y sin agua en las líneas:
   * En la **Card 2 (Caudalímetros)** del SCADA, presionar directamente el botón **`🧪 Modo Seco`**.
   * El botón cambiará inmediatamente a color ámbar: **`⚠ Seco: ON`**, y aparecerá una notificación toast: *"Modo Seco ACTIVADO"*.
   *(Alternativa por URL directa si no se usa la UI: `http://192.168.4.1/cmd?act=MODO_SECO_ON`).*
2. Desde los controles de la bomba en el SCADA, encender el motor y recorrer los escalones:
   * **20 RPM** por 30 segundos.
   * **50 RPM** por 30 segundos.
   * **80 RPM** por 30 segundos.
3. **Observación en vivo:**
   * El indicador de Caudal de Permeado en el SCADA debe permanecer estrictamente en **0.0 mL/min**.
   * En el monitor serial, la telemetría periódica debe indicar `V:0` (pulsos válidos = 0).
4. Al finalizar la prueba en seco y antes de ingresar agua, volver a presionar el botón **`⚠ Seco: ON`** para restaurar el modo normal.
   *(El firmware ejecuta `reiniciarEstimadorLocked()` e incrementa la época, impidiendo que cualquier flanco del ensayo en seco contamine las lecturas con agua).*

---

### PASO 3: Verificación Rápida de Alimentación (Control a 50 RPM)
1. Colocar el tubo de succión en el reservorio de agua y conectar la salida a la probeta de $1000\text{ mL}$.
2. Encender la bomba a **50 RPM** cronometrando exactamente 60 segundos.
3. **Comprobación:**
   * El volumen recogido debe situarse en torno a $\approx 720\text{ mL}$ ($\pm 5\%$).
   * El display del SCADA debe indicar un caudal estable de $\approx 720\text{ mL/min}$.
   * El caudalímetro de permeado (con la válvula cerrada) debe marcar estrictamente **0.0 mL/min**.

---

### PASO 4: Caracterización Experimental del Permeado (Foco Principal)
1. Conectar el cartucho de membrana Fresenius FX100 en el lazo hidráulico.
2. Dirigir la línea de permeado a una **probeta fina graduada de 100 mL** (o un vaso sobre balanza digital tarada a $0.0\text{ g}$).
3. Encender la bomba a régimen moderado (**40 a 60 RPM**) y cerrar gradualmente la válvula de retentado para generar presión hidráulica tangencial y forzar el permeado.
4. Una vez establecido el flujo continuo, cronometrar un intervalo de ensayo (ej. 2 minutos o 120 segundos).
5. Registrar los valores:
   * **Volumen o Masa real de permeado ($V_{\text{real}}$ o $m_{\text{real}}$):** leído en la probeta o balanza ($1\text{ g} \approx 1\text{ mL}$).
   * **Caudal Real ($Q_{\text{perm,real}}$):**
     $$Q_{\text{perm,real}}\;[\text{mL/min}] = \frac{V_{\text{real}}\;[\text{mL}]}{t_{\text{ensayo}}\;[\text{min}]}$$
   * **Frecuencia del Sensor en Pantalla ($F_{\text{perm}}$):** leída en Hz desde el panel SCADA.

---

### PASO 5: Calibración del Factor K en Vivo (Excel, SCADA y Botones `+` / `−`)

#### A. ¿Cómo y dónde se calcula el Factor K?
Existen 3 alternativas directas según la preferencia de trabajo en el laboratorio:

1. **Opción 1: En las Planillas Excel del Proyecto (Recomendado para la Tesis)**  
   En las planillas [`PLANILLA_CALIBRACION_ENSAYOS_2_Y_3.xlsx`](file:///c:/Users/enzoo/OneDrive/Documentos/ENZO/Domotica/SistemaUF/02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v5/EN_USO_firmware_planta/PLANILLA_CALIBRACION_ENSAYOS_2_Y_3.xlsx) o [`CALIBRACION_CAUDALIMETROS_PROBETA_50RPM_72RPM.xlsx`](file:///c:/Users/enzoo/OneDrive/Documentos/ENZO/Domotica/SistemaUF/02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v5/EN_USO_firmware_planta/CALIBRACION_CAUDALIMETROS_PROBETA_50RPM_72RPM.xlsx) ya diseñadas, ingresan el volumen en probeta ($V_{\text{real}}$) y el tiempo ($t$). La planilla calcula automáticamente el caudal y el factor $K$:
   $$K_{\text{perm}}\;[\text{Hz / (L/min)}] = \frac{F_{\text{perm}}\;[\text{Hz}] \times 1000}{Q_{\text{perm,real}}\;[\text{mL/min}]}$$

2. **Opción 2: Automático en el SCADA (Modo Dev — Sección B)**  
   En la interfaz web, abren **`🛠️ Modo Dev`** $\rightarrow$ van a **"🎯 CALIBRAR POR RPM & CAUDAL (PROBETA)"**:
   * Ingresan las RPM del ensayo (ej. 50).
   * Ingresan el caudal de permeado medido en la probeta en mL/min (ej. 45).
   * Tocan **`🚀 Calcular y Guardar Calibración`**: el microcontrolador calcula la cilindrada y los factores $K$ y los almacena directamente.

3. **Opción 3: Ajuste Fino en Vivo con Botones `[ − ]` y `[ + ]` (Novedad V5.2)**  
   En el SCADA $\rightarrow$ **`🛠️ Modo Dev`** $\rightarrow$ **Sección C (Edición Directa de Parámetros)**:
   * Cada factor ($K_{\text{alim}}$ y $K_{\text{perm}}$) dispone de botones táctiles **`[ − ]`** y **`[ + ]`**.
   * **Criterio Físico:** Como ambos caudalímetros son exactamente el mismo modelo (**YF-S401**), ambos factores $K$ deben ser del mismo orden de magnitud ($K_{\text{alim}} \approx 196.50$, $K_{\text{perm}} \approx 180 - 196$). Se descarta el valor histórico de $687.33$, que era un remanente provisional de ensayos previos con ruido.
   * Cada pulsación ajusta en $\pm 1.0\text{ Hz/(L/min)}$ e impacta **inmediatamente en la RAM del ESP32 en caliente**.
   * Permite que el operador en el celular presione `+` o `−` mientras mira la probeta hasta que el display digital marque exactamente los mL/min que caen en la probeta.
   * Una vez calibrado a gusto, tocan **`💾 Guardar en Flash ESP32`** para que el valor quede grabado en memoria NVS de por vida.

> **💡 ¿HACE FALTA LLENAR PLANILLAS EXCEL DURANTE EL ENSAYO?**  
> **NO.** Durante las pruebas en el laboratorio **no abran ni llenen planillas a mano**.  
> 1. El ESP32 tiene un datalogger interno que registra automáticamente todo segundo a segundo en RAM.  
> 2. Durante la prueba física, solo anoten en un cuaderno/celular dos datos por corrida: **los mL medidos en la probeta** y la **frecuencia en Hz** que mostraba la pantalla.  
> 3. Al terminar la sesión, descargan el archivo con un solo click desde el botón verde del SCADA (`📥 Descargar Ensayo en Excel`), y luego en la computadora abren el Excel para generar los gráficos y anexos de la tesis con tranquilidad.

---

### PASO 6: Descarga de Datos desde los Botones del SCADA

En la **Card 4 ("📊 HISTORIAL DE ENSAYOS & EXCEL")** de la pantalla principal del SCADA se encuentran disponibles los dos botones de descarga directa:

1. **Botón Verde: `📥 Descargar Ensayo Seleccionado en Excel (.CSV - 16 cols)`**
   * Formato industrial simplificado para abrir en Excel o Google Sheets y graficar:
   * **Columnas (16):** `Ensayo_ID`, `Tiempo_s`, `Tiempo_MinSec`, `RPM_Bomba`, `Estable_1_0`, `Q_Bomba_Teorico_mLmin`, `Frec_Alimentacion_Hz`, `Q_Alimentacion_mLmin`, `Vol_Alimentacion_L`, `Frec_PERMEADO_Hz`, `Q_PERMEADO_mLmin`, `Vol_PERMEADO_L`, `Q_Retentado_mLmin`, `Recuperacion_Y_Pct`, `Desviacion_Bomba_Alim_Pct`, `J_LMH`.
   * *(También cada corrida en la tabla al pie tiene su propio botón verde `📥 CSV` individual).*

2. **Botón Azul/Índigo: `🔬 Descargar Registro Metrológico Completo (29 cols)`**
   * Descarga la telemetría académica de bajo nivel para la defensa formal de la tesis.

---

### ¿POR QUÉ 29 COLUMNAS EN EL REGISTRO METROLÓGICO? (DEFENSA DE TESIS)

Un archivo CSV estándar de proceso solo informa *"Caudal Permeado = 30.5 mL/min"*. Cuando el tribunal examinador o un auditor consulte:  
> *«¿Cómo demuestran que a 80 RPM ese valor no fue ruido inducido por la etapa de potencia de 48V o un remanente del filtro digital?»*

La respuesta científica está en el registro de **29 columnas**, que audita cada pulso y microsegundo de hardware de los caudalímetros:

| Bloque | Columnas | Variables Incluidas | Justificación Metrológica para la Tesis |
| :--- | :---: | :--- | :--- |
| **I. Sesión y Bomba** | **5** | `sesion`, `tipo`, `estado`, `t_reg_us`, `rpm` | Marca de tiempo absoluta en microsegundos de hardware de 64 bits (`esp_timer_get_time()`), consigna y tipo de evento (arranque, periódico cada 10s, fin). |
| **II. Metrología Alimentación** | **12** | `tA_us`, `epocaA`, `calidadA`, `kA`, `flancosA`, `aceptadosA`, `nA`, `rechazosA`, `edadA_us`, `fA_Hz`, `qA_mLmin`, `vA_L` | **Trazabilidad total:**<br>• `flancosA`: Conteo bruto de interrupciones ISR.<br>• `aceptadosA`: Pulsos que superaron el umbral RC de 200 nF.<br>• `rechazosA`: Pulsos descartados por filtro geométrico anti-glitch.<br>• `epocaA`: Garantiza que cambios de modo seco no contaminen la muestra.<br>• `edadA_us`: Tiempo transcurrido desde el último pulso físico (certifica si hay flujo activo). |
| **III. Metrología Permeado** | **12** | `tP_us`, `epocaP`, `calidadP`, `kP`, `flancosP`, `aceptadosP`, `nP`, `rechazosP`, `edadP_us`, `fP_Hz`, `qP_mLmin`, `vP_L` | Las mismas 12 variables aplicadas al sensor YF-S401 de permeado. Demuestra que si el caudal marca 0, es porque no hay pulsos válidos y no por una retención ciega de la variable. |

**Total exacto:** $5 + 12 + 12 = \mathbf{29\text{ columnas}}$. Aporta respaldo inobjetable de integridad metrológica.

---

## 5. TRANSICIÓN AL SUBHITO 2.3 (INTEGRACIÓN DE MANÓMETROS EL LUNES)

* **Estado de la Presión Transmembrana (TMP) y Modelo de Darcy ($R_m$):**  
  En el Firmware V5.2, la bandera de validez de Darcy está establecida en `valido = false` y los campos de TMP y $R_m$ exportan `NAN` (o vacío en CSV).
* **Razón Metrológica:**  
  No se inventan presiones virtuales ni constantes de viscosidad teóricas. El cálculo riguroso de TMP:
  $$\text{TMP} = \frac{P_{\text{entrada}} + P_{\text{retentado}}}{2} - P_{\text{permeado}}$$
  se activará formalmente el lunes cuando se conecten los transmisores de presión físicos a las entradas analógicas del ESP32, completando el Subhito 2.3 sin ninguna suposición ciega en la tesis.
