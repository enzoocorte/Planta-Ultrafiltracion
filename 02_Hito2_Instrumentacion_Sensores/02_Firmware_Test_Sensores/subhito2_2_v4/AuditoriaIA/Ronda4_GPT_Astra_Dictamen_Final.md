# RONDA 4 — DICTAMEN DE AUDITORÍA TÉCNICA
## GPT ASTRA · Ingeniería de Sistemas Embebidos + Procesos de Separación por Membranas

**Proyecto:** Planta Piloto de Ultrafiltración FX100 + Reactor de Coagulación-Sedimentación
**Institución:** Universidad Nacional de Salta — Facultad de Ingeniería
**Equipo:** Antonella Guitián · Owen Cañizares · Ing. Enzo (Codirector)
**Fecha:** Octubre 2026
**Objeto auditado:** `subhito2_2_v4/EN_USO_firmware_planta/` (commit `9e05eaa`, rama `main`)

---

## 0. ALCANCE Y MÉTODO DE VERIFICACIÓN

Este dictamen **no** se emite sobre el listado pegado en el prompt: se emite sobre los archivos
que efectivamente están en el repositorio. Cada afirmación factual de este documento proviene de
una lectura, una búsqueda o una ejecución realizada durante esta auditoría.

### 0.1 Qué se verificó ejecutando código

No hay toolchain de ESP32 disponible en el entorno de auditoría (ver §6). Se construyó en cambio
un **arnés de verificación en host** que compila y **ejecuta el `caudalimetro.cpp` original del
firmware v4, sin modificarlo**, sobre un reloj virtual de 32 bits que reproduce el truncamiento
real de `micros()`:

```
subhito2_2_v4/verificacion_host/
├── shim/Arduino.h     # solo símbolos del API; no reimplementa lógica
├── shim2/Arduino.h    # shim equivalente para Bomba.cpp
├── harness.cpp        # 11 bancos de prueba sobre Caudalimetro
└── rampa.cpp          # simulación de Bomba::tick() con dt = 50 ms
```

Compilación y ejecución:

```bash
cd subhito2_2_v4/verificacion_host
g++ -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable \
    -I shim -I ../EN_USO_firmware_planta \
    harness.cpp ../EN_USO_firmware_planta/caudalimetro.cpp -o /tmp/harness && /tmp/harness
g++ -std=c++17 -O2 -w -I shim2 -I ../EN_USO_firmware_planta \
    rampa.cpp ../EN_USO_firmware_planta/Bomba.cpp -o /tmp/rampa && /tmp/rampa
```

El código bajo auditoría que esas corridas **ejecutan de verdad** es
`Caudalimetro::actualizar()` y `Caudalimetro::isrPuente()` (69 431 entradas a sección crítica
contabilizadas) y `Bomba::tick()`.

### 0.2 Identidad del código auditado

`caudalimetro.cpp` del repositorio (md5 `bd924057684b30a83679f217265227b0`, 118 líneas) es
**idéntico** al listado de la sección 3.3 del brief. Lo mismo para `caudalimetro.h`, `config.h`,
`Bomba.cpp/.h` y `darcy.h`. No hay deriva entre el brief y el disco en esos cinco archivos.

Donde **sí** hay deriva es entre la prosa del brief y el código — ver hallazgo **B-2**.

### 0.3 Tabla maestra de hallazgos

| ID | Severidad | Hallazgo | Estado |
|----|-----------|----------|--------|
| **B-1** | BLOQUEANTE | La planilla Excel trae datos pre-cargados circulares → emite CV = 0 % y R² = 1.0000 | Verificado numéricamente |
| **B-2** | BLOQUEANTE | No existe **ningún** código de presión, TMP ni interlock en el firmware v4 | Verificado por grep |
| **B-3** | BLOQUEANTE | El YF‑S401 alimentado a 5 V entrega 4.7 V; el ESP32 tolera 3.6 V máx. | Verificado con datasheets |
| **B-4** | BLOQUEANTE | La válvula de alivio a 0.70 bar **no** acota la TMP (límite 0.50 bar) | Deducción + spec |
| A-1 | ALTO | La rampa tarda 7.0 s (20 RPM) a 29.9 s (100 RPM); contamina la corrida si el cronómetro arranca con START | Simulado |
| A-2 | ALTO | El frenado desde 100 RPM tarda 2.22 s, no "< 1.5 s" como documenta `config.h` | Simulado |
| A-3 | ALTO | La auto-calibración de permeado usa una referencia inventada (`rpm × 2.0`) y la graba en Flash | Verificado |
| A-4 | ALTO | El datalogger registra 2 muestras espurias tras cada STOP, dentro del ensayo recién cerrado | Verificado |
| A-5 | ALTO | El filtro de plausibilidad (6000 mL/min) es inalcanzable en alimentación: techo real 4312 | Ejecutado (T9) |
| A-6 | ALTO | `K_ALIM = 154.62` es +58 % vs el nominal 98 Hz/(L/min): fuera de toda tolerancia de fabricación | Verificado |
| A-7 | ALTO | La EMA tarda 14 s en asentar a 0.1 %; las corridas de 30 s no llegan | Ejecutado (T10) |
| A-8 | ALTO | El punto de 20 RPM (204 mL/min) está bajo el caudal mínimo del sensor (0.3 L/min) | Datasheet |
| A-9 | ALTO | `darcy.h` calcula `Rm` con μ(20 °C) pero `K_UF` es dato a 37 °C → Rm subestimado 31 % | Calculado |
| M-1 | MEDIO | La "cota física" no decae asintóticamente: escalón 680 → 379 mL/min en 1 s | Ejecutado (T3) |
| M-2 | MEDIO | `modeloDarcy`/`resultadoDarcy` declarados y nunca usados; 8 constantes de `config.h` sin referencia | Verificado |
| M-3 | MEDIO | Límite de detección: 2.16 mL/min (alim) / 6.06 mL/min (perm), con parpadeo a cero | Ejecutado (T6) |
| M-4 | MEDIO | `/clear_csv` sin confirmación; todos los endpoints sin autenticación | Verificado |
| M-5 | MEDIO | La planilla histórica tiene el volumen integrado +30 % inconsistente con su propio Q | Calculado |
| M-6 | MEDIO | El blanking es refractario desde el último pulso **aceptado**: una ráfaga a 400 µs deja pasar 2 de 5 | Ejecutado (T7) |
| M-7 | MEDIO | `sendContent(buf, n)` no verifica truncamiento (hoy sobran 165 bytes) | Medido |

**Cerrados como NO-ISSUE tras verificación** (es decir: el brief tenía razón y yo lo comprobé):
rollover (§1.3), dimensiones de K y del volumen (§1.4), no-atenuación de la cota en régimen (§1.2),
rechazo del rizo peristáltico (§1.6), resolución del acumulador `float _vol` (§1.7), buffer de
`handleStatus` (§1.8).

---

## EJE 1 — METROLOGÍA DE CAUDAL Y ROBUSTEZ DE SOFTWARE

### 1.1 Período recíproco híbrido: correcto y exacto

Salida real del arnés (T1/T2), con la EMA asentada (40 ventanas):

```
   Q_true     f_true    n/ventana   f_med     Q_med      err      cota recorta?
  [mL/min]     [Hz]                 [Hz]     [mL/min]    [%]
      6.47     1.000           1     1.000       6.47   +0.000   no
     18.20     2.814           3     2.814      18.20   -0.000   no
    250.00    38.655          39    38.655     250.00   +0.000   no
    680.00   105.142         105   105.142     680.00   +0.000   no
   1360.00   210.283         210   210.283    1360.00   +0.000   no
   4200.00   649.404         650   649.404    4200.00   +0.000   no
```

**Veredicto:** el estimador es exacto al 0.000 % en todo el rango, incluido el caso `n = 1`.
La afirmación del comentario — "elimina por completo el error de discretización ±1 pulso" — es
**correcta**. Con el método de conteo clásico a 1 s, a 6.47 mL/min el error sería del ±100 %;
con el período recíproco es nulo.

**Condición de carrera remanente: no la hay.** Las cuatro variables compartidas
(`_pulsos`, `_periodo_us`, `_t_primero`, `_t_ultimo`) se capturan y se resetean dentro de una
única `portENTER_CRITICAL`, y `micros()` se llama **después** de `portEXIT_CRITICAL`. La
consistencia entre `n` y `t_ultimo` está garantizada porque ambas se leen en la misma sección.
Los handlers web corren en `loop()` vía `server.handleClient()`, es decir en la misma tarea que
`actualizar()`: `setK()` y `resetVolumen()` no introducen carrera entre tareas.

Un detalle fino que **sí** está bien resuelto: si el bucle se bloquea (exportación CSV sobre
Wi‑Fi), la ventana se estira, pero el estimador no depende de la longitud de la ventana —
depende de `t_ult − t_prim`. La frecuencia sigue siendo correcta, y la ISR no pierde pulsos.

### 1.2 La cota física superior: correcta en régimen, pero no hace lo que dice el comentario en transición

**En régimen permanente, la cota nunca recorta** (columna "cota recorta? = no" en los seis
puntos, incluido `n = 1`). La demostración es sencilla y el arnés la confirma: si el siguiente
pulso llega en `t_ult + T`, entonces en el instante de muestreo `tSinFlanco < T`, por lo que
`f_max = 1e6/tSinFlanco > 1e6/T = f`. Nunca hay recorte. ✔

**Pero la afirmación "decaimiento suave y asintótico hacia cero … sin escalones" es falsa.**
Salida real del arnés (T3), tras cortar un tren estable de 680 mL/min:

```
   ventana   t_sin_flanco[s]    f[Hz]     Q[mL/min]
         1                1     0.99339     378.838
         2                2     0.49834     228.592
         3                3     0.00000       0.000
```

El primer segundo tras el STOP lee **378.8 mL/min: el 56 % del régimen**. No es un decaimiento
asintótico desde 680; es un escalón del 44 % seguido de dos puntos más y un corte a cero.

**Causa:** la cota usa el tiempo desde el **último pulso**, no el período medido. Con ventana de
1 s, `tSinFlanco` puede ser hasta 1 s aunque el tren fuera de 105 Hz, así que la cota impone
`f ≤ 1 Hz` de inmediato. Lo que suaviza la caída no es la cota: es la EMA.

**Consecuencia operativa (ver A-4):** esos 378.8 y 228.6 mL/min **se registran en el datalogger**.

**Corrección sugerida** (no aplicada — el código está bajo certificación):

```cpp
// En actualizar(), tras el cálculo de _f, reemplazar la cota cruda por:
//   usar el último período medido como piso de la cota
uint32_t tRef = (_periodo_us > 0) ? _periodo_us : tSinFlanco;
if (tSinFlanco > tRef) {
  float f_max_posible = 1000000.0f / (float)tSinFlanco;
  if (_f > f_max_posible) _f = f_max_posible;
}
```

### 1.3 Rollover de `micros()`: inmunidad CONFIRMADA empíricamente

Se arrancó el reloj virtual **3 s antes** de `2^32` y se cruzó el desbordamiento en plena marcha:

```
   ventana      micros()      f_med[Hz]   err[%]
         1   4292967296     105.140   +0.000
         2   4293967296     105.140   +0.000
         3            0     105.140   +0.000   <-- cruce 2^32 en esta ventana
         4      1000000     105.140   -0.000
```

**Peor desvío tras el cruce: −0.000 %.** La resta modular `uint32_t dt = t - c->_t_ultimo` es
exacta para intervalos menores a 2³² µs = 71.58 min, y el blanking (`dt >= 1500`) y el descarte
de período (`dt < 5000000`) garantizan que ningún intervalo legítimo se acerque a ese límite.
**La afirmación del brief es correcta y queda certificada.**

Único caso teórico no cubierto: si la bomba está detenida más de 71.58 min, `tAhora - t_ult`
vuelve a envolver y la cota deja de acotar. Es inocuo porque `_f` ya fue forzado a 0 por la regla
de los 3 s. No requiere acción.

### 1.4 Unidad de K y volumen integrado: matemáticamente exactas

T5, emitiendo exactamente `K × 60` pulsos:

```
   [ OK ] pulsos por Litro = K * 60     emitidos=9277   teoricos K*60*V=9277.2   dif=-0.2
   [ OK ] V[L] = n / (K*60)             V=0.999979 L (objetivo 1.000000, err -0.0021 %)
   [ OK ] Q[mL/min] = f * 1000 / K      Q = 680.000 mL/min
   [ OK ] Coherencia K_PERMEADO         K_perm=55.00: f=5.50 Hz -> Q=100.000 mL/min
```

**La cadena dimensional es impecable:**

$$F\,[\mathrm{Hz}] = K\,[\mathrm{Hz/(L/min)}]\cdot Q\,[\mathrm{L/min}]
\quad\Longrightarrow\quad
V\,[\mathrm{L}] = \frac{n}{K\cdot 60}$$

El error residual de −0.0021 % es el fencepost de contar `n` pulsos que abarcan `n−1` períodos:
±1 pulso sobre 9277, es decir ±0.011 % por litro. Despreciable frente a cualquier patrón de banco.

**Aclaración metrológica formal para la memoria de tesis** (respuesta directa al Eje 1.4):

> La constante del transductor se expresa como **sensibilidad** `K = F/Q`, de dimensión
> `[Hz·min·L⁻¹]`, escrita por convención `Hz/(L/min)`. **No** es un "K‑factor" volumétrico.
> El K‑factor volumétrico es su recíproco temporal: `K_v = 60·K [pulsos/L]`.
> Para `K_alim = 154.62 Hz/(L/min)` → `K_v = 9277.2 pul/L` → 1 pulso = 0.1078 mL.
> Para `K_perm = 55.00 Hz/(L/min)` → `K_v = 3300.0 pul/L` → 1 pulso = 0.3030 mL.
>
> El fabricante expresa el mismo dato como `F = 98·Q`, que es la misma forma funcional. La
> confusión habitual `K = 98 pul/L` es dimensionalmente incorrecta y debe evitarse en la memoria.

### 1.5 Otros comportamientos verificados

**T6 — Límite de detección.** La regla de cero duro a los 3 s impone un piso:

| Sensor | K | Caudal mínimo detectable | Comportamiento bajo el piso |
|---|---|---|---|
| Alimentación | 154.62 | **2.16 mL/min** | parpadea entre 0 y un valor no nulo |
| Permeado | 55.00 | **6.06 mL/min** | parpadea entre 0 y un valor no nulo |

Medido: con un período de 3.6 s, **5 de 15 ventanas leen exactamente 0**. A J = 20 LMH el
permeado es 733 mL/min, así que el piso no compromete los ensayos — pero debe documentarse como
límite del instrumento secundario.

**T7 / M-6 — El blanking es un período refractario, no un filtro de ráfaga.** Se midió desde el
último pulso **aceptado**, así que un tren sostenido justo por encima de 1500 µs pasa entero.
Una ráfaga de 5 flancos a 400 µs deja pasar 2.

**T9 / A-5 — El filtro de plausibilidad es inerte en alimentación:**

```
   EMI a   600 Hz -> Q=3699 mL/min ; Q_MAX_FISICO=6000 -> ACEPTADA como caudal real
   EMI a   660 Hz -> Q=4069 mL/min ; Q_MAX_FISICO=6000 -> ACEPTADA como caudal real
   EMI a   666 Hz -> Q=4106 mL/min ; Q_MAX_FISICO=6000 -> ACEPTADA como caudal real
   Techo del blanking: f_max = 1e6/1500 = 666.7 Hz -> Q_alim_max = 4312 mL/min
```

Como `4312 < 6000`, el `if (q > Q_MAX_FISICO_MLMIN)` **nunca puede disparar** en el canal de
alimentación. El comentario "corte de picos transitorios por perturbación EMI" describe una
protección que en ese canal no existe. **Corrección:** bajar `Q_MAX_FISICO_MLMIN` a ~2000 mL/min
(1.5× el máximo operativo de 1360) o, mejor, hacerla dependiente de K:
`Q_MAX_FISICO = 1.5f * RPM_MAX * ML_POR_VUELTA`.

**T10 / A-7 — Asentamiento de la EMA:**

```
   ventanas para err<1%: 10 s   err<0.1%: 14 s   err<0.01%: 19 s
   tau = 1.96 s  (el comentario dice "~2 segundos": correcto)
```

**T11 — Rechazo del rizo peristáltico.** Con una modulación sintética de ±30 % a 2.5 Hz
(3 rodillos a 50 RPM), la salida queda en `687.6 ± 6.14 mL/min` → **±0.89 %**. Atenuación
mayor a 30×. El residuo es un batido entre la ventana de 1 s y la frecuencia de rizo, y depende
del RPM; es aceptable.

**T8 — El acumulador `float _vol` no es un problema:**

```
   K= 154.62  incremento=1.0779e-04 L/pulso -> _vol se congela en 2048.0 L
   K=  55.00  incremento=3.0303e-04 L/pulso -> _vol se congela en 8192.0 L
```

**M-7 — `handleStatus`:** medido el peor caso realista (mensaje de auto‑cal con el tick UTF‑8 y
todos los campos largos): `snprintf` retorna **602** sobre `sizeof(buf) = 768`. **No hay
desbordamiento hoy.** Sigue siendo frágil porque `n` no se acota antes de `sendContent(buf, n)`;
si el JSON creciera, se leería fuera del buffer. Una línea lo blinda:

```cpp
if (n > (int)sizeof(buf) - 1) n = sizeof(buf) - 1;
```

---

## EJE 2 — SENSOR DE PRESIÓN DE 30 PSI + ADS1115

### 2.1 ¿Es adecuado el rango de 30 PSI? **Sí, y es la única opción correcta de las tres.**

| Rango | Fondo de escala | Sensibilidad | A 0.50 bar | Veredicto |
|---|---|---|---|---|
| 0–1.0 bar | 1.0 bar | 4.0 V/bar | 50 % de escala | **Rechazado**: el alivio mecánico abre a 0.70 bar y un transitorio de oclusión lo lleva a fondo de escala. Se pierde la medición justo cuando más se la necesita. |
| **0–30 PSI (2.07 bar)** | 2.07 bar | **1.934 V/bar** | 24 % de escala | **Correcto**: cubre el alivio (0.70 bar = 34 %) con margen, y opera en el tercio inferior, el más lineal. |
| 0–100 PSI (6.9 bar) | 6.9 bar | 0.58 V/bar | 7 % de escala | Rechazado: la señal útil se comprime 3.3× y el offset de 0.5 V pasa a dominar. |

Verificación de la sensibilidad declarada: `4.0 V / 2.0684 bar = 1.9338 V/bar`. El brief dice
≈1.93 V/bar. **Correcto.**

A 0.6 bar la salida es `0.5 + 0.6 × 1.9338 = 1.660 V`. El brief dice 1.66 V. **Correcto.**

### 2.2 Resolución efectiva y relación señal/ruido

Cálculo para PGA = ±6.144 V (el único admisible: con ±4.096 V se saturaría a 4.5 V):

| Data rate | Bits efectivos | Ruido | En presión |
|---|---|---|---|
| 8 SPS | 15.5 | 44.2 µV rms | **0.023 mbar** |
| 16 SPS | 15.0 | 62.5 µV rms | 0.032 mbar |
| 64 SPS | 14.0 | 125.0 µV rms | 0.065 mbar |
| 128 SPS | 13.5 | 176.8 µV rms | 0.091 mbar |
| 860 SPS | 12.5 | 353.6 µV rms | 0.183 mbar |

LSB = 6.144/32768 = 187.5 µV = **0.097 mbar**.

Sobre el rango operativo de 0–500 mbar hay ~1 670 pasos útiles. **El ADS1115 sobra por dos
órdenes de magnitud.** Recomendación: **16 SPS**, que da 0.032 mbar de ruido y 62 ms de
conversión — compatible con el lazo de 50 ms si se lee un canal por ciclo.

### 2.3 ⚠ El error dominante NO es el ADC: es la ratiometría de la fuente

Este es el punto metrológico más importante del Eje 2. El transductor es **ratiométrico**: su
salida es proporcional a su alimentación. El ADS1115, en cambio, tiene referencia interna. Por lo
tanto **toda deriva de la fuente de 5 V se traduce íntegra en error de presión**:

| Deriva de la fuente 5 V | Error a 0.50 bar | Comparación con el ruido del ADC |
|---|---|---|
| 0.5 % | 2.5 mbar | 78× el ruido |
| 1.0 % | 5.0 mbar | 156× el ruido |
| 2.0 % | 10.0 mbar | 312× el ruido |
| 5.0 % | 25.0 mbar | 780× el ruido |

Comprar un ADC de 16 bits para medir una señal ratiométrica alimentada por un regulador de placa
sin caracterizar es **gastar resolución donde no está el cuello de botella**.

Tres mitigaciones, en orden de costo:

1. **Medir la alimentación real.** Quedan 2 canales libres en el ADS1115: usar A2 para medir el
   propio riel de 5 V (con divisor) y corregir en software:
   `P = (V_senal − 0.5·V_cc/5) / (1.9338 · V_cc/5)`. Esto cancela la deriva ratiométrica por
   completo y cuesta un divisor de dos resistencias. **Es la recomendación principal.**
2. **Re-cero periódico.** Con la bomba parada, P = 0 y la salida debe ser exactamente
   `0.5·V_cc/5`. Restar ese offset en cada arranque elimina el error de offset y deriva lenta.
3. **Contraste con los manómetros de glicerina** (ya previstos en el Hito 2.3). Deben tratarse
   como patrón de verificación, no como redundancia decorativa.

### 2.4 Diagnóstico de cable cortado — oportunidad gratuita

El offset de 0.5 V es un regalo: **un cable de señal cortado lee 0 V, que equivale a −0.26 bar**.
Es un valor físicamente imposible. Implementar:

```cpp
if (v_senal < 0.35f) falloSensor = true;   // cable cortado o sensor sin alimentar
if (v_senal > 4.65f) falloSensor = true;   // cortocircuito a VCC o sobre-rango
```

Dos comparaciones y se tiene diagnóstico de lazo completo, equivalente al que ya existe en los
caudalímetros (`sinSenal()`).

### 2.5 Precauciones eléctricas mínimas (respuesta al Eje 2.3)

**Alimentación del ADS1115 — punto crítico.** Debe alimentarse a **5 V**, no a 3.3 V:

- El absoluto máximo de entrada del ADS1115 es `VDD + 0.3 V`. Con `VDD = 3.3 V`, los 4.5 V del
  transductor **destruyen el canal**.
- Con `VDD = 3.3 V` ni siquiera se puede usar el rango ±6.144 V del PGA.

Consecuencia: el bus I²C queda pull‑up‑eado a 5 V por las resistencias onboard del módulo, y el
ESP32 tolera 3.6 V en sus GPIO (Espressif FAQ: *"The voltage tolerance of GPIO is 3.6 V. If the
voltage exceeds 3.6 V, please add a voltage divider to protect GPIO pins from damage"*).

**Solución:** alimentar el ADS1115 a 5 V y **sustituir los pull‑ups del módulo por 4.7 kΩ a
3.3 V** (o intercalar un 74HCT125 / BSS138). SDA/SCL a 3.3 V con el ADS1115 a 5 V funciona sin
problema: el umbral `V_IH` del ADS1115 es 0.7·VDD = 3.5 V… **ojo, esto queda justo**. La opción
limpia es el level shifter; la opción pragmática es alimentar el ADS1115 a 5 V y usar pull‑ups de
2.2 kΩ a 3.3 V, verificando con osciloscopio que el flanco llegue a 3.5 V.

**Cableado del transductor (3 hilos: 5 V, GND, Señal):**

1. **Cable apantallado o par trenzado**, apantalla conectada a tierra **en un solo extremo** (el
   del ADS1115). Doble puesta a tierra = lazo de masa = zumbido de 50 Hz.
2. **Ruteo físico separado de los cables del motor.** Los conductores del DM860 llevan 4.0 A
   conmutados; no compartir canal, no correr paralelos, cruzar en ángulo recto si es inevitable.
3. **Filtro en el conector del transductor:** 100 nF cerámico + 10 µF tantalio entre señal y
   masa, más 100 Ω en serie con la señal. Esto y el ADS1115 a 16 SPS dan un rechazo excelente.
4. **Masa en estrella.** La masa analógica del transductor no debe compartir camino de retorno
   con la corriente del motor. Un solo punto de unión en la bornera de alimentación.
5. **Protección de entrada:** diodo TVS bidireccional de 5.1 V o dos diodos Schottky a los rieles
   en el pin de entrada del ADS1115.
6. **Descartar la primera conversión tras conmutar el multiplexor.** El ADS1115 tiene un
   settling time del mux; a 16 SPS se descarta la primera muestra y se usa la segunda.
7. **Dirección I²C:** ADDR a GND → `0x48`. Los dos transductores van en A0 (P1) y A1 (P2); A2 para
   el riel de 5 V (§2.3) y A3 reservado para la sonda de temperatura o TDS del Hito 3.

---

## EJE 3 — PROTOCOLO EXPERIMENTAL EN BANCO

### 3.1 El protocolo de probeta es correcto en su diseño, pero tiene tres fugas metrológicas

La estructura (8 niveles de RPM, 1 min hasta 70 RPM, 0.5 min a 80/90 RPM, regresión Q vs RPM con
R², y F vs Q con R²) es **sólida y apropiada**. La planilla calcula lo que debe calcular:
`SLOPE`, `INTERCEPT`, `RSQ`, `STDEV.S`, `CV%`. Bien.

**Fuga 1 — El patrón volumétrico es el eslabón débil, y no el cronómetro:**

| Punto | Volumen | ±5 mL de lectura | ±10 mL de lectura |
|---|---|---|---|
| 20 RPM / 60 s | 272 mL | ±1.8 % | **±3.7 %** |
| 50 RPM / 60 s | 680 mL | ±0.7 % | ±1.5 % |
| 70 RPM / 60 s | 952 mL | ±0.5 % | ±1.1 % |
| 80 RPM / 30 s | 544 mL | ±0.9 % | ±1.8 % |
| 90 RPM / 30 s | 612 mL | ±0.8 % | ±1.6 % |

Un cronómetro manual aporta ±0.3 s sobre 60 s = **±0.5 %**. Es decir: **la probeta domina la
incertidumbre por un factor de 3 a 7**, y el punto de 20 RPM queda en ±3.7 %.

**Recomendación fuerte:** ya tienen una balanza de 0.1 g en el laboratorio (es el patrón primario
del permeado según la resolución del bloqueante 3 de la Ronda 3). Sobre 680 g, 0.1 g es
**±0.015 %**: 300 veces mejor que la probeta. Un balde de 2 L sobre la balanza sustituye a la
probeta sin costo adicional y sin cambio de protocolo — sólo cambia la columna "Volumen [mL]"
por "Masa [g]", con ρ = 0.9982 g/mL a 20 °C.

Si se conserva la probeta: **3 repeticiones por nivel**, y reportar la media y la desviación, no
un único valor.

**Fuga 2 — El arranque de la rampa contamina la corrida (A-1).** Simulando `Bomba::tick()` real:

```
  Consigna   t hasta enRegimenEstable()   volumen bombeado durante la rampa
      20 RPM          7.00 s                    17.4 mL
      50 RPM         15.55 s                    91.3 mL
      70 RPM         21.30 s                   173.6 mL
      80 RPM         24.15 s                   224.2 mL
      90 RPM         27.00 s                   281.1 mL
     100 RPM         29.85 s                   344.6 mL
```

**Si el cronómetro arranca al presionar START, la corrida de 30 s a 90 RPM transcurre íntegra
dentro de la rampa.** Es la causa más probable de un ensayo 2 invalidado el lunes.

El firmware ya expone `en_regimen` en `/status` y en la interfaz. **El protocolo debe decir
explícitamente: iniciar el cronómetro sólo cuando el indicador de régimen estable se encienda.**

Nota adicional: `enRegimenEstable()` exige `|actual − objetivo| < 0.3 RPM`. Aun así queda un
asentamiento pendiente de la EMA del caudalímetro: **14 s adicionales** para llegar a 0.1 %
(T10). En una corrida de 60 s eso es tolerable; **en una de 30 s no**. Sugerencia: en 80 y
90 RPM, colectar 30 s pero **descartar los primeros 10 s** (colectar 40 s y tomar el tramo final),
o directamente extender a 60 s usando un recipiente mayor.

**Fuga 3 — El punto de 20 RPM está fuera de especificación del sensor (A-8).** El YF‑S401 tiene
rango declarado **0.3–6 L/min**. A 20 RPM el caudal es 0.204 L/min: **32 % por debajo del mínimo
de especificación**. El rotor puede no arrancar de forma fiable o deslizar.

Sugerencia: conservar el punto pero **marcarlo como fuera de rango** en la planilla y excluirlo
del cálculo de `CV%` y de la regresión, o sustituirlo por 25 RPM (0.34 L/min, apenas dentro).

### 3.2 Sobre `K_ALIM = 154.62` (A-6): la desviación respecto del fabricante debe explicarse antes de grabarse en Flash

Cuatro fuentes independientes del datasheet del YF‑S401 coinciden:

> Flow pulse characteristics: **F = (98 × Q) ± 2 %**, Q en L/min
> Flow range: **0.3–6 L/min** · Accuracy: ±2 % (algunos vendedores: ±10 %)
> Output pulse high level: **> DC 4.7 V** (con alimentación de 5 V)

`154.62 / 98 = 1.578`. **Una desviación de +58 % no es compatible con ninguna tolerancia de
fabricación de ese sensor.** No digo que el valor esté mal — digo que hoy no está justificado,
y que el lunes tienen la oportunidad de zanjarlo.

Revisé el archivo histórico `CALIBRACION_CAUDALIMETROS_PROBETA_50RPM_72RPM.xlsx`. La hoja
"Ensayo 7 Minutos (50 RPM)" contiene datos reales, y efectivamente muestra ~105 Hz a 50 RPM
(valores: 97, 112, 120, 119, 108, 110, 114, 106, 107, 104, 114, 99, 105, 105, 105, 106, 109,
98, 100, 107, 103, 112, 104, 106, 101, 101, 99 Hz). Eso **sí** da K ≈ 154.6 con un caudal de
680 mL/min. Pero ese mismo archivo tiene un problema serio (**M-5**):

```
  Caudal derivado del VOLUMEN integrado :   869.2 mL/min
  Caudal reportado como INSTANTANEO     :   667.0 mL/min
  -> discrepancia = +30.3 %
  Ventana t=110->120 con RPM=0: el volumen igualmente crece 0.1086 L
```

El volumen integrado de ese registro **no es coherente con su propio caudal instantáneo**, y
acumula con la bomba detenida. El firmware que produjo esos datos tenía un defecto en el canal de
volumen. (El v4 **no** lo tiene: T5 prueba que `V = n/(K·60)` es exacto.)

**Conclusión:** el archivo histórico no es evidencia suficiente para fijar K. Tres hipótesis
quedan abiertas y el Ensayo 3 debe discriminarlas:

1. La unidad es realmente de 9277 pul/L (variante de rotor distinta a la nominal). → La regresión
   F vs Q dará pendiente ≈ 154 con R² > 0.99 y la probeta confirmará 680 mL/min a 50 RPM.
2. El caudal de referencia estaba mal medido. → La probeta dará un valor distinto de 680 mL/min.
3. Hay multiplicación de pulsos por EMI. → El `R²` será alto pero la dispersión punto a punto
   será grande y asimétrica.

**Acción:** en el Ensayo 3 registrar **probeta y frecuencia simultáneamente**, y contrastar el
caudal de probeta contra `RPM × 13.6`. Si el Ensayo 2 y el Ensayo 3 discrepan en el caudal, el
problema está en la hidráulica (aire, deslizamiento), no en el sensor.

### 3.3 Postergación del Ensayo 4: **respaldada sin reservas**

Calibrar el caudalímetro de permeado sin medición de TMP significa variar una variable que no se
observa. Con `K_perm = 55 Hz/(L/min)` el sensor tiene un piso de **6.06 mL/min** (§1.5) y un
rango declarado que arranca en 0.3 L/min: a TMP bajas el permeado puede caer fuera de rango.
El patrón gravimétrico con balanza de 0.1 g es la decisión correcta y ya está formalizado como
patrón primario en la Ronda 3.

**Una condición adicional:** al postergar el Ensayo 4, el valor `K_PERMEADO = 55.00` queda
**sin respaldo experimental**. Debe tratarse como valor nominal y el canal de permeado debe
marcarse como **no metrológico** en el SCADA hasta el Hito 2.3. Esto interactúa con A‑3.

---

## EJE 4 — PUNTOS CIEGOS Y VALIDACIÓN DE LA HOJA DE RUTA

### 4.1 B-1 · La planilla de calibración está pre-cargada con datos circulares ⚠ BLOQUEANTE

**Este es el hallazgo más peligroso para el lunes.** Las celdas amarillas de entrada de
`PLANILLA_CALIBRACION_ENSAYOS_2_Y_3.xlsx` **ya contienen valores**:

| Hoja | Celdas | Valores pre-cargados |
|---|---|---|
| Ensayo 2 | `D12:D19` | 272, 408, 544, 680, 816, 952, 544, 612 mL |
| Ensayo 3 | `C12:C19` | 42.06, 63.09, 84.11, 105.14, 126.17, 147.2, 168.23, 189.26 Hz |
| Ensayo 3 | `E12:E19` | 272, 408, 544, 680, 816, 952, 544, 612 mL |

Verifiqué la procedencia de esos números:

```
  13.6 * RPM * t  == volumen pre-cargado ? True
  154.62 * Q[L/min] == frecuencia pre-cargada ? True
```

**Son exactamente los valores generados por las dos constantes que el ensayo pretende determinar.**
El resultado que produce la planilla tal como está:

```
ENSAYO 2:  sigma = 0    CV% = 0    SLOPE (-> ML_POR_VUELTA) = 13.600000   R2 = 1.0000000000
ENSAYO 3:  sigma = 0.0065  CV% = 0.0042   SLOPE (-> K_ALIMENTACION) = 154.621849   R2 = 0.9999999965
```

Si Owen y Antonella abren la planilla, hacen sus mediciones en dos o tres filas y dejan el resto,
o si directamente la entregan como está, **el informe mostrará σ = 0, CV = 0 % y R² = 1.0000**:
la firma inconfundible de una calibración que no midió nada. Un jurado lo detecta en diez
segundos, y con razón.

**Acción obligatoria antes del lunes:**

1. Borrar `C12:C19` y `E12:E19` (Ensayo 3) y `C12:D19` (Ensayo 2).
2. Verificar que el bloque de resumen devuelva `#DIV/0!` o vacío — **no** 13.60 y 154.62.
3. Si se quiere conservar los valores como ejemplo, moverlos a una hoja aparte rotulada
   `EJEMPLO — NO USAR`, o ponerlos como comentario de celda.
4. Protección de hoja sobre todo lo que no sea amarillo (`Revisar → Proteger hoja`), para que un
   clic desafortunado no pise una fórmula. Hoy `protection.sheet = False`.

### 4.2 B-2 · El firmware v4 no contiene instrumentación de presión ni interlocks ⚠ BLOQUEANTE

La sección 2 del brief afirma que los bloqueantes de Claude Opus quedaron **"Resueltos"**, en
particular:

> *"el disparo de parada dura en software a 0.60 bar y el enclavamiento de TMP a 0.45 bar"*
> *"Enclavamiento a 1 Hz: Resuelto. Se evalúa en cada ciclo de 50 ms en la máquina de estados."*

Búsqueda exhaustiva sobre `config.h`, `caudalimetro.*`, `Bomba.*`, `darcy.h` y
`EN_USO_firmware_planta.ino`:

```
ads1115 | presion | pressure | tmp_bar | TMP | interlock | 0.60 | 0.45 | sobrepresion | wire | i2c | ds18b20
```

**Resultado: cero coincidencias fuera de comentarios y de `darcy.h`.** No hay `#include <Wire.h>`,
no hay GPIO 21/22, no hay lectura analógica de ningún tipo, no hay función de interlock, y el
bloque de 50 ms de `loop()` sólo contiene `bomba.tick(dt)`, el LED testigo y la detección de
flancos de ensayo.

Además, `TMP_MAX_SEGURA_BAR`, `RPM_ALARMA_MEMBRANA`, `Q_CLINICO_SANGRE_MAX`, `K_UF_NOMINAL`,
`DIAMETRO_CAPILAR_UM` y `ESPESOR_PARED_UM` aparecen **exactamente una vez cada una en todo el
firmware: en su propia declaración**. Ninguna se lee.

Y `modeloDarcy` / `resultadoDarcy` se declaran en las líneas 84‑85 del `.ino` y **no vuelven a
aparecer**: `calcular()`, `finalizarCalibracionRm()`, `acumularPuntoAguaLimpia()` y `setRm()` son
código muerto.

**No es un defecto del código — es una inconsistencia del dictamen.** Esos interlocks son
correctamente el Hito 3 de la hoja de ruta. Lo que está mal es declarar cerrado un bloqueante que
no está implementado. **Corrección de documentación obligatoria:** en el acta de la Ronda 3 los
bloqueantes 1 y 2 deben figurar como *mitigados por hardware (válvula de alivio) y pendientes de
implementación en software (Hito 3)*, no como "Resueltos".

### 4.3 B-3 · Nivel lógico del YF‑S401: riesgo de daño al ESP32 ⚠ BLOQUEANTE

`Guia_Subhito2_1_Caudalimetros.md`, línea 24 y 34:

> `🔴 ROJO │ Alimentación VCC (5V) │ Borne [ 5V ] o [ VIN ]`
> *"El firmware activa internamente `INPUT_PULLUP` en los pines 14 y 27 del ESP32, garantizando
> pulsos limpios de 0V a 3.3V sin necesidad de agregar resistencias externas en el banco."*

**Esto es incorrecto.** El datasheet del YF‑S401 declara salida push‑pull:
*"Output pulse high level > DC 4.7 V (input voltage DC 5 V)"*. Una salida push‑pull **impone** su
nivel: un pull‑up interno de ~45 kΩ a 3.3 V no puede sujetarla. El pin vería 4.7 V.

Espressif FAQ: *"The voltage tolerance of GPIO is 3.6 V. If the voltage exceeds 3.6 V, please add
a voltage divider to protect GPIO pins from damage."*

**4.7 V > 3.6 V.** Fuera del absoluto máximo. Puede funcionar durante semanas y degradar el pin,
o latchearse en la primera conexión en caliente.

La `Guia_Montaje_Placa_Filtrado_FrontEnd.md` sí agrega un pull‑up de 4.7 kΩ a 3.3 V (líneas 104‑105,
117‑118) y verifica 3.3 V en reposo en P14 y P27 (línea 186) — **pero un pull‑up no recorta un
nivel alto push‑pull**. La verificación de "3.3 V en reposo" se hace **sin flujo**, es decir con
la salida en estado alto o bajo según la unidad, y no prueba el nivel del flanco.

**Acción (5 minutos, antes de energizar):**

1. Multímetro entre la línea de señal del sensor y GND, **con el sensor alimentado a 5 V y sin
   conectar al ESP32**, midiendo durante flujo.
2. Si el nivel alto es > 3.6 V → **intercalar un divisor**. Con las piezas que ya tienen:
   `4.7 kΩ` en serie (la R del filtro RC existente) + `10 kΩ` de la unión a GND, con el
   `100 nF` en la unión. Resultado: `4.7 V × 10/14.7 = 3.20 V` ✔ seguro, y
   `f_c = 1/(2π · 3197 Ω · 100 nF) = 498 Hz` ✔ muy por encima de los 105 Hz de trabajo.
3. Corregir la línea 34 de la guía de montaje.

### 4.4 B-4 · La válvula de alivio a 0.70 bar no protege la membrana ⚠ BLOQUEANTE

Con permeado a presión atmosférica:

$$\mathrm{TMP} = \frac{P_1 + P_2}{2} - P_{perm} \approx \frac{P_1 + P_2}{2}$$

La válvula de alivio está **en la impulsión**: acota `P₁`, no `P₂`. Si el retentado se estrangula
con la válvula de aguja y el permeado se obstruye, ambas presiones convergen:

$$P_1 \to 0.70 \text{ bar (alivio)} \quad\Rightarrow\quad P_2 \to 0.70 \text{ bar} \quad\Rightarrow\quad \mathrm{TMP} = 0.70 \text{ bar}$$

**Contra un límite declarado de 0.50 bar.** El alivio mecánico protege la carcasa y la línea,
**no las fibras**. Hay una ventana de TMP entre 0.50 y 0.70 bar perfectamente alcanzable por vía
mecánica, sin que ningún software intervenga (B‑2).

**Acciones:**

1. **Retarar la válvula de alivio a 0.55 bar** (margen de 0.05 bar sobre el límite, por debajo
   del cual el transitorio de la rampa no debería llegar). Verificar con el manómetro de
   glicerina de P1.
2. **Regla operativa para el lunes:** con la membrana instalada, la válvula de aguja **nunca se
   cierra más de 2 vueltas desde abierta**, y nunca se ajusta con la bomba a más de 50 RPM.
3. **Preferible para el lunes:** hacer los Ensayos 2 y 3 **by‑passeando la carcasa de la
   membrana**. La calibración de la bomba y del caudalímetro de alimentación no necesita la
   membrana, y eliminarla elimina el riesgo. Esto además estabiliza la contrapresión y mejora la
   repetibilidad de `mL/rev`.
4. Confirmar contra el datasheet el límite real de la FX100. Los datos públicos de Fresenius
   confirman K_UF = 73 mL/(h·mmHg), 2.2 m² y pared/lumen 35/185 µm, y la literatura reporta
   *maximum blood flow* de 600 mL/min para la FX100 — **el brief extiende el rango a 1360 mL/min,
   2.3× el máximo declarado por el fabricante**. Es defendible como caracterización hidráulica
   con agua, pero debe declararse explícitamente como operación fuera de especificación en la
   memoria de tesis.

### 4.5 A-3 · La auto-calibración de permeado fabrica un dato y lo graba en Flash

`EN_USO_firmware_planta.ino`, dentro del bloque de auto‑calibración:

```cpp
if (fPromPerm > 0.2f) {
  float targetPerm = bomba.rpmActual() * 2.0f;
  float nuevoKp = (fPromPerm * 1000.0f) / targetPerm;
  sensorPermeado.setK(nuevoKp);
}
guardarParametrosNVS(...);
```

`rpm × 2.0 mL/min` es una **referencia inventada**. A 50 RPM asume 100 mL/min de permeado, es
decir una recuperación del 14.7 % caída del cielo. No hay ninguna medición que la respalde — de
hecho el Ensayo 4, que es el que la determinaría, está postergado (§3.3).

Y el umbral `fPromPerm > 0.2 Hz` equivale, con K = 55, a apenas 3.6 mL/min: se activa con
cualquier señal residual. El resultado se escribe en NVS con `guardarParametrosNVS`, es decir
**sobrevive al reinicio** y contamina silenciosamente todas las corridas posteriores.

**Acción:** inhabilitar el bloque de permeado en la auto‑calibración. El de alimentación es
legítimo (usa `qRefAlim = rpm × mL/rev`, trazable al Ensayo 2); el de permeado no lo es.

```cpp
// K_perm SOLO se calibra gravimétricamente (Ensayo 4, Hito 2.3). No auto-calibrar.
// if (fPromPerm > 0.2f) { ... }   <-- deshabilitado por auditoría Ronda 4
```

Y en la interfaz, el botón "Auto-Calibrar" debe indicar que sólo afecta a `K_ALIMENTACION`.

### 4.6 A-4 · Dos muestras espurias por cada parada, dentro del ensayo recién cerrado

El guard del datalogger en `loop()`:

```cpp
if (bomba.enMarcha() || sensorAlimentacion.caudal_mLmin() > 10.0f || sensorPermeado.caudal_mLmin() > 5.0f) {
  guardarMuestraDatalogger();
}
```

Combinando esto con la salida real de T3 (378.8 y 228.6 mL/min tras el STOP):

- STOP → `enMarcha()` pasa a `false`, pero `caudal_mLmin()` = 378.8 > 10 → **se registra**.
- 10 s después → 228.6 > 10 → **se registra otra vez**.
- 20 s después → 0 → no se registra.

**Dos filas por parada, con caudal espurio, etiquetadas con el `id_ensayo` de la corrida que
acaba de terminar.** Caen dentro del rango que el CSV exporta para ese ensayo y contaminan
directamente la regresión.

**Acción mínima:** agregar una condición de régimen al guard.

```cpp
if (bomba.enMarcha() && bomba.enRegimenEstable()) {
  guardarMuestraDatalogger();
}
```

Esto además descarta automáticamente los 7–30 s de rampa (A‑1), que también se están registrando
hoy con caudal y RPM transitorios. Es la corrección de mayor valor por línea de código de todo
este dictamen.

**Acción transitoria para el lunes** (si no se recompila): después de cada STOP, esperar 25 s y
**descartar las dos últimas filas** del CSV antes de llevarlas a la planilla.

### 4.7 A-2 · El frenado no cumple la especificación documentada

`config.h`: `FRENADO_PARADA_RPM_S = 45.0f; // 45.0 RPM/s frenado rápido al presionar STOP (< 1.5s)`
`Bomba.cpp`: `// PARADA RÁPIDA: Frenado ágil en menos de 1.5s`

Simulación real de `Bomba::tick()`: **2.25 s desde 100 RPM hasta 0**.

| RPM | t de frenado | Cumple < 1.5 s |
|---|---|---|
| 50 | 1.11 s | ✔ |
| 67.5 | 1.50 s | límite |
| 80 | 1.78 s | ✘ |
| 90 | 2.00 s | ✘ |
| 100 | 2.22 s | ✘ |

Al ampliar `RPM_MAX` a 100 se rompió la especificación de frenado sin actualizarla. O se sube
`FRENADO_PARADA_RPM_S` a 67 (100/1.5), o se corrige la documentación a "< 2.3 s en todo el
rango". Un NEMA 34 con 4.0 N·m de retención frena sin problema a 45 RPM/s; subir a 67 RPM/s es
seguro.

### 4.8 A-9 · `darcy.h`: la resistencia intrínseca está calculada con la viscosidad equivocada

`darcy.h` fija `_Rm = 1.44e13 m⁻¹` a partir de `K_UF = 73 mL/(h·mmHg)` y usa `MU20 = 1.002e-3 Pa·s`.

La conversión es correcta: verifiqué que `1/(μ₂₀ · Lp)` con `Lp = K_UF/A` da **1.4439e13**, que
coincide con el valor del código. El error está en **qué μ corresponde**.

Fresenius especifica los datos in vitro de la serie FX:

> *"The in vitro performance data were obtained with QD = 500 ml/min; QF = 0 ml/min; **T = 37 °C**
> (ISO 8637)"*

Es decir, **K_UF = 73 es un dato a 37 °C**. Como `Rm = 1/(μ · Lp)` es una propiedad geométrica de
la membrana, debe usarse el μ del fluido con que se midió K_UF:

```
  Rm con mu(37 C) = 0.6904 mPa.s  ->  2.0951e+13 1/m     <-- correcto
  Rm con mu(20 C) = 1.0017 mPa.s  ->  1.4439e+13 1/m     <-- lo que calcula darcy.h
  razon = 0.689
```

**`Rm` está subestimado en un 31 %.** Como `R_torta = R_total − Rm`, el error se transfiere
íntegro y con signo invertido a la resistencia de torta: un sesgo constante de **+6.51e12 m⁻¹**.
A J = 7 LMH y TMP = 0.30 bar, `R_total = 1.54e13`, así que **el error en Rm representa el 42 %
de la resistencia total** — y `R_torta` es precisamente la variable dependiente central de la
tesis.

**Acción:**

```cpp
// darcy.h
static constexpr float MU_KUF_REF = 0.6904e-3f;  // mu del agua a 37 C (condicion ISO 8637 del K_UF)
float _Rm = 1.0f / (MU_KUF_REF * Lp_nominal);    // = 2.095e13 m^-1
```

Y **mejor aún**: como ya tienen `acumularPuntoAguaLimpia()` y `finalizarCalibracionRm()`,
calibrar `Rm` experimentalmente con agua a temperatura conocida y usar el valor nominal sólo como
semilla. La regresión `J vs TMP` sin ordenada al origen está bien planteada — sólo falta
conectarla (M‑2).

Un dato adicional para el Hito 4: la caída de presión en el haz de fibras es significativa.

```
   Q= 204 mL/min  v_lumen=0.94 cm/s  Re=  1.7  dP_haz=  24.6 mbar = 0.025 bar
   Q= 680 mL/min  v_lumen=3.12 cm/s  Re=  5.8  dP_haz=  81.9 mbar = 0.082 bar
   Q=1360 mL/min  v_lumen=6.25 cm/s  Re= 11.5  dP_haz= 163.8 mbar = 0.164 bar
```

Todo laminar. **`P₂` no es una medición redundante:** a 1360 mL/min la diferencia `P₁ − P₂` es
0.164 bar, y su crecimiento es un indicador directo de obstrucción del haz. Además obliga a usar
`TMP = (P₁ + P₂)/2` y no `P₁` — el extremo de entrada ve 0.082 bar más que el promedio.
Vale la pena registrarlo como variable propia del ensayo: es un dato publicable.

### 4.9 Hallazgos menores

- **M-4 · Seguridad y borrado.** `/clear_csv` destruye los 600 registros sin confirmación y sin
  respaldo; todos los endpoints (`/set_dev`, `/reset_dev`, `/calibrar_rpm_q`) están sin
  autenticación y la contraseña del AP está en `config.h` en texto plano. Para un banco
  universitario es aceptable, pero `/clear_csv` debería exigir `?confirm=yes`.
- **M-2 · Código muerto.** 8 constantes de `config.h` sin ninguna referencia y todo `darcy.h`
  sin usar. No es un defecto funcional, pero un auditor externo lo lee como deuda técnica.
  Compilar con `-Wunused-const-variable` y decidir: conectar o eliminar.
- **Persistencia.** El datalogger vive sólo en RAM (600 × 64 B = 38.4 kB). Un corte de energía
  pierde todo. Para el Hito 4 (12 corridas) conviene exportar el CSV al terminar cada ensayo,
  como rutina obligatoria.
- **`filtroId = (uint8_t)targetEnsayo.toInt()`** desborda si `ensayoActualId > 255`. Inocuo con
  `MAX_ENSAYOS = 20`.

### 4.10 Validación de la hoja de ruta

**La secuencia es correcta y la respaldo.** Hito 2.2 → 2.3 → 3 → 4 → 5 tiene la lógica
instrumental adecuada: no se puede cerrar lazo sobre una variable que no se mide, y no se puede
hacer un factorial 3² sin lazo cerrado. Cuatro observaciones de ajuste:

1. **El Hito 2.3 debería absorber el interlock, no el Hito 3.** El interlock de sobrepresión
   depende del transductor, no del controlador. En cuanto existan P₁ y P₂, la comparación
   `if (P1 > 0.60f || tmp > 0.45f) bomba.detener();` son cinco líneas dentro del bloque de 50 ms
   que ya existe. Esperar al Hito 3 deja corridas reales — incluidas las del factorial — sin
   protección activa. Dado B‑4, esto importa.
2. **La DS18B20 pertenece al Hito 2.3, no al 3.** `J₂₀` depende de la viscosidad, y el factorial
   del Hito 4 compara corridas que pueden estar a distinta temperatura. Sin sonda térmica, la
   normalización a 20 °C se hace a mano y es fuente de error sistemático. El sensor cuesta poco y
   `darcy.h` ya tiene `viscosidadAgua()` y `TCF` listos.
3. **El factorial 3² con 3 réplicas en el punto central = 12 corridas es correcto**, pero a
   204 mL/min la velocidad en el lumen es de 0.94 cm/s (§4.8). Es una velocidad tangencial muy
   baja: la polarización de concentración va a dominar y el ensuciamiento será rápido. Sugerencia:
   que los puntos de baja velocidad sean los de menor duración, y que se registre el tiempo desde
   el inicio del ensuciamiento, no sólo el estado final.
4. **El Hito 5 depende de un contrato que aún no existe: el esquema del CSV.** Ver §4.11.

### 4.11 Recomendaciones de transición (respuesta al Eje 4c)

**La más importante: congelar el esquema del CSV ahora, no en el Hito 5.**

`RegistroCalibracion` y el encabezado de `handleExportCSV` son el contrato entre el firmware y los
scripts de Pandas del Hito 5. Hoy el encabezado tiene 17 columnas y ninguna es de presión. Si en
el Hito 2.3 se insertan columnas en el medio, **todo script escrito sobre el formato actual se
rompe**.

Añadir ahora las columnas del Hito 2.3/3 con valor 0 — el costo es 5 `float` × 600 registros =
12 kB de RAM, y el heap libre lo soporta:

```cpp
struct RegistroCalibracion {
  // ... campos existentes, sin reordenar ...
  float p1_bar;      // Hito 2.3 — 0.00 hasta montar el transductor
  float p2_bar;      // Hito 2.3
  float tmp_bar;     // Hito 2.3
  float temp_C;      // Hito 2.3 (DS18B20)
  float j20_lmh;     // Hito 2.3 (J normalizado a 20 C)
  bool  interlock;   // Hito 3
};
```

Y agregar las columnas al final del encabezado CSV, **después de `J_LMH`**, nunca en el medio.
Así el parser de Python se escribe una sola vez.

**Segunda: crear el esqueleto del módulo de presión ahora.** Un `presion.h` con la interfaz final
y una implementación *stub* que devuelve 0, más un `chequearInterlocks()` llamado desde el bloque
de 50 ms. El Hito 2.3 se limita entonces a reemplazar el cuerpo del stub por la lectura del
ADS1115 — sin tocar el lazo de control, que es lo que no se quiere reescribir.

**Tercera: convertir `ML_POR_VUELTA` en un modelo, no en un escalar.** La planilla ya calcula la
ordenada al origen (`INTERCEPT`, celda C26 de Ensayo 2). Si esa ordenada es estadísticamente
distinta de cero — y en una peristáltica con deslizamiento dependiente de la contrapresión suele
serlo — entonces `q = rpm × mL/rev` tiene un error sistemático que **se propaga al auto-calibrador
de K_alim**, porque `qRefAlim = rpm × mL/rev` es su única referencia. El resultado sería una
constante K calibrada contra un caudal teórico sesgado: **dependencia circular entre las dos
calibraciones.** Almacenar pendiente y ordenada, o al menos verificar el lunes que la ordenada es
despreciable frente a su error estándar.

**Cuarta:** corregir A‑9 (viscosidad de referencia de `Rm`) **antes** de conectar `darcy.h` en el
Hito 2.3, no después. Corregirlo después implica re-procesar todos los datos.

**Quinta:** versionar el firmware por hito y etiquetar cada CSV exportado con la versión.
`git tag hito-2.2-precalibracion` antes del lunes cuesta diez segundos y permite defender la
trazabilidad de los datos ante el jurado.

---

## EJE 5 — DICTAMEN FINAL

### Veredicto: **APROBADO CONDICIONADO**

**No está "100 % blindado".** La pregunta tal como está formulada tiene respuesta negativa, y
conviene ser explícito: hay cuatro bloqueantes, uno de los cuales puede dañar la placa y otro que
puede invalidar por completo los datos del lunes.

Ahora bien, separando las dos cosas que se están certificando:

**El algoritmo de medición de caudal está aprobado sin condiciones.** El método de período
recíproco híbrido es correcto y exacto al 0.000 % de 6.5 a 4200 mL/min. La inmunidad al rollover
está demostrada empíricamente en un cruce real de 2³². La cadena dimensional de K y del volumen
integrado es exacta al 0.002 %. La sincronización ISR/tarea es correcta. La cota física no atenúa
en régimen permanente. El rechazo del rizo peristáltico supera 30×. **Este es trabajo sólido y
defendible.**

**La puesta en banco del lunes no está aprobada sin las cuatro condiciones siguientes.**

| # | Condición | Tiempo | Bloqueante que cierra |
|---|---|---|---|
| 1 | Medir el nivel alto de la señal del YF‑S401; si supera 3.6 V, intercalar el divisor 4.7 kΩ / 10 kΩ | 10 min | B‑3 |
| 2 | Vaciar las celdas amarillas de la planilla y verificar que el resumen quede vacío | 5 min | B‑1 |
| 3 | Bypassear la carcasa de la membrana, o retarar el alivio a 0.55 bar y no cerrar la válvula de aguja | 15 min | B‑4 |
| 4 | No presionar "Auto-Calibrar" mientras el bloque de permeado siga activo | 0 min | A‑3 |

Con esas cuatro condiciones satisfechas, **Owen y Antonella pueden encender la bomba y calibrar
los instrumentos.** El resto de los hallazgos (A‑1, A‑2, A‑4, A‑7) son de protocolo y de
procesamiento, no impiden la corrida, y se resuelven con la lista de chequeo.

B‑2 no es una condición para el lunes sino una **corrección de documentación del acta de la
Ronda 3**: los bloqueantes 1 y 2 de Claude Opus no están "Resueltos"; están mitigados por
hardware y pendientes de implementación en el Hito 3. Firmar lo contrario en un acta de
certificación es un riesgo para el equipo.

---

### CHECKLIST DE 5 PASOS PARA EL LUNES — Laboratorio de Operaciones Unitarias, UNSa

**PASO 0 — Antes de energizar (15 min).**
Multímetro en la señal del sensor, con el sensor alimentado y **desconectado del ESP32**, durante
flujo. Si el nivel alto supera 3.6 V → divisor 4.7 kΩ serie + 10 kΩ a GND, con el 100 nF en la
unión. Abrir `PLANILLA_CALIBRACION_ENSAYOS_2_Y_3.xlsx`, **borrar `C12:D19` (Ensayo 2) y `C12:C19`
y `E12:E19` (Ensayo 3)**, y comprobar que el bloque de resumen dé error o vacío — no 13.60 ni
154.62. Conectar la membrana en bypass, o verificar el alivio en 0.55 bar.

**PASO 1 — Cebar y purgar (10 min).**
Circular agua a 30 RPM con la línea de retorno abierta hasta que no quede una sola burbuja en la
manguera de silicona ni en el cuerpo del caudalímetro. El aire en el rotor es la causa más común
de sobreconteo, y explica desvíos del orden del que se observa en `K_ALIM`. Verificar el sentido
de la flecha del sensor.

**PASO 2 — Ensayo 2, cilindrado de la bomba (45 min).**
Por cada nivel de RPM (20 · 30 · 40 · 50 · 60 · 70 · 80 · 90): fijar la consigna, presionar START,
y **esperar a que el SCADA muestre régimen estable** — son 7 s a 20 RPM y 27 s a 90 RPM, no es
instantáneo. **Sólo entonces** arrancar el cronómetro y colectar 60 s (40 s a 80 y 90 RPM, tomando
los 30 s finales). Repetir 3 veces por nivel. Si la balanza de 0.1 g está disponible, **usarla en
lugar de la probeta**: sobre 680 g aporta ±0.015 % contra ±1.8 % de la probeta.

**PASO 3 — Ensayo 3, constante K del caudalímetro (30 min).**
En los mismos niveles, con el sensor en serie, anotar la frecuencia del SCADA **en régimen
estable y tras al menos 15 s de asentamiento** (la EMA tarda 14 s en llegar a 0.1 %). Registrar
frecuencia y volumen de probeta **simultáneos** en la misma corrida. Contrastar el caudal de
probeta contra `RPM × 13.6`: si discrepan, el problema es hidráulico, no del sensor. Este cruce es
el que discrimina las tres hipótesis sobre `K_ALIM = 154.62` (§3.2).

**PASO 4 — Cierre y saneamiento de datos (15 min).**
Tras cada STOP: **esperar 25 s antes de exportar** y descartar las dos últimas filas del CSV de
cada ensayo (son las muestras espurias de parada: 378 y 229 mL/min). Verificar en el resumen de la
planilla que **σ ≠ 0, CV% ≠ 0 y R² < 1.0000** — si R² da 1.0000 exacto, quedan datos pre-cargados.
Revisar la ordenada al origen de la regresión Q vs RPM: si es significativa frente a su error
estándar, `ML_POR_VUELTA` como escalar no basta (§4.11). **No presionar "Auto-Calibrar".**
Etiquetar el commit con `git tag hito-2.2-YYYYMMDD` antes de cargar los parámetros a la NVS.

---

## 6. LO QUE NO SE PUDO VERIFICAR, DECLARADO EXPLÍCITAMENTE

- **No se compiló el firmware para ESP32.** No hay `arduino-cli` ni PlatformIO en el entorno; se
  instaló PlatformIO, pero la descarga de la plataforma `espressif32` falla con `HTTPClientError`
  (el registro está bloqueado en este sandbox). **No puedo certificar que el sketch compila limpio
  para el target.** Lo que sí se compiló y ejecutó es `caudalimetro.cpp` y `Bomba.cpp` contra
  shims de API en x86‑64, lo que valida la lógica pero no el enlazado, el uso de IRAM, el tamaño
  de binario ni el comportamiento de `portENTER_CRITICAL` real en doble núcleo.
- **La exclusión mutua multinúcleo se razonó, no se ejecutó.** El argumento es sólido
  (`portENTER_CRITICAL` enmascara interrupciones en el núcleo llamante y toma el spinlock; la ISR
  usa `portENTER_CRITICAL_ISR` sobre el mismo `portMUX`), pero no se sometió a una prueba de
  estrés concurrente.
- **`index_html.cpp` (739 líneas) no se auditó.** La interfaz puede exponer el botón de
  auto‑calibración sin advertencia (A‑3); no lo verifiqué.
- **El comportamiento eléctrico real del YF‑S401 no se midió.** La conclusión B‑3 se basa en el
  datasheet del fabricante (nivel alto > 4.7 V con alimentación de 5 V) y en la FAQ de Espressif
  (tolerancia de GPIO 3.6 V). Unidades concretas pueden variar: **por eso la condición 1 del
  checklist es una medición con multímetro, no una suposición.**
- **El límite de 0.50 bar de TMP de la FX100 se tomó del brief.** Los datos públicos de Fresenius
  que pude verificar confirman K_UF = 73, 2.2 m², pared/lumen 35/185 µm y un caudal máximo de
  sangre de 600 mL/min, pero no localicé una cifra pública de TMP máxima. **Confirmar contra el
  prospecto antes de retarar la válvula.**
- **Las cifras de bits efectivos del ADS1115** son las nominales del datasheet para el rango
  ±6.144 V. La resolución efectiva en el banco dependerá del layout y del acoplamiento con el
  DM860; debe medirse con la entrada cortocircuitada antes de confiar en ella.

---

*Auditoría Ronda 4 · Emitida sobre el commit `9e05eaa` · Todos los resultados numéricos citados
son reproducibles ejecutando `verificacion_host/harness.cpp` y `verificacion_host/rampa.cpp`.*
