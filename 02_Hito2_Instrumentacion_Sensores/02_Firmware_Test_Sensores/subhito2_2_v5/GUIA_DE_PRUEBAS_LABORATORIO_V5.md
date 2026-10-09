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
4. **Calibración Dinámica del Factor K en Caliente:** Actualizar el factor $K_{\text{perm}}$ directamente desde el SCADA o navegador (`/set_dev?kp=...&save=1`) guardándolo en la memoria Flash NVS del ESP32 sin necesidad de recompilar el código.
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
   * Si se accede desde una laptop, ingresar a: `http://192.168.4.1`.

---

### PASO 2: Certificación del Modo Seco (Cero Hidráulico)
1. Con los caudalímetros vacíos y sin agua en las líneas, abrir en el navegador:
   ```text
   http://192.168.4.1/cmd?act=MODO_SECO_ON
   ```
   *(El Monitor Serial imprimirá: `[AUDITORIA] Modo Seco ACTIVADO`).*
2. Desde la interfaz web del SCADA, encender la bomba y configurar los siguientes escalones:
   * **20 RPM** por 30 segundos.
   * **50 RPM** por 30 segundos.
   * **80 RPM** por 30 segundos.
3. **Observación en vivo:**
   * El indicador de Caudal de Permeado en el SCADA debe permanecer estrictamente en **0.0 mL/min**.
   * En el monitor serial, la telemetría periódica debe indicar `V:0` (pulsos válidos = 0).
4. Desactivar el modo seco antes de introducir fluidos:
   ```text
   http://192.168.4.1/cmd?act=MODO_SECO_OFF
   ```
   *(El firmware ejecutará `reiniciarEstimadorLocked()` e incrementará la época, impidiendo que cualquier flanco del ensayo en seco contamine la medición con agua).*

---

### PASO 3: Verificación Rápida de Alimentación (Control a 50 RPM)
1. Colocar el tubo de succión en el reservorio de agua desionizada/red y conectar la salida de recirculación a la probeta de $1000\text{ mL}$.
2. Encender la bomba a **50 RPM** cronometrando exactamente 60 segundos.
3. **Comprobación:**
   * El volumen recogido debe situarse en torno a $\approx 720\text{ mL}$ ($\pm 5\%$).
   * El display del SCADA debe indicar un caudal estable de $\approx 720\text{ mL/min}$.
   * El caudalímetro de permeado (si la válvula de ultrafiltración está cerrada) debe marcar **0.0 mL/min**.

---

### PASO 4: Caracterización Experimental del Permeado (Foco Principal)
1. Conectar el cartucho de membrana Fresenius FX100 en el lazo hidráulico.
2. Dirigir la línea de permeado a una **probeta fina graduada de 100 mL** (o un recipiente plástico sobre una balanza digital tarada a $0.0\text{ g}$).
3. Encender la bomba a un régimen moderado (**40 a 60 RPM**) y cerrar gradualmente la válvula de retentado para generar presión hidráulica tangencial y forzar el permeado.
4. Una vez establecido el goteo/flujo continuo de permeado, cronometrar un intervalo de ensayo (ej. 2 minutos o 120 segundos).
5. Registrar los valores:
   * **Volumen o Masa real de permeado ($V_{\text{real}}$ o $m_{\text{real}}$):** leído en la probeta o balanza ($1\text{ g} \approx 1\text{ mL}$).
   * **Caudal Real ($Q_{\text{perm,real}}$):**
     $$Q_{\text{perm,real}}\;[\text{mL/min}] = \frac{V_{\text{real}}\;[\text{mL}]}{t_{\text{ensayo}}\;[\text{min}]}$$
   * **Frecuencia del Sensor en Pantalla ($F_{\text{perm}}$):** leída en Hz desde el panel SCADA o Monitor Serial.

---

### PASO 5: Calibración del Factor K en Vivo (Sin Recompilar)
1. Calcular el factor de calibración experimental $K_{\text{perm}}$:
   $$K_{\text{perm}}\;[\text{Hz / (L/min)}] = \frac{F_{\text{perm}}\;[\text{Hz}] \times 1000}{Q_{\text{perm,real}}\;[\text{mL/min}]}$$
2. Actualizar el factor en el microcontrolador y guardarlo en la memoria Flash NVS ingresando en el navegador:
   ```text
   http://192.168.4.1/set_dev?kp=VALOR_CALCULADO&save=1
   ```
   *(Ejemplo: si $K$ calculado es 685.4: `http://192.168.4.1/set_dev?kp=685.4&save=1`).*
3. A partir de ese milisegundo, el SCADA calculará el caudal con la constante real y el valor quedará grabado permanentemente incluso si se reinicia la placa.

---

### PASO 6: Descarga y Análisis de Datos (Datalogger V5.2)

Al terminar la sesión de pruebas, descargar los dos registros para el informe de laboratorio y el anexo de tesis:

1. **Registro de Proceso (Formato Excel):**
   * URL: `http://192.168.4.1/export_csv`
   * Contenido: 16 columnas con tiempo transcurrido en segundos (iniciando en $t = 0\text{ s}$), consigna RPM, caudal de alimentación, caudal de permeado, volumen acumulado y banderas de estado.
2. **Registro Metrológico Científico (Formato Académico Completo):**
   * URL: `http://192.168.4.1/export_metrologia`
   * Contenido: 29 columnas detalladas con marcas de tiempo en microsegundos, conteo de flancos totales, pulsos descartados por filtro geométrico, períodos en microsegundos, frecuencia bruta, estabilidad de reloj, épocas de modo seco y banderas de validez de Darcy.

---

## 5. TRANSICIÓN AL SUBHITO 2.3 (INTEGRACIÓN DE MANÓMETROS EL LUNES)

* **Estado de la Presión Transmembrana (TMP) y Modelo de Darcy ($R_m$):**  
  En el Firmware V5.2, la bandera de validez de Darcy está establecida en `valido = false` y los campos de TMP y $R_m$ exportan `NAN` (o vacío en CSV).
* **Razón Metrológica:**  
  No se inventan presiones virtuales ni constantes de viscosidad teóricas. El cálculo riguroso de TMP:
  $$\text{TMP} = \frac{P_{\text{entrada}} + P_{\text{retentado}}}{2} - P_{\text{permeado}}$$
  se activará formalmente el lunes cuando se conecten los transmisores de presión físicos a las entradas analógicas del ESP32, completando el Subhito 2.3 sin ninguna suposición ciega en la tesis.
