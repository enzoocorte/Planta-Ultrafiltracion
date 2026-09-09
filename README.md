# 💧 PLANTA PILOTO DE ULTRAFILTRACIÓN INDUSTRIAL (FX100) & REACTOR DE COAGULACIÓN-SEDIMENTACIÓN
## Proyecto de Tesis de Grado en Ingeniería Industrial — Automatización, Control IoT & Modelado de Membranas

---

## 👥 Equipo del Proyecto & Contexto Académico
* **Codirector de Tesis**: **Ing. Enzo** *(Investigador Doctoral — Tesis Doctoral en Tratamiento de Aguas y Procesos de Separación por Membranas)*
* **Tesistas de Grado (Ingeniería Industrial)**: **Antonella Guitián** & **Owen Cañizares**
* **Asesor Técnico y Arquitectura de Control**: Antigravity AI
* **Microcontrolador Principal**: ESP32 NodeMCU (38 Pines USB-C, Dual Core 240 MHz)
* **Membrana de Ultrafiltración**: Fresenius Medical Care FX100 (Polisulfona / Helixone®, $A_m = 2.2\text{ m}^2$)

> 📘 **DOCUMENTO PRINCIPAL**: Para una explicación exhaustiva de la arquitectura del sistema, el flujo de proceso y la guía detallada de qué se busca en cada hito, consultar el **[MANUAL_GENERAL_Y_GUIA_DE_NAVEGACION.md](./MANUAL_GENERAL_Y_GUIA_DE_NAVEGACION.md)**.

---

## 🧭 Estructura Modular de los 6 Hitos de Tesis

Este repositorio está organizado en **6 carpetas modulares independientes**, estructuradas cronológicamente para que el equipo pueda avanzar paso a paso, con código limpio, documentación técnica, esquemas y simulaciones:

```
SistemaUF/
├── 📁 01_Hito1_Control_Accionamiento_NEMA34_DM860/ # ⚡ HITO 1: Control de Accionamiento de Potencia (NEMA 34 + DM860 + ESP32)
├── 📁 02_Hito2_Bomba_Peristaltica_NEMA34/          # 🌀 HITO 2: Bomba MBP-2000, Rampa de Caudal (mL/rev), Web y OTA
├── 📁 03_Hito3_Instrumentacion_Sensores/           # 📊 HITO 3: Caudalímetros YF-S401, Sonda TDS, DS18B20 y ADS1115
├── 📁 04_Hito4_Reactor_Sedimentador_Agitador/      # 🌪️ HITO 4: Driver L298N, paleta PWM, boya de nivel y turbidez
├── 📁 05_Hito5_Integracion_Automatizacion_IoT/     # 🌐 HITO 5: Automatización completa, seguridad TMP ≤ 0.50 atm y SCADA
└── 📁 06_Hito6_Ensayos_Membrana_VidaUtil/          # 🧪 HITO 6: Protocolo de ensayos, Ley de Darcy, Fouling y Tesis
```

---

## 🗺️ Mapa de Ruta del Proyecto

```mermaid
flowchart LR
    H1["⚡ HITO 1<br>Potencia NEMA 34"] --> H2["🌀 HITO 2<br>Bomba & Dashboard"]
    H2 --> H3["📊 HITO 3<br>Sensores & TDS"]
    H3 --> H4["🌪️ HITO 4<br>Sedimentador L298N"]
    H4 --> H5["🌐 HITO 5<br>Integración & IoT"]
    H5 --> H6["🧪 HITO 6<br>Ensayos & Tesis"]
```

---

## ⚡ Resumen Rápido de Pines del ESP32 (38 Pines)

| Pin ESP32 | Función en Planta | Tipo de Señal | Componente Asociado |
| :--- | :--- | :--- | :--- |
| **GPIO 18** | Pulsos STEP (LEDC Hardware) | Salida Digital PWM | Driver Leadshine DM860 (PUL+) |
| **GPIO 19** | Dirección de Giro (CW/CCW) | Salida Digital | Driver Leadshine DM860 (DIR+) |
| **GPIO 4** | Velocidad Paleta Agitadora | Salida PWM | Driver L298N (Pin ENA) |
| **GPIO 16** | Sentido de Giro Agitador A | Salida Digital | Driver L298N (Pin IN1) |
| **GPIO 17** | Sentido de Giro Agitador B | Salida Digital | Driver L298N (Pin IN2) |
| **GPIO 32** | Boya de Nivel (Seguridad) | Entrada Digital Pull-Up | Sensor Nivel Acero Inoxidable |
| **GPIO 34** | Sensor Temperatura | Protocolo OneWire | Sonda Sumergible DS18B20 |
| **GPIO 27** | Caudal Permeado ($Q_p$) | Interrupción por Pulsos | Caudalímetro YF-S401 |
| **GPIO 14** | Caudal Retentado ($Q_c$) | Interrupción por Pulsos | Caudalímetro YF-S401 |
| **GPIO 21** | I2C SDA (Datos) | Bus de Comunicación | Conversor ADS1115 (16 Bits) |
| **GPIO 22** | I2C SCL (Reloj) | Bus de Comunicación | Conversor ADS1115 (16 Bits) |

---

## 🚀 Cómo Empezar a Trabajar
1. Navega a la carpeta del hito en el que estés trabajando (ej. `02_Hito2_Bomba_Peristaltica_NEMA34`).
2. Lee el archivo `README.md` de esa carpeta para comprender los fundamentos físicos y las conexiones.
3. Abre el archivo de firmware `.ino` en Arduino IDE y súbelo al ESP32.
