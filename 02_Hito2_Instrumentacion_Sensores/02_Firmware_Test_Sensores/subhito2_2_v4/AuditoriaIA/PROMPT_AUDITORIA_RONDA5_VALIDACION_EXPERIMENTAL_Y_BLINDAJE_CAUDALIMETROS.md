# 🧠 PROMPT MAESTRO DE AUDITORÍA TÉCNICA EXTERNA — RONDA 5
## VALIDACIÓN EXPERIMENTAL, BLINDAJE DE CAUDALÍMETROS Y TRANSICIÓN A TRANSDUCTORES DE PRESIÓN

> **Destinatario:** IA Auditora Técnica y Científica Senior (GPT-4o / Claude 3.5 Sonnet / GLM-5.3 / DeepSeek R1 / GPT Astra)  
> **Proyecto:** Planta Piloto Modular de Tratamiento de Agua por Ultrafiltración por Flujo Cruzado (UF) & Reactor de Coagulación-Sedimentación  
> **Institución:** Universidad Nacional de Salta (UNSa), Facultad de Ingeniería — Salta, Argentina  
> **Equipo de Trabajo:** Antonella Guitián & Owen Cañizares (Tesistas de Grado de Ing. Industrial), Ing. Enzo (Codirector / Investigador Doctoral), Antigravity AI (Asistente de Arquitectura de Sistemas Embebidos)  
> **Fecha:** Octubre 2026  
> **Estado del Proyecto:** Cierre del Subhito 2.2 (Validación Experimental de Caudalímetros) y Preparación del Subhito 2.3 (Transductores de Presión y TMP)

---

## 🎯 INSTRUCCIONES DE ROL PARA LA IA AUDITORA

Actúa como un **Comité de Expertos de Nivel Principal** compuesto por:
1. **Ingeniero Principal de Sistemas Embebidos Industriales:** Especialista en ESP32, FreeRTOS multinúcleo, acondicionamiento de señales en presencia de interferencia electromagnética (EMI/EMC), drivers paso a paso industriales e instrumentación de sensores.
2. **Especialista Senior en Procesos de Separación por Membranas:** Experto en ultrafiltración capilar de fibra hueca, modelado hidráulico mediante la Ley de Darcy, presión transmembrana (TMP), resistencia de membrana ($R_m$) y fenómenos de ensuciamiento (*fouling*).
3. **Metrólogo Industrial:** Especialista en incertidumbre de medición, calibración con patrones primarios (gravimétricos / volumétricos) y linealidad instrumental.

Este documento contiene el **100% del contexto físico, matemático, experimental y de código fuente** de nuestra planta piloto. Tu objetivo es emitir un **Dictamen Técnico Exhaustivo de Certificación**, evaluar las soluciones aplicadas a los problemas reales detectados en el laboratorio, auditar el código fuente completo y proponer mejoras críticas para la integración de los próximos sensores (presión y temperatura).

---

# 1. CONTEXTO GENERAL, OBJETIVOS Y ESTRUCTURA POR HITOS

### 1.1 Objeto del Proyecto de Tesis
Diseño, fabricación, automatización e instrumentación de una **Planta Piloto Modular de Ultrafiltración por Flujo Cruzado (Cross-Flow)** con cartucho capilar de hemodiálisis/separación **Fresenius Medical Care FX100 (Helixone®)**, acoplada a un sedimentador/coagulador de acero inoxidable para tratamiento de aguas turbias.

El sistema se opera mediante un **SCADA Web local embebido en el ESP32** (sin dependencia de conexión a Internet, con Punto de Acceso propio y Servidor DNS Captive Portal integrado para impedir desconexiones automáticas en celulares y PCs).

### 1.2 Mapa de Ruta por Hitos del Proyecto
* **HITO 1 — Control Cinemático de Impulsión (COMPLETADO):**
  * Bomba peristáltica con cabezal **MBP-2000** (3 rodillos a $120^\circ$) y manguera de silicona ($\varnothing_{\text{int}} = 12\text{ mm}$, $\varnothing_{\text{ext}} = 18\text{ mm}$).
  * Motor paso a paso **NEMA 34 ($4.0\text{ N}\cdot\text{m}$, 3.0 A)** con driver industrial **Leadshine DM860 configurado a 3200 micropasos/rev (1/16)**.
  * Control por hardware LEDC PWM con rampa Sigmoide (S-Curve) de aceleración suave ($2.0\text{ RPM/s}$) para evitar el golpe de ariete sobre la membrana, parada rápida en $< 1.5\text{ s}$ e inversión de giro para retrolavado (*backwash*).
* **HITO 2 — Instrumentación y Metrología de Sensores (EN CURSO):**
  * **Subhito 2.1 (Completado):** Acondicionamiento analógico pasivo (filtros RC $R = 4.7\text{ k}\Omega, C = 100\text{ nF}, f_c \approx 338\text{ Hz}$).
  * **Subhito 2.2 (En Cierre / Objeto de esta Auditoría):** Caudalímetros de turbina Effect Hall YF-S401 en líneas de Alimentación y Permeado. Medición por período recíproco en microsegundos dentro de IRAM, cálculo de caudal instantáneo, balance de masa ($Q_{\text{ret}} = Q_{\text{alim}} - Q_{\text{perm}}$, Tasa de recuperación $Y\%$, Desviación $\Delta\%$), Datalogger multi-sesión con exportación a Excel (.CSV) y auto-tuning en régimen.
  * **Subhito 2.3 (Inmediato):** Transductores piezorresistivos de presión de acero inoxidable ($0\text{ a }30\text{ PSI}$ / $0\text{ a }2.07\text{ bar}$, salida ratiométrica $0.5\text{ a }4.5\text{ V}$) en entrada ($P_1$) y salida ($P_2$), conectados a un conversor ADC **ADS1115 de 16 bits** por bus $I^2C$. Cálculo de TMP en tiempo real ($\text{TMP} = \frac{P_1 + P_2}{2} - P_{\text{perm}}$) e interbloqueos de seguridad ($< 0.50\text{ bar}$).
  * **Subhito 2.4:** Sensor de temperatura (PT100 con MAX31865 o DS18B20 digital) para corrección por viscosidad dinámica del agua $\mu(T)$ según ecuación de Vogel y normalización a $20\ ^\circ\text{C}$ ($J_{20}$).
* **HITO 3 — Control Automático en Lazo Cerrado (PRÓXIMO):**
  * Algoritmo PID para control de caudal/presión y regulación automática de TMP.
  * Detección precoz de ensuciamiento (*fouling*) mediante el aumento de la resistencia hidráulica de torta ($R_{\text{torta}}$) según Darcy.
* **HITO 4 — Estructura Mecánica, Modularidad y Fabricación (EN INTEGRACIÓN):**
  * Tres módulos físicos: (1) Sedimentador cónico de acero inoxidable con motorreductor 12V y paleta de mezcla lenta/rápida, (2) Maletín de filtración estanco IP65 ($450 \times 450 \times 450\text{ mm}$ en chapa perforada) con segregación eléctrica/hidráulica, (3) Carro móvil de permeado ($700\text{ mm}$) con tanque acumulador de 20-25 L.

---

# 2. LOS 5 PROBLEMAS EXPERIMENTALES DETECTADOS EN LABORATORIO

El lunes 5 de octubre de 2026, el equipo (Enzo, Antonella y Owen) realizó en el Laboratorio de Operaciones Unitarias de la UNSa la **campaña experimental de calibración de la bomba y los caudalímetros con probeta graduada de $1000\text{ mL}$** recorriendo desde 20 hasta 90 RPM.

Durante esta prueba de campo surgieron **5 anomalías críticas**:

### Problema 1: Caudalímetro de permeado marcando flujo FANTASMA estando desconectado (sin agua)
* **Manifestación física:** Para calibrar la bomba y el caudalímetro de alimentación, se desconectó la manguera de alimentación de la membrana y se descargó directamente sobre la probeta graduada. Por la carcasa de permeado **no circulaba una sola gota de agua**. Sin embargo, la app SCADA marcaba que el caudalímetro de permeado tenía caudal:
  * A 60 RPM: $f_{\text{perm}} \approx 15 - 20\text{ Hz}$ ($Q_{\text{perm}} \approx 25\text{ mL/min}$).
  * A 80 RPM: $f_{\text{perm}} \approx 35 - 50\text{ Hz}$ ($Q_{\text{perm}} \approx 50\text{ mL/min}$).
  * A 90 RPM: $f_{\text{perm}} \approx 45 - 65\text{ Hz}$ ($Q_{\text{perm}} \approx 75\text{ mL/min}$).
  * ¡El sensor llegó a acumular **$10.2\text{ Litros}$ ficticios** en el datalogger!
* **Diagnóstico de Causa Raíz:**  
  La frecuencia fantasma aumentaba en **proporción directa a las RPM del motor**. Los cables del motor NEMA 34 (que conducen pulsos de 3.0 A a alta frecuencia del chopper del DM860) corren en proximidad física a los cables de señal del sensor de permeado. El pin GPIO 27 estaba configurado en el firmware como `pinMode(pin, INPUT)` (sin activar el pull-up interno del ESP32). Al quedar la línea con alta impedancia, actuó como una antena receptora que captaba los picos de conmutación electromagnética (EMI), disparando interrupciones por flanco de bajada `FALLING`. Además, el firmware anterior calculaba frecuencia incluso con un único pulso aislado en la ventana.

### Problema 2: El Datalogger sobreescribió las RPM bajas y no arrancó en $t = 0\text{ s}$
* **Manifestación física:** El archivo CSV exportado empezó en el segundo **1109** (minuto 18:29) con 60 RPM. Todos los ensayos de 20, 30, 40 y 50 RPM desaparecieron por completo. Además, todas las muestras figuraban con `Ensayo_ID = 1`.
* **Diagnóstico de Causa Raíz:**  
  1. *Sobreescritura FIFO:* El buffer circular en RAM tiene un límite de `MAX_REGISTROS = 600`. Como el firmware estaba muestreando a **1 muestra cada segundo (1 Hz)**, el buffer se llenó a los 600 segundos (10 minutos). Al extenderse la prueba a 28 minutos, el desplazamiento circular FIFO descartó los primeros 18 minutos de datos.
  2. *Falta de segmentación en caliente:* Los operadores no presionaron `PARAR` entre cada velocidad; cambiaron la consigna de RPM en el slider directamente con la bomba en marcha. Como la bomba nunca se detuvo, el firmware nunca detectó un flanco de bajada de marcha para archivar el ensayo ni incrementó `ensayoActualId`.

### Problema 3: Ruido mecánico violento a 30-40 RPM vs marcha silenciosa a 70-80 RPM
* **Manifestación física:**  
  * A 20-30 RPM: La bomba hace un ruido ensordecedor y vibra fuertemente.
  * A 40 RPM: Disminuye un poco pero sigue haciendo ruidos fuertes.
  * A 50-60 RPM: Entra en régimen.
  * A 70-80 RPM: La bomba no hace ruidos extraños, tiene un zumbido bajo y uniforme.
  * El motor se calienta considerablemente al cabo de un tiempo.
* **Diagnóstico de Causa Raíz:**  
  El motor está alimentado con **3.0 A** y el driver DM860 configurado a **3200 micropasos/rev**. A 30 RPM, la frecuencia de pasos fundamental es $f_{\text{pasos}} = \frac{30 \times 3200}{60} = 1600\text{ Hz}$ (con subarmónicos a $100-300\text{ Hz}$), coincidiendo con la **frecuencia de resonancia natural electromecánica del conjunto rotor-cabezal-chasis metálico**. A 70-80 RPM ($3733\text{ a }4266\text{ Hz}$), la frecuencia supera ampliamente la zona de resonancia mecánica y el motor trabaja en régimen armónico suave. El calentamiento se vincula a las pérdidas en el hierro por alta frecuencia y al estado del switch SW4 (corriente en reposo).

### Problema 4: Caudal pulsante y oscilaciones al manipular la manguera
* **Manifestación física:** El caudal en pantalla oscilaba continuamente y saltaba abruptamente al mover la manguera de silicona para enfocarla dentro de la probeta de 1000 mL.
* **Diagnóstico de Causa Raíz:**  
  El cabezal peristáltico de 3 rodillos produce por física intrínseca un flujo pulsante discreto (cada rodillo desplaza $\sim 4.5\text{ mL}$ por oclusión). Al doblar o mover la manguera de silicona ($\varnothing_{\text{int}} 12\text{ mm}$), cambia el volumen interno elástico, transmitiendo ondas de presión que aceleran o frenan bruscamente la pequeña turbina plástica del YF-S401. El filtro digital anterior (EMA con $\alpha = 0.4$) era demasiado reactivo y no amortiguaba el rizo hidráulico.

### Problema 5 (Ya superado): Bootloop por frecuencia SPI Flash a 80 MHz vs 40 MHz DIO
* **Diagnóstico:** El firmware anterior se grabó con `FlashFreq=80` (80 MHz). Al energizarse los bobinados del motor NEMA 34 con 3 A, el ruido de conmutación en la línea provocaba jitter en el bus SPI de la memoria Flash del ESP32, arrojando errores de lectura de caché (`1150 mmu set...`) y reiniciando el procesador. Se solucionó definitivamente flasheando el bootloader y firmware en modo **DIO a 40 MHz (`FlashMode=dio, FlashFreq=40`, `clock div: 2`)**.

---

# 3. DATOS EXPERIMENTALES OBTENIDOS EN EL BANCO DE PRUEBAS

Los tesistas Antonella y Owen cargaron los siguientes datos empíricos obtenidos con probeta graduada de $1000\text{ mL}$:

### 3.1 Ensayo 2: Calibración Cinemática de la Bomba Peristáltica (MBP-2000)
| Corrida | Consigna RPM | Tiempo [min] | Volumen Probeta [mL] | Caudal Real $Q$ [mL/min] | Cilindrada Puntual [mL/rev] | Desviación vs 13.60 mL/rev [%] |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| 1 | 20.0 | 1.0 | 280.0 | 280.0 | **14.00** | +2.9 % |
| 2 | 30.0 | 1.0 | 430.0 | 430.0 | **14.33** | +5.4 % |
| 3 | 40.0 | 1.0 | 597.0 | 597.0 | **14.93** | +9.7 % |
| 4 | 50.0 | 1.0 | 720.0 | 720.0 | **14.40** | +5.9 % |
| 5 | 60.0 | 1.0 | 820.0 | 820.0 | **13.67** | +0.5 % |
| 6 | 70.0 | 1.0 | 915.0 | 915.0 | **13.07** | -3.9 % |
| 7 | 80.0 | 0.5 | 480.0 | 960.0 | **12.00** | -11.8 % |
| 8 | 90.0 | 0.5 | 540.0 | 1080.0 | **12.00** | -11.8 % |

* **Cilindrada media global:** **$13.55\text{ mL/rev}$** (Consistente con los $13.60\text{ mL/rev}$ nominales).
* **Fenómeno reológico observado:** A partir de 80 RPM la cilindrada desciende a $12.0\text{ mL/rev}$ debido a que a alta velocidad el tubo de silicona no alcanza a recuperar su sección transversal elástica completa entre el paso de rodillos sucesivos.

### 3.2 Ensayo 3: Calibración del Caudalímetro de Alimentación YF-S401 (GPIO 14)
| Corrida | Consigna RPM | Frecuencia SCADA [Hz] | Caudal Real Probeta [L/min] | Factor K Puntual [Hz / (L/min)] | Pulsos por Litro [pul/L] |
| :---: | :---: | :---: | :---: | :---: | :---: |
| 1 | 20.0 | 60.0 | 0.280 | 214.3 | 12857 |
| 2 | 30.0 | 100.0 | 0.430 | 232.6 | 13953 |
| 3 | 40.0 | 130.0 | 0.597 | 217.8 | 13065 |
| 4 | 50.0 | 150.0 | 0.720 | 208.3 | 12500 |
| 5 | 60.0 | 160.0 | 0.820 | **195.1** | 11707 |
| 6 | 70.0 | 180.0 | 0.915 | **196.7** | 11803 |
| 7 | 80.0 | 190.0 | 0.960 | **197.9** | 11875 |
| 8 | 90.0 | 200.0 | 1.080 | 185.2 | 11111 |

* **Regresión Lineal ($F\text{ vs }Q$):** Pendiente $K = 172.72\text{ Hz/(L/min)}$, Ordenada al origen $+20.98\text{ Hz}$, $R^2 = 0.9849$.
* **Meseta de Operación Normal (60 a 80 RPM):** El factor K puntual se estabiliza en una meseta de **$195.1\text{ a }197.9\text{ Hz/(L/min)}$** (promedio: **$196.58\text{ Hz/(L/min)}$**). Nuestro valor previo precargado en firmware ($196.37$) coincidió con el valor experimental con un error menor al **$0.1\%$**.

---

# 4. SOLUCIONES Y MEJORAS IMPLEMENTADAS EN EL FIRMWARE

Para erradicar los problemas observados, aplicamos las siguientes modificaciones:

1. **Blindaje de Entrada con `INPUT_PULLUP`:**  
   En `caudalimetro.cpp`, se configuró `pinMode(_pin, INPUT_PULLUP)`. El pull-up interno del ESP32 ($\approx 45\text{ k}\Omega$) queda en paralelo con el pull-up externo de $4.7\text{ k}\Omega$, logrando $R_{\text{eq}} \approx 4.25\text{ k}\Omega$. Esto reduce la impedancia de entrada, drena corrientes capacitivas parásitas inducidas por el motor y garantiza que si el borne físico vibra o tiene falso contacto, el pin nunca quede flotando en el aire.
2. **Corte por Umbral Físico de Rotación (*Deadband / Zero-Flow Cut-off*):**  
   - Se exige que existan **al menos 2 pulsos continuos coherentes en la ventana** ($n \ge 2$) para calcular frecuencia. Un pulso solitario en 1 segundo se clasifica como ruido transitorio y se descarta.
   - Si la frecuencia calculada es menor a **$2.0\text{ Hz}$** ($< 20\text{ mL/min}$), se fuerza a cero absoluto: `_f = 0.0f; q = 0.0f;`. La turbina YF-S401 no puede girar por debajo de ese límite debido a la fricción estática del eje cerámico. Esto **erradica por completo el caudal fantasma en el permeado cuando está sin agua**.
   - Solo se integra volumen si $q > 0.0$ y $n \ge 2$, impidiendo que el permeado acumule litros ficticios.
3. **Filtro Digital EMA Sintonizado para Flujo Peristáltico:**  
   Se modificó el filtro exponencial a:
   $$\bar{q}_k = 0.20 \cdot q_k + 0.80 \cdot \bar{q}_{k-1}$$
   Con una constante de tiempo de $\tau \approx 3.5\text{ s}$, amortigua la pulsación rodillo a rodillo del cabezal peristáltico y absorbe las variaciones mecánicas al mover la manguera en la probeta. Al arrancar desde 0, responde instantáneamente sin retardo.
4. **Datalogger a 10 Segundos y Capacidad de 100 Minutos:**  
   Se fijó `INTERVALO_LOG_MS = 10000;`. Con $N = 600$ registros en RAM, la ventana de grabación cubre **$6000\text{ segundos} = 100\text{ minutos (1.66 horas)}$**, asegurando que ningún dato de bajas RPM sea sobreescrito.
5. **Segmentación Inteligente en Caliente por Cambio de Consigna de RPM:**  
   Se refactorizó la lógica en `iniciarNuevoEnsayo()` y `finalizarEnsayoActual()`. En `handleSetRPM()`, si la bomba está en marcha y el operador cambia la consigna en $\ge 1.0\text{ RPM}$ (o toca un botón de 50 a 60 RPM en el SCADA):
   - El firmware archiva automáticamente el ensayo anterior con su duración, muestras y volumen.
   - Inicia un nuevo ensayo correlativo (`Ensayo #2`, `#3`, etc.).
   - Reinicia el cronómetro del ensayo en **$t = 0\text{ s}$**.
   - Toma inmediatamente una muestra en $t = 0\text{ s}$ y continúa cada 10 segundos.

---

# 5. CÓDIGO FUENTE COMPLETO DEL FIRMWARE v4

A continuación se adjunta el código fuente completo, real y compilado actualmente en producción:

---

### 5.1. `config.h`
```cpp
#pragma once
// ==============================================================================
// CONFIGURACIÓN TÉCNICA — PLANTA DE ULTRAFILTRACIÓN (UNSa)
// Parámetros Cinemáticos, Calibración y Asignación de Pines
// ==============================================================================

#include <Arduino.h>

// ------------------------------------------------------------------------------
// 1. ASIGNACIÓN DE PINES (PINOUT)
// ------------------------------------------------------------------------------
// Driver DM860 en CÁTODO COMÚN: GPIO envía HIGH -> Optoacoplador ON
constexpr uint8_t PIN_PUL                = 18;  // DM860 PUL+ (PUL- a GND común)
constexpr uint8_t PIN_DIR                = 19;  // DM860 DIR+ (DIR- a GND común)
constexpr uint8_t PIN_LED_BOMBA          = 2;   // LED Azul Onboard (indicador de marcha)

// Caudalímetros de Efecto Hall YF-S401
constexpr uint8_t PIN_SENSOR_ALIMENTACION = 14;  // Sensor de Alimentación (Borne 12 Sup)
constexpr uint8_t PIN_SENSOR_PERMEADO     = 27;  // Sensor de Permeado (Borne 11 Sup, Pull-up 4.7k a 3.3V)

// ------------------------------------------------------------------------------
// 2. PARÁMETROS CINEMÁTICOS DE LA BOMBA (MOTOR NEMA 34 / DM860)
// ------------------------------------------------------------------------------
constexpr uint16_t PULSOS_POR_REV = 3200;     // DM860 configurado a 1/16 micropasos (3200 pulsos/rev)
constexpr float ML_POR_VUELTA     = 13.6000f; // Calibrado: 680.0 mL/min / 50.0 RPM = 13.6000 mL/rev

// Rango de Operación en RPM
constexpr float RPM_MIN           = 15.0f;    // ~204 mL/min
constexpr float RPM_MAX           = 100.0f;   // ~1360 mL/min (Habilitado para diseño factorial experimental en agua)
constexpr float RPM_INICIO        = 25.0f;    // Consigna de arranque suave (~340 mL/min)
constexpr float RPM_ALARMA_MEMBRANA = 44.0f;  // 44.12 RPM = 600 mL/min (Umbral clínico hemodiálisis / referencia)

// ------------------------------------------------------------------------------
// 3. ESPECIFICACIONES DE LA MEMBRANA FRESENIUS FX100 (HELIXONE®)
// ------------------------------------------------------------------------------
constexpr float AREA_MEMBRANA_M2       = 2.2f;    // Superficie interfacial efectiva (m²)
constexpr float K_UF_NOMINAL           = 73.0f;   // mL / (h * mmHg)
constexpr float DIAMETRO_CAPILAR_UM    = 185.0f;  // Diámetro interno capilar (μm)
constexpr float ESPESOR_PARED_UM       = 35.0f;   // Grosor de pared capilar (μm)
constexpr float TMP_MAX_SEGURA_BAR     = 0.50f;   // Límite máximo admisible de TMP (bar)
constexpr float Q_CLINICO_SANGRE_MAX   = 600.0f;  // mL/min (límite en hemodiálisis clínica)

// Rampas: Arranque Suave Confiable (LEDC Seguro) y Frenado Rápido (< 1.5s)
constexpr float ACEL_ARRANQUE_RPM_S  = 2.0f;   // 2.0 RPM/s despegue inicial suave y confiable
constexpr float ACEL_NOMINAL_RPM_S   = 3.5f;   // 3.5 RPM/s aceleración progresiva
constexpr float DESACEL_AJUSTE_RPM_S = 8.0f;   // 8.0 RPM/s desaceleración progresiva en marcha
constexpr float FRENADO_PARADA_RPM_S = 45.0f;  // 45.0 RPM/s frenado rápido al pulsar STOP (< 1.5s)

// Constantes de Calibración de Caudalímetros YF-S401
// Convención metrológica: K en [Hz / (L/min)]
//   F [Hz] = K * Q [L/min]  ===>  Q [mL/min] = (F [Hz] * 1000) / K
//   Pulsos por Litro = K * 60
// Calibración experimental con probeta validada en laboratorio (20 a 90 RPM):
constexpr float K_ALIMENTACION = 196.50f; // Hz/(L/min) -> 11790 pulsos/L (meseta experimental a 60-80 RPM)
constexpr float K_PERMEADO     = 687.33f; // Hz/(L/min) -> valor calibrado nominal sensor permeado

constexpr uint32_t FILTRO_RUIDO_US = 1500;    // 1.5 ms de blanking anti-rebote (hasta 666 Hz / ~4300 mL/min)
// Auditoría Ronda 4: A 100 RPM el caudal máximo de bomba es 1360 mL/min.
// Un límite de 2500 mL/min actúa como filtro activo anti-ruido EMI sin limitar el flujo real.
constexpr float Q_MAX_FISICO_MLMIN = 2500.0f; // Límite físico de plausibilidad (2.5 L/min)

// ------------------------------------------------------------------------------
// 4. PARÁMETROS DEL DATALOGGER MULTI-SESIÓN Y AUTO-CALIBRACIÓN
// ------------------------------------------------------------------------------
constexpr uint32_t INTERVALO_LOG_MS   = 10000;  // Muestreo cada 10 segundos (600 muestras = 100 minutos continuos)
constexpr size_t   MAX_REGISTROS      = 600;    // 600 muestras = 1.66 horas de ensayo continuo sin sobreescritura
constexpr size_t   MAX_ENSAYOS        = 20;     // Hasta 20 corridas/ensayos registrados en memoria
constexpr uint8_t  MUESTRAS_AUTO_CAL  = 15;     // 15 muestras (15 seg) para auto-calibración en régimen permanente

// ------------------------------------------------------------------------------
// 5. CONFIGURACIÓN DE RED WI-FI Y SCADA
// ------------------------------------------------------------------------------
constexpr const char* SSID_AP  = "Bomba_Peristaltica_UF";
constexpr const char* PASS_AP  = "plantapiloto2";
constexpr const char* SSID_STA = "Box804";
constexpr const char* PASS_STA = "plantapiloto2";
```

---

### 5.2. `caudalimetro.h`
```cpp
#pragma once
#include <Arduino.h>
#include "config.h"

// ==============================================================================
// CLASE CAUDALÍMETRO (Sensor de Efecto Hall YF-S401)
// - Lectura por interrupción en memoria IRAM (IRAM_ATTR) con paso de puntero
// - Blanking anti-rebote (FILTRO_RUIDO_US = 1500 us) + Filtro RC analógico
// - Medición por período recíproco de alta resolución (inmune a cuantización ±1 pulso)
// - Corte por velocidad mínima física (Deadband) anti-ruido EMI
// - Sincronización atómica multinúcleo con spinlock FreeRTOS (portMUX_TYPE)
// - Calibración en tiempo real (setK / getK) en unidades [Hz / (L/min)]
// - Diagnóstico de señal asimétrico (Alarma solo en Alimentación)
// ==============================================================================

class Caudalimetro {
public:
  Caudalimetro(uint8_t pin, float k, const char* nombre, bool esAlimentacion = true);

  void begin();
  void actualizar(float dt_s, bool bombaEmpuja);

  float caudal_mLmin()   const { return _q; }
  float caudal_Lmin()    const { return _q / 1000.0f; }
  float frecuencia_Hz()  const { return _f; }
  float volumen_L()      const { return _vol; }
  bool  sinSenal()       const { return _fallo; }
  void  resetVolumen()         { _vol = 0.0f; }
  const char* nombre()   const { return _nombre; }
  bool  esAlimentacion() const { return _esAlimentacion; }

  // Calibración dinámica sin recompilar [Hz / (L/min)]
  void  setK(float nuevoK)     { if (nuevoK > 0.1f) _k = nuevoK; }
  float getK()           const { return _k; }

private:
  static void IRAM_ATTR isrPuente(void* arg);

  const uint8_t _pin;
  float         _k;
  const char*   _nombre;
  const bool    _esAlimentacion;

  volatile uint32_t _pulsos = 0;
  volatile uint32_t _t_ultimo = 0;
  volatile uint32_t _t_primero = 0;
  volatile uint32_t _periodo_us = 0;

  float _f = 0.0f;
  float _q = 0.0f;
  float _vol = 0.0f;
  float _tiempoSinPulso_s = 0.0f;
  bool  _fallo = false;

  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
};
```

---

### 5.3. `caudalimetro.cpp`
```cpp
#include "caudalimetro.h"

Caudalimetro::Caudalimetro(uint8_t pin, float k, const char* nombre, bool esAlimentacion)
  : _pin(pin), _k(k), _nombre(nombre), _esAlimentacion(esAlimentacion) {}

void Caudalimetro::begin() {
  // Activar resistencia INPUT_PULLUP interna del ESP32 (45 kΩ a 3.3V) en paralelo con el circuito RC externo.
  // Garantiza máxima rigidez contra acoplamiento inductivo y asegura nivel lógico HIGH
  // evitando que el pin actúe como antena ante el campo magnético del motor NEMA 34.
  pinMode(_pin, INPUT_PULLUP);
  attachInterruptArg(digitalPinToInterrupt(_pin), isrPuente, this, FALLING);
}

void Caudalimetro::actualizar(float dt_s, bool bombaEmpuja) {
  // Captura atómica de variables acumuladas por la ISR en la ventana transcurrida
  portENTER_CRITICAL(&_mux);
  uint32_t n      = _pulsos;
  _pulsos         = 0;
  uint32_t per_us = _periodo_us;
  uint32_t t_prim = _t_primero;
  uint32_t t_ult  = _t_ultimo;
  portEXIT_CRITICAL(&_mux);

  // 1. CÁLCULO DE FRECUENCIA CON MÉTODO DE PERÍODO RECÍPROCO DE ALTA RESOLUCIÓN:
  // Exige al menos 2 pulsos continuos en la ventana de 1 segundo para validar rotación real.
  // Un pulso único o aislado es descartado como ruido transitorio electromagnético.
  if (n >= 2 && (t_ult - t_prim) > 0) {
    _f = ((float)(n - 1) * 1000000.0f) / (float)(t_ult - t_prim);
  } else {
    _f = 0.0f;
  }

  // 2. UMBRAL DE CORTE DE VELOCIDAD MÍNIMA (DEADBAND / ZERO-FLOW CUT-OFF):
  // La turbina YF-S401 tiene fricción mecánica estática en su eje cerámico. Físicamente no gira
  // de forma continua por debajo de ~2.5 Hz (< 20 mL/min).
  // Toda frecuencia < 2.0 Hz es cortada a cero absoluto para garantizar 0.0 mL/min cuando
  // no circula agua (eliminando el caudal fantasma de permeado por acoplamiento con el motor).
  if (_f < 2.0f) {
    _f = 0.0f;
  }

  // 3. CÁLCULO DE CAUDAL INSTANTÁNEO EN mL/min:
  // Q [mL/min] = (F * 1000.0) / K
  float q = (_f * 1000.0f) / _k;

  // Filtro de plausibilidad física (corte de picos transitorios por perturbación EMI)
  if (q > Q_MAX_FISICO_MLMIN) {
    q = 0.0f;
    _f = 0.0f;
    Serial.printf("[%s] Ruido EMI descartado: %lu pulsos espurios\n", _nombre, (unsigned long)n);
  } else if (_f > 0.0f) {
    // 4. INTEGRACIÓN DE VOLUMEN TOTALIZADO EN LITROS:
    // Solo se acumula volumen si hay flujo real continuo confirmado
    _vol += (float)n / (_k * 60.0f);
  }

  // 5. FILTRO EXPONENCIAL PONDERADO (EMA) SINTONIZADO PARA FLUJO PERISTÁLTICO:
  // Suaviza la pulsación rodillo a rodillo del cabezal peristáltico de 3 rodillos (tau ≈ 3.5 segundos).
  // Evita que la lectura salte o baile en pantalla al mover la manguera en la probeta.
  if (q > 0.0f) {
    if (_q == 0.0f) {
      _q = q; // Respuesta rápida desde reposo
    } else {
      _q = 0.20f * q + 0.80f * _q; // Filtrado estable
    }
  } else {
    // Decaimiento rápido a cero al detenerse el flujo
    _q = 0.5f * _q;
    if (_q < 1.0f) _q = 0.0f;
  }

  // 6. DIAGNÓSTICO ASIMÉTRICO DE PÉRDIDA DE SEÑAL / CABLE CORTADO:
  // - Alimentación: si la bomba empuja (RPM > 1) y pasan 5 segundos sin pulsos -> FALLA CRÍTICA.
  // - Permeado: caudal nulo es una condición admisible (válvula cerrada o baja TMP) -> NO alarma.
  if (n > 0 || _f > 0.05f) {
    _tiempoSinPulso_s = 0.0f;
    _fallo = false;
  } else if (bombaEmpuja && _esAlimentacion) {
    _tiempoSinPulso_s += dt_s;
    if (_tiempoSinPulso_s >= 5.0f) _fallo = true;
  } else {
    _tiempoSinPulso_s = 0.0f;
    _fallo = false;
  }
}

void IRAM_ATTR Caudalimetro::isrPuente(void* arg) {
  Caudalimetro* c = reinterpret_cast<Caudalimetro*>(arg);
  uint32_t t = micros();

  portENTER_CRITICAL_ISR(&c->_mux);
  // Diferencia sin signo uint32_t: segura ante desbordamiento de micros() cada 71.58 min
  uint32_t dt = t - c->_t_ultimo;

  // Blanking anti-rebote: descarta transitorios mecánicos y capacitivos (FILTRO_RUIDO_US = 1500 us)
  if (dt >= FILTRO_RUIDO_US) {
    if (c->_pulsos == 0) {
      c->_t_primero = t;
    }
    // Si el tiempo transcurrido es menor a 2 segundos, registramos el período inter-pulso real
    if (dt < 2000000UL) {
      c->_periodo_us = dt;
    } else {
      c->_periodo_us = 0; // Tras una parada prolongada, se descarta el período espurio
    }
    c->_t_ultimo = t;
    c->_pulsos++;
  }
  portEXIT_CRITICAL_ISR(&c->_mux);
}
```

---

### 5.4. `Bomba.h`
```cpp
#pragma once
#include <Arduino.h>
#include <cmath>
#include "config.h"

// ==============================================================================
// CLASE BOMBA PERISTÁLTICA — DECLARACIÓN
// - Control de frecuencia por hardware LEDC PWM (Driver DM860)
// - Rampa Cuadrática S-Curve de arranque suave (sin golpe de torque)
// - Parada rápida en < 1.5s
// ==============================================================================

class Bomba {
public:
  void begin();
  void arrancar();
  void detener();
  bool setRPM(float rpm);
  void toggleSentido();

  float rpmActual() const           { return _actual; }
  float rpmObjetivo() const         { return _objetivo; }
  bool  enMarcha() const            { return _enMarcha; }
  bool  invirtiendo() const         { return _invirtiendo; }
  bool  sentidoHorario() const      { return _horario; }
  bool  enRegimenEstable() const    { return (_enMarcha && fabsf(_actual - _objetivo) < 0.3f && _actual > 5.0f); }
  float caudalTeorico_mLmin() const { return _actual * _mlPorVuelta; }

  void  setMlPorVuelta(float ml)    { if (ml > 0.1f) _mlPorVuelta = ml; }
  float getMlPorVuelta() const      { return _mlPorVuelta; }

  void     setPulsosPorRev(uint16_t p) { if (p >= 200) _pulsosPorRev = p; }
  uint16_t getPulsosPorRev() const     { return _pulsosPorRev; }

  void tick(float dt);

private:
  void fijarSentido(bool horario);

  bool _enMarcha = false;
  bool _horario = true;
  bool _invirtiendo = false;

  float _objetivo = RPM_INICIO;
  float _actual = 0.0f;
  float _rpmGuardada = RPM_INICIO;
  uint32_t _fActual = 0;

  float    _mlPorVuelta  = ML_POR_VUELTA;
  uint16_t _pulsosPorRev = PULSOS_POR_REV;
};
```

---

### 5.5. `Bomba.cpp`
```cpp
#include "Bomba.h"

// ==============================================================================
// IMPLEMENTACIÓN DE LA CLASE BOMBA
// Generación PWM por hardware LEDC con frecuencia mínima segura (>= 50 Hz)
// ==============================================================================

void Bomba::begin() {
  pinMode(PIN_DIR, OUTPUT);
  fijarSentido(true); // Sentido horario por defecto

#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
  ledcAttach(PIN_PUL, 800, 10);
  ledcWrite(PIN_PUL, 0); // Reposo: LOW -> Opto OFF (motor libre)
#else
  ledcSetup(0, 800, 10);
  ledcAttachPin(PIN_PUL, 0);
  ledcWrite(0, 0);
#endif
}

void Bomba::arrancar() {
  _enMarcha = true;
  if (_objetivo < RPM_MIN) {
    _objetivo = (_rpmGuardada >= RPM_MIN) ? _rpmGuardada : RPM_INICIO;
  }
}

void Bomba::detener() {
  _enMarcha = false;
  _invirtiendo = false;
  if (_objetivo >= RPM_MIN) {
    _rpmGuardada = _objetivo;
  }
}

bool Bomba::setRPM(float rpm) {
  if (!std::isfinite(rpm)) return false;
  float r = constrain(rpm, RPM_MIN, RPM_MAX);
  if (_invirtiendo) {
    _rpmGuardada = r;   // Almacena consigna si el usuario mueve slider durante frenado
  } else {
    _objetivo = r;
    _rpmGuardada = r;
  }
  return true;
}

void Bomba::toggleSentido() {
  if (_invirtiendo) return;
  if (_actual < 1.0f) {
    fijarSentido(!_horario);
  } else {
    _invirtiendo = true;
    _rpmGuardada = (_objetivo >= RPM_MIN) ? _objetivo : RPM_INICIO;
    _objetivo = 0.0f;
  }
}

void Bomba::fijarSentido(bool horario) {
  _horario = horario;
  digitalWrite(PIN_DIR, horario ? LOW : HIGH);
}

void Bomba::tick(float dt) {
  float objetivo = _enMarcha ? _objetivo : 0.0f;

  if (_actual < objetivo) {
    if (_actual < 1.0f) {
      _actual = 1.0f;
    }
    
    // Rampa de aceleración progresiva
    float tasaAcel = ACEL_NOMINAL_RPM_S;
    if (_actual < 10.0f) {
      tasaAcel = ACEL_ARRANQUE_RPM_S + (_actual / 10.0f) * (ACEL_NOMINAL_RPM_S - ACEL_ARRANQUE_RPM_S);
    }
    float delta = objetivo - _actual;
    if (delta < 3.0f) {
      tasaAcel = fmaxf(0.8f, tasaAcel * (delta / 3.0f));
    }
    _actual = fminf(objetivo, _actual + tasaAcel * dt);

  } else if (_actual > objetivo) {
    if (!_enMarcha) {
      // PARADA RÁPIDA: Frenado ágil en menos de 1.5s
      _actual = fmaxf(0.0f, _actual - FRENADO_PARADA_RPM_S * dt);
      if (_actual < 1.0f) {
        _actual = 0.0f;
      }
    } else {
      // Reducción suave de velocidad en marcha
      float tasaDecel = DESACEL_AJUSTE_RPM_S;
      float delta = _actual - objetivo;
      if (delta < 3.0f) {
        tasaDecel = fmaxf(1.0f, tasaDecel * (delta / 3.0f));
      }
      _actual = fmaxf(objetivo, _actual - tasaDecel * dt);
    }
  }

  // Inversión de giro segura
  if (_invirtiendo && _actual <= 0.1f) {
#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    ledcWrite(PIN_PUL, 0);
#else
    ledcWrite(0, 0);
#endif
    _fActual = 0;
    fijarSentido(!_horario);
    _invirtiendo = false;
    _objetivo = (_rpmGuardada >= RPM_MIN) ? _rpmGuardada : RPM_INICIO;
  }

  // Generación de pulsos PWM por hardware LEDC
  if (_actual >= 1.0f) {
    uint32_t f = (uint32_t)(_actual * (float)_pulsosPorRev / 60.0f);
    if (f < 50) f = 50; // Límite de seguridad mínimo del temporizador LEDC

#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    if (f != _fActual) { 
      ledcChangeFrequency(PIN_PUL, f, 10); 
      _fActual = f; 
    }
    ledcWrite(PIN_PUL, 512); // Ciclo de trabajo 50%
#else
    if (f != _fActual) { 
      ledcSetup(0, f, 10); 
      _fActual = f; 
    }
    ledcWrite(0, 512);
#endif
  } else if (_fActual != 0) {
#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    ledcWrite(PIN_PUL, 0);
#else
    ledcWrite(0, 0);
#endif
    _fActual = 0;
  }
}
```

---

### 5.6. `darcy.h`
```cpp
#pragma once
// ==============================================================================
// MODELO DE TRANSPORTE Y RESISTENCIAS EN SERIE (LEY DE DARCY) — MEMBRANA FX100
// Diseñado para la Tesis de Grado (Antonella Guitián & Owen Cañizares / Codir. Ing. Enzo)
// ==============================================================================

#include <Arduino.h>
#include <cmath>
#include "config.h"

struct ResultadoDarcy {
  float J_LMH;       // Flujo de permeado volumétrico específico [L / (m² · h)]
  float J20_LMH;     // Flujo normalizado a 20 °C [LMH]
  float mu_Pas;      // Viscosidad dinámica del agua a temperatura T [Pa · s]
  float TCF;         // Factor de corrección por temperatura (mu(T) / mu_20)
  float R_total;     // Resistencia hidráulica total [m^-1]
  float R_torta;     // Resistencia por capa de torta / ensuciamiento [m^-1]
  float TMP_bar;     // Presión transmembrana efectiva [bar]
  bool  valido;      // Estado de cálculo válido
};

class ModeloDarcy {
public:
  static constexpr float MU20 = 1.002e-3f; // Pa · s (Agua destilada a 20.0 °C)

  // Viscosidad dinámica del agua mediante ecuación de Vogel (válida 5 a 60 °C)
  static float viscosidadAgua(float temp_C) {
    const float T_K = temp_C + 273.15f;
    return 2.414e-5f * powf(10.0f, 247.8f / (T_K - 140.0f));
  }

  void  setRm(float rm) { if (rm > 1e11f && rm < 1e16f) _Rm = rm; }
  float Rm() const      { return _Rm; }

  // Cálculo hidráulico en tiempo real
  ResultadoDarcy calcular(float qPerm_mLmin, float tmp_bar, float temp_C = 20.0f) const {
    ResultadoDarcy r{};
    if (!std::isfinite(tmp_bar) || !std::isfinite(temp_C) || !std::isfinite(qPerm_mLmin)) {
      return r;
    }
    if (tmp_bar < 0.01f || qPerm_mLmin < 0.5f) {
      return r;
    }

    r.TMP_bar = tmp_bar;
    r.mu_Pas  = viscosidadAgua(temp_C);
    r.TCF     = r.mu_Pas / MU20;

    // J [LMH] = Q [L/h] / Area [m²] = (qPerm [mL/min] * 0.06) / 2.2 m²
    r.J_LMH   = (qPerm_mLmin * 0.06f) / AREA_MEMBRANA_M2;
    r.J20_LMH = r.J_LMH * r.TCF;

    // J en unidades SI [m/s]: 1 LMH = 1 / 3.6e6 m/s
    const float J_SI   = r.J_LMH / 3.6e6f;
    const float TMP_Pa = tmp_bar * 100000.0f;

    // Ley de Darcy: J = TMP / (mu * R_total) => R_total = TMP / (mu * J)
    r.R_total = TMP_Pa / (r.mu_Pas * J_SI);
    r.R_torta = r.R_total - _Rm;
    r.valido  = true;

    return r;
  }

  // Calibración experimental de Rm con agua limpia (regresión lineal J vs TMP sin ordenada al origen)
  void acumularPuntoAguaLimpia(float J_SI, float TMP_Pa) {
    _sumJP += J_SI * TMP_Pa;
    _sumPP += TMP_Pa * TMP_Pa;
  }

  bool finalizarCalibracionRm(float temp_C = 20.0f) {
    if (_sumPP <= 0.0f || _sumJP <= 0.0f) return false;
    const float Lp = _sumJP / _sumPP; // Permeabilidad hidráulica en m / (s · Pa)
    const float mu = viscosidadAgua(temp_C);
    _Rm = 1.0f / (mu * Lp);
    _sumJP = 0.0f;
    _sumPP = 0.0f;
    return true;
  }

private:
  // Resistencia intrínseca nominal de la membrana FX100 Helixone® (K_UF = 73 mL/h*mmHg, 2.2 m²).
  // Según ISO 8637, K_UF se especifica a 37 °C (mu_37 = 0.6915 mPa·s).
  // Lp = (73 mL/h/mmHg) / (2.2 m² * 3600 s/h * 133.322 Pa/mmHg) = 6.9135e-11 m/(s·Pa)
  // Rm_nominal = 1 / (mu_37 * Lp) = 2.09e13 m^-1.
  float _Rm = 2.09e13f; 
  float _sumJP = 0.0f;
  float _sumPP = 0.0f;
};
```

---

### 5.7. `EN_USO_firmware_planta.ino` (Extracto Principal con Datalogger y Máquina de Estados)
```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "config.h"
#include "caudalimetro.h"
#include "Bomba.h"
#include "darcy.h"
#include "index_html.h"

// Servidor DNS para Portal Cautivo Anti-Desconexión en Android / iOS / Windows
DNSServer dnsServer;
constexpr uint16_t DNS_PORT = 53;

// ------------------------------------------------------------------------------
// ESTRUCTURAS DE DATOS PARA EL DATALOGGER Y GESTIÓN DE ENSAYOS
// ------------------------------------------------------------------------------
struct RegistroCalibracion {
  uint8_t  id_ensayo;
  uint32_t t_relativo_s;
  float    rpm;
  float    q_bomba;
  float    f_alim;
  float    q_alim;
  float    vol_alim;
  float    f_perm;
  float    q_perm;
  float    vol_perm;
  float    q_ret;
  float    recov;
  float    delta;
  float    k_alim;
  float    k_perm;
  float    j_lmh;
};

struct EnsayoInfo {
  uint8_t  id;
  float    rpm_consigna;
  uint32_t t_inicio_ms;
  uint32_t duracion_s;
  uint16_t muestras;
  float    vol_alim;
  float    vol_perm;
};

// Buffers de almacenamiento en RAM
RegistroCalibracion bufferLog[MAX_REGISTROS];
size_t numRegistros = 0;

EnsayoInfo listaEnsayos[MAX_ENSAYOS];
size_t numEnsayos = 0;
uint8_t ensayoActualId = 1;

uint32_t tInicioEnsayo_ms = 0;
float volAlimInicioEnsayo = 0.0f;
float volPermInicioEnsayo = 0.0f;
bool bombaEnMarchaAnterior = false;

// Instancias de Clases
Bomba bomba;
Caudalimetro sensorAlimentacion(PIN_SENSOR_ALIMENTACION, K_ALIMENTACION, "ALIMENTACION", true);
Caudalimetro sensorPermeado(PIN_SENSOR_PERMEADO, K_PERMEADO, "PERMEADO", false);
ModeloDarcy darcy;
WebServer server(80);
Preferences prefs;

// Variables de Control
float qRet_mLmin = 0.0f;
float recuperacion = 0.0f;
float deltaBomba = 0.0f;
float jLMH_actual = 0.0f;
bool flagCruceSensores = false;

// Auto-Calibración
bool autoCalibrando = false;
uint8_t autoCalMuestras = 0;
float autoCalSumFrecAlim = 0.0f;
float autoCalSumFrecPerm = 0.0f;
String autoCalMensaje = "";

// Temporizadores no bloqueantes
uint32_t tLoop = 0;
uint32_t tCaudal = 0;
uint32_t tDatalogger = 0;

// ------------------------------------------------------------------------------
// FUNCIÓN PARA GUARDAR MUESTRA EN EL DATALOGGER (CADA 10 SEGUNDOS)
// ------------------------------------------------------------------------------
void guardarMuestraDatalogger() {
  if (tInicioEnsayo_ms == 0) {
    tInicioEnsayo_ms = millis();
  }

  uint32_t t_rel_s = (millis() - tInicioEnsayo_ms) / 1000;

  // Desplazamiento FIFO circular si el buffer se llena
  if (numRegistros >= MAX_REGISTROS) {
    for (size_t i = 0; i < MAX_REGISTROS - 1; i++) {
      bufferLog[i] = bufferLog[i + 1];
    }
    numRegistros = MAX_REGISTROS - 1;
  }

  RegistroCalibracion reg;
  reg.id_ensayo    = ensayoActualId;
  reg.t_relativo_s = t_rel_s;
  reg.rpm          = bomba.rpmActual();
  reg.q_bomba      = bomba.caudalTeorico_mLmin();
  reg.f_alim       = sensorAlimentacion.frecuencia_Hz();
  reg.q_alim       = sensorAlimentacion.caudal_mLmin();
  reg.vol_alim     = sensorAlimentacion.volumen_L();
  reg.f_perm       = sensorPermeado.frecuencia_Hz();
  reg.q_perm       = sensorPermeado.caudal_mLmin();
  reg.vol_perm     = sensorPermeado.volumen_L();
  reg.q_ret        = qRet_mLmin;
  reg.recov        = recuperacion;
  reg.delta        = deltaBomba;
  reg.k_alim       = sensorAlimentacion.getK();
  reg.k_perm       = sensorPermeado.getK();
  reg.j_lmh        = (reg.q_perm * 0.06f) / AREA_MEMBRANA_M2;

  bufferLog[numRegistros++] = reg;

  Serial.printf("[LOG #%u][Ensayo %u] t=%us | RPM=%.1f | Q_Alim=%.1f mL/min | Q_Perm=%.1f mL/min | J=%.2f LMH | Y=%.1f%%\n",
                (unsigned int)numRegistros, ensayoActualId, t_rel_s, reg.rpm, reg.q_alim, reg.q_perm, reg.j_lmh, reg.recov);
}

// ------------------------------------------------------------------------------
// GESTIÓN AUTOMÁTICA DE SESIONES Y ENSAYOS
// ------------------------------------------------------------------------------
void finalizarEnsayoActual() {
  uint32_t duracion_s = (tInicioEnsayo_ms > 0) ? ((millis() - tInicioEnsayo_ms) / 1000) : 0;
  uint16_t muestrasEnsayo = 0;
  for (size_t i = 0; i < numRegistros; i++) {
    if (bufferLog[i].id_ensayo == ensayoActualId) muestrasEnsayo++;
  }

  if (muestrasEnsayo == 0) {
    guardarMuestraDatalogger();
    muestrasEnsayo = 1;
  }

  if (numEnsayos < MAX_ENSAYOS) {
    listaEnsayos[numEnsayos].id           = ensayoActualId;
    listaEnsayos[numEnsayos].rpm_consigna = bomba.rpmObjetivo();
    listaEnsayos[numEnsayos].t_inicio_ms  = tInicioEnsayo_ms;
    listaEnsayos[numEnsayos].duracion_s   = duracion_s;
    listaEnsayos[numEnsayos].muestras     = muestrasEnsayo;
    listaEnsayos[numEnsayos].vol_alim     = sensorAlimentacion.volumen_L() - volAlimInicioEnsayo;
    listaEnsayos[numEnsayos].vol_perm     = sensorPermeado.volumen_L() - volPermInicioEnsayo;
    numEnsayos++;

    Serial.printf("\n<<< [ENSAYO #%u FINALIZADO Y REGISTRADO] Consigna: %.1f RPM | Duracion: %us | Muestras: %u | Vol Perm: %.3f L >>>\n\n",
                  ensayoActualId, bomba.rpmObjetivo(), duracion_s, muestrasEnsayo, sensorPermeado.volumen_L() - volPermInicioEnsayo);
  }
}

void iniciarNuevoEnsayo(float consignaRpm) {
  uint16_t muestrasPrevias = 0;
  for (size_t i = 0; i < numRegistros; i++) {
    if (bufferLog[i].id_ensayo == ensayoActualId) muestrasPrevias++;
  }
  if (muestrasPrevias > 0) {
    ensayoActualId++;
  }
  tInicioEnsayo_ms = millis();
  volAlimInicioEnsayo = sensorAlimentacion.volumen_L();
  volPermInicioEnsayo = sensorPermeado.volumen_L();
  tDatalogger = millis();
  guardarMuestraDatalogger(); // Muestra inicial garantizada en t = 0s
  Serial.printf("\n>>> [ENSAYO #%u INICIADO] Consigna: %.1f RPM <<<\n", ensayoActualId, consignaRpm);
}

// ------------------------------------------------------------------------------
// MANEJADORES DE RUTAS DEL SERVIDOR WEB
// ------------------------------------------------------------------------------
void handleSetRPM() {
  if (server.hasArg("rpm")) {
    float nuevoRpm = server.arg("rpm").toFloat();
    float rpmActualConsigna = bomba.rpmObjetivo();

    // Segmentar automáticamente si se cambia la velocidad con la bomba en marcha
    if (bomba.enMarcha() && fabs(nuevoRpm - rpmActualConsigna) >= 1.0f) {
      finalizarEnsayoActual();
      if (bomba.setRPM(nuevoRpm)) {
        iniciarNuevoEnsayo(nuevoRpm);
        server.send(200, "text/plain", "OK");
      } else {
        server.send(400, "text/plain", "ERROR: RPM fuera de rango");
      }
      return;
    }

    if (bomba.setRPM(nuevoRpm)) {
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "ERROR: RPM fuera de rango o invalido");
    }
  } else {
    server.send(400, "text/plain", "Falta argumento rpm");
  }
}

// ------------------------------------------------------------------------------
// BUCLE PRINCIPAL (LOOP NO BLOQUEANTE)
// ------------------------------------------------------------------------------
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  uint32_t tAhora = millis();

  // 1. Control cinemático cada 50 ms
  if (tAhora - tLoop >= 50) {
    float dt = (tAhora - tLoop) / 1000.0f;
    tLoop = tAhora;
    bomba.tick(dt);

    digitalWrite(PIN_LED_BOMBA, (bomba.rpmActual() > 0.5f) ? HIGH : LOW);

    bool enMarcha = bomba.enMarcha();
    if (enMarcha && !bombaEnMarchaAnterior) {
      iniciarNuevoEnsayo(bomba.rpmObjetivo());
    }
    if (!enMarcha && bombaEnMarchaAnterior) {
      finalizarEnsayoActual();
    }
    bombaEnMarchaAnterior = enMarcha;
  }

  // 2. Adquisición de caudales cada 1000 ms (1 segundo)
  if (tAhora - tCaudal >= 1000) {
    float dt = (tAhora - tCaudal) / 1000.0f;
    tCaudal = tAhora;

    bool bombaEmpuja = (bomba.rpmActual() > 1.0f);
    sensorAlimentacion.actualizar(dt, bombaEmpuja);
    sensorPermeado.actualizar(dt, bombaEmpuja);

    float qAlim = sensorAlimentacion.caudal_mLmin();
    float qPerm = sensorPermeado.caudal_mLmin();

    flagCruceSensores = (qPerm > qAlim + 50.0f) && (bomba.rpmActual() > 5.0f);
    qRet_mLmin = (qAlim > qPerm) ? (qAlim - qPerm) : 0.0f;
    recuperacion = (qAlim > 5.0f) ? ((qPerm / qAlim) * 100.0f) : 0.0f;

    float qTeorico = bomba.caudalTeorico_mLmin();
    deltaBomba = (qTeorico > 5.0f) ? (((qAlim - qTeorico) / qTeorico) * 100.0f) : 0.0f;
  }

  // 3. Datalogger periódico cada INTERVALO_LOG_MS (10 segundos)
  if (tAhora - tDatalogger >= INTERVALO_LOG_MS) {
    tDatalogger = tAhora;
    if (bomba.enMarcha()) {
      guardarMuestraDatalogger();
    }
  }
}
```

---

# 6. CUESTIONARIO DE AUDITORÍA Y CERTIFICACIÓN

Como IA Auditora Externa, evalúa los siguientes puntos y emite tu veredicto:

### Pregunta 1: Eficacia del Blindaje contra Ruido EMI en los Caudalímetros
¿Consideras que la combinación de:
* `pinMode(_pin, INPUT_PULLUP)` interno en paralelo con el circuito RC externo ($4.7\text{ k}\Omega + 100\text{ nF}$).
* La exigencia estricta de al menos 2 pulsos consecutivos ($n \ge 2$).
* La zona muerta de corte (*Deadband*) a $f < 2.0\text{ Hz}$ ($< 20\text{ mL/min}$).
es suficiente y metrológicamente adecuada para garantizar que el caudalímetro de permeado marque exactamente **0.0 mL/min** cuando la línea esté cerrada o sin agua? ¿Recomiendas algún mecanismo adicional en software o hardware?

### Pregunta 2: Arquitectura del Datalogger y Segmentación en Caliente
¿Es adecuada la estrategia de muestreo a 10 segundos ($600\text{ registros} = 100\text{ minutos}$) y la segmentación automática al cambiar las RPM sobre la marcha para un ensayo experimental de ingeniería de ultrafiltración? ¿Observas algún riesgo de fragmentación de memoria o desbordamiento de índices en el buffer circular de RAM del ESP32?

### Pregunta 3: Diagnóstico del Ruido a 30-40 RPM vs 70-80 RPM
Dado que el driver Leadshine DM860 ya está configurado en **3200 micropasos (1/16)** y la corriente está ajustada en **3.0 A** (en un motor NEMA 34 nominal de 4.0 A):
¿Coincides en que el ruido a 30-40 RPM es puramente una resonancia armónica estructural de baja frecuencia? ¿Qué medidas mecánicas o de configuración del driver (función de auto-tuning del DM860, desacoplo elastomérico, silentblocks) recomiendas para amortiguar este efecto en el chasis metálico?

### Pregunta 4: Preparación para los Próximos Sensores (Hito 2.3: Presión y TMP)
Estamos a punto de incorporar los dos transductores piezorresistivos de acero inoxidable de **$0\text{ a }30\text{ PSI}$ ($0\text{ a }2.07\text{ bar}$)** con salida ratiométrica de **$0.5\text{ a }4.5\text{ V}$** conectados mediante un conversor ADC **ADS1115 de 16 bits** por bus $I^2C$ (GPIO 21 SDA, GPIO 22 SCL):
* ¿Qué consideraciones críticas de acondicionamiento de señal, divisor resistivo o protección de sobretensión de 5V a 3.3V recomiendas para el bus $I^2C$ y las entradas analógicas?
* ¿Cómo debe estructurarse el cálculo de la Presión Transmembrana ($\text{TMP} = \frac{P_1 + P_2}{2} - P_{\text{perm}}$) y el interbloqueo de seguridad para apagar la bomba si la presión supera $0.50\text{ bar}$ antes de romper las fibras de la membrana FX100?

### Pregunta 5: Veredicto Final y Propuestas de Mejora
Emite tu dictamen:  
**[APROBADO / APROBADO CON OBSERVACIONES / RECHAZADO]**  
¿Está el firmware y el sistema de caudalímetros listo para certificar el Subhito 2.2 y dar paso formal a la instalación de los sensores de presión? ¿Qué otras mejoras propones?
