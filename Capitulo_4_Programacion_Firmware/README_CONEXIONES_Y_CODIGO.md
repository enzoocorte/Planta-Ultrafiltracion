# 📡 Documentación Técnica: Instrumentación y Adquisición de Señales (Sección 4.3.4)

**Tesis de Grado:** *Diseño experimental y escalamiento industrial de un módulo automatizado de ultrafiltración para la potabilización de agua, utilizando membranas de hemodiálisis reutilizadas*  
**Autores:** Owen Cañizares & Antonella Guitián  
**Directores:** Dr. Ing. Jorge Emilio Almazán & Ing. Enzo Marcelo Corte  
**Institución:** Universidad Nacional de Salta (UNSa) — Facultad de Ingeniería  

---

## 🗺️ 1. Correspondencia Físico-Lógica del Hardware

El código desarrollado en esta carpeta implementa de forma directa la topología de conexión representada en el esquema interactivo:
👉 **[Esquema de Conexionado de Sensores (HTML)](file:///D:/ANTIGRAVITY/projects/antigravity-pipeline/TESIS/Capitulos/Capitulo%204%20-%20Ingenieria%20del%20Proyecto/4.3.4_Instrumentacion_y_Adquisicion_Sensores/conexion_nodo_pullup_3v3_caudalimetros.html)**

### 📦 Nivel 1: Placa 1 Superior (Acondicionamiento y Filtros de Campo)
* **Borne 1 Sup (3.3V):** Riel de referencia a 3.3V proveniente del regulador LDO del ESP32. Alimenta la resistencia de pull-up de $4.7\text{ k}\Omega$ fijando el nivel alto del sensor de permeado en 3.3V.
* **Borne 11 Sup (D27):** Nodo del caudalímetro S1 de Permeado (Cable Amarillo) + Resistencia Pull-up $4.7\text{ k}\Omega$ + Condensador cerámico multicapa $100\text{ nF}$ (104) derivado a GND para filtrado pasivo pasabajos.
* **Borne 12 Sup (D14):** Nodo del caudalímetro S2 de Alimentación / Feed (Cable Amarillo) + Condensador cerámico $100\text{ nF}$ a GND.
* **Borne 15 Sup (GND):** Retorno de masa de alta frecuencia de los condensadores cerámicos.
* **Borne 17 Inf (5V/VIN):** Riel de potencia de 5V común que alimenta los dos sensores YF-S401 (Cables Rojos) y la pata positiva $(+)$ del capacitor de desacoplo electrolítico de $2200\ \mu\text{F}$.
* **Borne 18 Inf (GND):** Riel de masa estrella común que conecta los cables negros de los sensores, la pata negativa $(-)$ del capacitor de $2200\ \mu\text{F}$ y la referencia de tierra del sistema.

### ⚡ Nivel 2: Troncal Vertical Directo (5 Conductores)
Une en sentido vertical recto la Placa 1 (Superior) con la Placa 2 (Inferior):
1. **Riel 3.3V (Celeste):** Borne 1 Sup ➔ Borne 1 Sup.
2. **Señal Permeado P27 (Amarillo):** Borne 11 Sup ➔ Borne 11 Sup (GPIO 27 / Interrupción S1).
3. **Señal Alimentación P14 (Cian):** Borne 12 Sup ➔ Borne 12 Sup (GPIO 14 / Interrupción S2).
4. **Riel 5V / VIN (Rojo):** Borne 17 Inf ➔ Borne 17 Inf.
5. **Riel GND Masa Estrella (Gris):** Borne 18 Inf ➔ Borne 18 Inf.

### ⚙️ Nivel 3: Placa 2 Inferior (CPU ESP32 + Driver DM860)
* **ESP32 NodeMCU-32S:** Procesa las interrupciones por hardware en Core 0/1 con cerrojos `portMUX_TYPE`.
* **Driver Paso a Paso DM860 (NEMA 34) en Cátodo Común:**
  - `PUL+` ➔ `GPIO 18` (`PUL-` conectado a GND común).
  - `DIR+` ➔ `GPIO 19` (`DIR-` conectado a GND común).

---

## 📊 2. Parámetros de Calibración Gravimétrica del Firmware

| Parámetro | Variable en Código | Valor Calibrado | Razón de Ingeniería / Cálculo |
| :--- | :--- | :--- | :--- |
| **Factor K Feed** | `K_ALIMENTACION` | **205.98 Hz/(L/min)** | Calibración Ensayo 3 en probeta (12,359 pulsos/L). |
| **Factor K Permeado** | `K_PERMEADO` | **205.98 Hz/(L/min)** | Homologado al mismo modelo físico de sensor YF-S401 (12,359 pulsos/L). |
| **Cilindrada Bomba** | `ML_POR_VUELTA` | **13.5496 mL/rev** | Media Ensayo 2 en probeta (20 a 90 RPM). |
| **Rampa Aceleración** | `ACEL_NOMINAL_RPM_S` | **18.0 RPM/s** | Rampa dinámica fluida con despegue instantáneo en 6.0 RPM (320 Hz). |
| **Rampa Desaceleración**| `DESACEL_AJUSTE_RPM_S` | **16.0 RPM/s** | Desaceleración ágil (parada rápida garantizada en 60.0 RPM/s). |
| **Filtro / Blanking ISR** | `blanking_us` | **1200 µs (Alim) / 3500 µs (Perm)** | Blanking digital diferenciado: tolera crestas pulsátiles de 410 Hz a 90-100 RPM. |

---

## 📁 3. Estructura de Archivos del Firmware

```text
4.3.4_Instrumentacion_y_Adquisicion_Sensores/
├── 4.3.4_Instrumentacion_y_Adquisicion_Sensores.ino  # Sketch principal Arduino (Setup, Loop, WebServer)
├── config.h                                          # Definición de pines, factores K y constantes cinemáticas
├── caudalimetro.h                                    # Clase Caudalimetro (Interrupciones atómicas y filtros)
├── caudalimetro.cpp                                  # Implementación en IRAM del ISR con portMUX_TYPE
├── Bomba.h                                           # Controlador de motor paso a paso con rampa suave
├── index_html.h                                      # Dashboard SCADA Web responsivo (Flash PROGMEM)
├── conexion_nodo_pullup_3v3_caudalimetros.html       # Plano esquemático interactivo SVG de la topología
├── SUBSECCION_4_3_4_INSTRUMENTACION_Y_AQUISICION_SENSORES.md # Subsección redactada para la memoria de Tesis
└── README_CONEXIONES_Y_CODIGO.md                     # Esta guía técnica
```

---

## 🚀 4. Instrucciones de Compilación y Carga en Arduino IDE

1. Abrir **Arduino IDE** (versión 2.x o superior).
2. Seleccionar la placa: **ESP32 Dev Module** (o NodeMCU-32S).
3. Parámetros recomendados:
   - **Upload Speed:** `921600` o `115200` baudios.
   - **Flash Frequency:** `80MHz`.
   - **Partition Scheme:** `Default 4MB with spiffs (1.2MB APP / 1.5MB SPIFFS)`.
   - **Core Debug Level:** `None` (o `Info` para depuración).
4. Abrir [`4.3.4_Instrumentacion_y_Adquisicion_Sensores.ino`](file:///D:/ANTIGRAVITY/projects/antigravity-pipeline/TESIS/Capitulos/Capitulo%204%20-%20Ingenieria%20del%20Proyecto/4.3.4_Instrumentacion_y_Adquisicion_Sensores/4.3.4_Instrumentacion_y_Adquisicion_Sensores.ino).
5. Compilar y subir al ESP32.
6. Abrir el Monitor Serie a **115200 baudios**.
7. Conectarse a la red Wi-Fi `Bomba_Peristaltica_UF` (clave: `plantapiloto2`) y acceder en el navegador a:
   `http://192.168.4.1` (o a la IP asignada por el router local).
