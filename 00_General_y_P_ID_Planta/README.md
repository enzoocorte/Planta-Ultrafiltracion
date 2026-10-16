# 📐 HITO 0: Arquitectura General, Estructura Mecánica y P&ID de la Planta
## Planta Piloto de Coagulación-Sedimentación y Ultrafiltración FX100 (Helixone®)
**Proyecto de Tesis de Grado:** Universidad Nacional de Salta (UNSa) — Facultad de Ingeniería  
**Tesistas:** Antonella Guitián & Owen Cañizares | **Codirector:** Ing. Enzo Corte  
**Ubicación:** `00_General_y_P_ID_Planta/`

---

## 🎯 1. Objetivo General del Hito 0

Establecer la **línea de base arquitectónica, mecánica e hidráulica** de la planta piloto mediante el diseño del bastidor físico modular de 3 cuerpos, la formalización del diagrama de flujo e instrumentación (P&ID bajo norma ISA-5.1) y la definición de las interfaces de acople entre el pretratamiento fisicoquímico y la separación por membranas.

---

## 🔬 2. Objetivos Específicos

1. **Diseño Estructural Modular (DFM/DFA):** Diseñar el bastidor en tres módulos autónomos (Módulo 1: Sedimentador cónico; Módulo 2: Caja de filtración y bombeo 450×450×450 mm; Módulo 3: Torre rodante de permeado) garantizando ergonomía y portabilidad.
2. **Segregación Eléctrico-Hidráulica (Seguridad Ocupacional):** Aislar las fuentes de $220\text{V}/12\text{V}$ y el microcontrolador ESP32 dentro de un gabinete estanco IP65, separándolo físicamente del circuito presurizado de mangueras.
3. **Diagrama P&ID Formal (Norma ISA-5.1):** Mapear el recorrido de fluidos (agua turbia, coagulante, sobrenadante, permeado, retentado, purga y retrolavado), instrumentos y lazos de control hacia el ESP32.
4. **Definición de Circuitos Hidráulicos y Tanques:** Establecer el posicionamiento de boyas de nivel mínimo, prefiltro tipo Y-Strainer (120 Mesh) y puntos de toma manométrica con baño de glicerina ($P_1, P_2$).

---

## 📂 3. Mapa de Contenido y Subcarpetas

La carpeta `00_General_y_P_ID_Planta/` centraliza todo el diseño conceptual y físico preliminar del proyecto:

```
00_General_y_P_ID_Planta/
├── 📁 01_Estructura_y_Montaje_Filtro/     ──> 🏗️ Diseño mecánico, DFM/DFA, planos, 3D Three.js y fotos de bastidor
├── 📁 02_Diagrama_PID_y_Filosofia_Control/ ──> 📐 P&ID ISA-5.1, simulador interactivo, tanques y constantes FX100
├── 📁 03_Organizacion_Equipo_y_Protocolos/ ──> 👥 Roles del equipo, gobernanza de Git y protocolo de trabajo
└── 📄 README.md                            ──> 🧭 Este documento maestro del Hito 0
```

### 3.1 Subcarpeta `01_Estructura_y_Montaje_Filtro/`
* [README.md](./01_Estructura_y_Montaje_Filtro/README.md): Explicación técnica de la modularidad en 3 cuerpos y análisis de esfuerzos.
* [`DOCUMENTO_MAESTRO_ESTRUCTURA_FILTRO.md`](./01_Estructura_y_Montaje_Filtro/DOCUMENTO_MAESTRO_ESTRUCTURA_FILTRO.md): Memoria técnica detallada de ingeniería industrial (análisis dimensional, esbeltez de eje, segregación IP65, hojas de proceso y costos).
* [`guia_visual_estructura.html`](./01_Estructura_y_Montaje_Filtro/guia_visual_estructura.html): Maqueta 3D interactiva renderizada con Three.js en el navegador.
* [`Estructura para el modulo de ultrafiltracion.docx`](./01_Estructura_y_Montaje_Filtro/Estructura%20para%20el%20modulo%20de%20ultrafiltracion.docx): Documento preliminar de despiece y cortes de chapa.
* Fotografías y esquemas mecánicos: `3 modulos.jpeg`, `base del sedimentador.jpeg`, `modulo sedimentador.jpeg`, `explicacion estructura.jpeg`, `paleta giratoria.jpeg`, `tanque permeado.jpg`.

### 3.2 Subcarpeta `02_Diagrama_PID_y_Filosofia_Control/`
* [README.md](./02_Diagrama_PID_y_Filosofia_Control/README.md): Filosofía hidráulica, circuitos y balance de presiones.
* [`diagrama_pid_interactivo.html`](./02_Diagrama_PID_y_Filosofia_Control/diagrama_pid_interactivo.html): Simulador interactivo del proceso P&ID animado en SVG/HTML5.
* [`boceto_original_enzo_pid.jpg`](./02_Diagrama_PID_y_Filosofia_Control/boceto_original_enzo_pid.jpg): Plano original manuscrito en taller de la UNSa.
* [`Guia_Construccion_y_Montaje_Tanques.md`](./02_Diagrama_PID_y_Filosofia_Control/Guia_Construccion_y_Montaje_Tanques.md): Procedimiento para perforación, sellado de pasamuros y montaje de boyas.
* [`ParametrosFiltroFX100.txt`](./02_Diagrama_PID_y_Filosofia_Control/ParametrosFiltroFX100.txt): Ficha técnica con constantes hidráulicas del dializador FX100.

### 3.3 Subcarpeta `03_Organizacion_Equipo_y_Protocolos/`
* [README.md](./03_Organizacion_Equipo_y_Protocolos/README.md): Guía de roles y sincronización.
* [`ORGANIZACION_PROYECTO_Y_EQUIPO.md`](./03_Organizacion_Equipo_y_Protocolos/ORGANIZACION_PROYECTO_Y_EQUIPO.md): Manual operativo del equipo (Antonella, Owen, Ing. Enzo).

---

## 📊 4. Estado de Avance Actual

| Tarea / Componente | Estado | Porcentaje | Observaciones |
| :--- | :---: | :---: | :--- |
| **Diagrama P&ID bajo norma ISA-5.1** | ✅ Completado | 100% | Validado e implementado en simulador interactivo. |
| **Diseño Conceptual y DFM de Estructura** | ✅ Completado | 100% | Especificado en `DOCUMENTO_MAESTRO_ESTRUCTURA_FILTRO.md`. |
| **Modelado 3D de Bastidor (Three.js)** | ✅ Completado | 100% | Disponible en `guia_visual_estructura.html`. |
| **Definición de Circuitos Hidráulicos y Tanques** | ✅ Completado | 100% | Guía de tanques y selección de mallas lavables finalizada. |
| **Fabricación de Bastidor en Taller** | 🟡 En Curso | 85% | Ensamblaje de base cónica y carro rodante en ejecución. |

---

## 🏁 5. Entregables Críticos para Habilitar el Paso al Hito 1

Para considerar superado el Hito 0 y proceder formalmente con los ensayos cinemáticos del Hito 1, deben cumplirse las siguientes condiciones de compuerta (*Stage-Gate*):

1. **Gabinete Módulo 2 preparado:** Bastidor metálico con bandeja para soportar el cabezal de la bomba peristáltica MBP-2000 y el motor NEMA 34.
2. **Caja Estanca IP65 montada:** Compartimento segregado con perforaciones pasacables tipo prensaestopas para alojar el driver DM860 y el ESP32 sin exposición a agua.
3. **P&ID Validado:** Aprobación del circuito de recirculación y puntos de conexionado para alimentación, retentado y permeado.

> [!TIP]
> Una vez verificado el montaje físico del motor sobre el bastidor, dirigirse a [`01_Hito1_Control_Accionamiento_NEMA34_DM860/`](file:///c:/Users/enzoo/OneDrive/Documentos/ENZO/Domotica/SistemaUF/01_Hito1_Control_Accionamiento_NEMA34_DM860/README.md) para el control cinemático y prueba de torque de la bomba.
