# 🛠️ GUÍA DE MONTAJE DE BOMBA & SHIELD DE BORNERAS ESP32
## Control Inalámbrico Wi-Fi • Hito 1

Esta guía explica cómo conectar físicamente el **ESP32 con su Shield de Borneras a tornillo** al driver Leadshine DM860 y cómo manejar la bomba peristáltica desde el celular o la computadora por Wi-Fi.

---

## 🧭 1. Encastre del ESP32 en el Shield de Borneras

1. Ubica el ESP32 (38 pines) sobre el shield de expansión.
2. Orienta el conector **USB-C hacia el borde exterior** de la placa.
3. Asegúrate de que los 19 pines de cada lado calcen rectos en los zócalos hembra sin doblar ninguna patita.
4. Presiona con suavidad y firmeza hasta que el microcontrolador quede completamente asentado.

---

## 🔌 2. Cableado Pin a Pin a Tornillo (Bornera Shield ➔ Driver DM860)

Gracias al shield de borneras, todos los cables quedan fijados con tornillos de bornera clema, sin cables sueltos ni falsos contactos:

```
    SHIELD BORNERAS ESP32 (38 PINES)                 DRIVER LEADSHINE DM860
  ┌─────────────────────────────────┐              ┌────────────────────────┐
  │  Borne a tornillo [ D18 ] ──────┼─────────────►│ Borne [ PUL+ ]         │
  │  Borne a tornillo [ D19 ] ──────┼─────────────►│ Borne [ DIR+ ]         │
  │  Borne a tornillo [ GND ] ──────┼─┬───────────►│ Borne [ PUL- ]         │
  │                                 │ └───────────►│ Borne [ DIR- ]         │
  │  Borne a tornillo [ VIN ] ◄─────┼── (+5.00V)   │                        │
  │  Borne a tornillo [ GND ] ◄─────┼── (0V Masa)  │ Bornes ENA+ / ENA-     │
  └─────────────────────────────────┘              │ (DESCONECTADOS)        │
                                                   └────────────────────────┘
```

### Detalle de los cables a atornillar:
1. **Paso (STEP)**: Un cable flexible desde el borne **`D18`** del shield al borne **`PUL+`** del DM860.
2. **Dirección (DIR)**: Un cable flexible desde el borne **`D19`** del shield al borne **`DIR+`** del DM860.
3. **Masa Común (GND)**:
   * Saca un cable desde el borne **`GND`** del shield.
   * Llévalo a uno de tus conectores rápidos a presión y desde allí distribuye a los bornes **`PUL-`** y **`DIR-`** del DM860.
4. **Habilitación (ENA)**:
   * **Dejar `ENA+` y `ENA-` vacíos (al aire)**. El driver se mantiene habilitado con torque permanente.

---

## ⚡ 3. Alimentación del ESP32 por Bornera

Para que la planta funcione de forma autónoma sin depender de tener una computadora conectada por cable USB:
1. Asegúrate de que el módulo **Step-Down LM2596** esté calibrado con tester a **`5.00 V DC exactos`**.
2. Conecta un cable desde el borne `OUT+` (+5V) del LM2596 al borne **`VIN`** del shield del ESP32.
3. Conecta un cable desde el borne `OUT-` (0V) del LM2596 al borne **`GND`** del shield del ESP32.

---

## 📱 4. Cómo Conectarse y Manejar la Bomba por Wi-Fi

El firmware [`bomba.ino`](./bomba.ino) cuenta con un sistema híbrido inteligente:

### Modo A: Conexión Automática a tu Router
Si la red Wi-Fi de la planta (`Box804`, clave `plantapiloto2`) está encendida:
1. El ESP32 se conectará automáticamente al router.
2. Abre cualquier navegador (Chrome, Safari, Edge) en tu celular o notebook y escribe:
   👉 **`http://bomba.local`** (o la IP que imprima por monitor serie).

### Modo B: Red Propia del ESP32 (Punto de Acceso Autónomo)
Si estás en una mesa sin router o fuera del laboratorio:
1. El ESP32 creará su propia red Wi-Fi llamada: **`Bomba_Peristaltica_UF`**.
2. Desde tu celular, conéctate a esa red Wi-Fi (Clave: **`plantapiloto2`**).
3. Abre el navegador y escribe:
   👉 **`http://192.168.4.1`**

---

## 🎛️ 5. Controles del Dashboard Táctil

En la pantalla verás un panel oscuro estilo SCADA industrial:

1. **Slider de Velocidad**: Desliza el dedo para regular las RPM suavemente de $0\text{ a }120\text{ RPM}$.
2. **Botones Rápidos de Preseteo**:
   * `10 RPM` ➔ Flujo ultra-lento ($0.042\text{ L/min}$).
   * `30 RPM` ➔ Flujo nominal suave ($0.126\text{ L/min}$).
   * `60 RPM` ➔ Flujo estándar de operación ($0.252\text{ L/min}$).
   * `100 RPM` ➔ Alto flujo ($0.420\text{ L/min}$).
3. **Botón Invertir Sentido**:
   * **Horario (CW)**: Modo Filtración impulsando agua hacia la membrana FX100.
   * **Antihorario (CCW)**: Modo Retrolavado (*Backwash*) para limpiar las fibras.
4. **Cálculo en Tiempo Real**:
   * Muestra el **Caudal instantáneo en L/min** ($Q = \text{RPM} \times 4.2\text{ mL/rev}$).
   * Muestra el **Volumen total bombeado en Litros** acumulado durante la prueba.

---

## 🎯 Entregable Concreto de esta Guía
* **Módulo de bombeo peristáltico inalámbrico operativo**: ESP32 montado en shield de borneras a tornillo, firmware `bomba.ino` cargado, y accionamiento de la bomba controlado en tiempo real vía Wi-Fi (Dashboard Web local en `http://bomba.local` o AP `192.168.4.1`) con rampas suaves de aceleración e inversión de giro.

---

## 📋 Lista de Verificación (Checklist de Avance)
- [ ] ESP32 calzado firmemente en el shield de borneras con conector USB-C hacia el exterior.
- [ ] Conexión a tornillo firme de `D18` a `PUL+`, `D19` a `DIR+`, y `GND` a `PUL-` / `DIR-`.
- [ ] Alimentación del ESP32 probada: $5.00\text{ VDC}$ desde el LM2596 al pin `VIN` y `GND`.
- [ ] Firmware `bomba.ino` compilado y subido correctamente vía Arduino IDE.
- [ ] Conexión Wi-Fi establecida: acceso al dashboard en `http://bomba.local` o mediante AP `Bomba_Peristaltica_UF`.
- [ ] Respuesta de la bomba peristáltica verificada con los botones de preseteo (10, 30, 60 y 100 RPM).
- [ ] Inversión de giro comprobada (Horario para filtración, Antihorario para retrolavado).
- [ ] Acumulador de volumen de líquido bombeado funcionando en la interfaz web.
