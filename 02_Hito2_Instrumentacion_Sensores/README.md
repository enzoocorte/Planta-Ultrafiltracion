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
│ 📊 SUBHITO 2.4: Instrumentación Manométrica Primaria (P1, P2) e Integración Híbrida SCADA│
│    • Manómetros mecánicos amortiguados con baño de glicerina (inmunes a EMI, sin deriva).│
│    • Carga de P1 (Entrada) y P2 (Retentado) en vivo en SCADA web (http://192.168.4.1). │
│    • Cálculo automático en tiempo real de TMP = (P1 + P2)/2 - P3 en módulo darcy.h.    │
│    • Integración en cada fila del CSV: Vogel TCF, J20 (LMH), R_total y R_torta (m^-1). │
│    • Enclavamiento de seguridad: válvula mecánica de alivio a 0.70 bar + alertas SCADA. │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 🎯 Entregable Maestro del Hito 2
* **Módulo de telemetría de instrumentación operando al 100%**: Adquisición periódica y continua (1 Hz) de Caudal de Entrada ($Q_1$), Caudal de Permeado ($Q_p$), Temperatura del agua ($^\circ\text{C}$), Sólidos Totales Disueltos ($\text{ppm}$), presiones manométricas ($P_1, P_2$) y Presión Transmembrana ($\text{TMP}$) transmitidos sin ruido eléctrico.

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

### Fase 2.4: Instrumentación Manométrica Primaria y SCADA Darcy
- [ ] Manómetros con baño de glicerina instalados en impulsión ($P_1$) y retentado ($P_2$).
- [ ] Válvula de alivio de seguridad calibrada a $0.70\text{ bar}$ instalada en la impulsión.
- [ ] Carga en vivo de $P_1, P_2$ y Temperatura en la interfaz web del SCADA (`http://192.168.4.1`).
- [ ] Ecuación de TMP calculando en tiempo real: $\text{TMP} = \frac{P_1 + P_2}{2} - P_3$.
- [ ] Exportación en datalogger CSV de $J_{20}$, TCF de Vogel, $R_{\text{total}}$ y $R_{\text{torta}}$ en cada muestra.
- [ ] Alerta de sobrepresión en SCADA comprobada ante $P_1 \ge 0.60\text{ bar}$ o $\text{TMP} \ge 0.45\text{ bar}$.

---

## 📂 Estructura y Navegación de Subcarpetas de este Hito:

* 📁 **[`01_Guias_Montaje_y_Calibracion/`](./01_Guias_Montaje_y_Calibracion/)**:
  * 📊 **[`Guia_Subhito2_4_Manometros_Glicerina_y_Darcy_SCADA.md`](./01_Guias_Montaje_y_Calibracion/Guia_Subhito2_4_Manometros_Glicerina_y_Darcy_SCADA.md)**: **Guía oficial Subhito 2.4**. Montaje de manómetros Bourdon con glicerina ($P_1, P_2$), supresión de transductores analógicos genéricos/ADS1115, carga asistida en SCADA web y cálculo de Darcy en tiempo real.
  * 🩺 **[`Diagnostico_y_Resolucion_Problemas_Instrumentacion.md`](./01_Guias_Montaje_y_Calibracion/Diagnostico_y_Resolucion_Problemas_Instrumentacion.md)**: **Documento formal para tesis**. Diagnóstico de causas raíz del ruido de 81 Hz (835.5 mL/min), filtro software, solución RC en Placa 2 y protocolo de ensayos.
  * 🔌 **[`Guia_Montaje_Placa_Filtrado_FrontEnd.md`](./01_Guias_Montaje_y_Calibracion/Guia_Montaje_Placa_Filtrado_FrontEnd.md)**: Manual de conexionado físico paso a paso de las 2 borneras ZS-1057 (Placa 1 Master ESP32 + Placa 2 Acondicionamiento RC).
  * 📘 **[`Manual_Conexionado_Sensores_ESP32_Ultrafiltracion.docx`](./01_Guias_Montaje_y_Calibracion/Manual_Conexionado_Sensores_ESP32_Ultrafiltracion.docx)**: Manual técnico consolidado en Word con pinouts, borneras y esquemas.
  * 📈 **[`Simulaciones_Filtro_RC/`](./01_Guias_Montaje_y_Calibracion/Simulaciones_Filtro_RC/)**: Simulación en **LTspice** (`simulacion_filtro_caudalimetro.asc`), script Python y curvas de atenuación.
  * 🖥️ **[`Esquemas_Conexionado_HTML/`](./01_Guias_Montaje_y_Calibracion/Esquemas_Conexionado_HTML/)**: Colección de planos interactivos SVG (borneras ZS-1057, protoboard, capacitor de desacoplo y nodo pull-up).
  * 📝 **[`Bitacoras_Calibracion/`](./01_Guias_Montaje_y_Calibracion/Bitacoras_Calibracion/)**: Registro cronológico de sesiones de laboratorio de Owen, ensayos en probeta y matriz de fallas resueltas.
  * 📘 **[`Documentacion_Tecnica/`](./01_Guias_Montaje_y_Calibracion/Documentacion_Tecnica/)**: Guía de arquitectura de programación del ESP32, prompts de diseño y cabeceras de referencia.
  * 🌊 **[`Guia_Subhito2_1_Caudalimetros.md`](./01_Guias_Montaje_y_Calibracion/Guia_Subhito2_1_Caudalimetros.md)**: Conexión, factor K (205.98 Hz/(L/min)), monitoreo serie y calibración gravimétrica del YF-S401.
  * 🚰 **[`Guia_Montaje_Hidraulico_Sensores.md`](./01_Guias_Montaje_y_Calibracion/Guia_Montaje_Hidraulico_Sensores.md)**: Instalación física de caudalímetros y sondas en tubería con entregable y checklist.

* 📁 **[`02_Firmware_Test_Sensores/`](./02_Firmware_Test_Sensores/)**:
  * 🌟 **[`Subhito2_2_v6/`](./02_Firmware_Test_Sensores/Subhito2_2_v6/)**: **FIRMWARE ACTIVO DE PRODUCCIÓN (GRABADO EN ESP32)**:
    * ⭐ **[`EN_USO_firmware_planta/`](./02_Firmware_Test_Sensores/Subhito2_2_v6/EN_USO_firmware_planta/)**: Integración de Manómetros Bourdon de Glicerina, endpoint `/set_presion`, cálculo continuo de Ley de Darcy (`darcy.h`), factor TCF de Vogel, exportación CSV de 8 columnas metrológicas y panel visual interactivo SCADA.
  * 🚀 **[`subhito2_2_v4/`](./02_Firmware_Test_Sensores/subhito2_2_v4/)**: Firmware previo refactorizado tras auditoría cruzada de 4 IAs.
  * 🌟 **[`subhito2_2_v3/`](./02_Firmware_Test_Sensores/subhito2_2_v3/)**: Versión previa de consolidación de banco.
  * 🚀 **[`subhito2_2_v2/`](./02_Firmware_Test_Sensores/subhito2_2_v2/)**: Firmware modular base previo con arquitectura C++ (Bomba y Caudalímetro).
  * 🌊 **[`subhito2_1_caudalimetros/`](./02_Firmware_Test_Sensores/subhito2_1_caudalimetros/)**: Firmware básico inicial para Subhito 2.1.

* 📁 **[`Datos/`](./Datos/)**:
  * 📊 **[`CALIBRACION_CAUDALIMETROS_PROBETA_50RPM_72RPM.xlsx`](./Datos/CALIBRACION_CAUDALIMETROS_PROBETA_50RPM_72RPM.xlsx)**: Planilla oficial de calibración de probeta de Owen a 50 RPM y 72 RPM con factores K resultantes.
  * 📑 **[`datos_planta_uf_2026-09-30.csv`](./Datos/datos_planta_uf_2026-09-30.csv)**: Telemetría cruda en banco a 50–94 RPM.
  * 📑 **[`datos_planta_uf_2026-09-30 (1).csv`](./Datos/datos_planta_uf_2026-09-30 (1).csv)**: Ensayo de verificación a 25, 36, 50, 74 y 80 RPM.

---

## 📊 Estado de Avance Actual

| Subhito / Actividad | Estado | Porcentaje | Observaciones |
| :--- | :---: | :---: | :--- |
| **Subhito 2.1: Caudalímetros YF-S401** | ✅ Completado | 100% | Calibrado con probeta ($K = 205.98\text{ Hz/(L/min)}$) y filtro anti-EMI. |
| **Subhito 2.2: Temperatura DS18B20 & TCF** | ✅ Completado | 100% | Integrado en `darcy.h` con viscosidad de Vogel. |
| **Subhito 2.3: Calidad de Agua TDS / ADS1115** | ✅ Completado | 100% | Protocolo I2C verificado a 0x48. |
| **Subhito 2.4: Manómetros Glicerina & Darcy SCADA**| ✅ Completado | 100% | Implementado en firmware v6 y panel SCADA web. |

---

## 🏁 Criterios de Aceptación y Entregables para Pasar al Hito 3 y 4

1. [x] Medición de caudal en tiempo real libre de ruido EMI ($81\text{ Hz}$ eliminado por filtro RC + software).
2. [x] Presión manométrica de entrada ($P_1$) y retentado ($P_2$) legible con precisión de $\pm 16\text{ mbar}$ sobre manómetros Bourdon amortiguados.
3. [x] Ley de Darcy calculando en vivo en el firmware ESP32 y exportando en cada fila del CSV.
4. [ ] **Habilitación formal:** Interconexión con el reactor sedimentador del **Hito 3** y montaje central en el **Hito 4**.




