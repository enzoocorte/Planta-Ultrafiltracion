# New conversation

## User

[empty message]

## Assistant

## Dictamen ejecutivo

**La v4 mejora sustancialmente la v3, pero todavía no la consideraría validada para ensayos desatendidos ni para proteger automáticamente la membrana.** El parche de inversión resuelve el conflicto principal con el slider; el estimador recíproco reduce la cuantización; y el streaming HTTP es una mejora correcta. Sin embargo, quedan problemas de temporización, estados transitorios y verificabilidad.

La ampliación a **100 RPM puede plantearse como rango experimental**, pero no queda justificada únicamente por sustituir sangre por agua: deben verificarse los límites hidráulicos del cartucho, conexiones, tuberías y circuito, además de la TMP. **Una alarma por RPM no equivale a una protección de membrana.**

Mi evaluación se basa en el código y las descripciones incluidos en *pasted-text.txt*, §§1–4. Las funciones HTTP, la inicialización LEDC y otras partes están resumidas u omitidas; por tanto, las conclusiones sobre ellas son condicionadas.

---

# A. Validación del firmware v4

## 1. Inversión: corregida parcialmente, no cerrada completamente

### Lo que sí queda resuelto

Durante `_invirtiendo`, `setRPM()` actualiza `_rpmGuardada` sin sobrescribir `_objetivo = 0`. Esto elimina el bloqueo señalado en la ronda anterior.

Además, `detener()` cancela la inversión y permite conservar una consigna recuperable. El caso **STOP durante inversión → START posterior** parece coherente con las funciones mostradas.

### Problemas pendientes

**a) “Cero físico” no está demostrado**

```cpp
if (_invirtiendo && _actual <= 0.1f)
```

`_actual` representa una velocidad calculada, no medida. Cortar PUL tampoco prueba que el rotor esté completamente detenido.

Sin intervenir mecánicamente la bomba, recomiendo introducir estados explícitos:

```text
PARADA → ACELERANDO → MARCHA
                         ↓
                  FRENANDO_INVERSIÓN
                         ↓
                  PULSOS_DESHABILITADOS
                         ↓
                    ESPERA_PARADA
                         ↓
                  CAMBIO_DIR / ESPERA
                         ↓
                    ACELERANDO
```

La espera debe respetar el tiempo de establecimiento de DIR indicado por el manual del driver y un margen de parada validado experimentalmente.

**b) Cambio de sentido con `_actual < 1 RPM`**

En `toggleSentido()`, esta condición permite cambiar DIR directamente. Es una aproximación de software, no una garantía de parada. Conviene que **todo cambio de DIR pase por la misma secuencia de deshabilitación y espera**.

**c) STOP no cumple siempre “< 1,5 s”**

Con la desaceleración declarada:

\[
t_{\mathrm{STOP}}\approx \frac{\mathrm{RPM}}{45}
\]

Desde 100 RPM son **2,22 s**, antes de considerar dinámica real y temporización. Debe corregirse el comentario o redefinirse el requisito.

Una parada hidráulica de emergencia tampoco debería depender exclusivamente de esta rampa.

**d) La rampa no es una S-curve estricta**

El código implementa una aceleración variable, pero no limita explícitamente el *jerk*. También introduce un salto de la consigna interna desde cero hasta 1 RPM. Puede ser aceptable, pero debe describirse y ensayarse como tal.

**e) Validar `dt`**

Si HTTP bloquea el lazo y luego llega un `dt` grande, el siguiente incremento puede ser abrupto. La solución principal es desacoplar tareas; adicionalmente:

- comprobar que `dt` sea finito y positivo;
- detectar vencimientos del período;
- establecer una respuesta definida ante retrasos excesivos.

### Verificaciones eléctricas indispensables

- Confirmar que **la variante exacta del DM860 acepta de forma fiable 3,3 V**, con la corriente necesaria en sus entradas optoacopladas. No lo asumiría por el nombre comercial.
- Fijar versión de Arduino-ESP32: la API LEDC cambia entre versiones.
- Comprobar errores de inicialización y cambio de frecuencia.
- Medir PUL y DIR con osciloscopio durante arranque, STOP e inversión.

A 100 RPM se requieren aproximadamente **5,33 kHz**. La capacidad nominal del periférico no sustituye la comprobación de los pulsos reales.

*Fuente: pasted-text.txt, §§1.2 y 3.A–B.*

---

## 2. Conteo recíproco: buena elección, con cuatro bordes importantes

El estimador:

\[
f=\frac{n-1}{t_{\mathrm{último}}-t_{\mathrm{primero}}}
\]

es adecuado para evitar el salto de un pulso por ventana. **Reduce la cuantización temporal, pero no elimina la incertidumbre de la turbina, su umbral de arranque ni la pulsación de la bomba.**

### a) Retención después de una parada real

Sí: mantener frecuencia hasta \(2,5T\) introduce una indicación residual.

A 5,5 Hz:

\[
2,5T\approx 0,455\ \mathrm{s}
\]

La indicación puede durar más según el período de `actualizar()` y el filtrado aplicado. A frecuencias menores el retardo aumenta considerablemente.

**Recomendación:** publicar también:

- edad del último flanco;
- estado `VALIDO / RETENIDO / SIN_FLUJO / NO_INICIALIZADO`;
- período y cantidad de pulsos utilizados.

La frecuencia retenida puede servir para visualización; **no debería tratarse como evidencia independiente de flujo para una protección**.

La integración por pulsos evita acumular volumen durante la retención, lo cual es correcto.

### b) Primer pulso después de encender o de una pausa

`_periodo_us` se calcula respecto de `_t_ultimo`, inicialmente cero o antiguo. El primer pulso puede producir una frecuencia basada en el tiempo de inactividad.

Conviene invalidar el período después de un timeout y requerir **dos flancos válidos** para reconstruir una medición recíproca.

### c) Rollover de `micros()`

Esta condición es problemática:

```cpp
t_ult > t_prim
```

Si la ventana cruza el rollover de `micros()`, deja de cumplirse, aunque exista un intervalo válido. En ese caso el código puede conservar una frecuencia anterior.

Usar resta unsigned:

```cpp
uint32_t intervalo = t_ult - t_prim;
if (n >= 2 && intervalo > 0) {
    // ...
}
```

También debe revisarse:

```cpp
(25 * per_us) / 10
```

La multiplicación puede desbordar. Usar aritmética de 64 bits y un timeout máximo explícito.

### d) El diagnóstico “cinco segundos” depende del scheduler

`_segSinPulso` cuenta llamadas, no segundos, y `dt_s` no se utiliza. Si `actualizar()` pasa a ejecutarse cada 50 ms, cinco llamadas representan 250 ms.

Implementar el diagnóstico con tiempo transcurrido. Además:

- alimentación sin pulsos con bomba impulsando puede señalar una anomalía;
- permeado sin pulsos **no implica necesariamente fallo del sensor**: puede indicar baja TMP, obstrucción o flujo inferior al umbral de la turbina.

### Unidades del volumen

Con la definición utilizada, \(K\) está expresado en Hz por L/min, y:

```cpp
_vol += n / (_k * 60.0f);
```

produce **litros**. Si el getter o CSV lo etiqueta en mL, falta multiplicar por 1000.

El límite de caudal y el blanking tampoco distinguen ruido de pulsos plausibles: una interferencia dentro del rango admisible puede seguir contabilizándose.

*Fuente: pasted-text.txt, §§1.3 y 3.C.*

---

## 3. Heap: mitigado, todavía no demostrado

Eliminar concatenaciones repetidas y transmitir el CSV por filas es correcto. Pero **no puede afirmarse “cero asignaciones” para toda la ruta HTTP** sin ver la implementación y la biblioteca: pueden existir conversiones a `String`, buffers de red y asignaciones internas.

Verificar:

- retorno de `snprintf()` y detección de truncamiento;
- finalización correcta de respuestas chunked;
- cliente desconectado o muy lento;
- acceso concurrente al datalogger;
- uso y ubicación del buffer de 768 bytes;
- stack disponible en cada tarea.

Ensayo recomendado: **8–24 horas**, con consultas `/status`, exportaciones repetidas y desconexiones. Registrar:

- heap libre y mínimo;
- mayor bloque libre;
- mínimo de stack;
- retrasos del control;
- errores HTTP y reinicios.

No basta con observar heap libre estable: el mayor bloque disponible también debe estabilizarse.

*Fuente: pasted-text.txt, §3.E.*

---

# B. FreeRTOS y Snapshot

## Arquitectura mínima recomendada

| Componente | Responsabilidad |
|---|---|
| `taskControl`, Core 1, prioridad inicial 10 | Máquina de estados, rampas, comandos, supervisión y publicación del snapshot |
| `taskWeb`, Core 0, prioridad inferior | HTTP, validación de solicitudes y envío de comandos |
| Adquisición | Máquina de estados ADC; puede integrarse inicialmente al control si todas las operaciones están acotadas |
| ISR de caudal | Captura mínima de pulsos y timestamps |

Usar `vTaskDelayUntil()` con período nominal de 50 ms, medir el `dt` real y contabilizar vencimientos. La prioridad 10 es un punto de partida, no una garantía: debe comprobarse frente a las tareas del framework, Wi-Fi y watchdogs.

**Regla central: la web no modifica directamente el objeto `Bomba`.** Envía comandos mediante una cola; `taskControl` es el único propietario del estado de control.

Para evitar una cola llena de movimientos del slider:

- coalescer consignas de RPM;
- definir una política para comandos incompatibles;
- hacer que STOP tenga tratamiento prioritario y no pueda perderse por saturación.

Una emergencia física requiere una vía independiente del servidor web.

## Snapshot: elegiría un mutex corto

Para este sistema recomiendo primero **snapshot POD + mutex FreeRTOS con herencia de prioridad**, no doble buffer artesanal.

```text
Control:
  construye copia local
  toma mutex
  copia estructura compartida
  libera mutex

Web:
  toma mutex
  copia estructura a variable local
  libera mutex
  serializa y transmite fuera del mutex
```

El snapshot debería incluir timestamp, secuencia, mediciones, validez, alarmas y estado cinemático.

**Nunca mantener el mutex durante HTTP, I²C, Serial o escritura de archivos.**

Un doble buffer con puntero atómico no resuelve por sí solo la vida útil del buffer: el productor puede reutilizarlo mientras el lector continúa copiándolo. Requiere un protocolo adicional y orden de memoria correcto.

Ningún mecanismo garantiza literalmente ausencia de contención. Aquí interesa **contención breve, acotada y medida**, no complejidad innecesaria.

---

# C. ADS1115, presión y TDS

## 1. Recomiendo single-ended para las tres presiones

Medir individualmente \(P_1,P_2,P_3\) permite calcular:

\[
TMP=\frac{P_1+P_2}{2}-P_3
\qquad
\Delta P_{\mathrm{axial}}=P_1-P_2
\]

y detectar sobrepresiones que podrían quedar ocultas al observar únicamente diferencias.

La lectura diferencial directa solo proporciona una diferencia de presiones correctamente si las funciones de transferencia de los transductores están calibradas y son compatibles. Diferentes offsets o sensibilidades impiden convertir simplemente \(V_1-V_3\) en \(P_1-P_3\).

Antes de conectar:

- identificar salidas: 0–5 V, 0,5–4,5 V, 4–20 mA, etc.;
- acondicionar y proteger cada entrada;
- respetar los límites absolutos del ADS1115;
- comprobar alimentación y niveles lógicos del bus.

**El rango del PGA no permite aplicar tensiones fuera de los límites eléctricos del chip.**

## 2. Muestreo no bloqueante

Propuesta inicial:

- ADS1115 en *single-shot* a **475 SPS**;
- secuencia \(P_1\rightarrow P_2\rightarrow P_3\);
- TDS intercalado a menor frecuencia, si su módulo entrega una salida analógica compatible;
- 860 SPS si el ensayo de pulsaciones y la precisión requerida lo justifican.

Los SPS corresponden al convertidor, **no a cada canal**. El ADS1115 es multiplexado, no simultáneo.

Máquina de estados:

```text
CONFIGURAR_CANAL
       ↓
INICIAR_CONVERSIÓN
       ↓
ESPERAR_READY sin bloqueo
       ↓
LEER_RESULTADO + TIMESTAMP
       ↓
SIGUIENTE_CANAL
```

Usar ALERT/RDY o consultar el estado de conversión, con timeout y recuperación ante fallos. Las transacciones I²C también deben tener tiempo máximo.

La bomba de tres rodillos introduce una componente principal próxima a 5 Hz a 100 RPM, además de armónicos. Para TMP conviene adquirir las presiones consecutivamente y aplicar filtrado temporal equivalente. Para picos de seguridad deben considerarse el desfase entre canales y el ancho de banda del sistema.

## 3. TDS no sustituye concentración de sólidos

Una sonda de conductividad no mide directamente sólidos suspendidos, turbidez ni masa de torta. La conversión a TDS depende de composición y temperatura.

Registrar conductividad y temperatura, con calibración apropiada. Para fouling, medir aparte sólidos/turbidez y las variables del tratamiento con Opuntia.

*Base del requisito: pasted-text.txt, §4.C.*

---

# D. Diseño experimental: separar flujo crítico y compresibilidad

## 1. Un factorial único no demuestra ambos parámetros

Tres factores con tres niveles constituyen **\(3^3=27\) combinaciones**, no nueve.

Además, variar la dosis de Opuntia modifica tamaño de agregados, material residual y propiedades del depósito. No equivale a variar solamente la concentración de sólidos alimentados.

Propondría dos campañas relacionadas.

## Campaña 1: respuesta operativa y umbral de fouling

Niveles iniciales **provisionales**, sujetos a caracterización hidráulica:

| Factor | Bajo | Medio | Alto |
|---|---:|---:|---:|
| RPM | 25 | 60 | 95 |
| TMP media, bar | 0,10 | 0,25 | 0,40 |
| Dosis de Opuntia | Baja | Óptima preliminar | Alta |

La dosis debe expresarse en una magnitud reproducible —por ejemplo, masa seca equivalente por volumen— y no solo en volumen de un extracto variable.

Condiciones necesarias:

- mantener o medir concentración de sólidos, pH, temperatura y protocolo de coagulación;
- evitar que la concentración cambie inadvertidamente durante la corrida;
- registrar caudal real: RPM no sustituye caudal;
- bloquear por lote de alimentación, cartucho y día;
- aleatorizar el orden;
- realizar réplicas independientes.

Tres repeticiones completas darían **81 corridas**. Si no es viable, realizar primero un cribado y después replicar las regiones relevantes. Repetir puntos centrales no sustituye replicar las condiciones extremas.

### Flujo crítico

El factorial ayuda a localizar condiciones favorables, pero **no basta para determinar \(J_c\)**. Agregar ensayos escalonados:

1. Estabilizar condiciones de alimentación y flujo tangencial.
2. Incrementar TMP por escalones y medir \(J(t)\).
3. Evaluar pendiente de pérdida de flujo y resistencia.
4. Volver a escalones inferiores para observar histéresis.
5. Repetir con diferentes velocidades tangenciales.

Si no existe control de flujo de permeado, el resultado debe presentarse como **umbral operativo de fouling bajo escalones de presión**, indicando su dependencia del protocolo. Para un ensayo de flujo impuesto, interesa observar el crecimiento sostenido de TMP.

Definir antes del ensayo el criterio de detección, duración de cada escalón e incertidumbre. “Irrefutable” debe sustituirse por **reproducible y estadísticamente sustentado**.

## Campaña 2: resistencia específica y compresibilidad

El modelo:

\[
R_{\mathrm{total}}=\frac{TMP}{\mu J}
\]

estima resistencia aparente. Pero:

\[
R_{\mathrm{total}}-R_m
\]

puede incluir torta, bloqueo de poros y otros efectos; no identifica exclusivamente torta.

Para obtener resistencia específica:

\[
R_c=\alpha\frac{M_c}{A}
\]

se necesita medir o estimar justificadamente la masa depositada \(M_c\). Sin ella, solo se dispone de resistencia adicional aparente.

La compresibilidad puede analizarse mediante:

\[
\alpha=\alpha_0
\left(\frac{\Delta P_c}{\Delta P_0}\right)^s
\]

donde \(s\) es el exponente de compresibilidad y \(\Delta P_c\) la caída de presión atribuible a la torta, no necesariamente toda la TMP.

Realizar ciclos ascendentes y descendentes de presión sobre depósitos caracterizados ayuda a distinguir compresión reversible y fouling irreversible.

### Observación sobre el modelo Darcy

La corrección:

\[
J_{20}=J_T\frac{\mu_T}{\mu_{20}}
\]

es coherente para comparar permeabilidad a igual presión bajo las hipótesis del modelo. Añadiría:

- rango válido de temperatura;
- comprobación de resultados finitos;
- incertidumbre de TMP y flujo;
- diagnóstico de resistencias adicionales negativas.

No conviene truncar automáticamente valores negativos: pueden revelar error de calibración o un \(R_m\) de referencia inadecuado.

El \(K_{UF}\) nominal no debe asumirse como permeabilidad de agua del cartucho concreto. Como referencia matemática, el valor indicado produciría a 0,5 bar aproximadamente **456 mL/min y 12,44 LMH**; no constituye una garantía experimental.

*Fuente: pasted-text.txt, §§1.1, 3.D y 4.D.*

---

# E. Lavado y reproducibilidad de \(R_m\)

Recomiendo un protocolo gobernado por **recuperación de permeabilidad**, no solo por minutos de lavado:

1. **Caracterización inicial:** agua de calidad definida, temperatura medida y varios puntos de baja TMP.
2. **Enjuague entre corridas:** desplazar la suspensión y circular agua a flujo tangencial validado, con baja TMP, sin desmontar ni modificar la bomba.
3. **Criterio de finalización:** turbidez, conductividad y estabilidad hidráulica.
4. **Recalibración:** repetir la curva de permeabilidad del cartucho.
5. **Aceptación:** retorno dentro de una tolerancia predefinida, inicialmente ±5 % solo si la incertidumbre instrumental permite distinguirla.
6. **Si no recupera:** aplicar únicamente una limpieza compatible y validada, o retirar el cartucho de ese bloque experimental.

No recomendaría una receta genérica de hipoclorito, álcali, ácido, temperatura o contralavado basándome únicamente en “Helixone/polisulfona”. También importan encapsulado, adhesivos, conectores y uso previsto del dispositivo.

---

## Condiciones mínimas antes de habilitar ensayos a alta RPM

1. Verificar eléctricamente PUL/DIR y validar la secuencia de inversión.
2. Corregir rollover, primer pulso y temporización de diagnósticos.
3. Separar control y HTTP.
4. Incorporar presiones calibradas y protección con margen para incertidumbre, pulsaciones y retardo.
5. Validar también presión de entrada y caída axial, no únicamente TMP.
6. Ensayar pérdida de sensores, bloqueo de salidas, cliente HTTP lento y reinicio del ESP32.
7. Definir una parada segura y protección hidráulica independiente del software.

**Conclusión:** aprobaría la v4 como base para la siguiente etapa de validación, pero no como firmware ya cerrado. La prioridad ahora es demostrar temporización y seguridad hidráulica; después, estructurar la campaña experimental para no confundir permeabilidad aparente, flujo crítico y compresibilidad de torta.
