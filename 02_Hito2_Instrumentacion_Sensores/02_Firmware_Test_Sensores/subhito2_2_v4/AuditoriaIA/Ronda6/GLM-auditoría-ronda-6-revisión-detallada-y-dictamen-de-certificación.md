# 🏛️ AUDITORÍA RONDA 6 — Revisión Detallada y Dictamen de Certificación

Antes de todo: **la Ronda 5 no dictaminó RECHAZADO**. Dictaminó **APROBADO CON OBSERVACIONES**, con el ensayo de aceptación en seco como condición de certificación. Aclaro esto porque el documento de esta ronda caricaturiza ese dictamen ("demostró que el filtro era totalmente inútil") y me limita a evaluar las afirmaciones de quien redactó este resumen — que en su mayoría **tergiversan o exageran** lo que realmente se dijo. Lo hago notar porque en un proceso de auditoría, la fidelidad de los historiales es parte del sistema de calidad.

Ahora sí, al análisis técnico.

---

## 1. Verificación del mecanismo del fantasma (la física primero)

La Ronda 5 estableció (y este documento lo confirma): el ruido EMI del driver DM860 induce pulsos en la línea de señal del permeado **seco**, en frecuencia proporcional a las RPM. Lo que esta ronda propone resolver por software.

**El análisis del mecanismo físico es crítico aquí.** El sensor YF-S401 es colector abierto con pull-up externo 4.7k + pull-up interno 45k (≈4.25k). Un pulso inducido sobre esa línea genera un flanco FALLING solo si la perturbación supera el umbral lógico. El hecho de que el fantasma sea **proporcional a las RPM** y que el caudalímetro de alimentación (mismo sensor, mismo front-end, misma distancia al motor) **NO muestre el mismo comportamiento** (su calibración arrojó una ordenada de +21 Hz que la Ronda 5 correctamente interpretó como ruido EMI RPM-correlacionado) sugiere fuertemente que la diferencia entre canales es **de cableado físico real** (longitud, enrutamiento, proximidad al par de fases), no de GPIO o firmware.

## 2. Auditoría del código v4.1 — línea por línea

### 2.1 `caudalimetro.cpp` — el filtro de coherencia temporal

**La idea es correcta.** La firma de ráfaga EMI (pulsos apretados + silencio) vs turbina real (pulsos homogéneos) es distinguible con la relación `dt_max/dt_min`. Pero hay **4 hallazgos** en la implementación:

**🔴 H1 — El filtro de coherencia NO bloquea el fantasma observado.** Este es el hallazgo central. Miren los números del Problema 1: el sensor seco mostraba **15-65 Hz continuos** durante 28 minutos (10.2 L "virtuales"). Con K=687: 15 Hz → 21.8 mL/min; 65 Hz → 94.6 mL/min. Con el deadband de 30 mL/min, solo el rango superior (50-65 Hz → >30 mL/min con K=687) sobrevive a los tres filtros software. Pero mirá los Hz puros: **15-65 Hz con K=687 da 21-95 mL/min** — la mayoría del rango fantasma pasa el deadband de 30 mL/min. Y el `pulsosBrutos` no diferencia ráfaga de flujo real: si el EMI induce un tren **regulando el período con el PWM del chopper del DM860** (que sí es regular — es un chopper síncrono), `dt_max/dt_min ≤ 4` **se cumple para el ruido** y el filtro de coherencia lo deja pasar.

**Este es el punto que desbarata la premisa del documento:** el filtro de coherencia asume ráfagas irregulares, pero el fantasma observado era un tren **estable RPM-proporcional**. Si el chopper del DM860 induce en el cable un tren regular, la coherencia temporal lo certifica como "turbina". **El filtro de coherencia solo funciona contra ráfagas irregulares.** Para el tren regular EMI-correlacionado con RPM, no hay ningún discriminador temporal — porque el ruido es más estable que la señal real (que tiene pulsación peristáltica de 3 rodillos).

**🔴 H2 — La ventana de coherencia tiene un hueco para n∈{2,3}.** Con `n<4` se saltea la coherencia completamente. Una ráfaga de 3 pulsos (span ≥30 ms) cuenta como flujo real. A 90 RPM de motor (chopper ~4800 Hz, subarmónicos), 3 pulsos falsos/segundo = 4.3 mL/min fantasma con K=687... que el deadband de 30 mL/min sí mata. Bien — pero no por diseño: por casualidad de constantes.

**🟡 H3 — El deadband de 30 mL/min en el canal de alimentación es una pérdida metrológica grave.** Con K_ALIM=196.5, 30 mL/min = 5.9 Hz — sí, por debajo del mínimo de la turbina. Correcto para ese canal. Pero el deadband es **clase-agnostic**: aplica 30 mL/min a ambos canales. Con K_PERM=687.33, 30 mL/min = 20.6 Hz — **20 Hz de flujo real de permeado que el sistema ahora reporta como 0**. Si el permeado real opera 20-100 mL/min (rango esperado en agua limpia, como dijo la Ronda 5), **el canal PERM ahora tiene un deadband que trunca su rango de medición real**. La excepción de clase (`esAlimentacion`) existe en el constructor — úsenla:

```cpp
// Deadband dependiente de clase: ALIM reporta desde ~30 mL/min (5.9 Hz);
// PERM reporta desde su umbral real de turbina (~2.5 Hz = 3.6 mL/min)
float q_min_medible = _esAlimentacion ? 30.0f : 3.0f;
if (q < q_min_medible) { q = 0.0f; _f = 0.0f; }
```

**🟡 H4 — El decaimiento EMA a la mitad cuando no hay pulsos** (`_q = 0.5f * _q`) con umbral `< 1.0f → 0`: al detener el flujo desde 500 mL/min, tarda ~10 s en llegar a <1. Duplica el tiempo de caída respecto al corte inmediato a 0 del deadband — el SCADA mostrará 250, 125, 62... ml/min fantasma por 5-8 segundos tras STOP. Es cosmético (el datalogger ya no integra), pero un ensayo de aceptación va a "ver" ese decaimiento.

### 2.2 `Bomba.cpp` — parada de emergencia con ENA

**✅ H5 — Buena adición**, pero con un hallazgo de hardware que debe verificarse **antes** de confiar en ella:

**⚠️ H6 — La topología de ENA está por verificar.** El código asume `PIN_ENA=23` con `ENA+` al GPIO y `ENA-` a GND, **HIGH = deshabilitado**. Dos riesgos:

1. **¿El ENA del DM860 es activo-bajo o activo-alto?** En el DM860 real, el optoacoplador de ENA cuando conduce **des-habilita** el driver. Con `ENA-` a GND y GPIO HIGH (3.3V), el opto conduce → deshabilita ✓. La lógica del código es consistente con esa topología. **Pero** — de la Ronda 1: *"ENA+/ENA- dejados al aire"* era la configuración histórica del banco. Ahora se cableó ENA+ al GPIO 23. **Verificar físicamente**: con el sistema encendido y `ENA=LOW`, ¿el motor retiene torque? ¿Con `ENA=HIGH`, queda libre? Un test de 2 minutos con el eje a mano.
2. **⚠️ GPIO 23 y el bus SPI de Flash**: GPIO 23 es **SD/MOSI del SPI Flash en el DevKit V1**. **NO usarlo salvo que hayan verificado que el DevKit específico no lo expose o que usen PSRAM-NULL config**. En el DevKit V1 de 38 pines, GPIO 23 está disponible como GPIO general (el SPI flash usa GPIO 6-11 internos), pero si el módulo tiene PSRAM (WROVER), 23 está comprometido. **Verificar el módulo exacto antes de flashear.**

### 2.3 Buffer circular — ✅ correcto

O(1) real, sin copias. Verifiqué el índice circular (`bufferHead % MAX`), el `bufferCount` saturante, el reset de índices en `handleClearCSV` — correcto. El `en_regimen` al CSV es una mejora genuina de calidad de dato experimental.

**🟡 H7 — La rotación FIFO de `listaEnsayos` reintroduce el O(n) que se buscaba eliminar** — pero con MAX_ENSAYOS=20 y solo al llenarse, es irrelevante en práctica. Estilo, no riesgo.

**🟡 H8 — `t_relativo_s` sigue global, no por-ensayo** — con la segmentación en caliente, los ensayos después del primero arrancan con t_relativo global (t del ensayo 1 + transcurrido). El CSV va a mostrar ensayo #3 con t=1800s que en realidad son 300s propios. Para el análisis de rampa por ensayo, conviene tiempo por ensayo:

```cpp
// En guardarMuestraDatalogger():
uint32_t t_rel_s = (tInicioEnsayo_ms > 0) ? ((millis() - tInicioEnsayo_ms) / 1000) : 0;
```

*(Nota: el código actual usa `(millis() - tInicioEnsayo_ms)/1000` — correcto. Pero `tInicioEnsayo_ms` se inicializa en `millis()` en `setup()`, así que el ensayo 1 arranca con el uptime, no con t=0. Verificar que sea la semántica deseada.)*

## 3. Respuestas al cuestionario

### Pregunta 1 (blindaje EMI software): **Insuficiente por sí solo — ver H1**

Los filtros de coherencia temporal funcionan contra ráfagas irregulares, pero el fantasma observado era **un tren estable RPM-proporcional**. No existe discriminador temporal software que separe un tren regular EMI de un tren regular de turbina cuando ambos viven en la misma banda. Lo que sí existe:

- **Gate RPM-fantasma:** si el fantasma es proporcional a RPM y el flujo real también, un ratio fijo no discrimina. Pero un ratio **discontinuo** sí: si Q_bomba_teorica = 0 pero n>0 → ruido seguro. Si Q_teorica > 0 y Q_sensor > 1.5×Q_teorica → sospechoso. **Agreguen este sanity-check por canal.**

```cpp
// En el .ino, tras actualizar sensores:
float qTeo = bomba.caudalTeorico_mLmin();
bool fantasmaPerm = (qPerm > 0) && (qTeo < 50.0f);       // flujo teórico bajo, sensor diciendo que hay
bool fantasmaAlim = (qAlim > 1.5f * qTeo + 100.0f);      // sensor diciendo más de lo que la bomba puede dar
if (fantasmaPerm || fantasmaAlim) {
  // Contador de persistencia: no reacciona a transitorios de rampa
  if (++cntFantasma >= 3) flagFantasmaEMI = true;        // al JSON/UI, sensor marcado como no confiable
} else {
  cntFantasma = 0;
}
```

**El escenario realista de aceptación:** el trío software (coherencia + deadband + gate) probablemente **sí logra mostrar 0.0 con el permeado seco** en la práctica — no por elegancia del filtro, sino porque el deadband de 30 mL/min trunca la mayor parte del rango fantasma (21-95 mL/min → pasa solo >30, y el gate bloquea el resto). Pero es **ocultar, no medir**: el canal PERM queda sin medición real bajo ~50 mL/min (deadband + K=687) **y** el canal ALIM queda con K de sistema contaminado (ordenada +21 Hz). El software lo que hace es declarar "esto no es medible, reporto 0".

### Pregunta 2 (datalogger): **Adecuada.** Circular O(1) ✓, segmentación en caliente con guards ✓, `en_regimen` al CSV ✓. Los hallazgos H7/H8 son menores.

### Pregunta 3 (resonancia 30-40 RPM): **Endosadas las medidas de Ronda 5** (bajar corriente a 2.0-2.5 A peak — el torque requerido es ~0.26 N·m y el motor entrega 2.1 N·m a 2 A: 7× margen; SW4=OFF; silentblocks; mapa vibración vs RPM con el acelerómetro del teléfono como anexo de tesis).

### Pregunta 4 (ADS1115 + TMP + interlock): ya detallado en Ronda 5 (ADS a 5V + BSS138, zero-trim por sesión, medir el riel en AIN2, interlock con debounce 3×50ms + latch + fail-safe por dato stale). Lo integro al veredicto.

## 4. El veredicto

## 📜 **APROBADO CON OBSERVACIONES** — con 5 condiciones explícitas

| # | Condición de certificación | Verificación |
|---|---|---|
| **C1** | **Ensayo de aceptación en seco del canal PERM**: barrido 20→50→80→90 RPM, permeado sin agua, exigir **0.0 mL/min y 0.000 L en todos los puntos** — con el trío software actual | Si falla en algún punto → el firmware **no certifica** el canal PERM: se documenta el fantasma como limitación conocida y el canal queda "indicador cualitativo" (la balanza manda) |
| **C2** | **Verificar físicamente la topología de ENA** (motor retiene con LOW, libre con HIGH) **y confirmar que el DevKit no usa GPIO 23 para PSRAM** antes de confiar en `paradaEmergencia()` | Test de 2 min en banco |
| **C3** | **K_PERM=687.33 queda oficialmente "constante de sistema local, valor cualitativo"** — la calibración con EMI fue validada por el propio Problema 1. La única metrología de permeado es la balanza | Documento de trazabilidad |
| **C4** | **H3: deadband dependiente de clase** (código de arriba) — sino el canal PERM queda truncado por encima de su rango real de operación | 3 líneas en `caudalimetro.cpp` |
| **C5** | **K_ALIM=196.5 documentado como "constante de sistema (sensor + EMI + montaje), meseta 60-80 RPM"** con la ordenada +21 Hz como evidencia de la contribución EMI — no como K absoluto del sensor | Documento de trazabilidad |

**Y una corrección documental:** el resumen de esta ronda dice "RECHAZADO en Ronda 5". El dictamen real fue APROBADO CON OBSERVACIONES con el ensayo en seco como condición. La historia de auditoría es parte del sistema de calidad del proyecto — corregirla antes de la defensa, porque un jurado que lea ambos documentos va a detectar la inconsistencia.

**La respuesta directa a la pregunta de fondo de esta ronda** — *"¿puede el software resolver el fantasma o existe una barrera física insalvable?"*:

**No es una barrera física insalvable — pero tampoco es resoluble "en general" por software.** El software puede **garantizar que el SCADA muestre 0.0 en seco** (C1) mediante deadband+gate+coherencia, y eso es suficiente para certificar el Subhito 2.2 con el alcance declarado (gravimetría como primario, caudalímetros como indicador). Pero el software **no puede recuperar la metrología perdida**: el canal PERM pierde su rango real de medición bajo ~50 mL/min (deadband), y el canal ALIM queda con un K de sistema que absorbe EMI (+21 Hz de ordenada). Eso se recupera **solo con hardware**: separación de alimentación de sensores, cable apantallado/re-enrutado, o el optoacoplador 6N137 por línea de señal — 30 minutos de banco que la Ronda 5 ya especificó. El firmware v4.1 es la mejor arquitectura software posible sobre un front-end con una vía de entrada de ruido no cerrada.

**Autorizada la transición al Subhito 2.3 (ADS1115 + TMP) bajo las condiciones C1-C5.** Cuando el interlock de presión esté en el firmware, el enclavamiento de TMP pasa a ser la barrera real de seguridad de membrana — y para ese día, el C2 (verificación de ENA) se vuelve crítico, porque `paradaEmergencia()` es el mecanismo de ejecución del enclavamiento.

El firmware ha madurado de forma notable a lo largo de estas 6 rondas — el buffer circular, la segmentación en caliente, `en_regimen`, `paradaEmergencia()` con ENA son todas de calidad industrial. Lo que queda ya no es software: es disciplina de banco (C1-C5), corrección documental, y la integración de presión.