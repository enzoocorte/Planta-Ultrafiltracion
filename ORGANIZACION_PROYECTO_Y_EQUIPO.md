# 🧭 GUÍA DE ORGANIZACIÓN DEL REPOSITORIO Y PROTOCOLO DEL EQUIPO
**Proyecto:** Planta Piloto de Ultrafiltración FX100 — Tesis de Grado (UNSa)  
**Equipo:** Antonella Guitián, Owen Cañizares & Ing. Enzo Corte  
**Fecha de Consolidación:** 1 de Octubre de 2026  
**Objetivo:** Mantener una arquitectura unificada, limpia y modular donde todos los integrantes trabajen sobre la misma fuente de verdad.

---

## 🎯 1. ¿Por qué se reorganizó el repositorio?

Durante las jornadas de calibración y pruebas de instrumentación, se identificó que coexistían múltiples ramas y carpetas paralelas (como una carpeta raíz `Capitulo_4_Programacion_Firmware/` separada de `02_Hito2_Instrumentacion_Sensores/`). Esto provocó:
* Desfasajes en las versiones de firmware entre lo que se ensayaba en el laboratorio y lo que estaba documentado.
* Duplicación de parámetros (por ejemplo, versiones antiguas con 1600 pulsos/rev y 4.2 mL/rev conviviendo con la calibración real de 3200 pulsos/rev y 13.6 mL/rev).
* Dificultad para saber cuál era el código definitivo grabado en el ESP32.

**Solución aplicada:** Se integró **todo el valioso trabajo desarrollado por Owen** dentro de la estructura formal de hitos del proyecto, eliminando carpetas redundantes y resguardando los códigos históricos en `Archivado/`.

---

## 🗺️ 2. Mapa Rápido: ¿Dónde está cada cosa ahora?

```text
SistemaUF/
│
├── 📁 00_General_y_P_ID_Planta/
│   ├── diagrama_pid_interactivo.html    ──> Diagrama P&ID bajo norma ISA 5.1
│   └── ParametrosFiltroFX100.txt        ──> Especificaciones oficiales del dializador FX100 (2.2 m², KUF 73)
│
├── 📁 01_Hito1_Control_Accionamiento_NEMA34_DM860/
│   ├── 01_Hardware_y_Cableado/          ──> Inventario consolidado, torque y cableado DM860
│   └── 02_Firmware_Pruebas/             ──> Pruebas de movimiento cinemático
│
├── 📁 02_Hito2_Instrumentacion_Sensores/  ──> 🌟 ¡NÚCLEO ACTIVO DE CALIBRACIÓN Y TRABAJO!
│   │
│   ├── 📁 01_Guias_Montaje_y_Calibracion/
│   │   ├── Diagnostico_y_Resolucion_Problemas_Instrumentacion.md ──> Documento formal para tesis (Ruido 81 Hz)
│   │   ├── Guia_Montaje_Placa_Filtrado_FrontEnd.md              ──> Manual de cableado de las 2 borneras ZS-1057
│   │   │
│   │   ├── 📁 Simulaciones_Filtro_RC/    ──> Simulación en LTspice (.asc), script Python y curva PNG
│   │   ├── 📁 Esquemas_Conexionado_HTML/ ──> Todos los planos interactivos SVG (Borneras, protoboard, pull-up)
│   │   ├── 📁 Bitacoras_Calibracion/     ──> Registro cronológico de jornadas de Owen y matriz de fallas
│   │   └── 📁 Documentacion_Tecnica/     ──> Memoria de programación ESP32 y notas técnicas
│   │
│   ├── 📁 02_Firmware_Test_Sensores/
│   │   │
│   │   ├── 🌟 firmware_planta/           ──> 🚀 FIRMWARE OFICIAL DE PRODUCCIÓN (Grabado en el ESP32)
│   │   │   ├── firmware_planta.ino       ──> Servidor Web SCADA, SoftAP anti-desconexión y datalogger
│   │   │   ├── config.h                  ──> Pines, 3200 pul/rev, K_alim=154.62, K_perm=55.0, RPM_INICIO=25
│   │   │   ├── Bomba.h / Bomba.cpp       ──> Rampa S-Curve, control LEDC PWM y Cátodo Común
│   │   │   ├── caudalimetro.h / .cpp     ──> Cerrojos FreeRTOS (portMUX) y filtro 2000 µs
│   │   │   └── index_html.h / .cpp       ──> Dashboard web con calibración y exportación CSV
│   │   │
│   │   ├── 📁 firmware_esp32_platformio/ ──> Proyecto configurado para compilar con PlatformIO Core CLI
│   │   └── 📁 scripts_compilacion_rapida/──> Scripts .bat (compilar, flashear y monitor en 3 segundos)
│   │
│   └── 📁 Datos/                        ──> 📊 REGISTROS Y ENSAYOS EXPERIMENTALES
│       ├── CALIBRACION_CAUDALIMETROS_PROBETA_50RPM_72RPM.xlsx ──> Planilla de calibración de probeta de Owen
│       ├── datos_planta_uf_2026-09-30.csv                     ──> Telemetría cruda en banco
│       └── datos_planta_uf_2026-09-30 (1).csv                 ──> Telemetría a 25, 36, 50, 74 y 80 RPM
│
├── 📁 03_Hito3_Reactor_Sedimentador_Agitador/  ──> Coagulación, floculación, Jar-Test y L298N
├── 📁 04_Hito4_Integracion_Automatizacion_IoT/ ──> Lógica FreeRTOS centralizada y seguridad TMP
├── 📁 05_Hito5_Ensayos_Membrana_VidaUtil/      ──> Ley de Darcy, TCF y ensuciamiento
│
└── 📁 Archivado/
    └── 📁 Historial_Firmware_Owen_Capitulo4/   ──> Versiones preliminares de Arduino UNO y prototipos viejos
```

---

## ⚙️ 3. El Firmware Oficial de la Planta (`firmware_planta`)

El código oficial que rige el proyecto y que se encuentra compilado en el microcontrolador es:  
👉 **[`02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/firmware_planta/`](./02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/firmware_planta/)**

### Parámetros Maestros en `config.h`:
* **Driver Leadshine DM860:** Cátodo Común (`PIN_PUL = 18`, `PIN_DIR = 19`).
* **Resolución de micropasos:** `PULSOS_POR_REV = 3200` (1/16 paso, SW5=OFF, SW6=OFF, SW7=ON, SW8=ON).
* **Consigna de arranque seguro:** `RPM_INICIO = 25.0f` ($\approx 340\text{ mL/min}$).
  * *Motivo de seguridad:* Previene reventar la membrana Fresenius FX100 (cuyo límite capilar es $600\text{ mL/min}$ / $36\text{ RPM}$). Para calibrar con agua en probeta, se sube desde la interfaz Web a 50 o 72 RPM.
* **Cilindrada calibrada:** `ML_POR_VUELTA = 13.6000f` ($680\text{ mL/min}$ a 50 RPM).
* **Factor K Alimentación:** `K_ALIMENTACION = 154.62f` ($105.14\text{ Hz}$ para $680\text{ mL/min}$).
* **Factor K Permeado:** `K_PERMEADO = 55.00f`.
* **Filtro Software Anti-Ruido:** `FILTRO_RUIDO_US = 2000` ($2.0\text{ ms}$ de desrebote, techo hasta $500\text{ Hz}$).
* **Persistencia NVS:** Los ajustes realizados en la Web se guardan en la memoria Flash permanente mediante `Preferences.h`.

---

## 🤝 4. Protocolo de Trabajo y Comunicación del Equipo

Para que Owen, Antonella y Enzo mantengan una coordinación impecable:

1. **Trabajar sobre la rama `main`:**
   * Antes de comenzar cualquier jornada: ejecutar `git pull origin main`.
   * Al finalizar la jornada: subir los cambios con un mensaje claro (`git commit` y `git push origin main`).
2. **Ubicación de nuevos datos experimentales:**
   * Cualquier nuevo archivo CSV exportado desde la Web SCADA o planilla de probeta debe guardarse en:  
     👉 `02_Hito2_Instrumentacion_Sensores/Datos/`
3. **No crear carpetas paralelas en la raíz:**
   * Toda la programación pertenece a `02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/`.
   * Toda la documentación de apoyo o guías pertenece a `01_Guias_Montaje_y_Calibracion/`.
4. **Resguardo de versiones:**
   * Si alguien realiza una prueba temporal que luego es superada, se traslada a `Archivado/` para mantener limpios los directorios principales.
