# 👥 Organización del Equipo de Trabajo, Roles y Protocolo de Repositorio
## Planta Piloto de Ultrafiltración FX100 — Tesis UNSa 2026

**Ubicación:** `00_General_y_P_ID_Planta/03_Organizacion_Equipo_y_Protocolos/`  
**Institución:** Universidad Nacional de Salta (UNSa) — Facultad de Ingeniería

---

## 🎯 1. Propósito de la Carpeta

Garantizar la sincronización de trabajo, la trazabilidad de código y la coherencia metodológica entre los tesistas de grado y la codirección docente. Este marco previene divergencias en el firmware, desfasajes de constantes de calibración y asegura que el repositorio de Git sea la **única fuente de verdad** (*Single Source of Truth*).

---

## 👥 2. Estructura y Roles del Equipo

* **Antonella Guitián (Tesista - Ingeniería Industrial):**
  * Responsable de la caracterización fisicoquímica, cinética de coagulación con *Opuntia ficus-indica* (Hito 3), matriz factorial $3^2$ en planta piloto (Hito 5) y análisis de costos operativos OPEX (Hito 6).
* **Owen Cañizares (Tesista - Ingeniería Industrial):**
  * Responsable del montaje electromecánico, instrumentación de caudalímetros, calibración metrológica y programación del firmware en C++/ESP32 (Hitos 1, 2 y 4).
* **Ing. Enzo Corte (Codirector de Tesis - Investigador Doctoral en Membranas):**
  * Dirección técnica, formulación de modelos matemáticos de transporte (Darcy, Hermia, Vogel), supervisión de seguridad industrial, validación de autopsia SEM y arquitectura de software.

---

## 📂 3. Archivos en esta Carpeta

| Archivo | Tipo | Descripción |
| :--- | :--- | :--- |
| **[`ORGANIZACION_PROYECTO_Y_EQUIPO.md`](./ORGANIZACION_PROYECTO_Y_EQUIPO.md)** | Protocolo Maestro | Guía operativa del equipo que explica la reestructuración de hitos, convención de nombres, gestión de branches en Git y flujo de trabajo en el laboratorio. |

---

## 📜 4. Reglas Operativas Mandatorias

1. **Cero código suelto:** Todo firmware debe compilarse desde la carpeta oficial del hito en curso (`02_Hito2_.../02_Firmware_Test_Sensores/Subhito2_2_v6/EN_USO_firmware_planta/`).
2. **Registro de bitácora:** Cada ensayo en banco debe quedar documentado con su respectivo archivo CSV descargado directamente del SCADA (`http://192.168.4.1/export_csv`).
3. **Commit y Push diario:** Al concluir cada jornada en el laboratorio de la UNSa, se realiza `git add .`, `git commit` y `git push origin main`.
