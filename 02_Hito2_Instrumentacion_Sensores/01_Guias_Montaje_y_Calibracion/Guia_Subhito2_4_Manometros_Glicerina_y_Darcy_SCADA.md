# 📊 GUÍA SUBHITO 2.4: Instrumentación Manométrica Primaria (P1, P2) e Integración Híbrida en SCADA para Modelado Darcy en Tiempo Real
## Medición Mecánica Amortiguada en Glicerina, Inmunidad a EMI y Telemetría de Resistencias en Membrana FX100

**Proyecto:** Planta Piloto de Ultrafiltración por Flujo Cruzado & Reactor de Coagulación-Sedimentación  
**Tesistas de Grado:** Antonella Guitián & Owen Cañizares  
**Codirector:** Ing. Enzo *(Tesis Doctoral en Membranas)*  
**Ubicación:** Laboratorio de Ingeniería, Universidad Nacional de Salta (UNSa)  
**Fecha:** Octubre 2026  

---

## 🎯 1. Objetivo del Subhito 2.4
1. Instalar físicamente los **manómetros de tubo Bourdon con baño de glicerina** en la línea de alimentación ($P_1$) y retentado ($P_2$) del cartucho de ultrafiltración Fresenius FX100.
2. Integrar la lectura manométrica en la interfaz web del SCADA (`http://192.168.4.1`) mediante carga asistida en vivo junto con la temperatura ($T\ [^\circ\text{C}]$).
3. Habilitar el módulo embebido `darcy.h` del ESP32 para calcular en tiempo real:
   * Presión Transmembrana: $\text{TMP} = \frac{P_1 + P_2}{2} - P_3\ [\text{bar}]$.
   * Flujo normalizado a 20°C: $J_{20} = J \cdot \text{TCF}(T)\ [\text{LMH}]$.
   * Resistencia hidráulica total: $R_{\text{total}}\ [\text{m}^{-1}]$.
   * Resistencia adicional de torta: $R_{\text{torta}} = R_{\text{total}} - R_m\ [\text{m}^{-1}]$.
4. Registrar automáticamente todas las variables calculadas en el archivo `.CSV` del datalogger cada 10 segundos.

---

## 🧠 2. Fundamento y Decisión Estratégica de Ingeniería
En el diseño preliminar se evaluó instalar transductores electrónicos piezoresistivos comerciales con interfaz analógica al conversor ADS1115 (16 bits). Sin embargo, la auditoría técnica y la realidad experimental determinaron descartar los transductores genéricos en favor de **manómetros mecánicos con baño de glicerina**:

| Criterio Técnico | Transductor Genérico (0 - 10 bar) + ADS1115 | Manómetro con Baño de Glicerina (0 - 1.0 / 1.6 bar) |
| :--- | :--- | :--- |
| **Rango Operativo Real** | Trabaja entre $1\%$ y $4\%$ de Fondo de Escala ($0.10 - 0.40\text{ bar}$). | Trabaja entre $15\%$ y $50\%$ de escala, máxima sensibilidad de aguja. |
| **Incertidumbre Absoluta** | $\pm 1.5\%\ \text{FS} \rightarrow \pm 0.15\text{ bar}$ ($\pm 150\text{ mbar}$). | $\pm 1.6\%\ \text{FS} \rightarrow \pm 0.016\text{ bar}$ ($\pm 16\text{ mbar}$). |
| **Error Relativo a TMP=0.25 bar** | **$\mathbf{\pm 60\%}$ de error relativo** (Inaceptable para tesis). | **$< \mathbf{6.4\%}$ de error relativo** (Aceptable para modelado Darcy). |
| **Ruido EMI (DM860)** | Muy alto (pulsos inductivos de 3A a 3200 microsteps inducen espigas en cables y bus I2C). | **Inmunidad absoluta (100% mecánico, cero electrónica vulnerable)**. |
| **Pulsaciones Peristálticas** | Saturación de ADC por oscilación a 1.25–5 Hz. | **Amortiguación viscosa por glicerina; lectura visual estable del valor medio**. |
| **Deriva de Cero (*Zero Drift*)** | Severa por calentamiento resistivo y temperatura ambiente en Salta. | Cero deriva mecánica; calibrado con tornillo de puesta a cero. |

---

## 🚰 3. Esquema de Montaje Hidráulico en la Planta

```text
       Línea de Impulsión (Bomba NEMA 34)
                     │
                     ├────────► [ Válvula de Alivio Mecánica (0.70 bar) ] ──► Retorno Tanque
                     │
                     ▼
              [ MANÓMETRO P1 ] (Baño de glicerina, 0-1.0 bar, Rosca G 1/4")
                     │
                     ▼
         ┌───────────────────────┐
         │ CABEZAL ENTRADA FX100 │
         │   (Flujo Luminal)     │
         │                       │
         │   Fibras Capilares    │──────► [ LÍNEA DE PERMEADO ] ──► [ Caudalímetro Permeado ]
         │  Polisulfona/Helixone │                                  (Descarga libre P3 ≈ 0.0 bar)
         │                       │
         │ CABEZAL SALIDA FX100  │
         └───────────────────────┘
                     │
                     ▼
              [ MANÓMETRO P2 ] (Baño de glicerina, 0-1.0 bar, Rosca G 1/4")
                     │
                     ▼
           [ Válvula de Aguja ] ──► [ Caudalímetro Retentado ] ──► Recirculación Tanque
```

### Reglas de Montaje para Owen:
1. **Conexiones Roscadas:** Utilizar cinta de teflón de alta densidad en todas las roscas macho $G 1/4"$ de los manómetros y acoples en T de polipropileno/bronce. Ajustar con llave fija sin sobretorque para no fisurar los cabezales.
2. **Cota Geodésica:** Instalar ambos manómetros ($P_1$ y $P_2$) a la **misma altura geométrica** respecto a la base de la planta. Esto anula el error estático de columna hidrostática ($\Delta h = 0$).
3. **Pérdida de Carga Luminal:**
   $$\Delta P_{\text{lumen}} = P_1 - P_2$$
   Si $\Delta P_{\text{lumen}} > 0.35\text{ bar}$ a caudales nominales ($70\text{ RPM}$), indica atascamiento físico de partículas en el cabezal de entrada de las fibras capilares.

---

## 💻 4. Integración Asistida en el Servidor Web SCADA

### Flujo de Operación para Antonella y Owen:
1. Conectar la notebook o smartphone al Wi-Fi de la planta (`Bomba_Peristaltica_UF`, clave: `plantapiloto2`).
2. Abrir el navegador en `http://192.168.4.1` o `http://bomba.local`.
3. En el panel **"📊 Presión Manométrica Primaria & Darcy"**:
   - Ingresar $P_1$ observado en el manómetro de entrada (ej: `0.30` bar).
   - Ingresar $P_2$ observado en el manómetro de retentado (ej: `0.20` bar).
   - Ingresar la temperatura del agua $T$ (ej: `21.5` °C).
   - Presionar **"Fijar Presiones"**.
4. El ESP32 recalcula automáticamente:
   $$\text{TMP} = \frac{0.30 + 0.20}{2} - 0.00 = 0.25\text{ bar}$$
   $$\text{TCF} = \frac{\mu(21.5^\circ\text{C})}{\mu(20^\circ\text{C})} \approx 0.965$$
   $$J_{20} = J \cdot \text{TCF} \quad [\text{LMH}]$$
   $$R_{\text{total}} = \frac{\text{TMP}}{\mu(T) \cdot J_{\text{SI}}} \quad [\text{m}^{-1}] \quad , \quad R_{\text{torta}} = R_{\text{total}} - R_m$$
5. El datalogger guarda estos valores en cada registro temporal de 10 segundos.

---

## 📋 5. Checklist de Validación del Subhito 2.4
- [ ] Manómetro $P_1$ instalado y verificado sin fugas hidráulicas a $0.50\text{ bar}$.
- [ ] Manómetro $P_2$ instalado en la salida de retentado previo a la aguja.
- [ ] Válvula de alivio de seguridad calibrada a $0.70\text{ bar}$ operativa.
- [ ] Interfaz web SCADA recibiendo y actualizando $P_1$, $P_2$, $T$ y TMP.
- [ ] Módulo `darcy.h` reportando $J_{20}$, TCF y resistencias hidráulicas coherentes ($R_{\text{total}} \sim 10^{13}\text{ m}^{-1}$).
- [ ] Archivo `.CSV` descargado desde el SCADA conteniendo las columnas: `P1_bar`, `P2_bar`, `TMP_bar`, `Temp_C`, `TCF_Vogel`, `J20_LMH`, `R_total_m1`, `R_torta_m1`.
