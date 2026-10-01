# 📊 HITO 2: Instrumentación y Sensores Disponibles (Caudal, TDS, Temperatura y ADC)

Este hito tiene como objetivo instrumentar la planta piloto con los sensores físicos ya disponibles en el laboratorio, aprender a leer señales digitales y analógicas en el ESP32, comprender el funcionamiento del conversor ADC ADS1115 de 16 bits y visualizar toda la telemetría en la computadora.

---

## 🗺️ Hoja de Ruta Modular del Hito 2 (Subhitos Secuenciales)

Para avanzar con método científico y validación segura paso a paso en el banco, el Hito 2 se desglosa en **4 subhitos independientes**:

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                        HITO 2: INSTRUMENTACIÓN Y SENSORES                              │
├────────────────────────────────────────────────────────────────────────────────────────┤
│ 🌊 SUBHITO 2.1: Caudalímetros de Microflujo (Efecto Hall YF-S401)                     │
│    • Conexión de pulsos por interrupción en GPIO 14 (Feed) y GPIO 27 (Permeado).      │
│    • Calibración del factor K (98 pulsos/L) y contraste con caudal de bomba MBP-2000. │
│    • Acumulación de volumen en Litros y verificación de estanqueidad sin fugas.       │
├────────────────────────────────────────────────────────────────────────────────────────┤
│ 🌡️ SUBHITO 2.2: Sensor de Temperatura Sumergible (DS18B20 OneWire)                   │
│    • Lectura digital precisa en GPIO 34 con resistencia pull-up de 4.7 kΩ a 3.3V.     │
│    • Cálculo en tiempo real del factor térmico Darcy: TCF = exp[0.0239 * (20 - T)].   │
├────────────────────────────────────────────────────────────────────────────────────────┤
│ 🧪 SUBHITO 2.3: Conversor ADC ADS1115 (16 Bits) y Sonda de Calidad de Agua (TDS)      │
│    • Comunicación digital I2C en GPIO 21 (SDA) y GPIO 22 (SCL) en dirección 0x48.    │
│    • Lectura analógica en Canal A0, conversión a voltaje y ppm de sales disueltas.   │
│    • Compensación de conductividad por temperatura normalizada a 25°C.                │
├────────────────────────────────────────────────────────────────────────────────────────┤
│ 📊 SUBHITO 2.4: Transductores de Presión (P1, P2, P3) y Presión Transmembrana (TMP)   │
│    • Conexión a Canales A1 (Feed), A2 (Retentado) y A3 (Permeado) del mismo ADS1115.  │
│    • Algoritmo de cálculo en tiempo real: TMP = (P1 + P2)/2 - P3.                     │
│    • Enclavamiento mandatorio: parada de emergencia de bomba si TMP > 0.50 atm.       │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 🎯 Entregable Maestro del Hito 2
* **Módulo de telemetría de instrumentación operando al 100%**: Adquisición periódica y continua (1 Hz) de Caudal de Entrada ($Q_1$), Caudal de Permeado ($Q_p$), Temperatura del agua ($^\circ\text{C}$), Sólidos Totales Disueltos ($\text{ppm}$) y Presión Transmembrana ($\text{TMP}$) transmitidos sin ruido eléctrico.

---

## 📋 Lista de Verificación Maestra (Checklist del Hito 2)
### Fase 2.1: Caudalímetros
- [ ] Caudalímetro(s) YF-S401 intercalado(s) en la tubería con la flecha de flujo correctamente orientada.
- [ ] Conexión a bornera: Alimentación 5V/VIN, GND y señal amarilla a `GPIO 14` / `GPIO 27`.
- [ ] Firmware de prueba cargado y pulsos detectados al circular agua.
- [ ] Calibración gravimétrica contrastada con probeta y con la consigna volumétrica de la bomba MBP-2000.

### Fase 2.2: Temperatura
- [ ] Sonda DS18B20 conectada a `GPIO 34` con resistencia de $4.7\text{ k}\Omega$ a 3.3V.
- [ ] Lectura estable de temperatura ambiente y en agua reportando en tiempo real.

### Fase 2.3: Conversor ADS1115 y TDS
- [ ] Bus I2C verificado en dirección `0x48` (`GPIO 21` SDA y `GPIO 22` SCL).
- [ ] Canal A0 leyendo la sonda TDS con respuesta coherente en agua de canilla y agua destilada.

### Fase 2.4: Presión y TMP
- [ ] Transductores P1, P2 y P3 conectados a los canales A1, A2 y A3 del ADS1115.
- [ ] Ecuación de TMP calculando en tiempo real y disparando enclavamiento si $\text{TMP} > 0.50\text{ atm}$.

---

## 📂 Estructura y Navegación de Subcarpetas de este Hito:

* 📁 **[`01_Guias_Montaje_y_Calibracion/`](./01_Guias_Montaje_y_Calibracion/)**:
  * 🩺 **[`Diagnostico_y_Resolucion_Problemas_Instrumentacion.md`](./01_Guias_Montaje_y_Calibracion/Diagnostico_y_Resolucion_Problemas_Instrumentacion.md)**: **Documento formal para tesis**. Diagnóstico de causas raíz del ruido de 81 Hz (835.5 mL/min), demostración matemática del filtro software, solución RC en Placa 2, corrección de inversión de canales y protocolo de ensayos.
  * 🔌 **[`Guia_Montaje_Placa_Filtrado_FrontEnd.md`](./01_Guias_Montaje_y_Calibracion/Guia_Montaje_Placa_Filtrado_FrontEnd.md)**: Manual de conexionado físico paso a paso de las 2 borneras ZS-1057 (Placa 1 Master ESP32 + Placa 2 Acondicionamiento RC).
  * 📈 **[`Simulaciones_Filtro_RC/`](./01_Guias_Montaje_y_Calibracion/Simulaciones_Filtro_RC/)**: Simulación en **LTspice** (`simulacion_filtro_caudalimetro.asc`), script de modelado en Python y curvas de atenuación de ruido.
  * 🖥️ **[`Esquemas_Conexionado_HTML/`](./01_Guias_Montaje_y_Calibracion/Esquemas_Conexionado_HTML/)**: Colección de planos interactivos SVG (borneras ZS-1057, protoboard, capacitor de desacoplo y nodo pull-up).
  * 📝 **[`Bitacoras_Calibracion/`](./01_Guias_Montaje_y_Calibracion/Bitacoras_Calibracion/)**: Registro cronológico de sesiones de laboratorio de Owen, ensayos en probeta y matriz de fallas resueltas.
  * 📘 **[`Documentacion_Tecnica/`](./01_Guias_Montaje_y_Calibracion/Documentacion_Tecnica/)**: Guía de arquitectura de programación del ESP32, prompts de diseño y cabeceras de referencia.
  * 🌊 **[`Guia_Subhito2_1_Caudalimetros.md`](./01_Guias_Montaje_y_Calibracion/Guia_Subhito2_1_Caudalimetros.md)**: Conexión, factor K (98 pulsos/L), monitoreo serie y calibración gravimétrica del YF-S401.
  * 🚰 **[`Guia_Montaje_Hidraulico_Sensores.md`](./01_Guias_Montaje_y_Calibracion/Guia_Montaje_Hidraulico_Sensores.md)**: Instalación física de caudalímetros y sondas en tubería con entregable y checklist.
  * 📐 **[`Guia_Calibracion_ADC_ADS1115_y_Sensores.md`](./01_Guias_Montaje_y_Calibracion/Guia_Calibracion_ADC_ADS1115_y_Sensores.md)**: Fórmulas de conversión matemática, resolución de 16 bits y compensación térmica con entregable y checklist.

* 📁 **[`02_Firmware_Test_Sensores/`](./02_Firmware_Test_Sensores/)**:
  * 🌟 **[`firmware_planta/`](./02_Firmware_Test_Sensores/firmware_planta/)**: **FIRMWARE OFICIAL DE PRODUCCIÓN (Grabado en ESP32)**: Auto-calibración en marcha con probeta, persistencia Flash NVS (`Preferences.h`), filtro anti-ruido optimizado a 2000 µs, datalogger multi-sesión (600 muestras), rampa S-Curve progresiva, y Web SCADA en SoftAP puro (`192.168.4.1`).
  * 💻 **[`firmware_esp32_platformio/`](./02_Firmware_Test_Sensores/firmware_esp32_platformio/)**: Entorno de compilación rápida para desarrolladores con PlatformIO Core CLI.
  * ⚡ **[`scripts_compilacion_rapida/`](./02_Firmware_Test_Sensores/scripts_compilacion_rapida/)**: Accesos directos `.bat` para compilar, subir y abrir el monitor serie en segundos.
  * 🚀 **[`subhito2_2_v2/`](./02_Firmware_Test_Sensores/subhito2_2_v2/)**: Firmware modular base previo con arquitectura C++ (Bomba y Caudalímetro).
  * 🌊 **[`subhito2_1_caudalimetros/`](./02_Firmware_Test_Sensores/subhito2_1_caudalimetros/)**: Firmware básico de prueba para Subhito 2.1 con interrupciones por hardware.
  * 💻 **[`firmware_sensores_test/`](./02_Firmware_Test_Sensores/firmware_sensores_test/)**: Sketch integral multivariable de adquisición de datos en tiempo real y transmisión serie.

* 📁 **[`Datos/`](./Datos/)**:
  * 📊 **[`CALIBRACION_CAUDALIMETROS_PROBETA_50RPM_72RPM.xlsx`](./Datos/CALIBRACION_CAUDALIMETROS_PROBETA_50RPM_72RPM.xlsx)**: Planilla oficial de calibración de probeta de Owen a 50 RPM y 72 RPM con factores K resultantes.
  * 📑 **[`datos_planta_uf_2026-09-30.csv`](./Datos/datos_planta_uf_2026-09-30.csv)**: Telemetría cruda en banco a 50–94 RPM donde se diagnosticó la inversión física de sensores.
  * 📑 **[`datos_planta_uf_2026-09-30 (1).csv`](./Datos/datos_planta_uf_2026-09-30 (1).csv)**: Ensayo de verificación a 25, 36, 50, 74 y 80 RPM.




