# 🌀 HITO 2: Accionamiento de Precisión de la Bomba Peristáltica (NEMA 34 + DM860)

Este hito constituye el corazón del sistema hidráulico de la planta de ultrafiltración. Su objetivo es lograr el control milimétrico de la velocidad de giro (0 a 120 RPM), rampa de aceleración suave, inversión de sentido (Filtración / Retrolavado) y entender a fondo la física y electrónica del driver Leadshine DM860.

---

## 📚 1. Fundamento Teórico y Diagnóstico de Fallas

### A. ¿Por qué se calentaba el motor NEMA 34 en las primeras pruebas?
Un motor paso a paso consume corriente incluso estando detenido para mantener su torque de retención (*Holding Torque*).
* En el lateral del driver DM860, el interruptor **SW4** controla la reducción de corriente en reposo (*Standstill Current*):
  * **SW4 en ON (100% permanente)**: Inyecta corriente máxima continua a las bobinas aun con la bomba parada. Por efecto Joule ($P = I^2 R$), el motor hierve a $> 80^\circ\text{C}$ en pocos minutos y zumba fuerte.
  * **SW4 en OFF (50% en reposo) ➔ [CONFIGURACIÓN CORRECTA]**: Cuando el motor se detiene, el driver baja la corriente automáticamente a la mitad. **El motor trabaja frío ($\approx 28-35^\circ\text{C}$) y seguro**.

### B. ¿Por qué hacía ruidos extraños cuando se conectó a 5V?
* El ESP32 trabaja con niveles lógicos de **$3.3\text{V}$**, mientras que el driver DM860 está diseñado internamente para señales de $5\text{V}$ (resistencia interna $R_{\text{in}} = 270\,\Omega$ y diodo optoacoplador $V_F = 1.2\,\text{V}$).
* Si se conecta en **Ánodo Común a $+5\text{V}$** y los pines negativos al ESP32:
  * Cuando el ESP32 envía `HIGH` ($3.3\text{V}$), la diferencia de tensión es $5\text{V} - 3.3\text{V} = 1.7\text{V}$.
  * Como $1.7\text{V} > 1.2\text{V}$, **el optoacoplador NUNCA se apaga del todo (conducción parásita)**. Esto deforma los pulsos, hace perder pasos y genera un gruñido/vibración violenta.
* **Solución (Cátodo Común a GND)**:
  * Conectar `PUL-` y `DIR-` a la masa (`GND`) del ESP32.
  * Conectar `PUL+` a `GPIO 18` y `DIR+` a `GPIO 19`.
  * Cuando el ESP32 manda `3.3V` entrega $\approx 7.8\text{ mA}$ (encendido nítido). Cuando manda `0.0V`, la corriente es $0.0\text{ mA}$ (apagado perfecto).

---

## ⚙️ 2. Configuración de los DIP Switches del Driver Leadshine DM860

Configurar los 8 microswitches en esta posición con la fuente apagada:

```
┌───────────────┬───────────────────────────┬─────────────────────────────────────────────────┐
│ Switch        │ Posición Requerida        │ Función Técnica                                 │
├───────────────┼───────────────────────────┼─────────────────────────────────────────────────┤
│ SW1, SW2, SW3 │ OFF - ON - ON             │ Corriente RMS: ~3.8A Peak (Ideal NEMA 34).      │
├───────────────┼───────────────────────────┼─────────────────────────────────────────────────┤
│ SW4           │ ❗ OFF (OBLIGATORIO) ❗   │ 50% Standstill Current (Elimina sobrecalor).   │
├───────────────┼───────────────────────────┼─────────────────────────────────────────────────┤
│ SW5, SW6,     │ ON - OFF - ON - ON        │ 1600 micropasos/rev (Movimiento suave sin       │
│ SW7, SW8      │                           │ resonancia acústica).                           │
└───────────────┴───────────────────────────┴─────────────────────────────────────────────────┘
```

---

## 🔌 3. Diagrama de Conexionado Físico

```
       ESP32 DevKit V1 (38 Pines)                  DRIVER LEADSHINE DM860
     ┌────────────────────────────┐              ┌────────────────────────┐
     │           GPIO 18 (D18) ├───► (Borne 1) ─►│ PUL+ (Paso / STEP)     │
     │           GPIO 19 (D19) ├───► (Borne 2) ─►│ DIR+ (Dirección)       │
     │                     GND ├───► (Borne 3) ─┬►│ PUL- (Masa Cátodo)     │
     │                            │              └►│ DIR- (Masa Cátodo)     │
     │ (Sin Conectar)             │               │                        │
     │                            │               │ ENA+ / ENA- (SUELTOS)  │ ◄── Habilitado siempre
     └────────────────────────────┘               │                        │
                                                  │ AC / AC ◄──────────────┼──── Salida Transformador AC
                                                  │ A+ / A- ───────────────┼──── Fase A Motor NEMA 34
                                                  │ B+ / B- ───────────────┼──── Fase B Motor NEMA 34
                                                  └────────────────────────┘
```

---

## 📐 4. Ecuación Matemática de Frecuencia de Pulsos (LEDC Hardware)

Para girar a una velocidad dada de $\text{RPM}$ con el driver configurado en $1600\text{ pulsos/rev}$:

$$f (\text{Hz}) = \frac{\text{RPM} \times 1600\text{ pulsos}}{60\text{ segundos}} = \text{RPM} \times 26.6667\text{ Hz}$$

* A $30\text{ RPM} \rightarrow f = 800\text{ Hz}$.
* A $60\text{ RPM} \rightarrow f = 1600\text{ Hz}$.
* A $120\text{ RPM} \rightarrow f = 3200\text{ Hz}$.

> [!TIP]
> El firmware utiliza la librería nativa de hardware **LEDC Timer (`ledcWriteTone`)** del ESP32. Esto genera un tren de ondas cuadradas perfecto en silicio a **0% de carga de CPU**, garantizando que el motor gire con total suavidad aunque se transmitan datos por Wi-Fi o se atiendan conexiones web simultáneas.

---

## 💻 5. Instrucciones de Uso del Firmware

1. Abre el sketch [`firmware_bomba_nema34.ino`](./firmware_bomba_nema34/firmware_bomba_nema34.ino) en Arduino IDE.
2. Selecciona la placa **`ESP32 Dev Module`** y el puerto COM correspondiente.
3. Haz clic en **Subir (➔)**.
4. Abre el Monitor Serie a **115200 baudios** o accede desde el celular/PC a:
   * **`http://bomba-uf.local`** (o a la IP que imprima en consola, ej. `http://192.168.50.37`).
5. **Comandos por Consola Serie**:
   * Escribir `S` y presionar Enter ➔ Iniciar bomba a 30 RPM.
   * Escribir `X` y presionar Enter ➔ Detener bomba con desaceleración suave.
   * Escribir `R` y presionar Enter ➔ Invertir sentido de giro (Horario / Antihorario).
   * Escribir `V60` y presionar Enter ➔ Fijar velocidad a 60 RPM.
