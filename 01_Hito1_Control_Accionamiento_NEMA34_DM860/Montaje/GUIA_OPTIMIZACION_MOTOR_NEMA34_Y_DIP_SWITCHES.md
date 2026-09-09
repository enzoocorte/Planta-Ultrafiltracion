# ⚙️ GUÍA DE PUESTA A PUNTO Y OPTIMIZACIÓN: MOTOR NEMA 34 (4.5 Nm) & DRIVER DM860
## Basado en la Especificación Oficial del Fabricante (CNC Insumos S.R.L. / Modelo FX3.162)

---

## 🔍 1. Análisis Técnico de la Hoja de Datos del Motor

A partir de la placa de datos oficial provista por el fabricante (**CNC Insumos S.R.L.**), el motor es un **NEMA 34 de 8 cables** con ángulo de paso de $1.8^\circ \pm 5\%$ y un torque de retención de **$4.5\text{ Nm}$**.

El motor cuenta con **4 bobinados independientes** (8 conductores en total), lo que permite conectarlo en dos configuraciones bipolares distintas:

```
┌──────────────────────────────┬───────────────────────────────┬───────────────────────────────┐
│ Parámetro Eléctrico          │ Conexión Bipolar SERIE        │ Conexión Bipolar PARALELO     │
├──────────────────────────────┼───────────────────────────────┼───────────────────────────────┤
│ **Torque de Retención**      │ **4.5 Nm**                    │ **4.5 Nm**                    │
│ **Corriente Nominal**        │ **3.0 A**                     │ **6.0 A**                     │
│ **Resistencia por Fase**     │ $2.0\,\Omega \pm 10\%$        │ $0.5\,\Omega \pm 10\%$        │
│ **Inductancia por Fase**     │ $16.0\text{ mH} \pm 20\%$     │ $4.0\text{ mH} \pm 20\%$      │
│ **Régimen de Operación**     │ **Ideal Bajas/Medias RPM**    │ Ideal Altas RPM (>600 RPM)    │
│ **Comportamiento Térmico**   │ **Frío / Mínimo consumo**     │ Mayor calentamiento           │
│ **Carga en Transformador**   │ Solo ~75 VA (50% capacidad)   │ ~150 VA (100% capacidad)      │
└──────────────────────────────┴───────────────────────────────┴───────────────────────────────┘
```

---

## 🏆 2. Recomendación de Ingeniería: ¿Qué conexión conviene para la Bomba MBP-2000?

### 👉 **RECOMENDACIÓN: CONEXIÓN BIPOLAR SERIE (3.0 A)**

#### ¿Por qué es la configuración óptima para la planta de ultrafiltración?
1. **Rango de Trabajo de la Bomba**: La bomba peristáltica MBP-2000 opera entre **$10\text{ y }100\text{ RPM}$** (un caudal de $0.04\text{ a }0.45\text{ L/min}$). A estas bajas velocidades, **la conexión Serie entrega el 100% del torque ($4.5\text{ Nm}$)**.
2. **Menor Consumo y Cero Calentamiento**: Al consumir solo **$3.0\text{ A}$** (la mitad que en paralelo), la potencia disipada por calor según la ley de Joule ($P = I^2 R$) se reduce drásticamente. El motor trabaja templado/frío.
3. **Protección del Transformador ($150\text{ VA}$)**: Tu transformador de $24\text{ VAC}$ entrega hasta $\approx 6.25\text{ A}$. Con el motor en Serie ($3\text{ A}$), el transformador trabaja descansado al **$48\%$ de su potencia nominal**, garantizando larga vida útil sin vibraciones ni zumbidos.

---

## 🎨 3. Diagrama de Conexión de los 8 Cables del Motor

El motor posee 8 cables divididos en dos fases (Fase A y Fase B):
* **Fase A**: Rojo, Amarillo, Azul, Negro.
* **Fase B**: Blanco, Naranja, Marrón, Verde.

---

### Opción A: Bipolar SERIE (Consumo 3.0A) — ⭐ [RECOMENDADA]

```
     MOTOR NEMA 34 (8 Cables)                     DRIVER LEADSHINE DM860
   ┌──────────────────────────┐                 ┌────────────────────────┐
   │        Cable ROJO ───────┼────────────────►│ Borne A+               │
   │                          │                 │                        │
   │        Cable AMARILLO ───┼─┐               │                        │
   │                          │ ├─ (Unir entre sí y aislar con cinta)    │
   │        Cable AZUL ───────┼─┘ (NO SE CONECTA AL DRIVER)              │
   │                          │                 │                        │
   │        Cable NEGRO ──────┼────────────────►│ Borne A-               │
   │                          │                 │                        │
   │        Cable BLANCO ─────┼────────────────►│ Borne B+               │
   │                          │                 │                        │
   │        Cable NARANJA ────┼─┐               │                        │
   │                          │ ├─ (Unir entre sí y aislar con cinta)    │
   │        Cable MARRÓN ─────┼─┘ (NO SE CONECTA AL DRIVER)              │
   │                          │                 │                        │
   │        Cable VERDE ──────┼────────────────►│ Borne B-               │
   └──────────────────────────┘                 └────────────────────────┘
```

> **Pasos para conectar en Serie**:
> 1. Pela la punta del cable **Amarillo** y del cable **Azul**, retuércelos juntos con firmeza y cúbrelos con cinta aisladora o termocontraíble.
> 2. Pela la punta del cable **Naranja** y del cable **Marrón**, retuércelos juntos y cúbrelos con cinta aisladora.
> 3. Al driver DM860 solo entran **4 cables**: **Rojo** a `A+`, **Negro** a `A-`, **Blanco** a `B+` y **Verde** a `B-`.

---

### Opción B: Bipolar PARALELO (Consumo 6.0A)

Si en el futuro requirieras mover el motor a más de $800\text{ RPM}$ (aplicaciones CNC de corte rápido):
* **Rojo + Azul** (unidos juntos) ──► Borne **`A+`**
* **Amarillo + Negro** (unidos juntos) ──► Borne **`A-`**
* **Blanco + Marrón** (unidos juntos) ──► Borne **`B+`**
* **Naranja + Verde** (unidos juntos) ──► Borne **`B-`**

---

## 🎛️ 4. Configuración Óptima de los DIP Switches (SW1 a SW8)

El driver digital DM860 posee 8 pequeños interruptores deslizantes en su lateral. A continuación se detalla la calibración exacta según la conexión elegida:

### Tabla para Conexión Bipolar SERIE (3.0 A) — [CONFIGURACIÓN ÓPTIMA]:

```
 ┌───────────┬──────────────┬──────────────────┬─────────────────────────────────────────────────┐
 │ Switch    │ Posición     │ Parámetro        │ Función Técnica Explicada                       │
 ├───────────┼──────────────┼──────────────────┼─────────────────────────────────────────────────┤
 │ **SW1**   │ **ON**       │ Corriente de     │ Configura el driver para una corriente          │
 │ **SW2**   │ **OFF**      │ Trabajo (RMS)    │ de **~3.14A RMS (3.77A Pico)**. Entrega el      │
 │ **SW3**   │ **ON**       │                  │ torque completo de 4.5 Nm sin saturar bobinas.  │
 ├───────────┼──────────────┼──────────────────┼─────────────────────────────────────────────────┤
 │ **SW4**   │ ❗**OFF**❗   │ Reducción en     │ **STANDSTILL CURRENT AL 50% (OBLIGATORIO)**:     │
 │           │              │ Reposo           │ Cuando la bomba frena, el driver baja la        │
 │           │              │                  │ corriente a 1.5A. **El motor se mantiene frío**.│
 ├───────────┼──────────────┼──────────────────┼─────────────────────────────────────────────────┤
 │ **SW5**   │ **ON**       │                  │                                                 │
 │ **SW6**   │ **OFF**      │ Resolución de    │ **1600 PULSOS POR REVOLUCIÓN (8 micropasos)**:   │
 │ **SW7**   │ **ON**       │ Micropasos       │ Movimiento suave como la seda, sin resonancia   │
 │ **SW8**   │ **ON**       │                  │ acústica y sin vibrar la manguera peristáltica. │
 └───────────┴──────────────┴──────────────────┴─────────────────────────────────────────────────┘
```

---

### Si prefieres usar Bipolar PARALELO (6.0 A):
* **SW1**: `OFF`, **SW2**: `OFF`, **SW3**: `OFF` (7.2A Pico / 6.0A RMS).
* **SW4**: `OFF` (50% en reposo).
* **SW5**: `ON`, **SW6**: `OFF`, **SW7**: `ON`, **SW8**: `ON` (1600 P/R).

---

## ⚡ 5. Resumen Visual de la Posición de las Palanquitas (Bipolar Serie 3A)

```
        VISTA LATERAL DE LOS DIP SWITCHES DEL DM860
        ===========================================

              SW1   SW2   SW3   SW4   SW5   SW6   SW7   SW8
        ON   [ ■ ] [   ] [ ■ ] [   ] [ ■ ] [   ] [ ■ ] [ ■ ]
        OFF  [   ] [ ■ ] [   ] [ ■ ] [   ] [ ■ ] [   ] [   ]
              ▲     ▲     ▲     ▲     ▲     ▲     ▲     ▲
              └─────┬─────┘     │     └─────────┬─────────┘
                 Corriente      │            Resolución
                  ~3.14A       50%            1600 P/R
                   RMS        Reposo        (8 micropasos)
                             (No calienta)
```

---

## 📋 6. Checklist de Puesta en Marcha en el Banco

1. [ ] ¿Los cables Amarillo-Azul y Naranja-Marrón están empalmados y bien aislados?
2. [ ] ¿Los 4 cables restantes (Rojo, Negro, Blanco, Verde) están ajustados en `A+, A-, B+, B-`?
3. [ ] ¿Los interruptores **SW1 a SW8** están exactamente como en el gráfico superior?
4. [ ] ¿El switch **SW4 está en OFF**?
5. [ ] ¿El transformador de 24 VAC está conectado a los bornes `AC / AC`?
6. [ ] ¿El ESP32 tiene cargado el firmware `Hito1_ControlMotor.ino`?

¡Listo! Con esta puesta a punto el conjunto motor-driver operará con su máxima eficiencia mecatrónica, torque de 4.5 Nm garantizado y funcionamiento térmico óptimo.
