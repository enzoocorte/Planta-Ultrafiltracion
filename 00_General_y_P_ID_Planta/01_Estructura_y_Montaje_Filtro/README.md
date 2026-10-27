# 🏗️ Estructura Física, Bastidor y Modularidad Mecánica del Filtro
## Planta Piloto de Ultrafiltración FX100 — UNSa 2026

**Ubicación:** `00_General_y_P_ID_Planta/01_Estructura_y_Montaje_Filtro/`  
**Responsables:** Antonella Guitián & Owen Cañizares (Tesistas) | Ing. Enzo Corte (Codirector)  
**Disciplina:** Ingeniería Mecánica / Industrial aplicada a Procesos Químicos

---

## 🎯 1. Objetivo del Módulo Mecánico y Estructural

Diseñar, validar y construir el bastidor físico modular de la planta piloto bajo el paradigma de **Diseño para Fabricación y Ensamble (DFM/DFA)**. El sistema asegura la **segregación estricta entre fluidos presurizados y electrónica**, garantiza la estabilidad dinámica de la bomba peristáltica NEMA 34 frente a vibraciones mecánicas y proporciona una arquitectura portátil en **tres (3) módulos interconectables**.

---

## 📐 2. Arquitectura de los Tres (3) Módulos

```
                                  [ PLANTA PILOTO MODULAR ]
                                              │
           ┌──────────────────────────────────┼──────────────────────────────────┐
           ▼                                  ▼                                  ▼
 ┌───────────────────┐             ┌───────────────────────┐          ┌───────────────────────┐
 │    MÓDULO 1       │             │       MÓDULO 2        │          │       MÓDULO 3        │
 │ REACTOR CÓNICO Y  │             │   CAJA DE FILTRACIÓN  │          │    TORRE / CARRO DE   │
 │   SEDIMENTADOR    │             │       Y CONTROL       │          │   PERMEADO Y SOPORTE  │
 ├───────────────────┤             ├───────────────────────┤          ├───────────────────────┤
 │ • Tanque Inox     │  Manguera   │ • Bomba Peristáltica  │ Manguera │ • Tanque Permeado     │
 │ • Base Trípode    │ ──────────► │ • Membrana FX100      │ ───────► │   (Bidón PEAD 50L) │
 │ • Agitador + Boya │  Aspiración │ • Caja Estanca IP65   │ Permeado │ • Ruedas con Freno    │
 │ • Purga de Fondo  │             │   (ESP32 + Drivers)   │          │ • Encastre Caja Mod 2 │
 └───────────────────┘             └───────────────────────┘          └───────────────────────┘
```

1. **Módulo 1 — Reactor y Sedimentador Cónico:**
   * Tanque de acero inoxidable de $43\text{ cm}$ de altura con fondo cónico para decantación de flóculos de bentonita/*Opuntia*.
   * Base de apoyo con patas niveladoras para asegurar la simetría axial del vértice de purga de lodos.
   * Sistema de agitación con eje guiado y boya de seguridad de acero inoxidable contra marcha en seco.
2. **Módulo 2 — Caja de Filtración y Control (450 × 450 × 450 mm):**
   * Gabinete cúbico portátil con manijas de transporte tipo maletín.
   * Chapa lateral perforada para ventilación forzada y disipación térmica del motor NEMA 34 y driver DM860.
   * **Caja estanca plástica estandarizada (IP65)** en lateral independiente, segregando la electrónica (ESP32, fuentes, borneras RC) de las mangueras de agua a presión.
   * Soporte elástico antivibratorio para la membrana Fresenius CorDiax FX100 y bomba peristáltica MBP-2000.
3. **Módulo 3 — Torre/Carro de Permeado y Soporte:**
   * Estructura rodante móvil (altura $700\text{ mm}$) con 4 ruedas giratorias (2 con freno de bloqueo).
   * Bandeja inferior para tanque acumulador de permeado (bidón PEAD de $20\text{ a } 25\text{ Litros}$).
   * Brazos superiores con topes mecánicos para encastre rápido y fijación sin herramientas del Módulo 2.

---

## 📂 3. Archivos y Recursos en esta Carpeta

| Archivo | Formato / Tipo | Descripción y Función Técnica |
| :--- | :--- | :--- |
| **[`DOCUMENTO_MAESTRO_ESTRUCTURA_FILTRO.md`](./DOCUMENTO_MAESTRO_ESTRUCTURA_FILTRO.md)** | Documento Markdown | **Memoria técnica exhaustiva de ingeniería:** diagnóstico dimensional, análisis de esbeltez de eje, cálculo de potencia de agitación, segregación IP65, hojas de proceso y costos de manufactura. |
| **[`Estructura para el modulo de ultrafiltracion.docx`](./Estructura%20para%20el%20modulo%20de%20ultrafiltracion.docx)** | Documento Word | Especificación preliminar de requerimientos mecánicos y cortes de chapa. |
| **[`guia_visual_estructura.html`](./guia_visual_estructura.html)** | Visualizador 3D (Three.js) | Maqueta virtual interactiva en 3D para rotar, orbitar y despiezar los módulos mecánicos en el navegador. |
| **[`3 modulos.jpeg`](./3%20modulos.jpeg)** | Fotografía técnica | Esquema fotográfico del conjunto ensamblado de los 3 módulos. |
| **[`base del sedimentador.jpeg`](./base%20del%20sedimentador.jpeg)** | Fotografía técnica | Relevamiento de la base metálica del sedimentador y puntos de soldadura. |
| **[`modulo sedimentador.jpeg`](./modulo%20sedimentador.jpeg)** | Fotografía técnica | Vista lateral del tanque reactor inoxidable montado. |
| **[`explicacion estructura.jpeg`](./explicacion%20estructura.jpeg)** | Esquema visual | Plano con cotas preliminares y distribución de espacios de la caja de filtración. |
| **[`paleta giratoria.jpeg`](./paleta%20giratoria.jpeg)** | Fotografía técnica | Detalle del rotor de agitación y vástago de transmisión. |
| **[`tanque permeado.jpg`](./tanque%20permeado.jpg)** | Fotografía técnica | Dimensionamiento del bidón colector de agua clarificada en el Módulo 3. |

---

## 📊 4. Estado de Avance

* **Diseño Dimensional y Conceptual:** 100% completado (validado en `DOCUMENTO_MAESTRO_ESTRUCTURA_FILTRO.md`).
* **Modelado 3D Interactivo:** 100% completado (`guia_visual_estructura.html`).
* **Segregación IP65 / DFM:** 100% definida (caja estanca para ESP32 y aislamiento hidráulico).
* **Fabricación de Bastidor en Taller:** 85% completado (en etapa de ensamble y ajuste de encastres).

---

## 🏁 5. Criterios de Aceptación para Paso al Hito 1

1. [x] Verificación de la rigidez torsional del soporte de la bomba peristáltica frente a un torque motriz de $4.5\text{ Nm}$.
2. [x] Barrera física estanca verificada entre conexiones eléctricas a $220\text{V}/12\text{V}$ y el circuito presurizado de mangueras.
3. [x] Nivelación de la base cónica del sedimentador para garantizar descarga centrada de lodos.
4. [ ] **Habilitación formal:** Montaje definitivo de la bomba peristáltica y driver DM860 en el Módulo 2 para iniciar las pruebas cinemáticas del **Hito 1**.
