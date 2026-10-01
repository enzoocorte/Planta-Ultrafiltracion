# 💧 PLANTA PILOTO DE ULTRAFILTRACIÓN INDUSTRIAL (FX100) & REACTOR DE COAGULACIÓN-SEDIMENTACIÓN
## Proyecto de Tesis de Grado en Ingeniería Industrial / Química — Automatización, Control IoT & Modelado de Membranas
**Tesistas de Grado:** Antonella Guitián & Owen Cañizares  
**Codirector de Tesis:** Ing. Enzo *(Investigador Doctoral — Tesis Doctoral en Procesos de Separación por Membranas)*  
**Asesor de Automatización e Instrumentación:** Antigravity AI  
**Ubicación de Montaje:** Laboratorio de Ingeniería — Salta, Argentina  
**Última Actualización:** Septiembre 2026  

---

## 🌐 1. Acceso Rápido al Servidor Web SCADA del ESP32

Para operar la planta piloto, ver los caudalímetros en vivo, arrancar la bomba y exportar los datos a Excel, el ESP32 levanta una red Wi-Fi propia y simultáneamente se enlaza a la red del laboratorio:

```
                  ┌───────────────────────────────────────────────┐
                  │             CÓMO CONECTARSE AL SCADA          │
                  └───────────────────────────────────────────────┘
                                          │
                  ┌───────────────────────┴───────────────────────┐
                  ▼                                               ▼
     [ MODO 1: Wi-Fi Propio Directo ]               [ MODO 2: Router de Laboratorio ]
     (Ideal en banco sin router)                    (Notebook conectada al Box804)
     • Red Wi-Fi: Bomba_Peristaltica_UF             • Red Wi-Fi: Box804
     • Clave: plantapiloto2                         • Clave: plantapiloto2
     • En tu navegador ingresa a:                   • En tu navegador ingresa a:
       👉 http://192.168.4.1                          👉 http://bomba.local
```

> [!TIP]
> **Consejo para celulares**: Si te conectas a la red `Bomba_Peristaltica_UF` desde el teléfono móvil y la página no abre, desactiva momentáneamente los **Datos Móviles (4G/5G)** para que el teléfono no intente buscar la IP en internet.

---

## 🧠 2. ¿Por qué usamos Git y Control de Versiones en esta Tesis?
*(Sección de lectura obligatoria para Antonella y Owen)*

Muchas veces surge la pregunta: *¿Por qué no guardamos simplemente carpetas como `codigo_final_v2_este_si.ino` en un pendrive?*  
En un proyecto de ingeniería industrial y aplicada, **el control de versiones con Git no es una herramienta para programadores informáticos; es la Bitácora de Laboratorio Digital e Inalterable**.

### Las 4 razones fundamentales de ingeniería:
1. **Trazabilidad Científica y Defensa de Tesis**:  
   Ante el jurado evaluador, ustedes no están presentando un proyecto escolar que "funciona por casualidad". Git registra con fecha y hora exacta cada hipótesis, cada ensayo, cada fallo de ruido y la justificación matemática de cómo se solucionó.
2. **Registro de Fallos y Diagnóstico Reproducible**:  
   En la experimentación de laboratorio, si un ensayo funcionaba el lunes y el miércoles deja de funcionar, con Git se compara exactamente qué línea de código, qué resistencia o qué parámetro mecánico cambió en el camino.
3. **Seguridad Absoluta (Cero Pérdida de Trabajo)**:  
   Si una modificación de prueba desestabiliza el motor o rompe la lógica de la bomba, volver al estado anterior 100% estable toma un solo segundo con `git checkout`, sin temor a haber destruido horas de trabajo.
4. **Trabajo Colaborativo en Paralelo**:  
   Permite que Enzo supervise y corrija la arquitectura, mientras Antonella y Owen cargan datos de calibración o actualizan manuales, sin pisarse ni duplicar archivos.

---

## 📜 3. Bitácora de Commits Clave y Solución de Problemas (Changelog de Ingeniería)

Esta tabla resume la evolución cronológica del sistema, los desafíos encontrados en el banco de pruebas y las soluciones implementadas:

| Fecha | Commit | Desafío de Ingeniería Encontrado | Solución Técnica Implementada en el Repositorio |
| :---: | :---: | :--- | :--- |
| **01/10/2026** | *(Actual)* | **Integración de Calibración de Owen, Auto-Calibración y Persistencia Flash NVS**: Owen calibró caudalímetros en probeta a 50 RPM ($680\text{ mL/min}$) obteniendo $K_{\text{alim}} = 154.62$, $K_{\text{perm}} = 55.00$ y cilindrada real de $13.6\text{ mL/rev}$, incorporando almacenamiento no-volátil en Flash NVS (`Preferences.h`), datalogger multi-sesión y optimización de filtro a 2000 µs. | Se integró formalmente la rama de Owen a `main`. Se aplicó ajuste de seguridad preventiva en `config.h`: `RPM_INICIO = 25.0f` ($\approx 340\text{ mL/min}$) para resguardar la membrana Fresenius FX100 ($600\text{ mL/min}$ / $36\text{ RPM}$). Se consolidó [`firmware_planta/`](./02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/firmware_planta/) como versión oficial activa y se integró [`Capitulo_4_Programacion_Firmware/`](./Capitulo_4_Programacion_Firmware/) con simulaciones LTspice y bitácoras. |
| **30/09/2026** | `0629e00` | **Ruido de 81 Hz (835.5 mL/min) en Reposo e Inversión de Canales**: Con bomba detenida, Feed registraba 81.9 Hz constante por acoplamiento de zumbido de 50 Hz/100 Hz que saturaba el filtro ISR de 12 ms ($1/0.0122\text{ s} \approx 81.9\text{ Hz}$). Además, los datasets del CSV revelaron $Q_{\text{perm}} \gg Q_{\text{feed}}$ ($627\text{ mL/min}$ en P27 vs $0\text{ mL/min}$ en P14). | Se documentó formalmente el modelado matemático para la tesis en [Diagnostico_y_Resolucion_Problemas_Instrumentacion.md](./02_Hito2_Instrumentacion_Sensores/01_Guias_Montaje_y_Calibracion/Diagnostico_y_Resolucion_Problemas_Instrumentacion.md). Se activó `INPUT_PULLUP` interno como doble barrera en firmware, se corrigió el cruce de cables amarillos de señal entre borneras y se protocolizó el filtro RC ($4.7\text{ k}\Omega + 100\text{ nF}$) y masa común rígida ($0.0\ \Omega$). |
| **29/09/2026** | `32a1994` | **Torque Ripple y Ruido en Bajas RPM**: Al operar a $20-35\text{ RPM}$ con manguera de $12\text{ mm}$, el NEMA 34 vibraba por pasos discretos a 1600 pulsos/rev. | Se actualizó `config.h` a **3200 pulsos/rev** (16 micropasos en Leadshine DM860: `SW5=OFF, SW6=OFF, SW7=ON, SW8=ON`), logrando un giro ultrasuave y silencioso. |
| **28/09/2026** | `d731cdb`<br>`f6ec04f` | **Ruido EMI en Caudalímetros YF-S401**: El chopper de conmutación del motor (3A inductivos) inducía pulsos falsos en los pines con pull-up interno débil ($45\text{ k}\Omega$). | Diseño del **Módulo Front-End** con la segunda bornera ZS-1057: resistencias de pull-up externas de **$4.7\text{ k}\Omega$ a 3.3V** + filtro pasabajos RC con capacitor cerámico de **$100\text{ nF}$**. |
| **28/09/2026** | `3047a8e`<br>`3f3013a` | **Bloqueo en Inversión de Sentido**: Si el operador presionaba STOP durante la rampa de frenado a 0 RPM para invertir giro, la consigna quedaba huérfana en 0 RPM. | Corrección de la máquina de estados finitos (FSM) de la bomba: fallback seguro a `RPM_INICIO` y totalizador de volumen desacoplado. |
| **25/09/2026** | `5bfc067`<br>`ed5e8d6` | **Límite de Caudal de la Membrana FX100 ($600\text{ mL/min}$)**: La manguera de $12\text{ mm}$ desplaza $15.4\text{ mL/vuelta}$. A más de 36 RPM se supera el caudal seguro del filtro capilar. | Adaptación de la rampa SCADA a rango $20 - 42\text{ RPM}$, incorporación de alerta visual de caudal crítico y exportador CSV para planillas Excel. |
| **24/09/2026** | `81949db`<br>`4f47370` | **Sentido de Giro Invertido y Error de Unidades**: Al cablear el DM860 en Cátodo Común, el pin DIR arrancaba en sentido antihorario (retrolavado) y el caudal dividía por 60 dos veces. | Inversión lógica de `PIN_DIR` para arranque en Filtración horaria por defecto y corrección analítica de la ecuación de flujo ($Q = F / K$). |
| **23/09/2026** | `a0db29c` | **Código Monolítico Inmantenible**: El firmware inicial tenía más de 800 líneas en un solo archivo `.ino`, dificultando el aislamiento de errores. | Refactorización completa en arquitectura modular C++ orientada a objetos: `bomba.cpp`, `caudalimetro.cpp`, `config.h` e `index_html.h`. |

---

## 🗂️ 4. Estructura del Repositorio y Guía de Navegación de Archivos

Cada carpeta tiene un propósito específico en las etapas de la tesis. **Aquí se detalla qué contiene cada una y para qué deben consultarla:**

```
SistemaUF/
├── 📁 00_General_y_P_ID_Planta/
├── 📁 01_Hito1_Control_Accionamiento_NEMA34_DM860/
├── 📁 02_Hito2_Instrumentacion_Sensores/
├── 📁 03_Hito3_Reactor_Sedimentador_Agitador/
├── 📁 04_Hito4_Integracion_Automatizacion_IoT/
├── 📁 05_Hito5_Ensayos_Membrana_VidaUtil/
├── 📁 Capitulo_4_Programacion_Firmware/
└── 📁 Archivado/
```

### Detalle de Carpetas:

#### 📐 `00_General_y_P_ID_Planta/`
* **Contenido**: Planos maestros de instrumentación y tuberías bajo norma ISA 5.1, bocetos originales de ingeniería, dimensionamiento de tanques y el [diagrama_pid_interactivo.html](./00_General_y_P_ID_Planta/diagrama_pid_interactivo.html).
* **¿Para qué leerlo?**: Para entender el flujo global del agua, la ubicación de las válvulas, tomas de presión y balances de materia de toda la planta.

#### ⚡ `01_Hito1_Control_Accionamiento_NEMA34_DM860/`
* **Contenido**: Toda la ingeniería de impulsión: motor NEMA 34, driver Leadshine DM860, cabezal MBP-2000, cálculo de torque y el [Inventario_Consolidado.md](./01_Hito1_Control_Accionamiento_NEMA34_DM860/01_Hardware_y_Cableado/Inventario_Consolidado.md).
* **¿Para qué leerlo?**: Para consultar qué componentes tenemos en el laboratorio, qué ferretería falta comprar en Salta, y cómo se calculan las rampas de aceleración.

#### 📊 `02_Hito2_Instrumentacion_Sensores/` *(¡Carpeta en Operación Actual!)*
* **Contenido**: 
  - 🌟 **[firmware_planta/](./02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/firmware_planta/)**: **Firmware Oficial de Producción**. NVS Flash persistente, auto-calibración en marcha con probeta, rampa S-Curve, y Web SCADA.
  - 🩺 **[Diagnostico_y_Resolucion_Problemas_Instrumentacion.md](./02_Hito2_Instrumentacion_Sensores/01_Guias_Montaje_y_Calibracion/Diagnostico_y_Resolucion_Problemas_Instrumentacion.md)**: **Documento para tesis**. Análisis matemático y físico de la saturación a 81 Hz (835.5 mL/min), solución al zumbido de 50 Hz/100 Hz, filtro pasabajos RC y resolución del cruce de canales.
  - La guía de conexión física: [Guia_Montaje_Placa_Filtrado_FrontEnd.md](./02_Hito2_Instrumentacion_Sensores/01_Guias_Montaje_y_Calibracion/Guia_Montaje_Placa_Filtrado_FrontEnd.md).
  - El esquema gráfico interactivo: [esquema_conexion_borneras.html](./02_Hito2_Instrumentacion_Sensores/01_Guias_Montaje_y_Calibracion/esquema_conexion_borneras.html).
  - 📁 [Datos/](./02_Hito2_Instrumentacion_Sensores/Datos/): Registros CSV reales exportados en banco de pruebas y planilla Excel de calibración a 50/72 RPM.
* **¿Para qué leerlo?**: Es la guía práctica para el trabajo diario en el laboratorio, cableado de la segunda bornera, calibración de instrumentos y análisis de fallas para la redacción de la tesis.

#### 💻 `Capitulo_4_Programacion_Firmware/` *(¡Nuevo! Documentación y Firmware para Tesis)*
* **Contenido**: 
  - Simulación de filtros pasabajos RC en **LTspice** y script Python de análisis de respuesta temporal.
  - Esquemas interactivos HTML de conexionado en protoboard y shield.
  - Scripts de automatización en consola (`.bat` para compilar en 2.8s con PlatformIO CLI).
  - Bitácoras cronológicas completas de desarrollo y calibración experimental.
* **¿Para qué leerlo?**: Fuente principal para redactar el Capítulo 4 de la Memoria de Tesis de Grado (UNSa).

#### 🌪️ `03_Hito3_Reactor_Sedimentador_Agitador/`
* **Contenido**: Pretratamiento por coagulación-floculación (Jar Test), control del motor de agitación con driver L298N, cálculo del gradiente de velocidad ($G$), paleta normalizada y boya de nivel en acero inoxidable.
* **¿Para qué leerlo?**: Para preparar los ensayos de dosificación química y acondicionamiento del agua turbia antes de pasar a la membrana.

#### 🌐 `04_Hito4_Integracion_Automatizacion_IoT/`
* **Contenido**: Automatización centralizada en FreeRTOS, enclavamientos de seguridad por Presión Transmembrana ($\text{TMP} \le 0.50\text{ bar}$) y corte por marcha en seco.
* **¿Para qué leerlo?**: Para entender cómo el ESP32 coordina todos los subsistemas de forma autónoma sin intervención humana.

#### 🧪 `05_Hito5_Ensayos_Membrana_VidaUtil/`
* **Contenido**: Modelado fenomenológico del cartucho Fresenius FX100, Ley de Darcy modificada por temperatura (TCF), mecanismos de ensuciamiento (*cake layer*, *pore blocking*) y planillas modelo para la tesis.
* **¿Para qué leerlo?**: Para redactar los capítulos de resultados experimentales, curvas de permeabilidad y conclusiones de la tesis de grado.

#### 📦 `Archivado/`
* **Contenido**: Códigos preliminares de prueba, librerías intermedias y documentación obsoleta.
* **¿Para qué sirve?**: Historial de respaldo; no debe utilizarse en las pruebas activas de laboratorio.

---

## 📌 5. Asignación Rápida de Pines del ESP32

| Pin ESP32 | Función Operativa | Tipo de Señal | Conexión Física de Destino |
| :---: | :--- | :--- | :--- |
| **GPIO 18** | Pulsos STEP Bomba (LEDC) | Salida Digital | Driver Leadshine DM860 (`PUL+` / Cátodo Común) |
| **GPIO 19** | Dirección de Giro (CW/CCW) | Salida Digital | Driver Leadshine DM860 (`DIR+` / Cátodo Común) |
| **GPIO 14** | Caudalímetro FEED (Entrada) | Interrupción IRAM | Placa 2 Front-End (Borne P14 filtrado con RC) |
| **GPIO 27** | Caudalímetro PERMEADO | Interrupción IRAM | Placa 2 Front-End (Borne P27 filtrado con RC) |
| **GPIO 4** | Sensor Temperatura DS18B20 | Bus 1-Wire Digital | Placa 2 Front-End (Borne P4 con Pull-Up $4.7\text{ k}\Omega$) |
| **GPIO 32** | Boya de Nivel Inox (Corte) | Entrada Digital Pull-Up | Placa 2 Front-End (Borne P32 con capacitor 100nF) |
| **GPIO 21** | I2C SDA (Datos) | Bus I2C a 400 kHz | Conversor ADC 16-Bit ADS1115 (Pin SDA) |
| **GPIO 22** | I2C SCL (Reloj) | Bus I2C a 400 kHz | Conversor ADC 16-Bit ADS1115 (Pin SCL) |
| **GPIO 16/17** | Sentido Agitador Jar Test | Salidas Digitales | Driver L298N (Pines IN1 e IN2) |
| **GPIO 5** | Velocidad Agitador (PWM) | Salida PWM 1 kHz | Driver L298N (Pin ENA) |

---

> 📖 **Nota Metodológica**: Cada hito posee su propio archivo `README.md` interno donde se profundiza en las ecuaciones físico-químicas, diagramas de flujo y rutinas de calibración específicas.
