# 📦 INVENTARIO CONSOLIDADO DE COMPRAS Y HARDWARE
## Planta Piloto de Ultrafiltración FX100 — Tesis de Ingeniería Industrial

---

## 🟢 1. Componentes Físicos en Mano (Disponibles en el Laboratorio)

| Componente | Cantidad | Especificaciones / Modelo | Función Específica en la Planta | Hito |
| :--- | :---: | :--- | :--- | :---: |
| **Driver Leadshine DM860** | 1 | Microstepping 1600 P/R, AC/DC | Driver de potencia con aislamiento optoacoplado para el NEMA 34. | **Hito 2** |
| **Motor Paso a Paso NEMA 34** | 1 | Bipolar 4.0 Nm, 4 cables | Accionamiento de alto torque para la bomba peristáltica MBP-2000. | **Hito 2** |
| **Bomba Peristáltica MBP-2000** | 1 | Cabezal con manguera de silicona | Impulsión estéril de precisión hacia la membrana ($0 \text{ a } 0.5\text{ L/min}$). | **Hito 2** |
| **Transformador AC** | 1 | Primario 220V ➔ Secundario AC | Alimentación exclusiva para los bornes `AC/AC` del driver DM860. | **Hito 2** |
| **Driver Puente H L298N** | 1 | Doble puente H 2A con disipador | Modulación PWM de velocidad y sentido para la paleta del sedimentador. | **Hito 4** |
| **Sensor de Nivel de Acero Inox.** | 1 | Boya flotante 100mm (N/C - N/A) | Detección de nivel mínimo en el reactor para corte automático por marcha en seco. | **Hito 4** |
| **Sensor de Temp. DS18B20** | 1 | Sonda sumergible digital OneWire | Mide temperatura ($^\circ\text{C}$) para calcular la viscosidad y el factor TCF de Darcy. | **Hito 3** |
| **Caudalímetros YF-S401** | 2 | $0.3 \text{ a } 6\text{ L/min}$ (Efecto Hall) | Medición de flujo en línea: Permeado ($Q_p$) y Retentado/Alimentación ($Q_c$). | **Hito 3** |
| **Conversor ADC ADS1115** | 1 | 16-Bit I2C de 4 canales ($A_0 - A_3$) | Lectura de altísima resolución sin ruido para presiones y calidad de agua. | **Hito 3/5** |
| **Módulo Step-Down LM2596** | 1 | $3\text{A}$ CC/CV Regulable | Baja los $12\text{V DC}$ a $5.00\text{V DC}$ estables para alimentar el ESP32. | **Base** |
| **Fuente de Alimentación 12V** | 1 | $12\text{V DC} - 1.5\text{A}$ ($18\text{W}$) | Alimentación de corriente continua para L298N, LM2596 y sensores. | **Base** |
| **Conectores Empalme Rápido** | 100 | Bloques dobles a presión | Conexiones firmes, seguras y limpias para masa común y líneas de poder. | **Base** |
| **Base Shield Expansión ESP32** | 1 | USB-C / Micro-USB 38 pines | Placa de soporte y expansión para el ESP32. | **Base** |
| **Membrana Fresenius FX100** | 1 | Capilares Polisulfona ($A_m=2.2\text{ m}^2$) | Módulo central de ultrafiltración ($0.01\,\mu\text{m}$) para potabilización. | **Hito 5/6** |

---

## 🚚 2. Componentes en Tránsito (Comprados)

| Componente | Cantidad | Función en la Planta |
| :--- | :---: | :--- |
| **ESP32 NodeMCU (38 Pines USB-C)** | 1 | Microcontrolador central (Wi-Fi, WebServer, OTA, LEDC Timer de hardware). |
| **Sensor TDS (Calidad de Agua)** | 1 | Sonda sumergible analógica para medir Sólidos Totales Disueltos ($\text{ppm}$) según Código Alimentario Argentino. |

---

## 🟡 3. Lo que Resta Comprar para el Cierre de la Planta

1. **3x Transductores de Presión Hidráulica ($0 \text{ a } 1.2\text{ bar}$ / $0 \text{ a } 17\text{ PSI}$, rosca G1/4")**:
   * Para medir $P_1$ (Entrada membrana), $P_2$ (Retentado) y $P_3$ (Permeado) y calcular la Presión Transmembrana ($\text{TMP} \le 0.50\text{ atm}$).
2. **Accesorios Hidráulicos de Ferretería**:
   * Manguera de silicona cristal (diámetro interior $6\text{ mm}$ u $8\text{ mm}$).
   * 3 Tees plásticas/latón rosca G1/4" con espigas.
   * Válvulas miniatura esféricas o de estrangulamiento manual para ajuste del retentado.
