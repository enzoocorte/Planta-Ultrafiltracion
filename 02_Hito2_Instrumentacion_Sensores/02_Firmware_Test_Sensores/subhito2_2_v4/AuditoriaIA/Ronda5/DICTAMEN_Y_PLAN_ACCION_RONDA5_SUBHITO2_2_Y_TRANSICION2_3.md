# DICTAMEN TÉCNICO Y PLAN DE ACCIÓN INTEGRAL — AUDITORÍA RONDA 5
## Planta Piloto de Ultrafiltración FX100 — Tesis de Grado en Ingeniería Industrial (UNSa)
**Autores del Proyecto:** Antonella Guitián & Owen Cañizares  
**Codirección / Soporte:** Ing. Enzo  
**Fecha de Auditoría:** Octubre 2026  
**Auditor Externo:** use.ai (Veredictos: *Cierre Subhito 2.2 Rechazado* & *Dictamen Técnico Aprobado con Observaciones Bloqueantes*)

---

## 1. RESUMEN EJECUTIVO: VEREDICTO DE LA AUDITORÍA EXTERNA

La auditoría externa de **Ronda 5** ha emitido un dictamen de **RECHAZO FORMAL para certificar el cierre definitivo del Subhito 2.2**, concediendo una **HABILITACIÓN CONDICIONAL para avanzar en paralelo con el montaje de los transductores de presión (Subhito 2.3)** en banco controlado.

Este dictamen **no desacredita el trabajo realizado**, sino que aporta un rigor metrológico e ingenieril fundamental para blindar la defensa de tesis frente al tribunal evaluador. Identifica con precisión matemática por qué los filtros iniciales de software no resolvían el problema físico de fondo, y establece las condiciones exactas para levantar el rechazo.

| Área Auditada | Veredicto Ronda 5 | Causa Principal | Acción Requerida |
| :--- | :--- | :--- | :--- |
| **Blindaje EMI Caudalímetros** | **BLOQUEANTE** | El ruido en permeado oscilaba entre **15 y 65 Hz**. Un deadband de 2.0 Hz y el blanking de 1500 µs no lo atenúan; provocan *aliasing*. | Separación de cables, ferritas, pull-up de 1.5–2.2 kΩ, filtro de coherencia de período y **Prueba de Permeado Seco**. |
| **Metrología y Factores K** | **OBSERVADO** | La calibración previa de alimentación tenía ordenada al origen de $+20.98\text{ Hz}$ (contaminada por EMI). El K de permeado ($687\text{ vs }196$) discrepa 3.5× en idéntico sensor. | Repetir calibración de alimentación con hardware apantallado. Para permeado (< 100 mL/min), incorporar **patrón gravimétrico (balanza)**. |
| **Datalogger y Memoria** | **CORREGIDO** | El buffer lineal anterior desplazaba 38 KB en cada muestra al llenarse. El slider web podía saturar los 20 ensayos. | Implementado **Buffer Circular Indexado $O(1)$**, guarda de segmentación estable y columna `Estable_1_0` en CSV. |
| **Dinámica del Motor NEMA 34** | **ANALIZADO** | Ruido a 30–40 RPM responde a resonancia estructural por pasos completos (100 Hz) y choque de 3 rodillos sobre silicona. Calentamiento por $I^2 R$. | Subir driver DM860 a **6400 micropasos**, reducir corriente a 2.4–2.7 A, activar SW4 (Half Current) y amortiguar soporte con silentblocks. |
| **Arquitectura Subhito 2.3** | **APROBADO COND.** | ADS1115 a 3.3V + divisor $10\text{ k}\Omega/20\text{ k}\Omega$ en transductores piezorresistivos de 5V. | Monitoreo ratiométrico en canal A3, interbloqueo con **corte físico por PIN_ENA** si $\text{TMP} > 0.50\text{ bar}$ y válvula de alivio mecánica (0.6 bar). |

---

## 2. DESGLOSE EXHAUSTIVO DE LOS HALLAZGOS Y FENÓMENOS FÍSICOS

### 2.1. El Caudal Fantasma y el Mecanismo de Aliasing
- **El problema:** En los ensayos de calibración con línea de permeado seca (sin circulación de agua), el sensor registraba frecuencias de **15 a 20 Hz a 60 RPM** y de **45 a 65 Hz a 90 RPM**, totalizando más de 10 litros falsos en 28 minutos.
- **Por qué el filtro anterior de software ($f < 2.0\text{ Hz}$) falló:** Al estar el ruido entre 15 y 65 Hz, superaba con creces el umbral de 2 Hz, pasando libremente al totalizador de volumen.
- **El error del Blanking de 1500 µs:** El motor NEMA 34 con 3200 pasos/rev genera frecuencias de conmutación de 1067 a 4800 Hz entre 20 y 90 RPM. Un blanking de 1500 µs tiene un corte de $f_{\text{max}} \approx 666.7\text{ Hz}$. Lejos de suprimir el ruido, el blanking actúa como un **filtro de submuestreo (aliasing)**: deja pasar 1 pulso espurio de cada ráfaga de pulsos del chopper, produciendo una frecuencia alias espuria perfectamente proporcional a las RPM del motor.
- **Insignificancia del INPUT_PULLUP interno:** Poner la resistencia interna del ESP32 (~45 kΩ) en paralelo con los 4.7 kΩ externos da $R_{\text{eq}} \approx 4.26\text{ k}\Omega$, lo que representa una reducción de impedancia de apenas el 9.5%. Frente a la alta tasa de variación de corriente ($dI/dt$) de un chopper de 3.0 A, esa variación es despreciable.

### 2.2. Metrología: Limitaciones de la Turbina YF-S401 en Permeado
- **Rango de Operación del Filtro FX100:** En ultrafiltración de agua limpia, para una permeabilidad típica de $L_p \approx 6.9 \cdot 10^{-11}\text{ m/(s}\cdot\text{Pa)}$ y $\text{TMP} = 0.2\text{--}0.4\text{ bar}$, el flujo de permeado esperado ronda apenas los **20 a 100 mL/min** ($0.02\text{ a }0.10\text{ L/min}$).
- **Fricción Estática de la Turbina:** La turbina comercial YF-S401 tiene un umbral de despegue y linealidad a partir de $\sim 200\text{--}300\text{ mL/min}$. Por debajo de ese valor, la turbina resbala o se clava de forma errática.
- **Propuesta de Oro para la Tesis (Aporte Metrológico):** La auditoría recomienda enfáticamente medir el permeado mediante **método gravimétrico** (balanza o celda de carga con módulo HX711 midiendo la masa acumulada en el tanque de permeado en función del tiempo: $Q_{\text{perm}} = \Delta m / \Delta t$). Este método es el estándar indiscutible en plantas de ultrafiltración y dotará a la tesis de trazabilidad metrológica primaria.

### 2.3. Ruido Acústico y Resonancia Mecánica a 30–40 RPM
- **Frecuencia Fundamental de Pasos:** A 30 RPM, la velocidad angular es 0.5 rev/s = **100 pasos completos por segundo (100 Hz)**.
- **Resonancia Electromecánica:** La frecuencia natural de vibración de la estructura de chapa, soporte de motor y cabezal peristáltico cae exactamente en la banda de 80 a 150 Hz. A esto se le suma la pulsación de oclusión de los 3 rodillos sobre la manguera de 12 mm ($3 \times 0.5\text{ rev/s} = 1.5\text{ Hz}$ con armónicos superiores). A velocidades más altas (60–80 RPM), la mayor inercia del rotor y la velocidad de paso superan la zona crítica, estabilizando el régimen.
- **Calentamiento Térmico:** En motores paso a paso operados por drivers de corriente constante, la disipación dominante es por efecto Joule en el cobre ($P = I^2 R$). A 3.0 A continuos, una temperatura de carcasa de 60–75 °C es típica. Si el par lo permite, bajar a 2.4–2.7 A reducirá drásticamente el calor sin comprometer el arrastre.

---

## 3. MEJORAS IMPLEMENTADAS EN FIRMWARE V4 (COMPILACIÓN 100% VERIFICADA)

El firmware ubicado en `02_Firmware_Test_Sensores/subhito2_2_v4/EN_USO_firmware_planta/` fue actualizado y validado mediante compilación real con `arduino-cli`:

### 3.1. Filtro Digital de Coherencia Temporal (`caudalimetro.cpp` y `caudalimetro.h`)
- **Detección de Dispersión de Períodos:** Se agregaron variables atómicas `_dt_min_us` y `_dt_max_us`. En una rotación real impulsada por fluido incompresible, la relación $dt_{\text{max}} / dt_{\text{min}}$ dentro de la ventana de 1 segundo es acotada ($\le 4.0$). Si un tren de pulsos espurios de EMI ocurre concentrado en 5 ms y luego hay 995 ms de silencio, la relación se dispara ($> 200$) y el firmware **descarta la ráfaga como ruido espurio**.
- **Ocupación de Ventana Temporal:** Para $n$ pulsos a baja frecuencia, se valida que el tiempo entre el primer y último pulso ocupe un span temporal creíble ($\ge 30\text{ ms}$).
- **Deadband Físico Estricto:** Frecuencia cortada a $0.0\text{ Hz}$ si $f < 2.5\text{ Hz}$, y corte de caudal a $0.0\text{ mL/min}$ si $q < 30.0\text{ mL/min}$ (límite de cuantificación de la turbina).
- **Diagnóstico de Pulsos Brutos:** Se expone `pulsosBrutos()` en la telemetría periódica para que Enzo y Antonella puedan monitorear en Serial si el sensor seco capta cualquier pulso espurio.

### 3.2. Arquitectura de Datalogger con Buffer Circular $O(1)$ (`EN_USO_firmware_planta.ino`)
- **Eliminación del desplazamiento FIFO de memoria:** Se reemplazó el bucle de copia manual por un **Buffer Circular Indexado** con índices `bufferHead` y `bufferCount`. La inserción de muestras es $O(1)$ estricto, sin mover 38 KB en cada segundo ni fragmentar la memoria RAM.
- **Columna `Estable_1_0` en el CSV:** El CSV exportado ahora incluye explícitamente si la muestra fue tomada en régimen permanente (`Estable_1_0 = 1`) o durante una rampa de aceleración/desaceleración (`Estable_1_0 = 0`). Esto permite a Antonella filtrar en Excel con un solo clic las muestras transitorias.
- **Protección del Slider y Segmentación Segura:** En `handleSetRPM()`, solo se genera un nuevo ensayo si la consigna cambia en $\ge 1.5\text{ RPM}$ **Y** el ensayo actual tuvo al menos 5 segundos de duración. Si el operador mueve el slider continuamente, ya no se saturan los 20 ensayos en segundos.
- **Formato y Seguridad de Exportación:** Corrección de tipos `%02lu:%02lu` en `snprintf`, chequeo de tamaño de buffer antes de enviar streaming HTTP y reinicio coherente de contadores de volumen en `handleClearCSV()`.

### 3.3. Control de Bomba y Parada de Emergencia Física (`Bomba.cpp`, `Bomba.h` y `config.h`)
- **Frenado rápido garantizado en $< 1.5\text{ s}$:** Se incrementó la desaceleración de parada a $FRENADO\_PARADA\_RPM\_S = 70.0\text{ RPM/s}$ (desde 100 RPM se detiene en 1.43 s).
- **Parada de Emergencia Física Instantánea (`PIN_ENA = 23`):** Se implementó `paradaEmergencia()`, que corta los pulsos PWM a cero y polariza `PIN_ENA = HIGH` en el driver DM860. Esto desenergiza instantáneamente las bobinas del motor NEMA 34 en $< 1\text{ ms}$, liberando el eje sin esperar la rampa de desaceleración.
- **Rearme Manual Enclavado:** Método `rearmarEmergencia()` accesible vía comando SCADA (`/cmd?act=REARM`) tras corregir la causa de la sobrepresión.

### 3.4. Ley de Darcy y Validación Termodinámica (`darcy.h`)
- **Rango de Temperatura Seguro:** Acotamiento estricto de la temperatura entre $5.0\text{ }^\circ\text{C}$ y $60.0\text{ }^\circ\text{C}$ para garantizar la validez física de la ecuación de Vogel.
- **Protección contra No-Finitos:** Verificación exhaustiva con `std::isfinite()` en todas las resistencias y flujos.
- **Rigor Metrológico:** Renombrado de $R_{\text{torta}}$ a $R_{\text{adicional}}$ (resistencia hidráulica adicional aparente) para mantener la precisión científica hasta que se realicen ensayos específicos de ensuciamiento.

---

## 4. PROTOCOLO OBLIGATORIO PARA LEVANTAR EL RECHAZO EN BANCO
*(Procedimiento experimental para Enzo y Antonella)*

Para que el Subhito 2.2 pase formalmente de **RECHAZADO** a **APROBADO SIN OBSERVACIONES**, se debe ejecutar y documentar la **"Prueba de Permeado Seco"**:

### Pasos en Banco:
1. **Adecuación de Hardware (Blindaje Físico):**
   - Separar el mazo de cables del motor NEMA 34 del mazo de señales de los caudalímetros (separación mínima de 10 a 15 cm; si se cruzan, hacerlo a 90°).
   - Reemplazar la resistencia pull-up del sensor de permeado por una de **1.5 kΩ o 2.2 kΩ** a 3.3V (en lugar de 4.7 kΩ).
   - Colocar una ferrita toroidal en los cables de salida del driver DM860 hacia el motor.
   - Conectar la malla del cable de motor a la tierra del chasis/fuente, y la malla del cable de señal a GND del ESP32.
2. **Prueba de Línea de Permeado Seca:**
   - Dejar el caudalímetro de permeado completamente seco (sin agua, desenroscado o sin flujo).
   - Encender la bomba peristáltica e iniciar corridas de 5 minutos en cada una de las siguientes velocidades:
     - 20 RPM (5 minutos)
     - 40 RPM (5 minutos)
     - 60 RPM (5 minutos)
     - 80 RPM (5 minutos)
     - 100 RPM (5 minutos)
3. **Criterio de Aceptación Estricto:**
   - La telemetría en Serial y en el SCADA debe indicar exactamente:
     $$\text{Pulsos Permeado} = 0 \quad | \quad Q_{\text{perm}} = 0.0\text{ mL/min} \quad | \quad V_{\text{perm}} = 0.000\text{ L}$$
   - Repetir la prueba con el cartucho lleno de agua y la válvula de permeado cerrada.
4. **Repetición del Ensayo de Calibración de Alimentación (Ensayo 3):**
   - Una vez blindado el sistema y con 0 pulsos fantasmas comprobados, repetir la calibración por probeta de alimentación (3 réplicas por velocidad, midiendo 60 segundos por punto).
   - Verificar que la recta de regresión pase por el origen ($b \approx 0\text{ Hz}$).

---

## 5. HOJA DE RUTA Y ARQUITECTURA PARA EL SUBHITO 2.3 (PRESIÓN Y TMP)

Con el blindaje de firmware v4 listo y verificado, el equipo tiene luz verde para iniciar el montaje del banco de presión:

```
[Transductor P1: 0-30 PSI (5V)] ---> Divisor 10k/20k (x0.667) ---> ADS1115 A0 (0.33 - 3.00 V)
[Transductor P2: 0-30 PSI (5V)] ---> Divisor 10k/20k (x0.667) ---> ADS1115 A1 (0.33 - 3.00 V)
[Transductor Perm: 0-15 PSI (5V)] -> Divisor 10k/20k (x0.667) ---> ADS1115 A2 (0.33 - 3.00 V)
[Monitoreo Riel 5V de Sensores] ---> Divisor 10k/20k (x0.667) ---> ADS1115 A3 (Referencia Ratiométrica)
                                                                           |
                                                                           v I2C (SDA=21, SCL=22) a 3.3V
                                                                        [ESP32]
                                                                           |
                                           Si TMP > 0.50 bar o P1 > 0.60 bar (3 ticks = 150 ms):
                                                                           v
                                                [PIN_ENA (GPIO 23) -> HIGH] + [LEDC PWM -> 0]
                                                (PARADA DE EMERGENCIA FÍSICA INSTANTÁNEA)
```

1. **Alimentación y Divisores de Tensión:**
   - ADS1115 alimentado a 3.3V (VCC = 3.3V, I2C a 3.3V nativo).
   - Transductores alimentados a 5V (salida 0.5 a 4.5V).
   - Divisor $10\text{ k}\Omega / 20\text{ k}\Omega$ ($\beta = 20 / 30 = 0.6667$): convierte $0.5\text{--}4.5\text{ V} \to 0.333\text{--}3.000\text{ V}$, protegiendo el ADC.
2. **Cálculo Ratiométrico:** El canal A3 mide la tensión real de 5V para normalizar $\text{ratio} = V_{\text{out}} / V_{5V}$, eliminando el error por caídas o fluctuaciones de la fuente.
3. **Interbloqueo de Seguridad:**
   - Disparo por sobrepresión si $\text{TMP} > 0.50\text{ bar}$ o $P_1 > 0.60\text{ bar}$ sostenido durante 3 lecturas consecutivas (150 ms).
   - El ESP32 conmuta `PIN_ENA = HIGH` y `ledcWrite = 0`, cortando la potencia del motor en microsegundos.
4. **Protección Mecánica Obligatoria:** Instalar en la tubería una válvula de alivio calibrada a 0.6–0.7 bar como fusible hidráulico redundante frente a cualquier bloqueo involuntario.

---
*Documento elaborado para el repositorio oficial de tesis — Octubre 2026.*
