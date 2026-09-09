# 🌪️ HITO 4: Reactor de Coagulación / Floculación & Sedimentador Cónico

Este hito aborda el tratamiento fisicoquímico previo a la membrana de ultrafiltración. El objetivo es desestabilizar coloides y partículas suspendidas mediante coagulantes naturales (*Opuntia ficus-indica* o *Moringa oleifera*), promover la formación de flóculos sedimentables y clarificar el agua antes de enviarla al módulo FX100.

---

## 🔬 1. Fundamentos de Ingeniería Química e Hidráulica

### A. Ecuación del Gradiente de Velocidad de Camp-Stein ($G$)
Para lograr una floculación óptima sin romper los flóculos por cizallamiento (*shear stress*), el agitador debe operar a dos velocidades distintas:

$$G = \sqrt{\frac{P}{\mu \cdot V}} = \sqrt{\frac{N_p \cdot \rho \cdot N^3 \cdot D^5}{\mu \cdot V}} \quad \left[\text{s}^{-1}\right]$$

* $P$: Potencia disipada al fluido ($\text{W}$).
* $\mu$: Viscosidad dinámica del agua ($\text{Pa}\cdot\text{s}$).
* $V$: Volumen del reactor ($L$).
* $N$: Velocidad de rotación ($\text{rev/s}$).
* $D$: Diámetro de la paleta ($\text{m}$).
* $N_p$: Número de potencia del impulsor.

```
┌──────────────────────────────┬──────────────────┬───────────────────┬────────────────────────────────────────────┐
│ Etapa del Proceso            │ Velocidad Agitador│ Gradiente G (s⁻¹) │ Objetivo de Proceso                        │
├──────────────────────────────┼──────────────────┼───────────────────┼────────────────────────────────────────────┤
│ 1. Mezcla Rápida (Coagulación)│ 150 RPM (PWM 220)│ ~ 350 - 450 s⁻¹   │ Dispersión homogénea del biopolímero (1 min)│
│ 2. Mezcla Lenta (Floculación) │ 30 RPM  (PWM 80) │ ~ 25 - 40 s⁻¹     │ Colisión y crecimiento de flóculos (15 min) │
│ 3. Sedimentación Estática    │ 0 RPM   (PWM 0)  │ 0 s⁻¹             │ Decantación gravitacional de lodos (30 min)│
└──────────────────────────────┴──────────────────┴───────────────────┴────────────────────────────────────────────┘
```

---

## ⚡ 2. Conexionado del Driver Puente H L298N y Boya de Nivel

```
       ESP32 DevKit V1 (38 Pines)                     DRIVER PUENTE H L298N
     ┌────────────────────────────┐                 ┌────────────────────────┐
     │           GPIO 4 (D4)  ────┼────────────────►│ ENA (PWM Velocidad)    │ ◄── Quitar jumper negro
     │           GPIO 16 (D16) ───┼────────────────►│ IN1 (Sentido Giro A)   │
     │           GPIO 17 (D17) ───┼────────────────►│ IN2 (Sentido Giro B)   │
     │           GPIO 32 (D32) ───┼─┐               │ GND (Masa Común)       │
     │                     GND ───┼─┼──────────────►│ +12V (Alimentación) ◄──┼──── (+12V Fuente)
     └────────────────────────────┘ │               │                        │
                                    │               │ OUT1 ──────────────────┼──► Cable Motor Paleta A
                                    │               │ OUT2 ──────────────────┼──► Cable Motor Paleta B
                                    │               └────────────────────────┘
                                    │
                                    ▼
                     [ BOYA DE NIVEL EN ACERO INOXIDABLE ]
                     • Cable 1 ──► GPIO 32 (con INPUT_PULLUP interno)
                     • Cable 2 ──► GND
                     • Función: Si el flotante baja (tanque vacío), frena el motor.
```

---

## 🧪 3. Protocolo de Ensayo de Remoción de Turbidez (Jar Test)

1. Llenar el reactor con $5\text{ Litros}$ de agua turbia sintética (agua con arcilla/tierra tamizada, $\approx 500\text{ NTU}$).
2. Tomar una muestra testigo y medir turbidez/TDS inicial.
3. Iniciar la secuencia automática en el firmware:
   * Dosificar la solución coagulante de *Opuntia* ($20\text{ mg/L}$).
   * Iniciar **Mezcla Rápida (150 RPM)** durante $60\text{ s}$.
   * Pasar a **Mezcla Lenta (30 RPM)** durante $15\text{ min}$.
   * Detener el motor (**0 RPM**) y dejar decantar $30\text{ min}$.
4. Tomar muestra del sobrenadante clarificado:
   $$\text{Eficiencia de Remoción } \eta (\%) = \left(\frac{\text{Turbidez Inicial} - \text{Turbidez Sobrenadante}}{\text{Turbidez Inicial}}\right) \times 100$$
