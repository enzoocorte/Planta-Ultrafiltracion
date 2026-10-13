l ensayo y la falta de comprobación de la interfaz eléctrica del DM860.

A continuación separo el dictamen, los hallazgos y una **variante reducida de banco** que permite probar adquisición y calibración sin arrastrar las funciones problemáticas del SCADA. El código propuesto está revisado por inspección; **no lo he compilado ni ejecutado en un ESP32**, por lo que no lo presentaré como firmware certificado o experimentalmente aprobado.

## 1. Respuestas a las cuatro preguntas del expediente

| Pregunta | Dictamen | Fundamento |
|---|---|---|
| Concurrencia | Aceptable con condiciones | Los contadores compartidos están protegidos. No toda la clase es segura para llamadas concurrentes desde múltiples tareas. |
| SCADA y REST | No aprobado | Hay mensajes falsos de éxito, escrituras no comprobadas y rutas que eluden los bloqueos. |
| Sesiones y trazabilidad | Parcialmente correcto | La reserva de FIN funciona, pero se cierra durante el frenado y existen dos sistemas de sesiones incompatibles. |
| Compilación y campaña | Separar etapas | Compilar y probar sin accionamiento: sí. Campaña con V5.2 sin cambios: no. |

Esta es una **revisión técnica**, no una autorización institucional ni una certificación de seguridad. La habilitación física del banco corresponde al responsable del laboratorio.

## 2. Revisión de los diez hallazgos declarados como resueltos

La matriz del expediente no puede darse por cerrada íntegramente.

| Hallazgo original | Estado observado | Observación |
|---|---|---|
| 1. `lbl_kp` inexistente | Corregido | El elemento está en el HTML. |
| 2. Lectura de 64 bits en tara | Corregido localmente | La copia de `_pulsosMedicion` ahora está protegida. |
| 3. Calibración documentada pisada en `setup()` | Corregido | Ya no se vuelve a declarar documentada después de cargar K. |
| 4. Cambio de K y época | Incompleto | Cambia época, pero reinterpreta volumen anterior; otras rutas siguen cambiando K en marcha. |
| 5. Validación previa de endpoints | Incompleto | `/set_dev` mejora; `/calibrar_rpm_q` no valida todos los resultados antes de modificar. |
| 6. Framing del CSV metrológico | Corregido en estructura | El adaptador usa `sendContent`, pero no informa fallos reales de transmisión. |
| 7. Exportación con sesión activa | Corregido localmente | Existe HTTP 409, pero sesión inactiva no significa motor parado. |
| 8. Errores LEDC | Incompleto | No se actualiza `_fActual` si falla frecuencia, pero se sigue intentando aplicar duty y no se declara fallo. |
| 9. RPM en eventos | Parcial | Registra el estado de la rampa, no la velocidad medida del eje. FIN sigue adelantado. |
| 10. Bajo flujo sin nuevos períodos | Correcto con matiz | La muestra contiene `NaN`; el indicador se vuelve cero. No son equivalentes metrológicos. |

## 3. Hallazgos que impiden aprobar la V5.2 completa

### 3.1. El bloqueo de reconfiguración tiene vías de escape

**Se puede modificar K y escribir NVS durante un ensayo activo.**

Ocurre por dos caminos:

- **`handleResetDev()`** no comprueba `registroEnsayos.activo()`.
- **La finalización de auto-calibración** llama a `setK()` y `guardarParametrosNVS()` mientras la bomba sigue en marcha.
- **El estado de sesión puede no existir**, porque se ignora el retorno de `registroEnsayos.iniciar()`.

El tercer punto importa especialmente: cuando se agotan las 20 sesiones, falla la reserva o no se inicializa un sensor, la bomba puede funcionar sin sesión metrológica. Los bloqueos basados exclusivamente en `activo()` dejan entonces de proteger el funcionamiento.

**Corrección necesaria:** prohibir la reconfiguración si hay sesión, movimiento, arranque pendiente, frenado o inversión; comprobar siempre el inicio del registro. Para la primera campaña recomiendo directamente desactivar auto-calibración, reset de fábrica y cambios en caliente.

### 3.2. `setK()` todavía cambia retrospectivamente el volumen

Actualmente se calcula:

```cpp
m.volumenEstimado_L = (double)pulsosNetos / (60.0 * m.k);
```

Pero `setK()` no cierra el totalizador ni modifica su referencia. Por tanto, todos los pulsos anteriores a la nueva K se convierten usando la K nueva.

**Ejemplo calculado:** con 11 790 pulsos y K = 196,5, se obtiene 1 L. Si después K pasa a 200, el mismo acumulado se muestra como 0,9825 L, aunque no haya circulado agua adicional.

Incrementar `_epoca` protege el cálculo entre muestras de épocas distintas, pero **no corrige esa reinterpretación**.

Hay dos soluciones válidas, que deben documentarse:

- **Volumen por época:** al cambiar K, iniciar un volumen nuevo desde cero.
- **Volumen histórico:** cerrar el tramo anterior con su K y sumar después los incrementos del nuevo tramo.

Para la campaña inicial conviene la primera, por simplicidad.

### 3.3. `/calibrar_rpm_q` no implementa una calibración sincronizada

El endpoint utiliza el caudal introducido por el operador y la **última frecuencia disponible**, no la frecuencia del intervalo durante el cual se recogió el volumen.

Además:

- **Rechaza la calibración con sesión activa**, que es precisamente cuando hay flujo y frecuencia actualizada.
- **Después de parar**, la frecuencia puede ser residual o cero.
- **Puede modificar únicamente la cilindrada** y responder éxito aunque no haya actualizado ninguna K.
- **No rechaza `qp` negativo**.
- **No valida previamente los límites de las K calculadas** con la misma política de `/set_dev`.
- **No comprueba los retornos** de `setK()` ni de persistencia.

No se resuelve añadiendo solamente más límites numéricos.

La calibración por volumen debe utilizar el incremento de pulsos **del mismo intervalo de recogida**:

$$
K=\frac{\Delta N}{60\,V_{\mathrm{ref,L}}}
$$

O, si el volumen se introduce en mililitros:

$$
K=\frac{1000\,\Delta N}{60\,V_{\mathrm{ref,mL}}}
$$

El tiempo permite calcular el caudal de referencia:

$$
Q_{\mathrm{ref,mL/min}}=
\frac{60\,V_{\mathrm{ref,mL}}}{\Delta t_{\mathrm{s}}}
$$

**Para obtener K por volumen, no hace falta suponer un caudal constante ni usar las RPM como patrón.** Sí hace falta sincronizar los límites de recogida y conteo.

### 3.4. La auto-calibración no es una referencia independiente

La rutina ajusta K usando:

```cpp
float qRefAlim = bomba.rpmActual() * bomba.getMlPorVuelta();
```

Eso es un **ajuste contra el modelo de la bomba**, no una calibración independiente por probeta o balanza.

Puede ser una comprobación útil después de caracterizar la bomba, pero no debe confundirse con establecer la exactitud del caudalímetro.

Además, las 15 muestras:

- No son necesariamente **15 segundos consecutivos**.
- Pueden pertenecer a **distintas consignas**, porque las sumas no se reinician al perder estabilidad.
- Pueden incorporar frecuencias nulas por falta de período disponible.
- Se contrastan con el caudal teórico **del último instante**.

La interfaz también dice que calibra ambos sensores, aunque el código deja K de permeado intacta.

**Para la primera campaña: desactivar esta función.**

### 3.5. FIN se registra antes de que termine el movimiento

`detener()` cambia inmediatamente `_enMarcha` a `false`. Después, el primer `tick()` detecta el flanco de bajada y finaliza la sesión, aunque la rampa todavía esté frenando.

A 100 RPM, con una actualización de aproximadamente 50 ms, el primer descenso nominal sería:

$$
100-70\cdot0{,}05=96{,}5\ \text{RPM}
$$

Es decir, **FIN puede quedar registrado con casi toda la velocidad inicial**.

Además se pasa `bombaEmpuja=false`, aunque todavía se están emitiendo pulsos. Eso puede activar `SIN_IMPULSION` incorrectamente.

Hay que distinguir:

- **Fin del intervalo de recogida:** decisión metrológica.
- **Orden STOP:** decisión de control.
- **Fin de emisión STEP:** condición eléctrica.
- **Fin del flujo residual:** condición hidráulica.

No tienen por qué coincidir. El firmware debe registrar cuál de ellos define FIN.

### 3.6. Las RPM no son una medida del eje

`rpmActual()` devuelve `_actual`, una variable de la rampa.

No existe encoder ni tacómetro. Por tanto, no se detectan pérdida de pasos, atasco, desacoplamiento ni error de microstepping.

Incluso como velocidad comandada hay discrepancias:

- Se redondea la frecuencia a entero.
- Se impone un mínimo de 50 Hz.
- La actualización de LEDC puede fallar.
- El número configurado de pulsos por vuelta puede no coincidir con los DIP físicos.

**Ejemplo calculado:** con 200 pulsos/vuelta, 50 Hz equivalen a 15 RPM. La variable de rampa podría estar indicando aproximadamente 1 RPM durante ese arranque.

Recomiendo renombrar el dato como **`rpm_comandada`** o **`rpm_equivalente_STEP`**. Solo debe llamarse velocidad real si se mide.

### 3.7. El manejo de errores LEDC no es suficiente

En `begin()` no se comprueban todos los resultados. En `tick()`, si falla la frecuencia, se sigue con `ledcWrite(..., 512)`.

La API documenta que `ledcAttach()` y `ledcWrite()` pueden fallar, y que `ledcChangeFrequency()` devuelve cero cuando no configura la frecuencia. Esos retornos tienen que afectar al estado operativo, no únicamente a `_fActual`. [Espressif LEDC](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html)

**Respuesta requerida ante fallo:** solicitar inhibición, cortar STEP, enclavar un estado de error y no seguir anunciando régimen estable.

### 3.8. No se ha demostrado la compatibilidad GPIO-DM860

**La conexión directa de GPIO de 3,3 V a las entradas del DM860 no queda validada por el expediente.**

Un manual de DM860 consultado especifica señales nominales de 5 V y corriente de entrada de 7 a 16 mA. Otra variante, DM860T V3.0, exige niveles de 4,5-5 V o 24 V y documenta tiempos particulares de habilitación. No corresponde extrapolar automáticamente estas especificaciones a tu unidad, pero sí muestran por qué hace falta identificar el driver exacto. [Manual DM860](https://uelectronics.com/wp-content/uploads/2020/12/DMA860H-Driver-Manual.pdf?srsltid=AU7gw4WZrKw-4XKNUxmGyUGXsYcwXkNyvG_QVt5Gwd1eJes0OLJE77jx) [Manual DM860T V3.0](https://www.omc-stepperonline.com/download/DM860T_V3.0.pdf)

Antes de accionar:

- **Identificar modelo y fabricante exactos**, incluyendo configuración de entrada y microstepping.
- **Verificar nivel y corriente** en PUL, DIR y ENA; instalar interfaz adaptadora si corresponde.
- **Comprobar físicamente la polaridad de ENA**, sin deducirla solo del nombre de los bornes.
- **Verificar el estado durante encendido y reset**, cuando el firmware todavía no controla los GPIO.
- **Disponer de parada física independiente** del navegador y del programa.

Poner duty cero significa **dejar de enviar pasos**. No significa necesariamente liberar el motor: el driver puede mantener corriente y par de retención.

### 3.9. No están demostrados los tiempos garantizados de parada

La rampa está atendida después de:

```cpp
dnsServer.processNextRequest();
server.handleClient();
```

Una respuesta HTTP lenta puede retrasar el control. El periférico LEDC continúa generando pulsos mientras el programa atiende otras tareas.

`yield()` no ejecuta automáticamente `bomba.tick()`.

Por tanto:

- **70 RPM/s** es una pendiente nominal del algoritmo.
- **Menos de 1,5 s** no está garantizado desde el clic del operador.
- **Menos de 1 ms** no está demostrado para el sistema completo.
- La detección `_falloAlimentacion` es **diagnóstica**: no está conectada a un interbloqueo.

Para un banco supervisado no hace falta convertir el proyecto en un sistema industrial, pero sí retirar las garantías no medidas y no depender del Wi-Fi para detenerlo.

## 4. Concurrencia, metrología y registro: revisión adicional

### 4.1. Qué está bien en los spinlocks

**La protección principal de los contadores es adecuada para la arquitectura actual.**

La ISR modifica sus variables bajo `_mux`; `capturar()` extrae los contadores y consume la ventana bajo el mismo bloqueo. `resetVolumen()` ahora toma la referencia de 64 bits correctamente.

Hay tres límites:

- **`_pulsosOffset` no lo escribe la ISR.** Su lectura posterior fuera del bloqueo no constituye por sí sola una carrera si todas las operaciones de usuario ocurren en la misma tarea.
- **`_ultimaMuestra` y los getters no son una interfaz multitarea segura.** No deben usarse desde otras tareas mientras se actualizan sin añadir sincronización.
- **Las dos muestras no son simultáneas.** Se captura alimentación y después permeado; sus marcas temporales permiten conservar esa diferencia.

No corresponde afirmar que toda la clase es universalmente thread-safe.

### 4.2. Rutina llamada desde la ISR

`reiniciarEstimadorLocked()` se llama desde la ISR, pero no tiene el atributo ISR que sí tienen `isr()` e `isrPuente()`.

**No es un fallo demostrado en todas las compilaciones:** depende de ubicación, optimización y configuración. Sin embargo, no conviene depender de que el compilador la integre.

Añadir el atributo y comprobar el mapa de enlace es una corrección razonable, especialmente si se permiten operaciones Flash.

### 4.3. El umbral mínimo no elimina pulsos del totalizador

**`qMinDetectable` es un umbral del resultado de caudal**, no un filtro del contador.

Los pulsos aceptados ya incrementaron `_pulsosMedicion` antes de aplicar el umbral. Por eso puede haber:

- Caudal mostrado igual a cero.
- Volumen que sigue creciendo.

Esto puede ser una decisión correcta para no borrar datos brutos, pero el comentario "rechazo de ruido" resulta engañoso. La validación temporal de la ISR tampoco distingue un pulso hidráulico de una perturbación eléctrica con geometría válida.

### 4.4. Ausencia de nuevos períodos no equivale a caudal nulo

**Conservar `NaN` en la muestra es preferible a inventar un cero medido.**

El problema está en el SCADA, que transforma falta de dato en cero y no muestra suficientemente la calidad. Para bajo flujo, el valor analítico principal debería ser el caudal medio por incremento de pulsos sobre una ventana suficientemente larga.

La fórmula de `caudalMedio_mLmin()` es dimensionalmente correcta.

### 4.5. El filtro RC necesita ensayo, no aprobación por componentes

Para 4,7 kΩ y 200 nF:

$$
\tau=0{,}94\ \text{ms}
$$

La frecuencia de corte ideal de primer orden es aproximadamente:

$$
f_c=169\ \text{Hz}
$$

Con K = 196,5 y 1 360 mL/min:

$$
f=267{,}24\ \text{Hz}
$$

**No significa automáticamente que el contador falle**, porque intervienen forma de onda, duty, salida del sensor y umbrales del ESP32. Sí significa que el filtro afecta significativamente la forma de los pulsos en la zona superior.

Es necesario comprobar con osciloscopio o analizador, antes y después del filtro:

- **Niveles eléctricos**, sin superar la tensión admisible del GPIO.
- **Ancho HIGH/LOW** frente a los mínimos del software.
- **Pulsos emitidos frente a aceptados** a varias frecuencias y duty cycles.

### 4.6. El registrador metrológico es útil, pero no está integrado de forma segura

La reserva para FIN está bien planteada. Sin embargo:

- **Los retornos de `iniciar()` y `finalizar()` se ignoran**.
- **El fallo de reserva RAM no bloquea la prueba**.
- **Cambiar RPM segmenta solo el registrador antiguo**, no `RegistroEnsayos`.
- **La tara durante una sesión rompe la continuidad de `vA_L` y `vP_L`**, aunque el contador bruto permanezca utilizable.
- **La inversión no queda registrada en el CSV metrológico**.
- **El CSV de 29 columnas no es el mismo registro que el mostrado en el historial**.

Hay que usar **un solo registro como fuente oficial de la campaña**.

### 4.7. NVS: distinguir corrupción de inconsistencia

No veo una razón suficiente para afirmar que estas funciones necesariamente corrompan físicamente NVS. Sí hay riesgo de **configuración parcialmente actualizada y falsa confirmación**.

Las cuatro escrituras son independientes, no una transacción de aplicación única. Se ignoran sus retornos y el de `prefs.begin()`. La documentación permite comprobar éxito: por ejemplo, `putFloat()` devuelve 4 bytes y `putUShort()` 2 bytes cuando funcionan. [Espressif Preferences](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/preferences.html)

Para la primera campaña, **eliminar escrituras Flash durante la adquisición** es más sencillo que diseñar ahora una persistencia transaccional completa.

### 4.8. SCADA: corregir `lbl_kp` no garantiza ausencia de errores

**`fetch()` no rechaza automáticamente HTTP 400 o 409.** Varias funciones leen el JSON y muestran éxito aunque el servidor haya contestado error.

Además:

- **`catch(() =\u003e {})` oculta la pérdida de telemetría** y deja valores antiguos visibles.
- **Truncar `snprintf()` puede producir JSON inválido**; imprimir una advertencia no lo repara.
- **Los mensajes insertados en JSON necesitan escape**.
- **No se muestra el enclavamiento de emergencia**, aunque el servidor lo envía.
- **Las órdenes desconocidas reciben `OK`**.
- **No hay autenticación de comandos a nivel de aplicación**; cualquier cliente con acceso al AP puede operar.
- **Las solicitudes periódicas pueden solaparse**.

Para el banco, mostrar explícitamente **telemetría vencida**, fallos y respuestas rechazadas es más importante que añadir más controles gráficos.

### 4.9. Darcy y documentación

**Es correcto dejar Darcy inactivo sin TMP medida.** No lo considero un impedimento para calibrar los caudalímetros.

Quedan correcciones documentales y de alcance:

- La ley implementada de aceleración **no demuestra una S-curve cuadrática con jerk controlado**.
- Las constantes de presión **no implementan interbloqueos**.
- La referencia nominal de membrana **no sustituye la caracterización hidráulica con agua**.
- El CSV tradicional tiene **18 columnas**, no 16.
- El registrador tiene **300 filas**, no las 600 mencionadas en su encabezado.
- "Certificado", "validado" y "garantizado" deben reservarse para condiciones con evidencia.
- La recirculación abierta **no demuestra por sí sola presión nula aguas arriba**: siguen existiendo pérdidas de carga y posibles obstrucciones.

## 5. Correcciones pequeñas al núcleo reutilizable

Estas modificaciones se aplican a `caudalimetro.h` y `caudalimetro.cpp`. La política elegida es **volumen nuevo por época de K**, conservando los contadores brutos.

### 5.1. Atributo de la función usada por la ISR

En `caudalimetro.h`, sustituir la declaración privada:

```cpp
void ARDUINO_ISR_ATTR reiniciarEstimadorLocked();
```

En `caudalimetro.cpp`, conservar el cuerpo y modificar la firma:

```cpp
void ARDUINO_ISR_ATTR Caudalimetro::reiniciarEstimadorLocked() {
  _altoConocido = false;
  _candidato = false;
  _hayAnterior = false;
  _bajada_us = 0;
  _bajadaAnterior_us = 0;
  _ultimoAceptado_us = -1;
  _ultimoPeriodo_us = 0;
  _periodosVentana = 0;
  _sumaPeriodosVentana_us = 0;
}
```

### 5.2. Sustitución de `setK()`

Esta versión debe llamarse **desde la tarea principal y fuera de una adquisición activa**.

```cpp
bool Caudalimetro::setK(double nuevoK) {
  if (!std::isfinite(nuevoK) ||
      nuevoK \u003c 10.0 || nuevoK \u003e 1000.0) {
    return false;
  }

  portENTER_CRITICAL(\u0026_mux);

  _cfg.k_Hz_por_Lmin = nuevoK;
  _cfg.calibracionDocumentada = false;
  ++_epoca;

  // Nueva epoca de volumen; los contadores brutos se conservan.
  _pulsosOffset = _pulsosMedicion;
  _anteriorMedicion = _pulsosMedicion;

  reiniciarEstimadorLocked();

  if (_iniciado) {
    _nivelAlto = (digitalRead(_cfg.pin) == HIGH);
    _inicioNivel_us = esp_timer_get_time();
  }

  portEXIT_CRITICAL(\u0026_mux);

  _volumenEstimadoAcumulado_L = 0.0;
  _qSuavizadoDisplay = 0.0f;
  _inicioSinPulso_us = -1;
  _falloAlimentacion = false;

  // No publicar como actual una muestra de la epoca anterior.
  _ultimaMuestra = Muestra{};
  _ultimaMuestra.k = nuevoK;
  _ultimaMuestra.frecuencia_Hz = NAN;
  _ultimaMuestra.q_mLmin = NAN;
  _ultimaMuestra.volumenEstimado_L = NAN;

  return true;
}
```

**Esto no corrige por sí solo la V5.2:** las rutas HTTP y auto-calibración seguirían necesitando los bloqueos descritos.

## 6. Variante reducida para la primera prueba

Para no convertir la primera campaña en una depuración simultánea de hidráulica, HTTP, NVS y auto-calibración, propongo **un sketch separado de banco**.

Mantiene tus clases de sensor y registro, pero elimina:

- **Wi-Fi, SCADA y NVS** durante la prueba.
- **Cambios de K y microstepping** durante la ejecución.
- **Inversión y auto-calibración**.
- **El segundo datalogger**.
- **La presentación de RPM como velocidad medida**.

Usa una consigna fija de 25 RPM, editable **antes de compilar**, y distingue la marcha de la ventana de recogida. Permite iniciar y finalizar el registro con la bomba estabilizada.

### 6.1. Archivos que debe contener esta variante

Crear una carpeta de sketch separada con estos archivos:

- **`Banco_calibracion.ino`**, con el código siguiente.
- **`config.h`**, del expediente.
- **`caudalimetro.h` y `caudalimetro.cpp`**, con los dos cambios anteriores.
- **`registro_ensayos.h`**, con los cambios de este apartado.

No incluir el `.ino` original, `Bomba.cpp`, el HTML ni los demás módulos en esta variante.

Está orientada a **Arduino-ESP32 3.x**; no mezcla APIs de 2.x y 3.x. El modelo concreto de placa debe tener el pinout del ESP32 del expediente.

### 6.2. Cambios en `registro_ensayos.h`

Agregar:

```cpp
#include \u003cnew\u003e
```

Sustituir el constructor y añadir estas funciones públicas. Se evita depender de la construcción de objetos mediante `malloc`.

```cpp
RegistroEnsayos() {
  _filas = new (std::nothrow) Fila[MAX_FILAS]{};
}

~RegistroEnsayos() {
  delete[] _filas;
}

RegistroEnsayos(const RegistroEnsayos\u0026) = delete;
RegistroEnsayos\u0026 operator=(const RegistroEnsayos\u0026) = delete;

bool disponible() const {
  return _filas != nullptr;
}

void marcarFalloControl() {
  _incompleto = true;
  _estadoSesion |= FALLO_CONTROL;
}
```

Sustituir el enum `Estado` por:

```cpp
enum Estado : uint16_t {
  NORMAL        = 0,
  CAMBIO_EPOCA  = (1U \u003c\u003c 0),
  OMISIONES     = (1U \u003c\u003c 1),
  FALLO_CONTROL = (1U \u003c\u003c 2)
};
```

### 6.3. Sketch completo de prueba

**El arranque queda bloqueado por defecto.** `INTERFAZ_DRIVER_VERIFICADA` solo debe ponerse en `true` después de comprobar la adaptación eléctrica, polaridad ENA, microstepping y parada física.

La espera de habilitación de 250 ms es un valor de esta variante, **no una especificación universal del DM860**. Debe contrastarse con tu unidad.

```cpp
#include \u003cArduino.h\u003e
#include \u003ccmath\u003e
#include \u003cesp_arduino_version.h\u003e

#include "config.h"
#include "caudalimetro.h"
#include "registro_ensayos.h"

#if ESP_ARDUINO_VERSION_MAJOR != 3
#error "Esta variante requiere Arduino-ESP32 3.x"
#endif

// Cambiar solo despues de la verificacion electrica documentada.
constexpr bool INTERFAZ_DRIVER_VERIFICADA = false;

// Deben coincidir con la interfaz y el driver reales.
constexpr uint8_t NIVEL_INHIBIR = HIGH;
constexpr uint8_t NIVEL_HABILITAR = LOW;

constexpr float RPM_PRUEBA = 25.0f;
constexpr uint32_t ESPERA_HABILITACION_MS = 250;
constexpr uint32_t PERIODO_CONTROL_MS = 20;
constexpr uint32_t MAX_RETRASO_CONTROL_MS = 250;
constexpr uint32_t PERIODO_MUESTRA_MS = 1000;

static_assert(RPM_PRUEBA \u003e= RPM_MIN \u0026\u0026
              RPM_PRUEBA \u003c= RPM_MAX,
              "RPM_PRUEBA fuera de rango");

static_assert(PULSOS_POR_REV == 3200,
              "Revisar el control para otros micropasos");

Caudalimetro alim(SENSOR_ALIM_CFG, "ALIMENTACION", true);
Caudalimetro perm(SENSOR_PERM_CFG, "PERMEADO", false);
RegistroEnsayos registro;

bool listo = false;
bool fallo = false;
bool pwmAdjunto = false;
bool marchaSolicitada = false;
bool esperandoHabilitacion = false;

float rpmRampa = 0.0f;
uint32_t frecuenciaStep = 0;

uint32_t tControl = 0;
uint32_t tMuestra = 0;
uint32_t tHabilitacion = 0;

// Es la velocidad equivalente de la frecuencia STEP configurada.
// NO es una medicion del eje.
float rpmStep() {
  return 60.0f * static_cast\u003cfloat\u003e(frecuenciaStep) /
         static_cast\u003cfloat\u003e(PULSOS_POR_REV);
}

bool motorParado() {
  return !marchaSolicitada \u0026\u0026
         !esperandoHabilitacion \u0026\u0026
         rpmRampa == 0.0f \u0026\u0026
         frecuenciaStep == 0;
}

void cerrarVentana() {
  if (!registro.activo()) return;

  const bool impulsa = frecuenciaStep != 0;
  if (!registro.finalizar(alim, perm, rpmStep(), impulsa)) {
    registro.marcarFalloControl();
    fallo = true;
    digitalWrite(PIN_ENA, NIVEL_INHIBIR);
  }
}

void fallar(const char* causa) {
  // Solicitar inhibicion antes de cualquier salida por Serial.
  digitalWrite(PIN_ENA, NIVEL_INHIBIR);

  fallo = true;
  marchaSolicitada = false;
  esperandoHabilitacion = false;
  rpmRampa = 0.0f;

  if (pwmAdjunto) {
    // Si falla, no se puede garantizar por software el corte STEP.
    // La inhibicion y la parada fisica siguen siendo necesarias.
    (void)ledcWrite(PIN_PUL, 0);
  }

  frecuenciaStep = 0;

  if (registro.activo()) {
    registro.marcarFalloControl();
    cerrarVentana();
  }

  Serial.print("# FALLO_ENCLAVADO: ");
  Serial.println(causa);
}

bool aplicarRPM(float rpm) {
  if (rpm \u003c 1.0f) {
    if (!ledcWrite(PIN_PUL, 0)) {
      fallar("No se pudo poner STEP a duty cero");
      return false;
    }

    frecuenciaStep = 0;
    return true;
  }

  const uint32_t pedida = static_cast\u003cuint32_t\u003e(
      lroundf(rpm * PULSOS_POR_REV / 60.0f));

  if (pedida != frecuenciaStep) {
    const uint32_t obtenida =
        ledcChangeFrequency(PIN_PUL, pedida, 10);

    if (obtenida == 0) {
      fallar("Fallo al configurar frecuencia STEP");
      return false;
    }

    frecuenciaStep = obtenida;
  }

  if (!ledcWrite(PIN_PUL, 512)) {
    fallar("Fallo al configurar duty STEP");
    return false;
  }

  return true;
}

void iniciarMotor() {
  if (!listo || fallo || !motorParado() || registro.activo()) {
    Serial.println("# START rechazado: estado no disponible");
    return;
  }

  if (!INTERFAZ_DRIVER_VERIFICADA) {
    Serial.println("# START bloqueado: verificar interfaz driver");
    return;
  }

  if (!ledcWrite(PIN_PUL, 0)) {
    fallar("No se pudo asegurar STEP en reposo");
    return;
  }

  // Direccion fija; no existe inversion en esta variante.
  digitalWrite(PIN_DIR, LOW);
  digitalWrite(PIN_ENA, NIVEL_HABILITAR);

  marchaSolicitada = true;
  esperandoHabilitacion = true;
  tHabilitacion = millis();
  tControl = tHabilitacion;

  Serial.println("# Motor: arranque solicitado");
}

void solicitarParada() {
  marchaSolicitada = false;

  if (esperandoHabilitacion) {
    esperandoHabilitacion = false;
  }

  Serial.println("# Motor: parada solicitada");
}

void actualizarMotor(uint32_t ahora) {
  const uint32_t transcurrido = ahora - tControl;
  if (transcurrido \u003c PERIODO_CONTROL_MS) return;

  tControl = ahora;

  if (fallo || !listo) return;

  const bool ocupado =
      marchaSolicitada || esperandoHabilitacion ||
      frecuenciaStep != 0;

  if (ocupado \u0026\u0026 transcurrido \u003e MAX_RETRASO_CONTROL_MS) {
    fallar("Retraso excesivo del control");
    return;
  }

  if (esperandoHabilitacion) {
    if (ahora - tHabilitacion \u003c ESPERA_HABILITACION_MS) {
      return;
    }

    esperandoHabilitacion = false;
  }

  const float dt = transcurrido / 1000.0f;

  if (marchaSolicitada) {
    if (rpmRampa \u003c 1.0f) rpmRampa = 1.0f;

    rpmRampa = fminf(
        RPM_PRUEBA,
        rpmRampa + ACEL_ARRANQUE_RPM_S * dt);
  } else {
    rpmRampa = fmaxf(
        0.0f,
        rpmRampa - FRENADO_PARADA_RPM_S * dt);

    if (rpmRampa \u003c 1.0f) rpmRampa = 0.0f;
  }

  if (!aplicarRPM(rpmRampa)) return;

  if (!marchaSolicitada \u0026\u0026 frecuenciaStep == 0) {
    digitalWrite(PIN_ENA, NIVEL_INHIBIR);

    // Si el operador no cerro antes con F, FIN queda definido
    // por el fin de STEP. No incluye todo posible flujo residual.
    cerrarVentana();
  }
}

void iniciarVentana() {
  if (fallo || !listo || registro.activo() ||
      !marchaSolicitada || esperandoHabilitacion ||
      fabsf(rpmRampa - RPM_PRUEBA) \u003e 0.01f ||
      frecuenciaStep == 0) {
    Serial.println("# INICIO rechazado: esperar regimen comandado");
    return;
  }

  if (!registro.iniciar(alim, perm, rpmStep(), true)) {
    fallar("No se pudo iniciar el registro");
    return;
  }

  Serial.println("# Ventana INICIO registrada");
}

void exportar() {
  if (!motorParado() || registro.activo()) {
    Serial.println("# Exportacion rechazada: detener primero");
    return;
  }

  Serial.println("# variante=banco_reducido");
  Serial.println("# rpm=equivalente_STEP_no_medicion_eje");
  Serial.println("# FALLO_CONTROL=bit_2_del_campo_estado");
  Serial.println("# FIN_por_F_o_fin_STEP;ver_protocolo_de_recogida");

  if (!registro.exportarCSV(Serial)) {
    Serial.println("# ERROR: exportacion no completada");
  }
}

void atenderComando(char c) {
  switch (c) {
    case 'S':
    case 's':
      iniciarMotor();
      break;

    case 'I':
    case 'i':
      iniciarVentana();
      break;

    case 'F':
    case 'f':
      if (!registro.activo()) {
        Serial.println("# No existe ventana activa");
      } else {
        cerrarVentana();
        Serial.println("# Ventana FIN procesada");
      }
      break;

    case 'P':
    case 'p':
      solicitarParada();
      break;

    case 'X':
    case 'x':
      fallar("Parada de software solicitada");
      break;

    case 'E':
    case 'e':
      exportar();
      break;

    case 'C':
    case 'c':
      if (!motorParado() || registro.activo()) {
        Serial.println("# Limpieza rechazada");
      } else if (registro.limpiar()) {
        Serial.println("# Registro limpio");
      }
      break;

    case '\r':
    case '\
':
    case ' ':
      break;

    default:
      Serial.println("# Comandos: S I F P X E C");
      break;
  }
}

void setup() {
  // Esta orden no protege el intervalo previo al setup.
  // Se requiere estado electrico seguro durante reset/arranque.
  digitalWrite(PIN_ENA, NIVEL_INHIBIR);
  pinMode(PIN_ENA, OUTPUT);

  digitalWrite(PIN_PUL, LOW);
  pinMode(PIN_PUL, OUTPUT);

  digitalWrite(PIN_DIR, LOW);
  pinMode(PIN_DIR, OUTPUT);

  Serial.begin(115200);

  // K iniciales son provisionales para esta campana.
  const bool ka = alim.setK(K_ALIMENTACION);
  const bool kp = perm.setK(K_PERMEADO);

  const bool okAlim = alim.begin();
  const bool okPerm = perm.begin();

  pwmAdjunto = ledcAttach(PIN_PUL, 800, 10);
  const bool pwmReposo =
      pwmAdjunto \u0026\u0026 ledcWrite(PIN_PUL, 0);

  listo = ka \u0026\u0026 kp \u0026\u0026 okAlim \u0026\u0026 okPerm \u0026\u0026
          registro.disponible() \u0026\u0026 pwmReposo;

  tControl = millis();
  tMuestra = tControl;

  if (!listo) {
    fallar("Inicializacion incompleta");
    return;
  }

  Serial.println("# Banco reducido: 115200 baudios");
  Serial.println("# S: arrancar; I: inicio recogida; F: fin recogida");
  Serial.println("# P: parar; X: fallo enclavado; E: CSV; C: limpiar");
  Serial.println("# Sin rearme por software: fallo requiere revision");
  Serial.println("# Parada fisica independiente obligatoria");
}

void loop() {
  uint32_t ahora = millis();
  actualizarMotor(ahora);

  // Un caracter por vuelta: sin readString ni esperas de linea.
  if (Serial.available() \u003e 0) {
    atenderComando(static_cast\u003cchar\u003e(Serial.read()));
  }

  ahora = millis();

  if (ahora - tMuestra \u003e= PERIODO_MUESTRA_MS) {
    tMuestra = ahora;

    const bool impulsa = frecuenciaStep != 0;
    const auto ma = alim.capturar(impulsa);
    const auto mp = perm.capturar(impulsa);

    registro.tick(ma, mp, rpmStep());

    if (registro.activo() \u0026\u0026 registro.incompleto()) {
      fallar("Registro incompleto: revisar capacidad o temporizacion");
    }
  }
}
```

### 6.4. Qué garantiza y qué no garantiza esta variante

**Por construcción**, no permite cambiar K en ejecución, no escribe NVS, no depende de HTTP y no empieza una ventana si falla el registrador.

No garantiza:

- **Velocidad real del motor**, porque no hay medición del eje.
- **Una parada física de tiempo certificado**.
- **Sincronización automática entre probeta y contador**.
- **La exactitud del YF-S401** a bajo flujo.
- **Que el montaje eléctrico esté correcto**.
- **Que el control sobreviva a un bloqueo indefinido**; detectar un retraso al recuperar ejecución no sustituye un interbloqueo independiente.

La variante reduce el alcance para poder ensayar con menos fuentes de error. **No es una aprobación retroactiva del SCADA V5.2.**

## 7. Procedimiento de aceptación antes de medir agua

### 7.1. Compilación e inicialización

**Primero, con potencia del motor desconectada.**

- Registrar **placa, versión exacta de Arduino-ESP32 y opciones de compilación**.
- Compilar todos los archivos de la variante en una carpeta independiente.
- Confirmar que **`S` queda bloqueado** con la interfaz no verificada.
- Confirmar que no hay reinicios ni errores de inicialización.
- Verificar reserva de RAM y exportación de un registro vacío.

No doy por comprobada la compatibilidad binaria hasta tener el resultado real de compilación.

### 7.2. Prueba eléctrica de señales

**Antes de conectar el driver a la carga.**

- Medir **STEP, DIR y ENA**, incluyendo encendido y reset.
- Confirmar **3 200 pulsos por vuelta** en los DIP.
- A 25 RPM, comprobar una frecuencia esperada de aproximadamente **1 333 Hz**, calculada como 25 × 3 200 / 60.
- Verificar que **P** lleva STEP a reposo y que **X** enclava fallo.
- Comprobar la parada física **sin depender de Serial, Wi-Fi ni firmware**.
- Confirmar que una falla de control no se interpreta como una corrida válida.

### 7.3. Prueba de adquisición con pulsos conocidos

**Esta prueba debe preceder a la calibración hidráulica.**

Aplicar señales compatibles con 3,3 V a cada entrada y comparar pulsos generados con los aceptados. Conviene probar, como mínimo, frecuencias de 2, 10, 50, 100 y 270 Hz, con varios duty cycles y respetando los anchos configurados de cada canal.

En particular:

- **Caracterizar el efecto de arranque**: el detector necesita conocer niveles antes de aceptar pulsos.")</script><script>((self[Symbol