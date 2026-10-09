# GUÍA DE ENSAYOS EXPERIMENTALES EN BANCO — SUBHITO 2.2 (FIRMWARE V5.1)

**Planta Piloto de Ultrafiltración Tangencial — Membrana Fresenius FX100 (Helixone®)**  
**Tesistas:** Antonella Guitián & Owen Cañizares  
**Codirección:** Ing. Enzo Corte  
**Ubicación:** Laboratorio de Operaciones Unitarias / Domótica — UNSa, Salta  

---

## 1. OBJETIVO DEL PROTOCOLO
1. **Validación del Filtro Anti-EMI:** Demostrar empíricamente que el firmware V5.1 erradica el 100% del ruido electromagnético inducido por el motor paso a paso NEMA 34 sobre los caudalímetros YF-S401 bajo la condición de hardware congelado ($R = 4.7\text{ k}\Omega, C = 200\text{ nF}$).
2. **Aprovechamiento de los Datos Existentes (Ensayos 2 y 3):** Los valores volumétricos de alimentación ($20\text{ a }90\text{ RPM}$) ya fueron aforados con probeta graduada en las sesiones previas (cilindrada media validada en $13.55\text{ mL/rev}$). Solo se realiza una verificación puntual de control en alimentación.
3. **Foco Principal del Ensayo — Caudal de Permeado:** Caracterizar el caudalímetro de permeado a bajo flujo ($10\text{ a }100\text{ mL/min}$) mediante probeta fina o balanza digital, dejando el sistema listo para incorporar los manómetros (presión TMP) el próximo lunes.
4. **Validación del Datalogger V5:** Confirmar el muestreo cada 10 segundos, inicio garantizado en $t = 0\text{ s}$ y exportación CSV completa sin pérdida de datos.

---

## 2. DATOS HISTÓRICOS YA VALIDADOS CON PROBETA (ENSAYOS 2 Y 3)

Estos datos fueron medidos experimentalmente con probeta graduada de $1000\text{ mL}$ por Antonella, Owen y Enzo. **Ya forman parte de la tesis y no es necesario repetirlos desde cero**:

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

* **Cilindrada media de la bomba:** $13.55\text{ mL/rev}$ (configurada en $13.60\text{ mL/rev}$ nominal en `config.h`).
* **Factor K experimental por regresión:** $172.72\text{ Hz / (L/min)}$ ($R^2 = 0.985$).

---

## 3. PASO A PASO DEL ENSAYO EN EL LABORATORIO

### ETAPA 1: Puesta en Marcha y Verificación de Red Wi-Fi
1. Conectar el ESP32 a la PC por USB (Monitor Serial a **115200 baudios**) y alimentar la etapa de potencia (48V).
2. Conectar el celular a la red Wi-Fi `Bomba_Peristaltica_UF` (clave: `plantapiloto2`).
3. **Verificación:** Gracias a la redirección HTTP 302, el celular debe abrir **automáticamente** la pantalla del SCADA en pantalla completa.

---

### ETAPA 2: Prueba de Certificación en Seco (Sin Agua) — Cero Hidráulico
> **Objetivo:** Demostrar que el motor girando a alta velocidad no induce flujo fantasma en permeado ni en alimentación.

1. **Condición Física:** Caudalímetros vacíos (sin agua circulando).
2. En el navegador del celular o PC, activar el Modo Seco:
   ```text
   http://192.168.4.1/cmd?act=MODO_SECO_ON
   ```
   *(El monitor serial confirmará: `[AUDITORIA] Modo Seco ACTIVADO`).*
3. En la interfaz web del SCADA, encender la bomba y pasar por los escalones:
   * **20 RPM** (30 s) $\rightarrow$ **40 RPM** (30 s) $\rightarrow$ **60 RPM** (30 s) $\rightarrow$ **80 RPM** (30 s).
4. **Criterio de Aceptación:**
   * En la pantalla web del SCADA: $Q_{\text{alim}} = 0.0\text{ mL/min}$ y $Q_{\text{perm}} = 0.0\text{ mL/min}$.
   * En el monitor serial (`[TELEMETRIA]`): `V:0` (pulsos válidos = 0).
   * La bandera `ruido_seco` debe permanecer en `false`.
5. Desactivar el Modo Seco al finalizar:
   ```text
   http://192.168.4.1/cmd?act=MODO_SECO_OFF
   ```

---

### ETAPA 3: Verificación Rápida de Alimentación (Control Puntual a 50 RPM)
> Como ya tenemos la curva de 20 a 90 RPM validada, solo hacemos un control rápido para confirmar estabilidad en la app:

1. Conectar la alimentación con agua y dirigir la manguera a la probeta de 1000 mL.
2. Encender la bomba a **50 RPM** durante 60 segundos.
3. Verificar que el volumen recogido ronde los $\approx 720\text{ mL}$ y que la app muestre un caudal estable alrededor de $\approx 720\text{ mL/min}$.
4. Constatar que durante esta prueba el caudal de permeado marque estrictamente **0.0 mL/min**.

---

### ETAPA 4: Caracterización Experimental del Caudalímetro de Permeado (Bajo Flujo)
> **Objetivo Principal:** Medir el caudal de ultrafiltración real para caracterizar la turbina de permeado en su rango de trabajo ($10\text{ a }100\text{ mL/min}$).

1. Conectar el circuito hidráulico completo a través del cartucho de membrana FX100.
2. Dirigir la manguera de salida de permeado hacia una probeta fina de $100\text{ mL}$ o un vaso sobre una balanza digital tarada a $0.0\text{ g}$.
3. Operar la bomba a velocidades moderadas (p. ej. 30, 50 y 70 RPM) regulando la válvula de retentado para generar flujo de ultrafiltración.
4. Medir durante 2 a 3 minutos:
   * Volumen o masa real de permeado recogido ($V_{\text{real}}$ o $m_{\text{real}}$ en gramos, $1\text{ g} \approx 1\text{ mL}$).
   * Frecuencia promedio en el SCADA ($F_{\text{perm}}$ en Hz).
   * Caudal en el SCADA ($Q_{\text{perm}}$ en mL/min).
5. **Cálculo de Calibración:**
   $$K_{\text{perm}} = \frac{F_{\text{prom}} [\text{Hz}] \times 1000}{Q_{\text{real}} [\text{mL/min}]}$$
6. **Integración con Manómetros (Lunes):**
   * El lunes, al instalar los manómetros en entrada ($P_1$), retentado ($P_2$) y permeado ($P_3$), se medirá la Presión Transmembrana real:
     $$\text{TMP} = \frac{P_1 + P_2}{2} - P_3$$
   * Con la TMP medida y el caudal de permeado caracterizado, el modelo de Darcy ($R_m$) quedará completamente resuelto sin presiones simuladas.

---

### ETAPA 5: Descarga del Datalogger (CSV)
1. Descargar el registro de ensayo ingresando a:
   ```text
   http://192.168.4.1/export_csv
   ```
2. Verificar que los datos arranquen en $t = 0\text{ s}$ con muestras regulares cada 10 segundos.
