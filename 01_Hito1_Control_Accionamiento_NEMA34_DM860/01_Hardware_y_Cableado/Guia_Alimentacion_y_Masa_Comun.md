# ⚡ GUÍA DE ALIMENTACIÓN ELÉCTRICA, MASA COMÚN Y BLINDAJE EMI
## Hito 1: Alimentación Híbrida 24VAC + 12VDC, Desacople y Supresión de Ruido
**Proyecto:** Planta Piloto de Ultrafiltración UNSa  
**Referencia:** Auditoría Técnica de Cierre — Dictamen Argon (Octubre 2026)

Esta guía explica cómo distribuir la energía de la planta piloto para garantizar que los circuitos de control no sufran interferencias (*brownout*, ruidos EMI o disparos en interrupciones) cuando los motores entren en carga.

---

## ⚠️ 1. Regla de Oro: Separación Estricta de Circuitos AC y DC

```
                          DISTRIBUCIÓN DE ENERGÍA HÍBRIDA
                          ===============================

 1. CIRCUITO AC (Potencia Bomba Peristáltica NEMA 34):
    Transformador 220V/24VAC ──► [Fusible 6A Lento] ──► Driver DM860 (Bornes AC / AC)
    *(El secundario de 24 VAC flota aislado; NO se une a la masa lógica GND)*

 2. CIRCUITO DC (Agitador Sedimentador y Lógica ESP32):
    Fuente 12V DC (8-10A) ──┬─────► [Capacitor 1000 µF] ──► Driver BTS7960 (B+ y B-)
                            │
                            └─────► Step-Down LM2596 ──► 5.00V DC ──► [Capacitor 470 µF + 100 nF] ──► ESP32 VIN
```

### Justificación Técnica de Tensión de Bus:
El driver **Leadshine DM860** posee internamente un puente rectificador de onda completa y banco de capacitores que eleva la alterna rectificada a:
$$V_{\text{bus}} \approx 24\text{ VAC} \times \sqrt{2} - 1.4\text{ V} \approx 32.5\text{ VDC}$$
Esta tensión es óptima para que el motor paso a paso NEMA 34 trabaje con alto torque y bajas temperaturas en régimen peristáltico continuo.

---

## 🛠️ 2. Calibración del Módulo Step-Down LM2596 (Paso Obligatorio)

Antes de conectar la salida al ESP32:
1. Conecta el cable **+12V** al borne `IN+` del LM2596.
2. Conecta el cable **GND (0V)** al borne `IN-` del LM2596.
3. Enchufa la fuente de 12V a 220V.
4. Mide con un multímetro/tester en los bornes `OUT+` y `OUT-`.
5. Con un destornillador plano pequeño, **gira el tornillo dorado del potenciómetro azul en sentido antihorario** hasta que la pantalla marque exactamente **`5.00 V DC`**.
6. Desenchufa la fuente.

---

## 🔗 3. Cómo Conectar la Masa Común (GND) con los Conectores Rápidos

Para no amontonar cables en la patita `GND` del ESP32, se utiliza la técnica de **cadena de distribución con los conectores rápidos a presión**:

```
 [ Pin GND del ESP32 ]
          │ (1 solo cable fino)
          ▼
   ┌───────────────┐
   │ CONECTOR #1   │ ──────► Driver DM860 (Bornes optoacoplados PUL- y DIR-)
   │               │ ──────► Cable puente hacia Conector #2 ──┐
   └───────────────┘                                          │
                                                              ▼
                                                       ┌───────────────┐
                                                       │ CONECTOR #2   │ ──────► Driver BTS7960 (GND lógica)
                                                       │               │ ──────► Filtro Boya de Nivel (GND)
                                                       │               │ ──────► Cable puente a Conector #3 ──┐
                                                       └───────────────┘                                      │
                                                                                                              ▼
                                                                                                       ┌───────────────┐
                                                                                                       │ CONECTOR #3   │ ──► Sensores DS18B20,
                                                                                                       │               │     TDS, ADS1115 y
                                                                                                       └───────────────┘     Caudalímetros
```

---

## 🛡️ 4. Protocolo Anti-EMI contra Cuelgues y Reinicios del ESP32 (Reglas de Argon)

Para evitar caídas por transitorios (*brownout*), lecturas fantasma en el caudalímetro YF-S401 o lazos de masa en la sonda de TDS:

1. **Aislamiento Galvánico Real en DM860:**  
   Los bornes `PUL-` y `DIR-` son cátodos de optoacopladores internos. Conéctalos a `GND` del ESP32, pero **JAMÁS unas los cables de potencia de 24 VAC del transformador a la masa de 12V/5V**. La potencia del NEMA 34 debe flotar aislada.
2. **Capacitor de Desacople Local en ESP32 (Reservorio Wi-Fi):**  
   Soldar directamente entre los pines `VIN (5V)` y `GND` del zócalo del ESP32 un **capacitor electrolítico de 470 µF (16V o 25V) en paralelo con uno cerámico de 100 nF**. Esto amortigua el pico de consumo del transmisor Wi-Fi de la Web SCADA (`192.168.4.1`) evitando el reseteo del microcontrolador.
3. **Supresión en Bornes del Motorreductor del Agitador:**  
   * Soldar un **capacitor cerámico de 100 nF / 50V** directamente entre los dos bornes del motorreductor de 12V (en la torreta del sedimentador) para suprimir en origen las chispas de escobillas.
   * Colocar un **capacitor electrolítico de 1000 µF / 25V** en los bornes de alimentación `B+` y `B-` del driver BTS7960 para absorber picos regenerativos de frenado activo.
4. **Protección de la Boya en la Línea Umbilical GX16-4:**  
   Al viajar la señal lógica de la boya por la misma manguera de 4 hilos que la potencia PWM del motor:
   * Colocar una resistencia *pull-up* física externa de **2.2 kΩ a 3.3V** y un capacitor cerámico de **100 nF a GND** en el maletín.
   * **Agregar una resistencia de 470 Ω en serie** justo antes del pin `GPIO 32` del ESP32 para limitar cualquier corriente inducida.
5. **Verificación de Chasis en Motor de Limpiaparabrisas:**  
   Si se recicla un motor de limpiaparabrisas, comprobar con multímetro que la carcasa externa **no tenga continuidad con el polo negativo**. Si tuviera continuidad, aislar el motor de la tapa del sedimentador con bujes de goma/nylon para evitar que el tanque de acero inoxidable quede energizado con PWM, arruinando la lectura analógica de TDS.

---

## 📋 Lista de Verificación (Checklist de Avance)
- [ ] Transformador de 24 VAC aislado con fusible de 6A/8A lento en secundario, conectado exclusivamente a los bornes `AC / AC` del DM860.
- [ ] Fuente conmutada de 12V 10A conectada a la entrada `IN+` e `IN-` del reductor LM2596 y a la bornera `B+ / B-` del BTS7960 (con capacitor de 1000 µF).
- [ ] Módulo Step-Down LM2596 medido con tester en vacío y calibrado exactamente a **`5.00 V DC`**.
- [ ] Capacitor de 470 µF + 100 nF soldado entre `VIN` y `GND` del ESP32.
- [ ] Cable de masa común conectado desde el pin `GND` del ESP32 a la entrada del Conector Rápido #1.
- [ ] Puentes de masa derivados a optoacopladores del DM860 (`PUL-` y `DIR-`), lógica del BTS7960 y sensores.
- [ ] Filtro de boya con pull-up de 2.2k, capacitor de 100nF y resistencia serie de 470 Ω en `GPIO 32`.
- [ ] Prueba de continuidad con multímetro entre todos los puntos de masa lógica (resistencia $< 0.2\,\Omega$).
