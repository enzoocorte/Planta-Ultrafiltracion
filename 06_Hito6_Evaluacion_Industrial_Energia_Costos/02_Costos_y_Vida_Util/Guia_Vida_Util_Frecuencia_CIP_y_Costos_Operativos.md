# Guía Técnica y Metodológica - Subhito 6.2
## Estimación de Vida Útil de Membranas, Frecuencia de CIP y Costos Unitarios de Tratamiento (OPEX)

**Proyecto:** Planta Piloto de Ultrafiltración para Agua de Consumo Humano  
**Unidad de Filtración:** Cartucho Fresenius CorDiax FX100 (Helixone® - Polissulfona modificada, $1.8 \text{ m}^2$, $\sim 10.900$ fibras capilares de $\varnothing_{int} = 185 \ \mu\text{m}$)  
**Institución:** Universidad Nacional de Salta (UNSa) - Facultad de Ingeniería  
**Hito Asociado:** Hito 6 - Evaluación Industrial: Eficiencia Energética, Vida Útil y Costos de Escalamiento

---

## 1. Fundamento y Objetivos

### 1.1 Objetivos de Ingeniería
1. **Modelar la degradación progresiva de permeabilidad:** Estimar la evolución de la resistencia irreversible acumulada ($R_{\text{irrev}}$) y el decaimiento del caudal a lo largo de ciclos sucesivos de filtración y retrolavado hidrostático.
2. **Determinar la frecuencia óptima de limpieza química (CIP):** Establecer el umbral técnico-económico que minimice el tiempo de inactividad, el consumo de reactivos químicos y el deterioro estructural de la fibra por exposición química, manteniendo el flujo dentro de especificación.
3. **Calcular la vida útil proyectada del cartucho:** Definir el criterio terminal de descarte de la membrana Helixone® ($FRR < 60\%$ pos-CIP o daño mecánico/químico irreversible).
4. **Desarrollar la estructura de costos operativos unitarios (OPEX en $\text{USD/m}^3$):** Cuantificar el costo real de producción de agua clarificada, contrastando el impacto del pretratamiento con coagulante natural (*Opuntia ficus-indica*) frente a la filtración directa testigo.

---

## 2. Dinámica de Ensuciamiento y Vida Útil del Módulo FX100

### 2.1 Resistencia Hidráulica en Ciclos Repetitivos

En cada ciclo de filtración $k$, la resistencia total al final del ciclo se compone de:

$$R_{\text{total}, k} = R_m + R_{\text{irrev}, k} + R_{\text{rev}, k}$$

Donde:
* $R_m$: Resistencia intrínseca de la membrana limpia ($1.5 - 2.5 \times 10^{11} \text{ m}^{-1}$).
* $R_{\text{rev}, k}$: Resistencia reversible removible mediante retrolavado hidrostático (Backwash, BW).
* $R_{\text{irrev}, k}$: Resistencia irreversible por adsorción en poros y formación de capa sub-superficial tenaz, acumulada hasta el ciclo $k$:

$$R_{\text{irrev}, k} = R_{\text{irrev}, k-1} + \Delta R_{\text{irrev}, k}$$

El retrolavado hidrostático a $1.20 \text{ mca}$ ($\sim 0.118 \text{ bar}$) restaura únicamente la porción $R_{\text{rev}}$. Al inicio del ciclo siguiente $(k+1)$, la resistencia inicial es superior a la del ciclo anterior:

$$R_{0, k+1} = R_m + R_{\text{irrev}, k} > R_{0, k}$$

```
Resistencia Total [m⁻¹]
  ^
  |        /\              /\              /\           [Límite CIP: R_total >= R_crit]
  |       /  \ (BW)       /  \ (BW)       /  \ (BW)     -------------------------------
  |      /    \          /    \          /    \       /
  |     /      \        /      \        /      \     / 
  |    /        \      /        \      /        \   /  
  |   /          \    /          \    /          \ / 
  |  /            \  /            \  /            v  (Limpieza CIP)
  | /              \/              \/              \
  |/ R_irrev(1)     \ R_irrev(2)    \ R_irrev(3)    \ R_rem residuos post-CIP
  +----------------------------------------------------> Tiempo / Ciclos
    Ciclo 1          Ciclo 2          Ciclo 3
```

### 2.2 Eficiencia de Restauración de Flujo ($FRR$)

El índice de recuperación de flujo para cada limpieza (hidrostática o química) se define como:

$$FRR_k = \frac{J_{w, k}}{J_{w, 0}} \times 100\%$$

Donde $J_{w, k}$ es el flujo con agua desionizada/red a $\text{TMP}$ estándar ($0.20 \text{ bar}$) pos-limpieza, y $J_{w, 0}$ es el flujo inicial de la membrana virgen.

### 2.3 Criterio Terminal de Retiro de Membrana
El cartucho Fresenius FX100 se considera agotado para operación continua cuando se cumple cualquiera de las siguientes condiciones:
1. **$FRR_{\text{post-CIP}} < 60\%$:** La resistencia irreversible remanente triplica la resistencia virgen ($R_{\text{rem}} > 3 \cdot R_m$), exigiendo presiones energéticamente inviables.
2. **Caída severa de retención:** Detección de turbidez en el permeado $> 0.5 \text{ NTU}$ atribuible a rotura de fibras capilares por fatiga hidrodinámica o ataque químico.
3. **Exposición química acumulada máxima:** Superación del límite de tolerancia del polímero de polisulfona a pH extremos ($> 50.000 \text{ ppm}\cdot\text{h}$ de cloro activo equivalente).

---

## 3. Optimización de Frecuencia de CIP (Clean-In-Place)

### 3.1 Protocolo Químico Escalable
Para el polisulfonato/Helixone® del FX100, la limpieza CIP consta de dos etapas sucesivas con enjuague intermedio:

1. **Etapa Alcalina / Oxidante (Remoción de materia orgánica y biológica):**
   * Solución: $\text{NaOH}$ al $0.1 \text{ M}$ (pH $11.0 - 11.5$) + $\text{NaOCl}$ ($100 - 200 \text{ ppm}$ Cl activo).
   * Temperatura: $30 - 35 ^\circ\text{C}$ (ó ambiente si no hay calentamiento auxiliar).
   * Recirculación: $15 \text{ min}$ en lumen + remojo estático $15 \text{ min}$.
   * Enjuague: Agua neutra hasta $\text{pH} \approx 7.0$ y conductividad de entrada.
2. **Etapa Ácida (Disolución de precipitados inorgánicos y sales metálicas):**
   * Solución: Ácido cítrico al $1.5 - 2.0 \% \text{ p/v}$ (pH $2.2 - 2.5$).
   * Recirculación: $15 \text{ min}$ en lumen + remojo estático $10 \text{ min}$.
   * Enjuague final: Agua neutra hasta $\text{pH} \approx 6.8 - 7.2$.

### 3.2 Balance Técnico-Económico de Frecuencia CIP
El intervalo óptimo de CIP ($T_{\text{CIP}}^*$ en horas de filtración o $V_{\text{CIP}}^*$ en $\text{m}^3$) se determina resolviendo el compromiso entre tres costos contrapuestos:

$$\min_{T_{\text{CIP}}} \ C_{\text{ciclo}}(T_{\text{CIP}}) = \frac{C_{\text{CIP\_evento}} + \int_0^{T_{\text{CIP}}} \left[ C_{\text{energia}}(t) + C_{\text{agua\_rechazo}}(t) \right] dt}{V_{\text{permeado}}(T_{\text{CIP}})}$$

* **Si el CIP es muy frecuente:** Se incurre en alto gasto de reactivos químicos, mayor generación de efluente de neutralización, pérdida de disponibilidad de la planta y mayor degradación acelerada de la polisulfona.
* **Si el CIP es muy espaciado:** La membrana opera con $\text{TMP}$ elevada (alto consumo de energía eléctrica del motor paso a paso NEMA 34), el ensuciamiento por torta se compacta volviéndose irreversible, y la productividad media $\bar{J}$ decae drásticamente.

**Regla de Operación de la Planta Piloto:**  
Se ejecuta CIP cuando se verifique:
$$\Delta \text{TMP} \ge +50\% \text{ respecto a la TMP inicial del ciclo, o } FRR_{\text{post-BW}} \le 70\%$$

---

## 4. Efecto Protector del Pretratamiento con *Opuntia ficus-indica*

Uno de los aportes científicos clave del proyecto es cuantificar cómo el mucílago de tuna actúa como **capa de sacrificio dinámica**:

| Parámetro | Sin Coagulante (Control Testigo) | Con *Opuntia ficus-indica* ($40 \text{ mg/L}$) | Mecanismo Físico-Químico |
| :--- | :--- | :--- | :--- |
| **Mecanismo Dominante de Hermia** | Bloqueo estándar / intermedio ($n = 1.5, 1$) | Filtración sobre torta porosa ($n = 0$) | Los flóculos bio-poliméricos retienen coloides antes de que penetren los poros de $30 \text{ nm}$. |
| **Porcentaje Reversible ($R_{\text{rev}} / R_{\text{total}}$)** | $45 - 60\%$ | $80 - 92\%$ | La torta de mucílago es hidrofílica, esponjosa y se desprende fácilmente en retrolavado. |
| **Tasa de Acumulación $d(R_{\text{irrev}})/dt$** | Alta ($3.2 \times 10^9 \text{ m}^{-1}\text{/h}$) | Baja ($0.8 \times 10^9 \text{ m}^{-1}\text{/h}$) | Escasa adsorción de sustancias húmicas y arcillas directamente en las paredes luminales de Helixone®. |
| **Frecuencia Requerida de CIP** | Cada $12 - 24 \text{ h}$ de filtración | Cada $80 - 150 \text{ h}$ de filtración | Reducción de $\sim 75\%$ en el uso de soda cáustica y ácido cítrico. |
| **Vida Útil Proyectada del Módulo** | $600 - 900 \text{ h}$ de servicio | $2.500 - 4.000 \text{ h}$ de servicio | Extensión de la vida útil en un factor de $3\times$ a $4\times$. |

---

## 5. Estructura y Cálculo del OPEX ($\text{USD/m}^3$)

El costo operativo unitario por metro cúbico de agua clarificada producida se estructura en cuatro componentes elementales:

$$\text{OPEX} = C_{\text{energia}} + C_{\text{coagulante}} + C_{\text{CIP}} + C_{\text{reposicion}}$$

### 5.1 Componente Energético ($C_{\text{energia}}$)
$$C_{\text{energia}} = \text{SEC} \times P_e \quad \left[\frac{\text{USD}}{\text{m}^3}\right]$$
* $\text{SEC}$: Consumo Específico de Energía calculado en Subhito 6.1 (rango $0.20 - 0.55 \text{ kWh/m}^3$).
* $P_e$: Tarifa eléctrica industrial/local (valor de referencia Salta / EDESA: $\approx 0.08 - 0.12 \text{ USD/kWh}$).

### 5.2 Componente de Coagulante ($C_{\text{coagulante}}$)
Para *Opuntia ficus-indica*:
$$C_{\text{coagulante}} = \text{Dosis} \times C_{\text{materia\_prima\_prep}} \quad \left[\frac{\text{USD}}{\text{m}^3}\right]$$
* Dosis óptima: $0.040 \text{ kg/m}^3$ ($40 \text{ mg/L}$).
* Costo de obtención y acondicionamiento (cosecha cladodios, corte, extracción acuosa, deshidratación o filtrado simple): $\approx 0.40 - 0.80 \text{ USD/kg}$.
* Costo unitario: $C_{\text{coagulante}} \approx 0.016 - 0.032 \text{ USD/m}^3$.
*(A modo comparativo, el Sulfato de Aluminio al 16% cuesta $\approx 0.35 \text{ USD/kg}$ con dosis de $30 \text{ mg/L} \implies 0.011 \text{ USD/m}^3$, pero genera lodos tóxicos con disposición controlada).*

### 5.3 Componente de Limpieza Química CIP ($C_{\text{CIP}}$)
$$C_{\text{CIP}} = \frac{V_{\text{quim}} \times \left( C_{\text{NaOH}} \cdot c_{\text{NaOH}} + C_{\text{NaOCl}} \cdot c_{\text{NaOCl}} + C_{\text{AcCit}} \cdot c_{\text{AcCit}} \right) + C_{\text{agua\_lavado}}}{V_{\text{ciclo\_tratado}}} \quad \left[\frac{\text{USD}}{\text{m}^3}\right]$$
* Volumen de circuito CIP de la planta: $V_{\text{quim}} \approx 4 \text{ L} = 0.004 \text{ m}^3$.
* Reactivos por evento:
  * Ácido Cítrico: $60 \text{ g}$ a $3.00 \text{ USD/kg} = 0.18 \text{ USD}$.
  * Hidróxido de Sodio: $16 \text{ g}$ a $1.80 \text{ USD/kg} = 0.03 \text{ USD}$.
  * Hipoclorito de Sodio: $10 \text{ mL}$ conc. a $0.80 \text{ USD/L} = 0.01 \text{ USD}$.
  * Costo reactivos por lavado: $\approx 0.22 \text{ USD/evento}$.
* Con $T_{\text{CIP}} = 100 \text{ h}$ a flujo medio de $35 \text{ L/h}$, el volumen tratado por ciclo es $3.5 \text{ m}^3$:
$$C_{\text{CIP}} \approx \frac{0.22 \text{ USD}}{3.5 \text{ m}^3} \approx 0.063 \text{ USD/m}^3$$

### 5.4 Componente de Reposición y Amortización de Membrana ($C_{\text{reposicion}}$)
$$C_{\text{reposicion}} = \frac{C_{\text{cartucho\_FX100}}}{V_{\text{total\_vida\_util}}} \quad \left[\frac{\text{USD}}{\text{m}^3}\right]$$
* Costo unitario de reposición de cartucho Fresenius FX100 (nuevo/sanitario): $\approx 45 - 65 \text{ USD}$.
* Volumen total producido a lo largo de su vida útil:
  * Operando **con *Opuntia*** ($3.000 \text{ h}$ a $35 \text{ L/h}$): $V_{\text{total}} = 105 \text{ m}^3 \implies C_{\text{reposicion}} \approx \frac{55 \text{ USD}}{105 \text{ m}^3} \approx 0.52 \text{ USD/m}^3$.
  * Operando **sin coagulante** ($750 \text{ h}$ a $30 \text{ L/h}$): $V_{\text{total}} = 22.5 \text{ m}^3 \implies C_{\text{reposicion}} \approx \frac{55 \text{ USD}}{22.5 \text{ m}^3} \approx 2.44 \text{ USD/m}^3$.

---

## 6. Cuadro Resumen Comparativo de Escenarios de Costos

| Rubro de Costo | Escenario A: Control Testigo (Sin Coagulante) | Escenario B: Con *Opuntia ficus-indica* ($40 \text{ mg/L}$) | Ahorro / Impacto Porcentual |
| :--- | :--- | :--- | :--- |
| **Energía Eléctrica ($C_{\text{energia}}$)** | $0.054 \text{ USD/m}^3$ (SEC $= 0.54 \text{ kWh/m}^3$) | $0.028 \text{ USD/m}^3$ (SEC $= 0.28 \text{ kWh/m}^3$) | **-48.1%** (menor $\text{TMP}$ media) |
| **Coagulante ($C_{\text{coag}}$)** | $0.000 \text{ USD/m}^3$ | $0.024 \text{ USD/m}^3$ | Inversión compensada ampliamente |
| **Limpieza Química CIP ($C_{\text{CIP}}$)** | $0.314 \text{ USD/m}^3$ (Lavado c/15 h) | $0.063 \text{ USD/m}^3$ (Lavado c/100 h) | **-79.9%** (menor frecuencia CIP) |
| **Reposición Membrana ($C_{\text{reposicion}}$)**| $2.440 \text{ USD/m}^3$ ($750 \text{ h}$ vida) | $0.524 \text{ USD/m}^3$ ($3.000 \text{ h}$ vida) | **-78.5%** (preservación de fibras) |
| **TOTAL OPEX ESTIMADO** | **$2.808 \text{ USD/m}^3$** | **$0.639 \text{ USD/m}^3$** | **-77.2% REDUCCIÓN GLOBAL** |

> [!NOTE]
> La aplicación de *Opuntia ficus-indica* reduce el costo operativo unitario en más de un **$77\%$**. Esto demuestra que la torta biopolimérica de sacrificio no solo incrementa la retención y la calidad sanitaria del agua, sino que es el factor determinante para la viabilidad económica del escalamiento rural e industrial de la ultrafiltración capilar.

---

## 7. Script de Modelado y Análisis Numérico

Para automatizar la simulación de vida útil y la sensibilidad del OPEX, se provee el script `evaluacion_industrial_opex.py` ubicado en la carpeta de cálculo del Hito 6.

### Variables de Entrada del Modelo
```python
# Parámetros operativos y económicos del modelo OPEX
COSTO_FX100_USD = 55.0         # Costo unitario del cartucho Helixone FX100
TARIFA_KWH_USD = 0.10          # Costo energía eléctrica (USD/kWh)
COSTO_OPUNTIA_KG_USD = 0.60    # Costo preparación de mucílago seco (USD/kg)
COSTO_CIP_EVENTO_USD = 0.22    # Costo insumos reactivos por ciclo CIP (USD)

VOL_CIRCUITO_CIP_L = 4.0       # Volumen solución de lavado (Litros)
HORAS_OPERACION_DIARIA = 16.0  # Horas de servicio por día
```

---

## 8. Verificación y Aprobación

* **Responsable Técnico:** Enzo - Investigador / Desarrollador Principal  
* **Cátedra / Proyecto:** Proyecto Final de Ingeniería Química - Planta Piloto Ultrafiltración UNSa  
* **Estado:** Aprobado e Incorporado en Línea de Base del Proyecto
