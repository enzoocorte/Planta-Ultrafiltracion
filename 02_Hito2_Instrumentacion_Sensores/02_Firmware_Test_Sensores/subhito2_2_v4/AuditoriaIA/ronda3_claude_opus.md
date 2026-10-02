# Dictamen de Auditoría Técnica Externa — Ronda 3 (Cierre de Arquitectura)

**Auditor:** Claude 5.5 Opus (Anthropic)  
**Destinatarios:** Antigravity AI, Ing. Enzo, Antonella Guitián, Owen Cañizares  
**Fecha:** Octubre 2026  
**Alcance:** Revisión integral de arquitectura, firmware v4, concurrencia FreeRTOS, instrumentación de presión ADS1115 y protocolo experimental de ultrafiltración FX100 (Hito 5).

---

## 1. Evaluación ejecutiva

**Veredicto: APROBADO CONDICIONADO.**  
La arquitectura general es sólida: la separación de núcleos, el snapshot, los dos tipos de parada y la corrección de la TMP mínima. Antes de encender la planta con membrana hay cuatro bloqueantes:

1. Los umbrales de sobrepresión están en el fondo de escala del sensor.
2. El enclavamiento de TMP se evalúa a 1 Hz.
3. El caudalímetro de permeado opera por debajo de su rango útil.
4. El permeado no se recircula, así que la concentración del tanque cambia durante el ensayo.

### Cifras que verifiqué

| Dato | Resultado |
|---|---|
| ΔP lumen a 1360 mL/min | 0,164 bar. Correcto. |
| Corte γ_w a 1360 mL/min | ≈2700 s⁻¹. Correcto. |
| Área π·dᵢ·L·N | 2,197 m². Correcto. |
| Vogel a 20 °C | 1,0016 mPa·s. Correcto. |
| Dirección de la TCF | J₂₀ = J·μ(T)/μ₂₀. Correcta. |
| Valores de K | Probablemente en Hz/(L/min), no en "pul/L". |
| K_UF | Inconsistente (ver abajo). |
| Resistencia de membrana Rm | ≈1,4·10¹³ m⁻¹ con el K_UF corregido. |
| Diseño 3² | "9 + 3 réplicas" suma 12, no 11. |

### Errores de datos

* **K_UF:** 73 mL/(h·mmHg) × 750 mmHg/bar = 54 750 mL/h/bar, no 5475. Tu propio "365 mL/min a 0,40 bar" usa el valor correcto (912 mL/min/bar). Corrige la tabla.
* **Unidades de K:** el YF-S401 es F = 98·Q con Q en L/min, es decir Hz por (L/min), unas 5880 pulsos/L. Tus 154,62 y 55,00 son coherentes con "Hz por L/min". Esto cuadra con el comentario del ISR (210 Hz a 1360 mL/min). El rótulo "pul/L" es erróneo y puede causar errores de 60×.
* **Dispersión de K:** +58 % en alimentación y −44 % en permeado respecto al nominal es mucho para el mismo modelo. Revisa cómo se determinó K, porque la probeta a 50 y 72 RPM solo calibra la bomba. Con dos puntos no se puede comprobar linealidad.

---

## 2. Ejes 1 a 4

### Eje 1: Metrología de caudal

#### Corner cases en `actualizar()`
* **Primera ventana con n = 1:** span = tUlt − tPrim = 0, así que f no se actualiza. El volumen sí suma, pero hay un ciclo de retardo. Es inocuo, aunque conviene documentarlo.
* **Cota física:** la cota solo reduce _f, nunca lo eleva. Tras un arranque brusco, el siguiente ciclo con pulsos lo recalcula bien. Con el EMA de 0,5 el resultado converge en 2 o 3 s.
* **Permeado a bajo caudal:** a 55 Hz por L/min, 10 mL/min son 0,55 Hz y hay menos de un pulso por ventana de 1 s. El timeout de 5 s equivale a 3,6 mL/min. Para permeado conviene medir Q como Δvolumen/Δt sobre 10 a 30 s en vez de usar una ventana de 1 s.
* **Filtro del ISR:** en isrPuente lee _t_ultimo fuera de la sección crítica. Es aceptable en 32 bits alineados. Usa un solo flanco para evitar la asimetría de subida y bajada del RC (τ = 0,47 ms, fc = 338 Hz, cerca de los 210 Hz).
* **Rango del YF-S401:** Su rango útil es ≈0,3 a 6 L/min. El permeado esperado (45 a 365 mL/min, y menos con ensuciamiento) está casi todo por debajo. La turbina tiene fricción de rodamiento y no linealidad ahí. Recomiendo una balanza con salida serial como referencia primaria de J. El método estándar en UF es gravimétrico, y el YF-S401 queda como indicador secundario con K(f) por tramos.

#### Rizo peristáltico
La frecuencia es 3·RPM/60: 0,75 Hz a 15 RPM, 2,2 Hz a 44 RPM y 5 Hz a 100 RPM. Tu rango de 1,25 Hz corresponde a 25 RPM. Una ventana de 1 s casi nunca abarca un número entero de ciclos y mete un sesgo de fase. El EMA no lo corrige, porque el problema es la ventana y no el filtrado.
* **Promediado sincrónico:** cierra las ventanas cada k vueltas (k·3200 pasos, contados por hardware) o cada k·(20/RPM) s, con k ≥ 3.
* **Presiones:** muestrea el ADS1115 a 20 Hz como mínimo y promedia sobre las mismas ventanas. Reporta también el pico, que es lo que ve el flujo crítico.
* **Hardware:** un amortiguador de pulsación en la impulsión reduce el rizo, pero cambia la compliance. Si lo instalas, hazlo antes de calibrar.
* **Darcy:** como Darcy es lineal, la media de TMP da la media de J. El sesgo aparece solo si el rizo cruza el umbral del flujo crítico.
* **K fijo o K(f):** usa K(f) por tramos para el permeado con al menos 5 a 8 puntos, y verifica el error de reproducibilidad. Para la alimentación, un K lineal basta, porque la bomba da Q = 13,6·RPM como referencia. Un K fijo sirve de alarma de pasos perdidos si |Q − 13,6·RPM| > 15 %. Esa alarma no está en el diseño y conviene agregarla.

---

### Eje 2: FreeRTOS

* **Inmunidad al Wi-Fi:** no es total. La tarea del driver Wi-Fi va en el Core 0, pero tcpip_thread (prioridad 18) suele estar sin afinidad y puede correr en el Core 1. Con prioridad 19 no te preempta, pero compartís bus de memoria y caché. Mide el jitter con un GPIO y un analizador lógico durante tráfico HTTP real, y define el criterio de aceptación (por ejemplo, <2 ms pico).
* **Starvation y watchdog:** tareaControl bloquea en vTaskDelayUntil, así que IDLE1 corre. En el Core 0, el Wi-Fi (prioridad 23) puede dejar sin tiempo a IDLE0 bajo carga. Controla el TWDT, y registra tareaControl en él con una acción segura.
* **Cuelgue de la tarea:** el LEDC sigue generando pulsos a la última frecuencia si la tarea se cuelga, y la bomba seguiría girando. El TWDT con reset corta los pines, pero verifica el estado de ENA y de los pines de arranque (strapping) tras un reset. Un watchdog externo sobre ENA es lo ideal.
* **I2C:** usa Wire solo desde tareaControl y configura setTimeOut.
* **Spinlock vs seqlock:** el snapshot real pesa unos 80 B, así que el spinlock es suficiente y más simple. El static_assert dice 320 B y el texto dice 300 B, y no coinciden. El seqlock no aporta ventaja práctica aquí. Una alternativa igual de simple es una cola de longitud 1 con xQueueOverwrite.
* **Bug en el código:** sLocal nunca se llena. Es un esqueleto, pero conviene dejarlo explícito.
* **Latencia de parada:** las banderas se evalúan cada 50 ms. Para ESTOP físico, usa una ISR de GPIO que corte los pulsos directamente.

#### Flash (NVS y LittleFS)
* El borrado de sector (decenas de ms) deshabilita la caché en ambos núcleos. Las ISR que no estén en IRAM, o que no se hayan registrado con ESP_INTR_FLAG_IRAM, se retrasan o se pierden. A 210 Hz hay un flanco cada 4,8 ms, así que se pierden varios pulsos.
* Los pulsos LEDC/RMT son por hardware y no se ven afectados, salvo que no puedas actualizar la frecuencia durante ese tiempo.
* **Medidas:** escribe NVS solo con la bomba parada, nunca durante un ensayo. Guarda los datos en RAM o en tarjeta SD (que no detiene la caché) o transmítelos por Wi-Fi a la PC. Mejor aún, mide los pulsos de caudal con MCPWM capture o PCNT, que no dependen de la latencia de ISR. Verifica que micros() y la ISR estén en IRAM.

---

### Eje 3: Seguridad mecánica

* **Umbrales:** el sensor de 0 a 1,0 bar satura en su fondo de escala. "P1 > 1,0 bar" es inalcanzable por lectura, y con la válvula de alivio a 1,0 a 1,2 bar se supera el límite de TMP de la membrana. Con P2 ≈ 1,0 la TMP llega a ~0,9 bar, por encima del límite de 0,5 a 0,67 bar. Baja el alivio a ≈0,7 bar (probado con manómetro de referencia), el disparo de software a ≈0,6 bar y el de TMP en 0,45 bar.
* **Presostato:** sí, recomiendo un presostato electromecánico en serie con ENA y la alimentación del DM860, independiente del firmware. Debe calibrarse a ~0,7 bar y cubrir un fallo del ADS o del ESP32.
* **Enclavamiento a 1 Hz:** el chequeo de TMP está dentro del if (++ciclo % 20 == 0). Con una bomba de 4,5 Nm, un segundo es mucho. Evalúalo en cada ciclo de 50 ms con un FSM no bloqueante del ADS (4 canales a 250 SPS ≈ 20 ms).
* **ESTOP y cola:** g_estop.store(false) desenclava la parada de emergencia, y un CMD_ARRANCAR pendiente en la cola puede reiniciar la bomba en el mismo ciclo. La parada debe quedar enclavada hasta un reset explícito, y la cola debe vaciarse.
* **Techo de 44 RPM:** hay riesgo de oscilación si la telemetría parpadea en el umbral de 1,5 s. Agrega histéresis: baja el techo de inmediato, pero solo lo sube tras 5 a 10 s de telemetría continua, con rampa.
* **Inversión:** 300 a 500 ms sobra para la inercia del rotor. El riesgo real es hidráulico: al invertir, la válvula de alivio queda del lado de aspiración y se pierde la protección, y la membrana puede recibir presión o vacío por el lado de alimentación. Bloquea la inversión con la membrana conectada, o instala un alivio en ambos sentidos.
* **Driver DM860:** verifica la corriente de entrada de PUL/DIR con 3,3 V del ESP32. Muchos DM860 necesitan 5 V o un buffer (74AHCT125) para no depender de la corriente del GPIO.

---

### Eje 4: Protocolo científico

#### Espacio no rectangular
* Tu cálculo de TMP_min ≈ ΔP/2 es correcto. A 95 RPM, TMP = 0,10 deja P2 ≈ 0,02 bar, casi en el límite.
* El problema es la esquina de 25 RPM y 0,40 bar. Con 340 mL/min de alimentación, la TMP necesitaría ~365 mL/min de permeado con agua limpia. Es inviable y, además, la recuperación sería enorme.
* **Recomiendo:** definir el diseño sobre variables físicas (corte en pared y flujo de permeado, o recuperación) y usar un diseño D-óptimo o un CCD de cara centrada sobre la región factible. Un 3² completo no es publicable si hay celdas inalcanzables. Si mantienes el 3², cambia los niveles (por ejemplo 40, 70, 95 RPM) y aclara si son 11 o 12 corridas.
* **Gradiente axial:** la TMP varía a lo largo de la fibra (hasta 0,16 bar). El Jc que midas es un promedio y no un valor local. Repórtalo así.

#### Flujo crítico
* Con solo una válvula de retentado, el método escalonado de TMP es el factible. Constant-flux con estrangulamiento no controla J. Para flux-stepping real haría falta una bomba en el lado de permeado.
* Define Jc por la desviación respecto a la recta de agua limpia (Fase 0) con umbral, por ejemplo 5 % y 3σ de la incertidumbre de presión. La inflexión de dJ/dTMP con pasos de 0,05 bar es ruidosa.
* Los pasos de 15 min pueden no alcanzar estado estacionario. Usa dTMP/dt o dJ/dt como criterio.
* **Recircula el permeado al tanque durante los ensayos**, o define un volumen de tanque y un factor de concentración. Con 100 a 365 mL/min durante 45 a 60 min se extraen de 5 a 20 L, y la concentración de alimentación cambia en cada paso.
* **Temperatura:** la bomba calienta el circuito y la viscosidad varía ≈2,4 %/°C. El DS18B20 tiene ±0,5 °C, o sea ≈1,2 % de error en J₂₀. Calíbralo contra una referencia y ponlo en la línea de alimentación.
* **Presión a TMP baja:** Un transductor de 0 a 1 bar con ±0,5 a 1 % FS tiene 5 a 10 mbar de error, es decir 10 a 20 % de la TMP en el escalón de 0,05 bar. Además, con el ADS a 3,3 V, el sensor ratiométrico alimentado a 5 V pierde la ratiometría. Alimenta los sensores con un 5 V estable o mide el riel. Calibra con una columna de agua (0 a 1 m) en varios puntos y tara in situ.
* **Fase 0:** R² ≥ 0,985 con 5 puntos es un criterio débil. Exige también que el intercepto sea compatible con cero. Si no lo es, es un offset de presión, y esa regresión sirve para corregir la tara. Especifica RPM ≥ 60, porque a 15 RPM no se pueden alcanzar los 0,25 bar. Incluye el cebado, el enjuague y la eliminación de aire de un FX100 nuevo.

#### Limpieza
* Para el retrolavado, usa una carga hidrostática desde un depósito elevado (≤1,5 m ≈ 0,15 bar) con el lumen abierto al drenaje. Es lo más simple y seguro. Mantén la contrapresión inversa ≤0,1 a 0,2 bar, en pulsos de 30 a 60 s.
* NaOCl a 100 a 200 ppm y pH 10: es razonable para el mucílago de Opuntia, pero degrada el PVP y la polisulfona de forma acumulativa. Lleva un registro de dosis (ppm·h) y compara Rm,0 entre días. Fija un tope acumulado con la hoja técnica del fabricante. Para flóculos inorgánicos usa ácido cítrico en un paso aparte, nunca junto con cloro.

---

## 3. Red flags (Eje 5)

### Críticos
1. Disparo y alivio de presión en el fondo de escala del sensor y por encima del límite de la membrana.
2. Interlock de TMP a 1 Hz; ESTOP desenclavable y comandos de cola que reinician la bomba.
3. Permeado medido con una turbina fuera de rango, sin balanza de referencia.
4. Inversión de bomba con la membrana conectada y sin protección en sentido inverso.

### Altos
5. Permeado no recirculado; concentración variable.
6. Rótulo de K en pul/L y K_UF con factor 10 de error.
7. Diseño factorial con esquinas inviables.
8. El LEDC sigue pulsando si la tarea se cuelga.

### Seguridad de laboratorio
* Diferencial de 30 mA y puesta a tierra en el banco, con agua y 4 A cerca.
* Protección contra salpicaduras y rotura (policarbonato) alrededor de la membrana y las mangueras.
* Manguera de silicona con presión nominal verificada.
* EPP para NaOCl y para ácidos.

---

## 4. Checklist de aceptación

### Antes de encender
- [ ] Corregir K_UF (54 750) y el rótulo de unidades de K.
- [ ] Alivio a ≈0,7 bar, probado con manómetro; presostato en serie con ENA; E-Stop físico.
- [ ] Disparos de software en 0,6 bar (P1) y 0,45 bar (TMP), evaluados cada 50 ms.
- [ ] ESTOP enclavado, cola vaciada al disparar, bloqueo de inversión con membrana.
- [ ] Buffer 5 V en PUL/DIR y estado seguro de ENA tras el reset.

### Metrología
- [ ] Calibrar los 3 transductores con columna de agua; tara in situ; alimentación estable.
- [ ] Balanza serial para J; K(f) del permeado con ≥5 puntos; alarma de pasos perdidos.
- [ ] Ventanas sincrónicas con el rizo; muestreo de presión ≥20 Hz.
- [ ] Verificar el DS18B20 contra una referencia.

### Firmware
- [ ] Sin escrituras a flash durante ensayos; datos a SD o PC.
- [ ] Medir el jitter del lazo de 50 ms con tráfico Wi-Fi.
- [ ] Rellenar sLocal; unificar 300/320 B.

### Protocolo
- [ ] Recircular el permeado; controlar temperatura.
- [ ] Rediseñar el factorial (región factible: 40, 70, 95 RPM; o D-óptimo / CCD).
- [ ] Jc por desviación respecto a la recta de agua limpia con incertidumbre.
- [ ] Retrolavado hidrostático ≤0,15 bar; registro de dosis de NaOCl acumulada.
