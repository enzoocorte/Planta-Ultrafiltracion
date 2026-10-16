# 📈 GUÍA SUBHITO 5.5: Modelado Cinético de Fouling (Modelos de Hermia) y Optimización por Superficie de Respuesta (RSM / ANOVA)
## Ajuste Matemático de Series Temporales J(t), Determinación de Mecanismo Rector y Diseño Factorial 3D

**Proyecto:** Planta Piloto de Ultrafiltración por Flujo Cruzado & Reactor de Coagulación-Sedimentación  
**Tesistas de Grado:** Antonella Guitián & Owen Cañizares  
**Codirector:** Ing. Enzo *(Tesis Doctoral en Membranas)*  
**Ubicación:** Laboratorio de Ingeniería, Universidad Nacional de Salta (UNSa)  
**Fecha:** Octubre 2026  

---

## 🎯 1. Objetivo del Subhito 5.5
1. Analizar las series temporales de flujo permeado $J(t)$ y $J_{20}(t)$ registradas cada 10 segundos en los archivos `.CSV` de las 13 corridas experimentales de ultrafiltración.
2. Ajustar los **4 modelos clásicos de bloqueo de Hermia** ($n = 2.0, 1.5, 1.0, 0$) para identificar el mecanismo físico preponderante de colmatación (bloqueo completo, estándar interno, intermedio o filtración por formación de torta).
3. Comparar cinéticamente la **Corrida Testigo (agua cruda directa sin coagular)** contra las **corridas pretratadas con *Opuntia ficus-indica*** para demostrar cuantitativamente la transición y retardo del mecanismo de ensuciamiento.
4. Construir la **Superficie de Respuesta 3D (RSM)** y la tabla **ANOVA** de la matriz factorial $3^2$ (Factores: RPM y TMP) para hallar el punto óptimo $(\text{RPM}^*, \text{TMP}^*)$.

---

## 🔬 2. Ecuaciones Rectoras de los Modelos de Hermia

La ley diferencial general de Hermia (1982) para filtración a presión constante se expresa como:

$$\frac{d^2 t}{d V^2} = k \left(\frac{d t}{d V}\right)^n \quad \Longleftrightarrow \quad -\frac{dJ}{dt} = K_H \, J \, (A_m J)^{2-n}$$

Donde el índice adimensional $n$ define el mecanismo de bloqueo:

```text
┌──────────────────────────────────────┬───────┬──────────────────────────────────┬─────────────────────────────────┐
│ Mecanismo de Ensuciamiento           │   n   │ Ecuación Diferencial             │ Forma Linealizada para Regresión│
├──────────────────────────────────────┼───────┼──────────────────────────────────┼─────────────────────────────────┤
│ 1. Bloqueo Completo de Poros         │  2.0  │ - dJ/dt = k_b · J                │ ln(J / J0) = - k_b · t          │
│ 2. Bloqueo Estándar Interno          │  1.5  │ - dJ/dt = k_s · J^(1.5)          │ 1/√J - 1/√J0 = (k_s / 2) · t    │
│ 3. Bloqueo Intermedio                │  1.0  │ - dJ/dt = k_i · J^2              │ 1/J - 1/J0 = k_i · t            │
│ 4. Filtración por Formación de Torta │  0.0  │ - dJ/dt = k_c · J^3              │ 1/J² - 1/J0² = 2 · k_c · t      │
└──────────────────────────────────────┴───────┴──────────────────────────────────┴─────────────────────────────────┘
```

### Protocolo de Ajuste para Antonella y Owen:
1. Extraer las columnas `Tiempo_s` y `J_LMH` del `.CSV` de cada ensayo.
2. Calcular las transformaciones para cada modelo:
   * **Modelo 1:** $Y_1 = \ln(J/J_0)$ vs $t$. Pendiente $= -k_b$.
   * **Modelo 2:** $Y_2 = (1/\sqrt{J} - 1/\sqrt{J_0})$ vs $t$. Pendiente $= k_s/2$.
   * **Modelo 3:** $Y_3 = (1/J - 1/J_0)$ vs $t$. Pendiente $= k_i$.
   * **Modelo 4:** $Y_4 = (1/J^2 - 1/J_0^2)$ vs $t$. Pendiente $= 2 k_c$.
3. Evaluar el coeficiente de correlación $R^2$ para cada modelo. El modelo con mayor $R^2$ ($R^2 > 0.95$) describe el mecanismo dominante en esa etapa del ensayo.
4. **Análisis de Etapas Múltiples:** Es común que durante los primeros 5 a 10 minutos domine el bloqueo intermedio ($n=1$) o estándar ($n=1.5$), y luego de los 15 minutos se consolide la formación de torta ($n=0$).

---

## 🌿 3. Contraste Científico: Agua Cruda vs. Coagulada con *Opuntia*

| Muestra Ensayada (70 RPM, 0.25 bar) | Mecanismo Predominante Esperado | Constante Cinética | Interpretación Física |
| :--- | :--- | :--- | :--- |
| **Corrida Testigo (Agua Cruda Directa)** | Bloqueo Estándar / Intermedio ($n=1.5$ a $1.0$) | $k$ muy elevado | Coloides finos penetran en los poros de $0.01\,\mu\text{m}$, provocando bloqueo irreversible severo y caída abrupta de flujo. |
| **Punto Central con *Opuntia* (Hito 3)** | Formación de Torta Suelta ($n=0.0$) | $k_c$ reducido | El biocoagulante aglomera los coloides en macroflóculos que no entran al poro y son barridos por el flujo cruzado tangencial. |

---

## 📊 4. Metodología de Superficie de Respuesta (RSM) y ANOVA 3D

Para la matriz factorial $3^2$:
* **Factor $X_1$:** Caudal de Impulsión / RPM ($40, 70, 95\text{ RPM}$).
* **Factor $X_2$:** Presión Transmembrana TMP ($0.10, 0.25, 0.40\text{ bar}$).
* **Variables de Respuesta ($Y$):**
  1. Flujo Estacionario Final: $J_{\text{ss}}\ [\text{LMH}]$.
  2. Resistencia de Torta: $R_{\text{torta}}\ [\text{m}^{-1}]$.
  3. Factor de Recuperación de Flujo: $FRR\ [\%]$.

### Modelo Polinomial de Segundo Orden:
$$Y = \beta_0 + \beta_1 X_1 + \beta_2 X_2 + \beta_{12} X_1 X_2 + \beta_{11} X_1^2 + \beta_{22} X_2^2 + \varepsilon$$

### Script Python de Automatización (para la Tesis):
```python
import numpy as np
import pandas as pd
from scipy.optimize import curve_fit
import statsmodels.api as sm
from statsmodels.formula.api import ols

# Cargar CSV del ensayo
df = pd.read_csv("PlantaUF_MonitoreoTorta_Ensayo_1.csv", sep=";", decimal=".")
t = df["Tiempo_s"].values
J = df["J_LMH"].values
J0 = J[0]

# 1. Ajuste Hermia n=0 (Torta)
y_cake = (1.0 / (J**2)) - (1.0 / (J0**2))
popt_c, _ = np.polyfit(t, y_cake, 1, cov=True)
print(f"Constante de torta 2*kc: {popt_c[0]:.4e}")
```

---

## 📋 5. Entregables del Subhito 5.5 para la Tesis
1. Tabla con los coeficientes $R^2$ y constantes cinéticas ($k_b, k_s, k_i, k_c$) de las 13 corridas.
2. Gráfico comparativo de curvas de Hermia: linealización $1/J^2$ vs $t$ para agua cruda vs. agua con *Opuntia*.
3. Gráficas vectoriales 3D de Superficie de Respuesta (RSM) y curvas de contorno de $J_{\text{ss}}$ y $R_{\text{torta}}$.
4. Tabla ANOVA con estadísticos $F$ y p-values ($p < 0.05$) demostrando la significancia del esfuerzo de corte sobre la prevención del fouling.
