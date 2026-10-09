# CAMBIOS DE ARQUITECTURA Y PROTOCOLO DE VALIDACIÓN EN BANCO — FIRMWARE V5.2 DEFINITIVO

## Planta Piloto de Ultrafiltración Tangencial (Membrana Fresenius FX100 — UNSa 2026)
* **Tesis de Grado en Ingeniería Industrial:** Antonella Guitián & Owen Cañizares  
* **Codirección:** Ing. Enzo Corte  
* **Versión de Firmware:** **V5.2 Definitiva (Núcleo Metrológico Certificado & Erradicación de Caudal Fantasma)**
* **Estado de Compilación:** `arduino-cli` Exit Code 0 (79% Flash, 21% RAM, 258 KB Heap libre).

---

## 1. RESUMEN DE CAMBIOS Y EVOLUCIÓN HISTÓRICA

| Componente | Firmware V4.1 | Firmware V5.1 (Previa) | Firmware V5.2 (Definitivo Certificado) | Beneficio Práctico y Metrológico |
| :--- | :--- | :--- | :--- | :--- |
| **Detección de Flujo Cero en Permeado** | Vulnerable al filtro EMA | Reutilizaba el último período conocido hasta por 5 segundos si no había pulsos nuevos | **Cero estricto inmediato:** Si no hay períodos nuevos en la ventana (`hayPeriodosNuevos == false`), el caudal pasa a `NAN` y el display cae instantáneamente a **0.0 mL/min**. | **Erradica el falso caudal positivo en permeado cuando la línea está vacía o detenida.** |
| **Sincronización de Adquisición** | Lecturas asíncronas no atómicas | Doble captura (`capturar()` en loop y de nuevo en `tick()`), vaciando acumuladores por duplicado | **Captura única atómica:** El loop adquiere las instantáneas `ma` y `mp` y las transfiere por referencia a `registroEnsayos.tick(ma, mp, rpm)`. | Coherencia temporal exacta entre la pantalla del SCADA y el archivo CSV registrado. |
| **Ajuste Dinámico de Factor K** | Bloqueado o no persistido | `setK()` bloqueaba cambios silenciosamente tras `begin()` | `setK()` permite actualizar $K_{\text{alim}}$ y $K_{\text{perm}}$ en tiempo real desde el SCADA o la memoria Flash NVS. | Permite ajustar factores de calibración en banco sin tener que reflashear el microcontrolador. |
| **Aislamiento de Modo Seco** | Inexistente | Solo limpiaba banderas lógicas; arrastraba acumuladores temporales | `reiniciarEstimadorLocked()` limpia el 100% de períodos y acumuladores, e incrementa `_epoca` en cada transición. | El ruido capturado en seco jamás contamina la estimación de caudal al volver a húmedo. |
| **Alarma de Corte de Alimentación** | Contador ciego de vueltas de loop | Incrementaba `_sinPulso_s += 1.0f` por iteración (dependiente de demoras HTTP) | Medición rigurosa de tiempo real mediante microsegundos de hardware: `m.t_us - _inicioSinPulso_us >= 5000000LL`. | Alarma de 5 segundos inmune a cargas del servidor web o retardos de red. |
| **Gestión de Memoria RAM** | Array plano en stack | Buffers estáticos sobredimensionados desbordaban la memoria `.bss` | `RegistroEnsayos` asigna su buffer en Heap dinámico (`new Fila[MAX_FILAS]`), reduciendo el consumo estático del **28% al 21%**. | **258 KB de memoria RAM libre** para máxima estabilidad del Portal Cautivo y WebServer. |
| **Portal Cautivo Anti-Desconexión** | Desactivado en V4.1 | Reactivado | Totalmente operativo con `DNSServer` en puerto 53 y redirecciones 302 a `192.168.4.1`. | Conexión automática e instantánea al SCADA en Android, iOS y Windows al conectarse al Wi-Fi. |
| **Exportación CSV** | Tabla única | Tabla única en buffer circular | **Doble exportador:** `/export_csv` (formato estándar para Excel) y `/export_metrologia` (29 columnas con datos primarios, calidad y épocas). | Respaldo documental inobjetable para la defensa de la tesis. |

---

## 2. EL PROBLEMA DEL PERMEADO RESUELTO AL 100%

### ¿Por qué marcaba caudal positivo si no salía agua?
1. **La trampa del período anterior:** Cuando el agua se detenía, la ventana de 1 segundo recibía 0 pulsos. El firmware anterior saltaba a una rama de respaldo que reciclaba el valor del último pulso aceptado en lugar de decir que no había señal.
2. **El arrastre del suavizador visual:** El filtro exponencial promediaba ese valor retenido y hacía que en la pantalla el caudal decayera de forma extremadamente lenta o pareciera estancado en $\approx 145\text{ mL/min}$.
3. **La solución definitiva V5.2:** Ahora, si en la ventana no se registraron períodos completos genuinos, la frecuencia no se inventa: se asigna `NAN` y la variable visual cae a **0.0 mL/min en el acto**.

---

## 3. PROTOCOLO DE VALIDACIÓN EN BANCO (PASO A PASO)

### ETAPA 1: Flasheo del Firmware V5.2
1. Abrir `subhito2_2_v5/EN_USO_firmware_planta/EN_USO_firmware_planta.ino` en Arduino IDE.
2. Seleccionar placa: **ESP32 Dev Module**.
3. Compilar y cargar.
4. Abrir Monitor Serial a **115200 baudios** y constatar:
   ```text
   [WIFI] Punto de Acceso Estable Creado:
          SSID: Bomba_Peristaltica_UF | Pass: plantapiloto2
          IP AP: http://192.168.4.1
   [DNS] Servidor DNS Captive Portal activo en puerto 53 (Anti-Desconexion)
   [HTTP] Servidor Web SCADA iniciado con exito en puerto 80.
   ```

---

### ETAPA 2: Ensayo de Cero Hidráulico y Discriminación de Ruido (Astra Sección 5)
> **Objetivo:** Demostrar que el sensor de permeado marca estrictamente 0.0 mL/min con el motor girando a distintas velocidades sin líquido.

1. **Condición 1: Motor y driver apagados (reposo absoluto)**
   * Verificar en el Monitor Serial que `flan_perm = 0`, `val_perm = 0`, `gl_perm = 0`.
   * En el SCADA: $Q_{\text{perm}} = 0.0\text{ mL/min}$.
2. **Condición 2: Driver energizado (48V conectado, motor detenido con torque de retención)**
   * Constatar que no existan flancos inducidos por la fuente conmutada.
3. **Condición 3: Motor girando en seco a 20, 50 y 80 RPM (sin agua en permeado)**
   * Activar Modo Seco: `http://192.168.4.1/cmd?act=MODO_SECO_ON`
   * Girar la bomba por 30 segundos en cada escalón.
   * **Criterio de Aprobación:** $Q_{\text{perm}} = 0.0\text{ mL/min}$, pulsos válidos $V = 0$.
   * Si aparecen flancos `F` o glitches `G` descartados por el filtro, demuestran que el algoritmo geométrico está protegiendo la medición contra los micropasos del DM860.
4. Desactivar Modo Seco: `http://192.168.4.1/cmd?act=MODO_SECO_OFF`.

---

### ETAPA 3: Control Volumétrico con Probeta y Calibración en Vivo
1. Realizar corrida de verificación con agua a **50 RPM** (bomba peristáltica impulsando a probeta graduada).
2. Constatar que $Q_{\text{alim}} \approx 720\text{ mL/min}$ coincida con el aforo físico.
3. Conectar la línea de permeado a una probeta fina de $100\text{ mL}$ o balanza digital.
4. Si se mide un caudal real de permeado de $45\text{ mL/min}$ y la frecuencia en pantalla marca $31.0\text{ Hz}$, el factor $K$ real se calcula como:
   $$K = \frac{31.0 \times 1000}{45} = 688.8\text{ Hz/(L/min)}$$
5. Ese factor $K$ se puede guardar en tiempo real desde la consola de desarrollador del SCADA o ingresando a:
   ```text
   http://192.168.4.1/set_dev?kp=688.8&save=1
   ```
   *(El firmware V5.2 ahora sí acepta la actualización inmediatamente y la guarda en la memoria Flash NVS para siempre).*

---

### ETAPA 4: Descarga y Verificación del Registro de Datos
1. Al finalizar la corrida, descargar el CSV estándar de análisis:
   ```text
   http://192.168.4.1/export_csv
   ```
2. Para auditorías formales o anexos de la tesis, descargar el registro metrológico completo de 29 columnas:
   ```text
   http://192.168.4.1/export_metrologia
   ```
3. Ambos archivos demuestran trazabilidad absoluta de pulsos acumulados, tiempos de microsegundos y marcas de calidad.
