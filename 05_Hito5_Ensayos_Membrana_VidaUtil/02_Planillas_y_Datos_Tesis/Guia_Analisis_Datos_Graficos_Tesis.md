# 📊 GUÍA METODOLÓGICA: ANÁLISIS DE DATOS, GRÁFICOS CIENTÍFICOS Y SCRIPT DE TESIS
## Tratamiento de Telemetría y Modelado Matemático de la Membrana FX100 • Hito 5
**Tesistas**: Antonella Guitián & Owen Cañizares  
**Codirector**: Ing. Enzo (Investigación Doctoral en Membranas)

---

## 🎯 Entregable Concreto de esta Guía
* **Paquete completo de análisis de datos y gráficos para la Tesis de Grado**: Procesamiento sistemático de las series temporales del archivo `plantilla_datos_ensayo_tesis.csv`, generación de las curvas científicas de transporte Darcy ($J$ vs $\text{TMP}$ y $R_{\text{total}}$ vs tiempo) y script en Python automatizado para exportación de figuras vectoriales en alta resolución ($300\text{ DPI}$).

---

## 📋 Lista de Verificación (Checklist de Avance)
- [ ] Archivo `plantilla_datos_ensayo_tesis.csv` poblado con lecturas experimentales del banco de ensayo.
- [ ] Conversión de unidades verificada: caudales en $\text{L/min}$, presiones en $\text{atm}$ y flujo en $\text{LMH}$.
- [ ] Gráfico 1 generado: Decaimiento de flujo $J(t)$ y normalizado $J_{20}(t)$ en función del tiempo.
- [ ] Gráfico 2 generado: Curva de Presión Transmembrana ($\text{TMP}$) con cota máxima de seguridad en $0.50\text{ atm}$.
- [ ] Gráfico 3 generado: Resistencia hidráulica total $R_{\text{total}}$ en función del volumen filtrado acumulado.
- [ ] Gráfico 4 generado: Diagrama de barras de recuperación de flujo ($FRR$) post-retrolavado.
- [ ] Script en Python ejecutado sin errores generando las imágenes `.png` en la carpeta de la tesis.
- [ ] Tablas resumen integradas en el borrador del documento final de tesis.

---

## 📑 1. Estructura y Variables del Archivo CSV

El archivo [`plantilla_datos_ensayo_tesis.csv`](./plantilla_datos_ensayo_tesis.csv) contiene las 15 columnas fundamentales para el análisis de transporte:

```
┌───────────────────────────┬──────────────┬────────────────────────────────────────────────────────┐
│ Columna                   │ Unidad       │ Definición Física / Significado                        │
├───────────────────────────┼──────────────┼────────────────────────────────────────────────────────┤
│ Tiempo_seg                │ s            │ Tiempo cronometrado desde el inicio del ensayo.         │
│ Etapa_Proceso             │ Texto        │ Estado operativo (FILTRACION, RETROLAVADO, etc.).      │
│ RPM_Bomba                 │ RPM          │ Velocidad angular del motor NEMA 34.                   │
│ P1_Entrada_atm            │ atm          │ Presión hidrostática en el cabezal de alimentación.    │
│ P2_Retentado_atm          │ atm          │ Presión en la salida de concentrado capilar.           │
│ P3_Permeado_atm           │ atm          │ Presión en el puerto lateral de extracción de filtrado. │
│ TMP_atm                   │ atm          │ Presión Transmembrana: ((P1 + P2) / 2) - P3.          │
│ Caudal_Perm_Lmin          │ L/min        │ Flujo volumétrico medido por caudalímetro YF-S401 (Qp).│
│ Caudal_Ret_Lmin           │ L/min        │ Flujo volumétrico en la línea de retentado (Qc).       │
│ Flux_J_LMH                │ L/(m²·h)     │ Flujo volumétrico por unidad de área (Qp * 60 / 2.2).  │
│ Flux_Normalizado_J20_LMH  │ L/(m²·h)     │ Flujo normalizado a 20°C mediante factor TCF.          │
│ Temp_Agua_C               │ °C           │ Temperatura medida por la sonda OneWire DS18B20.       │
│ TDS_Permeado_ppm          │ ppm (mg/L)   │ Sólidos Totales Disueltos del agua producida.          │
│ Turbidez_Salida_NTU       │ NTU          │ Turbidez nefelométrica del permeado.                   │
│ Resistencia_Total_m1      │ m⁻¹          │ Resistencia hidráulica de Darcy (R_total).             │
└───────────────────────────┴──────────────┴────────────────────────────────────────────────────────┘
```

---

## 📈 2. Gráficas Científicas Requeridas en la Tesis

Para el capítulo de **Resultados y Discusión** de la tesis de grado, Antonella y Owen deben presentar:

1. **Curva de Declinación de Flujo ($J$ vs $t$)**: Muestra cómo la acumulación superficial de torta reduce la permeabilidad y cómo el retrolavado restaura el flujo inicial.
2. **Evolución de la Presión Transmembrana ($\text{TMP}$ vs $t$)**: Evidencia la necesidad de operar con el enclavamiento de seguridad para nunca sobrepasar los $0.50\text{ atm}$.
3. **Resistencia Hidráulica de Darcy ($R_{\text{total}}$ vs $V_{\text{acum}}$)**: Permite determinar si el mecanismo de ensuciamiento predominante es colmatación completa de poros, colmatación estándar o filtración por torta.

---

## 🐍 3. Script Automatizado en Python para Generación de Figuras de Tesis

Los tesistas pueden ejecutar este script en Python (compatible con Jupyter Notebook, Google Colab o terminal local) para procesar el archivo CSV y exportar las figuras listas para el informe:

```python
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Configuración de estilo visual profesional
plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')
plt.rcParams['font.family'] = 'DejaVu Sans'
plt.rcParams['font.size'] = 11

# 1. Cargar archivo CSV experimental
csv_file = "plantilla_datos_ensayo_tesis.csv"
df = pd.read_csv(csv_file)

# -------------------------------------------------------------
# FIGURA 1: Flujo Normalizado J20 y TMP vs Tiempo
# -------------------------------------------------------------
fig, ax1 = plt.subplots(figsize=(10, 5), dpi=300)

color_flux = '#0284c7'
ax1.set_xlabel('Tiempo de Ensayo (s)', fontweight='bold')
ax1.set_ylabel('Flujo Normalizado $J_{20}$ (L/m²·h)', color=color_flux, fontweight='bold')
line1 = ax1.plot(df['Tiempo_seg'], df['Flux_Normalizado_J20_LMH'], color=color_flux, marker='o', linewidth=2, label='Flujo $J_{20}$')
ax1.tick_params(axis='y', labelcolor=color_flux)

ax2 = ax1.twinx()
color_tmp = '#dc2626'
ax2.set_ylabel('Presión Transmembrana TMP (atm)', color=color_tmp, fontweight='bold')
line2 = ax2.plot(df['Tiempo_seg'], df['TMP_atm'], color=color_tmp, marker='s', linestyle='--', linewidth=2, label='TMP')
ax2.axhline(0.50, color='#991b1b', linestyle=':', linewidth=1.5, label='Límite Seguro (0.50 atm)')
ax2.tick_params(axis='y', labelcolor=color_tmp)

plt.title('Cinética de Ultrafiltración FX100: Declinación de Flujo y Evolución de TMP', fontweight='bold', pad=14)
fig.tight_layout()
plt.savefig('Figura1_Flux_y_TMP_vs_Tiempo.png')
print("[OK] Figura 1 guardada como 'Figura1_Flux_y_TMP_vs_Tiempo.png'")

# -------------------------------------------------------------
# FIGURA 2: Resistencia Hidráulica de Darcy vs Tiempo
# -------------------------------------------------------------
plt.figure(figsize=(9, 4.5), dpi=300)
plt.plot(df['Tiempo_seg'], df['Resistencia_Total_m1'] / 1e11, color='#7c3aed', marker='^', linewidth=2)
plt.xlabel('Tiempo de Ensayo (s)', fontweight='bold')
plt.ylabel('Resistencia Total $R_{total}$ ($\\times 10^{11}$ m⁻¹)', fontweight='bold')
plt.title('Evolución de la Resistencia Hidráulica de Darcy durante el Ciclo', fontweight='bold', pad=14)
plt.tight_layout()
plt.savefig('Figura2_Resistencia_Darcy.png')
print("[OK] Figura 2 guardada como 'Figura2_Resistencia_Darcy.png'")
```

---

## 📊 4. Procedimiento Rápido en Microsoft Excel

Si los tesistas prefieren procesar los datos en Excel:
1. Abrir Excel y presionar **Datos ➔ Desde el texto/CSV**.
2. Seleccionar `plantilla_datos_ensayo_tesis.csv` con delimitador **Coma (,)**.
3. Seleccionar las columnas `Tiempo_seg`, `Flux_Normalizado_J20_LMH` y `TMP_atm`.
4. Insertar gráfico de **Dispersión con líneas suavizadas**.
5. Hacer clic derecho sobre la serie de $\text{TMP}$ y seleccionar **Dar formato a serie de datos ➔ Eje secundario**.
6. Añadir títulos de ejes con sus respectivas unidades físicas.
