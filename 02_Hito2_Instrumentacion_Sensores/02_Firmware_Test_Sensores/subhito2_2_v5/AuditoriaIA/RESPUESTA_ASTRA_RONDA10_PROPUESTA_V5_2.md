**Rechazo la aprobación final de esta V5.2 para la campaña experimental tal como está.** Sí tiene sentido compilarla como parte de la depuración, pero compilar correctamente no demostraría que sea segura ni metrológicamente válida.

Revisé los diez archivos proporcionados mediante análisis estático; **no ejecuté una compilación ni pruebas sobre hardware**. Este dictamen es una revisión técnica, no una certificación profesional de la instalación. Encontré errores concretos que contradicen varias afirmaciones del expediente.

## 1. La calibración cargada desde NVS vuelve a marcarse como documentada

Este es un **hallazgo bloqueante de trazabilidad** y demuestra que la observación 3 de la matriz no está resuelta.

En `cargarParametrosNVS()` se ejecuta:

```cpp
sensorAlimentacion.setK(ka);
```

Esto correctamente pone `calibracionDocumentada = false`. Sin embargo, después, en `setup()`:

```cpp
sensorAlimentacion.declararCalibrado(SENSOR_ALIM_CFG.calibracionDocumentada);
```

Como esa configuración contiene `true` y todavía no se llamó a `begin()`, la declaración se acepta.

**Resultado:** cualquier K válido cargado de NVS queda marcado como calibración documentada, utilizando además el intervalo fijo de fábrica/configuración.

La solución no es solamente borrar esa llamada. Hace falta distinguir una configuración predeterminada de una calibración identificada, con sensor, intervalo, método, fecha, referencia e incertidumbre. En ausencia de esa evidencia, debe permanecer como **no documentada**.

## 2. Cambiar K modifica retroactivamente el volumen acumulado

En `capturar()`:

```cpp
m.volumenEstimado_L = (double)pulsosNetos / (60.0 * m.k);
```

La relación dimensional es correcta para un **K constante**, pero `setK()` permite cambiarlo durante la marcha.

Por ejemplo, con 11.790 pulsos netos:

- Con **K = 196,50**, el volumen calculado es 1 L.
- Al cambiar a **K = 393,00**, pasa a 0,5 L, sin cambiar los pulsos.

Esto altera los volúmenes de sesión y puede producir diferencias negativas entre inicio y cierre.

Además, `setK()` **no incrementa `_epoca`**. Aunque `caudalMedio_mLmin()` detecta factores diferentes en los extremos, no detectaría una secuencia K1 -\u003e K2 -\u003e K1 dentro del intervalo.

**Corrección requerida:** para esta etapa, la solución más sencilla es prohibir cambios de K durante un ensayo. Si se necesita calibración en caliente, hay que cerrar el segmento anterior, conservar su volumen, incrementar la época y comenzar otro segmento con el nuevo K.

## 3. La tara contiene una lectura de 64 bits sin protección

En `resetVolumen()`:

```cpp
_pulsosOffset = _pulsosMedicion;
```

`_pulsosMedicion` es un contador de 64 bits modificado por la ISR. Esa lectura está **fuera del spinlock**. `volatile` no garantiza atomicidad de una lectura de 64 bits en este contexto.

Las capturas principales sí utilizan correctamente el mismo `_mux`, pero esa excepción impide afirmar que no existen carreras. En ESP-IDF, la protección de recursos compartidos depende del uso consistente de las secciones críticas correspondientes. [Espressif, FreeRTOS](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html)

La lectura para la tara debe estar protegida. Además, **resetear volumen durante un ensayo debe rechazarse o registrarse como evento**, porque actualmente cambia la referencia del volumen sin dejar constancia en el CSV metrológico.

## 4. No están garantizados los tiempos de parada ni los interbloqueos

El cálculo ideal del frenado es correcto:

$$
t = \frac{100}{70} \approx 1,43\ \text{s}
$$

Pero eso **no garantiza una parada real inferior a 1,5 s**.

`bomba.tick()` comparte el bucle con atención HTTP, exportaciones y otras operaciones. Mientras una operación retrasa el bucle, el PWM hardware continúa con su configuración anterior. Un `dt` grande al regresar no elimina el tiempo que el motor ya siguió funcionando.

También encontré estas diferencias entre comentarios y comportamiento:

- **Sobrepresión:** `TMP_MAX_SEGURA_BAR` y `P1_MAX_SEGURA_BAR` no participan en ningún interbloqueo.
- **Fallo de alimentación:** se calcula `_falloAlimentacion`, pero el programa no lo utiliza para detener la bomba.
- **Emergencia:** la orden web depende de que se atienda HTTP; no constituye una parada física independiente.
- **Inicialización fallida:** si falla un caudalímetro, se imprime un error, pero no se inhibe el arranque.
- **Rearme:** `rearmarEmergencia()` no verifica una condición segura.
- **LEDC:** se ignoran los resultados de configuración. `ledcAttach()` puede fallar y `ledcChangeFrequency()` devuelve cero ante error, pero `_fActual` se actualiza igualmente. [Espressif, LEDC](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html)

Sin presión instrumentada, la campaña necesita **protección hidráulica independiente y límites de operación comprobados**. Las RPM o el caudal nominal no sustituyen una limitación de presión.

También queda por verificar eléctricamente la compatibilidad entre GPIO de 3,3 V y las entradas del **DM860 concreto**, incluyendo corriente de los optoacopladores, polaridad ENA y temporización DIR/PUL. El comentario del código no prueba esa compatibilidad.

## 5. El filtrado mejora la señal, pero no garantiza ausencia de caudal fantasma

Hay una **base razonable**: validación de niveles, separación del modo seco, conteo entero y muestras con indicadores de calidad. Sin embargo, las garantías del expediente son excesivas.

### Los umbrales no coinciden con lo declarado

| Parámetro | Declaración del expediente | Configuración efectiva |
|---|---|---|
| HIGH/LOW mínimos | 400/400 us | Alimentación: 200/200; permeado: 600/600 |
| Período mínimo | 1800 us | Alimentación: 1000; permeado: 2500 |

Debe existir una única especificación respaldada por ensayos.

### No tener períodos nuevos no demuestra caudal cero

Cuando no hay períodos nuevos, `m.q_mLmin` **permanece en `NAN`**. Lo que cae a cero es el indicador suavizado.

La distinción importa:

- **CSV metrológico:** conserva el dato no disponible.
- **SCADA:** muestra cero.
- **CSV operativo:** registra ese cero de visualización como caudal.

Por tanto, el mismo estado termina representado como ausencia de dato en un registro y como cero en el otro.

`hayPeriodosNuevos` evita reutilizar indefinidamente un período viejo, pero **no rechaza ruido que cumpla las condiciones geométricas**. Un tren de interferencia con anchos y períodos admisibles puede generar pulsos aceptados y volumen.

Además, el umbral de caudal se aplica después del conteo: poner el caudal en cero **no elimina esos pulsos del totalizador**.

### El RC necesita validación experimental

Con los valores externos indicados:

$$
\tau = RC = 4700 \times 200 \times 10^{-9}
= 0,94\ \text{ms}
$$

Con K = 196,50 y Q = 1360 mL/min, la frecuencia calculada es aproximadamente **267,24 Hz**, con período de 3,74 ms. El tiempo del RC no es despreciable respecto de ese período.

Esto no demuestra que el filtro falle, pero exige comprobar con osciloscopio amplitud, flancos y conteo en todo el rango, especialmente con el motor energizado.

## 6. No hay fundamento para transferir K al permeado como calibración

Que los sensores sean del mismo modelo **no demuestra que tengan el mismo factor real**, especialmente bajo condiciones hidráulicas diferentes.

El propio expediente atribuye K = 196,50 de alimentación al flujo peristáltico pulsátil. Esa explicación no justifica transferirlo al permeado ni extrapolarlo por debajo del rango nominal declarado.

También hay una inconsistencia: se menciona caracterización de alimentación entre **20 y 90 RPM**, pero se declara intervalo calibrado de **200 a 1360 mL/min**. Con 13,60 mL/rev, esas RPM corresponden nominalmente a 272-1224 mL/min. Se necesitan los datos reales para justificar los extremos.

La autocalibración presenta otros problemas:

- **Referencia no independiente:** ajusta K de alimentación al caudal teórico de la bomba; es un ajuste al modelo, no una calibración gravimétrica independiente.
- **Estabilidad insuficiente:** compara RPM internas calculadas, no velocidad medida ni estabilidad hidráulica.
- **Ventana discontinua:** si se pierde estabilidad, no reinicia los acumuladores; puede mezclar muestras de distintas consignas.
- **Calibración por probeta no sincronizada:** utiliza la frecuencia disponible al enviar el formulario, no necesariamente la del intervalo de recolección.

Para un volumen de referencia en litros, la calibración por conteo debe relacionar **los mismos límites temporales**:

$$
K = \frac{\Delta N}{60\,V_{\mathrm{ref}}}
$$

La resolución digital del contador no elimina la incertidumbre de detección, del patrón de volumen ni del método de recolección.

## 7. Los dos dataloggers no tienen sesiones equivalentes

La coexistencia de dos registros es posible, pero **la implementación actual no es coherente como expediente único**.

| Aspecto | Log operativo | Log metrológico |
|---|---|---|
| Columnas exportadas | 18, no 16 | 29 |
| Retención | Circular, 250 filas | Sin sobrescritura, 300 filas |
| Cambio de consigna | Puede abrir otro ensayo | No abre otra sesión |

Además:

- **Cierre prematuro:** el ensayo termina al cambiar `enMarcha` a falso, cuando la bomba todavía está frenando. El flujo residual no es necesariamente espurio; puede ser volumen físico real.
- **RPM incorrectas:** la fila metrológica inicial recibe la consigna, no las RPM actuales; la final recibe `0.0`, aunque la rampa de parada siga activa.
- **Resultados ignorados:** no se comprueba el éxito de `iniciar()`, `finalizar()` o `limpiar()`.
- **Borrado divergente:** `/clear_csv` borra el operativo aunque el metrológico rechace el borrado por sesión activa.
- **Exportación vacía con éxito HTTP:** `/export_metrologia` envía HTTP 200 antes de comprobar que `exportarCSV()` puede ejecutarse. Con sesión activa, devuelve `false`.
- **Pérdida no señalada:** el buffer circular puede sobrescribir muestras sin actualizar los resúmenes históricos.
- **TMP no exportada:** el `NAN` se asigna correctamente al campo interno `reg.tmp_bar`, pero ese campo no aparece en el CSV operativo; tampoco está en el metrológico.

La doble captura periódica sí fue corregida en `tick(ma, mp, rpm)`. No obstante, `iniciar()` y `finalizar()` todavía hacen capturas adicionales que consumen la ventana del estimador; conviene centralizar toda adquisición y registrar muestras o eventos bien definidos.

## 8. La interfaz tiene un error JavaScript comprobable

En la actualización del estado aparece:

```javascript
$('lbl_kp').innerText = d.k_perm.toFixed(2);
```

Pero **no existe un elemento con `id="lbl_kp"`** en el HTML adjunto.

Esto lanza una excepción y corta el resto de esa actualización: régimen, progreso de autocalibración, parámetros e historial. El `.catch(() =\u003e {})` oculta el error.

Debe agregarse el elemento o eliminarse la referencia. También hay que revisar las respuestas HTTP: varias funciones muestran éxito sin comprobar `r.ok` ni el contenido de `status`.

## 9. NVS y validación de parámetros requieren cambios

El almacenamiento **no está verificado**: se ignoran los resultados de `prefs.begin()` y de las escrituras, pero se informa éxito igualmente.

En `/set_dev` se aplican cambios parciales y hasta se pueden guardar antes de responder que hubo un error. La entrada `pul` se convierte a `uint16_t` antes de validar su rango; la cilindrada no tiene control de finitud ni límite superior.

La solución debe ser **validar primero y aplicar después**: todos los campos, rangos, compatibilidad entre parámetros y restricciones durante el ensayo. Solo entonces guardar una configuración versionada y comprobar el resultado.

Cambiar pulsos por revolución mientras gira la bomba también puede alterar bruscamente la frecuencia, aunque las RPM internas no cambien.

## 10. Condiciones mínimas para reconsiderar la aprobación

La versión debe superar **correcciones y pruebas**, no solamente otra revisión de comentarios:

1. **Compilación reproducible:** placa exacta, versión fija de Arduino-ESP32, dependencias y registro completo de compilación.
2. **Trazabilidad corregida:** NVS sin certificación automática, gestión de épocas y K, tara protegida y eventos documentados.
3. **Control seguro:** tratamiento de fallos LEDC y sensores, protección independiente y medición de tiempos de parada bajo carga de comunicaciones.
4. **Ensayos de adquisición:** generador de pulsos, ruido en seco con motor energizado, arranque, parada, pérdida de señal y límites de frecuencia.
5. **Calibración independiente:** cada sensor por separado, incluyendo permeado a bajo caudal, repeticiones, incertidumbre y criterio de aceptación.
6. **Integridad de registros:** correspondencia de sesiones, cierre durante frenado, agotamiento de memoria, exportación activa, borrado y pérdida de conexión.
7. **SCADA funcional:** corregir el elemento inexistente y mostrar por separado cero, dato no disponible, fallo y medición fuera de rango.

**Dictamen: NO APROBADO como firmware definitivo para campaña experimental.** La arquitectura es aprovechable y puede pasar a compilación de diagnóstico y pruebas de banco con el accionamiento aislado, pero los defectos de trazabilidad, registro y seguridad impiden dar la aprobación solicitada. Los nombres "certificado", "definitivo" o las atribuciones a auditorías anteriores no sustituyen evidencia de validación.1:[["$","$L39",null,{"experimentName":"$undefined","experimentValue":"$undefined","isOwner":false,"isPrivateInvite":false,"linkType":"public","sharedChat":{"id":"b2b9b815-e739-43b4-a984-24edf7e20cd8","originalChatId":"7c33b856-d046-4b65-9341-7322bc7f98ae","ownerUserId":"ELCKlaP9ws4A270MOmIUHecupqDlBGaQ","title":"Revisión Propuesta V5.2","messages":[{"id":"IuOiKinRALq73X7u","role":"user","parts":[{"type":"file","mediaType":"text/markdown","url":"https://files.use.ai/files/chat%2Ffiles%2F73c9d3ee-58dd-4bbd-90c4-5e6c5c147718-PROMPT_AUDITORIA_FINAL_V5_2_EXPEDIENTE_COMPLETO.md?token=1791912897826.zX7IDYQg5QGJ-HVj7hACh-Epz1YkQgb8sn1scBvypsg","filename":"PROMPT_AUDITORIA_FINAL_V5_2_EXPEDIENTE_COMPLETO.md"},{"type":"text","text":"revisa este prompt para ver si aceptas o rechazas lo que quiero compilar en ESP32 como ingeniero experto"}]},{"id":"fca1dd70-95e9-45eb-ad55-62d8db310d8e","role":"assistant","parts":[{"type":"step-start"},{"type":"tool-PARALLEL_SEARCH_WEB","toolCallId":"call_QirYUXWMM1yjgU5Bx3qDVzyQ","state":"output-available","input":{"objective":"Verify official Arduino ESP32 LEDC API 3 migration return failure frequency and ESP-IDF spinlock shared 64 bit critical sections relevant static firmware audit.","search_queries":["site.docs.espressif.com arduino esp32 ledcChangeFrequency return 0","site.docs.espressif.com esp32 freertos SMP critical sections shared data"]},"output":{"successful":true,"data":{"results":[{"url":"https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html","title":"LED Control (LEDC) -  -  — Arduino ESP32 latest documentation","publish_date":null,"excerpts":["* `pin` select LEDC pin.\
* `freq` select frequency of pwm.\
* `resolution` select resolution for LEDC channel.\
  + range is 1-14 bits (1-20 bits for ESP32).\
    This function will return `true` if configuration is successful.\
    If `false` is returned, error occurs and LEDC channel was not configured.\
\
...\
\
# LED Control (LEDC) \
## Arduino-ESP32 LEDC API \
### ledcChangeFrequency \
This function is used to set frequency for the LEDC pin.\
This updates the **timer** used by the pin. Every channel that shares that timer\
gets the new frequency and resolution. See Channels, timers and frequency sharing .\
```\
uint32_t ledcChangeFrequency ( uint8_t pin , uint32_t freq , uint8_t resolution );\
```\
\
...\
\
* `pin` select LEDC pin.\
* `freq` select frequency of pwm.\
* `resolution` select resolution for LEDC channel.\
  + range is 1-14 bits (1-20 bits for ESP32).\
    This function will return `frequency` configured for the LEDC channel.\
    If `0` is returned, error occurs and the LEDC channel frequency was not set."]},{"url":"https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html","title":"FreeRTOS (IDF) - ESP32 -  — ESP-IDF Programming Guide v6.1 documentation","publish_date":null,"excerpts":["# FreeRTOS (IDF) \
## Critical Sections \
### API Changes \
Vanilla FreeRTOS implements critical sections by disabling interrupts, which prevents preemptive context switches and the servicing of ISRs during a critical section. Thus a task/ISR that enters a critical section is guaranteed to be the sole entity to access a shared resource.\
\
...\
\
However, in an SMP system, merely disabling interrupts does not constitute a critical section as the presence of other cores means that a shared resource can still be concurrently accessed. Therefore, critical sections in IDF FreeRTOS are implemented using spinlocks.\
\
...\
\
* any `...ForCore(..., BaseType_t xCoreID)` SMP API will only accept `0` as a valid value for `xCoreID` .\
* `...PinnedToCore()` task creation APIs will simply ignore the `xCoreID` core affinity argument.\
* Critical section APIs will still require a spinlock argument, but no spinlock will be taken and critical sections revert to simply disabling/enabling interrupts."]},{"url":"https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/ledc.html","title":"LED Control (LEDC) - ESP32 -  — ESP-IDF Programming Guide v6.1 documentation","publish_date":null,"excerpts":["# LED Control (LEDC) \
## API Reference \
### Functions \
```\
LEDC channel configuration Configure LEDC channel with the given channel/output gpio\\_num/interrupt/source timer/frequency(Hz)/LEDC duty.\
```\
Parameters :\
**ledc_conf** -- Pointer of LEDC channel configure struct\
Returns :\
* ESP_OK Success\
* ESP_ERR_INVALID_ARG Parameter error\
\
...\
\
Parameters :\
* **speed_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.\
* **timer_num** -- LEDC timer index (0-3), select from ledc_timer_t\
* **freq_hz** -- Set the LEDC frequency\
  Returns :\
* ESP_OK Success\
Parameters :\
* **speed_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.\
* **timer_num** -- LEDC timer index (0-3), select from ledc_timer_t\
  Returns :\
* 0 error\
* Others Current LEDC frequency\
* ESP_ERR_INVALID_ARG Parameter error\
* ESP_ERR_INVALID_STATE Channel not initialized\
* ESP_FAIL Fade function init error"]},{"url":"https://docs.espressif.com/projects/arduino-esp32/en/latest/migration_guides/2.x_to_3.0.html","title":"Migration from 2.x to 3.0 -  -  — Arduino ESP32 latest documentation","publish_date":null,"excerpts":["# Migration from 2.x to 3.0 \
## Introduction \
This is a guide to highlight **breaking changes** in the API and build system to help the migration of projects from versions 2.X (based on ESP-IDF 4.4) to version 3.0 (based on ESP-IDF 5.1) of the Arduino ESP32 core.\
\
...\
\
## APIs \
### BLE \
#### Changes in APIs \
* Changed APIs return and parameter type from `std::string` to Arduino style `String` .\
* Changed UUID data type from `uint16_t` to `BLEUUID` class.\
* `BLEScan::start` and `BLEScan::getResults` methods return type changed from `BLEScanResults` to `BLEScanResults*` .\
\
...\
\
### RMT \
#### Changes in APIs \
* `rmtWrite` now sending data in blocking mode. It only returns after sending all data or through a timeout. For Async mode use the new `rmtWriteAsync` function.\
* `rmtWrite` new input parameter `uint32_t timeout_ms` .\
* `rmtLoop` renamed to `rmtWriteLooping` ."]},{"url":"https://docs.espressif.com/projects/esp-idf/en/v6.0.3/esp32/api-reference/peripherals/ledc.html","title":"LED Control (LEDC) - ESP32 -  — ESP-IDF Programming Guide v6.0.3 documentation","publish_date":null,"excerpts":["# LED Control (LEDC) \
## API Reference \
### Functions \
```\
LEDC channel configuration Configure LEDC channel with the given channel/output gpio\\_num/interrupt/source timer/frequency(Hz)/LEDC duty.\
```\
Parameters :\
**ledc_conf** -- Pointer of LEDC channel configure struct\
Returns :\
* ESP_OK Success\
* ESP_ERR_INVALID_ARG Parameter error\
\
...\
\
Parameters :\
* **speed_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.\
* **timer_num** -- LEDC timer index (0-3), select from ledc_timer_t\
* **freq_hz** -- Set the LEDC frequency\
  Returns :\
* ESP_OK Success\
Parameters :\
* **speed_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.\
* **timer_num** -- LEDC timer index (0-3), select from ledc_timer_t\
  Returns :\
* 0 error\
* Others Current LEDC frequency\
* ESP_ERR_INVALID_ARG Parameter error\
* ESP_ERR_INVALID_STATE Channel not initialized\
* ESP_FAIL Fade function init error"]},{"url":"https://docs.espressif.com/projects/esp-idf/en/v5.1/esp32/api-reference/peripherals/ledc.html","title":"LED Control (LEDC) - ESP32 -  — ESP-IDF Programming Guide v5.1 documentation","publish_date":null,"excerpts":["# LED Control (LEDC) \
## API Reference \
### Functions \
```\
LEDC channel configuration Configure LEDC channel with the given channel/output gpio\\_num/interrupt/source timer/frequency(Hz)/LEDC duty resolution.\
```\
Parameters\
**ledc_conf** – Pointer of LEDC channel configure struct\
Returns\
* ESP_OK Success\
* ESP_ERR_INVALID_ARG Parameter error\
Parameters\
**timer_conf** – Pointer of LEDC timer configure struct\
Returns\
* ESP_OK Success\
* ESP_ERR_INVALID_ARG Parameter error\
* ESP_FAIL Can not find a proper pre-divider number base on the given frequency and the current duty_resolution.\
\
...\
\
Parameters\
* **speed_mode** – Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.\
* **timer_num** – LEDC timer index (0-3), select from ledc_timer_t\
  Returns\
* 0 error\
* Others Current LEDC frequency"]},{"url":"https://docs.espressif.com/projects/esp-idf/en/v5.0.2/esp32/api-reference/peripherals/ledc.html","title":"LED Control (LEDC) - ESP32 -  — ESP-IDF Programming Guide v5.0.2 documentation","publish_date":null,"excerpts":["# LED Control (LEDC) \
## API Reference \
### Functions \
```\
LEDC channel configuration Configure LEDC channel with the given channel/output gpio\\_num/interrupt/source timer/frequency(Hz)/LEDC duty resolution.\
```\
Parameters\
**ledc_conf** – Pointer of LEDC channel configure struct\
Returns\
* ESP_OK Success\
* ESP_ERR_INVALID_ARG Parameter error\
Parameters\
**timer_conf** – Pointer of LEDC timer configure struct\
Returns\
* ESP_OK Success\
* ESP_ERR_INVALID_ARG Parameter error\
* ESP_FAIL Can not find a proper pre-divider number base on the given frequency and the current duty_resolution.\
\
...\
\
Parameters\
* **speed_mode** – Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.\
* **timer_num** – LEDC timer index (0-3), select from ledc_timer_t\
  Returns\
* 0 error\
* Others Current LEDC frequency"]},{"url":"https://docs.espressif.com/projects/esp-idf/en/v4.3.4/esp32/api-guides/freertos-smp.html","title":"ESP-IDF FreeRTOS SMP Changes - ESP32","publish_date":null,"excerpts":["# ESP-IDF FreeRTOS SMP Changes ¶\
## Overview ¶\
Critical Sections \u0026 Disabling Interrupts : In ESP-IDF FreeRTOS, critical sections are implemented using\
mutexes. Entering critical sections involve taking a mutex, then disabling the\
scheduler and interrupts of the calling core. However the other core is left\
unaffected. If the other core attemps to take same mutex, it will spin until\
\
...\
\
## Critical Sections \u0026 Disabling Interrupts ¶\
For this reason, ESP-IDF FreeRTOS implements critical sections using special\
mutexes, referred by `portMUX_Type` objects. These are implemented on top of a\
specific spinlock component. Calls to `taskENTER_CRITICAL` or `taskEXIT_CRITICAL` each provide a spinlock object as an argument. The\
\
...\
\
## Configuring ESP-IDF FreeRTOS ¶\
```\
E (25) FreeRTOS: FreeRTOS task should not return. Aborting now!\
abort() was called at PC 0x40085c53 on core 0\
```"]},{"url":"https://docs.espressif.com/projects/esp-idf/en/v5.0.7/esp32/api-reference/peripherals/ledc.html","title":"LED Control (LEDC) - ESP32 -  — ESP-IDF Programming Guide v5.0.7 documentation","publish_date":null,"excerpts":["# LED Control (LEDC) \
## API Reference \
### Functions \
```\
LEDC channel configuration Configure LEDC channel with the given channel/output gpio\\_num/interrupt/source timer/frequency(Hz)/LEDC duty resolution.\
```\
Parameters\
**ledc_conf** – Pointer of LEDC channel configure struct\
Returns\
* ESP_OK Success\
* ESP_ERR_INVALID_ARG Parameter error\
Parameters\
**timer_conf** – Pointer of LEDC timer configure struct\
Returns\
* ESP_OK Success\
* ESP_ERR_INVALID_ARG Parameter error\
* ESP_FAIL Can not find a proper pre-divider number base on the given frequency and the current duty_resolution.\
\
...\
\
Parameters\
* **speed_mode** – Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.\
* **timer_num** – LEDC timer index (0-3), select from ledc_timer_t\
  Returns\
* 0 error\
* Others Current LEDC frequency"]},{"url":"https://docs.espressif.com/projects/esp-idf/en/v5.2.4/esp32/api-reference/peripherals/ledc.html","title":"LED Control (LEDC) - ESP32 -  — ESP-IDF Programming Guide v5.2.4 documentation","publish_date":null,"excerpts":["# LED Control (LEDC) \
## Supported Range of Frequency and Duty Resolutions \
```\
E (196) ledc: requested frequency and duty resolution cannot be achieved, try reducing freq_hz or duty_resolution. div_param=128\
```\
\
...\
\
## API Reference \
### Functions \
```\
LEDC channel configuration Configure LEDC channel with the given channel/output gpio\\_num/interrupt/source timer/frequency(Hz)/LEDC duty.\
```\
Parameters\
**ledc_conf** -- Pointer of LEDC channel configure struct\
Returns\
* ESP_OK Success\
* ESP_ERR_INVALID_ARG Parameter error\
\
...\
\
Parameters\
* **speed_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.\
* **timer_num** -- LEDC timer index (0-3), select from ledc_timer_t\
  Returns\
* 0 error\
* Others Current LEDC frequency"]},{"url":"https://docs.espressif.com/projects/esp-idf/en/v4.4.8/esp32/api-reference/peripherals/ledc.html","title":"LED Control (LEDC) - ESP32 -  — ESP-IDF Programming Guide v4.4.8 documentation","publish_date":null,"excerpts":["Note, however, that these parameters depend on the clock signal clocking the LED PWM Controller timer which in turn clocks the channel (see timer configuration and the ESP32 Technical Reference Manual \u003e LED PWM Controller (LEDC) [ PDF ]).\
Parameters * speed_mode – Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.\
* timer_num – LEDC timer index (0-3), select from ledc_timer_t\
Returns * 0 error\
* Others Current LEDC frequency\
LEDC fade function will block until fading to the target duty\
enumerator LEDC_FADE_MAX \
[Provide feedback about this document](https://www.espressif.com/en/company/documents/documentation_feedback?docId=4287\u0026sections=LED Control (LEDC) (api-reference/peripherals/ledc)\u0026version=esp32 v4.4.8 (v4.4.8))\
Previous Next"]},{"url":"https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/freertos.html","title":"FreeRTOS Overview - ESP32-S3 -  — ESP-IDF Programming Guide v6.1 documentation","publish_date":null,"excerpts":["FreeRTOS Overview - ESP32-S3 - — ESP-IDF Programming Guide v6 ...\
ESP-IDF FreeRTOS is a FreeRTOS implementation based on Vanilla FreeRTOS v10.5.1, but contains significant modifications to support SMP. ESP-IDF FreeRTOS only supports two cores at most (i.e., dual core SMP), but is more optimized for this scenario by design."]}],"citations":[{"url":"https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html","title":"LED Control (LEDC) -  -  — Arduino ESP32 latest documentation","snippet":"* `pin` select LEDC pin.\
* `freq` select frequency of pwm.\
* `resolution` select resolution for LEDC channel.\
  + range is 1-14 bits (1-20 bits for ESP32).\
    This function will return `true` if configuration is successful.\
    If `false` is returned, error occurs and LEDC channel was not configure","favicon":null},{"url":"https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html","title":"FreeRTOS (IDF) - ESP32 -  — ESP-IDF Programming Guide v6.1 documentation","snippet":"# FreeRTOS (IDF) \
## Critical Sections \
### API Changes \
Vanilla FreeRTOS implements critical sections by disabling interrupts, which prevents preemptive context switches and the servicing of ISRs during a critical section. Thus a task/ISR that enters a critical section is guaranteed to be the so","favicon":null},{"url":"https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/ledc.html","title":"LED Control (LEDC) - ESP32 -  — ESP-IDF Programming Guide v6.1 documentation","snippet":"# LED Control (LEDC) \
## API Reference \
### Functions \
```\
LEDC channel configuration Configure LEDC channel with the given channel/output gpio\\_num/interrupt/source timer/frequency(Hz)/LEDC duty.\
```\
Parameters :\
**ledc_conf** -- Pointer of LEDC channel configure struct\
Returns :\
* ESP_OK Succes","favicon":null},{"url":"https://docs.espressif.com/projects/arduino-esp32/en/latest/migration_guides/2.x_to_3.0.html","title":"Migration from 2.x to 3.0 -  -  — Arduino ESP32 latest documentation","snippet":"# Migration from 2.x to 3.0 \
## Introduction \
This is a guide to highlight **breaking changes** in the API and build system to help the migration of projects from versions 2.X (based on ESP-IDF 4.4) to version 3.0 (based on ESP-IDF 5.1) of the Arduino ESP32 core.\
\
...\
\
## APIs \
### BLE \
#### Cha","favicon":null},{"url":"https://docs.espressif.com/projects/esp-idf/en/v6.0.3/esp32/api-reference/peripherals/ledc.html","title":"LED Control (LEDC) - ESP32 -  — ESP-IDF Programming Guide v6.0.3 documentation","snippet":"# LED Control (LEDC) \
## API Reference \
### Functions \
```\
LEDC channel configuration Configure LEDC channel with the given channel/output gpio\\_num/interrupt/source timer/frequency(Hz)/LEDC duty.\
```\
Parameters :\
**ledc_conf** -- Pointer of LEDC channel configure struct\
Returns :\
* ESP_OK Succes","favicon":null},{"url":"https://docs.espressif.com/projects/esp-idf/en/v5.1/esp32/api-reference/peripherals/ledc.html","title":"LED Control (LEDC) - ESP32 -  — ESP-IDF Programming Guide v5.1 documentation","snippet":"# LED Control (LEDC) \
## API Reference \
### Functions \
```\
LEDC channel configuration Configure LEDC channel with the given channel/output gpio\\_num/interrupt/source timer/frequency(Hz)/LEDC duty resolution.\
```\
Parameters\
**ledc_conf** – Pointer of LEDC channel configure struct\
Returns\
* ESP_OK ","favicon":null},{"url":"https://docs.espressif.com/projects/esp-idf/en/v5.0.2/esp32/api-reference/peripherals/ledc.html","title":"LED Control (LEDC) - ESP32 -  — ESP-IDF Programming Guide v5.0.2 documentation","snippet":"# LED Control (LEDC) \
## API Reference \
### Functions \
```\
LEDC channel configuration Configure LEDC channel with the given channel/output gpio\\_num/interrupt/source timer/frequency(Hz)/LEDC duty resolution.\
```\
Parameters\
**ledc_conf** – Pointer of LEDC channel configure struct\
Returns\
* ESP_OK ","favicon":null},{"url":"https://docs.espressif.com/projects/esp-idf/en/v4.3.4/esp32/api-guides/freertos-smp.html","title":"ESP-IDF FreeRTOS SMP Changes - ESP32","snippet":"# ESP-IDF FreeRTOS SMP Changes ¶\
## Overview ¶\
Critical Sections \u0026 Disabling Interrupts : In ESP-IDF FreeRTOS, critical sections are implemented using\
mutexes. Entering critical sections involve taking a mutex, then disabling the\
scheduler and interrupts of the calling core. However the other core i","favicon":null},{"url":"https://docs.espressif.com/projects/esp-idf/en/v5.0.7/esp32/api-reference/peripherals/ledc.html","title":"LED Control (LEDC) - ESP32 -  — ESP-IDF Programming Guide v5.0.7 documentation","snippet":"# LED Control (LEDC) \
## API Reference \
### Functions \
```\
LEDC channel configuration Configure LEDC channel with the given channel/output gpio\\_num/interrupt/source timer/frequency(Hz)/LEDC duty resolution.\
```\
Parameters\
**ledc_conf** – Pointer of LEDC channel configure struct\
Returns\
* ESP_OK ","favicon":null},{"url":"https://docs.espressif.com/projects/esp-idf/en/v5.2.4/esp32/api-reference/peripherals/ledc.html","title":"LED Control (LEDC) - ESP32 -  — ESP-IDF Programming Guide v5.2.4 documentation","snippet":"# LED Control (LEDC) \
## Supported Range of Frequency and Duty Resolutions \
```\
E (196) ledc: requested frequency and duty resolution cannot be achieved, try reducing freq_hz or duty_resolution. div_param=128\
```\
\
...\
\
## API Reference \
### Functions \
```\
LEDC channel configuration Configure LED","favicon":null},{"url":"https://docs.espressif.com/projects/esp-idf/en/v4.4.8/esp32/api-reference/peripherals/ledc.html","title":"LED Control (LEDC) - ESP32 -  — ESP-IDF Programming Guide v4.4.8 documentation","snippet":"Note, however, that these parameters depend on the clock signal clocking the LED PWM Controller timer which in turn clocks the channel (see timer configuration and the ESP32 Technical Reference Manual \u003e LED PWM Controller (LEDC) [ PDF ]).\
Parameters * speed_mode – Select the LEDC channel group with ","favicon":null},{"url":"https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/freertos.html","title":"FreeRTOS Overview - ESP32-S3 -  — ESP-IDF P