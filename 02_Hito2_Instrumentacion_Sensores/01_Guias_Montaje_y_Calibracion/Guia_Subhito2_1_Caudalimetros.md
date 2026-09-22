# 🌊 SUBHITO 2.1: Calibración y Lectura de Caudalímetros de Microflujo (YF-S401)

Este documento detalla el procedimiento técnico, eléctrico e hidráulico para la puesta en marcha del **Subhito 2.1**, dedicado a medir el caudal de alimentación/retentado y permeado en la Planta Piloto de Ultrafiltración.

---

## 🎯 1. Objetivos del Subhito 2.1
1. Detectar e interpretar los pulsos generados por el sensor de efecto Hall de los caudalímetros **YF-S401**.
2. Cuantificar el caudal instantáneo en **mL/min** y **L/min**, y el volumen total acumulado en **Litros**.
3. Validar la medición contrastándola directamente con el desplazamiento volumétrico de la bomba peristáltica **MBP-2000** ($4.2\text{ mL/vuelta}$).

---

## 🔌 2. Diagrama de Conexionado Eléctrico

El caudalímetro YF-S401 posee 3 cables. Se conectan directamente a la bornera del shield ZS-1057 del ESP32:

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                           CONEXIONADO DEL CAUDALÍMETRO YF-S401                          │
├─────────────────────────┬───────────────────────────────┬───────────────────────────────┤
│ Cable del Sensor        │ Función                       │ Borne en Shield ESP32         │
├─────────────────────────┼───────────────────────────────┼───────────────────────────────┤
│ 🔴 ROJO                 │ Alimentación VCC (5V)         │ Borne [ 5V ] o [ VIN ]        │
│ ⚫ NEGRO                │ Masa de Referencia (GND)      │ Borne [ GND ]                 │
│ 🟡 AMARILLO (Sensor 1)  │ Señal Digital Pulsos Hall     │ Borne [ P14 ] (GPIO 14)       │
│ 🟡 AMARILLO (Sensor 2)  │ Señal Digital Pulsos Hall     │ Borne [ P27 ] (GPIO 27)       │
└─────────────────────────┴───────────────────────────────┴───────────────────────────────┘
```

> [!NOTE]
> **Resistencias Pull-Up:** El firmware activa internamente `INPUT_PULLUP` en los pines 14 y 27 del ESP32, garantizando pulsos limpios de 0V a 3.3V sin necesidad de agregar resistencias externas en el banco.

---

## ⚙️ 3. Principio Físico y Ecuaciones de Medición

El sensor YF-S401 aloja una turbina plástica interna con imanes permanentes en sus extremos. A medida que el líquido atraviesa la cámara calibrada, hace girar la turbina. Un chip de **Efecto Hall** detecta el paso de cada imán y conmuta una señal cuadrada a nivel lógico.

### Fórmulas Matemáticas:
* **Ecuación del Fabricante**:
  $$F (\text{Hz}) = 98 \times Q (\text{L/min})$$
* **Caudal Instantáneo en Litros por Minuto**:
  $$Q (\text{L/min}) = \frac{F (\text{Hz})}{98.0}$$
* **Caudal Instantáneo en Mililitros por Minuto**:
  $$Q (\text{mL/min}) = Q (\text{L/min}) \times 1000.0$$
* **Pulsos por Litro (Factor K)**:
  $$K = 98 \times 60 = 5880\text{ pulsos/Litro}$$
* **Volumen Total Acumulado ($V$)**:
  $$V (\text{Litros}) = \frac{\text{Pulsos Totales}}{5880.0}$$

---

## 💻 4. Firmware de Validación Dedicado

El código se encuentra en:
📁 **[`02_Firmware_Test_Sensores/subhito2_1_caudalimetros/subhito2_1_caudalimetros.ino`](../02_Firmware_Test_Sensores/subhito2_1_caudalimetros/subhito2_1_caudalimetros.ino)**

### Características Principales:
* Utiliza interrupciones de hardware (`attachInterrupt`) en memoria rápida `IRAM_ATTR` para no perder pulsos incluso a caudales máximos.
* Incluye un **menú interactivo por teclado** para accionar la bomba peristáltica MBP-2000 en distintos puntos de consigna y contrastar la lectura:

| Tecla | Acción en el Monitor Serie | Caudal Teórico Bomba |
|:---:|:---|:---:|
| `1` | Arrancar Bomba a **80 RPM** | $0.336\text{ L/min}$ ($336\text{ mL/min}$) |
| `2` | Arrancar Bomba a **100 RPM** | $0.420\text{ L/min}$ ($420\text{ mL/min}$) |
| `3` | Arrancar Bomba a **120 RPM** | $0.504\text{ L/min}$ ($504\text{ mL/min}$) |
| `4` | Arrancar Bomba a **140 RPM** | $0.588\text{ L/min}$ ($588\text{ mL/min}$) |
| `0` | **Detener Bomba (0 RPM)** | $0.000\text{ L/min}$ |
| `r` | **Resetear contadores** de volumen a 0.000 L | - |
| `?` | Mostrar menú de ayuda | - |

---

## 🧪 5. Procedimiento de Prueba y Calibración en el Banco

1. **Cargar el Firmware**: Abrir el sketch en Arduino IDE, seleccionar placa *ESP32 Dev Module*, puerto correspondiente (ej. `COM5`) y subir.
2. **Abrir Monitor Serie**: Configurar la velocidad a **`115200 baudios`**.
3. **Verificación en Reposo**: Con la bomba apagada, el monitor serie debe indicar `0.0 Hz -> 0.0 mL/min`.
4. **Verificación en Marcha**:
   * Escribir `1` y presionar Enter. La bomba arrancará suavemente a 80 RPM.
   * Observar cómo el caudalímetro empieza a reportar frecuencia ($\approx 33\text{ Hz}$) y caudal ($\approx 336\text{ mL/min}$).
   * El monitor mostrará la discrepancia porcentual $\Delta\%$ entre el caudal teórico de la bomba y el medido por el YF-S401.
5. **Calibración Gravimétrica (Contraste Fino)**:
   * Colocar una probeta graduada o vaso de precipitado sobre una balanza a la salida.
   * Escribir `r` para reiniciar el contador a 0.000 L.
   * Purgar un volumen conocido (ej. 200 mL).
   * Si el volumen real pesado en la balanza difiere ligeramente del reportado en pantalla, ajustar el `FACTOR_K_YFS401 = 98.0f` proporcionalmente.

---

## ⚠️ 6. Puntos Críticos y Resolución de Problemas

1. **Flecha de Sentido de Flujo:**
   * En el cuerpo plástico del YF-S401 hay una **flecha estampada**. Si el agua circula en sentido contrario, la turbina girará con fricción irregular o no contará correctamente.
2. **Purgado de Burbujas de Aire:**
   * Las burbujas atrapadas en la cavidad de la turbina amortiguan el giro del rotor. Antes de tomar datos oficiales, hacer circular agua a 120 RPM durante 30 segundos para purgar todo el aire de la manguera.
3. **Posición de Montaje:**
   * Se recomienda montar el caudalímetro en posición horizontal para evitar empuje axial constante sobre los cojinetes plásticos de la turbina.
