# 🧪 HITO 6: Ensayos Experimentales de Ultrafiltración, Modelado Darcy y Vida Útil de la Membrana FX100

Este hito representa el cierre científico y experimental de la tesis de grado. Su objetivo es ejecutar ensayos sistemáticos de filtración con agua sintética turbia, validar la efectividad del pretratamiento por coagulación-sedimentación, calcular los parámetros de la Ley de Darcy y diseñar la estrategia de operación que maximice la vida útil de la membrana Fresenius FX100 obteniendo agua potable según la norma del Código Alimentario Argentino (CAA).

---

## 📚 1. Modelo Matemático de Fenómenos de Transporte (Ley de Darcy)

El flujo volumétrico específico a través de los capilares de Polisulfona ($J$) se rige por la Ley de Darcy modificada para membranas de fibra hueca:

$$J = \frac{Q_p}{A_m} = \frac{\text{TMP}}{\mu(T) \cdot R_{\text{total}}} \quad \left[\frac{\text{L}}{\text{m}^2\cdot\text{h}}\right]$$

Donde la resistencia hidráulica total ($R_{\text{total}}$) se descompone en:

$$R_{\text{total}} = R_m + R_{\text{torta}} + R_{\text{poros}}$$

* $A_m$: Área interfacial efectiva de la membrana FX100 ($2.2\text{ m}^2$).
* $\mu(T)$: Viscosidad dinámica del agua a la temperatura de ensayo ($\text{Pa}\cdot\text{s}$).
* $R_m$: Resistencia intrínseca de la membrana limpia ($\text{m}^{-1}$).
* $R_{\text{torta}}$: Resistencia por acumulación de flóculos y partículas en la superficie interna de los capilares.
* $R_{\text{poros}}$: Resistencia irreversible por adsorción o bloqueo de poros.

---

## 🧪 2. Protocolo de Ensayos Experimentales para la Tesis

### Ensayo 1: Determinación de la Resistencia Intrínseca Limpia ($R_m$)
1. Cargar el sistema con agua desionizada o potable limpia ($< 1\text{ NTU}$).
2. Operar la bomba a 4 escalones de caudal: $20, 40, 60 \text{ y } 80\text{ RPM}$.
3. Registrar la $\text{TMP}$ y el caudal $Q_p$ para cada escalón.
4. Graficar $\text{TMP}$ vs. $J_{20}$: La pendiente de la recta es exactamente $\mu_{20} \cdot R_m$.

### Ensayo 2: Filtración Directa (Sin Coagulación) vs. Filtración con Pretratamiento
1. **Lote A (Filtración Directa)**: Alimentar agua cruda turbia ($500\text{ NTU}$) directo a la membrana FX100. Medir el tiempo hasta que la $\text{TMP}$ alcanza el límite de seguridad de $0.50\text{ atm}$.
2. **Lote B (Con Pretratamiento Hito 4)**: Realizar coagulación rápida con *Opuntia* ($150\text{ RPM}$), floculación lenta ($30\text{ RPM}$) y decantación ($30\text{ min}$). Alimentar el sobrenadante ($< 20\text{ NTU}$) a la membrana.
3. **Resultado de Tesis**: Demostrar que el pretratamiento reduce la velocidad de colmatación ($dR/dt$) en más de un $70\%$, multiplicando la vida útil del módulo capilar.

---

## 🔄 3. Eficiencia de Recuperación por Retrolavado (Backwash)

Al finalizar cada ciclo de filtración, se invierte el sentido de giro de la bomba peristáltica (Giro Antihorario) durante $60\text{ s}$ para desprender la torta de lodos:

$$\text{Eficiencia de Limpieza } \eta_{\text{BW}} (\%) = \left(\frac{J_{\text{post-lavado}} - J_{\text{colmatado}}}{J_{\text{inicial}} - J_{\text{colmatado}}}\right) \times 100$$

---

## 📊 4. Plantilla de Registro de Datos

Utilizar el archivo [`plantilla_datos_ensayo_tesis.csv`](./plantilla_datos_ensayo_tesis.csv) para volcar las lecturas segundo a segundo. Este archivo es 100% compatible para importación directa en **Python (Pandas / Matplotlib)** o **Microsoft Excel** para la generación de las gráficas finales del libro de tesis.
