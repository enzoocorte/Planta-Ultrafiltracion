# 🏭 HITO 6: Evaluación de Indicadores de Ingeniería Industrial (SEC, Costo Operativo y Vida Útil)
## Consumo Específico de Energía, Proyección de Degradación de Membrana y Viabilidad Económica OPEX

**Proyecto:** Planta Piloto de Ultrafiltración por Flujo Cruzado & Reactor de Coagulación-Sedimentación  
**Tesistas de Grado:** Antonella Guitián & Owen Cañizares  
**Codirector de Tesis:** Ing. Enzo *(Tesis Doctoral en Procesos de Separación por Membranas)*  
**Asesoría Técnica:** Antigravity AI  
**Ubicación:** Laboratorio de Ingeniería, Universidad Nacional de Salta (UNSa)  
**Fecha de Consolidación:** Octubre 2026  

---

## 🎯 1. Propósito y Relevancia en Ingeniería Industrial

El **Hito 6** representa la síntesis y el cierre profesional de la Tesis de Grado en Ingeniería Industrial / Química.  
Mientras que los Hitos 1 al 4 aseguraron la automatización de la planta y el Hito 5 validó la física del transporte por membrana y el pretratamiento con *Opuntia ficus-indica*, el **Hito 6 traduce los datos experimentales en indicadores tecno-económicos y energéticos para la toma de decisiones productivas**:

1. **Eficiencia Energética:** Determinar en qué combinación de caudal y presión $(\text{RPM}, \text{TMP})$ la planta produce agua tratada con el **mínimo consumo eléctrico por metro cúbico** ($\text{SEC}$ en $\text{kWh/m}^3$).
2. **Ciclo de Vida del Activo:** Proyectar la degradación progresiva de permeabilidad por acumulación de ensuciamiento irreversible ($R_{\text{irrev}}$) y determinar la frecuencia óptima de limpiezas químicas (CIP) y recambio del cartucho capilar Fresenius FX100.
3. **Costo Unitario de Producción:** Estructurar los costos operativos ($\text{OPEX}$) por metro cúbico ($\text{USD/m}^3$ y $\$AR/\text{m}^3$) para evaluar la viabilidad de transferir esta tecnología a comunidades rurales aisladas del Noroeste Argentino (NOA).

---

## 🗺️ 2. Subhitos del Hito 6

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│             HITO 6: EVALUACIÓN DE INDICADORES DE INGENIERÍA INDUSTRIAL                 │
├────────────────────────────────────────────────────────────────────────────────────────┤
│ ⚡ SUBHITO 6.1: Consumo Específico de Energía (SEC en kWh/m³)                          │
│    • Medición de potencia eléctrica absorbida (Driver DM860 + Motor NEMA 34 + L298N). │
│    • Cálculo matricial de SEC = P_electrica / Q_perm en cada punto de la matriz 3².    │
│    • Identificación del punto operativo de mínima intensidad energética.               │
├────────────────────────────────────────────────────────────────────────────────────────┤
│ 💰 SUBHITO 6.2: Estimación de Vida Útil, Frecuencia CIP y Costos Operativos (OPEX)     │
│    • Modelado de acumulación de R_irrev ciclo a ciclo y criterio de recambio de FX100. │
│    • Optimización del compromiso entre tiempo de filtración (tf) y retrolavado (tl).   │
│    • Estructura de OPEX: Energía + Biocoagulante Opuntia + Sanitizantes CIP + Membrana.│
│    • Determinación del costo unitario final ($/m³ de agua potable producida).          │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 📋 3. Lista de Verificación Maestra (Checklist del Hito 6)
- [ ] Medición de potencia eléctrica en bornes de alimentación ($24\text{ VAC}$ y $12\text{ VDC}$) para cada nivel de velocidad de bomba ($40, 70, 95\text{ RPM}$) y agitador ($30, 150\text{ RPM}$).
- [ ] Cálculo de la potencia hidráulica teórica: $P_{\text{hid}} = Q_{\text{alim}} \cdot P_1$.
- [ ] Determinación de la eficiencia electromecánica global del tren de impulsión peristáltico: $\eta = P_{\text{hid}} / P_{\text{eléctrica}}$.
- [ ] Matriz de $\text{SEC}\ [\text{kWh/m}^3]$ calculada para las 13 corridas experimentales del Hito 5.
- [ ] Gráfico de curvas de iso-consumo energético específico ($\text{SEC}$ vs $\text{RPM}$ y $\text{TMP}$).
- [ ] Proyección matemática de ciclos útiles hasta alcanzar el límite de recambio ($FRR < 60\%$).
- [ ] Cronograma optimizado de limpiezas físicas periódicas (cada $45\text{ min}$) y químicas CIP (cada $N$ ciclos).
- [ ] Hoja de cálculo de costos unitarios ($\text{OPEX}$) consolidada y lista para la defensa de la tesis.

---

## 📂 4. Estructura de Carpetas de este Hito

* 📁 **[`01_Consumo_Energetico_SEC/`](./01_Consumo_Energetico_SEC/)**:
  * ⚡ **[`Guia_Calculo_SEC_Consumo_Especifico.md`](./01_Consumo_Energetico_SEC/Guia_Calculo_SEC_Consumo_Especifico.md)**: Fundamento termodinámico y eléctrico, fórmulas de cálculo de $\text{SEC}$, instrumentación de potencia y análisis de sensibilidad.
  * 🐍 **[`calculo_sec_matriz13.py`](./01_Consumo_Energetico_SEC/calculo_sec_matriz13.py)**: Script en Python para el procesamiento matricial automatizado de energía en las 13 corridas de la tesis.
* 📁 **[`02_Costos_y_Vida_Util/`](./02_Costos_y_Vida_Util/)**:
  * 💰 **[`Guia_Vida_Util_Frecuencia_CIP_y_Costos_Operativos.md`](./02_Costos_y_Vida_Util/Guia_Vida_Util_Frecuencia_CIP_y_Costos_Operativos.md)**: Proyección de degradación irreversible de Polisulfona/Helixone, optimización de frecuencia de lavado, costos de reactivos y matriz económica unitaria.
  * 🐍 **[`evaluacion_industrial_opex.py`](./02_Costos_y_Vida_Util/evaluacion_industrial_opex.py)**: Script en Python para la simulación de OPEX comparativo ($0.639\text{ USD/m}^3$ con *Opuntia* vs $2.987\text{ USD/m}^3$ sin pretratamiento).

---

## 📊 5. Estado de Avance Actual (Avance Global: 5% — Solo Modelos Teóricos Preliminares)

> [!NOTE]
> **Situación Real de Hito 6:**
> Este hito **aún no se ha iniciado con datos empíricos**. Los scripts en Python (`calculo_sec_matriz13.py` y `evaluacion_industrial_opex.py`) representan herramientas analíticas teóricas de proyección tecno-económica que esperan los datos experimentales reales de la matriz factorial del Hito 5 para su cálculo definitivo.

| Subhito / Actividad | Estado | Porcentaje | Observaciones |
| :--- | :---: | :---: | :--- |
| **Subhito 6.1: Consumo Específico (SEC)** | 🟡 Formulado | 30% | Ecuaciones y script listos; pendiente alimentar con telemetría real. |
| **Subhito 6.2: Vida Útil, CIP y OPEX** | 🟡 Formulado | 30% | Modelo financiero y script listos; a la espera de tasas de fouling de H5. |

---

## 🏁 6. Entregables Críticos para la Defensa de Tesis

1. [ ] Matriz de SEC calculada con datos experimentales reales de planta evaluando el impacto del pretratamiento con *Opuntia*.
2. [ ] Determinación de viabilidad económica con el cálculo real del OPEX unitario por metro cúbico tratado.
3. [ ] Protocolo de descarte y recambio de cartucho dializador Fresenius FX100 validado empíricamente ante $FRR < 60\%$.
4. [ ] **Cierre de Tesis:** Capítulo de Conclusiones e Ingeniería de Escalamiento consolidado para el tribunal examinador de la UNSa.
