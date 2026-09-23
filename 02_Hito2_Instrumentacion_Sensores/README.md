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
  * 🌊 **[`Guia_Subhito2_1_Caudalimetros.md`](./01_Guias_Montaje_y_Calibracion/Guia_Subhito2_1_Caudalimetros.md)**: Conexión, factor K (98 pulsos/L), monitoreo serie y calibración gravimétrica del YF-S401.
  * 🚰 **[`Guia_Montaje_Hidraulico_Sensores.md`](./01_Guias_Montaje_y_Calibracion/Guia_Montaje_Hidraulico_Sensores.md)**: Instalación física de caudalímetros y sondas en tubería con entregable y checklist.
  * 📐 **[`Guia_Calibracion_ADC_ADS1115_y_Sensores.md`](./01_Guias_Montaje_y_Calibracion/Guia_Calibracion_ADC_ADS1115_y_Sensores.md)**: Fórmulas de conversión matemática, resolución de 16 bits y compensación térmica con entregable y checklist.
* 📁 **[`02_Firmware_Test_Sensores/`](./02_Firmware_Test_Sensores/)**:
  * 🌊 **[`subhito2_1_caudalimetros/subhito2_1_caudalimetros.ino`](./02_Firmware_Test_Sensores/subhito2_1_caudalimetros/subhito2_1_caudalimetros.ino)**: Firmware modular de prueba para Subhito 2.1 con interrupciones por hardware y comandos interactivos de bomba MBP-2000.
  * 🚀 **[`subhito2_2_v2/subhito2_2_v2.ino`](./02_Firmware_Test_Sensores/subhito2_2_v2/subhito2_2_v2.ino)**: **Firmware Modular V2 (Cátodo Común + Web SCADA)**: Arquitectura orientada a objetos (Bomba y Caudalímetro) con triple filtro anti-ruido, Wi-Fi dual (AP + STA) y servidor HTTP asíncrono.
  * 💻 **[`firmware_sensores_test/firmware_sensores_test.ino`](./02_Firmware_Test_Sensores/firmware_sensores_test/firmware_sensores_test.ino)**: Sketch integral multivariable de adquisición de datos en tiempo real y transmisión serie en formato CSV.



