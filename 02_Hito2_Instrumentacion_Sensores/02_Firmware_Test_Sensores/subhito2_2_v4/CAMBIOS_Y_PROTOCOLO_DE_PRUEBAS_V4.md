# 📋 GUÍA DE CAMBIOS Y PROTOCOLO DE PRUEBAS DE BANCO — VERSIÓN V4
## Planta Piloto de Ultrafiltración Tangencial FX100 (Helixone®)
**Codirector**: Ing. Enzo  
**Tesistas**: Antonella Guitián & Owen Cañizares  
**Carrera**: Ingeniería Industrial / Química — Universidad Nacional de Salta (UNSa)  
**Fecha**: Octubre 2026  
**Ubicación**: `02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v4/`

---

# 1. Propósito y Alcance de este Documento

Este documento es una **guía operativa directa y protocolo de ensayos de banco** para que el equipo de trabajo (Owen, Antonella e Ing. Enzo) pueda:
1. Conocer exactamente **qué cambios se implementaron en el firmware `v4`** respecto a la versión `v3` utilizada en la calibración preliminar con probeta.
2. Comprender la justificación física y matemática de cada cambio para defenderlo con solidez técnica en la tesis de grado.
3. Ejecutar de forma metódica y ordenada el **protocolo experimental de laboratorio** para calibrar la bomba, poner a punto los dos caudalímetros (alimentación y permeado), contrastar con la balanza digital y registrar las curvas de filtración.

---

# 2. Resumen Detallado de Cambios Implementados en la Versión v4

| # | Módulo / Área | Cambio en Versión v4 | ¿Por qué se hizo? (Justificación Técnica) |
| :-: | :--- | :--- | :--- |
| **1** | **Metrología de Caudal** (`caudalimetro.cpp`) | **Medición por Período Recíproco en microsegundos**: <br>$f = \frac{(n - 1) \times 10^6}{t_{\text{último}} - t_{\text{primero}}}$ para $n \ge 2$, y $f = \frac{10^6}{\text{periodo\_us}}$ para $n = 1$. | En v3, el conteo simple por segundo ($f = n / \Delta t$) producía oscilaciones espurias de $\pm 30\%$ a bajo caudal (permeado $< 100\text{ mL/min}$) porque 1 pulso caía dentro o fuera de la ventana. Con período recíproco, la resolución temporal pasa de $1\text{ s}$ a $1\ \mu\text{s}$, eliminando por completo el error de discretización. |
| **2** | **Dinámica de Frenado** (`caudalimetro.cpp`) | **Cota Física Superior Continua**: <br>Si pasan $\Delta t_{\text{sin}}$ microsegundos sin pulsos, la frecuencia se acota por $f \le \frac{10^6}{\Delta t_{\text{sin}}}$. Corte definitivo a 0 tras $3\text{ s}$. | En v3, al apagar la bomba la frecuencia colapsaba bruscamente a 0 o quedaba congelada en lecturas fantasma. La cota física emula la desaceleración inercial suave del rotor de la turbina. |
| **3** | **Unidades Metrológicas** (`config.h`, `index_html.cpp`) | **Aclaración y Unificación de Unidades de $K$**: <br>Se unificó formalmente que $K$ se expresa en $[\text{Hz}/(\text{L/min})]$. <br>Equivalencia: $\text{Pulsos por Litro} = K \times 60$. <br>Corrección del rótulo en la pantalla SCADA de `pulsos/L` a `Hz/(L/min)`. | En notas preliminares se rotulaba erróneamente $K$ como "pulsos/Litro" cuando su valor era $154.62$. Un factor de 60 causaba confusión. La matemática del firmware siempre calculó $V = n / (K \times 60)$, lo que es 100% coherente con $K$ en $\text{Hz}/(\text{L/min})$. |
| **4** | **Diagnóstico de Hardware** (`caudalimetro.h/cpp`) | **Alarma Asimétrica de Pérdida de Señal**: <br>El parámetro `esAlimentacion` desactiva la alarma en el sensor de permeado cuando no hay flujo. | Si la bomba gira a más de $1\text{ RPM}$, en alimentación es obligatorio que haya pulsos; si pasan 5s sin señal, hay un cable cortado o manguera rota (alarma roja). En cambio, en permeado, si la TMP es baja o la válvula está cerrada, flujo 0 es normal; en v3 daba falsa alarma de error. |
| **5** | **Rango Cinemático** (`config.h`, `Bomba.cpp`) | **Ampliación de Techo a 100 RPM**: <br>Rango operativo fijado de $15.0$ a $100.0\text{ RPM}$ ($\approx 204$ a $1360\text{ mL/min}$). | El límite anterior de 44 RPM correspondía al uso clínico en sangre (600 mL/min). Para la tesis y los ensayos experimentales con agua limpia y diseño factorial $3^2$, se requiere explorar regímenes turbulentos/tangenciales altos en la membrana capilar. |
| **6** | **Modelo de Filtración** (`darcy.h`) | **Incorporación de Modelo Darcy-Vogel en Tiempo Real**: <br>Cálculo de viscosidad dinámica del agua con ecuación de Vogel ($5$ a $60^\circ\text{C}$), corrección por temperatura TCF ($J_{20}$), flujo superficial $J$ $[\text{LMH}]$, resistencia total $R_{\text{total}}$ y resistencia de torta $R_{\text{torta}}$. | Permite computar directamente en el microcontrolador la permeabilidad hidráulica real de la membrana y desacoplar el efecto de la temperatura ambiente del laboratorio. |
| **7** | **Servidor Web y SCADA** (`EN_USO_firmware_planta.ino`, `index_html.cpp`) | **Transmisión Chunked sin Fragmentación**: <br>Rutas `/status` y `/export_csv` utilizan buffers en stack y streaming directo sin objetos dinámicos `String`. | Previene el agotamiento de memoria dinámica (*heap fragmentation*) en ensayos de larga duración ($> 1\text{ hora}$) y acelera la respuesta del panel web a menos de $5\text{ ms}$. |
| **8** | **Seguridad Multinúcleo** (`caudalimetro.cpp`, `Bomba.cpp`) | **Secciones Críticas Atómicas con Cerrojos FreeRTOS**: <br>`portENTER_CRITICAL` y `portENTER_CRITICAL_ISR` en todas las lecturas de microsegundos y pulsos. | Garantiza inmunidad ante condiciones de carrera entre el Core 0 (servidor web / Wi-Fi) y el Core 1 (lazo cinemático e interrupciones de hardware). |

---

# 3. Protocolo Experimental de Banco de Pruebas (Paso a Paso)

Para poner a punto los sensores y re-calibrar la planta con el firmware v4, se deben ejecutar los siguientes 6 ensayos en orden:

---

## 🧪 ENSAYO 1: Comprobación Eléctrica, Red Wi-Fi y Giro de Bomba

### Objetivo:
Verificar que la electrónica de potencia, el microcontrolador ESP32, el driver DM860 y la interfaz SCADA respondan correctamente sin vibraciones ni pérdidas de pasos.

### Instrumental Requerido:
- Planta de ultrafiltración armada en banco.
- Fuente de alimentación de potencia ($24\text{ V}$ o $36\text{ V}$ / $5\text{ A}$ para el DM860).
- Teléfono móvil, tablet o notebook con conexión Wi-Fi.

### Procedimiento:
1. Energizar la electrónica de control y potencia de la planta.
2. Verificar el encendido del LED indicador en la placa NodeMCU-32S y los LEDs verde de encendido del driver DM860.
3. Desde la notebook o celular, buscar y conectarse a la red Wi-Fi:
   - **SSID**: `Bomba_Peristaltica_UF`
   - **Contraseña**: `plantapiloto2`
4. Abrir un navegador web (Chrome, Firefox o Edge) e ingresar a la dirección IP:
   - **URL**: `http://192.168.4.1` (o alternativamente `http://bomba.local`).
5. Verificar que el panel de control cargue en modo oscuro con todos los relojes e indicadores en 0.
6. En el deslizador de velocidad, colocar una consigna baja de **$20.0\text{ RPM}$** y presionar **START**.
   - Comprobar que el motor NEMA 34 arranca suavemente (rampa de $2.0\text{ RPM/s}$) sin golpes de torque ni chillidos mecánicos.
   - Observar que el LED azul onboard del pin 2 se enciende como testigo de bomba en marcha.
7. Presionar el botón **INVERTIR SENTIDO (DIR)**:
   - Constatar que la bomba desacelera progresivamente hasta llegar exactamente a $0.0\text{ RPM}$, conmuta el sentido de giro por software de manera segura, y vuelve a acelerar en sentido contrario hasta $20.0\text{ RPM}$.
8. Presionar **STOP**:
   - Verificar que la bomba se clava rápidamente en menos de $1.5\text{ segundos}$ gracias a la rampa de frenado de parada ($45.0\text{ RPM/s}$).

---

## 🧪 ENSAYO 2: Calibración Volumétrica de la Bomba Peristáltica ($\text{mL/rev}$)

### Objetivo:
Determinar el desplazamiento volumétrico real por revolución ($V_{\text{vuelta}}$) de la manguera de silicona ($\varnothing_{\text{int}} = 12\text{ mm}$) en el cabezal MBP-2000 a descarga libre, eliminando cualquier error de calibración anterior.

### Instrumental Requerido:
- Probeta graduada de vidrio de $500\text{ mL}$ o $1000\text{ mL}$.
- Balde o reservorio de agua desionizada/destilada limpia a temperatura ambiente.
- Cronómetro digital (o el contador de tiempo del SCADA).

```
  [Tanque Agua] ──(Succión)──► [Cabezal Peristáltico MBP-2000] ──(Descarga Libre)──► [Probeta Graduada]
```

### Procedimiento:
1. Desconectar la manguera de impulsión de la entrada de la membrana y colocar el extremo libre descargando directamente dentro de la probeta graduada.
2. Realizar cuatro corridas de prueba a distintos niveles de RPM durante un tiempo cronometrado exacto de **$2\text{ minutos}$ ($120\text{ segundos}$)** cada una:
   - **Corrida A**: $30.0\text{ RPM}$
   - **Corrida B**: $50.0\text{ RPM}$
   - **Corrida C**: $70.0\text{ RPM}$
   - **Corrida D**: $90.0\text{ RPM}$
3. En cada corrida: vaciar la probeta, encender la bomba con el botón START, arrancar el cronómetro al alcanzar las RPM de régimen, y presionar STOP al cumplirse los 120 segundos.
4. Leer y anotar el volumen recolectado ($V_{\text{probeta}}$ en $\text{mL}$).
5. Calcular el desplazamiento volumétrico específico para cada corrida:
   $$V_{\text{vuelta}, i} = \frac{V_{\text{probeta}, i}}{\text{RPM}_i \times 2.0\text{ min}}\quad [\text{mL/rev}]$$
6. Promediar los cuatro valores para obtener el $V_{\text{vuelta, medio}}$.
7. **Carga en el Firmware**:
   - En la interfaz web, abrir el modal **"Modo Desarrollador"**.
   - En el campo `Cilindrada Bomba (mL/rev)`, escribir el valor obtenido (ej. `13.60`).
   - Tildar la casilla **"Guardar en Memoria Flash (NVS)"** y hacer clic en **Guardar Cambios**.
   - Constatar que en la consola Serial del ESP32 aparezca el mensaje: `[NVS] Parametros guardados en memoria Flash con exito`.

---

## 🧪 ENSAYO 3: Calibración y Puesta a Punto del Caudalímetro de Alimentación ($K_{\text{alim}}$)

### Objetivo:
Ajustar la constante de calibración $K_{\text{alim}}$ $[\text{Hz}/(\text{L/min})]$ del sensor YF-S401 de la línea de impulsión, verificando la linealidad entre la frecuencia de pulsos generada y el caudal real medido por probeta.

### Instrumental Requerido:
- Sensor YF-S401 conectado en el borne 12 superior (GPIO 14).
- Circuito hidráulico en serie: Bomba $\rightarrow$ Sensor de Alimentación $\rightarrow$ Probeta.

```
  [Bomba Peristáltica] ──► [Sensor Alimentación YF-S401] ──► [Probeta Graduada]
```

### Procedimiento:
1. Purgar completamente la línea de manguera hasta que no queden burbujas de aire atrapadas en el cuerpo de la turbina del caudalímetro.
2. Realizar corridas de **$1\text{ minuto}$ ($60\text{ segundos}$)** a cinco velocidades de bomba:
   - **Punto 1**: $20\text{ RPM}$ ($Q \approx 270\text{ mL/min}$)
   - **Punto 2**: $40\text{ RPM}$ ($Q \approx 540\text{ mL/min}$)
   - **Punto 3**: $50\text{ RPM}$ ($Q \approx 680\text{ mL/min}$)
   - **Punto 4**: $70\text{ RPM}$ ($Q \approx 950\text{ mL/min}$)
   - **Punto 5**: $90\text{ RPM}$ ($Q \approx 1220\text{ mL/min}$)
3. Durante cada corrida, observar en el SCADA y registrar en la planilla:
   - La frecuencia promedio en Hertz ($F_{\text{alim}}$).
   - El volumen recolectado en la probeta en 1 minuto ($Q_{\text{real}}$ en $\text{mL/min}$).
4. Calcular el factor puntual para cada escalón:
   $$K_i = \frac{F_{\text{alim}, i}\ [\text{Hz}] \times 1000}{Q_{\text{real}, i}\ [\text{mL/min}]}\quad \left[\frac{\text{Hz}}{\text{L/min}}\right]$$
5. En una planilla Excel, graficar $F_{\text{alim}}$ en el eje $Y$ vs. $Q_{\text{real}} / 1000$ en el eje $X$. La pendiente de la recta forzada por el origen ($y = m \cdot x$) es el **$K_{\text{alim}}$ definitivo**.
6. Cargar el valor resultante en el SCADA (Modo Desarrollador $\rightarrow$ `Factor K Alimentación`) y presionar Guardar.
7. **Prueba de Diagnóstico de Cable Cortado**:
   - Con la bomba girando a 50 RPM y agua circulando, desconectar deliberadamente el cable de señal del sensor de alimentación de la bornera.
   - Constatar que a los **5 segundos exactos** el indicador en pantalla cambia a color rojo con la leyenda **`SIN SEÑAL`** y el registro alerta de pérdida de flujo.
   - Reconectar el cable: verificar que el sensor recupera la lectura en menos de 1 segundo de forma automática.

---

## 🧪 ENSAYO 4: Calibración Gravimétrica del Caudalímetro de Permeado ($K_{\text{perm}}$)

### Objetivo:
Caracterizar el comportamiento del sensor YF-S401 de permeado a caudales bajos ($< 350\text{ mL/min}$) y contrastar contra el método patrón primario gravimétrico de laboratorio.

### Instrumental Requerido:
- Balanza analítica o digital de precisión ($0.1\text{ g}$ o $0.01\text{ g}$).
- Recipiente colector de vidrio o plástico liviano tarado.
- Filtro de ultrafiltración FX100 montado en el circuito con su válvula de contrapresión de retentado.

```
                         ┌──► [Válvula Aguja Retentado] ──► [Tanque Retorno]
  [Bomba] ──► [FX100] ──┤
                         └──► [Sensor Permeado] ──► [Vaso sobre Balanza Digital]
```

### Procedimiento:
1. Conectar la línea de permeado del filtro capilar FX100 al sensor de permeado (GPIO 27) y derivar la salida hacia el vaso colector colocado sobre la balanza digital.
2. Encender la bomba a $50.0\text{ RPM}$ con recirculación de retentado.
3. Estrangular suavemente la válvula de aguja del retentado para generar tres niveles estables de presión transmembrana (TMP):
   - **Nivel Bajo**: TMP $\approx 0.15\text{ bar}$ ($Q_{\text{perm}} \approx 80-100\text{ mL/min}$)
   - **Nivel Medio**: TMP $\approx 0.30\text{ bar}$ ($Q_{\text{perm}} \approx 180-220\text{ mL/min}$)
   - **Nivel Alto**: TMP $\approx 0.45\text{ bar}$ ($Q_{\text{perm}} \approx 300-350\text{ mL/min}$)
4. En cada nivel, tarar la balanza a cero, presionar el botón **"Reset Vol"** en el panel web para poner en cero el totalizador del SCADA, y cronometrar **$3\text{ minutos}$ ($180\text{ segundos}$)**.
5. Al finalizar los 3 minutos:
   - Leer la masa acumulada en la balanza: $m_{\text{balanza}}$ en gramos. (Como $\rho_{\text{agua}} \approx 1.00\text{ g/mL}$, $V_{\text{balanza}}\ [\text{mL}] \approx m_{\text{balanza}}$).
   - Leer el volumen integrado en la pantalla web: $V_{\text{SCADA}}\ [\text{Litros}] \times 1000$.
6. Calcular el factor corregido:
   $$K_{\text{perm, nuevo}} = K_{\text{perm, actual}} \times \frac{V_{\text{SCADA}}\ [\text{mL}]}{V_{\text{balanza}}\ [\text{mL}]}$$
7. Guardar el valor final de $K_{\text{perm}}$ en la memoria Flash NVS desde el panel web.
8. **Comprobación de Alarma en Reposo**:
   - Cerrar completamente la llave de paso del permeado ($Q_{\text{perm}} = 0$).
   - Verificar que en el panel SCADA la tarjeta de permeado marca `0.0 mL/min` pero **permanece en estado verde / OK**, sin disparar falsas alarmas de cable cortado.

---

## 🧪 ENSAYO 5: Verificación Dinámica de Cota Física y Parada Rápida

### Objetivo:
Demostrar que la nueva **cota física continua** del firmware v4 extingue el caudal de forma asintótica y suave al frenar la bomba, sin saltos bruscos ni caudales "fantasma".

### Procedimiento:
1. Conectar la computadora a la placa NodeMCU-32S mediante el cable micro-USB y abrir el **Monitor Serial de Arduino IDE** a **$115200\text{ baudios}$**.
2. Desde la página web, colocar la bomba a **$60.0\text{ RPM}$** y encender con **START**.
3. Aguardar $30\text{ segundos}$ hasta que el sistema alcance el régimen permanente (frecuencias y caudales estables en el monitor serie).
4. Presionar el botón **STOP** súbitamente.
5. Observar en el Monitor Serial la secuencia de telemetría:
   - Constatar que la bomba frena en $< 1.5\text{ s}$.
   - Verificar que en ausencia de pulsos, el algoritmo de cota física no congela el valor anterior, sino que reduce progresivamente la frecuencia calculada:
     $$\Delta t_{\text{sin}} \approx 1\text{ s} \implies f \le 1.0\text{ Hz} \quad\longrightarrow\quad \Delta t_{\text{sin}} \approx 2\text{ s} \implies f \le 0.5\text{ Hz} \quad\longrightarrow\quad \Delta t_{\text{sin}} > 3\text{ s} \implies f = 0.0\text{ Hz}$$
   - Confirmar que tras 3 segundos de parada no queda ningún valor remanente distinto de cero.

---

## 🧪 ENSAYO 6: Prueba del Datalogger Multi-Sesión y Exportación CSV a Excel

### Objetivo:
Validar que el datalogger integrado guarde las muestras cada 10 segundos en memoria RAM y permita la descarga limpia de la sesión en archivo `.CSV` listo para Excel.

### Procedimiento:
1. En el panel SCADA, hacer clic en el botón **"Borrar Historial"** para reiniciar los buffers.
2. Iniciar una corrida de prueba de **$5\text{ minutos}$** con la siguiente secuencia:
   - Minuto 0 a 2: Consigna de $30.0\text{ RPM}$.
   - Minuto 2 a 4: Subir a $60.0\text{ RPM}$.
   - Minuto 4 a 5: Bajar a $20.0\text{ RPM}$ y luego presionar STOP.
3. Observar en la parte inferior del panel web la sección de **"Historial de Ensayos Registrados"**: debe figurar el `Ensayo #1` con su duración, muestras tomadas y volúmenes acumulados.
4. Hacer clic en el botón **"Descargar CSV (Excel)"**.
5. Abrir el archivo descargado (`PlantaUF_Calibracion_Ensayo_1.csv` o similar) con Microsoft Excel:
   - Verificar que las columnas se separen automáticamente gracias a la directiva `sep=;`.
   - Constatar que los campos de balance de masa (`Q_Retentado_mLmin`, `Recuperacion_Y_Pct`), de cinemática (`RPM_Bomba`, `Q_Bomba_Teorico`) y de membrana (`J_LMH`) contengan valores coherentes y continuos.

---

# 4. Planilla de Registro de Ensayos de Laboratorio

*(Esta sección está diseñada para imprimirse o completarse en tablet en el laboratorio de la UNSa)*

### 📝 Registro del Ensayo 2: Cilindrada de la Bomba ($V_{\text{vuelta}}$)
- **Fecha**: ____ / ____ / 2026
- **Operadores**: Owen Cañizares / Antonella Guitián / Enzo
- **Temperatura del Agua**: ________ $^\circ\text{C}$

| Corrida | Velocidad [RPM] | Tiempo [s] | Volumen Probeta [mL] | Cilindrada Calculada [mL/rev] |
| :---: | :---: | :---: | :---: | :---: |
| **A** | $30.0$ | $120$ | | |
| **B** | $50.0$ | $120$ | | |
| **C** | $70.0$ | $120$ | | |
| **D** | $90.0$ | $120$ | | |
| **PROMEDIO** | — | — | — | **$V_{\text{vuelta}} =$ ________________ mL/rev** |

---

### 📝 Registro del Ensayo 3: Calibración Sensor Alimentación ($K_{\text{alim}}$)
- **Valor Inicial en Firmware**: $154.62\text{ Hz/(L/min)}$ ($9277.2\text{ pulsos/L}$)

| Punto | RPM | Frecuencia SCADA [Hz] | Vol. Probeta en 1 min [mL] | Caudal Real [L/min] | Factor $K_i$ [Hz/(L/min)] |
| :---: | :---: | :---: | :---: | :---: | :---: |
| **1** | $20.0$ | | | | |
| **2** | $40.0$ | | | | |
| **3** | $50.0$ | | | | |
| **4** | $70.0$ | | | | |
| **5** | $90.0$ | | | | |
| **FINAL** | — | — | **Pendiente Regresión Lineal:** | — | **$K_{\text{alim}} =$ ________________ Hz/(L/min)** |

---

### 📝 Registro del Ensayo 4: Calibración Gravimétrica Permeado ($K_{\text{perm}}$)
- **Valor Inicial en Firmware**: $55.00\text{ Hz/(L/min)}$ ($3300.0\text{ pulsos/L}$)
- **Tiempo de Cada Corrida**: $180\text{ segundos}$ ($3.0\text{ min}$)

| Nivel TMP | TMP Indicada [bar] | Masa Balanza $\Delta m$ [g] | Vol. Balanza [mL] | Vol. SCADA [mL] | Error Relativo [%] | $K_{\text{perm}}$ Corregido |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Bajo** | $\approx 0.15$ | | | | | |
| **Medio** | $\approx 0.30$ | | | | | |
| **Alto** | $\approx 0.45$ | | | | | |
| **DEFINITIVO**| — | — | — | — | — | **$K_{\text{perm}} =$ ________________ Hz/(L/min)** |

---

> [!TIP]
> **Recomendación para la Escritura de la Tesis**:  
> Con los datos completados en estas planillas y los archivos `.CSV` descargados, Owen y Antonella tendrán todo el material experimental requerido para redactar el capítulo de **"Validación Metrológica e Instrumentación de la Planta Piloto"** de su tesis de grado con rigor científico de estándar internacional.
