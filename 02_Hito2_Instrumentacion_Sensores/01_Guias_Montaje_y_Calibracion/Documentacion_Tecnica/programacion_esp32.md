# PROGRAMACIÓN Y CONTROL EMBEBIDO ESP32
**Módulo de Ultrafiltración FX100 – Bomba Peristáltica MBP-2000 / NEMA 34**  
*Universidad Nacional de Salta (UNSa) – Proyecto Final de Grado*

---

## 1. Arquitectura de Hardware y Pines del ESP32 NodeMCU-32S

### A. Conexión Eléctrica en Ánodo Común (+5V / VIN)
Para garantizar la saturación y conmutación rápida de los optoacopladores industriales del driver DM860:
- **Borne `PUL+` (DM860):** Conectado a pin **`VIN` (5V)** del ESP32.
- **Borne `DIR+` (DM860):** Conectado a pin **`VIN` (5V)** del ESP32.
- **Borne `PUL-` (DM860):** Conectado a **`GPIO 18`** (Pulso activo por nivel bajo `LOW`, ancho = $50\,\mu\text{s}$).
- **Borne `DIR-` (DM860):** Conectado a **`GPIO 19`** (`HIGH` = Filtración Horaria, `LOW` = Retrolavado Antihorario).
- **LED Indicador:** **`GPIO 2`** (Enciende durante marcha).
- **Bornes `ENA+` y `ENA-`:** Desconectados (habilitación permanente).

### B. Driver DM860 y Motor NEMA 34
- **Microswitches DM860:**
  - Corriente: Ajustada a ~2,4 A a 3,14 A RMS (`SW1: ON`, `SW2: OFF`, `SW3: ON`).
  - Corriente de reposo: *Half Current* (`SW4: OFF`) para control térmico.
  - Resolución: 1600 pulsos/rev – 1/8 de paso (`SW5: ON`, `SW6: OFF`, `SW7: ON`, `SW8: ON`).
- **Conexión Bipolar Serie (3 A):**
  - Fase A: A+ (Rojo), A- (Negro) | Empalme aislado: Amarillo + Azul.
  - Fase B: B+ (Blanco), B- (Verde) | Empalme aislado: Naranja + Marrón.

---

## 2. Lógica Cinemática, Rampa de Aceleración y Ecuación de Caudal

- **Generador de pulsos:** Asíncrono no bloqueante mediante máquina de estados basada en `micros()` con reinicio limpio de temporizador (`tiempoAnterior = t;`).
- **Rango ampliado de velocidad (hasta 200+ RPM):**
  - $3000\,\mu\text{s} \rightarrow \sim 12{,}3\text{ RPM}$ (Arranque con alto torque).
  - $1200\,\mu\text{s} \rightarrow 30{,}7\text{ RPM}$.
  - $575\,\mu\text{s} \rightarrow 64{,}1\text{ RPM}$.
  - $137\,\mu\text{s} \rightarrow 200{,}0\text{ RPM}$.
  - $100\,\mu\text{s} \rightarrow \sim 250{,}0\text{ RPM}$ (Límite máximo seguro con rampa).
- **Rampa de Aceleración por Software:**
  La función `fijarRetardoConRampa(targetDelay)` ejecuta saltos suaves de 50 µs con trenes de 8 micro-pulsos intermedios para prevenir pérdida de pasos (*stall*).
- **Fórmulas de Operación:**
  $$\text{Frecuencia (Hz)} = \frac{1\,000\,000}{\text{Delay} + 50\,\mu\text{s}}$$
  $$\text{RPM} = \left(\frac{\text{Frecuencia}}{1600}\right) \times 60 = \frac{37\,500}{\text{Delay} + 50}$$
  $$Q\,(\text{L/min}) = -0{,}029 \times \left(\frac{\text{Delay}}{10}\right) + 3{,}858$$

---

## 3. Canales de Comunicación y Control

1. **Servidor Web Wi-Fi (Modo Access Point):**
   - **SSID:** `ESP32_Bomba_Control` | **Clave:** `12345678` | **IP:** `http://192.168.4.1`
   - Sondeo periódico estable a 1000 ms con `encodeURIComponent` para evitar pérdida de caracteres especiales (`+`, `V`).
   - Sincronización JSON bajo la clave `"delay"`.
2. **Bluetooth Classic SPP:** Broadcast activo `Bomba_Filtro_ESP32`.
3. **Monitor Serie USB:** Baudrate a $115\,200\text{ bps}$.

---

## 4. Tabla de Comandos Unificados

| Comando | Acción | Estado Web | Respuesta Serial / BT |
| :---: | :--- | :---: | :--- |
| **`1`** | Puesta en marcha | `EN MARCHA` (Verde) | `>> [ESTADO]: BOMBA EN MARCHA` |
| **`0`** | Parada controlada | `DETENIDO` (Rojo) | `>> [ESTADO]: BOMBA DETENIDA` |
| **`D` / `d`** | Inversión de giro | Filtración $\leftrightarrow$ Retrolavado | `>> [MODO]: ...` |
| **`+` / `V` / `v`** | Subir RPM (baja retardo en 100 µs) | Actualiza telemetría | `>> [VELOCIDAD +]: Retardo = ...` |
| **`-`** | Bajar RPM (sube retardo en 100 µs) | Actualiza telemetría | `>> [VELOCIDAD -]: Retardo = ...` |

---

## 5. Parámetros de Compilación en Arduino IDE

- **Placa:** `ESP32 Dev Module` (NodeMCU-32S, 38 pines)
- **Partition Scheme (Crítico):** `Huge APP (3MB No OTA/1MB SPIFFS)`
- **Código Fuente Actualizado:**  
  [`firmware_esp32_tesis.ino`](file:///d:/antigravity-pipeline/TESIS/firmware_esp32_tesis/firmware_esp32_tesis.ino)
