# 🚰 GUÍA DE COMPRAS HIDRÁULICAS, ADAPTACIÓN 1/2" A 1/4" Y PROVEEDORES EN SALTA CAPITAL
## Hito 3: Reactor, Sedimentador y Líneas Hidráulicas de Instrumentación
**Tesistas**: Antonella Guitián & Owen Cañizares  
**Codirector**: Ing. Enzo (Investigación Doctoral en Membranas)

---

## 🎯 Entregable Concreto de esta Guía
* **Kit de adaptación hidráulica de 1/2" a 1/4" adquirido y ensamblado**: Accesorios de bronce roscados (Tee, bujes reductores, espigas para manguera, abrazaderas cremallera y teflón) adquiridos en comercios especializados de Salta Capital para conectar herméticamente la línea de impulsión de la bomba peristáltica con los caudalímetros e instrumentación sin fugas.

---

## 📋 Lista de Verificación (Checklist de Avance)
- [ ] 1x TEE de bronce de 1/2" Hembra-Hembra-Hembra comprada.
- [ ] 2x Bujes de reducción de bronce de 1/2" Macho a 1/4" Hembra comprados.
- [ ] 1x Espiga para manguera rosca Macho de 1/2" (para manguera de 12 mm) comprada.
- [ ] 1x Espiga para manguera rosca Macho de 1/4" (para manguera de 6 mm) comprada.
- [ ] 1x Tapón de bronce rosca Macho de 1/4" comprado (para clausurar la boca del sensor hasta el Hito 4).
- [ ] 2 metros de manguera de silicona cristal de 1/4" (diámetro interior 6 mm) adquiridos.
- [ ] Abrazaderas cremallera metálicas y cinta teflón de alta densidad en mano.
- [ ] Ensamble con teflón (5-6 vueltas) ajustado con llave fija.
- [ ] Prueba hidrostática de estanqueidad completada con la bomba peristáltica a 30 RPM (cero goteos).

---

## 💡 1. Fundamento Técnico: ¿Por qué adaptar de 1/2" a 1/4"?

En la planta piloto, la bomba peristáltica MBP-2000 utiliza manguera gruesa de $1/2"$ ($12.7\text{ mm}$ int.), mientras que los caudalímetros de turbina YF-S401 y los transductores de presión utilizan conexiones de $1/4"$ ($6.35\text{ mm}$).

Por el **Principio de Continuidad Hidráulica**:
$$Q = v_1 \cdot A_1 = v_2 \cdot A_2$$

Al reducir el diámetro a la mitad ($1/2" \rightarrow 1/4"$):
1. El área de flujo se reduce a la cuarta parte ($A_2 = A_1 / 4$).
2. **La velocidad lineal del líquido se multiplica por 4 ($v_2 = 4 \cdot v_1$)**.
3. **Ventaja para la Medición**: El caudalímetro YF-S401 opera por turbina de efecto Hall. En caudales bajos ($0.05 \text{ a } 0.5\text{ L/min}$), una velocidad baja en caño de $1/2"$ no vencería la inercia de la turbina. Al estrangular a $1/4"$, el chorro adquiere velocidad suficiente para girar la turbina con altísima precisión sin generar caídas de presión perjudiciales ($< 0.02\text{ bar}$).

---

## 🔩 2. Esquema de Encastre Mecánico con Buje de Bronce

El ensamble utiliza piezas comerciales estándar. Como las espigas de manguera comerciales son siempre de **rosca Macho**, se utiliza una **Tee Hembra de 1/2"** y un **Buje de reducción** que achica la rosca de forma natural:

```
                            [ TRANSDUCTOR DE PRESIÓN 1/4" ]
                            (O Tapón Macho 1/4" provisorio)
                                          │
                                          ▼ (Rosca Macho 1/4")
                            ┌─────────────────────────┐
                            │ BUJE BRONCE 1/2"M a 1/4"H│
                            └─────────────┬───────────┘
                                          │ (Rosca Macho 1/2")
                                          ▼ (Boca lateral de la Tee)
[Manguera 1/2"] ────────► ┌──────────────────────────────────────┐ ────────► [Manguera 1/4"]
(Espiga Macho 1/2")       │           TEE 1/2" (H-H-H)           │ (Buje 1/2"M-1/4"H + Espiga 1/4"M)
(Manguera de Bomba)       └──────────────────────────────────────┘ (Hacia Caudalímetro YF-S401)
```

* **Paso 1**: La **Espiga Macho de 1/2"** entra en la boca izquierda de la Tee (Hembra 1/2").
* **Paso 2**: En la boca derecha se enrosca el **Buje de Reducción (Macho 1/2" exterior / Hembra 1/4" interior)**.
* **Paso 3**: Adentro del buje se enrosca la **Espiga Macho de 1/4"** hacia el caudalímetro.
* **Paso 4**: En la boca superior se enrosca el segundo **Buje de Reducción** con el transductor de presión (o el tapón macho de 1/4").

---

## 📋 3. Lista de Compras para el Mostrador

Copiar y presentar este pedido en el mostrador:

```markdown
LISTA DE MATERIALES - PLANTA PILOTO UF (Conexión 1/2" a 1/4")

1. Accesorios de Bronce (Roscas Gas / BSP):
   - 1x TEE de bronce 1/2" Hembra-Hembra-Hembra.
   - 2x Buje de reducción de bronce 1/2" Macho a 1/4" Hembra.
   - 1x Espiga para manguera rosca Macho 1/2" (para manguera de 12 mm).
   - 1x Espiga para manguera rosca Macho 1/4" (para manguera de 6 mm).
   - 1x Tapón de bronce rosca Macho 1/4" (provisorio para el sensor de presión).

2. Mangueras y Consumibles:
   - 2 metros de manguera de silicona o PVC cristal de 1/4" (diámetro interior 6 mm).
   - 2x Abrazaderas metálicas cremallera (sin-fín) para manguera de 1/2".
   - 2x Abrazaderas metálicas miniatura (o precintos plásticos resistentes) para 1/4".
   - 1x Rollo de cinta teflón de alta densidad (amarilla o blanca gruesa).
```

---

## 📍 4. Comercios y Proveedores Recomendados en Salta Capital

En Salta Capital, **evitar ferreterías de barrio o sanitarios comunes** (que solo trabajan medidas domiciliarias grandes). Dirigirse a comercios especializados en mangueras industriales, neumática y bulonería técnica:

```
┌──────────────────────────────┬──────────────────────────────────┬────────────────────────────────────────────────────────┐
│ Comercio Recomendado         │ Dirección en Salta Capital       │ Especialidad / Qué conviene comprar allí                │
├──────────────────────────────┼──────────────────────────────────┼────────────────────────────────────────────────────────┤
│ **Centro Goma S.R.L.**       │ Pellegrini 411                   │ Mangueras de silicona cristal, espigas plásticas/bronce│
│                              │ (casi esq. Corrientes / San Juan)│ y abrazaderas cremallera de todas las medidas.         │
├──────────────────────────────┼──────────────────────────────────┼────────────────────────────────────────────────────────┤
│ **Fuga Cero**                │ Islas Malvinas 697               │ Neumática, conexiones hidráulicas, bujes de bronce     │
│                              │ (esquina Caseros)                │ de 1/8", 1/4" y 1/2", manómetros y teflón técnico.     │
├──────────────────────────────┼──────────────────────────────────┼────────────────────────────────────────────────────────┤
│ **Bulonera San Martín**      │ Av. San Martín 1833              │ Bujes de reducción para manómetros, niples y fittings. │
├──────────────────────────────┼──────────────────────────────────┼────────────────────────────────────────────────────────┤
│ **Ferrinor**                 │ José E. Uriburu 96               │ Ferretería industrial pesada y accesorios roscados.    │
├──────────────────────────────┼──────────────────────────────────┼────────────────────────────────────────────────────────┤
│ **Sanitarios Salta**         │ Ituzaingó 158                    │ TEEs y cuplas de bronce tradicionales de fontanería.   │
└──────────────────────────────┴──────────────────────────────────┴────────────────────────────────────────────────────────┘
```

> [!TIP]
> **Ruta Rápida para los Alumnos**:  
> Comenzar directamente por **Centro Goma (Pellegrini 411)** o **Fuga Cero (Islas Malvinas 697)**. Tienen personal técnico capacitado para aplicaciones industriales que arma el ensamble en el momento sobre el mostrador.
