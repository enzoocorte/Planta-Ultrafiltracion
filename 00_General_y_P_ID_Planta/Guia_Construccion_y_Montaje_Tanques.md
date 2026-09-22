# 🧪 GUÍA DE DISEÑO, CONSTRUCCIÓN Y MONTAJE DE TANQUES Y SONDAS
## Proyecto de Tesis: Planta Piloto de Ultrafiltración FX100

Esta guía describe cómo construir, perforar, sellar e instrumentar los recipientes de la planta piloto: el **Tanque Sedimentador/Alimentación (T-01)** y el **Tanque de Permeado (T-02)**, detallando la ubicación física exacta de las sondas de nivel, temperatura y calidad de agua (TDS).

---

## 🗺️ 1. Esquema de Integración Mecánica del Tanque T-01

```
                        TANQUE SEDIMENTADOR / ALIMENTACIÓN (T-01)
                        =========================================

                      [ Motorreductor 12V (30-60 RPM) ]
                                    │
               ┌────────────────────┴────────────────────┐  <-- Tapa Desmontable
               │  [Boca de Carga]           [Eje Central]│
               │   (Agua Turbia/                         │
               │    Coagulante)             │            │
               │                            │            │
               │                            ▼            │
               │                   [Paleta de Agitación] │
               │                       (Floculación)     │
               │                                         │
[Sonda TDS] ──►│ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ │ <--- Nivel Máximo Operativo
(Sumergible)   │                                         │
               │                                         │
[Termocupla]──►│ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ │ <--- Nivel Medio de Trabajo
(DS18B20 Inox) │                                         │
               │                                         │
[Boya Nivel]──►│ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ │ <--- Nivel Mínimo Crítico (3-5 cm)
(Boya Inox)    │    ┌─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─   │      (Enclavamiento de Bomba)
               │    │ Toma Succión Sobrenadante         │
               └────┼────────────────────────────────────┘
                    │        ▲
                    │        │  [Prefiltro Malla Inox 50-100 µm]
                    │        └──────────────► Hacia Bomba Peristáltica MBP-2000
                    ▼
          [ Válvula de Purga Fondo ]
              (Drenaje de Lodos)
```

---

## 🛠️ 2. Especificación Técnica de Componentes para Tanque T-01

### A. Recipiente Principal
* **Volumen sugerido**: 3 a 5 Litros.
* **Geometría ideal**: Cilíndrico de fondo cónico (o cilíndrico estándar con el fondo ligeramente inclinado) en acrílico cristal o polipropileno alimentario. El fondo cónico permite que los flóculos pesados de bentonita decanten en el vértice inferior sin quedar esparcidos.
* **Tapa superior**: Rígida (acrílico de 5 mm o polietileno), desmontable para limpieza profunda entre corridas experimentales.

### B. Sistema de Agitación Lenta (Floculación)
* **Motor**: Motorreductor DC 12V con caja reductora metálica (eje D-shaft de 4 a 6 mm), velocidad nominal de 30 a 60 RPM.
* **Eje**: Varilla de acero inoxidable AISI 304 de 4 o 6 mm de diámetro.
* **Paleta**: Tipo pala plana o rejilla rectangular (50 a 70 mm de ancho). La agitación debe ser suave (gradiente de velocidad $G \approx 20\text{ a }50\text{ s}^{-1}$) para favorecer la colisión y aglomeración de partículas con el mucílago de nopal sin romper las cadenas macromoleculares de los flóculos.
* **Control**: Puente H L298N conectado a la fuente de 12V y comandado por PWM desde el ESP32 (`GPIO 4` y pines de sentido `GPIO 16` y `GPIO 17`).

---

## 📍 3. Ubicación y Montaje de Sensores en el Tanque T-01

### 1. Boya de Nivel Inox (Sensor de Parada de Emergencia - LS-01)
* **¿Por qué colocarla?** Si el tanque se queda sin agua y la bomba peristáltica sigue girando, la manguera de silicona fricciona en seco contra los rodillos, sobrecalentándose y degradándose prematuramente.
* **Ubicación de perforación**: Perforar en la pared lateral con broca escalonada (diámetro según la rosca de la boya, típicamente M10 o 1/2''), a una altura de **3 a 5 cm por encima de la toma de succión**.
* **Orientación**: Montar horizontalmente con la junta de silicona por fuera y la tuerca plástica/metálica ajustada firmemente. Comprobar con multímetro que cuando la boya cae por falta de líquido, el contacto conmute.
* **Conexión al ESP32**:
  * Un cable de la boya al borne **[ GND ]** del shield.
  * El otro cable al borne **[ P32 ]** (`GPIO 32`). En el ESP32 se activa `INPUT_PULLUP`.

### 2. Sonda de Temperatura Sumergible DS18B20 (TT-01)
* **¿Por qué colocarla?** La viscosidad dinámica del agua cambia $\approx 2.4\%$ por cada $1^\circ\text{C}$. Para calcular el caudal normalizado a $20^\circ\text{C}$ con la Ley de Darcy, es mandatorio conocer la temperatura exacta del líquido.
* **Montaje**:
  * Opción 1 (Recomendada): Pasar la vaina cilíndrica de acero inoxidable a través de un **prensaestopa PG7** de nylon con sello de goma colocado en la pared lateral a media altura.
  * Opción 2: Suspender la sonda desde la tapa superior mediante un soporte plástico rígido, dejando la punta metálica sumergida 5 a 10 cm bajo la superficie libre.
* **Conexión al ESP32**: Cable rojo a 3.3V, negro a GND, amarillo a `GPIO 34` con resistencia pull-up de $4.7\text{ k}\Omega$ a 3.3V.

### 3. Sonda de Calidad de Agua TDS (AT-01)
* **¿Por qué colocarla?** Mide la concentración basal de sales y conductividad en el agua cruda para calcular luego el porcentaje de remoción en el permeado ($R = [1 - TDS_p / TDS_f] \times 100\%$).
* **Montaje**:
  * La sonda cuenta con dos electrodos paralelos de titanio. Debe quedar permanentemente sumergida en una zona de líquido representativo.
  * **Cuidado crítico:** Debe situarse a una distancia prudencial del agitador para que las paletas giratorias no golpeen la sonda, y a no menos de 15 mm de las paredes metálicas para evitar acoplamientos capacitivos espurios.
* **Conexión**: Salida analógica conectada al **Canal A0** del conversor ADS1115 (comunicado por I2C en `GPIO 21` y `GPIO 22`).

---

## 🚰 4. Hidráulica de Fondo: Prefiltro y Válvula de Purga

```
                              DETALLE DE FONDO Y SUCCIÓN
                              ==========================

         Pared Tanque T-01
              │
              │             [ Prefiltro Malla Inox 50-100 µm ]
              │             ┌────────────────┐
  (Succión) ──┼─────────────┤  Malla Filtrante ├──────► Hacia Borne Succión Bomba
              │   Espiga    └────────────────┘
              │  3-5 cm
              │  sobre el fondo
              │
              │
              └───┐
                  └───┐
                      │
                      ▼
               [ Válvula de Purga Fondo (1/4'') ] ──► Descarga / Muestreo de Lodos Decantados
```

1. **Toma de Succión Lateral:**
   * Se coloca un niple pasamuros (bulkhead) con espiga de $6\text{ mm}$ ($1/4''$) ubicado a 3 a 5 cm del fondo. Esto asegura que la bomba tome únicamente el sobrenadante clarificado y no aspire los sedimentos decantados.
2. **Prefiltro de Protección:**
   * Se intercala en la manguera un prefiltro de canasta o en línea con malla de acero inoxidable de $50\text{ a }100\text{ }\mu\text{m}$.
   * **Objetivo:** Evitar que fragmentos grandes de turbidez o partículas extrañas ingresen a las fibras huecas del filtro FX100 (cuyo lumen es muy estrecho y sensible a taponamientos irreversibles).
3. **Válvula de Purga de Lodos:**
   * En el punto más bajo del fondo cónico se instala una pequeña válvula esférica de $1/4''$. Al finalizar cada prueba, se abre para vaciar y cuantificar el volumen de lodo decantado.

---

## 🍶 5. Tanque de Permeado (T-02)

* **Capacidad**: 1 a 2 Litros (probeta graduada o recipiente cilíndrico translúcido).
* **Entrada**: Superior, conectada a la salida del caudalímetro YF-S401 de permeado.
* **Sonda TDS de Permeado**: Sumergida en el fondo del recipiente para monitorear en tiempo real la pureza del agua obtenida.
* **Válvula de Fondo / Toma de Retrolavado**: Permite redirigir agua limpia de permeado en sentido inverso hacia el filtro FX100 cuando se activa la maniobra de retrolavado (*backwash*).

---

## 📋 6. Lista de Verificación para el Armado de Tanques

- [ ] Recipiente T-01 de 3 a 5 L seleccionado y limpio.
- [ ] Orificio central en tapa para acople del motorreductor de 12V con eje y paleta de floculación.
- [ ] Perforación pasamuros para niple de succión a 3-5 cm del fondo con junta hermética de silicona.
- [ ] Válvula de purga esférica de 1/4'' instalada en el vértice inferior cónico.
- [ ] Prefiltro de malla de 50-100 µm instalado en la línea de aspiración previa a la bomba.
- [ ] Boya de nivel inox instalada horizontalmente sobre la toma de succión y probada con multímetro.
- [ ] Prensaestopa para sonda de temperatura DS18B20 montado con estanqueidad garantizada.
- [ ] Soporte para sonda TDS asegurado lejos de las aspas del agitador.
- [ ] Prueba estática de llenado con agua durante 30 minutos: verificar **CERO goteos** en todas las perforaciones.
