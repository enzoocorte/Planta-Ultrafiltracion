# 🛠️ ORDEN DE TRABAJO PARA HERRERÍA / METALÚRGICA
## Planta Piloto de Ultrafiltración FX100 — UNSa 2026

**Trabajo:** Fabricación de Gabinete Metálico tipo "Maletín Industrial" (Bastidor + Chapas)  
**Medidas Finales Exteriores:** **500 mm (Alto) × 350 mm (Ancho) × 350 mm (Profundidad)**  
**Tolerancia de escuadra en la base:** $\pm 1\ \text{mm}$ (crítico: calza sobre base existente y carro inferior).  
**Destino:** Módulo de Filtración y Bombeo (Skid Desmontable)  

---

## 1. LISTA DE MATERIALES REQUERIDOS

1. **Estructura Portante (Esqueleto):**
   * 1 barra comercial (6 metros) de **Caño Estructural Cuadrado 20 × 20 mm (espesor 1.2 mm o 1.6 mm)**.
2. **Techo y Lateral Derecho (Chapas Ciegas):**
   * **Chapa lisa de hierro SAE 1010 N°18 (1.2 mm) o N°16 (1.6 mm)**:
     * 1 corte de **350 × 350 mm** (Techo).
     * 1 corte de **500 × 350 mm** (Lateral Derecho - Panel de Conexiones).
3. **Lateral Izquierdo y Tapa Trasera (Ventilación):**
   * **Chapa Perforada de hierro N°18 (1.2 mm)** (perforación redonda de $\varnothing\ 3\ \text{a}\ 5\ \text{mm}$ al tresbolillo):
     * 1 corte de **500 × 350 mm** (Lateral Izquierdo - Fijo soldado/remachado).
     * 1 corte de **500 × 350 mm** (Fondo Trasero - **DESMONTABLE con tornillos, NO SOLDAR**).
4. **Herrajes y Accesorios de Fijación:**
   * **1 Manija metálica rebatible reforzada** con resorte (tipo baúl / flight-case / amplificador) con empuñadura de goma.
   * **2 Bulones de acero M6 × 40 mm** con arandelas anchas (arandela chapista) y tuercas autofrenantes (nylon).
   * **4 Regatones plásticos embutidos** de $20 \times 20\ \text{mm}$ o **4 patas de goma cilíndricas** atornillables para las esquinas inferiores.
   * **8 Tornillos autoperforantes cabeza hexagonal 4.2 × 16 mm** (o remaches roscados M5) para la tapa trasera desmontable.

---

## 2. PLANILLA DE CORTE DEL CAÑO 20 × 20 mm (Total: 5.10 metros)

Para conformar el prisma exterior exacto de **500 mm (Alto) × 350 mm (Ancho) × 350 mm (Fondo)**:

| Cantidad | Denominación del Corte | Longitud de Corte | Observaciones |
| :---: | :--- | :---: | :--- |
| **4** | Parantes Verticales (Columnas) | **500 mm** | Cortes a $90^\circ$ limpios y a escuadra. |
| **4** | Travesaños Marco Superior (Techo) | **310 mm** (o 350 mm si es a $45^\circ$) | Forma cuadro exterior de $350 \times 350\ \text{mm}$. |
| **4** | Travesaños Marco Inferior (Base) | **310 mm** (o 350 mm si es a $45^\circ$) | Forma cuadro exterior de $350 \times 350\ \text{mm}$. |
| **1** | Travesaño Central Puente (Techo) | **310 mm** | **Refuerzo de izado:** Va soldado al medio del marco superior para recibir los bulones de la manija. |

*(Suma de cortes: $4 \times 500 + 4 \times 310 + 4 \times 310 + 310 = 2000 + 1240 + 1240 + 310 = 4790\ \text{mm} = 4.79\ \text{m}$. Una barra de 6 m cubre todo el bastidor con sobrante).*

---

## 3. INSTRUCCIONES DE ARMADO Y SOLDADURA (CARA POR CARA)

```text
               [MANIJA REBATIBLE M6]
                        ||
            +===========||===========+  <-- TECHO (350x350 mm): Chapa Lisa soldada
           /           /||          /|      con puente central de caño 20x20 debajo.
          /           / ||         / |
         +------------------------+  |  <-- LATERAL DER. (500x350): Chapa Lisa ciega.
         |                        |  |
         |                        |  |  <-- LATERAL IZQ. (500x350): Chapa Perforada fija.
         |     FRENTE ABIERTO     |  |
         |      (500x350 mm)      |  +  <-- FONDO TRASERO (500x350): Chapa Perforada
         |   (Con orejas o marco  | /       DESMONTABLE (con tornillos, ¡NO SOLDAR!).
         |    para placa frontal) |/
         +------------------------+     <-- BASE INFERIOR (350x350): Marco abierto con
                                            orejas para atornillar madera desde abajo.
```

### A. Techo Superior (350 × 350 mm) — ¡Refuerzo Estructural de Izado!
* Soldar el travesaño central de caño $20 \times 20\ \text{mm}$ cruzando de lado a lado exactamente al centro del marco superior.
* Soldar por arriba la **chapa lisa de 350 × 350 mm** con cordón perimetral continuo o punteado cerrado amolado al ras.
* Perforar en el centro dos orificios pasantes de **$\varnothing\ 6.5\ \text{mm}$** que atraviesen **tanto la chapa como el caño central de 20 × 20 mm**.
* Abulonar la manija rebatible con los 2 bulones M6 pasantes, arandelas de ala ancha por debajo y tuercas autofrenantes.  
  *(De esta forma, al levantar los 13–15 kg del maletín, la tracción mecánica es soportada directamente por el caño estructural y no flexiona la chapa).*

### B. Base Inferior (350 × 350 mm) — Alojamiento de la Base de Madera Actual
* El cuadro inferior de caño $20 \times 20\ \text{mm}$ queda **abierto** (sin chapa metálica de fondo), ya que allí calza la base de madera fenólica/multilaminada de $18\ \text{mm}$ donde apoya el motor NEMA 34.
* **Detalle clave:** Soldar del lado interno del marco inferior **6 u 8 pequeñas orejas de planchuela perforadas ($\varnothing\ 5.5\ \text{a } 6\ \text{mm}$)** al ras de la base inferior (o perforar verticalmente los caños) para atornillar la placa de madera firmemente con tirafondos desde abajo.

### C. Lateral Izquierdo (500 × 350 mm) — Ventilación Fija
* Soldar por puntos (o remachar) el paño de **Chapa Perforada N°18 de 500 × 350 mm** contra el marco de caño.
* Amolar todas las rebabas de corte para evitar filos cortantes al manipular el maletín.

### D. Lateral Derecho (500 × 350 mm) — Panel Eléctrico Ciego
* Soldar por puntos el paño de **Chapa Lisa N°18 de 500 × 350 mm**.
* Este panel queda cerrado para posterior mecanizado en laboratorio (instalación del conector C14 220V interlock y conector industrial GX16 de 4 pines).

### E. Fondo Trasero (500 × 350 mm) — ⚠️ ¡OBLIGATORIAMENTE DESMONTABLE!
* **PROHIBIDO SOLDAR ESTA CARA.**
* El paño de **Chapa Perforada N°18 de 500 × 350 mm** se fija al bastidor mediante **6 u 8 tornillos autoperforantes cabeza hexagonal** (o remaches roscados M5).
* *Justificación técnica:* Es la compuerta de acceso técnico para montaje mecánico del motor NEMA 34, transformador de 24 VAC, driver DM860 y tareas de cableado y mantenimiento.

### F. Cara Frontal (500 × 350 mm) — Fachada de Proceso
Se selecciona una de las dos opciones constructivas:
* **Opción Madera Fenólica (Recomendada por rapidez):** Dejar la cara frontal del marco de caño $20 \times 20\ \text{mm}$ limpia y al ras, con 6 orificios pasantes de $\varnothing\ 5\ \text{mm}$ para atornillar desde afuera una placa de fenólico marino de 12–15 mm barnizada.
* **Opción Sándwich Metálico Separado:** Soldar una chapa lisa ciega de $500 \times 350\ \text{mm}$ cerrando el frente, y sobre las 4 esquinas exteriores soldar **4 tuercas altas M8 (de 15 mm de separación)**, sobre las cuales se atornilla una chapa perforada de $500 \times 350\ \text{mm}$ separada 15 mm. Permite colgar accesorios en los agujeros sin perforar el gabinete estanco.

---

## 4. TERMINACIÓN SUPERFICIAL Y PINTURA

1. **Desbastado y Amolado:** Amolar al ras todas las costuras de soldadura exteriores y redondear suavemente las 8 esquinas para evitar bordes vivos.
2. **Tratamiento Anticorrosivo:** Desengrasar a fondo con solvente y aplicar 1 mano uniforme de **fondo antióxido sintético o convertidor de óxido al cromato/epoxi**.
3. **Pintura de Acabado:** Aplicar **2 manos de esmalte sintético o poliuretánico (color Negro Satinado, Gris Grafito o Azul Industrial)** apto para servicio en laboratorio con humedad.

---

## 📋 REGLA DE VERIFICACIÓN EN TALLER
> Llevar al taller la **placa de madera actual donde está fijado el motor NEMA 34** para que el herrero verifique la escuadra y el calce exacto del marco inferior antes de tirar los cordones de soldadura definitivos.
