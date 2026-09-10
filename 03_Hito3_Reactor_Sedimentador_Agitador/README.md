# 🌪️ HITO 3: Reactor de Coagulación / Floculación & Sedimentador Cónico
## Pretratamiento Fisicoquímico, Gradiente de Camp-Stein y Control por L298N con Boya de Seguridad
**Tesistas**: Antonella Guitián & Owen Cañizares  
**Codirector**: Ing. Enzo (Investigación Doctoral en Membranas)

Este hito aborda el tratamiento fisicoquímico previo al módulo de ultrafiltración. El objetivo es desestabilizar coloides y partículas suspendidas mediante coagulantes naturales (*Opuntia ficus-indica* o Moringa), promover la formación de flóculos sedimentables y clarificar el agua antes de enviarla a la membrana FX100, evitando su colmatación prematura.

---

## 🎯 Entregable Maestro del Hito 3
* **Reactor Floculador-Sedimentador Automatizado en Operación**: Módulo mecatrónico compuesto por el motor agitador comandado por modulación PWM vía driver Puente H L298N, supervisión continua de nivel con boya de acero inoxidable contra marcha en seco (`GPIO 32`), y ejecución estandarizada del ciclo de Jar Test (Mezcla Rápida $150\text{ RPM}$, Mezcla Lenta $30\text{ RPM}$, Decantación $0\text{ RPM}$) alcanzando una remoción de turbidez $\ge 85\%$ en el sobrenadante.

---

## 📋 Lista de Verificación Maestra (Checklist del Hito 3)
- [ ] Conexión del driver Puente H L298N completada: `GPIO 4` (ENA sin jumper), `GPIO 16` (IN1), `GPIO 17` (IN2).
- [ ] Alimentación de $12\text{V DC}$ conectada al L298N y masa compartida (GND común con ESP32).
- [ ] Boya de acero inoxidable instalada en el reactor y conectada entre `GPIO 32` y `GND`.
- [ ] Enclavamiento verificado: motor se apaga en seco si el nivel cae por debajo de la cota mínima.
- [ ] Firmware `firmware_sedimentador.ino` cargado en el ESP32 respondiendo a comandos serie a 115200 baudios.
- [ ] Solución madre de *Opuntia ficus-indica* al $1\%\text{ p/v}$ preparada según protocolo estandarizado.
- [ ] Ensayo de Jar Test ejecutado con agua sintética de bentonita ($\approx 500\text{ NTU}$).
- [ ] Medición de turbidez del sobrenadante clarificado y confirmación de remoción previa al ingreso a la membrana FX100.

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
┌──────────────────────────────┬──────────────────┬───────────────────┬──────────────┬────────────────────────────────────────────┐
│ Etapa del Proceso            │ Velocidad Agitador│ Gradiente G (s⁻¹) │ Tiempo (t)   │ Objetivo de Proceso                        │
├──────────────────────────────┼──────────────────┼───────────────────┼──────────────┼────────────────────────────────────────────┤
│ 1. Mezcla Rápida (Coagulación)│ 150 RPM (PWM 220)│ ~ 350 - 450 s⁻¹   │ 60 s (1 min) │ Dispersión homogénea del biopolímero       │
│ 2. Mezcla Lenta (Floculación) │ 30 RPM  (PWM 80) │ ~ 25 - 40 s⁻¹     │ 15 min       │ Colisión y crecimiento de flóculos         │
│ 3. Sedimentación Estática    │ 0 RPM   (PWM 0)  │ 0 s⁻¹             │ 30 min       │ Decantación gravitacional hacia fondo cónico│
└──────────────────────────────┴──────────────────┴───────────────────┴──────────────┴────────────────────────────────────────────┘
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
                     • Cable 2 ──► GND Común
                     • Función: Si el flotante baja (tanque vacío), frena el motor.
```

---

## 🧪 3. Protocolo Resumido de Ensayo de Jar Test

1. Llenar el reactor con $5\text{ Litros}$ de agua turbia sintética ($\approx 500\text{ NTU}$).
2. Tomar muestra testigo inicial ($T_0$).
3. Iniciar la secuencia automática en el firmware:
   * Dosificar coagulante de *Opuntia* ($20\text{ mg/L}$).
   * Mezcla Rápida ($150\text{ RPM}$) durante $60\text{ s}$.
   * Mezcla Lenta ($30\text{ RPM}$) durante $15\text{ min}$.
   * Sedimentación ($0\text{ RPM}$) durante $30\text{ min}$.
4. Tomar muestra del sobrenadante clarificado ($T_f$) y calcular la eficiencia:
   $$\eta (\%) = \left(\frac{T_0 - T_f}{T_0}\right) \times 100$$

---

## 📂 Estructura y Navegación de Subcarpetas de este Hito:

* 📁 **[`01_Protocolos_y_Teoria_Floculacion/`](./01_Protocolos_y_Teoria_Floculacion/)**:
  * 🧪 **[`Protocolo_Ensayo_Coagulacion_Floculacion_JarTest.md`](./01_Protocolos_y_Teoria_Floculacion/Protocolo_Ensayo_Coagulacion_Floculacion_JarTest.md)**: Protocolo químico detallado de extracción de mucílago de *Opuntia*, dimensionamiento del gradiente $G$, cinética de floculación, entregable y lista de verificación.
* 📁 **[`02_Firmware_Control_Agitador/`](./02_Firmware_Control_Agitador/)**:
  * ⚙️ **[`Guia_Control_Agitador_L298N_y_Boya.md`](./02_Firmware_Control_Agitador/Guia_Control_Agitador_L298N_y_Boya.md)**: Manual de conexiones del Puente H, modulación PWM, seguridad de nivel, entregable y lista de verificación.
  * 💻 **[`firmware_sedimentador/firmware_sedimentador.ino`](./02_Firmware_Control_Agitador/firmware_sedimentador/firmware_sedimentador.ino)**: Sketch de Arduino para ESP32 con Máquina de Estados Finitos (FSM) no bloqueante.
