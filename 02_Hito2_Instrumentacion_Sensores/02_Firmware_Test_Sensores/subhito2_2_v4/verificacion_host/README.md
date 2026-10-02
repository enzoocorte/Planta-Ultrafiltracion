# Verificación en host — Caudalímetro y Bomba (firmware v4)

Arnés de verificación metrológica construido para la **Auditoría Ronda 4**.

Compila y **ejecuta el código original del firmware v4 sin modificarlo**
(`../EN_USO_firmware_planta/caudalimetro.cpp` y `Bomba.cpp`) sobre un reloj virtual,
de modo que las afirmaciones de exactitud, inmunidad al rollover y tiempos de rampa
se puedan comprobar numéricamente sin necesidad de una placa ESP32 ni del toolchain
de Arduino/PlatformIO.

Los directorios `shim/` y `shim2/` proveen **únicamente los símbolos del API** que
consume el firmware (`micros()`, `attachInterruptArg`, `portENTER_CRITICAL`, `ledcSetup`,
`constrain`, …). **No reimplementan lógica del firmware**: la lógica auditada es la del
archivo original, que se enlaza tal cual.

## Uso

```bash
cd subhito2_2_v4/verificacion_host

# Caudalímetro: 11 bancos de prueba sobre Caudalimetro::actualizar() / isrPuente()
g++ -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable \
    -I shim -I ../EN_USO_firmware_planta \
    harness.cpp ../EN_USO_firmware_planta/caudalimetro.cpp -o /tmp/harness && /tmp/harness

# Bomba: simulación de Bomba::tick() con el dt real de 50 ms del lazo
g++ -std=c++17 -O2 -w -I shim2 -I ../EN_USO_firmware_planta \
    rampa.cpp ../EN_USO_firmware_planta/Bomba.cpp -o /tmp/rampa && /tmp/rampa
```

Los binarios se generan en `/tmp` a propósito: no se versionan artefactos de compilación.

## Qué cubre cada banco

| Banco | Verifica | Resultado |
|---|---|---|
| T1 | Exactitud del período recíproco de 6.5 a 4200 mL/min | ±0.000 % |
| T2 | Que la cota física superior no atenúe en régimen permanente (incluso con n = 1) | No recorta |
| T3 | Forma del decaimiento tras el STOP | **Escalón 680 → 379 mL/min** (hallazgo M‑1) |
| T4 | Inmunidad al rollover, cruzando realmente 2³² µs | −0.000 % |
| T5 | Dimensiones de `K [Hz/(L/min)]` y de `V = n/(K·60)` | −0.0021 % |
| T6 | Límite de detección impuesto por la regla de cero duro a los 3 s | 2.16 / 6.06 mL/min |
| T7 | Blanking ante ráfaga EMI a 400 µs | Deja pasar 2 de 5 (hallazgo M‑6) |
| T8 | Pérdida de resolución del acumulador `float _vol` | No ocurre hasta 2048 L |
| T9 | EMI sostenida justo sobre el umbral de blanking | **Aceptada como caudal real** (A‑5) |
| T10 | Tiempo de asentamiento de la EMA (α = 0.4) | τ = 1.96 s; 0.1 % a los 14 s (A‑7) |
| T11 | Rechazo del rizo peristáltico de 3 rodillos | ±30 % → ±0.89 % |

`rampa.cpp` entrega el tiempo hasta `enRegimenEstable()` para cada consigna de RPM
(7.0 s a 20 RPM, 29.9 s a 100 RPM — hallazgo A‑1) y el tiempo real de frenado
(2.25 s desde 100 RPM — hallazgo A‑2).

## Nota sobre el código de salida

El arnés termina con `exit 1` y **"1 chequeo fallido"**. No es un defecto del arnés:
ese chequeo fallido es deliberado y corresponde al hallazgo **M‑1** documentado en la
auditoría — el comentario de `caudalimetro.cpp` promete un *"decaimiento suave y
asintótico … sin escalones"* y la medición muestra que no se cumple. El chequeo quedará
en verde cuando se aplique la corrección propuesta en el §1.2 del dictamen.

## Alcance y limitaciones

Esto valida la **lógica**. No valida el enlazado para Xtensa, el uso real de IRAM, el
tamaño del binario, ni el comportamiento de `portENTER_CRITICAL` en doble núcleo real —
para eso hace falta compilar con el core de ESP32, que no estaba disponible en el
entorno donde se realizó la auditoría. Ver §6 del dictamen.
