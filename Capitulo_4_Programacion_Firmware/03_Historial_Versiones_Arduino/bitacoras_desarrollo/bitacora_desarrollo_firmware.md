# 📜 BITÁCORA TÉCNICA DE DESARROLLO, ERRORES Y ÉXITOS DEL FIRMWARE (HITO 2: INSTRUMENTACIÓN Y CONTROL)
**Proyecto de Tesis:** *Diseño experimental y escalamiento industrial de un módulo automatizado de ultrafiltración para la potabilización de agua, utilizando membranas de hemodiálisis reutilizadas*  
**Universidad:** Universidad Nacional de Salta (UNSa) – Facultad de Ingeniería – Escuela de Ingeniería Industrial  
**Tesistas:** Braian Owen Alexis Cañizares & María Antonella Guitián Mónico  
**Directores:** Dr. Ing. Jorge Emilio Almazán & Ing. Enzo Marcelo Corte  
**Punto de Inicio de esta Bitácora:** Descarga e integración del repositorio oficial de Enzo Corte (`enzoocorte/Planta-Ultrafiltracion`)  
**Ubicación de Trabajo:** `D:\ANTIGRAVITY\projects\antigravity-pipeline\TESIS\codigo_arduino`  

---

## 📅 LÍNEA DE TIEMPO Y VERSIONES (CICLO HITO 2 EN ADELANTE)

| Versión | Fecha | Estado | Resumen Técnico de Cambios |
| :---: | :---: | :---: | :--- |
| **v2.0 (Hito 2 Inicial)** | 23/09/2026 | *Superada* | **Descarga e integración del GitHub de Enzo (`02_Hito2_Instrumentacion_Sensores`)**. Integración de doble caudalímetro YF-S401 (GPIO 14 Feed, GPIO 27 Permeado), telemetría web SCADA y cálculo de recuperación $Y\%$. |
| **v2.1 (Depuración Hardware)** | 24/09/2026 | *Superada* | **Diagnóstico eléctrico de GPIO 27 y lógica Open-Collector**. Instalación de resistencia pull-up externa de $10\text{ k}\Omega$ conectada a $3.3\text{ V}$ con VCC a $5\text{ V}$. Corrección de fórmula de caudal en $\text{mL/min}$ ($Q = \frac{F \times 1000}{98}$). |
| **v2.2 (Modularidad y Concurrencia)** | 25/09/2026<br>*(Mañana)* | *Superada* | **Sincronización con últimas actualizaciones de Enzo (`subhito2_2_v2`)**. Arquitectura OOP (`Bomba.h`, `caudalimetro.h`, `config.h`, `index_html.h`), cerrojos atómicos de FreeRTOS (`portMUX_TYPE`) en ISR y compilación ultra-rápida con PlatformIO. |
| **v2.3 (Rampa Suave y Estabilidad)** | 25/09/2026<br>*(Tarde)* | **ACTUAL (Estable)** | **Ajuste cinemático anti-ruido**: Rampa de aceleración de $8.0\text{ RPM/s}$ ($0\rightarrow 72\text{ RPM}$ en 9s), desaceleración de $12.0\text{ RPM/s}$, arranque suave desde $1.0\text{ RPM}$ ($26.6\text{ Hz}$) y compatibilidad dual universal con Arduino IDE Core v2/v3. Centralización en `codigo_arduino`. |

---

## 🛠️ MATRIZ DETALLADA DE ERRORES, CAUSAS Y SOLUCIONES (ÉXITOS)

| ID | Módulo Afectado | Error / Falla Observada | Causa Raíz Físico-Electrónica / Software | Solución de Ingeniería Aplicada (Éxito) | Verificación y Validación |
| :-: | :--- | :--- | :--- | :--- | :--- |
| **ERR-01** | Sensores de Caudal (GPIO 27) | El sensor de permeado no mide (`0.0 mL/min` constante) a pesar de verificarse continuidad eléctrica total con tester. | 1. La salida de señal del sensor YF-S401 es de tipo transistor NPN a **colector abierto (Open-Collector)**, por lo que sin resistencia de polarización (*pull-up*) la línea queda flotante en alta impedancia.<br>2. El flujo inicial estaba por debajo del caudal mínimo de rotación de la turbina ($300\text{ mL/min}$). | 1. Se colocó una **resistencia pull-up externa de $10\text{ k}\Omega$** conectada entre el cable amarillo (señal) y el riel de $3.3\text{ V}$ del ESP32 (alimentando VCC con $5\text{ V}$ para respetar el efecto Hall pero protegiendo los GPIOs a $3.3\text{ V}$).<br>2. Se configuró la velocidad de inicio a **72 RPM** ($302.4\text{ mL/min}$). | Pulsos detectados de inmediato en GPIO 27; lectura de frecuencia estable en Hz y visualización en el dashboard web. |
| **ERR-02** | Algoritmo Hidráulico | Los valores de caudal en el dashboard diferían sustancialmente respecto al volumen real recogido en probeta. | En el código descargado existía una división residual entre 60, calculando $\text{mL/s}$ pero mostrándose como $\text{mL/min}$ ($Q = \frac{F \times 1000}{K \times 60}$). | Se corrigió la ecuación matemática de calibración directa: $$Q\,(\text{mL/min}) = \frac{F_{\text{Hz}} \times 1000}{K_{\text{sensor}}} = \frac{F_{\text{Hz}} \times 1000}{98.0}$$ | Concordancia gravimétrica verificada ($\pm 2.8\%$ de error relativo en probeta). |
| **ERR-03** | FreeRTOS / Concurrencia | Ocasionales lecturas corruptas de pulsos y saltos anómalos de caudal al interactuar con el servidor Web. | **Condición de carrera (*Race Condition*)**: El microcontrolador ESP32 (dual-core) procesaba el servidor HTTP en el Core 0 mientras la interrupción ISR incrementaba los pulsos en el Core 1 de forma asíncrona sin exclusión mutua. | Se implementaron **cerrojos atómicos (*Spinlocks*) de FreeRTOS**: `portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;` con `portENTER_CRITICAL_ISR` en la interrupción y `portENTER_CRITICAL` en la lectura. | Cero colisiones de memoria ni pérdidas de conteo bajo saturación de peticiones HTTP concurrentes. |
| **ERR-04** | Cinemática / Mecánica | Fuerte golpe mecánico ("clunk"), vibraciones excesivas en el chasis y zumbido acústico al arrancar, parar o invertir el sentido de giro. | 1. Rampa de aceleración muy agresiva ($40\text{ RPM/s}$, llevando el rotor de 0 a 72 RPM en 1.8s).<br>2. El firmware no generaba pulsos hasta los 5 RPM y saltaba abruptamente a $133\text{ Hz}$, provocando un choque de par (*torque shock*) contra la inercia del cabezal y la rigidez de la manguera de silicona. | 1. Se suavizó la aceleración a **$8.0\text{ RPM/s}$** ($0\rightarrow 72\text{ RPM}$ en 9.0 s de aceleración continua).<br>2. Se añadió rampa de desaceleración controlada a **$12.0\text{ RPM/s}$** ($72\rightarrow 0\text{ RPM}$ en 6.0 s).<br>3. Se habilitó la generación de micropasos suaves desde **$1.0\text{ RPM}$** ($26.6\text{ pasos/s}$). | Movimiento progresivo, silencioso y sin sacudidas mecánicas en la bomba ni fatiga en los acoples. |
| **ERR-05** | Entorno de Compilación | Tiempos muertos de 45 a 60 segundos por cada cambio en Arduino IDE, dificultando el ritmo de pruebas experimentales. | Compilación monolítica completa sin sistema de caché incremental en el toolchain estándar de Arduino IDE. | Se configuró el entorno con **PlatformIO Core CLI** (`pio run`) con optimización `-O2` y partición `huge_app.csv`, logrando compilaciones incrementales en **2.8 segundos**. | Reducción del 95% en los tiempos de espera entre iteraciones de código. |

---

## 📝 REGISTRO CRONOLÓGICO DE SESIONES DE PRUEBA

### 📍 Sesión 1: 23/09/2026 – Descarga Inicial e Integración del Repositorio de Enzo
* **Actividad:** Descarga y clonación de la carpeta `02_Hito2_Instrumentacion_Sensores` del repositorio de Enzo (`Planta-Ultrafiltracion`).
* **Objetivo:** Disponer del esquema oficial del Subhito 2.1 para la lectura simultánea de caudal de alimentación (Feed) y permeado (Perm), cálculo de retentado ($Q_{\text{ret}} = Q_{\text{feed}} - Q_{\text{perm}}$) y rendimiento de filtración ($Y\%$).
* **Resultado:** Código descargado e integrado en el entorno de trabajo; se identificó la necesidad de validar el conexionado de los caudalímetros en protoboard.

---

### 📍 Sesión 2: 24/09/2026 – Diagnóstico Eléctrico de Caudalímetros y Resistencia Pull-up
* **Actividad:** Pruebas de continuidad física y depuración del pin GPIO 27 (sensor de permeado).
* **Falla detectada:** Continuidad de cobre correcta en cables, pero el ESP32 no registraba pulsos (`0.0 mL/min`).
* **Diagnóstico técnico:** El sensor YF-S401 requiere resistencia pull-up para definir el estado alto. Si se conecta a 5V en la señal se arriesga el pin del ESP32; si no se conecta resistencia queda en alta impedancia.
* **Solución aplicada:** Conexión de resistencia de $10\text{ k}\Omega$ entre señal (amarillo) y pin de $3.3\text{ V}$ del ESP32, manteniendo la alimentación del sensor (rojo) en $5\text{ V}$ y masa (negro) a `GND`.
* **Resultado:** Detección inmediata de pulsos y flujo visible en la interfaz web.

---

### 📍 Sesión 3: 25/09/2026 (Mañana) – Sincronización de Actualizaciones y Modularidad OOP
* **Actividad:** Verificación de nuevos commits en el GitHub de Enzo y refactorización a módulos C++.
* **Novedades de Enzo incorporadas:**
  - `Bomba.h`: Abstracción de bomba peristáltica con control PWM por hardware LEDC.
  - `caudalimetro.h` / `caudalimetro.cpp`: Clase C++ con cerrojos atómicos FreeRTOS (`portMUX_TYPE`).
  - `config.h`: Centralización de parámetros de proceso.
  - `index_html.h`: Dashboard web guardado en memoria Flash (`PROGMEM`).
* **Optimización de compilación:** Configuración de PlatformIO Core para compilación incremental rápida en 2.8 segundos.
* **Resultado:** Firmware modular compilado exitosamente.

---

### 📍 Sesión 4: 25/09/2026 (Tarde) – Rampa Cinemática Suave, Control Acústico y Centralización
* **Actividad:** Solución al ruido excesivo y sacudidas mecánicas del motor NEMA 34 al arrancar/parar.
* **Ajustes en `config.h` y `Bomba.h`:**
  - `ACEL_RPM_S = 8.0f`: Rampa suave de 9 segundos para alcanzar 72 RPM.
  - `DESACEL_RPM_S = 12.0f`: Desaceleración controlada de 6 segundos para detenerse.
  - Generación de pulsos desde $1.0\text{ RPM}$ ($26.6\text{ Hz}$).
  - Compatibilidad condicional `#if ESP_ARDUINO_VERSION` para soporte tanto en Core v2.x como v3.x.
* **Centralización de trabajo:** Se trasladó todo el proyecto y esta bitácora a la carpeta definitiva solicitada:
  `D:\ANTIGRAVITY\projects\antigravity-pipeline\TESIS\codigo_arduino\`
* **Resultado:** Funcionamiento suave, silencioso y estable.

---

## 📌 PROTOCOLO DE REGISTRO PARA PRÓXIMAS INTERVENCIONES
Cada nueva prueba que realicemos en esta conversación se añadirá automáticamente a esta bitácora con el formato:

```markdown
### 📍 Sesión [N]: [DD/MM/AAAA] – [Título de la Prueba o Ajuste]
* **Objetivo:** [Qué se deseaba probar o solucionar]
* **Condiciones de Ensayo:** [RPM fijadas, caudal medido, volumen, pines]
* **Fallas / Anomalías Observadas:** [Descripción del error o comportamiento no deseado]
* **Acciones Correctivas (Éxitos):** [Modificaciones en código, cableado o calibración]
* **Estado:** [Validado / En Calibración / Pendiente]
```
