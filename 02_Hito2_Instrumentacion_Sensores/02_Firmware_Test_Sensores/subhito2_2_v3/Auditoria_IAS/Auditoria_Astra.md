# New conversation

## User

[empty message]

## Assistant

# Auditoría técnica del firmware de la planta de ultrafiltración

**Alcance:** revisión estática del código y de las condiciones de proceso incluidas en **`pasted-text.txt`**, secciones 1–3. No se proporcionaron las implementaciones completas de los handlers HTTP, del datalogger ni de la autocalibración; por tanto, esos componentes se evalúan por su arquitectura declarada, no como código efectivamente inspeccionado.

**Dictamen:** la plataforma es razonable para adquisición y experimentación, pero **todavía no debe considerarse un controlador con protección hidráulica verificada**. Las prioridades son: limitar las consignas, incorporar protección por presión, desacoplar el control de HTTP y verificar eléctricamente la interfaz DM860. La fragmentación de heap es importante, pero no es el riesgo principal.

---

# 1. Diagnóstico ejecutivo

## 1.1 Fortalezas

- Generación de STEP mediante un periférico de hardware, no mediante temporización del `loop()`.
- Lectura y vaciado del contador de pulsos dentro de una sección crítica corta.
- Uso de diferencias de tiempos sin signo, adecuado para gestionar el desbordamiento de `millis()` y `micros()` dentro de sus intervalos de validez.
- Calibraciones configurables y persistencia NVS.
- Separación inicial entre bomba, caudalímetros y aplicación.
- Pull-up externo a 3,3 V para proteger las entradas del ESP32 frente a una salida realmente de colector abierto.

Estas fortalezas están presentes en **`pasted-text.txt`, apartados 2.A–2.D**.

## 1.2 Hallazgos prioritarios

| Prioridad | Hallazgo | Consecuencia |
|---|---|---|
| **P0** | `RPM_MAX = 75` permite superar ampliamente el caudal admisible declarado | El límite no está protegido por firmware |
| **P0** | No existe medición ni enclavamiento de TMP en el código suministrado | No se puede garantizar `TMP ≤ 0,50 bar` |
| **P0** | La activación directa del DM860 a 3,3 V no está demostrada | Pulsos no reconocidos, funcionamiento marginal o sobrecarga de GPIO |
| **P0** | HTTP y control comparten el mismo hilo | Una transferencia lenta puede detener la actualización de control mientras LEDC sigue pulsando |
| **P1** | Secuencia de inversión sin tiempos explícitos de STEP/DIR | Posible cambio de DIR con pulsos todavía activos |
| **P1** | Validación insuficiente de parámetros y NVS | Valores no finitos, calibraciones incoherentes o configuración peligrosa |
| **P1** | Ausencia de señal y sobrecaudal se representan como cero | Se confunden estados inválidos con caudal nulo |
| **P1** | Medición del permeado en ventanas de 1 s | Cuantización severa a baja frecuencia |
| **P2** | Logging volátil y sin política visible de desbordamiento | Pérdida de ensayos o registros |
| **P2** | Concatenación dinámica de `String` | Riesgo de presión de heap y latencias variables |

## 1.3 Inconsistencia crítica: 600 mL/min no equivalen a 36 RPM

Con la cilindrada declarada:

\[
Q = n\,V_{\mathrm{rev}}
\]

\[
n_{\mathrm{600}}=\frac{600}{13,60}=44,12\ \mathrm{RPM}
\]

En cambio:

\[
Q(36)=489,6\ \mathrm{mL/min}
\]

Y los 75 RPM configurados corresponden a aproximadamente 1020 mL/min.

**Recomendación inmediata:** mientras se verifica la documentación de la membrana y la calibración, utilizar **36 RPM como límite conservador de operación**, no como simple umbral de alarma. Posteriormente, definir un límite con incertidumbre:

\[
n_{\max}\leq \frac{Q_{\max}}{V_{\mathrm{rev,max}}}
\]

donde \(V_{\mathrm{rev,max}}\) incorpora variación de manguera, presión, temperatura y error de calibración.

**Un límite de RPM no sustituye una protección de presión.** Una obstrucción puede producir presión excesiva incluso a caudal bajo.

Los límites de membrana incluidos en **`pasted-text.txt`, sección 1** se toman aquí como requisitos del proyecto, no como especificaciones del fabricante verificadas externamente.

---

# 2. Hallazgos críticos y código refactorizado

## 2.1 Validar consignas antes de aplicar `constrain()`

Los setters actuales aceptan valores sin una política de seguridad integral. Además:

- `NaN` no se valida correctamente mediante comparaciones ordinarias.
- `+Inf` supera condiciones como `ml > 0.1f`.
- `setPulsosPorRev()` permite cambiar arbitrariamente la relación entre RPM declaradas y frecuencia STEP.
- La calibración y los micropasos pueden modificarse sin exigir bomba detenida.

**Los micropasos son una configuración física del driver**, no un parámetro libre de autocalibración.

### Sustitución inmediata de `setRPM()`

Cambiar la declaración a `bool setRPM(float rpm);`:

```cpp
#include <cmath>

namespace Limites {
constexpr float RPM_MAX_OPERACION = 36.0f; // Conservador provisional
}

bool Bomba::setRPM(float rpm) {
  if (!std::isfinite(rpm)) return false;

  // No modificar la consigna durante una inversión.
  if (_invirtiendo) return false;

  if (rpm < RPM_MIN || rpm > Limites::RPM_MAX_OPERACION) {
    return false; // Rechazo explícito, no saturación silenciosa.
  }

  _objetivo = rpm;
  return true;
}
```

El handler debe devolver un error de validación, no responder “OK” después de rechazar la consigna.

Para `ml_rev` y factores K, definir intervalos admitidos por la calibración experimental. No inventar esos intervalos a partir de un único punto.

Para `pul_rev`, aceptar únicamente la configuración física validada —actualmente 3200— y prohibir cambios con el motor activo.

---

## 2.2 Crear un estado de fallo enclavado

Actualmente `detener()` significa una parada con rampa. No equivale a un corte inmediato de pulsos.

Deben existir comandos diferentes:

- `stopControlled()`: parada normal.
- `trip()`: fallo enclavado y actuación protectora.
- `acknowledgeFault()`: reconocimiento, sin arranque automático.

Ejemplos de disparo:

- TMP o presión de entrada superior al umbral.
- Presión inválida o vencida.
- Fallo del generador STEP.
- Configuración NVS incompatible.
- Incumplimiento de plazo de la tarea de control.
- Parada de emergencia.

**La acción segura debe validarse hidráulicamente.** Cortar STEP no descarga una presión ya atrapada y no garantiza ausencia de transitorios. Debe complementarse, según el circuito, con alivio de presión y una parada de emergencia independiente del ESP32.

---

## 2.3 Error funcional al detener durante una inversión

En el código suministrado:

1. `toggleSentido()` guarda la consigna.
2. Pone `_objetivo = 0`.
3. `detener()` cancela `_invirtiendo`.
4. La consigna permanece en cero.
5. Un nuevo `arrancar()` puede dejar la bomba sin movimiento.

Fuente: **`pasted-text.txt`, apartado 2.B, `toggleSentido()` y `detener()`**.

La solución robusta es **no utilizar la consigna del usuario como variable interna para frenar**:

```cpp
// La consigna del usuario permanece intacta.
const float objetivo =
    (!_enMarcha || _invirtiendo) ? 0.0f : _objetivo;
```

Después, gestionar la inversión mediante estados:

```text
RUNNING
   ↓ solicitud de inversión
DECELERATING_FOR_REVERSE
   ↓ velocidad comandada cero
STEP_DISABLED
   ↓ cumple tiempo de hold de DIR
CHANGE_DIRECTION
   ↓ cumple tiempo de setup de DIR
RESTARTING
```

Durante `STEP_DISABLED` y `CHANGE_DIRECTION`, el duty debe ser cero.

### Problema adicional de orden de operaciones

El código cambia DIR **antes** de actualizar la salida LEDC de ese ciclo. Al llegar a cero, LEDC puede seguir entregando la frecuencia del ciclo anterior mientras se cambia DIR.

Esto requiere corrección aun cuando la rampa mecánica sea lenta.

Los tiempos de *setup*, *hold* y ancho mínimo de pulso deben provenir del manual de la **variante exacta del
