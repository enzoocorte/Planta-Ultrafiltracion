# 🛠️ GUÍA DE MONTAJE: MÓDULO FRONT-END DE INSTRUMENTACIÓN Y FILTRADO ANTIRRUIDO
## Arquitectura de Doble Bornera Shield para Planta Piloto de Ultrafiltración FX100
**Autores:** Antonella Guitián & Owen Cañizares  
**Dirección Técnica:** Ing. Enzo  
**Ubicación:** Laboratorio de Ingeniería — Salta, Argentina  
**Fecha:** Septiembre 2026  

---

## 🎯 1. Concepto y Filosofía de Diseño Industrial

En sistemas de adquisición de datos industriales expuestos a conmutación de potencia (como el motor NEMA 34 operado por el driver Leadshine DM860 a 3.0A), **la separación física entre la lógica de control y la interfaz de campo es la regla de oro**.

Aprovechando que disponemos de **dos Shields de Borneras para ESP32 (ZS-1057)**, estructuramos el sistema en dos módulos complementarios:

```
 ┌────────────────────────────────────────────────────────┐
 │           PLACA 2: FRONT-END DE INSTRUMENTACIÓN         │
 │           (Segunda Bornera ZS-1057 SIN ESP32)          │
 │                                                        │
 │  • Punto de llegada de todos los cables de la planta.  │
 │  • Acondicionamiento pasivo (Filtros RC antirruido).   │
 │  • Resistencias de Pull-Up a nivel seguro (3.3V).      │
 │  • Soporte y borneras del Conversor ADC ADS1115.       │
 └──────────────────────────┬─────────────────────────────┘
                            │ Mazo de Cables Limpios
                            │ (Solo Señales Filtradas y Alimentación)
                            ▼
 ┌────────────────────────────────────────────────────────┐
 │              PLACA 1: CPU MASTER Y CONTROL             │
 │          (Primera Bornera ZS-1057 CON ESP32)           │
 │                                                        │
 │  • Microcontrolador ESP32 DevKit V1 montado.           │
 │  • Control del Driver DM860 (Pulsos y Dirección).      │
 │  • Servidor Web SCADA Wi-Fi y Adquisición FreeRTOS.    │
 │  • Placa totalmente limpia, sin arañas de resistencias │
 └────────────────────────────────────────────────────────┘
```

### Ventajas Técnicas Inmediatas:
1. **Inmunidad al Ruido**: Los filtros pasabajos RC ($4.7\text{ k}\Omega + 100\text{ nF}$) destruyen el ruido antes de que entre al microcontrolador.
2. **Cero Soldaduras**: Los zócalos hembra centrales ($2.54\text{ mm}$) y las borneras laterales permiten insertar y atornillar resistencias y capacitores firmemente.
3. **Mantenimiento y Modularidad**: Si se requiere cambiar o reprogramar el ESP32, no se desconecta ningún sensor de campo.
4. **Terminación Profesional**: Dos placas gemelas montadas sobre el tablero confieren aspecto de equipo comercial/planta piloto de investigación.

---

## 🧰 2. Materiales Necesarios para el Armado de Mañana

* **Estructura**:
  - 1x Shield Bornera ESP32 (Placa 1 - Con ESP32).
  - 1x Shield Bornera ESP32 (Placa 2 - Libre, sin micro).
* **Componentes Pasivos (Comprados en Salta Capital)**:
  - **3x Resistencias Metal Film $4.7\text{ k}\Omega$ - $1/4\text{W}$** (Amarillo - Violeta - Rojo - Dorado).
  - **3x Capacitores Cerámicos Multicapa $100\text{ nF}$** (Lenteja o multicapa con código `104`).
* **Cables de Interconexión**:
  - 8 a 10 cablecitos de puente (Jumpers macho-macho o cable unipolar de $0.5\text{ mm}^2$ pelado en las puntas).
* **Herramientas**:
  - Destornillador plano de precisión ("perillero").

---

## 🔌 3. Mapa de Conexionado Detallado (Paso a Paso)

```
===================================================================================================
                             ESQUEMA FÍSICO DE CONEXIÓN ENTRE PLACAS
===================================================================================================

       PLACA 2 (FRONT-END & FILTRADO)                           PLACA 1 (ESP32 MASTER)
   ┌─────────────────────────────────────┐                  ┌───────────────────────────────┐
   │                                     │                  │                               │
   │  [5V]  ◄────────────────────────────┼── Cable 1 ───────┤ [5V / VIN]                    │
   │  [GND] ◄────────────────────────────┼── Cable 2 ───────┤ [GND]                         │
   │  [3.3V]◄────────────────────────────┼── Cable 3 ───────┤ [3.3V]                        │
   │                                     │                  │                               │
   │  [P14] (Salida Filtrada Feed) ──────┼── Cable 4 ──────►│ [P14] (GPIO 14 - Int Feed)    │
   │  [P27] (Salida Filtrada Perm) ──────┼── Cable 5 ──────►│ [P27] (GPIO 27 - Int Perm)    │
   │  [P4]  (Salida Datos Temp)    ──────┼── Cable 6 ──────►│ [P4]  (GPIO 4  - OneWire)     │
   │  [P32] (Salida Boya Nivel)    ──────┼── Cable 7 ──────►│ [P32] (GPIO 32 - Nivel Inox)  │
   │                                     │                  │                               │
   │  [SDA] (Bus ADS1115)         ───────┼── Cable 8 ──────►│ [P21] (GPIO 21 - SDA)         │
   │  [SCL] (Bus ADS1115)         ───────┼── Cable 9 ──────►│ [P22] (GPIO 22 - SCL)         │
   │                                     │                  │                               │
   └─────────────────────────────────────┘                  └───────────────────────────────┘
```

---

### PASO 1: Enlace de Alimentación Troncal (Placa 1 ➔ Placa 2)
Tirar 3 cables firmes entre ambas placas para que la Placa 2 tenga tensión de referencia:
1. Conectar borne **[5V] de Placa 1** con borne **[5V] de Placa 2**.
2. Conectar borne **[GND] de Placa 1** con borne **[GND] de Placa 2**.
3. Conectar borne **[3.3V] de Placa 1** con borne **[3.3V] de Placa 2**.

---

### PASO 2: Sector Caudalímetros YF-S401 (En Placa 2)

#### Caudalímetro 1 (Alimentación / Feed):
1. **Cable ROJO del Sensor** $\rightarrow$ Atornillar a borne **[5V]** de Placa 2.
2. **Cable NEGRO del Sensor** $\rightarrow$ Atornillar a borne **[GND]** de Placa 2.
3. **Cable AMARILLO del Sensor** $\rightarrow$ Atornillar a borne **[P14]** de Placa 2.
4. **Resistencia Pull-Up ($4.7\text{ k}\Omega$)**:
   - Una pata entra a borne **[3.3V]** de Placa 2.
   - La otra pata entra a borne **[P14]** de Placa 2.
5. **Capacitor Cerámico ($100\text{ nF}$ - Código 104)**:
   - Una pata entra a borne **[GND]** de Placa 2.
   - La otra pata entra a borne **[P14]** de Placa 2.
6. **Cable de Salida Limpia**:
   - Un cable desde borne **[P14] de Placa 2** hacia borne **[P14] de Placa 1**.

#### Caudalímetro 2 (Permeado / Ultrafiltrado):
1. **Cable ROJO del Sensor** $\rightarrow$ Atornillar a borne **[5V]** de Placa 2.
2. **Cable NEGRO del Sensor** $\rightarrow$ Atornillar a borne **[GND]** de Placa 2.
3. **Cable AMARILLO del Sensor** $\rightarrow$ Atornillar a borne **[P27]** de Placa 2.
4. **Resistencia Pull-Up ($4.7\text{ k}\Omega$)**:
   - Una pata entra a borne **[3.3V]** de Placa 2.
   - La otra pata entra a borne **[P27]** de Placa 2.
5. **Capacitor Cerámico ($100\text{ nF}$ - Código 104)**:
   - Una pata entra a borne **[GND]** de Placa 2.
   - La otra pata entra a borne **[P27]** de Placa 2.
6. **Cable de Salida Limpia**:
   - Un cable desde borne **[P27] de Placa 2** hacia borne **[P27] de Placa 1**.

---

### PASO 3: Sector Sensor de Temperatura DS18B20 (En Placa 2)

El sensor sumergible One-Wire **no lleva capacitor** (para no destruir la trama serie):
1. **Cable ROJO del Sensor** $\rightarrow$ Atornillar a borne **[3.3V]** de Placa 2.
2. **Cable NEGRO del Sensor** $\rightarrow$ Atornillar a borne **[GND]** de Placa 2.
3. **Cable AMARILLO/DATOS del Sensor** $\rightarrow$ Atornillar a borne **[P4]** de Placa 2.
4. **Resistencia Pull-Up ($4.7\text{ k}\Omega$)**:
   - Una pata a borne **[3.3V]** de Placa 2.
   - La otra pata a borne **[P4]** de Placa 2.
5. **Cable de Salida**:
   - Un cable desde borne **[P4] de Placa 2** hacia borne **[P4] de Placa 1**.

---

### PASO 4: Sector Boya de Nivel Inoxidable 100mm (En Placa 2)

La boya magnética es un contacto seco tipo interruptor reed:
1. **Cable 1 de la Boya** $\rightarrow$ Atornillar a borne **[GND]** de Placa 2.
2. **Cable 2 de la Boya** $\rightarrow$ Atornillar a borne **[P32]** de Placa 2.
3. **Capacitor Antirrebote Mecánico ($100\text{ nF}$ - 104)**:
   - Una pata a borne **[GND]** de Placa 2.
   - La otra pata a borne **[P32]** de Placa 2.
4. **Cable de Salida**:
   - Un cable desde borne **[P32] de Placa 2** hacia borne **[P32] de Placa 1**.
   *(El firmware activa internamente `pinMode(32, INPUT_PULLUP)`).*

---

### PASO 5: Sector Conversor ADC ADS1115 (Presión TMP y TDS)

El módulo ADS1115 se cablea sobre la Placa 2, dejando el bus I2C limpio hacia la Placa 1:
1. **Alimentación**:
   - Pin **VDD** del ADS1115 $\rightarrow$ Borne **[5V]** de Placa 2.
   - Pin **GND** del ADS1115 $\rightarrow$ Borne **[GND]** de Placa 2.
   - Pin **ADDR** del ADS1115 $\rightarrow$ Borne **[GND]** de Placa 2 (Fija dirección I2C `0x48`).
2. **Bus de Datos I2C**:
   - Pin **SDA** del ADS1115 $\rightarrow$ Borne **[P21]** de Placa 2 $\rightarrow$ Cable a **[P21]** de Placa 1.
   - Pin **SCL** del ADS1115 $\rightarrow$ Borne **[P22]** de Placa 2 $\rightarrow$ Cable a **[P22]** de Placa 1.
3. **Entradas Analógicas de Campo (Transductores de Presión)**:
   - **A0**: Señal $P_{\text{feed}}$ (Transductor de entrada de membrana).
   - **A1**: Señal $P_{\text{ret}}$ (Transductor de línea de concentrado/retentado).
   - **A2**: Señal $P_{\text{perm}}$ (Transductor de línea de ultrafiltrado/permeado).
   - **A3**: Señal Analógica de la Sonda TDS.

---

## 🔍 4. Protocolo de Verificación con Multímetro (Antes de Energizar)

1. **Prueba de Continuidad de Masas**:
   - Colocar el multímetro en modo "Continuidad / Beep".
   - Tocar un borne GND de Placa 1 y un borne GND de Placa 2 $\rightarrow$ Debe pitar ($0\,\Omega$).
2. **Prueba de Ausencia de Cortocircuito en Alimentación**:
   - Medir resistencia entre borne [3.3V] y [GND] de Placa 2 $\rightarrow$ Debe dar alta resistencia ($>1\text{ k}\Omega$). Si pita o da $0\,\Omega$, revisar patitas de componentes que puedan estar tocándose.
3. **Prueba de Voltaje en Vacío**:
   - Energizar el ESP32 por USB.
   - Medir con voltímetro en DC:
     - Entre GND y 5V de Placa 2 $\rightarrow$ Debe marcar $\approx 4.8\text{ V a } 5.1\text{ V}$.
     - Entre GND y 3.3V de Placa 2 $\rightarrow$ Debe marcar $\approx 3.28\text{ V a } 3.32\text{ V}$.
     - Entre GND y los bornes P14 y P27 $\rightarrow$ Debe marcar exactamente $3.3\text{V}$ en reposo (evidencia de que el pull-up a 3.3V está funcionando).

---

## 🚀 5. Validación Funcional con el Firmware

Una vez conectado el mazo hacia la Placa 1:
1. Encender el motor NEMA 34 desde la interfaz Web SCADA a 80 RPM.
2. Observar el Monitor Serie o la pantalla web:
   - Los valores de frecuencia en Hz de FEED y PERMEADO deben mantenerse en **$0.0\text{ Hz}$** si el agua no fluye, sin registrar un solo pulso fantasma mientras el motor gira a plena carga.
   - Al hacer circular agua soplando suavemente o impulsando con líquido, los pulsos deben subir fluidos y estables.

---
*Documento homologado para montaje de laboratorio — Tesis UF FX100.*
