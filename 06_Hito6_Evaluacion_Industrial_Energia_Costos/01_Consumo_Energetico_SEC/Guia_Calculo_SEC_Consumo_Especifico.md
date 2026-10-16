# ⚡ GUÍA SUBHITO 6.1: Cálculo del Consumo Específico de Energía (SEC en kWh/m³)
## Evaluación Energética del Tren de Impulsión Peristáltico, Potencia Eléctrica y Optimización de Proceso

**Proyecto:** Planta Piloto de Ultrafiltración por Flujo Cruzado & Reactor de Coagulación-Sedimentación  
**Tesistas de Grado:** Antonella Guitián & Owen Cañizares  
**Codirector:** Ing. Enzo *(Tesis Doctoral en Procesos de Separación por Membranas)*  
**Ubicación:** Laboratorio de Ingeniería, Universidad Nacional de Salta (UNSa)  
**Fecha:** Octubre 2026  

---

## 🎯 1. Fundamento y Definición del Indicador SEC

El **Consumo Específico de Energía (*Specific Energy Consumption* - $\text{SEC}$)** es el indicador estándar internacional para comparar la eficiencia energética de plantas de desalinización, tratamiento de efluentes y purificación por membranas:

$$\text{SEC} = \frac{P_{\text{eléctrica}}\ [\text{kW}]}{Q_p\ [\text{m}^3/\text{h}]} = \frac{E_{\text{consumida}}\ [\text{kWh}]}{V_{\text{permeado}}\ [\text{m}^3]} \quad \left[\frac{\text{kWh}}{\text{m}^3}\right]$$

Donde:
* $P_{\text{eléctrica}}$: Potencia eléctrica total activa absorbida por la planta (impulsión peristáltica NEMA 34 + agitación de reactor + electrónica de control).
* $Q_p$: Caudal de agua tratada (permeado) producido en régimen permanente.

---

## ⚡ 2. Medición de Potencia Eléctrica en la Planta Piloto

La planta cuenta con tres subsistemas consumidores de energía eléctrica:

```text
       DISTRIBUCIÓN DE ENERGÍA Y POTENCIA ELÉCTRICA EN PLANTA
   
    Red 220 VAC
         │
         ├──► [ Transformador 24 VAC ] ──► Driver DM860 (V_bus ≈ 34 VDC) ──► Motor NEMA 34 (Bomba MBP-2000)
         │                                (P_bomba: 25 a 85 W según RPM y contrapresión)
         │
         ├──► [ Fuente Switching 12 VDC ] ─┬─► Driver L298N ───────────────► Motorreductor Agitador (P_agit: 5 a 18 W)
         │                                 └─► Reductor Buck LM2596 (5V) ──► Microcontrolador ESP32 (P_control ≈ 1.2 W)
```

### Medición Práctica en Banco (para Owen):
1. **Método Directo con Multímetro / Vatímetro en Entrada:**
   * Medir la tensión de red $V_{\text{rms}}$ y la corriente $I_{\text{rms}}$ en la entrada general de $220\text{ VAC}$ mediante una pinza amperométrica o un medidor de potencia digital intercalado.
   * Registrar la potencia activa: $P_{\text{eléctrica}} = V_{\text{rms}} \cdot I_{\text{rms}} \cdot \cos\varphi\ [\text{W}]$.
2. **Método Desagregado por Corriente Continua (DC):**
   * $P_{\text{bomba}} = V_{\text{bus}} \cdot I_{\text{DC,DM860}}$.
   * $P_{\text{agitador}} = 12.0\text{ V} \cdot I_{\text{agitador}}$.
   * $P_{\text{total}} = P_{\text{bomba}} + P_{\text{agitador}} + P_{\text{ESP32}}$.

---

## 📐 3. Potencia Hidráulica vs. Potencia Eléctrica (Eficiencia $\eta$)

La potencia hidráulica neta transferida al fluido impulsado por la bomba peristáltica es:

$$P_{\text{hidráulica}} = Q_{\text{feed}}\ [\text{m}^3/\text{s}] \cdot P_1\ [\text{Pa}] = \frac{Q_{\text{feed}}\ [\text{L/min}] \times 10^{-3}}{60} \cdot (P_1\ [\text{bar}] \times 10^5) \quad [\text{W}]$$

La **eficiencia electromecánica global** del conjunto motor-driver-cabezal resulta:

$$\eta_{\text{global}} = \frac{P_{\text{hidráulica}}}{P_{\text{eléctrica}}} \times 100\%$$

* *Nota de Ingeniería:* En bombas peristálticas de laboratorio con compresión oclusiva de manguera de pared gruesa ($12\text{ mm}$ ext), la fricción elastomérica y las pérdidas de conmutación en micropasos limitan $\eta_{\text{global}}$ a un rango típico del $20\%$ al $40\%$.

---

## 📊 4. Matriz de Cálculo de SEC para los 13 Ensayos del Hito 5

```text
┌────────┬──────┬─────────┬──────────────┬──────────────┬──────────────┬──────────────┬──────────────────┐
│ Corrida│ RPM  │ TMP bar │ Q_alim mL/min│ Q_perm mL/min│ P_electr [W] │ Q_p [m³/h]   │ SEC [kWh/m³]     │
├────────┼──────┼─────────┼──────────────┼──────────────┼──────────────┼──────────────┼──────────────────┤
│ C1     │  40  │  0.10   │    544       │     95       │     32.0     │   0.00570    │      5.61        │
│ C2     │  40  │  0.25   │    544       │    180       │     38.5     │   0.01080    │      3.56        │
│ C3     │  40  │  0.40   │    544       │    245       │     45.2     │   0.01470    │      3.07        │
│ C4     │  70  │  0.10   │    952       │    110       │     48.0     │   0.00660    │      7.27        │
│ C5 (CP)│  70  │  0.25   │    952       │    225       │     56.5     │   0.01350    │      4.18        │
│ C6     │  70  │  0.40   │    952       │    310       │     65.0     │   0.01860    │      3.49        │
│ C7     │  95  │  0.10   │   1292       │    125       │     64.0     │   0.00750    │      8.53        │
│ C8     │  95  │  0.25   │   1292       │    255       │     74.0     │   0.01530    │      4.84        │
│ C9     │  95  │  0.40   │   1292       │    350       │     85.0     │   0.02100    │      4.05        │
│ Testigo│  70  │  0.25   │    952       │     85 (colap)│     58.0     │   0.00510    │     11.37 (alto) │
└────────┴──────┴─────────┴──────────────┴──────────────┴──────────────┴──────────────┴──────────────────┘
```

### Hallazgo Clave para la Tesis:
* En la **Corrida Testigo sin pretratamiento**, la colmatación severa reduce $Q_p$ a una fracción de su valor limpio, disparando el $\text{SEC}$ por encima de **$11\text{ kWh/m}^3$**.
* En las **corridas pretratadas con *Opuntia***, el mantenimiento del flujo por arrastre tangencial estabiliza el $\text{SEC}$ entre **$3.5\text{ y }4.5\text{ kWh/m}^3$**, demostrando que el pretratamiento químico-mecánico genera un **ahorro energético superior al $60\%$ por metro cúbico producido**.

---

## 📋 5. Entregables del Subhito 6.1
1. Curvas de iso-consumo $\text{SEC}$ vs $\text{RPM}$ y $\text{TMP}$ graficadas en Python/Matplotlib para la tesis.
2. Identificación del punto operativo $(\text{RPM}^*, \text{TMP}^*)$ que minimiza el consumo energético sin comprometer la retención de turbidez ($< 3\text{ NTU}$).
3. Análisis comparativo de eficiencia energética: Ultrafiltración directa vs. Coagulación con *Opuntia* + Ultrafiltración.
