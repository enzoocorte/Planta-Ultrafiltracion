# 🔬 PROTOCOLO SUBHITO 5.6: Autopsia Morfológica Post-Mortem de las Fibras Capilares Helixone® (SEM / EDS)
## Microscopía Electrónica de Barrido, Espesor Micrométrico de Torta, Perfil Axial y Microanálisis Elemental

**Proyecto:** Planta Piloto de Ultrafiltración por Flujo Cruzado & Reactor de Coagulación-Sedimentación  
**Tesistas de Grado:** Antonella Guitián & Owen Cañizares  
**Codirector:** Ing. Enzo *(Tesis Doctoral en Procesos de Separación por Membranas)*  
**Ubicación:** Laboratorio de Ingeniería y Centro de Microscopía Electrónica, UNSa  
**Fecha:** Octubre 2026  

---

> [!CAUTION]
> **PROTOCOLO DESTRUCTIVO — CONDICIÓN TEMPORAL CRÍTICA:**  
> La autopsia requiere el corte de la carcasa de policarbonato del cartucho Fresenius FX100 y la extracción irreversible de fibras capilares. Este protocolo se ejecuta **ESTRICTAMENTE AL FINALIZAR LA TOTALIDAD DE LAS 13 CORRIDAS EXPERIMENTALES** de la tesis.

---

## 🎯 1. Objetivos de la Autopsia SEM / EDS

1. **Medición Directa del Espesor de Torta ($\delta_c$):** Medir micrométricamente sobre el corte transversal el espesor de la capa coloidal depositada en la cara luminal interna de la fibra, correlacionándolo con la resistencia hidráulica de torta $R_{\text{torta}}$ calculada por el modelo de Darcy en el SCADA.
2. **Explicación Física de la Resistencia Irreversible ($R_{\text{irrev}}$):** Contrastar la morfología superficial de:
   * **Fibra Virgen** (control de referencia).
   * **Fibra Ensuciada** (post-ensayo, previa a retrolavado).
   * **Fibra Regenerada** (post-retrolavado hidrostático), identificando coloides adsorbidos dentro de los microporos.
3. **Distribución Axial del Ensuciamiento:** Cuantificar el gradiente longitudinal de acumulación de torta en la **Entrada ($Z_1$)**, **Centro ($Z_2$)** y **Salida ($Z_3$)** del cartucho.
4. **Microanálisis Elemental por EDS:** Identificar la composición química elemental de la torta (arcillas inorgánicas $\text{Si}, \text{Al}, \text{Fe}$ vs. biopolímero orgánico de *Opuntia* $\text{C}, \text{O}$).

---

## 📐 2. Características Geométricas de la Fibra Capilar Helixone®

El cartucho Fresenius Medical Care FX100 está compuesto por aproximadamente **13.500 capilares huecos** de membrana asimétrica de Polisulfona / Polivinilpirrolidona (PSf/PVP):

```text
               CORTE TRANSVERSAL DE FIBRA HUECA HELIXONE®
   
       ┌────────────────── D_ext = 255 µm ──────────────────┐
       │                                                    │
       │     ██████████████████████████████████████████     │
       │   ███                                      ███     │
       │  ██    ┌───────── D_int = 185 µm ────────┐   ██    │
       │ ██     │                                 │    ██   │
       │ ██  Espesor │      LUMEN INTERNO        │    ██   │
       │ ██   Pared  │  (Capa activa filtrante)  │    ██   │
       │ ██  = 35 µm │       [Flujo Tangencial]   │    ██   │
       │ ██     │                                 │    ██   │
       │  ██    └─────────────────────────────────┘   ██    │
       │   ███                                      ███     │
       │     ██████████████████████████████████████████     │
       └────────────────────────────────────────────────────┘
                  ▲
                  └─ Capa soporte macroporosa externa
```

* **Diámetro interno ($d_i$):** $185\ \mu\text{m}$.
* **Espesor de pared ($\Delta r$):** $35\ \mu\text{m}$.
* **Diámetro externo ($d_e$):** $255\ \mu\text{m}$.
* **Estructura Asimétrica:** Capa selectiva ultrafina ($0.1 - 1\ \mu\text{m}$) sobre la cara interna (lumen) y estructura soporte trabecular abierta hacia el exterior (shell).

---

## 🛠️ 3. Protocolo de Disección y Preparación de Muestras

### Paso 1: Drenaje y Secado Preliminar
1. Drenar completamente el agua libre dentro del cartucho FX100 mediante aire comprimido a baja presión ($< 0.2\text{ bar}$) durante 10 minutos.
2. Dejar secar el cartucho en desecador con sílica gel a temperatura ambiente ($20^\circ\text{C}$) durante 48 horas.

### Paso 2: Apertura Mecánica de la Carcasa
1. Fijar el módulo en un soporte acolchado de banco.
2. Realizar dos cortes longitudinales opuestos sobre la carcasa cilíndrica de policarbonato utilizando una minisierra circular de mano a bajas RPM, con tope de profundidad de $2.0\text{ mm}$ para **no cortar ni raspar el haz de fibras capilares**.
3. Abrir la carcasa en dos valvas y retirar con cuidado los cabezales de resina epoxi de ambos extremos (*potting*).

### Paso 3: Muestreo Longitudinal (3 Zonas Axiales)
Cortar con tijeras microquirúrgicas fascículos de fibras de aproximadamente $3\text{ cm}$ de longitud en tres posiciones:
* **Muestra Z1 (Entrada / Feed):** A $2\text{ cm}$ de la placa de entrada.
* **Muestra Z2 (Centro del Cartucho):** En la zona media longitudinal ($L/2$).
* **Muestra Z3 (Salida / Retentado):** A $2\text{ cm}$ de la placa de salida.

### Paso 4: Fractura Criogénica para Cross-Section (Corte Transversal)
> [!IMPORTANT]
> **Prohibido cortar con bisturí o tijera a temperatura ambiente:** La polisulfona se aplasta plásticamente, sellando los poros y destruyendo la torta.  
1. Sumergir las fibras en un recipiente criogénico con **Nitrógeno Líquido ($-196^\circ\text{C}$)** durante $60\text{ segundos}$.
2. En estado vítreo ultra-frágil, quebrar las fibras transversalmente con pinzas de punta fina enfriadas. La fractura resultante es perfectamente perpendicular y plana, preservando intactos los poros y el espesor de la torta.

### Paso 5: Preparación de Superficie Luminal
Para observar el interior del lumen, cortar longitudinalmente una fibra con hoja de afeitar enfriada, abriendo el cilindro en canaleta para exponer la pared filtrante activa.

### Paso 6: Metalizado (Sputtering)
Montar las muestras sobre portamuestras metálicos cilíndricos (*stubs*) con cinta de carbono conductora de doble faz. Aplicar un recubrimiento conductivo de **Oro/Paladio (Au/Pd) de $10\text{ nm}$** de espesor en cámara de plasma a alto vacío.

---

## 📸 4. Observación en el Microscopio Electrónico (SEM)

### 4.1. Medición de Espesor de Torta ($\delta_c$) y Correlación con Darcy
* **Aumento recomendado:** 1.000x a 5.000x sobre el corte transversal.
* **Procedimiento:** Tomar 10 mediciones micrométricas del espesor de la torta $\delta_c\ [\mu\text{m}]$ distribuidas en el perímetro luminal mediante la herramienta de calibración del software SEM.
* **Cálculo de la Resistencia Específica de Torta ($\alpha$):**
  $$\delta_c = \frac{M_s}{\rho_{\text{torta}} \cdot A_m} \quad \Longrightarrow \quad R_{\text{torta}} = \alpha \cdot \frac{M_s}{A_m} = \frac{\delta_c}{P_{\text{torta}}}$$
  Donde $P_{\text{torta}}$ es la permeabilidad intrínseca de la capa coloidal calculada para la tesis.

### 4.2. Superficie Luminal (Bloqueo de Poros)
* **Aumento recomendado:** 5.000x a 25.000x.
* **Comparación Tripartita para la Tesis:**
  1. **Fibra Virgen:** Textura homogénea y microporos de polisulfona abiertos ($0.01 - 0.04\ \mu\text{m}$).
  2. **Fibra Ensuciada (Pre-Lavado):** Cobertura continua de agregados de bentonita/caolín y matriz mucilaginosa de *Opuntia*.
  3. **Fibra Post-Retrolavado:** Desprendimiento de la torta superficial pero permanencia de depósitos residuales en las constricciones de poro, evidenciando el origen físico de la resistencia irreversible $R_{\text{irrev}}$.

### 4.3. Perfil Longitudinal (Entrada vs. Centro vs. Salida)
* Demostrar experimentalmente si el espesor de torta aumenta hacia la salida:
  $$\delta_c(Z_1) < \delta_c(Z_2) < \delta_c(Z_3)$$
  Correlacionado con el incremento de concentración de sólidos suspendidos en el retentado a lo largo del canal.

---

## 🧪 5. Microanálisis Elemental por Rayos X (EDS)

Configurar el detector EDS a un voltaje de aceleración de **$15\text{ a }20\text{ keV}$**:
1. **Espectro de la Membrana Virgen (Sustrato):**
   * Picos dominantes de **Carbono ($\text{C}$)**, **Oxígeno ($\text{O}$)** y **Azufre ($\text{S}$)** (característico del grupo sulfona $-\text{SO}_2-$ de la polisulfona).
2. **Espectro de la Torta Coloidal:**
   * **Silicio ($\text{Si}$)** y **Aluminio ($\text{Al}$):** Confirman la deposición de filosilicatos de la suspensión de arcilla sintética.
   * **Hierro ($\text{Fe}$), Calcio ($\text{Ca}$) y Magnesio ($\text{Mg}$):** Cationes divalentes que actúan como puente electrostático.
   * **Exceso de Carbono ($\text{C}$) y Oxígeno ($\text{O}$):** Huella química de los polisacáridos y arabinogalactanos del mucílago de *Opuntia ficus-indica*.

---

## 📋 6. Entregables del Subhito 5.6 para el Informe de Tesis
1. Micrografías SEM en alta resolución (formato TIFF / JPG a 300 DPI) con barra de escala calibrada.
2. Histograma de distribución de espesores de torta $\delta_c$ en entrada, centro y salida.
3. Tabla de correlación entre $R_{\text{torta}}$ (modelo hidráulico Darcy del SCADA) vs. $\delta_c$ (observación microscópica real).
4. Espectros EDS comparativos cuantificados en porcentaje másico ($\%\ \text{peso}$) y atómico ($\%\ \text{atómico}$).
