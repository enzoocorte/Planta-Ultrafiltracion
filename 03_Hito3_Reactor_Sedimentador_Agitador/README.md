# 🌪️ HITO 3: Reactor de Coagulación / Floculación & Sedimentador Cónico
## Pretratamiento Fisicoquímico, Gradiente de Camp-Stein, Driver BTS7960 (43A) y Boya de Seguridad
**Tesistas**: Antonella Guitián & Owen Cañizares  
**Codirector**: Ing. Enzo (Investigación Doctoral en Membranas)

Este hito aborda el tratamiento fisicoquímico previo al módulo de ultrafiltración. El objetivo es desestabilizar coloides y partículas suspendidas mediante coagulantes naturales (*Opuntia ficus-indica* o Moringa), promover la formación de flóculos sedimentables y clarificar el agua antes de enviarla a la membrana FX100, evitando su colmatación prematura.

---

## 🎯 Entregable Maestro del Hito 3
* **Reactor Floculador-Sedimentador Automatizado en Operación**: Módulo mecatrónico compuesto por motorreductor de alto torque (Sinfín-Corona 5840-31ZY o limpiaparabrisas) comandado por modulación PWM ultrasónica ($18\text{ kHz}$) vía driver MOSFET **BTS7960 (43A)**, montaje mecánico desacoplado con **chumacera bridada KFL08** y acople elástico sobre placa de Grilón, supervisión continua de nivel con boya de acero inoxidable contra marcha en seco (`GPIO 32`), manguera umbilical con ficha rápida **GX16-4** y ejecución estandarizada del ciclo de Jar Test (Mezcla Rápida $100\text{ RPM}$, Mezcla Lenta $25 - 30\text{ RPM}$, Decantación $0\text{ RPM}$) alcanzando una remoción de turbidez $\ge 85\%$ en el sobrenadante.

---

## 📋 Lista de Verificación Maestra (Checklist del Hito 3)
- [ ] Motorreductor de alto torque ($15 - 25\text{ kg}\cdot\text{cm}$ nominal, sinfín-corona 5840-31ZY o limpiaparabrisas 12V con chasis aislado) disponible en banco.
- [ ] Chumacera bridada autoalineante KFL08 (8 mm) y acoplamiento elástico tipo Spider montados sobre placa sándwich de Grilón/Nylon en la tapa.
- [ ] Montaje excéntrico del eje ($1/6$ a $1/4$ del radio) para anulación de vórtice sin soldar deflectores.
- [ ] Driver BTS7960 (43A) ubicado dentro del maletín de control principal comandado por PWM nativo a 3.3V desde el ESP32.
- [ ] Manguera umbilical tipo taller 4x1.0 mm² con conectores industriales circulares tipo aviación GX16 de 4 pines.
- [ ] Boya de acero inoxidable instalada en el reactor con resistencia pull-up física de $2.2\text{ k}\Omega$ y capacitor de desacoplo de $100\text{ nF}$ en `GPIO 32`.
- [ ] Enclavamiento verificado: motor se apaga instantáneamente si el nivel desciende por debajo de la cota de seguridad.
- [ ] Firmware de agitación con rampa *soft-start* (2.5 s) y PWM ultrasónico ($18\text{ kHz}$) cargado en el ESP32.
- [ ] Solución madre de *Opuntia ficus-indica* al $1\%\text{ p/v}$ preparada según protocolo estandarizado.
- [ ] Ensayo de Jar Test ejecutado con agua sintética de bentonita ($\approx 500\text{ NTU}$) en volumen real de $25\text{ L}$.
- [ ] Medición de turbidez del sobrenadante clarificado tomado por encima del cono ($\ge 85\%$ remoción) previa al ingreso a la membrana FX100.

---

## 🔬 1. Fundamentos de Ingeniería Química e Hidráulica

### A. Ecuación del Gradiente de Velocidad de Camp-Stein ($G$)
Para lograr una floculación óptima sin romper los flóculos por cizallamiento (*shear stress*), el agitador debe operar a dos velocidades distintas:

$$G = \sqrt{\frac{P}{\mu \cdot V}} = \sqrt{\frac{N_p \cdot \rho \cdot N^3 \cdot D^5}{\mu \cdot V}} \quad \left[\text{s}^{-1}\right]$$

* $P$: Potencia disipada al fluido ($\text{W}$).
* $\mu$: Viscosidad dinámica del agua ($\text{Pa}\cdot\text{s}$).
* $V$: Volumen del reactor ($25\text{ L} = 0.025\text{ m}^3$).
* $N$: Velocidad de rotación ($\text{rev/s}$).
* $D$: Diámetro de la paleta ($0.11 - 0.14\text{ m}$).
* $N_p$: Número de potencia del impulsor.

```
┌──────────────────────────────┬──────────────────┬───────────────────┬──────────────┬────────────────────────────────────────────┐
│ Etapa del Proceso            │ Velocidad Agitador│ Gradiente G (s⁻¹) │ Tiempo (t)   │ Objetivo de Proceso                        │
├──────────────────────────────┼──────────────────┼───────────────────┼──────────────┼────────────────────────────────────────────┤
│ 1. Mezcla Rápida (Coagulación)│ 100 RPM (PWM alto)│ ~ 350 - 450 s⁻¹   │ 60 s (1 min) │ Dispersión homogénea del biopolímero       │
│ 2. Mezcla Lenta (Floculación) │ 25 - 30 RPM (PWM) │ ~ 25 - 40 s⁻¹     │ 15 - 20 min  │ Crecimiento de flóculos (Vtip < 0.4 m/s)   │
│ 3. Sedimentación Estática    │ 0 RPM (PWM 0)     │ 0 s⁻¹             │ 30 - 45 min  │ Decantación hacia fondo cónico de 60°      │
└──────────────────────────────┴──────────────────┴───────────────────┴──────────────┴────────────────────────────────────────────┘
```

---

## ⚡ 2. Conexionado del Driver BTS7960 (43A), Ficha GX16-4 y Boya de Nivel

```
       ESP32 DevKit V1 (38 Pines)                     DRIVER MOSFET BTS7960 (43A)
     ┌────────────────────────────┐                 ┌───────────────────────────┐
     │           GPIO 4 (D4)  ────┼────────────────►│ RPWM (PWM Velocidad 18kHz)│
     │           GPIO 16 (D16) ───┼────────────────►│ R_EN y L_EN (Habilitación)│
     │                    GND ───┼─┐               │ LPWM ──► GND (Sentido fijo)│
     │                    3.3V ───┼─┼──────────────►│ VCC (Lógica 3.3V - 5V)    │
     │                            │ │               │ GND (Masa Común)          │
     │                            │ │               │ B+ ◄── (+12V Fuente 10A)  │
     │                            │ │               │ B- ◄── (GND Fuente 10A)   │
     │                            │ │               │ M+ ──────┐  M- ───────┐   │
     │                            │ │               └──────────┼────────────┼───┘
     │                            │ │                          │            │
     │                            │ │      FICHA PANEL GX16-4  │            │
     │                            │ │    ┌─────────────────────┴────────────┴───┐
     │                            │ │    │ Pin 1: Motor PWM (+) ────────────────┼──► Cable Manguera 1.5mm²
     │                            │ │    │ Pin 2: Motor PWM (-) ────────────────┼──► Cable Manguera 1.5mm²
     │   GPIO 32 (con Pull-up 2k2)│ │    │ Pin 3: Boya Nivel Señal ─────────────┼──► Cable Manguera Señal
     │    + Filtro 100nF a GND ───┼─┼───►│ Pin 4: Retorno Señal GND ────────────┼──► Cable Manguera Señal
     │                            │ │    └──────────────────────────────────────┘
     └────────────────────────────┘ │
                                    ▼
                     [ REACTOR: BOYA EN ACERO INOXIDABLE Y MOTOR ]
                     • Pin 1 y 2 ──► Bornes Motor 12V (Sinfín 5840 o Limpiaparabrisas)
                     • Pin 3 y 4 ──► Contactos Boya Inox (Corte de bomba y motor por nivel bajo)
```

---

## 🧪 3. Protocolo Resumido de Ensayo de Jar Test (25 Litros)

1. Llenar el reactor con **$25\text{ Litros}$** de agua turbia sintética ($\approx 400 - 500\text{ NTU}$).
2. Tomar muestra testigo inicial ($T_0$).
3. Iniciar la secuencia automática en el firmware:
   * Dosificar coagulante de *Opuntia* ($20\text{ mg/L} \implies 50\text{ mL}$ de solución madre al 1%).
   * Mezcla Rápida (**$100\text{ RPM}$**) durante $60\text{ s}$.
   * Mezcla Lenta (**$30\text{ RPM}$**) durante $15\text{ min}$.
   * Sedimentación ($0\text{ RPM}$) durante $30\text{ min}$.
4. Tomar muestra del sobrenadante clarificado ($T_f$) y calcular la eficiencia:
   $$\eta (\%) = \left(\frac{T_0 - T_f}{T_0}\right) \times 100$$

---

## 📂 Estructura y Navegación de Subcarpetas de este Hito:

* 📁 **[`01_Protocolos_y_Teoria_Floculacion/`](./01_Protocolos_y_Teoria_Floculacion/)**:
  * 🧪 **[`Protocolo_Ensayo_Coagulacion_Floculacion_JarTest.md`](./01_Protocolos_y_Teoria_Floculacion/Protocolo_Ensayo_Coagulacion_Floculacion_JarTest.md)**: Protocolo químico detallado de extracción de mucílago de *Opuntia*, dimensionamiento del gradiente $G$ para $25\text{ L}$, cinética de floculación, entregable y lista de verificación.
* 📁 **[`02_Firmware_Control_Agitador/`](./02_Firmware_Control_Agitador/)**:
  * ⚙️ **[`Guia_Dimensionamiento_Motorreductor_y_Torque_Agitador.md`](./02_Firmware_Control_Agitador/Guia_Dimensionamiento_Motorreductor_y_Torque_Agitador.md)**: **Documento Maestro de Ingeniería Mecánica y Dictamen Argon**. Dimensionamiento hidrodinámico para $25\text{ L}$, torque de despegue, selección de motor (5840-31ZY / Limpiaparabrisas), montaje desacoplado con chumacera KFL08, driver BTS7960 (43A), manguera GX16-4 y reglas anti-vórtice.
  * ⚙️ **[`Guia_Control_Agitador_L298N_y_Boya.md`](./02_Firmware_Control_Agitador/Guia_Control_Agitador_L298N_y_Boya.md)**: Historial técnico de conexionado y transición desde L298N hacia el driver MOSFET BTS7960 con rampa *soft-start* y boya de seguridad.
  * 💻 **[`firmware_sedimentador/firmware_sedimentador.ino`](./02_Firmware_Control_Agitador/firmware_sedimentador/firmware_sedimentador.ino)**: Sketch de Arduino para ESP32 con Máquina de Estados Finitos (FSM) no bloqueante.
* 📁 **[`03_Compras_y_Ferreteria_Hidraulica/`](./03_Compras_y_Ferreteria_Hidraulica/)**:
  * 🚰 **[`Guia_Compras_Hidraulicas_y_Locales_Salta.md`](./03_Compras_y_Ferreteria_Hidraulica/Guia_Compras_Hidraulicas_y_Locales_Salta.md)**: Guía de adaptación de 1/2" a 1/4", chumacera KFL08, acople elástico spider, ficha GX16-4 y locales en Salta Capital.
* 📁 **[`04_Galeria_y_Montaje_Mecanico/`](./04_Galeria_y_Montaje_Mecanico/)**:
  * 🌐 **[`guia_conexion_sedimentador.html`](./04_Galeria_y_Montaje_Mecanico/guia_conexion_sedimentador.html)**: Guía visual interactiva con fotos reales, acople de tornillo sinfín, pines ESP32 y checklist de armado.
  * 📸 Fotografías técnicas de motor 12V, paleta, base y cuerpo del sedimentador.

---

## 📊 Estado de Avance Actual (Avance Global: 25% — Especificación Cerrada / Pendiente Adquisición y Taller)

> [!NOTE]
> **Resolución de Ingeniería del Hito 3 (Auditoría Cruzada Antigravity & Argon):**
> Se cerró la especificación técnica definitiva para superar el cuello de botella:
> 1. **Motor:** Motorreductor Sinfín-Corona 5840-31ZY (12V, 100 RPM, $25\text{ kg}\cdot\text{cm}$) o Limpiaparabrisas automotriz 12V (con masa aislada).
> 2. **Mecánica:** Chumacera KFL08 autoalineante en la tapa + Acople elástico spider 8mm + Placa sándwich de Grilón (120x120 mm) + Eje excéntrico anti-vórtice.
> 3. **Electrónica:** Driver BTS7960 (43A) en el maletín con PWM a 18 kHz y rampa suave de 2.5 s.
> 4. **Conectividad:** 1 solo ESP32 central en maletín + Manguera umbilical 4 hilos con conector de aviación GX16-4.

| Subhito / Actividad | Estado | Porcentaje | Observaciones |
| :--- | :---: | :---: | :--- |
| **Cuba Cónica del Sedimentador** | ✅ Disponible | 80% | Recipiente cónico en acero inoxidable disponible en taller. |
| **Protocolo Químico Jar Test (*Opuntia*)** | 🟡 Formulado | 60% | Dosificación para 25L calculada; pendiente recolección de cladodios. |
| **Especificación Mecánica y Actuador** | 🟢 Definida | 50% | Dictamen cerrado (5840-31ZY / Limpiaparabrisas + KFL08 + Grilón). |
| **Electrónica de Potencia (BTS7960)** | 🟡 En Transición | 30% | Descarte de L298N; adopción de BTS7960 comandado por ESP32. |
| **Boya de Nivel Inox (Seguridad en Seco)** | 🟡 Disponible | 30% | Boya disponible; circuito con pull-up 2k2 y capacitor 100nF. |
| **Mecanizado y Montaje en Tapa** | 🔴 Pendiente Taller| 10% | Agujero excéntrico, fijación de chumacera y torreta pendientes. |

---

## 🏁 Criterios de Aceptación y Entregables para Pasar al Hito 4

1. [ ] **Nuevo Motorreductor de Agitación Integrado:** Montaje del 5840-31ZY o limpiaparabrisas sobre chumacera KFL08 y acople elástico.
2. [ ] **Demostración de ciclo automático Jar Test:** Mezcla Rápida (100 RPM / 60 s) ➔ Mezcla Lenta (25-30 RPM / 15-20 min) ➔ Sedimentación (0 RPM / 30-45 min) controlado por FSM.
3. [ ] **Conexión Umbilical Segura:** Manguera envainada 4 hilos con conector industrial GX16-4 y enclavamiento instantáneo por boya.
4. [ ] **Clarificación demostrada:** Sobrenadante clarificado con remoción de turbidez $\ge 85\%$ tomado por encima del cono.
5. [ ] **Habilitación formal:** Interconexión de la succión del sedimentador hacia la bomba peristáltica del **Hito 4**.

