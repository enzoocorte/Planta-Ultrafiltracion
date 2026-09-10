# ⚡ GUÍA DE ALIMENTACIÓN ELÉCTRICA Y MASA COMÚN (GND)

Esta guía explica cómo distribuir la energía de la planta piloto para garantizar que los circuitos de control no sufran interferencias ni caídas de tensión cuando los motores entren en carga.

---

## ⚠️ 1. Regla de Oro: Separación de AC y DC

```
                          DISTRIBUCIÓN DE ENERGÍA
                          =======================

 1. CIRCUITO AC (Potencia de Bomba):
    Transformador AC ──────────► Driver Leadshine DM860 (Bornes AC / AC)
    *(No mezclar con la entrada del LM2596 ni del L298N)*

 2. CIRCUITO DC (Control y Agitador):
    Fuente 12V 1.5A DC ──┬─────► Driver L298N (+12V y GND)
                         │
                         └─────► Step-Down LM2596 (IN+ e IN-) ──► 5.00V DC ──► ESP32 VIN
```

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
  │ CONECTOR #1   │ ──────► Driver DM860 (Bornes PUL- y DIR-)
  │               │ ──────► Cable puente hacia Conector #2 ──┐
  └───────────────┘                                          │
                                                             ▼
                                                      ┌───────────────┐
                                                      │ CONECTOR #2   │ ──────► Driver L298N (GND)
                                                      │               │ ──────► Sensor de Nivel (Boya)
                                                      │               │ ──────► Cable puente a Conector #3 ──┐
                                                      └───────────────┘                                      │
                                                                                                             ▼
                                                                                                      ┌───────────────┐
                                                                                                      │ CONECTOR #3   │ ──► Sensores DS18B20,
                                                                                                      │               │     TDS, ADS1115 y
                                                                                                      └───────────────┘     Caudalímetros
```

### Ventajas de este montaje:
* Cada componente tiene su propio borne seguro.
* Si se necesita desconectar un sensor para probarlo, se abre una palanquita sin afectar al resto del circuito.
* Cero falsos contactos y cero soldaduras.

---

## 🎯 Entregable Concreto de esta Guía
* **Línea de alimentación de 5.00V DC estable** verificada con multímetro antes de energizar el microcontrolador.
* **Barra de distribución de masa (GND) continua** encadenada con conectores rápidos a presión, con 0V de caída entre el ESP32, los drivers y los sensores.

---

## 📋 Lista de Verificación (Checklist de Avance)
- [ ] Transformador de 24 VAC aislado y conectado exclusivamente a los bornes `AC / AC` del DM860.
- [ ] Fuente de 12V 1.5A conectada a la entrada `IN+` e `IN-` del reductor LM2596 y a la bornera del L298N.
- [ ] Módulo Step-Down LM2596 medido con tester en vacío y calibrado exactamente a **`5.00 V DC`**.
- [ ] Cable de masa común conectado desde el pin `GND` del ESP32 a la entrada del Conector Rápido #1.
- [ ] Puente de masa derivado correctamente al driver DM860 (`PUL-` y `DIR-`).
- [ ] Puente de masa derivado correctamente al driver L298N (`GND`).
- [ ] Prueba de continuidad con multímetro entre todos los puntos de masa (resistencia $< 0.2\,\Omega$).
