# 📘 MANUAL TÉCNICO EXHAUSTIVO, DIAGNÓSTICO DE LABORATORIO Y EXPLICACIÓN LÍNEA POR LÍNEA — FIRMWARE VERSIÓN V6

## Planta Piloto de Ultrafiltración Tangencial FX100 (Helixone®) • Tesis de Grado en Ingeniería Industrial

**Institución**: Universidad Nacional de Salta (UNSa) — Facultad de Ingeniería

**Tesistas**: Antonella Guitián & Owen Cañizares

**Codirector**: Ing. Enzo Corte

**Versión de Firmware Documentada**: `Versión V6` (Calibración Oficial Ensayos 2 y 3 + Monitoreo de Formación de Torta en Tiempo Real)

**Fecha**: Octubre 2026

# 1. Diagnóstico Forense de las Observaciones de Laboratorio (De V2/V4/V5 a V6)

Este capítulo documenta rigurosamente la causa raíz física, electrónica y algorítmica de cada uno de los problemas registrados durante las pruebas de laboratorio (documentados en `CALIBRACION DE CAUDALIMETRO DE ALIMENTACION.docx`, en el registro `PlantaUF_Calibracion_HistoricoCompleto (APP).csv` y en la planilla `PLANILLA_CALIBRACION_ENSAYOS_2_Y_3.xlsx`), junto con la solución definitiva implementada en la **Versión V6**.

## 1.1. ¿Por qué en versiones previas `K_Alim` (196.37) y `K_Perm` (687.33) dieron tan distintos siendo el mismo caudalímetro YF-S401?

En el archivo `PlantaUF_Calibracion_HistoricoCompleto (APP).csv` exportado durante sus ensayos previos, todas las filas registraron dos constantes drásticamente diferentes:

* `K_Alim = 196.37 Hz/(L/min)`

* `K_Perm = 687.33 Hz/(L/min)`

Como ambos sensores son exactamente el **mismo modelo constructivo (YF-S401)** con idéntica geometría de turbina e imán multipolar, sus factores $K$ físicos reales en régimen lineal deben ser prácticamente iguales ($\approx 205.98\text{ Hz/(L/min)}$). La divergencia a `687.33` se debió a la combinación de **dos factores**:

1. **Ruido electromagnético (EMI) ensanchado por el filtro RC pasivo en el pin de Permeado (`GPIO 27`)**:

   * Cada caudalímetro cuenta con una resistencia *pull-up* de $4.7\text{ k}\Omega$ a $3.3\text{ V}$ y un capacitor de $200\text{ nF}$ a `GND`. La constante de tiempo de carga de esta red RC es:
     

     $$
     \tau = R \cdot C = 4700\ \Omega \times 200 \times 10^{-9}\text{ F} = \mathbf{940\ \mu\text{s}}\ (0.94\text{ ms})
     $$

   * Cuando el motor paso a paso NEMA 34 conmuta corriente a través del driver DM860 (y dado que el Borne 11 de Permeado y el Borne 12 de Alimentación son contiguos), se inducen espigas negativas en la línea de permeado que descargan el capacitor de $200\text{ nF}$. Al recargarse a través de la resistencia de $4.7\text{ k}\Omega$, el pulso bajo dura entre $650\text{ y }1000\ \mu\text{s}$.

   * En las versiones previas, el umbral mínimo de pulso de permeado era de $600\ \mu\text{s}$, por lo que esos picos de ruido pasaban como si fueran giros reales de la turbina, generando una **frecuencia fantasma de entre** $15\text{ y }60\text{ Hz}$ (visible en la columna `Frec_PERMEADO_Hz` del CSV histórico).

2. **División automática sobre el valor precargado de `45.0 mL/min` en la interfaz web**:

   * En el código HTML anterior (`index_html.cpp`), el campo de entrada `cal_q_perm` tenía predefinido en duro el atributo `value="45.0"`.

   * Cuando se ejecutó el cálculo de calibración en el SCADA mientras el pin de permeado leía $\approx 30.93\text{ Hz}$ de ruido electromagnético sin flujo real de agua, el microcontrolador calculó:
     

     $$
     K_{\text{perm}} = \frac{F_{\text{perm (ruido)}} \times 1000}{Q_{\text{perm (valor por defecto HTML)}}} = \frac{30.93\text{ Hz} \times 1000}{45.0\text{ mL/min}} = \mathbf{687.33\ \frac{\text{Hz}}{\text{L/min}}}
     $$

   * Ese valor (`687.33`) se guardó automáticamente en la memoria Flash no volátil (`NVS`) del ESP32 y quedó fijo para todas las corridas siguientes.

**Solución en la Versión V6**:

* Se homologaron ambos sensores en **`K_ALIMENTACION = 205.98 Hz/(L/min)`** y **`K_PERMEADO = 205.98 Hz/(L/min)`** (valor experimental real obtenido en el Ensayo 3 con probeta graduada).

* Al encender el ESP32 con la V6, la bandera `VERSION_CALIBRACION = 6` actualiza de forma automática la memoria Flash NVS, borrando el `687.33` histórico.

* El campo `cal_q_perm` de la interfaz web ahora inicia en **`0.0`**, de modo que nunca se recalibra el permeado a menos que el operador ingrese explícitamente un caudal medido mayor a cero.

* Se elevó el filtro temporal de permeado a $3500\ \mu\text{s}$ **(**$3.5\text{ ms} \gg 940\ \mu\text{s}$**)** y se añadió validación de **simetría de onda cuadrada** y **racha de pulsos consecutivos**, erradicando el caudal fantasma.

## 1.2. Análisis de las Observaciones del Documento Word (`CALIBRACION DE CAUDALIMETRO DE ALIMENTACION.docx`)

| Observación registrada en el Word de Laboratorio | Causa Física / Algorítmica Exacta | Solución Implementada en Firmware V6 y Recomendación de Banco | 
| ----- | ----- | ----- | 
| **1. "Con 20 RPM está mal el permeado porque en la app nos indica que hay un caudal y en realidad nos debería dar 0"** | Acoplamiento capacitivo e inductivo (EMI) sobre el capacitor de $200\text{ nF}$ ($\tau = 940\ \mu\text{s}$) que superaba el filtro viejo de $600\ \mu\text{s}$, sumado a la ausencia de verificación de ciclo de trabajo (50% HIGH / 50% LOW). | **Resuelto en V6**: Triple barrera en `Caudalimetro::isr()` (`lowMin_us = 3500 us`, simetría de semiciclos `HIGH`/`LOW` y exigencia de $\ge 3$ pulsos coherentes seguidos) + botón **`🔒 Solo Alim`** en pantalla para bloquear el canal en ensayos exclusivos de alimentación. | 
| **2. "No toma el registro completo desde los 20 RPM, sino que empezó a tomar desde los 60–70 RPM. Toma muestras cada 1 segundo y debería tomar cada 10 segundos"** | En el CSV adjunto se observa que el registro va desde $t = 1306\text{ s}$ ($21:46\text{ min}$) hasta $t = 1920\text{ s}$ ($32:00\text{ min}$). Como guardaba 1 muestra cada $1\text{ s}$ en un buffer circular de $600$ filas, a los $10\text{ minutos}$ ($600\text{ s}$) el buffer se llenó y **sobrescribió** los primeros $21\text{ minutos}$ de ensayo (perdiendo $20, 30, 40, 50\text{ y }60\text{ RPM}$). | **Resuelto en V6**: El datalogger registra estrictamente **cada 10 segundos** (`INTERVALO_LOG_MS = 10000`), toma obligatoriamente una muestra inicial en $t = 0\text{ s}$ apenas arranca la bomba, y el buffer circular se amplió a **`MAX_REGISTROS = 600`**, brindando **100 minutos continuos (**$6000\text{ s}$**)** sin perder ni pisar ningún dato desde $20\text{ RPM}$. | 
| **3. "Al mover la manguera varía mucho el caudal de alimentación en la aplicación. También se observan variaciones porque tiene pulsos"** | El cabezal MBP-2000 tiene 3 rodillos a $120^\circ$ que generan un flujo pulsátil ($f_{\text{rizo}} = \text{RPM}/20$). Además, al mover la manguera cambia la altura manométrica y se desprenden microburbujas que provocan caídas de frecuencia de 1 segundo (ej. de $205\text{ Hz}$ a $126\text{ Hz}$ en el CSV). | **Resuelto en V6**: Se incorporó en `caudalimetro.cpp` un **Filtro de Mediana Deslizante de 3 ventanas (`mediana3`)** previo al filtro exponencial EMA ($\alpha = 0.30$). Cualquier perturbación brusca de 1 segundo al mover la manguera o pasar una burbuja es eliminada al instante por la mediana sin afectar la lectura de la merma real por formación de torta. | 
| **4. "A 30 y 40 RPM la bomba hace un ruido muy fuerte; a 60 RPM hace un ruido raro fuerte y vuelve a bajar; a 70 y 80 RPM está estable y con ruido uniforme"** | **Resonancia paramétrica del motor paso a paso NEMA 34**: Un motor de $1.8^\circ$ ($200\text{ pasos enteros/rev}$) tiene su frecuencia natural de resonancia mecánica en $f_n \approx 100\text{ pasos/s}$ ($30\text{ RPM}$) y su armónico secundario en $200\text{ pasos/s}$ ($60\text{ RPM}$). Además, en `v2`, cada $30\text{ s}$ el ESP32 intentaba reconectarse al router Wi-Fi (`WiFi.begin`), generando micro-cortes en el tren de pulsos a 60 RPM. A $70\text{–}80\text{ RPM}$ el rotor supera la zona de resonancia por inercia. | **Resuelto en software V6 + Ajuste en driver DM860**: En V6 se eliminó el escaneo Wi-Fi STA periódico (trabaja en `WIFI_AP` puro con `WiFi.setSleep(false)` y periférico hardware LEDC continuo), suprimiendo el "ruido raro" esporádico. Para reducir la vibración mecánica a 30–40 RPM y el calentamiento, ver la guía de switches DIP del DM860 en la Sección 1.3. | 
| **5. "La bomba se calienta mucho al subir las RPM"** | Los drivers paso a paso inyectan corriente constante en las bobinas. A altas RPM se suman las pérdidas en el hierro por corrientes parásitas (*eddy currents*). Si la corriente pico seleccionada en los switches `SW1–SW3` del DM860 supera la nominal del motor, o si `SW4` (reducción de corriente en reposo) está desactivado, la temperatura sube rápidamente. | **Recomendación operativa en banco**: Activar `SW4 = ON` (reduce la corriente al $50\%$ cuando la bomba está detenida, enfriando el motor entre ensayos) y ajustar `SW1–SW3` a la corriente RMS nominal del NEMA 34 ($3.5\text{ A a }4.5\text{ A}$). Una temperatura de carcasa de $50\text{ a }65\ ^\circ\text{C}$ en marcha continua es normal en motores NEMA 34 clase B. | 

## 1.3. Guía Práctica de Verificación del Driver Leadshine DM860 (Ruido a 30 RPM y Temperatura)

Para documentar en la tesis el comportamiento electromecánico del accionamiento:

1. **Mitigación de calentamiento (`SW4`)**: En el frente del driver DM860, verificar que el microinterruptor **`SW4` esté en posición `ON` (Abajo)**. En `ON`, apenas presionan `PARAR` en el SCADA, el driver reduce automáticamente la corriente de retención al $50\%$ (reduciendo la disipación térmica $P = I^2 R$ al $25\%$ mientras el banco está en espera).

2. **Corte total de corriente en espera (opcional)**: En la versión V6, cuando se activa `paradaEmergencia()`, el pin `GPIO 23` (`PIN_ENA`) pasa a `HIGH`, desenergizando al $100\%$ las bobinas del motor para que se enfríe por completo.

3. **Evasión de la frecuencia crítica de 30 RPM**: En los ensayos de filtración de larga duración, se recomienda operar en los puntos de régimen libre de resonancia ($20\text{ RPM}$**,** $45\text{–}50\text{ RPM}$ **o** $70\text{–}85\text{ RPM}$), donde el par electromagnético es continuo y el rizo acústico es mínimo.

# 2. Resumen de Mejoras de Ingeniería en la Versión V6

| \# | Archivo / Módulo | Mejora Incorporada en V6 | Impacto Directo en los Ensayos de la Tesis | 
| ----- | ----- | ----- | ----- | 
| **1** | `config.h` | **Constantes Calibradas Oficiales Ensayos 2 y 3**: `ML_POR_VUELTA = 13.5496f` ($\text{mL/rev}$) y `K_ALIMENTACION = K_PERMEADO = 205.98f` ($\text{Hz/(L/min)}$). | Elimina el sesgo del $20\%$ que producía usar la pendiente de regresión sin su ordenada al origen, logrando un error de apenas $+1.1\%$ en el punto nominal de $50\text{ RPM}$. | 
| **2** | `config.h` | **Ampliación del Buffer Datalogger a 100 Minutos**: `INTERVALO_LOG_MS = 10000` ($10\text{ s}$) y `MAX_REGISTROS = 600`. | Permite registrar corridas continuas de hasta **1 hora y 40 minutos** desde $t = 0\text{ s}$ sin que se sobrescriban los primeros escalones de $20\text{ a }50\text{ RPM}$. | 
| **3** | `caudalimetro.cpp` | **Triple Barrera Anti-Caudal Fantasma en `isr()`**: Ancho mínimo $> \tau_{\text{RC}}$ ($3500\ \mu\text{s}$ en permeado), simetría de onda cuadrada ($1:6$) y racha mínima de $3$ pulsos coherentes. | Erradica las lecturas falsas de permeado cuando solo circula agua por alimentación o cuando la bomba vibra en la mesa. | 
| **4** | `caudalimetro.cpp` | **Filtro Híbrido Mediana de 3 Puntos + EMA**: `mediana3(_qHist[0], _qHist[1], _qHist[2])` seguido de suavizado exponencial ($\alpha = 0.30$). | Suprime los saltos bruscos de caudal al mover la manguera o al pasar los 3 rodillos del cabezal peristáltico, manteniendo sensibilidad real ante la colmatación del filtro. | 
| **5** | `EN_USO_firmware_planta.ino` | **Monitoreo en Tiempo Real de Formación de Torta**: Cálculo continuo de $Q_{\text{alim},0}$, $Q_{\text{perm},0}$, `% Merma Alimentación`, `% Merma Permeado` y Flujo Normalizado $J/J_0$ ($\%$). | Permite observar en tiempo real la pérdida de caudal a medida que se deposita la torta sobre la membrana FX100, sin necesidad de medir constantemente con probeta. | 
| **6** | `EN_USO_firmware_planta.ino` + `index_html.cpp` | **Modo Híbrido para Bajo Caudal de Permeado**: Lectura automática por sensor YF-S401 (desde $2.0\text{ mL/min}$) + carga asistida por probeta en vivo cuando el goteo cae debajo del umbral mecánico de giro de la turbina. | Resuelve la limitación física del sensor de turbina a muy bajos caudales sin interrumpir el ensayo ni perder la trazabilidad en el archivo `.CSV`. | 
| **7** | `index_html.cpp` | **Gráfico Dinámico en Tiempo Real (`<canvas>`)**: Trazado en vivo (100% offline) de las curvas $Q_{\text{alim}}(t)$ y $Q_{\text{perm}}(t)$ respecto a la referencia limpia $Q_0$. | Visualización inmediata de la cinética de ensuciamiento (*fouling*) desde el celular o notebook en el laboratorio. | 

# 3. Explicación Exhaustiva del Código Fuente V6 (Módulo por Módulo y Línea por Línea)

A continuación se detalla el funcionamiento interno de cada uno de los archivos que integran la **Versión V6**, explicado línea por línea para su inclusión y defensa en el documento de tesis.

## 3.1. Archivo `config.h` — Configuración Técnica, Pines y Constantes Metrológicas

### Bloque 1: Directivas de Preprocesador y Asignación de Pines (Líneas 1 a 48)

```
#pragma once
#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

constexpr uint8_t PIN_PUL                = 18;  // DM860 PUL+ (PUL- a GND común)
constexpr uint8_t PIN_DIR                = 19;  // DM860 DIR+ (DIR- a GND común)
constexpr uint8_t PIN_ENA                = 23;  // DM860 ENA+ (ENA- a GND: HIGH = Driver deshabilitado)
constexpr uint8_t PIN_LED_BOMBA          = 2;   // LED Azul Onboard (indicador de marcha)

constexpr uint8_t PIN_SENSOR_ALIMENTACION = 14;  // Sensor de Alimentación (Borne 12 Sup)
constexpr uint8_t PIN_SENSOR_PERMEADO     = 27;  // Sensor de Permeado (Borne 11 Sup, Pull-up externo a 3.3V)

constexpr uint8_t PIN_I2C_SDA             = 21;  // ESP32 SDA
constexpr uint8_t PIN_I2C_SCL             = 22;  // ESP32 SCL

```

* **`#pragma once`**: Garantiza que el archivo de cabecera se compile una única vez por unidad de traducción, previniendo errores de símbolos duplicados.

* **`constexpr uint8_t PIN_...`**: Define constantes en tiempo de compilación sin ocupar memoria RAM dinámica:

  * **`PIN_PUL (GPIO 18)`**: Conecta al terminal `PUL+` del driver Leadshine DM860. El periférico hardware LEDC del ESP32 genera aquí el tren de ondas cuadradas cuya frecuencia determina la velocidad de giro del motor NEMA 34.

  * **`PIN_DIR (GPIO 19)`**: Conecta al terminal `DIR+` del DM860 (`LOW` = sentido horario / filtración; `HIGH` = sentido antihorario / retrolavado).

  * **`PIN_ENA (GPIO 23)`**: Conecta al terminal `ENA+` del DM860. En operación normal permanece en `LOW` (driver habilitado). Ante una parada de emergencia pasa a `HIGH`, abriendo los optoacopladores de potencia en menos de $1\text{ ms}$.

  * **`PIN_SENSOR_ALIMENTACION (GPIO 14)` y `PIN_SENSOR_PERMEADO (GPIO 27)`**: Pines con capacidad de interrupción externa por hardware en ambos flancos (`CHANGE`) asignados a los dos caudalímetros de efecto Hall YF-S401.

  * **`PIN_I2C_SDA (GPIO 21)` y `PIN_I2C_SCL (GPIO 22)`**: Bus I2C nativo del ESP32 reservado para el conversor analógico-digital de 16 bits **ADS1115** (lectura de transductores de presión y sonda TDS en el Subhito 2.3).

```
constexpr uint8_t PINES_SISTEMA[] = {
  PIN_PUL, PIN_DIR, PIN_ENA, PIN_LED_BOMBA,
  PIN_SENSOR_ALIMENTACION, PIN_SENSOR_PERMEADO,
  PIN_I2C_SDA, PIN_I2C_SCL
};

constexpr bool pinesUnicos() {
  for (size_t i = 0; i < sizeof(PINES_SISTEMA) / sizeof(PINES_SISTEMA[0]); ++i) {
    for (size_t j = i + 1; j < sizeof(PINES_SISTEMA) / sizeof(PINES_SISTEMA[0]); ++j) {
      if (PINES_SISTEMA[i] == PINES_SISTEMA[j]) return false;
    }
  }
  return true;
}
static_assert(pinesUnicos(), "ERROR: Hay pines GPIO compartidos en el sistema");

```

* **`pinesUnicos()` y `static_assert`**: Función evaluada íntegramente por el compilador antes de generar el binario. Compara todos los pares posibles de pines $(i, j)$; si algún periférico compartiera por error un mismo pin GPIO, la compilación se aborta automáticamente informando el conflicto.

### Bloque 2: Parámetros Cinemáticos Calibrados de la Bomba (Líneas 50 a 65)

```
constexpr uint16_t PULSOS_POR_REV = 3200;     // DM860 configurado a 1/16 micropasos (3200 pulsos/rev)
constexpr float ML_POR_VUELTA     = 13.5496f; // Calibración Ensayo 2 (20 a 90 RPM): Media = 13.55 mL/rev (CV = 8.12%)

constexpr float RPM_MIN           = 15.0f;    // ~203 mL/min
constexpr float RPM_MAX           = 100.0f;   // ~1355 mL/min (Habilitado para diseño factorial experimental en agua)
constexpr float RPM_INICIO        = 25.0f;    // Consigna de arranque suave (~339 mL/min)
constexpr float RPM_ALARMA_MEMBRANA = 44.0f;  // 44.28 RPM = 600 mL/min (Umbral clínico hemodiálisis / referencia)

constexpr float ACEL_ARRANQUE_RPM_S  = 2.0f;   // 2.0 RPM/s despegue inicial suave y confiable
constexpr float ACEL_NOMINAL_RPM_S   = 3.5f;   // 3.5 RPM/s aceleración progresiva
constexpr float DESACEL_AJUSTE_RPM_S = 8.0f;   // 8.0 RPM/s desaceleración en marcha
constexpr float FRENADO_PARADA_RPM_S = 70.0f;  // 70.0 RPM/s frenado rápido garantizado (< 1.43s desde 100 RPM)

```

* **`PULSOS_POR_REV = 3200`**: El motor NEMA 34 tiene $200\text{ pasos/vuelta}$ ($1.8^\circ/\text{paso}$); con el driver DM860 configurado a $1/16$ de micropaso, requiere $200 \times 16 = 3200\text{ pulsos}$ para completar una revolución exacta del cabezal peristáltico.

* **`ML_POR_VUELTA = 13.5496f`**: Promedio aritmético experimental determinado en el **Ensayo 2** con probeta graduada de $1000\text{ mL}$ a lo largo de las 8 corridas ($20\text{ a }90\text{ RPM}$). Permite estimar el caudal volumétrico ideal sin contrapresión como $Q_{\text{bomba}} = \text{RPM} \times 13.5496\ [\text{mL/min}]$.

* **Tasas de rampa (`ACEL_...` y `FRENADO_...`)**: Fijan las derivadas temporales $\frac{d(\text{RPM})}{dt}$ para proteger mecánicamente la manguera de silicona y evitar golpes de ariete sobre los capilares de la membrana FX100 en el arranque ($2.0\text{ a }3.5\text{ RPM/s}$), asegurando a la vez una detención rápida en menos de $1.43\text{ s}$ al presionar `STOP` ($70.0\text{ RPM/s}$).

### Bloque 3: Especificaciones de la Membrana y Configuración Anti-EMI de Sensores (Líneas 67 a 132)

```
constexpr float AREA_MEMBRANA_M2       = 2.2f;    // Superficie interfacial efectiva (m²)
constexpr float K_UF_NOMINAL           = 73.0f;   // mL / (h * mmHg)
constexpr float DIAMETRO_CAPILAR_UM    = 185.0f;  // Diámetro interno capilar (μm)
constexpr float ESPESOR_PARED_UM       = 35.0f;   // Grosor de pared capilar (μm)
constexpr float TMP_MAX_SEGURA_BAR     = 0.50f;   // Límite máximo admisible de TMP (bar)
constexpr float P1_MAX_SEGURA_BAR      = 0.60f;   // Límite máximo de presión en entrada de cartucho (bar)
constexpr float Q_CLINICO_SANGRE_MAX   = 600.0f;  // mL/min (límite en hemodiálisis clínica)

```

* Define las constantes físicas del dializador capilar **Fresenius FX100 Helixone®** ($A = 2.2\text{ m}^2$) utilizadas por `darcy.h` y por el bucle principal para convertir el caudal de permeado en flujo específico superficial $J\ [\text{L/(m}^2\cdot\text{h)} = \text{LMH}]$.

```
struct ConfigSensor {
  uint8_t  pin;
  double   k_Hz_por_Lmin;
  uint32_t lowMin_us;
  uint32_t highMin_us;
  uint32_t periodoMin_us;
  uint32_t timeout_us;
  double   qMaxOperativo_mLmin;
  double   qMinDetectable_mLmin;
  bool     calibracionDocumentada;
  double   qMinCal_mLmin;
  double   qMaxCal_mLmin;
};

```

* Estructura de parametrización metrológica individual de cada caudalímetro:

  * **`SENSOR_ALIM_CFG` (Pin 14)**: Configurado con `k_Hz_por_Lmin = 205.98`, `lowMin_us = 1200`, `highMin_us = 1200` y `periodoMin_us = 2800` (hasta $357\text{ Hz} \approx 1733\text{ mL/min}$). Como $1200\ \mu\text{s} > 940\ \mu\text{s}$ ($\tau_{\text{RC}}$), rechaza el ruido de conmutación.

  * **`SENSOR_PERM_CFG` (Pin 27)**: Configurado con el mismo factor físico `k_Hz_por_Lmin = 205.98`, pero con **`lowMin_us = 3500` (**$3.5\text{ ms}$**) y `highMin_us = 3500` (**$3.5\text{ ms}$**)**, `periodoMin_us = 8000` ($125\text{ Hz}$) y `timeout_us = 3500000` ($3.5\text{ s}$). Esto bloquea por completo cualquier espiga del filtro RC ($0.94\text{ ms}$) mientras permite medir frecuencias reales tan bajas como $0.35\text{ Hz}$ ($\approx 2.0\text{ mL/min}$).

### Bloque 4: Parámetros del Datalogger de 100 Minutos y Red Wi-Fi (Líneas 134 a 150)

```
constexpr float    K_ALIMENTACION       = 205.98f;
constexpr float    K_PERMEADO           = 205.98f;
constexpr uint8_t  VERSION_CALIBRACION  = 6;

constexpr uint32_t INTERVALO_LOG_MS   = 10000;  // Muestreo cada 10 segundos exactos
constexpr size_t   MAX_REGISTROS      = 600;    // 600 muestras en buffer circular (100 minutos continuos)
constexpr size_t   MAX_ENSAYOS        = 20;     // Hasta 20 ensayos/sesiones independientes
constexpr uint8_t  MUESTRAS_AUTO_CAL  = 15;     // 15 muestras (15 s) para auto-calibración en régimen permanente

```

* **`VERSION_CALIBRACION = 6`**: Identificador de versión almacenado en la memoria Flash NVS. Al detectar que cambió a `6`, el ESP32 actualiza automáticamente los factores guardados en Flash a `205.98` y `13.5496`.

* **`INTERVALO_LOG_MS = 10000` y `MAX_REGISTROS = 600`**: Soluciona definitivamente el problema observado en el Word donde se perdían las muestras de 20 a 60 RPM. Ahora toma 1 muestra cada $10\text{ segundos}$ y almacena hasta $600\text{ registros}$ ($600 \times 10\text{ s} = 6000\text{ s} = \mathbf{100\text{ minutos continuos}}$).

## 3.2. Archivo `caudalimetro.h` — Declaración de la Clase Caudalímetro

```
class Caudalimetro {
public:
  enum Calidad : uint16_t {
    INICIADO                = (1U << 0),
    CAL_DOCUMENTADA         = (1U << 1),
    PERIODO_DISPONIBLE      = (1U << 2),
    SIN_SENAL               = (1U << 3),
    FUERA_RANGO_CAL         = (1U << 4),
    FUERA_RANGO_OPERATIVO   = (1U << 5),
    SECO                    = (1U << 6),
    RUIDO_SECO              = (1U << 7),
    SIN_IMPULSION           = (1U << 8),
    MEDICION_EN_RANGO_CAL   = (1U << 9)
  };

```

* **`enum Calidad`**: Máscara de bits de 16 bits que codifica en cada muestra el estado metrológico exacto del sensor (si está iniciado, si hay período válido disponible, si está en condición de `SIN_SENAL`, si está dentro del rango calibrado de probeta, o si detectó ruido durante una prueba en seco).

```
  struct Muestra {
    int64_t  t_us = 0;
    int64_t  ultimoAceptado_us = -1;
    int64_t  edadPulso_us = -1;
    uint64_t flancos = 0;
    uint64_t aceptados = 0;
    uint64_t pulsosMedicion = 0;
    uint64_t rechazados = 0;
    uint32_t epoca = 0;
    uint16_t calidad = 0;
    double   k = 0.0;
    double   frecuencia_Hz = 0.0;
    double   q_mLmin = 0.0;
    double   volumenEstimado_L = 0.0;
  };

```

* **`struct Muestra`**: Fotografía (instantánea) inmutable y atómica del estado del sensor tomada exactamente una vez por segundo por el método `capturar()`. Usa enteros de 64 bits (`int64_t` y `uint64_t`) con el reloj de hardware de alta resolución `esp_timer_get_time()` (que desborda recién a los $292,000\text{ años}$, superando incluso el límite de $71.58\text{ minutos}$ de `micros()`).

* **Atributos privados de sincronización (`_mux`, variables `volatile` y `_qHist[3]`)**:

  * `portMUX_TYPE _mux`: Cerrojo (*spinlock*) de FreeRTOS que protege las variables modificadas por la interrupción `isr()` frente a lecturas simultáneas desde el bucle principal.

  * `float _qHist[3]` y `uint8_t _qHistCount`: Buffer de las últimas 3 lecturas de caudal instantáneo para aplicar el **filtro de mediana de 3 puntos (`mediana3`)** antes del suavizado exponencial.

## 3.3. Archivo `caudalimetro.cpp` — Algoritmo Anti-EMI, Período Recíproco y Filtro de Mediana

### Bloque 1: Función de Mediana de 3 Puntos y Reinicio de Estimador (Líneas 11 a 34)

```
float Caudalimetro::mediana3(float a, float b, float c) {
  if ((a >= b && a <= c) || (a <= b && a >= c)) return a;
  if ((b >= a && b <= c) || (b <= a && b >= c)) return b;
  return c;
}

```

* **`mediana3(a, b, c)`**: Devuelve el valor central entre las últimas 3 mediciones de 1 segundo. Si por mover la manguera o pasar una burbuja de aire la lectura de 1 segundo cae momentáneamente (ej. `720 -> 490 -> 718 mL/min`), la mediana devuelve **`718 mL/min`**, rechazando el pozo espurio sin retrasar la respuesta ante cambios reales sostenidos.

### Bloque 2: Configuración de Factor $K$, Tara de Volumen y Bloqueo de Canal (Líneas 36 a 112)

```
void Caudalimetro::setBloqueado(bool bloqueado) {
  portENTER_CRITICAL(&_mux);
  if (_bloqueado != bloqueado) {
    _bloqueado = bloqueado;
    reiniciarEstimadorLocked();
    if (_iniciado) {
      _nivelAlto = (gpio_get_level((gpio_num_t)_cfg.pin) != 0);
      _inicioNivel_us = esp_timer_get_time();
    }
  }
  portEXIT_CRITICAL(&_mux);
  if (bloqueado) {
    _qHist[0] = _qHist[1] = _qHist[2] = 0.0f;
    _qHistCount = 0;
    _qSuavizadoDisplay = 0.0f;
  }
}

```

* **`setBloqueado(bool)`**: Implementa el modo **`🔒 Solo Alim`** del SCADA. Cuando el usuario activa este botón durante un ensayo exclusivo de alimentación, la variable `_bloqueado` pasa a `true` dentro de una sección crítica, limpia los acumuladores de la ISR y fuerza inmediatamente la salida a `0.0 mL/min`.

### Bloque 3: Rutina de Interrupción en Silicio `Caudalimetro::isr()` (Líneas 148 a 256)

Esta función se ejecuta en memoria IRAM cada vez que el pin GPIO cambia de estado lógico (`CHANGE`, tanto en flanco de bajada como de subida):

```
void ARDUINO_ISR_ATTR Caudalimetro::isr() {
  portENTER_CRITICAL_ISR(&_mux);
  const int64_t ahora = esp_timer_get_time();
  const bool alto = (gpio_get_level((gpio_num_t)_cfg.pin) != 0);

  if (_bloqueado) {
    _nivelAlto = alto;
    _inicioNivel_us = ahora;
    portEXIT_CRITICAL_ISR(&_mux);
    return;
  }

  ++_flancos;
  if (_seco) _alarmaSeco = true;

  if (_hayAnterior && (ahora - _bajadaAnterior_us > (int64_t)_cfg.timeout_us)) {
    _hayAnterior = false;
    _ultimoPeriodo_us = 0;
    _pulsosConsecutivos = 0;
  }

```

* **Caducidad automática por Timeout**: Si el tiempo transcurrido desde el último flanco de bajada supera `_cfg.timeout_us` ($1.5\text{ s}$ en alimentación, $3.5\text{ s}$ en permeado), se borra la memoria del pulso viejo (`_hayAnterior = false`, `_pulsosConsecutivos = 0`). Esto impide que dos chispazos de ruido aislados separados por varios segundos formen un período falso.

```
  const int64_t duracion = ahora - _inicioNivel_us;

  if (!alto) {
    // FLANCO DESCENDENTE: Termina HIGH, comienza LOW
    if (_altoConocido && (duracion >= (int64_t)_cfg.highMin_us)) {
      _candidato = true;
      _ultimaDuracionHigh_us = duracion;
      _bajada_us = ahora;
    } else {
      _candidato = false;
      ++_rechazados;
    }
  } else {
    // FLANCO ASCENDENTE: Termina LOW, comienza HIGH
    if (_candidato && (duracion >= (int64_t)_cfg.lowMin_us)) {
      const int64_t duracionLow = duracion;
      const int64_t duracionHigh = _ultimaDuracionHigh_us;

```

* **Barrera 1 (Filtro Geométrico de Ancho de Semiciclo)**:

  * Cuando el pin baja a `LOW` (`!alto`), verifica que haya permanecido previamente en `HIGH` durante al menos `highMin_us` ($1200\ \mu\text{s}$ en alimentación, $3500\ \mu\text{s}$ en permeado). Si fue más corto, lo descarta y suma 1 a `_rechazados` (*Ruido EMI filtrado*).

  * Cuando el pin vuelve a subir a `HIGH` (`alto`), verifica que el pulso `LOW` haya durado al menos `lowMin_us` ($3500\ \mu\text{s}$ en permeado). Como cualquier pico de ruido eléctrico sobre el capacitor de $200\text{ nF}$ dura $\approx 940\ \mu\text{s}$, **el** $100\%$ **de los picos rápidos de EMI muere en esta barrera**.

```
      // FILTRO 1: SIMETRÍA DE ONDA CUADRADA (Turbina Real vs Glitch EMI)
      bool simetrico = true;
      if (_hayAnterior) {
        if (duracionHigh > duracionLow * 6 || duracionLow > duracionHigh * 6) {
          simetrico = false;
        }
      }

```

* **Barrera 2 (Simetría de Ciclo de Trabajo)**:

  * El rotor magnético del YF-S401 tiene polos Norte y Sur simétricos, por lo que al girar con agua produce una onda cuadrada donde `duracionHigh` y `duracionLow` son similares ($\approx 50\% / 50\%$).

  * Si un pulso tiene una asimetría extrema ($> 6:1$, típico de una perturbación eléctrica o un falso contacto), `simetrico` da `false`, se incrementa `_rechazados` y se reinicia la cuenta de pulsos consecutivos.

```
          // FILTRO 2: COHERENCIA CINEMÁTICA ENTRE PERÍODOS CONSECUTIVOS
          bool coherente = true;
          if (_ultimoPeriodo_us > 0) {
            if (periodo * 10 < _ultimoPeriodo_us * 3 || periodo > _ultimoPeriodo_us * 3) {
              coherente = false;
            }
          }

          if (coherente) {
            ++_pulsosConsecutivos;
            _ultimoPeriodo_us = periodo;
            _bajadaAnterior_us = _bajada_us;
            _ultimoAceptado_us = ahora;

            if (_pulsosConsecutivos >= 2) {
              if (_pulsosConsecutivos == 2) {
                ++_aceptados;
                if (!_seco) ++_pulsosMedicion;
              }
              ++_aceptados;
              if (!_seco) ++_pulsosMedicion;
              ++_periodosVentana;
              _sumaPeriodosVentana_us += (uint64_t)periodo;
            }
          }

```

* **Barrera 3 (Coherencia Cinemática y Racha Mínima de 3 Pulsos / 2 Períodos)**:

  * Compara el nuevo período $T_k$ con el período anterior $T_{k-1}$; debido a la inercia rotacional de la turbina y del agua, el período no puede saltar instantáneamente más de un factor $3\times$ en un solo pulso.

  * Exige que se completen al menos **2 períodos coherentes seguidos (`_pulsosConsecutivos >= 2`, es decir, 3 pulsos válidos consecutivos)** antes de convalidar el flujo y sumar al contador volumétrico `_pulsosMedicion` (compensando el pulso inicial de arranque cuando `_pulsosConsecutivos == 2`). De este modo, si alguien golpea la mesa o mueve un cable generando 1 o 2 pulsos sueltos, el caudalímetro permanece clavado en `0.0 mL/min`.

### Bloque 4: Captura Atómica `Caudalimetro::capturar()` (Líneas 274 a 424)

Invocada exactamente una vez cada $1000\text{ ms}$ desde `loop()`:

1. **Copia atómica (Líneas 293 a 318)**: Extrae bajo `portENTER_CRITICAL(&_mux)` los contadores acumulados en el último segundo (`_periodosVentana`, `_sumaPeriodosVentana_us`, `_flancos`, `_aceptados`, `_rechazados`) y pone a cero los acumuladores de ventana para el segundo siguiente.

2. **Frecuencia por Período Recíproco Exacto (Líneas 345 a 365)**:

   * Si hubo períodos completos en el último segundo (`hayPeriodosNuevos`), calcula la frecuencia media exacta a partir de la suma de duraciones en microsegundos:
     

     $$
     F\ [\text{Hz}] = \frac{10^6 \times \text{periodos}}{\sum T_i\ [\mu\text{s}]}
     $$

   * Si el caudal de permeado es muy bajo ($0.35\text{ a }1.5\text{ Hz}$, donde un pulso tarda más de $1\text{ s}$ en completarse), la condición `mantenerBajaFrecuencia` conserva la estimación $10^6 / T_{\text{último}}$ hasta por $2 \times T_{\text{último}}$, evitando que la lectura parpadee a cero entre gota y gota.

3. **Totalización Volumétrica Exacta (Líneas 384 a 386)**:
   

   $$
   V\ [\text{Litros}] = \frac{\text{pulsosNetos}}{60.0 \times K}
   $$

4. **Filtro Mediana + EMA (Líneas 400 a 415)**: Aplica `mediana3` sobre los últimos 3 segundos y luego el filtro exponencial $\hat{Q}_k = 0.30 \cdot Q_{\text{mediana}} + 0.70 \cdot \hat{Q}_{k-1}$.

## 3.4. Archivos `Bomba.h` y `Bomba.cpp` — Control Cinemático del Motor NEMA 34

* **`Bomba::begin()`**: Inicializa los pines `PIN_DIR` (GPIO 19) y `PIN_ENA` (GPIO 23, activo en `LOW`), y adjunta el periférico generador de pulsos por hardware LEDC al pin `PIN_PUL` (GPIO 18) con resolución de 10 bits ($0\text{ a }1023$).

* **`Bomba::paradaEmergencia()`**: Corta instantáneamente el tren de pulsos (`ledcWrite(..., 0)`) y pone `digitalWrite(PIN_ENA, HIGH)`, abriendo eléctricamente la etapa de salida del driver DM860 en menos de $1\text{ ms}$.

* **`Bomba::tick(float dt)`**: Ejecutado cada $50\text{ ms}$ ($20\text{ Hz}$):

  1. Calcula la velocidad instantánea `_actual` acercándose suavemente a `_objetivo` según la rampa progresiva de aceleración (`ACEL_ARRANQUE_RPM_S = 2.0 RPM/s` hasta `ACEL_NOMINAL_RPM_S = 3.5 RPM/s`) o desaceleración (`FRENADO_PARADA_RPM_S = 70.0 RPM/s` al apagar).

  2. Si se solicitó inversión de giro (`_invirtiendo`), desacelera primero hasta $0.0\text{ RPM}$, conmuta el pin físico `PIN_DIR` sin carga cinemática, y vuelve a acelerar suavemente en sentido opuesto.

  3. Convierte las RPM instantáneas en frecuencia de pasos:
     

     $$
     f_{\text{LEDC}}\ [\text{Hz}] = \frac{\text{RPM}_{\text{actual}} \times 3200}{60}
     $$

     
     y actualiza la frecuencia del temporizador por hardware mediante `ledcChangeFrequency(PIN_PUL, f, 10)` manteniendo un ciclo de trabajo simétrico del $50\%$ (`duty = 512`).

## 3.5. Archivo `darcy.h` — Modelo de Resistencias en Serie y Corrección Térmica de Vogel

* **`ModeloDarcy::viscosidadAgua(float temp_C)`**: Calcula la viscosidad dinámica del agua $\mu(T)$ en $\text{Pa}\cdot\text{s}$ mediante la fórmula de Vogel:
  

  $$
  \mu(T) = 2.414 \times 10^{-5} \times 10^{\frac{247.8}{T\ [\text{K}] - 140.0}}
  $$

* **`ModeloDarcy::calcular(...)`**:

  1. Calcula el Factor de Corrección por Temperatura $\text{TCF} = \mu(T) / \mu_{20}$ (con $\mu_{20} = 1.002\times 10^{-3}\text{ Pa}\cdot\text{s}$).

  2. Calcula el flujo tangencial de permeado $J = (Q_{\text{perm}} \times 0.06) / 2.2\ [\text{LMH}]$ y el flujo estandarizado a $20\ ^\circ\text{C}$ ($J_{20} = J \times \text{TCF}$).

  3. Cuando se dispone de lectura de presión transmembrana ($\text{TMP}$ en bar, Subhito 2.3), despeja la resistencia hidráulica total $R_{\text{total}} = \frac{\text{TMP}\ [\text{Pa}]}{\mu \cdot J\ [\text{m/s}]}$ y la resistencia de la torta $R_{\text{adicional}} = R_{\text{total}} - R_m$ (donde $R_m = 2.09 \times 10^{13}\text{ m}^{-1}$ es la resistencia intrínseca nominal de la membrana FX100 según ISO 8637).

## 3.6. Archivo `registro_ensayos.h` — Registro de Auditoría Metrológica

* Administra un registro complementario de eventos (`INICIO`, `PERIODICA`, `FIN`) exportable desde el botón **`🔬 Descargar Registro Metrológico Completo (29 cols)`**.

* **Corrección clave en V6**: Los métodos `iniciar()` y `finalizar()` leen `a.ultimaMuestra()` y `p.ultimaMuestra()` en lugar de invocar a `capturar()`, garantizando que `capturar()` sea llamado únicamente por el reloj maestro de $1\text{ segundo}$ en `loop()`.

## 3.7. Archivo `EN_USO_firmware_planta.ino` — Orquestador Principal, Monitoreo de Torta y Servidor Web

### Bloque 1: Estructuras del Datalogger y Variables de Formación de Torta (Líneas 28 a 95)

* **`struct RegistroCalibracion`**: Define las 24 variables que se almacenan cada $10\text{ segundos}$ en el buffer circular `bufferLog[600]` (capacidad de $100\text{ minutos}$). Además de las frecuencias, caudales y volúmenes, guarda:

  * `q_alim_ref` y `q_perm_ref`: Caudales iniciales con el filtro limpio ($Q_{\text{alim},0}$ y $Q_{\text{perm},0}$).

  * `merma_alim_pct`: Porcentaje de caída del caudal de alimentación debido al aumento de contrapresión por la torta.

  * `merma_perm_pct` y `j_sobre_j0_pct`: Porcentaje de caída del caudal de permeado ($1 - J/J_0$) y flujo relativo normalizado $J(t)/J_0\ [\%]$.

  * `perm_manual`: Indica si el dato de permeado provino del sensor Hall en tiempo real (`false`) o de una lectura asistida por probeta a muy bajo caudal (`true`).

### Bloque 2: Persistencia NVS con Auto-Actualización V6 (Líneas 118 a 148)

```
void cargarParametrosNVS() {
  prefs.begin("planta_uf", false);
  uint8_t ver = prefs.getUChar("cal_ver", 0);
  float ka = prefs.getFloat("k_alim", K_ALIMENTACION);
  float kp = prefs.getFloat("k_perm", K_PERMEADO);
  float ml = prefs.getFloat("ml_rev", ML_POR_VUELTA);
  uint16_t pul = prefs.getUShort("pul_rev", PULSOS_POR_REV);

  bool actualizarNvs = (ver != VERSION_CALIBRACION);
  ...
  if (actualizarNvs) {
    Serial.println("[NVS] Aplicando calibracion oficial Ensayos 2 y 3 (K=205.98 Hz/(L/min), Cilindrada=13.55 mL/rev)...");
    prefs.putUChar("cal_ver", VERSION_CALIBRACION);
    guardarParametrosNVS(ka, kp, ml, pul);
  }

```

* Al arrancar por primera vez tras compilar la **V6**, `ver` es distinto de `VERSION_CALIBRACION (6)`, por lo que el ESP32 graba automáticamente en la memoria Flash permanente los valores calibrados `K_Alim = 205.98`, `K_Perm = 205.98` y `Cilindrada = 13.5496`, sin que el usuario tenga que borrar manualmente la memoria.

### Bloque 3: Gestión de Sesiones y Fijación de Referencia Limpia $Q_0$ (Líneas 150 a 255)

* **`guardarMuestraDatalogger()`**: Inserta una fila completa en el buffer circular de 600 muestras e imprime el resumen en el Monitor Serie.

* **`iniciarNuevoEnsayo()`**: Se dispara automáticamente en el instante en que la bomba arranca (o cuando se cambia la consigna de RPM en más de $1.5\text{ RPM}$ luego de $5\text{ s}$). Reinicia el reloj relativo del ensayo a $t = 0\text{ s}$, reinicia las referencias de torta y **guarda inmediatamente la muestra inicial en** $t = 0\text{ s}$.

* **`fijarReferenciasTortaActuales()`**: Permite fijar manualmente con el botón **`📌 Fijar Q₀ Actual`** los caudales iniciales $Q_{\text{alim},0}$ y $Q_{\text{perm},0}$ cuando el filtro está limpio antes de comenzar la etapa de ensuciamiento.

### Bloque 4: Endpoints HTTP del Servidor Web y Exportación `.CSV` (Líneas 260 a 555)

* **`handleStatus()`**: Empaqueta en JSON de alta velocidad (usando `snprintf` en un buffer estático de pila sin fragmentar la memoria RAM) todas las variables de tiempo real cada $500\text{ ms}$, incluyendo las métricas de merma de torta y los contadores de pulsos válidos vs. ruido EMI rechazado.

* **`handleSetPermManual()`**: Recibe el caudal de permeado medido con probeta cuando el flujo cae por debajo del umbral mecánico de giro de la turbina YF-S401, integrándolo en tiempo real al cálculo de Darcy, al porcentaje de merma y al archivo `.CSV`.

* **`handleSetDev()` y `handleCalibrarRpmQ()`**: Permiten ajustar factores $K$ y cilindrada en caliente (con la bomba en marcha o detenida) y guardarlos en Flash NVS sin bloqueos `HTTP 409`.

* **`handleExportCSV()`**: Genera el archivo Excel `.CSV` incluyendo la marca `BOM UTF-8` (`\xEF\xBB\xBF`) y el separador `sep=;`, con las 24 columnas de proceso y ensuciamiento de membrana.

### Bloque 5: Inicialización `setup()` y Bucle Maestro `loop()` (Líneas 560 a 745)

* **`setup()`**: Inicializa los pines, arranca los caudalímetros y la bomba, actualiza la NVS a la versión V6, levanta el punto de acceso Wi-Fi dedicado `Bomba_Peristaltica_UF` en el canal 1 con potencia máxima ($19.5\text{ dBm}$) y ahorro de energía desactivado (`WiFi.setSleep(false)`), e inicia el servidor DNS de portal cautivo en el puerto 53 para evitar desconexiones automáticas en Android/iOS/Windows.

* **`loop()`**: Ejecuta tres tareas temporizadas no bloqueantes:

  1. **Cada** $50\text{ ms}$: Actualiza la rampa cinemática `bomba.tick(dt)` y detecta el encendido/apagado para abrir o cerrar sesiones de ensayo.

  2. **Cada** $1000\text{ ms}$ **(**$1\text{ s}$**)**:

     * Llama una única vez a `sensorAlimentacion.capturar()` y `sensorPermeado.capturar()`.

     * Cuando la bomba cumple $5\text{ segundos}$ continuos en régimen estable (`segEstablesRef >= 5`), captura automáticamente los caudales iniciales de referencia con filtro limpio (`qAlimRef_mLmin` y `qPermRef_mLmin`).

     * Calcula en tiempo real la **Merma de Alimentación** y la **Merma de Permeado** a medida que se forma la torta:
       

       $$
       \text{Merma}_{\text{alim}}\ [\%] = \max\left(0,\ \frac{Q_{\text{alim},0} - Q_{\text{alim}}(t)}{Q_{\text{alim},0}} \times 100\right)
       $$

       $$
       \text{Merma}_{\text{perm}}\ [\%] = \max\left(0,\ \frac{Q_{\text{perm},0} - Q_{\text{perm}}(t)}{Q_{\text{perm},0}} \times 100\right), \quad \frac{J(t)}{J_0}\ [\%] = \frac{Q_{\text{perm}}(t)}{Q_{\text{perm},0}} \times 100
       $$

  3. **Cada** $10,000\text{ ms}$ **(**$10\text{ s}$**)**: Si la bomba está en marcha, llama a `guardarMuestraDatalogger()`, almacenando la evolución completa del ensayo durante hasta $100\text{ minutos}$.

## 3.8. Archivos `index_html.h` e `index_html.cpp` — Interfaz Web SCADA V6

Almacenada en memoria Flash (`PROGMEM`), estructura el panel de control en 4 tarjetas operativas:

1. **Tarjeta 1 (Control de Bomba Peristáltica)**: Velocímetro digital en tiempo real, indicador de estado (`DETENIDA`, `EN MARCHA`, `INVIRTIENDO`), deslizador de $15\text{ a }100\text{ RPM}$, botonera directa para los puntos de ensayo ($20, 30, 40, 50, 70, 90\text{ RPM}$) y controles de arranque, parada e inversión de sentido.

2. **Tarjeta 2 (Caudales en Tiempo Real y Monitoreo de Formación de Torta)**:

   * Muestra el caudal en tiempo real ($\text{mL/min}$), volumen acumulado ($\text{L}$), frecuencia ($\text{Hz}$), caudal inicial de referencia ($Q_0$) y el diagnóstico de **Pulsos válidos vs. Ruido EMI filtrado** en cada sensor.

   * Incluye el botón **`🔒 Solo Alim`** para ensayos sin permeado y el cuadro de **Asistencia por Probeta** para cuando el caudal de permeado es tan bajo que no alcanza a girar la turbina.

   * Contiene el panel **`📉 MERMA DE CAUDAL POR FORMACIÓN DE TORTA (TIEMPO REAL)`** con el botón **`📌 Fijar Q₀ Actual (Filtro Limpio)`**, los indicadores porcentuales de caída de caudal y el **gráfico dinámico HTML5 `<canvas>`** que dibuja en vivo las curvas $Q_{\text{alim}}(t)$ y $Q_{\text{perm}}(t)$.

3. **Tarjeta 3 (Parámetros de Calibración / Factores K)**: Permite verificar y ajustar `K_Alim`, `K_Perm`, `Cilindrada (mL/rev)` y `Pulsos/Rev`, guardándolos de forma permanente en la memoria Flash NVS.

4. **Tarjeta 4 (Registro de Merma de Caudal y Exportación a Excel `.CSV`)**: Muestra la tabla resumen de corridas con la merma final de alimentación y permeado de cada ensayo y permite descargar el archivo `.CSV` listo para abrir en Microsoft Excel.

# 4. Protocolo Operativo de Laboratorio con la Versión V6

1. **Compilación y Carga**:

   * Descomprimir `EN_USO_firmware_planta_UNSa_V6.zip`, abrir `EN_USO_firmware_planta.ino` en Arduino IDE y subirlo al ESP32.

   * Al iniciar, el ESP32 grabará automáticamente en su memoria Flash `K_Alim = 205.98 Hz/(L/min)`, `K_Perm = 205.98 Hz/(L/min)` y `Cilindrada = 13.55 mL/rev`.

2. **Ensayo de Filtración y Seguimiento de Formación de Torta en Tiempo Real**:

   * Conectarse al Wi-Fi `Bomba_Peristaltica_UF` (`plantapiloto2`) y abrir `http://192.168.4.1`.

   * Seleccionar las RPM de trabajo (por ejemplo, $50\text{ RPM}$ o $70\text{ RPM}$) y presionar **`▶ ARRANCAR`**.

   * A los $5\text{ segundos}$ de alcanzar el régimen estable, el sistema fijará automáticamente el caudal inicial limpio **`Inicial Q₀`** (o pueden presionar **`📌 Fijar Q₀ Actual`** en el momento exacto en que comiencen a filtrar la suspensión).

   * Observar en tiempo real la evolución del caudal en ambos sensores, el porcentaje de **`MERMA EN ALIMENTACIÓN`**, **`MERMA EN PERMEADO (1 − J/J₀)`** y la curva en vivo en el gráfico inferior, sin necesidad de medir con probeta.

   * Si hacia el final del ensayo la torta reduce tanto el permeado que el caudal cae por debajo del mínimo mecánico de giro de la turbina ($\approx 2\text{ a }10\text{ mL/min}$), medir puntualmente el goteo con probeta chica, escribir el valor en **`¿Caudal < mín. del sensor? Cargar probeta:`** y presionar **`Fijar`**.

   * Al finalizar la corrida, presionar **`⏹ PARAR`** y tocar **`📥 Descargar Curva de Merma y Caudales en Excel (.CSV)`** para obtener la tabla completa tomada cada $10\text{ segundos}$ desde $t = 0\text{ s}$.

---

# 5. Diagnóstico Físico del Tablero (Foto de Laboratorio) y Guía de Conexionado Prolijo en las 2 Borneras

Al contrastar la fotografía real del montaje en banco con la documentación del repositorio (`Guia_Montaje_Placa_Filtrado_FrontEnd.md` y `esquema_conexion_borneras.html`), se identificaron **4 causas físicas en el cableado** que explican por qué los sensores dejaban de marcar caudal:

1. **Pistas aisladas en la Placa 2 (Bornera Izquierda sin ESP32)**:
   - En los adaptadores de bornera de 38 pines, cuando **no hay un módulo ESP32 insertado en el zócalo**, los distintos bornes marcados como `GND` (abajo a la izquierda, arriba a la derecha y centro) **NO están unidos entre sí** (la unión interna de masa la hace la propia placa del ESP32).
   - Si el cable `GND` que viene de la Placa 1 entra a un borne `GND` de un lado de la Placa 2, y los cables negros de los caudalímetros o los capacitores `104` se conectan a otro borne `GND` de la otra hilera, **los caudalímetros quedan con la masa desconectada (al aire)** y no pueden enviar pulsos.
2. **El tercer capacitor grande (electrolítico negro suelto en el telgopor)**:
   - En la foto se observa un capacitor electrolítico cilíndrico negro entre ambas placas con cables amarillos, marrones y violetas unidos en sus patas al aire.
   - Si ese capacitor grande (de varios $\mu\text{F}$) toca cualquiera de las líneas de señal (`P14` o `P27`), **cortocircuita los pulsos a masa** e impide que el voltaje cambie, dejando el caudal clavado en `0.0 mL/min`. Además, los empalmes volantes en sus patas generan falsos contactos. **Debe retirarse por completo.**
3. **Uso correcto de los 2 capacitores `104` ($100\text{ nF}$)**:
   - El código `104` equivale a $100\text{ nF}$ ($0.1\ \mu\text{F}$). Con la resistencia pull-up de $4.7\text{ k}\Omega$, cada capacitor `104` forma un filtro pasa-bajos con constante de tiempo $\tau = 470\ \mu\text{s}$ y frecuencia de corte:
     $$f_c = \frac{1}{2\pi \cdot 4700\ \Omega \cdot 100\times 10^{-9}\text{ F}} = \mathbf{338.6\text{ Hz}}$$
   - Este valor ($100\text{ nF}$) es óptimo: deja pasar sin atenuación todos los pulsos reales de los caudalímetros ($0\text{ a }220\text{ Hz}$) y filtra el ruido de alta frecuencia del driver DM860, siempre que haya **un solo capacitor `104` por sensor** (uno entre `P14` y `GND`, y otro entre `P27` y `GND`).
4. **Cables pasando por encima del transformador y de las fases del motor (`A+ A- B+ B-`)**:
   - En la foto, varios cables cruzan por encima del núcleo del transformador y de la bornera de potencia del DM860. Deben separarse hacia la izquierda (Zona de Señal débil) lejos de la derecha (Zona de Potencia de 220V AC / Motor).

## Tabla Paso a Paso para Reordenar las Conexiones en 10 Minutos

| Paso | Qué Conectar / Revisar | De dónde sale | A dónde llega | Verificación con Tester (Multímetro) |
| :-: | :--- | :--- | :--- | :--- |
| **1** | **Retirar el 3er capacitor grande** | Sacar el capacitor electrolítico negro del telgopor y eliminar empalmes volantes en el aire. | — | Todas las uniones deben hacerse en bornera o en el zócalo hembra de la misma fila. |
| **2** | **Unificar `GND` (Masa Común)** | Borne `GND` de Placa 1 (con ESP32) | Mismo riel `GND` de Placa 2 donde van los cables **Negros** de ambos caudalímetros y una pata de cada capacitor `104`. | En modo continuidad (pitido), medir entre el cable Negro de cada sensor y la chapa metálica del USB del ESP32: **debe pitar ($0.0\ \Omega$)**. |
| **3** | **Alimentación `5V` de Caudalímetros** | Borne `5V` (o `VIN`) de Placa 1 | Cables **Rojos** de los dos caudalímetros YF-S401. | Con el USB enchufado, medir tensión DC entre cable Rojo y cable Negro de cada sensor: **debe dar $\approx 4.8\text{ a }5.0\text{ V}$**. |
| **4** | **Canal Alimentación (`GPIO 14`)** | Cable **Amarillo** del sensor de Alimentación | Borne `P14` (Placa 2) $\rightarrow$ puente directo al borne `G14` (Placa 1). En ese mismo nodo `P14`: conectar 1 resistencia de $4.7\text{ k}\Omega$ hacia `3.3V` y 1 capacitor `104` hacia `GND`. | Medir continuidad entre el cable Amarillo del sensor de Alimentación y el pin `G14` del ESP32 ($0.0\ \Omega$). En reposo debe medir $3.3\text{ V}$ (o alternar entre $0\text{ V}$ y $3.3\text{ V}$ al girar la turbina). |
| **5** | **Canal Permeado (`GPIO 27`)** | Cable **Amarillo** del sensor de Permeado | Borne `P27` (Placa 2) $\rightarrow$ puente directo al borne `G27` (Placa 1). En ese mismo nodo `P27`: conectar 1 resistencia de $4.7\text{ k}\Omega$ hacia `3.3V` y 1 capacitor `104` hacia `GND`. | Medir continuidad entre el cable Amarillo del sensor de Permeado y el pin `G27` del ESP32 ($0.0\ \Omega$). |

