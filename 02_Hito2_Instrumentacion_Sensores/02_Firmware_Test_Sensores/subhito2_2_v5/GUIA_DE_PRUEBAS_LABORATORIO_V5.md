# GUÍA DE ENSAYOS EXPERIMENTALES EN BANCO — SUBHITO 2.2 (FIRMWARE V5.1)

**Planta Piloto de Ultrafiltración Tangencial — Membrana Fresenius FX100 (Helixone®)**  
**Tesistas:** Antonella Guitián & Owen Cañizares  
**Codirección:** Ing. Enzo Corte  
**Ubicación:** Laboratorio de Operaciones Unitarias / Domótica — UNSa, Salta  

---

## 1. OBJETIVO DEL PROTOCOLO
1. Demostrar empíricamente que el firmware V5.1 erradica el 100% del ruido electromagnético (EMI) generado por el motor paso a paso NEMA 34 sobre los caudalímetros YF-S401 con el circuito de hardware congelado ($R = 4.7\text{ k}\Omega, C = 200\text{ nF}$).
2. Obtener la curva real de calibración del Caudalímetro de Alimentación ($K_{\text{alim}}$) y la cilindrada de la bomba ($\text{mL/rev}$) mediante aforo volumétrico con probeta graduada (20 a 90 RPM).
3. Caracterizar el comportamiento a bajo flujo del Caudalímetro de Permeado ($10\text{ a }100\text{ mL/min}$) mediante medición gravimétrica (balanza) o probeta fina de 100 mL.
4. Validar el funcionamiento del nuevo Datalogger de 10 segundos y la descarga automática del archivo CSV para el Capítulo 4 de la tesis.

---

## 2. MATERIALES E INSTRUMENTOS NECESARIOS
* ESP32 DOIT DevKit V1 conectado al tablero de control de la planta.
* Cable USB para programación y monitor serial (115200 baudios).
* Computadora portátil o teléfono celular con Wi-Fi.
* Probeta graduada de vidrio o plástico de $1000\text{ mL}$ (para alimentación).
* Probeta fina de $100\text{ mL}$ o balanza digital de precisión ($0.1\text{ g}$) con vaso de precipitados (para permeado).
* Cronómetro digital.
* Agua limpia (potable o desmineralizada) en el tanque de alimentación ($5\text{ a }10\text{ L}$).

---

## 3. PASO A PASO DEL ENSAYO EN EL LABORATORIO

### ETAPA 1: Puesta en Marcha y Verificación de Red Wi-Fi
1. Conectar el ESP32 a la PC mediante USB y abrir el **Monitor Serial** en Arduino IDE a **115200 baudios**.
2. Conectar la fuente de 48V del driver DM860.
3. Verificar en el monitor serial el inicio del sistema:
   ```text
   ==================================================
    PLANTA PILOTO DE ULTRAFILTRACIÓN FX100 — UNSa   
    Firmware V5.1: Motor Anti-EMI & Datalogger 10s   
   ==================================================
   [WIFI] Punto de Acceso Estable Creado:
          SSID: Bomba_Peristaltica_UF | Pass: plantapiloto2
          IP AP: http://192.168.4.1
   ```
4. **Prueba del Portal Cautivo Automático:**
   * Tomar el teléfono celular o laptop y conectarse a la red Wi-Fi `Bomba_Peristaltica_UF`.
   * Introducir la contraseña: `plantapiloto2`.
   * **Resultado esperado:** En menos de 3 segundos debe saltar la notificación *"Acceder a la red Wi-Fi"* y abrirse automáticamente el SCADA en pantalla completa. (Si no abre automáticamente, entrar a `http://192.168.4.1` en Chrome o Safari).

---

### ETAPA 2: Prueba de Certificación en Seco (Sin Agua) — Cero Hidráulico
> **Objetivo:** Demostrar ante el jurado y auditores que el motor girando a alta velocidad no inyecta pulsos falsos en el sistema.

1. **Condición Física:** Caudalímetros vacíos (sin agua circulando).
2. En el navegador web del celular o PC, activar el Modo Seco:
   ```text
   http://192.168.4.1/cmd?act=MODO_SECO_ON
   ```
   *(El monitor serial confirmará: `[AUDITORIA] Modo Seco ACTIVADO`).*
3. En la interfaz web del SCADA, encender la bomba y configurar los siguientes escalones:
   * **20 RPM** durante 30 segundos.
   * **40 RPM** durante 30 segundos.
   * **60 RPM** durante 30 segundos.
   * **80 RPM** durante 30 segundos.
4. **Criterio de Aceptación:**
   * En la pantalla web del SCADA: $Q_{\text{alim}} = 0.0\text{ mL/min}$ y $Q_{\text{perm}} = 0.0\text{ mL/min}$.
   * En el monitor serial (`[TELEMETRIA]`):
     `V:0` (pulsos válidos = 0). Si hay picos electromagnéticos del driver, se observará que `G` (glitches) aumenta, pero `V` se mantiene en 0.
   * La bandera `ruido_seco` debe permanecer en `false`.
5. Desactivar el Modo Seco antes de ingresar agua:
   ```text
   http://192.168.4.1/cmd?act=MODO_SECO_OFF
   ```

---

### ETAPA 3: Calibración Volumétrica de Alimentación con Probeta (Línea Hidráulica)
> **Objetivo:** Calibrar con probeta graduada el caudal real versus RPM y determinar el factor $K_{\text{alim}}$ definitivo.

1. Llenar el tanque de alimentación con agua.
2. Desconectar la manguera a la entrada del cartucho de membrana y dirigirla hacia la probeta graduada de $1000\text{ mL}$.
3. La línea de permeado debe permanecer conectada o purgada (sin líquido circulando).
4. **Ejecución del Ensayo Escalonado:**
   * En el SCADA, presionar **ENCENDER**. (El sistema registrará automáticamente la primera muestra en $t = 0\text{ s}$).
   * Correr cada punto durante **60 segundos cronometrados**:
     * **Punto 1:** 20 RPM $\rightarrow$ Medir volumen recogido en probeta ($V_{\text{probeta}}$).
     * **Punto 2:** 30 RPM $\rightarrow$ Medir $V_{\text{probeta}}$.
     * **Punto 3:** 40 RPM $\rightarrow$ Medir $V_{\text{probeta}}$.
     * **Punto 4:** 50 RPM $\rightarrow$ Medir $V_{\text{probeta}}$.
     * **Punto 5:** 60 RPM $\rightarrow$ Medir $V_{\text{probeta}}$.
     * **Punto 6:** 70 RPM $\rightarrow$ Medir $V_{\text{probeta}}$.
     * **Punto 7:** 80 RPM $\rightarrow$ Medir $V_{\text{probeta}}$.
     * **Punto 8:** 90 RPM $\rightarrow$ Medir $V_{\text{probeta}}$.
5. Presionar **APAGAR** en el SCADA.
6. **Resultado esperado:**
   * Gracias al datalogger de 10 segundos, **no se pierde ningún dato de las bajas RPM**.
   * Durante toda la prueba de alimentación, el caudal de permeado debe marcar **0.0 mL/min** (sin flujo fantasma).

---

### ETAPA 4: Caracterización Experimental del Caudalímetro de Permeado
> **Objetivo:** Responder técnicamente a la objeción metrológica de GPT Astra respecto al funcionamiento del YF-S401 a caudales bajos ($10\text{ a }100\text{ mL/min}$).

1. Conectar el circuito completo con la membrana de ultrafiltración FX100.
2. Dirigir la manguera de salida de permeado hacia una probeta fina de $100\text{ mL}$ o sobre un vaso colocado en una balanza digital tarada a $0.0\text{ g}$.
3. Operar la bomba a velocidades controladas (p. ej. 30, 50 y 70 RPM) con la válvula de retentado parcialmente regulada para generar presión transmembrana suave.
4. Medir durante 2 a 3 minutos:
   * Volumen o masa real de permeado recogido ($V_{\text{real}}$ o $m_{\text{real}}$ en gramos, donde $1\text{ g} \approx 1\text{ mL}$).
   * Frecuencia promedio en el SCADA ($F_{\text{perm}}$ en Hz).
   * Pulsos acumulados en el datalogger.
5. **Cálculo del Factor $K_{\text{perm}}$ Real:**
   $$K_{\text{perm}} = \frac{F_{\text{prom}} [\text{Hz}] \times 1000}{Q_{\text{real}} [\text{mL/min}]}$$
   * Si la turbina gira reproduciblemente, el valor quedará documentado en la planilla de la tesis como calibración empírica en banco.

---

### ETAPA 5: Descarga y Análisis del Datalogger (CSV)
1. En el navegador del celular o PC, hacer clic en el botón de **Descargar CSV** del SCADA o ingresar a:
   ```text
   http://192.168.4.1/export_csv
   ```
2. Guardar el archivo generado con el nombre:
   `Ensayo_Calibracion_V5_Fecha_Antonella_Owen.csv`.
3. Abrir el archivo en Excel y verificar:
   * Columna `Tiempo_s`: inicia exactamente en $0$, luego $10$, $20$, $30\dots$
   * Columna `Q_Alimentacion_mLmin`: estable y sin oscilaciones espurias.
   * Columna `Q_PERMEADO_mLmin`: $0.0$ en pruebas sin filtración.
   * Columna `Estable_1_0`: $1$ cuando la bomba alcanzó régimen permanente.

---

## 4. PLANILLA DE REGISTRO DE LABORATORIO (PARA IMPRIMIR Y LLENAR)

| RPM Consigna | Tiempo (s) | Vol. Probeta Alim (mL) | Q Real Alim (mL/min) | Q SCADA Alim (mL/min) | Error Relativo (%) | Q SCADA Perm (mL/min) (Debe ser 0) | Ruido Acústico / Observaciones |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **20** | 60 | | | | | 0.0 | |
| **30** | 60 | | | | | 0.0 | |
| **40** | 60 | | | | | 0.0 | |
| **50** | 60 | | | | | 0.0 | |
| **60** | 60 | | | | | 0.0 | |
| **70** | 60 | | | | | 0.0 | |
| **80** | 60 | | | | | 0.0 | |
| **90** | 60 | | | | | 0.0 | |
