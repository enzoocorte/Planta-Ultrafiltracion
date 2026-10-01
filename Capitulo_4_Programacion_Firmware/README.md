# 📁 Estructura General de Programación y Firmware (Capítulo 4)

**Proyecto:** Planta Piloto de Ultrafiltración FX100 — Tesis de Grado (UNSa)  
**Tesistas:** Owen Cañizares & Antonella Guitián  
**Directores:** Dr. Ing. Jorge E. Almazán & Ing. Enzo M. Corte  

---

## 🗂️ Árbol de Organización por Módulos y Subcarpetas

```text
programacion/
│
├── 📂 01_Firmware_ESP32_Produccion/        ──> Firmware listo para flashear y operar en planta
│   ├── firmware_esp32_arduino/             ──> Proyecto modular oficial para Arduino IDE 2.x
│   ├── firmware_esp32_platformio/          ──> Proyecto compilable con PlatformIO Core CLI
│   └── firmware_esp32_tesis/               ──> Sketch base específico de la memoria de tesis
│
├── 📂 02_Firmware_Arduino_UNO/             ──> Firmware de prototipado inicial y banco de pruebas
│   ├── firmware_arduino_uno_tesis/         ──> Versión principal de instrumentación para UNO
│   └── firmware_arduino_uno_historico/     ──> Versión histórica de validación
│
├── 📂 03_Historial_Versiones_Arduino/      ──> Control de versiones cronológico y bitácoras
│   ├── v3_2026-09-25_Calibracion_Final/    ──> Versión actual en régimen tangencial (K_feed=110.06, K_perm=110.45)
│   ├── v2_2026-09-25_Calibracion_Serie/    ──> Versión con sensores en serie (K=751.3 / 754.0)
│   ├── v1_2026-09-25_Rampa_Suave/          ──> Versión con arranque suave a 8 RPM/s
│   ├── v0_2026-09-07_V1_Inicial/           ──> Primer prototipo funcional
│   ├── codigo_arduino_base/                ──> Archivos fuente base reutilizables
│   └── bitacoras_desarrollo/               ──> Bitácoras técnicas detalladas de cada jornada
│
├── 📂 04_Simulaciones_Filtro_RC/           ──> Modelado y simulación de filtros anti-ruido
│   ├── simular_calidad_filtro.py           ──> Script en Python para análisis de respuesta temporal
│   ├── simulacion_filtro_caudalimetro.asc  ──> Esquema de simulación en LTspice
│   └── curva_calidad_filtro_caudalimetro.png ──> Gráfica de simulación de atenuación de ruido
│
├── 📂 05_Esquemas_Conexionado_HTML/        ──> Planos interactivos SVG de conexionado y ruteo
│   ├── conexion_nodo_pullup_3v3_caudalimetros.html ──> Plano vertical directo Placa 1 a Placa 2
│   ├── conexion_caudalimetros_protoboard.html      ──> Esquema de conexión en protoboard
│   └── conexion_caudalimetro_shield_capacitor.html ──> Esquema con capacitor de desacoplo
│
├── 📂 06_Scripts_Automatizacion_BAT/       ──> Accesos directos de compilación y monitor serie
│   ├── compilar_esp32_rapido.bat           ──> Compila el firmware PlatformIO en 3 segundos
│   ├── subir_esp32_rapido.bat              ──> Sube el binario por puerto COM al ESP32
│   └── monitor_serie_rapido.bat            ──> Abre el monitor serie a 115200 bps
│
└── 📂 07_Documentacion_y_Guias/            ──> Documentación técnica, prompts y notas de diseño
    ├── programacion_esp32.md               ──> Memoria técnica y arquitectura del firmware
    ├── PROMPT_CODIGO_ESP32_CAUDALIMETROS_DM860.md ──> Requisitos y especificaciones de diseño
    └── caudalimetro_referencia.h           ──> Cabecera de referencia de caudalímetros
```
