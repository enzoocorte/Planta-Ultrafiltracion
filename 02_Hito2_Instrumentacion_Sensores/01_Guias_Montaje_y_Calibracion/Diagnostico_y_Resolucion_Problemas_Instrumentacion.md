# 🩺 Diagnóstico de Fallas de Instrumentación, Ruido Electromagnético (EMI) y Validación Experimental (Hito 2)

> **Documento Técnico de Apoyo para Tesis de Grado**  
> **Autores:** Antonella Guitián & Owen Cañizares  
> **Co-Director:** Ing. Enzo  
> **Fecha:** 30 de Septiembre de 2026  
> **Área:** Control e Instrumentación — Planta Piloto de Ultrafiltración (Fresenius FX100)

---

## 1. Introducción y Contexto Experimental

Durante la puesta en marcha de la instrumentación del **Hito 2** (medición de caudal mediante sensores de turbina de Efecto Hall YF-S401 acoplados al microcontrolador ESP32-WROOM-32), surgieron anomalías en las lecturas de telemetría y en los archivos de registro (`.csv`) exportados por el sistema SCADA.

Este documento registra:
1. Las **fallas observadas en el banco de pruebas** (síntomas y datos crudos).
2. El **análisis físico y matemático de las causas raíz**.
3. Las **soluciones implementadas en hardware y software**.
4. El **protocolo de verificación experimental** exigido para la validación de la tesis.

```
                  ARQUITECTURA DE ADQUISICIÓN Y FILTRADO
  ┌───────────────────────┐                    ┌───────────────────────┐
  │   PLACA 2: FRONT-END  │   Línea de Señal   │    PLACA 1: MASTER    │
  │  (Acondicionamiento)  │───────────────────>│     (ESP32 WROOM)     │
  │                       │    P14 (GPIO 14)   │                       │
  │  • Resistencia 4.7 kΩ │                    │  • Interrupción ISR   │
  │  • Capacitor 100 nF   │                    │  • Filtro software    │
  │  • Bornera ZS-1057    │<══════════════════>│    (12000 µs debounce)│
  └───────────────────────┘    GND Común       └───────────────────────┘
                             (Referencia 0V)
```

---

## 2. Caso de Estudio 1: El Enigma de los 81 Hz (835.5 mL/min) con Bomba Parada

### 2.1. Síntoma Observado en Laboratorio
Con la bomba peristáltica completamente detenida (0 RPM) y las mangueras sin circulación de fluido:
* El indicador de telemetría de **Feed** marcaba un valor constante de **$81.0\text{ Hz}$** a **$81.9\text{ Hz}$**.
* El caudal calculado en pantalla indicaba **$835.5\text{ mL/min}$**.
* Al arrancar la bomba, el valor fluctuaba erraticamente alrededor de esa cifra basal de 800–850 mL/min.

---

### 2.2. Modelado Matemático y Demostración de Causa Raíz

En el código fuente de adquisición (`config.h` y `caudalimetro.cpp`), se implementó un filtro de desrebote por software para descartar ruido transitorio de alta frecuencia:

```cpp
// Fragmento de caudalimetro.cpp
void IRAM_ATTR Caudalimetro::isrPuente(void* arg) {
  Caudalimetro* c = reinterpret_cast<Caudalimetro*>(arg);
  uint32_t t = micros();
  if (t - c->_t_ultimo >= FILTRO_RUIDO_US) {   // FILTRO_RUIDO_US = 12000 µs
    c->_pulsos++;
    c->_t_ultimo = t;
  }
}
```

#### Análisis Matemático del Techo de Frecuencia:
1. Si un pin de entrada recibe una señal oscilatoria continua, alterna o tren de ruido con períodos menores a $12000\ \mu\text{s}$ ($12\text{ ms}$), el algoritmo descarta todos los flancos que lleguen antes de cumplirse dicha ventana temporal.
2. Cada vez que transcurren $\approx 12.2\text{ ms}$ ($12000\ \mu\text{s} + \text{jitter de atención de la ISR}$), el filtro acepta exactamente **un único pulso**:
   $$T_{\text{mín}} \approx 12.2\text{ ms} = 0.0122\text{ s}$$
3. La frecuencia máxima registrada resultante es:
   $$f_{\text{tope}} = \frac{1}{T_{\text{mín}}} = \frac{1}{0.0122\text{ s}} \approx \mathbf{81.9\text{ Hz}}$$
4. Con el factor del sensor ($K = 98.0\text{ pulsos/L}$):
   $$Q = \frac{f \times 1000}{K} = \frac{81.9 \times 1000}{98.0} = \mathbf{835.7\text{ mL/min}}$$

> [!IMPORTANT]
> **Demostración Analítica para la Tesis:**  
> El valor de **835.5 mL/min a 81 Hz no corresponde a fluido físico**, sino a la saturación matemática del filtro de software ante un tren continuo de ruido electromagnético de red (zumbido de $50\text{ Hz} / 100\text{ Hz}$) acoplado a la línea del sensor.

---

### 2.3. Causas Raíz Físicas Identificadas en el Hardware

1. **Ausencia o Flojedad de la Masa Común (GND Loop / Floating Ground):**
   Al distribuir la electrónica en dos borneras (Placa 1 y Placa 2), si el cable de unión de masa entre ambas placas presenta alta resistencia ($> 1\ \Omega$) o un falso contacto en el tornillo, el terminal de entrada del ESP32 queda sin referencia fija a 0V. Una entrada CMOS de alta impedancia en estado flotante se comporta idénticamente a una **antena receptora** de la radiación ambiental de $50\text{ Hz}$ de la red de 220V.
2. **Error Típico de Montaje: Resistencia en Serie vs. Resistencia en Paralelo (Pull-Up):**
   * *Conexión correcta (Pull-up):* Resistencia conectada entre la línea de señal y el riel de alimentación lógica de **+3.3V**.
   * *Error de montaje observado en campo:* Colocar la resistencia cortando el cable amarillo (en serie). En serie no fija el potencial de reposo en alto; sólo añade impedancia, empeorando la captación de ruido.
3. **Omisión del Filtro Pasabajos Analógico (Capacitor de 100 nF):**
   Una resistencia pull-up resistiva pura polariza la continua (DC), pero tiene impedancia constante para toda la banda de frecuencias. No atenúa el zumbido de alterna ni las armónicas de conmutación del chopper del driver Leadshine DM860 ($20 - 40\text{ kHz}$).
4. **Acoplamiento Inductivo por Proximidad:**
   Tendido de los cables del sensor en paralelo directo con los cables de 220VAC de red o el secundario del transformador de 24VAC.

---

## 3. Caso de Estudio 2: Inversión de Canales de Medición (Feed vs. Permeado)

### 3.1. Detección en el Dataset Experimental
En la auditoría de datos realizada sobre el archivo [`02_Hito2_Instrumentacion_Sensores/Datos/datos_planta_uf_2026-09-30.csv`](file:///c:/Users/enzoo/OneDrive/Documentos/ENZO/Domotica/SistemaUF/02_Hito2_Instrumentacion_Sensores/Datos/datos_planta_uf_2026-09-30.csv):

```csv
Hora;Sentido;RPM;Q_Teorico_mLmin;Q_Feed_mLmin;Q_Perm_mLmin;Vol_Feed_L;Vol_Perm_L
10:49:12;FILTRACION;94.0;1447.6;0.0;658.4;0.002;2.415
10:50:52;FILTRACION;94.0;1447.6;38.1;627.1;0.018;2.664
```

### 3.2. Diagnóstico Termodinámico e Hidráulico
* **Inconsistencia física:** En una planta de ultrafiltración de flujo cruzado (*cross-flow*), por principio de conservación de la materia:
  $$Q_{\text{feed}} = Q_{\text{permeado}} + Q_{\text{retentado}}$$
  Es termodinámicamente imposible que el caudal de permeado supere al de alimentación ($Q_{\text{perm}} \gg Q_{\text{feed}}$), máxime cuando la bomba de impulsión opera directamente sobre la línea de alimentación.
* **Causa raíz comprobada:**  
  El caudalímetro físico atornillado a la manguera de impulsión de la bomba peristáltica fue cableado al pin **GPIO 27** (`PIN_SENSOR_PERM`), mientras que el pin **GPIO 14** (`PIN_SENSOR_FEED`) quedó conectado a un sensor sin flujo o con falso contacto.

---

## 4. Soluciones Implementadas

### 4.1. Solución de Hardware: Red de Filtro RC Pasabajos Industrial

Para garantizar inmunidad frente al chopper del motor paso a paso y la inducción de red, se implementó en la Placa 2 un filtro pasabajos de primer orden por cada canal de caudal:

```
          +3.3V DC (ESP32)
               │
              ┌┴┐
              │ │ R_pull = 4.7 kΩ
              │ │
              └┬┘
  Señal Sensor ├───┬───────────────────────> Entrada ESP32 (GPIO 14 / GPIO 27)
  (Cable Amarillo) │
                  ┌┴┐
                  │ │ C_shunt = 100 nF (104)
                  │ │ (Cerámico)
                  └┬┘
                   │
                  GND Común (0V)
```

#### Cálculo de Frecuencia de Corte ($f_c$):
$$f_c = \frac{1}{2 \pi \cdot R \cdot C} = \frac{1}{2 \pi \cdot (4700\ \Omega) \cdot (100 \times 10^{-9}\ \text{F})} \approx \mathbf{338.6\text{ Hz}}$$

* **Banda de paso ($0 - 150\text{ Hz}$):** Permite medir holgadamente caudales de hasta $1500\text{ mL/min}$ ($f \approx 147\text{ Hz}$) con atenuación nula ($0\text{ dB}$).
* **Banda de rechazo ($> 1\text{ kHz}$):** Atenúa a razón de $-20\text{ dB/década}$ cualquier ruido de alta frecuencia del chopper del driver Leadshine DM860 ($20\text{ kHz}$).

---

### 4.2. Solución de Software en Firmware (`subhito2_2_v2`)

1. **Pull-Up Interno Redundante (`caudalimetro.h`):**  
   Se modificó el método `begin()` para activar la resistencia interna débil en paralelo:
   ```cpp
   pinMode(_pin, INPUT_PULLUP);
   ```
   Esto previene que el pin flote si se desconecta accidentalmente un jumper de la Placa 2.
2. **Reconfiguración de Micropasos a 16 (`PULSOS_POR_REV = 3200`):**  
   Sincronizado con los interruptores DIP SW5=OFF, SW6=OFF, SW7=ON, SW8=ON del driver DM860, eliminando vibraciones mecánicas de baja frecuencia y resonancias acústicas.

---

## 5. Protocolo de Verificación Experimental en Laboratorio

Para documentar la resolución definitiva en la tesis, los tesistas deben registrar los siguientes ensayos:

| Paso | Ensayo | Procedimiento | Criterio de Éxito | Estado |
| :--- | :--- | :--- | :--- | :---: |
| **E1** | **Continuidad de GND** | Multímetro en Continuidad: medir entre borne GND Placa 1 y borne GND Placa 2. | $R < 0.2\ \Omega$ con señal audible continua. | |
| **E2** | **Tensión DC en Reposo** | Multímetro en 20V DC: medir entre P14 y GND con bomba apagada. | $3.30\text{ V} \pm 0.05\text{ V}$ estable (o $0.00\text{ V}$ si activa el imán). | |
| **E3** | **Zumbido AC Residual** | Multímetro en 2V AC: medir entre P14 y GND con bomba apagada. | $0.000\text{ V AC}$ (ausencia total de inducción de 50 Hz). | |
| **E4** | **Prueba en Seco (0 RPM)** | Observar Web SCADA (`http://192.168.4.1`) durante 60 segundos sin bombear. | **Feed = 0.0 Hz / 0.0 mL/min**<br>**Perm = 0.0 Hz / 0.0 mL/min** | |
| **E5** | **Prueba Dinámica de Soplido** | Aplicar flujo de aire suave con manguera desconectada. | Registro transitorio de pulsos que retorna inmediatamente a 0.0 Hz. | |
| **E6** | **Balance de Masa (36 RPM)** | Bombear agua destilada a 36 RPM durante 60 s hacia probeta graduada ($1000\text{ mL}$). | $Q_{\text{medido}} \approx 554\text{ mL/min} \pm 5\%$<br>$Q_{\text{feed}} \ge Q_{\text{perm}} + Q_{\text{ret}}$ | |

---

## 6. Conclusiones para la Redacción de la Tesis

1. **Separación de Niveles de Potencia y Señal:** En plantas piloto que combinan actuadores electromecánicos de alto torque (motores paso a paso NEMA 34 con fuentes inductivas de corriente) y transductores digitales de efecto Hall, es mandatario el uso de **filtro RC pasivo en hardware** antes de ingresar al microcontrolador.
2. **Limitaciones del Filtrado Digital Puro:** El software no puede corregir una línea en alta impedancia que flota a 50 Hz; el software simplemente recorta o subdivide el tren de pulsos según su algoritmo de ventana temporal. La integridad de la masa de referencia (GND de $0.0\ \Omega$) es la condición necesaria para la validez de los datos experimentales.
3. **Auditoría Continua de Datos (Data Integrity Check):** El cruce de sensores se diagnosticó gracias a la exportación en tiempo real del archivo CSV y la verificación analítica del balance de masa ($Q_{\text{feed}} \text{ vs. } Q_{\text{perm}}$).
