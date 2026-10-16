# 📐 Diagrama de Tuberías e Instrumentación (P&ID) y Filosofía de Control
## Planta Piloto de Ultrafiltración FX100 — UNSa 2026

**Ubicación:** `00_General_y_P_ID_Planta/02_Diagrama_PID_y_Filosofia_Control/`  
**Norma Aplicada:** ISA-5.1 (*Instrumentation Symbols and Identification*)  
**Responsables:** Ing. Enzo Corte | Antonella Guitián & Owen Cañizares

---

## 🎯 1. Propósito de la Carpeta

Esta subcarpeta contiene la documentación formal que gobierna el **recorrido hidráulico, los instrumentos de campo y la lógica de control del proceso** de la planta piloto. Conecta el pretratamiento por coagulación-floculación con la ultrafiltración tangencial de fibra hueca Fresenius FX100.

---

## 🗺️ 2. Filosofía del Proceso y Circuitos Hidráulicos

```
                      [ DIAGRAMA DE FLUJO HIDRÁULICO GENERAL ]

     Agua Turbia
    + Coagulante
         │
         ▼
 ┌───────────────┐               ┌─────────────────┐               ┌────────────────┐
 │  SEDIMENTADOR │  Sobrenadante │  PREFILTRO DE   │  Aspiración   │  BOMBA PERIST. │
 │     CÓNICO    │ ────────────► │  MALLA 120 MESH │ ────────────► │   MBP-2000     │
 │  (Módulo 1)   │  Clarificado  │   (125 µm)      │  (Manguera)   │ (NEMA 34+DM860)│
 └───────┬───────┘               └─────────────────┘               └───────┬────────┘
         │ Lodos                                                           │
         ▼ (Purga)                                                         ▼ Impulsión
      Drenaje                                                        [ Manómetro P1 ]
                                                                           │
                                                                           ▼
                                                                  ┌─────────────────┐
                                      Retentado (Concentrado)     │ MEMBRANA FX100  │
                         ┌─────────────────────────────────────── │  (Helixone®)    │
                         │                                        └────────┬────────┘
                         ▼                                                 │ Permeado
                 [ Válvula Aguja ]                                         ▼
                 [ Manómetro P2  ]                                [ Caudalímetro Qp]
                         │                                                 │
                         ▼ Recirculación / Drenaje                         ▼
                 Tanque Alimentación                              Tanque de Permeado
                                                                  (Agua Potable UF)
```

1. **Línea de Alimentación:**
   * Aspiración desde la toma lateral del sedimentador cónico ($5\text{ cm}$ sobre la cota de lodos).
   * Prefiltro tipo "Y-Strainer" con malla de acero inoxidable lavable de $120\text{ Mesh}$ ($125 \ \mu\text{m}$) para proteger la manguera y los rodillos de la bomba contra gravas o partículas macroscópicas.
   * Bomba peristáltica MBP-2000 accionada por motor paso a paso NEMA 34 con driver DM860.
   * Manómetro mecánico Bourdon amortiguado en glicerina $P_1$ ($0\text{--}1\text{ bar}$ ó $0\text{--}2.5\text{ bar}$) para medir la presión de impulsión en el lumen.
   * Caudalímetro primario $Q_{\text{alim}}$ (YF-S401) con filtro digital anti-EMI.

2. **Módulo de Filtración Tangencial FX100:**
   * Dializador capilar Fresenius Medical Care CorDiax FX100 ($1.8\text{ m}^2$ de superficie activa, $\sim 10.900$ fibras de polisulfona modificada Helixone®).
   * Línea de retentado con válvula de aguja micrométrica para regular la contrapresión y fijar la $\text{TMP}$ ($0.10$, $0.25$ y $0.40\text{ bar}$).
   * Manómetro de glicerina $P_2$ en la salida de retentado.

3. **Línea de Permeado y Retrolavado:**
   * Caudalímetro de permeado $Q_{\text{perm}}$ (YF-S401) asistido por probeta en régimen de ensuciamiento severo ($< 10\text{ mL/min}$).
   * Descarga atmosférica hacia tanque acumulador de permeado.
   * Puerto para retrolavado hidrostático (*Backwash*) a $1.20\text{ mca}$ ($\sim 0.118\text{ bar}$) para desprendimiento de torta reversible.

---

## 📂 3. Archivos en esta Carpeta

| Archivo | Tipo | Descripción |
| :--- | :--- | :--- |
| **[`diagrama_pid_interactivo.html`](./diagrama_pid_interactivo.html)** | Simulador Web HTML5/SVG | Diagrama P&ID interactivo con animación en tiempo real de flujos, válvulas, bombas y sensores. |
| **[`boceto_original_enzo_pid.jpg`](./boceto_original_enzo_pid.jpg)** | Fotografía técnica | Plano conceptual original dibujado a mano en el laboratorio de la UNSa. |
| **[`Guia_Construccion_y_Montaje_Tanques.md`](./Guia_Construccion_y_Montaje_Tanques.md)** | Manual de Montaje | Especificación de perforaciones para tanques, instalación de pasamuros, posicionamiento de boya de nivel y sellado. |
| **[`ParametrosFiltroFX100.txt`](./ParametrosFiltroFX100.txt)** | Ficha Técnica | Constantes del fabricante Fresenius para el filtro FX100 (área $1.8\text{ m}^2$, $K_{UF} = 73\text{ mL/(h}\cdot\text{mmHg)}$, $R_m = 2.09 \times 10^{13}\text{ m}^{-1}$). |

---

## 🏁 4. Criterios de Aceptación para Paso al Hito 2

1. [x] Diagrama de instrumentación ISA-5.1 validado y conciliado con la arquitectura de manómetros de glicerina.
2. [x] Selección de manómetros de baja escala ($0\text{--}1\text{ bar}$ / $0\text{--}2.5\text{ bar}$) con baño de glicerina para suprimir oscilaciones de los rodillos peristálticos.
3. [x] Guía de tanques y niveles mínimos aprobada para evitar aspiración de burbujas de aire al dializador.
4. [ ] **Habilitación formal:** Interconexión hidráulica para calibración de caudalímetros y verificación de presiones en el **Hito 2**.
