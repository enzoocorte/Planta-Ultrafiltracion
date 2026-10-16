# 🚀 PROMPT MAESTRO PARA ANTIGRAVITY — ACTUALIZACIÓN DE HOJA DE RUTA, SIMPLIFICACIÓN DE PRESIÓN CON MANÓMETROS, CIERRE INDUSTRIAL (HITO 6) Y AUTOPSIA SEM

Copiá y pegá el siguiente bloque directamente en el chat de **Antigravity**:

---

**Hola Antigravity. Necesito que actualices la arquitectura documental y la Hoja de Ruta de nuestro repositorio (`HOJA_DE_RUTA_DESARROLLO_Y_ENSAYOS.md`, `README.md`, `MANUAL_GENERAL_Y_GUIA_DE_NAVEGACION.md` y las guías dentro de `02_Hito2_...`, `04_Hito4_...` y `05_Hito5_...`) aplicando las siguientes decisiones estratégicas de ingeniería para nuestra Tesis:**

### 1. Simplificación del Subhito 2.4 y Hito 4: Reemplazo de Transductores Analógicos (`ADS1115`) por Manómetros con Baño de Glicerina + Carga Asistida en SCADA
* **Decisión de Ingeniería**: Ya adquirimos los **manómetros mecánicos con baño de glicerina** para la planta. No vamos a comprar ni instalar transductores de presión electrónicos (`0-10 bar` + `ADS1115`), dado que nuestro rango operativo de Presión Transmembrana en el `Hito 5` es de muy baja presión (**`0.10 bar`, `0.25 bar` y `0.40 bar`**), donde un transductor genérico de `0-10 bar` trabaja entre el `1%` y el `4%` de su fondo de escala con alto error de deriva de cero (*zero drift*) y susceptibilidad al ruido EMI del driver `DM860`.
* **Cambio a realizar en la documentación y en el SCADA (`Hito 2` y `Hito 4`)**:
  1. Redefinir el **`Subhito 2.4`** en `HOJA_DE_RUTA_DESARROLLO_Y_ENSAYOS.md` y `README.md` como **"Instrumentación Manométrica Primaria ($P_1, P_2$) e Integración Híbrida en SCADA para Cálculo de Darcy en Tiempo Real"**.
  2. Documentar que la lectura de presión de entrada ($P_1$) y salida/retentado ($P_2$) se realiza sobre los manómetros amortiguados en glicerina, y se ingresa en vivo en la interfaz web del SCADA (`http://192.168.4.1`) junto con la temperatura del agua ($T\ [^\circ\text{C}]$), permitiendo que el módulo `darcy.h` del ESP32 calcule automáticamente y exporte en cada fila del `.CSV`:
     * Presión Transmembrana: $\text{TMP} = \frac{P_1 + P_2}{2} - P_3$
     * Factor de Corrección Térmica de Vogel ($\text{TCF}$) y flujo normalizado a $20\ ^\circ\text{C}$ ($J_{20}\ [\text{LMH}]$)
     * Resistencia hidráulica total ($R_{\text{total}}\ [\text{m}^{-1}]$) y resistencia adicional de ensuciamiento/torta ($R_{\text{torta}} = R_{\text{total}} - R_m$)

### 2. Vinculación Explícita entre el Hito 3 (Coagulación con *Opuntia ficus-indica*) y el Hito 5
* En la planificación del **`Hito 5`** (`05_Hito5_Ensayos_Membrana_VidaUtil`), agregar una **Corrida Testigo de Control sin Pretratamiento** en las condiciones del Punto Central (**`70 RPM` y `TMP = 0.25 bar`**) para contrastarla directamente contra las **3 réplicas del Punto Central con agua pretratada mediante coagulación-floculación con *Opuntia ficus-indica***.
* Esto permitirá cuantificar en la tesis el impacto directo del biocoagulante sobre la reducción de $R_{\text{torta}}$, la disminución de la pendiente de caída de flujo $J(t)/J_0$ y la mejora del Factor de Recuperación de Flujo post-retrolavado ($\text{FRR}$).

### 3. Ampliación del Hito 5 con dos nuevos Subhitos de Alto Impacto Científico (`Subhito 5.5` y `Subhito 5.6`)
Dentro de `HOJA_DE_RUTA_DESARROLLO_Y_ENSAYOS.md` y de la carpeta `05_Hito5_Ensayos_Membrana_VidaUtil/`, agregar:
* **`Subhito 5.5 — Modelado Cinético de Mecanismos de Fouling (Modelos de Hermia) y Optimización por Superficie de Respuesta (RSM / ANOVA)`**:
  * Aprovechando las series temporales de $J(t)$ registradas cada $10\text{ segundos}$ en los archivos `.CSV` de las 12 corridas de la matriz factorial $3^2$, ajustar los 4 mecanismos clásicos de bloqueo de Hermia:
    1. Bloqueo completo de poros ($n = 2.0$)
    2. Bloqueo estándar interno ($n = 1.5$)
    3. Bloqueo intermedio ($n = 1.0$)
    4. Filtración por formación de torta ($n = 0$, linealizando $1/J^2\text{ vs }t$)
  * Construcción de la Superficie de Respuesta 3D y tabla ANOVA para identificar la combinación óptima $(\text{RPM}^*, \text{TMP}^*)$.
* **`Subhito 5.6 — Autopsia Morfológica Post-Mortem de las Fibras Capilares Helixone® mediante Microscopía Electrónica de Barrido (SEM / EDS)`**:
  * Ubicado estrictamente al **final de todas las corridas experimentales** (por tratarse de un ensayo destructivo sobre el cartucho `Fresenius FX100`).
  * Protocolo de disección y observación en el Microscopio Electrónico de Barrido (SEM):
    1. **Corte transversal (*Cross-section*)**: medición micrométrica directa del espesor real de la torta ($\delta_c\ [\mu\text{m}]$) sobre la pared asimétrica de `Helixone®` ($185\ \mu\text{m}$ DI, $35\ \mu\text{m}$ espesor) para correlacionar la observación microscópica con la resistencia hidráulica $R_{\text{torta}}$ calculada por el modelo de Darcy en el SCADA.
    2. **Superficie luminal interna (Pre-retrolavado vs. Post-retrolavado vs. Fibra Virgen)**: verificación visual directa del bloqueo de poros residual que origina la resistencia irreversible ($R_{\text{irrev}}$).
    3. **Perfil axial (Entrada, Centro y Salida del módulo)**: evaluación de la distribución longitudinal de la polarización por concentración y depósito de partículas a lo largo de la fibra hueca.
    4. **Microanálisis elemental por EDS (si está disponible en el equipo)**: identificación de la composición inorgánica (arcillas/sales: Si, Al, Ca, Mg, Fe) y orgánica (C, O del floculante *Opuntia*) sobre la superficie de la torta.

### 4. Creación del Nuevo `Hito 6`: Evaluación de Indicadores de Ingeniería Industrial ($\text{SEC}$, Costo Operativo y Vida Útil)
Crear la carpeta `06_Hito6_Evaluacion_Industrial_Energia_Costos/` e incorporarla en `HOJA_DE_RUTA_DESARROLLO_Y_ENSAYOS.md` y `README.md`:
* **`Subhito 6.1 — Consumo Específico de Energía ($\text{SEC}$ en $\text{kWh/m}^3$)`**: Cálculo del consumo energético por metro cúbico de permeado producido en cada punto de la matriz $3^2$ ($\text{SEC} = \frac{P_{\text{eléctrica}}(\text{RPM}, \text{TMP})}{Q_{\text{perm}}}$) para hallar el punto de mínimo costo energético.
* **`Subhito 6.2 — Estimación de Vida Útil del Módulo y Frecuencia Óptima de Limpieza/Recambio`**: Proyección de la degradación de permeabilidad por acumulación de $R_{\text{irrev}}$ ciclo a ciclo y determinación del costo operativo unitario por $\text{m}^3$ de agua tratada.

**Por favor actualiza todos los documentos correspondientes, crea la estructura del `Hito 6` y realiza el commit y push a `origin/main`.**
