# 🧪 HITO 5: Ensayos Experimentales de Ultrafiltración FX100, Modelado Darcy y Vida Útil
## Modelado de Fenómenos de Transporte, Ensuciamiento (Fouling), Eficiencia de Retrolavado y Calidad de Agua
**Tesistas**: Antonella Guitián & Owen Cañizares  
**Codirector**: Ing. Enzo (Investigación Doctoral en Membranas)

Este hito representa el cierre científico y experimental de la tesis de grado y su vinculación directa con la Beca Doctoral. Su objetivo es ejecutar ensayos sistemáticos de filtración con agua sintética turbia, validar la efectividad del pretratamiento por coagulación-sedimentación, calcular los parámetros de la Ley de Darcy y diseñar la estrategia de operación que maximice la vida útil de la membrana Fresenius FX100 obteniendo agua potable según la norma del Código Alimentario Argentino (CAA).

---

## 🎯 Entregable Maestro del Hito 5
* **Resultados experimentales, validación del modelo de Darcy y capítulo de resultados de tesis completo**: Determinación de la resistencia intrínseca ($R_m$), cinética de ensuciamiento ($R_{torta}$ y $R_{irrev}$), cálculo del Índice de Recuperación de Flujo post-retrolavado ($FRR \ge 90\%$), cumplimiento de los parámetros de agua potable del Código Alimentario Argentino (CAA Art. 982) y consolidación de gráficas de alta resolución para el informe final de tesis.

---

## 📋 Lista de Verificación Maestra (Checklist del Hito 5)
- [ ] Membrana Fresenius FX100 purgada y montada verticalmente con conexiones libres de aire.
- [ ] Ensayo con agua limpia (CWF) ejecutado a 20, 40, 60 y 80 RPM.
- [ ] Regresión lineal $\text{TMP}$ vs $J_{20}$ graficada con $R^2 \ge 0.98$, obteniendo el valor experimental de $R_m$.
- [ ] Ensayo comparativo de ensuciamiento completado: Agua cruda sin tratar ($500\text{ NTU}$) vs. Sobrenadante clarificado con *Opuntia*.
- [ ] Reducción en la velocidad de colmatación ($dR/dt \ge 70\%$) confirmada cuantitativamente.
- [ ] Ciclo de retrolavado hidráulico (*Backwash*) probado invirtiendo el giro de la bomba peristáltica durante $60\text{ s}$.
- [ ] Cálculo del Índice de Recuperación de Flujo ($FRR$) documentado en la planilla.
- [ ] Parámetros de calidad del agua permeada verificados contra el Código Alimentario Argentino (Turbidez $< 3\text{ NTU}$, TDS $< 1500\text{ ppm}$).
- [ ] Figuras científicas vectoriales generadas mediante el script de Python o Excel listas para su inserción en la tesis.

---

## 📚 1. Modelo Matemático de Fenómenos de Transporte (Ley de Darcy)

El flujo volumétrico específico a través de los capilares de Polisulfona ($J$) se rige por la Ley de Darcy modificada para membranas de fibra hueca:

$$J = \frac{Q_p}{A_m} = \frac{\text{TMP}}{\mu(T) \cdot R_{\text{total}}} \quad \left[\frac{\text{L}}{\text{m}^2\cdot\text{h}}\right]$$

Donde la resistencia hidráulica total ($R_{\text{total}}$) se descompone en resistencias en serie:

$$R_{\text{total}} = R_m + R_{\text{torta}} + R_{\text{irrev}}$$

* $A_m$: Área interfacial efectiva de la membrana FX100 ($2.2\text{ m}^2$).
* $\mu(T)$: Viscosidad dinámica del agua a la temperatura de ensayo ($\text{Pa}\cdot\text{s}$).
* $R_m$: Resistencia intrínseca de la membrana limpia ($\text{m}^{-1}$).
* $R_{\text{torta}}$: Resistencia por acumulación de flóculos y partículas sobre la pared capilar.
* $R_{\text{irrev}}$: Resistencia irreversible por adsorción o bloqueo de microporos.

---

## 🧪 2. Protocolo Resumido de Ensayos Experimentales para la Tesis

### Ensayo 1: Determinación de la Resistencia Intrínseca Limpia ($R_m$)
1. Cargar el sistema con agua desionizada o potable limpia ($< 1\text{ NTU}$).
2. Operar la bomba a 4 escalones de caudal: $20, 40, 60 \text{ y } 80\text{ RPM}$.
3. Registrar la $\text{TMP}$ y el caudal $Q_p$ para cada escalón.
4. Graficar $\text{TMP}$ vs. $J_{20}$: La pendiente de la recta es exactamente $\mu_{20} \cdot R_m$.

### Ensayo 2: Filtración Directa (Sin Coagulación) vs. Filtración con Pretratamiento
1. **Lote A (Filtración Directa)**: Alimentar agua cruda turbia ($500\text{ NTU}$) directo a la membrana FX100. Medir el tiempo hasta que la $\text{TMP}$ alcanza el límite de seguridad de $0.50\text{ atm}$.
2. **Lote B (Con Pretratamiento Hito 3)**: Realizar coagulación rápida con *Opuntia* ($150\text{ RPM}$), floculación lenta ($30\text{ RPM}$) y decantación ($30\text{ min}$). Alimentar el sobrenadante ($< 20\text{ NTU}$) a la membrana.
3. **Resultado de Tesis**: Demostrar que el pretratamiento reduce la velocidad de colmatación ($dR/dt$) en más de un $70\%$, multiplicando la vida útil del módulo capilar.

---

## 🔄 3. Eficiencia de Recuperación por Retrolavado (Backwash)

Al finalizar cada ciclo de filtración, se invierte el sentido de giro de la bomba peristáltica (Giro Antihorario) durante $60\text{ s}$ para desprender la torta superficial:

$$FRR (\%) = \left(\frac{J_{\text{post-lavado}}}{J_{\text{inicial}}}\right) \times 100$$

---

## 📂 Estructura y Navegación de Subcarpetas de este Hito:

* 📁 **[`01_Protocolo_Ensayos_Darcy_y_Fouling/`](./01_Protocolo_Ensayos_Darcy_y_Fouling/)**:
  * 🧪 **[`Protocolo_Ensayos_Darcy_y_VidaUtil.md`](./01_Protocolo_Ensayos_Darcy_y_Fouling/Protocolo_Ensayos_Darcy_y_VidaUtil.md)**: Protocolo científico exhaustivo de ensayos de ultrafiltración, factor de temperatura TCF, resistencias en serie y tabla comparativa del Código Alimentario Argentino (CAA).
* 📁 **[`02_Planillas_y_Datos_Tesis/`](./02_Planillas_y_Datos_Tesis/)**:
  * 📊 **[`Guia_Analisis_Datos_Graficos_Tesis.md`](./02_Planillas_y_Datos_Tesis/Guia_Analisis_Datos_Graficos_Tesis.md)**: Guía metodológica con script de Python para generación automática de figuras científicas vectoriales ($300\text{ DPI}$) e instrucciones paso a paso para Excel.
  * 📑 **[`plantilla_datos_ensayo_tesis.csv`](./02_Planillas_y_Datos_Tesis/plantilla_datos_ensayo_tesis.csv)**: Matriz de datos experimental con telemetría segundo a segundo, caudales, presiones, flujos y resistencias.
