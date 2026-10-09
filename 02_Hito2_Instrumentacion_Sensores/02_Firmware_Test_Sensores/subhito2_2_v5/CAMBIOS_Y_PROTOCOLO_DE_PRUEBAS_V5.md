# CAMBIOS Y PROTOCOLO DE VALIDACIÓN EN BANCO — FIRMWARE V5.0

## Planta Piloto de Ultrafiltración Tangencial (Membrana FX100 — UNSa)
**Tesis de Grado:** Antonella Guitián & Owen Cañizares  
**Codirección:** Ing. Enzo Corte  
**Versión de Firmware:** V5.0 (Motor Anti-EMI por Validación de Ancho de Nivel & Datalogger 10s)

---

## 1. RESUMEN DE CAMBIOS RESPECTO A V4.1

| Componente | Firmware V4.1 (Previo) | Firmware V5.0 (Actual) | Beneficio Práctico |
| :--- | :--- | :--- | :--- |
| **Detección de Pulsos** | `FALLING` con blanking ciego de 1500 µs | `CHANGE` con medición geométrica de anchos LOW y HIGH | Rechaza 100% de picos EMI del motor y chattering del filtro RC. |
| **Filtro de Glitches** | Control de relación $dt_{\max}/dt_{\min} \le 4.0$ (vulnerable al chopper regular) | $t_{\text{low}} \ge 200/600\text{ }\mu\text{s}$ y $t_{\text{high}} \ge 200/600\text{ }\mu\text{s}$ | Inmunidad total al ruido periódico generado por los 3200 micropasos del driver DM860. |
| **Zona Muerta (*Deadband*)** | Simétrica ($30.0\text{ mL/min}$ para ambos canales) | Asimétrica ($30.0\text{ mL/min}$ Alim / $4.0\text{ mL/min}$ Perm) | No ciega el sensor de permeado en corridas con agua limpia (10 a 50 mL/min). |
| **Compuerta de Sanidad (*Sanity Gate*)** | Inexistente (Permeado integraba ruido con bomba apagada) | Si bomba $< 20\text{ mL/min}$ o apagada $\rightarrow Q_{\text{perm}} = 0.0\text{ mL/min}$ | Evita flujo fantasma cuando no hay impulso hidráulico transmembrana. |
| **Parada de Bomba** | Decaimiento asintótico lento (cola de 10s) | Corte inmediato a $0.0\text{ mL/min}$ | Sin acumulación espuria de volumen tras pulsar STOP. |
| **Datalogger** | Muestreo a 1 Hz (máx 10 minutos) | Muestreo a 10 s (100 minutos continuos) + muestra en $t=0\text{ s}$ | Registra la prueba completa desde 20 hasta 90 RPM sin sobreescribir datos. |
| **Modelo de Darcy** | Presión simulada de 0.20 bar | En reposo ($valido = false$, $\text{TMP}=0$) hasta Subhito 2.3 | Preserva la integridad científica de los datos de la tesis. |
| **Diagnóstico de Ruido** | Pulsos acumulados brutos sin detalle | Flancos brutos, pulsos válidos y glitches descartados en `/status` | Visibilidad en tiempo real del blindaje contra ruido en la app. |
| **Modo Seco Enclavable** | Inexistente | Comandos `/cmd?act=MODO_SECO_ON` / `OFF` | Permite certificar el blindaje en seco a 80 RPM sin agua. |

---

## 2. PROTOCOLO DE CERTIFICACIÓN EXPERIMENTAL (ENZO & ANTONELLA)

### ETAPA 1: Flasheo y Conexión
1. Abrir `subhito2_2_v5/EN_USO_firmware_planta/EN_USO_firmware_planta.ino` en Arduino IDE.
2. Seleccionar la placa: **ESP32 Dev Module** (Puerto COM correspondiente).
3. Compilar y subir el firmware.
4. Abrir el Monitor Serial a **115200 baudios**.
5. Verificar el mensaje de bienvenida:
   ```text
   ==================================================
    PLANTA PILOTO DE ULTRAFILTRACIÓN FX100 — UNSa   
    Firmware V5.0: Motor Anti-EMI & Datalogger 10s   
   ==================================================
   [WIFI] Punto de Acceso Estable Creado:
          SSID: Bomba_Peristaltica_UF | Pass: plantapiloto2
          IP AP: http://192.168.4.1
   ```

---

### ETAPA 2: Ensayo de Calificación en Seco (Sin Agua)
> **Objetivo:** Demostrar que el motor paso a paso girando a alta velocidad no induce pulsos fantasma en el caudalímetro de permeado ni en el de alimentación.

1. Conectar celular o notebook a la red WiFi `Bomba_Peristaltica_UF`.
2. En el navegador web, activar el Modo Seco ingresando a:
   ```text
   http://192.168.4.1/cmd?act=MODO_SECO_ON
   ```
   *(El monitor serial confirmará `[AUDITORIA] Modo Seco ACTIVADO`).*
3. Abrir la interfaz SCADA en `http://192.168.4.1`.
4. Encender la bomba y configurar escalones de velocidad:
   * **20 RPM** por 30 segundos.
   * **40 RPM** por 30 segundos.
   * **60 RPM** por 30 segundos.
   * **80 RPM** por 30 segundos.
5. **Criterio de Éxito:**
   * En la interfaz web: Ambos caudales deben permanecer estrictamente en **0.0 mL/min**.
   * En la telemetría del monitor serial:
     ```text
     [TELEMETRIA] RPM: 80.0 | ESTABLE | Q_Alim: 0.0 mL/min (F:..., V:0, G:...) | Q_Perm: 0.0 mL/min (F:..., V:0, G:...)
     ```
     *Nota:* Si el chopper induce interferencias electromagnéticas, verán que el contador de flancos `F` o glitches `G` aumenta, pero los pulsos válidos `V` permanecen en **0**.
6. Desactivar el Modo Seco al finalizar:
   ```text
   http://192.168.4.1/cmd?act=MODO_SECO_OFF
   ```

---

### ETAPA 3: Calibración Volumétrica con Probeta Graduada (Línea Hidráulica)
> **Objetivo:** Validar la curva de respuesta y calibrar los factores $K$ y cilindrada real de la bomba.

1. Conectar la manguera de alimentación al circuito de agua con probeta graduada y cronómetro.
2. La línea de permeado permanece cerrada o conectada a su salida normal.
3. Iniciar el ensayo en la web SCADA a **20 RPM** y avanzar cada 1 o 2 minutos hasta **90 RPM** en escalones de 10 RPM.
4. **Comportamiento del Datalogger V5:**
   * Al encender la bomba, se registra automáticamente la muestra inicial en **$t = 0\text{ s}$**.
   * Cada **10 segundos** se almacena un registro en memoria (hasta 100 minutos continuos sin pérdida de bajas RPM).
   * Al presionar STOP, el registro se detiene limpiamente.
5. Descargar el archivo CSV ingresando a:
   ```text
   http://192.168.4.1/export_csv
   ```
   O hacer clic en el botón de descarga del SCADA.

---

### ETAPA 4: Interpretación del Factor K
* Gracias al filtro de ancho de nivel, se elimina el efecto de disparos múltiples (*chattering*) provocado por la rampa lenta del filtro RC ($4.7\text{ k}\Omega + 200\text{ nF}$).
* Si en la probeta se observa que el caudal experimental medido con $K = 196.50$ difiere del nominal, se puede ajustar $K_{\text{alim}}$ en tiempo real desde la consola de desarrollador del SCADA o mediante:
  ```text
  http://192.168.4.1/set_dev?ka=NUEVO_K&save=1
  ```
  *(Donde `NUEVO_K = (Frecuencia_Hz * 1000) / Q_Probeta_mLmin`).*
