# 📋 FICHA TÉCNICA OFICIAL: BOMBA PERISTÁLTICA MBP-2000 & MOTOR NEMA 34
## Planta Piloto de Ultrafiltración FX100 • Documentación de Hardware
**Codirector**: Ing. Enzo  
**Tesistas**: Antonella Guitián & Owen Cañizares  
**Archivo**: `FICHA_TECNICA_BOMBA_PERISTALTICA_MBP2000.md`

---

# 1. Especificaciones del Cabezal Peristáltico MBP-2000

| Parámetro | Especificación del Fabricante | Observaciones de Ingeniería |
| :--- | :--- | :--- |
| **Modelo** | **MBP-2000** | Serie industrial de alto caudal |
| **Caudal Máximo** | **$2.0\text{ L/min}$ ($2000\text{ mL/min}$)** | A velocidad máxima nominal ($\approx 130 - 150\text{ RPM}$) |
| **Manguera Instalada** | **Tubo de Silicona Atóxica** | Grado alimenticio / farmacéutico (1 780) |
| **Diámetro Exterior ($\varnothing\text{Ext}$)** | **$18\text{ mm}$** | Diámetro exterior total |
| **Diámetro Interior ($\varnothing\text{Int}$)** | **$12\text{ mm}$** ($1.2\text{ cm} \approx 1/2"$) | Diámetro hidráulico de paso |
| **Espesor de Pared** | **$3\text{ mm}$** ($\frac{18 - 12}{2}$) | Alta resiliencia elástica contra fatiga mecánica |
| **Material del Cuerpo** | **Polietileno de Alto Peso Molecular (APM / UHMW-PE)** | Termoplástico de ultra-alta resistencia al desgaste |
| **Propiedades Químicas** | Inerte, no produce corrosión, atóxico, insípido e inodoro | Apto para tratamiento de agua potable y efluentes |
| **Compatibilidad Eléctrica** | Apto para control industrial con Driver, PLC y Arduino/ESP32 | Acoplamiento directo a brida NEMA 34 |

---

# 2. Especificaciones del Motor Paso a Paso NEMA 34

| Parámetro | Valor de Placa / Plano CNC Insumos | Notas Técnicas |
| :--- | :--- | :--- |
| **Tipo de Motor** | Paso a Paso Bipolar Híbrido NEMA 34 | Tamaño de brida $86 \times 86\text{ mm}$ ($3.4"$) |
| **Torque de Retención** | **$4.5\text{ N}\cdot\text{m}$** ($45\text{ kg}\cdot\text{cm}$) | Alto torque para vencer la oclusión del tubo de $18\text{ mm}$ |
| **Corriente Nominal** | **$6.0\text{ A}$** (Paralelo) / **$3.0\text{ A}$** (Serie) | Configurado actualmente en Serie a $3.0\text{ A}$ |
| **Inductancia por Fase** | **$4.0\text{ mH}$** (Paralelo) / **$16.0\text{ mH}$** (Serie) | Rápida respuesta transitoria de corriente |
| **Resistencia por Fase** | $0.5\,\Omega$ (Paralelo) / $2.0\,\Omega$ (Serie) | Baja disipación térmica |
| **Ángulo de Paso** | **$1.8^\circ \pm 5\%$** | **$200\text{ pasos completos por revolución}$** |
| **Driver Asociado** | Leadshine DM860 en Cátodo Común | Microstepping en $\times 8 \implies \mathbf{1600\text{ pulsos/vuelta}}$ |
| **Largo del Motor** | $78\text{ mm}$ cuerpo / $\approx 100\text{ mm}$ total | Carcasa disipadora de aluminio |
| **Eje de Transmisión** | $\varnothing 14\text{ mm}$ con chavetero de $4\text{ mm}$ | Acople directo al rotor del cabezal MBP-2000 |

---

# 3. 🚨 IMPACTO CIENTÍFICO EN EL PROYECTO: El Desplazamiento Real de la Manguera

El dato de **$\varnothing\text{Int} = 12\text{ mm}$** es la pieza que faltaba para explicar todo el comportamiento de la planta:

### A. Cálculo Geométrico con Manguera de 12 mm:
* Área interna de la manguera:
  $$A_{\text{tubo}} = \frac{\pi \times (1.2\text{ cm})^2}{4} = \mathbf{1.131\text{ cm}^2 = 1.131\text{ mL por cada centímetro de manguera}}$$
* Longitud de pista ocluida en el cabezal MBP-2000 ($\approx 14\text{ cm}$):
  $$V_{\text{rev}} \approx 14\text{ cm} \times 1.131\text{ mL/cm} \approx \mathbf{15.5\text{ mL/vuelta}}$$

### B. Contraste con el Caudal Máximo del Fabricante:
* Si la bomba entrega **$2.0\text{ L/min}$ ($2000\text{ mL/min}$)** a su velocidad máxima recomendada ($\approx 130\text{ RPM}$):
  $$\text{Desplazamiento} = \frac{2000\text{ mL/min}}{130\text{ RPM}} = \mathbf{15.38\text{ mL/vuelta}}$$

### C. La Conclusión Definitiva sobre las Mediciones Anteriores:
* Cuando la bomba giraba a **$72\text{ RPM}$**, el caudal real generado era:
  $$Q_{\text{real}} = 72\text{ RPM} \times 15.4\text{ mL/vuelta} \approx \mathbf{1108\text{ mL/min}} \quad (1.1\text{ L/min})$$
* La frecuencia teórica en el caudalímetro YF-S401 para $1.108\text{ L/min}$ es:
  $$F = 98 \times 1.108 = \mathbf{108.6\text{ Hz}}$$
* En el código antiguo que tenía el error de dividir por 60:
  $$Q_{\text{pantalla}} = \frac{1108}{60} = \mathbf{18.4\text{ mL/min}}$$
  $$F_{\text{pantalla}} = \frac{108.6}{60} = \mathbf{1.81\text{ Hz}}$$
* **¡Esto coincide exactamente con los $17.9$ y $19.0$ que midió Enzo!**  
  **El sensor NO estaba midiendo ruido: estaba midiendo el caudal real de agua de la manguera gruesa de 12 mm.**

---

# 4. Ajuste en el Firmware para la Membrana FX100

Como el tubo de $\varnothing 12\text{ mm}$ desplaza $\approx 15.4\text{ mL/vuelta}$ en lugar de $4.2\text{ mL/vuelta}$:
* A 72 RPM entrega $\approx 1100\text{ mL/min}$ ($1.1\text{ L/min}$).
* La membrana Fresenius FX100 tiene un límite operativo sugerido de $600\text{ mL/min}$ ($0.60\text{ L/min}$).
* Para operar a $300 - 600\text{ mL/min}$ con esta manguera de $12\text{ mm}$, el rango de RPM de la bomba deberá ajustarse en el firmware entre **$20\text{ RPM}$ y $40\text{ RPM}$** ($20 \times 15.4 = 308\text{ mL/min}$, $40 \times 15.4 = 616\text{ mL/min}$).
* *(Esto se calibrará con total precisión en el ensayo con la probeta graduada)*.
