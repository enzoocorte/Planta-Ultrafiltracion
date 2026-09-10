# 🧪 PROTOCOLO DE ENSAYO: COAGULACIÓN-FLOCULACIÓN (JAR TEST) & SEDIMENTACIÓN
## Pretratamiento Fisicoquímico para Ultrafiltración FX100 — Tesis de Ingeniería Industrial
**Tesistas**: Antonella Guitián & Owen Cañizares  
**Codirector**: Ing. Enzo (Investigación Doctoral en Membranas)

---

## 🎯 Entregable Concreto de este Protocolo
* **Protocolo experimental de Jar Test validado y estandarizado**: Ejecución de ensayos de remoción de turbidez en el reactor cónico de $5\text{ L}$ utilizando coagulante natural (*Opuntia ficus-indica* o Moringa), determinando la dosis óptima ($\text{mg/L}$), el tiempo de floculación y la eficiencia de clarificación ($\eta \ge 85\%$) para acondicionar el agua antes de su ingreso a la membrana FX100.

---

## 📋 Lista de Verificación (Checklist de Avance)
- [ ] Solución madre de biopolímero coagulante (*Opuntia ficus-indica* al $1\%\text{ p/v}$) preparada y filtrada.
- [ ] Agua sintética estandarizada con bentonita/arcilla coloidal a turbidez inicial conocida ($\approx 400 - 500\text{ NTU}$).
- [ ] Boya de seguridad en acero inoxidable calibrada: corte inmediato si el nivel cae por debajo de $1\text{ L}$.
- [ ] Ciclo de Mezcla Rápida verificado: $150\text{ RPM}$ ($G \approx 400\text{ s}^{-1}$) durante $60\text{ s}$ para dispersión del coagulante.
- [ ] Ciclo de Mezcla Lenta verificado: $30\text{ RPM}$ ($G \approx 35\text{ s}^{-1}$) durante $15\text{ min}$ favoreciendo la colisión sin ruptura de flóculos.
- [ ] Período de sedimentación estática completado ($0\text{ RPM}$) durante $30\text{ min}$.
- [ ] Muestra de sobrenadante extraída a $5\text{ cm}$ bajo la superficie sin perturbar el manto de lodos del fondo cónico.
- [ ] Medición de turbidez final y cálculo de eficiencia de remoción $\eta$ registrado en planilla.

---

## 🔬 1. Fundamentos Fisicoquímicos del Pretratamiento

### 1.1. Estabilidad Coloidal y Potencial Zeta ($\zeta$)
Las partículas que causan turbidez en aguas naturales (arcillas, coloides orgánicos y microorganismos) poseen un tamaño entre $1\text{ nm}$ y $1\,\mu\text{m}$. Debido a sustituciones isomórficas en su red cristalina y grupos ionizados superficiales, estas partículas poseen **carga superficial negativa**, generando una doble capa eléctrica (Modelo de Gouy-Chapman-Stern).
Las fuerzas de repulsión electrostática impiden la aglomeración espontánea, manteniendo la suspensión estable en el tiempo.

### 1.2. Mecanismos de Coagulación con Biopolímeros Naturales (*Opuntia ficus-indica*)
El mucílago de *Opuntia ficus-indica* (penca de tuna/nopal) está compuesto por polisacáridos complejos de alto peso molecular (arabinogalactanos y ácido galacturónico). Opera principalmente mediante:
1. **Neutralización Parcial de Cargas**: Los cationes divalentes presentes interactúan con las cargas negativas de los coloides.
2. **Puente Interparticular (*Polymer Bridging*)**: Las largas cadenas macromoleculares del biopolímero se adsorben simultáneamente en la superficie de dos o más coloides, formando flóculos tridimensionales pesados y densos.
3. **Atrapamiento de Barrido (*Sweep Flocculation*)**: La red polimérica arrastra mecánicamente las partículas suspendidas durante la decantación.

---

## 📐 2. Modelo Hidrodinámico de Camp-Stein y Dimensionamiento del Agitador

### 2.1. Ecuación del Gradiente Medio de Velocidad ($G$)
El parámetro fundamental para el diseño de reactores de mezcla y floculación es el gradiente de velocidad propuesto por Camp y Stein (1943):

$$G = \sqrt{\frac{P}{\mu \cdot V}} = \sqrt{\frac{N_p \cdot \rho \cdot N^3 \cdot D^5}{\mu \cdot V}} \quad \left[\text{s}^{-1}\right]$$

Donde:
* $P$: Potencia disipada transferida por el impulsor al fluido ($\text{W}$).
* $\mu$: Viscosidad dinámica del agua a $20^\circ\text{C}$ ($\approx 1.002 \times 10^{-3}\text{ Pa}\cdot\text{s}$).
* $\rho$: Densidad del agua ($\approx 1000\text{ kg/m}^3$).
* $V$: Volumen útil del reactor ($5\text{ L} = 0.005\text{ m}^3$).
* $N$: Frecuencia de rotación del agitador ($\text{rev/s} = \frac{\text{RPM}}{60}$).
* $D$: Diámetro de la paleta agitadora ($\text{m}$).
* $N_p$: Número de potencia adimensional característico de la geometría del impulsor.

### 2.2. Parámetro Adimensional de Camp ($Gt$)
El producto $Gt$ cuantifica el número total de oportunidades de colisión interparticular:

$$Gt = G \times t$$

```
┌──────────────────────────────┬──────────────────┬───────────────────┬──────────────┬────────────────────────────────────────────┐
│ Etapa de Proceso             │ Velocidad (RPM)  │ Gradiente G (s⁻¹) │ Tiempo (t)   │ Objetivo Hidrodinámico                     │
├──────────────────────────────┼──────────────────┼───────────────────┼──────────────┼────────────────────────────────────────────┤
│ 1. Mezcla Rápida (Coagulación)│ 150 RPM          │ ~ 350 - 450 s⁻¹   │ 60 s         │ Dispersión molecular homogénea (Gt ≈ 24000)│
│ 2. Mezcla Lenta (Floculación) │ 30 RPM           │ ~ 25 - 45 s⁻¹     │ 900 s (15 m) │ Crecimiento de flóculos sin rotura (Gt ≈ 31500)│
│ 3. Sedimentación Estática    │ 0 RPM (Apagado)  │ 0 s⁻¹             │ 1800 s (30 m)│ Decantación gravitacional hacia fondo cónico│
└──────────────────────────────┴──────────────────┴───────────────────┴──────────────┴────────────────────────────────────────────┘
```

> [!WARNING]
> **Criterio de Rotura por Cizallamiento (*Shear Stress*)**: Durante la fase de floculación, si $G > 60\text{ s}^{-1}$ el esfuerzo cortante inducido por los remolinos turbulentos supera la resistencia mecánica de los puentes poliméricos del mucílago, rompiendo los flóculos en micropartículas que luego colmatarán los capilares de la membrana de ultrafiltración.

---

## 🍯 3. Preparación del Biocoagulante (*Opuntia ficus-indica*)

1. **Recolección y Lavado**: Cortar pencas maduras de *Opuntia ficus-indica*, remover espinas superficiales y lavar con agua destilada.
2. **Pelado y Trozado**: Retirar la epidermis verde externa y cortar el parénquima interno (médula translúcida) en cubos de $\approx 1\text{ cm}^3$.
3. **Extracción Acuosa**:
   * Pesar $100\text{ g}$ de parénquima fresco en balanza de precisión.
   * Licuar con $500\text{ mL}$ de agua destilada durante $3\text{ minutos}$ a máxima velocidad.
   * Filtrar la suspensión a través de una malla fina (gasa quirúrgica o filtro textil de $100\,\mu\text{m}$) para separar los restos de fibra insoluble.
4. **Estandarización de la Solución Madre**:
   * Enrasar a $1000\text{ mL}$ con agua destilada en matraz aforado.
   * Concentración nominal de la solución madre: $\approx 10\text{ g/L}$ ($1\%\text{ p/v}$).
   * Almacenar a $4^\circ\text{C}$ en heladera (vida útil: $48\text{ horas}$).

---

## 🧪 4. Procedimiento Experimental de Jar Test en el Reactor Cónico

1. **Llenado del Reactor**: Introducir $5.0\text{ L}$ de agua de ensayo en el reactor cónico.
2. **Medición Testigo Inicial**: Tomar una alícuota de $50\text{ mL}$ y medir turbidez inicial ($T_0$, en $\text{NTU}$) y $\text{TDS}_0$ ($\text{ppm}$).
3. **Dosificación del Coagulante**: Adicionar con pipeta graduada la dosis a evaluar:
   * Dosis baja: $10\text{ mg/L}$ ($5\text{ mL}$ de solución madre en $5\text{ L}$).
   * Dosis media (recomendada): $20\text{ mg/L}$ ($10\text{ mL}$ de solución madre en $5\text{ L}$).
   * Dosis alta: $30\text{ mg/L}$ ($15\text{ mL}$ de solución madre en $5\text{ L}$).
4. **Ejecución de la Secuencia Automatizada**:
   * Enviar el comando `AUTO` por el Monitor Serie o presionar el botón correspondiente en el SCADA.
   * El ESP32 activará el motor a $150\text{ RPM}$ por $60\text{ s}$.
   * Automáticamente reducirá la marcha a $30\text{ RPM}$ por $15\text{ minutos}$.
   * Detendrá el motor ($0\text{ RPM}$) durante $30\text{ minutos}$.
5. **Muestreo del Sobrenadante**: Abrir con suavidad la válvula de purga superior (o pipetear a $5\text{ cm}$ de profundidad) y medir la turbidez final ($T_f$).

---

## 📊 5. Ecuaciones de Cálculo de Eficiencia y Sedimentación

### 5.1. Eficiencia de Remoción de Turbidez ($\eta$)

$$\eta (\%) = \left(\frac{T_0 - T_f}{T_0}\right) \times 100$$

### 5.2. Velocidad de Sedimentación de Flóculos (Ley de Stokes)

$$v_s = \frac{g \cdot (\rho_p - \rho_f) \cdot d_p^2}{18 \cdot \mu}$$

* Donde $d_p$ es el diámetro hidrodinámico equivalente del flóculo y $\rho_p - \rho_f$ es el diferencial de densidad entre el flóculo hidratado y el agua circundante.
* En el fondo cónico del reactor, el ángulo de $\ge 60^\circ$ permite que los lodos resbalen gravitacionalmente hacia la purga inferior, dejando el sobrenadante superior listo para el cabezal de la bomba peristáltica MBP-2000.
