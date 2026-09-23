# 💧 Firmware Modular Planta Piloto de Ultrafiltración (`planta_uf.ino`)

Este directorio contiene la arquitectura modular en C++ orientada a objetos para el control integral de la Planta Piloto de Ultrafiltración (ESP32).

---

## 📁 Estructura Modular de Archivos

| Archivo | Responsabilidad |
| :--- | :--- |
| **`config.h`** | Centraliza **todas** las constantes de la planta: pines GPIO, límites de RPM, rampa de aceleración, constantes de calibración $K$ de los caudalímetros, tiempos de filtrado anti-ruido y credenciales Wi-Fi (AP y STA). |
| **`Bomba.h`** | Clase `Bomba`: encapsula el accionamiento de la bomba peristáltica con driver DM860 en **Cátodo Común** mediante hardware LEDC de 10 bits. Implementa arranque suave, inversión de sentido segura y cálculo de caudal teórico por geometría del cabezal ($4.2\text{ mL/rev}$). |
| **`caudalimetro.h`** / **`caudalimetro.cpp`** | Clase `Caudalimetro`: maneja los microcaudalímetros YF-S401 con triple defensa anti-ruido (interrupción atómica con mutex FreeRTOS, bloqueo temporal de 3 ms por ISR, descarte de caudales físicamente imposibles $>700\text{ mL/min}$ y filtro exponencial EMA 30/70 para el display). Detecta pérdida de señal/burbuja de aire tras 5 s de bombeo sin pulsos. |
| **`index_html.h`** | Interfaz Web SCADA responsive (HTML5 + CSS + JavaScript) embebida en memoria Flash (`PROGMEM`). Incluye monitoreo en vivo de FEED, PERMEADO, balance de masa, barra de capacidad de membrana y panel de mando de la bomba. |
| **`planta_uf.ino.ino`** | Punto de entrada principal (`setup` y `loop`). Inicia el Wi-Fi en modo dual (`WIFI_AP_STA`), servidor Web Asíncrono (`ESPAsyncWebServer`), mDNS (`http://bomba.local`) y refresca la instrumentación cada 1 segundo. |

---

## ⚡ Guía de Conexionado y Precauciones de Hardware

### 1. Driver DM860 (Bomba NEMA 34) — Configuración en CÁTODO COMÚN
El firmware commiteado utiliza lógica activa en `HIGH` con baja impedancia de salida hacia masa (`GND`), eliminando las falsas señales inducidas por EMI.

```text
Shield ESP32                    Driver DM860
┌──────────────┐               ┌──────────────┐
│  P18 (GPIO18)├──────────────►│ PUL+         │
│  P19 (GPIO19)├──────────────►│ DIR+         │
│              │               │              │
│  GND         ├───┬──────────►│ PUL-         │
│              │   └──────────►│ DIR-         │
└──────────────┘               └──────────────┘
(Nota: ENA+ y ENA- se dejan desconectados. VIN/+5V NO va al DM860).
```

### 2. Caudalímetros YF-S401 (Alimentación y Adaptación de Tensión)
* **Caudalímetro FEED (Alimentación):** Señal a **`P14`** (GPIO 14). Montaje en vertical ascendente entre la bomba y el módulo de membrana.
* **Caudalímetro PERMEADO:** Señal a **`P27`** (GPIO 27). Montaje en horizontal a la salida del permeado.

#### ⚠️ Alerta de Voltaje en Señal (Cable Amarillo):
Los caudalímetros alimentados a 5V elevan su cable amarillo a **4.2 V - 5.0 V** a través de su resistencia interna de pull-up. Los pines del ESP32 operan a **3.3 V (máximo seguro: 3.6 V)**.

**Opciones de conexión recomendadas:**
1. **Opción A (Divisor de tensión con 2 resistencias - Recomendada para Tesis):**
   * Cable Amarillo $\rightarrow$ Resistencia $1\text{ k}\Omega$ (o $2.2\text{ k}\Omega$) $\rightarrow$ Pin ESP32 (`P14` o `P27`).
   * Del Pin ESP32 a `GND` $\rightarrow$ Resistencia $2\text{ k}\Omega$ (o $3.3\text{ k}\Omega$).
   * *Resultado:* Señal protegida a $3.0\text{ V}$.
2. **Opción B (Resistencia limitadora en serie):**
   * Intercalar una resistencia de $10\text{ k}\Omega$ en el cable amarillo antes del pin del ESP32.
3. **Opción C (Prueba rápida a 3.3V):**
   * Conectar el cable Rojo directamente al pin de **3.3 V** del Shield. Si el sensor Hall interno de la partida conmuta a 3.3V, funcionará sin necesidad de resistencias adicionales y con total seguridad para el microcontrolador.

---

## 🚀 Instrucciones para Flashear el ESP32

Cuando conectes el cable micro-USB con líneas de datos al puerto de la PC (`COM5`), ejecuta:

```powershell
& "C:\Users\enzoo\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" upload -p COM5 --fqbn "esp32:esp32:esp32:FlashMode=dio,FlashFreq=40,UploadSpeed=115200" "c:\Users\enzoo\OneDrive\Documentos\ENZO\Domotica\SistemaUF\planta_uf.ino\planta_uf.ino.ino"
```

> **IMPORTANTE:** El parámetro `FlashMode=dio` y `FlashFreq=40` es mandatorio en esta placa NodeMCU ESP32 para evitar el error de suma de verificación (`csum err`) en el arranque.

---

## 🌐 Conexión al SCADA Web
Una vez encendido:
* **Punto de Acceso Wi-Fi:** Conectarse a la red `Bomba_Peristaltica_UF` (Clave: `plantapiloto2`) y abrir en el navegador `http://192.168.4.1/`.
* **Red de Laboratorio:** Si el ESP32 está conectado a la red `Box804`, ingresar desde cualquier PC o celular conectado a esa red mediante `http://bomba.local/`.
