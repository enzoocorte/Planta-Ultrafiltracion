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
    SHIELD BORNERAS ESP32 (38 PINES ZS-1057)         DRIVER LEADSHINE DM860
  ┌─────────────────────────────────────────┐      ┌────────────────────────┐
  │  Borne a tornillo [ VIN ] (+5V USB) ────┼──┬──►│ Borne [ PUL+ ]         │
  │                                         │  └──►│ Borne [ DIR+ ]         │
  │  Borne a tornillo [ P18 / D18 ] ────────┼─────►│ Borne [ PUL- ] (Paso)  │
  │  Borne a tornillo [ P19 / D19 ] ────────┼─────►│ Borne [ DIR- ] (Giro)  │
  │                                         │      │                        │
  │  Orientación: Pin 3V3 con Borne 3V3     │      │ Bornes ENA+ / ENA-     │
  │  y Pin VIN con Borne VIN (no girar 180°)│      │ (DESCONECTADOS)        │
  └─────────────────────────────────────────┘      └────────────────────────┘
```

### Detalle de los cables a atornillar (Configuración Ánodo Común a 5V):
1. **Ánodo Común (+5V)**: Un cable desde el borne **`VIN`** del shield hacia los bornes **`PUL+`** y **`DIR+`** del DM860 puenteados entre sí. Al usar 5V directos de `VIN`, el optoacoplador del DM860 conmuta con su corriente nominal de 13 mA, asegurando inmunidad total a ruidos inductivos.
2. **Paso (STEP)**: Un cable flexible desde el borne **`P18`** del shield al borne **`PUL-`** del DM860.
3. **Dirección (DIR)**: Un cable flexible desde el borne **`P19`** del shield al borne **`DIR-`** del DM860.
4. **Habilitación (ENA)**:
   * **Dejar `ENA+` y `ENA-` vacíos (al aire)**. El driver se mantiene habilitado con torque permanente de retención.

---

## ⚡ 3. Alimentación del ESP32 por Bornera

Para que la planta funcione de forma autónoma sin depender de tener una computadora conectada por cable USB:
1. Asegúrate de que el módulo **Step-Down LM2596** esté calibrado con tester a **`5.00 V DC exactos`**.
2. Conecta un cable desde el borne `OUT+` (+5V) del LM2596 al borne **`VIN`** del shield del ESP32.
3. Conecta un cable desde el borne `OUT-` (0V) del LM2596 al borne **`GND`** del shield del ESP32.

---

## 📱 4. Cómo Conectarse y Manejar la Bomba por Wi-Fi

El firmware [`bomba/bomba.ino`](./bomba/bomba.ino) cuenta con un sistema híbrido inteligente:

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

1. **Slider de Velocidad**: Desliza el dedo para regular las RPM suavemente en el rango seguro de **$70\text{ a }160\text{ RPM}$**.
2. **Botones Rápidos de Preseteo**:
   * `80 RPM` ➔ Flujo nominal suave ($0.336\text{ L/min}$).
   * `100 RPM` ➔ Flujo estándar de operación ($0.420\text{ L/min}$).
   * `120 RPM` ➔ Operación sostenida ($0.504\text{ L/min}$).
   * `140 RPM` ➔ Operación máxima con advertencia previa de caudal ($0.588\text{ L/min}$).
3. **Alerta de Membrana FX100**:
   * A partir de $0.55\text{ L/min}$ emite aviso preventivo en amarillo.
   * Superando $0.60\text{ L/min}$ emite alarma crítica pulsante en rojo para proteger las fibras capilares de la membrana.
4. **Botón Invertir Sentido con Rampa Automática**:
   * **Horario (CW)**: Modo Filtración impulsando agua hacia la membrana FX100.
   * **Antihorario (CCW)**: Modo Retrolavado (*Backwash*) para limpiar las fibras.
   * *Maniobra*: Desacelera a 0 RPM a 40 RPM/s, conmuta la dirección con motor detenido y reaccelera sin tirones.
5. **Cálculo en Tiempo Real**:
   * Muestra el **Caudal instantáneo en L/min** ($Q = \text{RPM} \times 4.2\text{ mL/rev}$).
   * Muestra el **Volumen total bombeado en Litros** acumulado durante la prueba.

---

## 🎯 Entregable Concreto de esta Guía
* **Módulo de bombeo peristáltico inalámbrico operativo**: ESP32 montado en shield de borneras a tornillo, firmware `bomba.ino` cargado, y accionamiento de la bomba controlado en tiempo real vía Wi-Fi (Dashboard Web local en `http://192.168.50.53` o AP `192.168.4.1`) con rampas suaves de aceleración e inversión de giro.

---

## 📋 Lista de Verificación (Checklist de Avance)
- [x] ESP32 calzado firmemente en el shield de borneras ZS-1057 respetando la orientación correcta (`3V3` con `3V3`, `VIN` con `VIN`).
- [x] Conexión a tornillo firme de `VIN` a `PUL+` y `DIR+` (Ánodo Común 5V), `P18` a `PUL-`, y `P19` a `DIR-` (Open-Drain).
- [x] Alimentación del ESP32 probada: alimentación por USB / bornera 5V operativa.
- [x] Firmware `bomba.ino` compilado y subido correctamente vía Arduino CLI / IDE.
- [x] Conexión Wi-Fi establecida: acceso al dashboard en `http://192.168.50.53` y AP de contingencia `Bomba_Peristaltica_UF`.
- [x] Respuesta de la bomba peristáltica verificada con los botones de preseteo (80, 100, 120 y 140 RPM).
- [x] Inversión de giro comprobada con rampa de deceleración segura a 0 RPM (Horario para filtración, Antihorario para retrolavado).
- [x] Acumulador de volumen de líquido bombeado y cálculo de caudal instantáneo funcionando en la interfaz web.

