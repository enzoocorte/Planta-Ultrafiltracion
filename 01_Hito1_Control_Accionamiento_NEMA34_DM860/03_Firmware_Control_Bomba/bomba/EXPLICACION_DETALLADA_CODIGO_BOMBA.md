# 📖 MANUAL TÉCNICO Y EXPLICACIÓN DETALLADA DEL FIRMWARE `bomba.ino`
## Planta Piloto de Ultrafiltración FX100 • Hito 1: Control de Accionamiento NEMA 34 + DM860
**Codirector**: Ing. Enzo (Investigación Doctoral en Membranas)  
**Tesistas**: Antonella Guitián & Owen Cañizares  
**Archivo fuente**: [`bomba.ino`](./bomba.ino)

---

## 🎯 1. Propósito de este Documento

Este documento es una guía pedagógica y técnica exhaustiva diseñada para que los tesistas y el equipo de investigación comprendan **cada bloque de código**, las **decisiones de ingeniería de hardware y software** adoptadas, las **ecuaciones matemáticas de control**, y cómo modificar parámetros si cambian las condiciones de la planta piloto (cambio de manguera, de driver o de microcontrolador).

---

## 🏗️ 2. Arquitectura General del Firmware

El programa transforma el ESP32 en un controlador industrial autónomo con tres roles simultáneos:

```
┌────────────────────────────────────────────────────────────────────────┐
│                        ESP32 DEVKIT V1 (240 MHz)                       │
├──────────────────────────┬───────────────────────┬─────────────────────┤
│ 1. GENERADOR DE PULSOS   │ 2. CONTROL CINEMÁTICO │ 3. SERVIDOR SCADA   │
│    POR HARDWARE          │    Y PROCESO          │    WEB WI-FI        │
│                          │                       │                     │
│ • Periférico LEDC        │ • Rampa suave 40RPM/s │ • WebServer HTTP 80 │
│ • Frecuencia calculada   │ • Inversión protegida │ • JSON API (/status)│
│ • Resolución 10 bits     │ • Caudal L/min        │ • Dashboard táctil  │
│ • Cero latencia de CPU   │ • Integración volumen │ • Alerta FX100      │
└──────────────────────────┴───────────────────────┴─────────────────────┘
```

---

## 🧩 3. Desglose Sección por Sección del Código

### 3.1. Librerías Incluidas

```cpp
#include <WiFi.h>
#include <WebServer.h>
```

#### ¿Por qué estas librerías y no otras?
* **`<WiFi.h>`**: Controlador nativo de conectividad inalámbrica del ESP32 (soporta modo estación STA para conectarse a un router y modo punto de acceso AP para emitir su propia red).
* **`<WebServer.h>`**: Servidor HTTP síncrono estándar de Arduino-ESP32.
  * *¿Por qué NO usamos `ESPAsyncWebServer`?*: En versiones recientes del Core de ESP32 (v3.x), las librerías asíncronas presentan conflictos con la pila LwIP, consumen memoria dinámica (Heap) de forma impredecible y pueden provocar cuelgues (`Guru Meditation Error`) por fugas de memoria. `WebServer.h` es ligero, predecible, extremadamente robusto y no requiere dependencias externas.

---

### 3.2. Parámetros Físicos y de Proceso (Líneas 19 a 33)

```cpp
const uint8_t PIN_PUL = 18; // Borne P18 -> Señal de pulsos (STEP)
const uint8_t PIN_DIR = 19; // Borne P19 -> Señal de dirección (DIR)
const uint16_t PULSOS_POR_REV = 1600; 
const float ML_POR_VUELTA = 4.2f;
const float LIMITE_CAUDAL_MAX_LMIN = 0.60f;
const float ACELERACION_RPM_SEG = 40.0f;
```

1. **`PIN_PUL` (GPIO 18) y `PIN_DIR` (GPIO 19)**:
   * Coinciden con los bornes `P18` y `P19` de la bornera de expansión ZS-1057.
   * Son pines libres de interferencia de arranque (a diferencia de GPIO 0, 2 o 12 que son pines de *strapping* de booteo).
2. **`PULSOS_POR_REV = 1600`**:
   * El motor NEMA 34 posee un paso nativo de $1.8^\circ$ ($200\text{ pasos/revolución}$).
   * El driver DM860 se configuró con **8 micropasos** (DIP Switches: `SW5: ON`, `SW6: OFF`, `SW7: ON`, `SW8: ON`).
   * Multiplicación: $200 \times 8 = \mathbf{1600\text{ pulsos por revolución}}$.
3. **`ML_POR_VUELTA = 4.2f`**:
   * Es el desplazamiento positivo volumétrico del cabezal peristáltico MBP-2000 con tubo de silicona/PharMed: en cada vuelta de los rodillos se transfieren **4.2 mililitros**.
4. **`LIMITE_CAUDAL_MAX_LMIN = 0.60f`**:
   * Límite hidráulico de seguridad de la membrana de ultrafiltración capilar **Fresenius FX100**. Operar por encima de $0.60\text{ L/min}$ generaría sobrepresión transmembrana (TMP) excesiva, acelerando el ensuciamiento irreversible (*fouling*) o la rotura de fibras capilares.
5. **`ACELERACION_RPM_SEG = 40.0f`**:
   * Pendiente de la rampa de aceleración: aumenta o disminuye a razón de **$40\text{ RPM por segundo}$**. Pasar de 0 a 100 RPM toma exactamente $2.5\text{ segundos}$.

---

### 3.3. Configuración de Red Wi-Fi Híbrida (Líneas 37 a 43)

```cpp
const char* ssid_router = "Box804";
const char* pass_router = "plantapiloto2";
const char* ssid_ap     = "Bomba_Peristaltica_UF";
const char* pass_ap     = "plantapiloto2";
WebServer server(80);
```

* **Modo A (Cliente Router)**: Intenta conectarse a la red `Box804` durante 7 segundos. Si lo logra, se le asigna una IP local (ej. `192.168.50.53`).
* **Modo B (Punto de Acceso Autónomo)**: Si no hay router encendido (o se lleva la bomba a otra sala), crea la red Wi-Fi `Bomba_Peristaltica_UF` con IP fija `192.168.4.1`.
* *Resultado*: La bomba **nunca queda incomunicada**.

---

### 3.4. Variables de Estado y Maniobra (Líneas 48 a 64)

```cpp
bool bombaEnMarcha = false;
bool sentidoHorario = true; // true = Filtración, false = Retrolavado

bool invirtiendoSentido = false;
bool nuevoSentidoDeseado = true;
float rpm_guardada_inversion = 80.0f;

float rpm_objetivo = 80.0f; 
float rpm_actual   = 0.0f;  

float caudal_Lmin = 0.0f;   
float volumen_L   = 0.0f;   

uint32_t frecuencia_hz_actual = 0;
unsigned long t_ultimo_loop_ms = 0;
```

* **`rpm_objetivo` vs `rpm_actual`**: El operador fija una consigna (`rpm_objetivo`). El motor no salta inmediatamente a esa velocidad; la variable `rpm_actual` sigue una rampa matemática suave en cada ciclo de cómputo.
* **Variables de inversión (`invirtiendoSentido`, `nuevoSentidoDeseado`, `rpm_guardada_inversion`)**: Máquina de estados que gestiona la parada previa antes de cambiar la dirección del giro.

---

### 3.5. Control de Hardware y Salida Open-Drain (Líneas 68 a 90)

```cpp
void fijarSentidoFisico(bool horario) {
  sentidoHorario = horario;
  digitalWrite(PIN_DIR, sentidoHorario ? LOW : HIGH);
}
```

#### 🔬 ¿Por qué LOW activa y HIGH desactiva?
Estamos en configuración de **Ánodo Común a 5V**:
* El borne `DIR+` del driver está fijado a $+5\text{V}$ (desde `VIN`).
* El borne `DIR-` está conectado a `P19` (GPIO 19), configurado en el `setup()` como:
  ```cpp
  pinMode(PIN_DIR, OUTPUT_OPEN_DRAIN);
  ```
* **Cuando escribimos `LOW`**: El pin conduce a masa ($0\text{V}$). Se establece una diferencia de potencial de $5\text{V} - 0\text{V} = 5\text{V}$. Circula corriente por el diodo infrarrojo interno del optoacoplador, encendiéndolo $\rightarrow$ Sentido Horario.
* **Cuando escribimos `HIGH`**: En modo Open-Drain (drenador abierto), el pin no empuja 3.3V, sino que entra en **Alta Impedancia (Hi-Z)** (actúa como un interruptor abierto). La corriente es exactamente **$0.0\text{ mA}$**, garantizando el apagado total del optoacoplador $\rightarrow$ Sentido Antihorario.

---

```cpp
void actualizarPulsosMotor(float rpm, bool marcha) {
  if (marcha && rpm >= 5.0f) {
    uint32_t f = (uint32_t)((rpm * (float)PULSOS_POR_REV) / 60.0f);
    if (f != frecuencia_hz_actual) {
      ledcChangeFrequency(PIN_PUL, f, 10);
      ledcWrite(PIN_PUL, 512); // 50% ciclo de trabajo
      frecuencia_hz_actual = f;
    }
  } else {
    if (frecuencia_hz_actual != 0) {
      ledcWrite(PIN_PUL, 1023); // Pin en HIGH -> Optoacoplador apagado
      frecuencia_hz_actual = 0;
    }
  }
}
```

#### 🔬 ¿Por qué usamos el periférico LEDC por hardware?
1. **Sin consumo de CPU**: El temporizador LEDC del ESP32 genera la onda cuadrada por silicio dedicado. El procesador queda 100% libre para atender las peticiones web HTTP y calcular la integración del volumen.
2. **Cero temblor de pulso (*Jitter*)**: Las librerías que usan retardos por software (`delayMicroseconds`) o interrupciones por software sufren micro-congelamientos cuando el Wi-Fi transmite datos. El LEDC es un contador por hardware totalmente inmune a la actividad de red.
3. **Optimización con `frecuencia_hz_actual`**: Cambiar la frecuencia en un temporizador de hardware requiere recalcular prescalers. Si llamáramos a `ledcChangeFrequency()` en cada iteración del `loop()` (cientos de veces por segundo), colapsaríamos el bus interno. Por eso **solo se reconfigura el hardware cuando el valor entero en Hz cambia**.
4. **Ciclo de trabajo (Duty Cycle)**:
   * `ledcWrite(PIN_PUL, 512)`: Con 10 bits de resolución ($0\text{ a }1023$), el valor $512$ representa exactamente un ciclo de trabajo del **$50\%$** (onda cuadrada simétrica perfecta).
   * `ledcWrite(PIN_PUL, 1023)`: Nivel ALTO continuo. En Ánodo Común, poner el cátodo en nivel ALTO apaga el diodo optoacoplador. El driver entra en modo *Standstill* (corriente de reposo reducida al 50% gracias a `SW4: OFF`), manteniendo el motor completamente frío.

#### Ecuación de Frecuencia:
$$f_{\text{pulsos}} (\text{Hz}) = \frac{\text{RPM} \times \text{PULSOS\_POR\_REV}}{60} = \frac{\text{RPM} \times 1600}{60} = \mathbf{\text{RPM} \times 26.6667\text{ Hz}}$$

| Consigna | Frecuencia Generada | Caudal Teórico MBP-2000 | Estado Proceso |
| :---: | :---: | :---: | :--- |
| **$70\text{ RPM}$** | $1866.7\text{ Hz}$ | $0.294\text{ L/min}$ | Límite inferior seguro (sin vibración) |
| **$80\text{ RPM}$** | $2133.3\text{ Hz}$ | $0.336\text{ L/min}$ | Flujo bajo nominal |
| **$100\text{ RPM}$** | $2666.7\text{ Hz}$ | $0.420\text{ L/min}$ | Flujo estándar de operación |
| **$120\text{ RPM}$** | $3200.0\text{ Hz}$ | $0.504\text{ L/min}$ | Flujo sostenido |
| **$140\text{ RPM}$** | $3733.3\text{ Hz}$ | $0.588\text{ L/min}$ | Alerta preventiva (cercano a $0.60$) |
| **$160\text{ RPM}$** | $4266.7\text{ Hz}$ | $0.672\text{ L/min}$ | Límite superior mecánico (Alarma roja) |

---

### 3.6. Interfaz SCADA y Protección del Cliente Web (Líneas 95 a 257)

La interfaz gráfica es servida directamente desde la memoria Flash del ESP32 mediante el macro `PROGMEM` (evita ocupar la escasa memoria RAM dinámica).

#### Características clave del frontend web:
1. **Semáforo contra saturación de red (`solicitando`)**:
   ```javascript
   let solicitando = false;
   setInterval(() => {
     if (solicitando) return;
     solicitando = true;
     fetch('/status')
       .then(r => r.json())
       .then(d => { ... })
       .finally(() => { solicitando = false; });
   }, 800);
   ```
   * *Problema común*: Si el navegador manda una petición cada 800 ms pero la red Wi-Fi sufre un retardo transitorio de 1.5 segundos, las peticiones se acumulan en cola, desbordando los sockets del ESP32 y colgando el servidor.
   * *Solución*: La bandera `solicitando` garantiza que **nunca se emita una nueva consulta hasta que la anterior haya terminado**, asegurando estabilidad infinita.
2. **Alerta Dinámica de Caudal para Membrana FX100**:
   ```javascript
   if (d.flow > 0.60) {
     alerta.className = "alerta-box visible";
     dispFlow.style.color = "var(--danger)";
   } else if (d.flow > 0.55) {
     alerta.className = "alerta-box visible";
     alerta.innerText = "⚠️ PRECAUCIÓN: PRÓXIMO AL LÍMITE DEL FILTRO FX100 (0.60 L/min)";
     dispFlow.style.color = "var(--warning)";
   } else {
     alerta.className = "alerta-box";
     dispFlow.style.color = "#fff";
   }
   ```

---

### 3.7. Función `setup()`: Configuración Segura (Líneas 262 a 368)

1. **`Serial.begin(115200)`**: Diagnóstico por cable USB a alta velocidad.
2. **`pinMode(PIN_DIR, OUTPUT_OPEN_DRAIN)`**: Configura la salida en drenador abierto para el circuito de Ánodo Común.
3. **`ledcAttach(PIN_PUL, 800, 10)`**: Asocia el pin 18 al generador LEDC con resolución de 10 bits.
4. **`WiFi.persistent(false)` y `WiFi.disconnect(true, true)`**:
   * *¿Por qué se hace esto?*: Por defecto, el ESP32 intenta guardar las credenciales Wi-Fi en la memoria flash NVS en cada reinicio. Esto desgasta la memoria flash y provocaba reinicios espontáneos en booteos previos. Al desactivar la persistencia, el stack Wi-Fi inicializa en un estado limpio y determinístico.
5. **Endpoints de la API REST**:
   * **`GET /`**: Devuelve la página HTML del SCADA.
   * **`GET /set?rpm=VALOR`**: Modifica la consigna `rpm_objetivo` (validada entre 70 y 160).
   * **`GET /cmd?act=START|STOP|TOGGLE_DIR`**: Ejecuta las órdenes de marcha, parada o cambio de sentido.
   * **`GET /status`**: Devuelve un JSON compacto con el estado en tiempo real para refrescar la pantalla:
     ```json
     {"on":true,"inv":false,"rpm":100.0,"dir":true,"flow":0.420,"vol":1.25,"ip":"192.168.50.53"}
     ```

---

### 3.8. Función `loop()`: Cinemática No Bloqueante (Líneas 373 a 444)

```cpp
void loop() {
  server.handleClient(); // Atiende peticiones HTTP entrantes

  unsigned long t_actual = millis();
  float dt = (t_actual - t_ultimo_loop_ms) / 1000.0f;

  if (dt >= 0.05f) { // Actualización cinemática a 20 Hz (cada 50 ms)
    t_ultimo_loop_ms = t_actual;
    ...
```

#### 🔬 ¿Por qué NO usamos `delay()`?
El uso de `delay()` congela el microprocesador. Si pusiéramos un `delay(100)`, el servidor web no respondería durante ese tiempo, la página web daría error de tiempo de espera y el cálculo del volumen sería errático.  
Se usa la técnica de **tiempo no bloqueante (`millis()`)**: cada 50 ms ($20\text{ Hz}$), se calcula la fracción de tiempo transcurrido $\Delta t$ y se actualiza la rampa.

#### Algoritmo de la Rampa de Velocidad:
```cpp
if (rpm_actual < rpm_objetivo) {
  rpm_actual += ACELERACION_RPM_SEG * dt;
  if (rpm_actual > rpm_objetivo) rpm_actual = rpm_objetivo;
} else if (rpm_actual > rpm_objetivo) {
  rpm_actual -= ACELERACION_RPM_SEG * dt;
  if (rpm_actual < rpm_objetivo) rpm_actual = rpm_objetivo;
}
```

#### Máquina de Estados: Inversión Segura de Sentido:
Si el usuario pulsa "Invertir Giro" mientras la bomba está girando a 120 RPM:
1. `invirtiendoSentido` pasa a `true`.
2. Guarda la velocidad actual: `rpm_guardada_inversion = 120.0f`.
3. Fuerza `rpm_objetivo = 0.0f` (el motor comienza a frenar suavemente a $40\text{ RPM/s}$).
4. Cuando el motor llega a reposo completo:
   ```cpp
   if (invirtiendoSentido && rpm_actual <= 0.1f) {
     fijarSentidoFisico(nuevoSentidoDeseado); // Conmuta el pin DIR con motor parado
     invirtiendoSentido = false;
     rpm_objetivo = rpm_guardada_inversion;  // Vuelve a acelerar hacia 120 RPM
   }
   ```
5. *Resultado*: Cero golpes mecánicos, cero pérdida de pasos y cero sobrepresión hidráulica en la membrana.

#### Integración Numérica del Volumen Bombeado:
En cada ciclo de 50 ms:
$$\text{caudal} (\text{L/min}) = \frac{\text{RPM}_{\text{actual}} \times 4.2}{1000}$$
$$\Delta \text{Volumen} (\text{Litros}) = \frac{\text{caudal}}{60} \times \Delta t$$
$$\text{Volumen Total} = \text{Volumen Total} + \Delta \text{Volumen}$$

---

## 💡 4. Justificación de Decisiones Críticas de Ingeniería

### 4.1. ¿Por qué el rango se fijó en 70 a 160 RPM?
1. **Límite Inferior (70 RPM)**:
   * Los motores paso a paso NEMA 34 poseen una masa rotórica considerable ($J_{\text{rotor}} \approx 1400\text{ g}\cdot\text{cm}^2$).
   * A frecuencias bajas ($30\text{ a }60\text{ RPM}$, equivalentes a $80-160\text{ Hz}$), la frecuencia de excitación coincide con la frecuencia natural de resonancia mecánica del conjunto rotor-cabezal-banco. Esto causa vibración severa, ruido áspero y pulsaciones de par. Por encima de $70\text{ RPM}$, el sistema entra en régimen giroscópico estable y suave.
2. **Límite Superior (160 RPM)**:
   * La tensión aplicada es de $24\text{ VAC}$ ($\approx 34\text{ VDC}$ de bus interno en el DM860).
   * Al aumentar las RPM, el rotor genera una fuerza contraelectromotriz creciente ($V_{\text{BEMF}}$) proporcional a la velocidad angular. Además, la reactancia inductiva de las bobinas ($X_L = 2\pi f L$) se opone a la entrada de corriente.
   * A más de $160\text{ RPM}$, los 34 VDC de la fuente no alcanzan para forzar los 3A nominales en el bobinado dentro de los estrechos semiciclos de tiempo disponibles, produciendo pérdida de pasos (*stall*). Como a $160\text{ RPM}$ ya se obtienen $0.672\text{ L/min}$ (por encima del máximo de la membrana FX100), no tiene sentido forzar el motor más allá.

### 4.2. ¿Por qué Ánodo Común a 5V y no Cátodo Común a 3.3V?
* Los optoacopladores del Leadshine DM860 tienen una caída de diodo interno de $1.2\text{V}$ y resistencia limitadora de $270\,\Omega$.
* Si se excita con 3.3V, la corriente que circula es:
  $$I = \frac{3.3\text{V} - 1.2\text{V}}{270\,\Omega} \approx 7.7\text{ mA}$$
  Aunque teóricamente conmuta, queda cerca del umbral mínimo de saturación del fototransistor, haciéndolo vulnerable al ruido eléctrico generado por la conmutación de potencia del transformador y los MOSFETs.
* Al usar **Ánodo Común a 5V** (`VIN`):
  $$I = \frac{5.0\text{V} - 1.2\text{V}}{270\,\Omega} \approx 14.0\text{ mA}$$
  La corriente se duplica, saturando con fuerza el optoacoplador y garantizando flancos de pulso perfectamente rectangulares y sin perturbaciones.

---

## 🛠️ 5. Recetario para Tesistas: ¿Cómo Modificar Parámetros?

Si en el laboratorio necesitan hacer ajustes, estas son las líneas exactas a modificar en `bomba.ino`:

| Parámetro a Cambiar | Línea en `bomba.ino` | Código Actual | Cómo Cambiarlo |
| :--- | :---: | :--- | :--- |
| **Calibración de manguera (mL/vuelta)** | `26` | `const float ML_POR_VUELTA = 4.2f;` | Medir con probeta en 100 vueltas y colocar el nuevo valor exacto. |
| **Límite de alarma de la membrana** | `29` | `const float LIMITE_CAUDAL_MAX_LMIN = 0.60f;` | Modificar si se cambia el filtro por uno de mayor o menor superficie. |
| **Suavidad de aceleración** | `32` | `const float ACELERACION_RPM_SEG = 40.0f;` | Reducir a `20.0f` si la rampa se quiere más lenta, o subir a `60.0f` si se quiere más ágil. |
| **Pines de conexión** | `19-20` | `PIN_PUL = 18; PIN_DIR = 19;` | Modificar si se mueven los cables a otros bornes de la placa. |
| **Red Wi-Fi del laboratorio** | `37-38` | `"Box804" / "plantapiloto2"` | Cambiar nombre y clave si se usa otro router o repetidor. |
| **Rango del deslizador web** | `176` | `min="70" max="160" value="80"` | Ajustar los atributos HTML `min` y `max` si se cambia el rango operativo. |

---

## 🎯 Entregable Concreto
* **Manual pedagógico de ingeniería del software de control del Hito 1**: Explicación exhaustiva línea por línea de `bomba.ino`, fundamentos físicos de Ánodo Común Open-Drain, modelado matemático de frecuencia/caudal/volumen, justificación de límites de velocidad (70–160 RPM) y guía de modificación rápida para los tesistas de ingeniería industrial.

---

## 📋 Lista de Verificación (Checklist de Avance)
- [x] Documentación técnica de `bomba.ino` redactada y alojada en la carpeta de firmware del Hito 1.
- [x] Justificación física y matemática de la generación de pulsos LEDC por hardware documentada.
- [x] Explicación del circuito de Ánodo Común a 5V con `OUTPUT_OPEN_DRAIN` explicada detalladamente.
- [x] Razones operativas de acotamiento a 70–160 RPM (resonancia vs BEMF) documentadas.
- [x] Guía rápida de modificación de constantes (mL/rev, aceleración, límites) elaborada para los tesistas.
