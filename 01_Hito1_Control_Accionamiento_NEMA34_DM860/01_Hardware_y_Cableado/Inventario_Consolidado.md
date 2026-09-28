# 📦 INVENTARIO CONSOLIDADO DE HARDWARE, INSTRUMENTACIÓN Y COMPRAS TÉCNICAS
## Planta Piloto de Ultrafiltración (UF) en Flujo Cruzado — Cartucho Fresenius FX100
**Tesistas:** Antonella Guitián & Owen Cañizares  
**Codirector:** Ing. Enzo  
**Ubicación de Montaje:** Salta, Argentina  
**Última Auditoría Técnica:** Septiembre 2026  

---

## 🟢 1. Componentes Físicos Validados y en Laboratorio

| Componente | Cantidad | Especificaciones Técnicas Exactas | Función en la Planta Piloto | Estado Operativo |
| :--- | :---: | :--- | :--- | :---: |
| **Motor Paso a Paso NEMA 34** | 1 | Bipolar Híbrido, 4.5 Nm, 8 cables (Bipolar Serie a 3.0A), eje Ø14 mm con chaveta 4 mm. | Accionamiento de alto torque para cabezal peristáltico MBP-2000. | 🟢 **Validado en banco** |
| **Driver Leadshine DM860** | 1 | Microstepping configurado a 3200 pulsos/rev (16 micropasos), cátodo común, entradas optoacopladas. | Control de corriente y modulación PWM para el NEMA 34. | 🟢 **Validado en banco** |
| **Cabezal Peristáltico MBP-2000** | 1 | Cuerpo en Polietileno de Alto Peso Molecular (APM / UHMW-PE), rodillos industriales. | Desplazamiento positivo sin contacto metálico (~16.6 mL/vuelta). | 🟢 **Validado en banco** |
| **Transformador AC 220V / 24V** | 1 | Primario 220 VAC ➔ Secundario 24 VAC (~4A). | Alimentación de potencia exclusiva para bornes `AC/AC` del DM860. | 🟢 **En mano (Va externo)** |
| **Microcontrolador ESP32 DevKit V1** | 1 | NodeMCU-32S, procesador dual core Tensilica Xtensa 32-bit, Wi-Fi 802.11 b/g/n. | Unidad Central de Proceso (FreeRTOS, WebServer SCADA, OTA, exportación CSV). | 🟢 **En mano** |
| **Shield de Expansión ZS-1057** | 2 | Borneras a tornillo paso 3.5 mm para ESP32 DevKit (30/38 pines). | **Placa 1**: CPU Master ESP32.<br>**Placa 2**: Módulo Front-End de Instrumentación y Filtrado Antirruido. | 🟢 **En mano (Arquitectura Doble)** |
| **Módulo ADC ADS1115 (16 bits)** | 1 | Conversor analógico-digital I2C de 4 canales ($A_0 - A_3$) con PGA programable. | Lectura de altísima resolución para sensores de presión y sonda TDS. | 🟢 **Comprado / En mano** |
| **Caudalímetros Microflujo YF-S401** | 2 | Turbinas efecto Hall, rango 0.3 a 6 L/min, rosca/espiga 1/4" BSP ($K \approx 98\text{ pulsos/L}$). | Medición continua de Alimentación ($Q_{\text{feed}}$) y Permeado ($Q_{\text{perm}}$). | 🟢 **Comprados (Requieren filtro RC)** |
| **Sensor de Temperatura DS18B20** | 1 | Sonda sumergible en cápsula de Acero Inox AISI 304, protocolo digital 1-Wire. | Monitoreo térmico ($^\circ\text{C}$) para normalización Darcy y viscosidad dinámica. | 🟢 **Comprado / En mano** |
| **Driver Puente H L298N** | 1 | Doble puente H 2A con disipador de aluminio integrado. | Accionamiento y control de velocidad (PWM) del motor de agitación del Jar Test. | 🟢 **Comprado / En mano** |
| **Sensor de Nivel de Acero Inox.** | 1 | Boya flotante magnética 100 mm AISI 304 (contacto seco Reed Switch N/C - N/A). | Enclavamiento de seguridad: parada de emergencia por tanque vacío (marcha en seco). | 🟢 **Comprado / En mano** |
| **Módulo Step-Down LM2596** | 1 | Regulador conmutado reductor DC-DC 3A con potenciómetro multivueltas. | Regulación de tensión auxiliar limpia y ultraestable (3.3V o 5.0V). | 🟢 **Comprado / En mano** |
| **Cartucho Fresenius FX100** | 1 | Membrana de fibras huecas Helixone / Polisulfona ($A_m = 2.2\text{ m}^2$, corte $0.01\,\mu\text{m}$). | Módulo central de separación y potabilización por Ultrafiltración en flujo cruzado. | 🟢 **En mano** |
| **Válvula de Retentado** | 1 | Válvula reguladora para línea de concentrado. | Control fino de contrapresión hidráulica y factor de recuperación ($Y$). | 🟢 **Comprada / En mano** |
| **Fuentes Auxiliares 12V y 5V** | 2 | Fuentes conmutadas AC/DC compactas (12V 1.5A y 5V 2A). | Alimentación de lógica digital, relés, L298N y transductores de instrumentación. | 🟢 **En mano** |

---

## 🟡 2. Acondicionamiento de Señal y Filtros de Hardware Pendientes

Debido a que el motor NEMA 34 opera con conmutación chopper inductiva de 3.0A mediante el driver Leadshine DM860, se generan perturbaciones electromagnéticas (EMI) conducidas y radiadas. Además, los pines GPIO del ESP32 **soportan un máximo absoluto de 3.3V**.

### Circuito de Acondicionamiento para Caudalímetros YF-S401 (x2):
```
        +3.3V (Shield ESP32)
           │
          [R1: 4.7 kΩ Pull-Up]
           │
Cable Amarillo (Señal Hall) ──┬───[ R2: 1 kΩ ]───┬───► GPIO ESP32 (Interrupción IRAM)
(YF-S401 alimentado a +5V)     │                  │
                              === C1: 100 nF     === C2: 10 nF (Opcional HF)
                               │  Cerámico        │
Cable Negro (GND) ────────────┴──────────────────┴───► GND Común (Shield ESP32)
```

- **Guía de Montaje Paso a Paso**: Consultar [Guia_Montaje_Placa_Filtrado_FrontEnd.md](../../02_Hito2_Instrumentacion_Sensores/01_Guias_Montaje_y_Calibracion/Guia_Montaje_Placa_Filtrado_FrontEnd.md) para el esquema detallado con la segunda bornera ZS-1057.

- **Materiales a adquirir en Salta Capital (Casas de Electrónica: Electrónica Salta / Norte Electrónica)**:
  1. **2x Resistencias Metal Film $4.7\text{ k}\Omega$ - $1/4\text{W}$** (Pull-Up de colector abierto a 3.3V seguro).
  2. **2x Resistencias Metal Film $1.0\text{ k}\Omega$ - $1/4\text{W}$** (Limitación de corriente hacia el GPIO).
  3. **2x Capacitores cerámicos multicapa $100\text{ nF}$ (código 104)** ($f_c \approx 1.6\text{ kHz}$, fulmina el ruido del motor).
  4. **1x Resistencia $4.7\text{ k}\Omega$** (Pull-Up requerido para la línea de datos 1-Wire del sensor DS18B20).
  5. **3 metros de Cable Mallado / Apantallado** tipo estéreo o $3 \times 0.25\text{ mm}^2$ (LiYCY) para prolongar los caudalímetros.

---

## 🔴 3. Faltantes Críticos de Instrumentación y Piping

| Ítem | Componente Técnico | Especificación Comercial Exacta | Cant. | Prioridad | Dónde Comprar |
| :---: | :--- | :--- | :---: | :---: | :--- |
| **F-01** | **Transductores de Presión Hidráulica** | Rango $0 - 1.2\text{ bar}$ (o $0 - 16\text{ psi}$ / $0 - 1.0\text{ bar}$), cuerpo Acero Inox AISI 316, rosca $1/4''\text{ NPT}$ macho, salida $0.5 - 4.5\text{ V}$ ratiométrica. | 3 | **URGENTE** | Mercado Libre (Buscar: *"Transductor de presión 5V 1.2 bar 1/4 NPT"*). |
| **F-02** | **Líneas de Sangre para Diálisis** | Set de líneas arterio-venosas descartables para hemodiálisis (Fresenius / Nipro / Gambro). | 1 set | **URGENTE** | Droguerías médicas en Salta o Mercado Libre (Descartable económico con los conectores ISO 8637 exactos del FX100). |
| **F-03** | **Soporte Mecánico para FX100** | Abrazaderas isofónicas con perfil de goma EPDM $\varnothing 60-70\text{ mm}$ (2 1/2") con espárrago M8. | 2 | **URGENTE** | Ferreterías industriales de Salta (Av. Chile / Av. Paraguay) o sanitarias. |
| **F-04** | **Tés de Derivación de Presión** | Tés hembra-hembra-hembra de $1/4''$ en latón niquelado o acero inoxidable. | 3 | Media | Ferreterías industriales / Casas de gas y refrigeración de Salta. |
| **F-05** | **Espigas y Conectores de 1/4"** | Espigas de $1/4''$ rosca macho a manguera de $1/4''$ (6.4 mm) en latón o plástico acetal/PP. | 6 | Media | Casas de refrigeración / sanitarias en Salta. |
| **F-06** | **Amortiguador de Pulsaciones (*Damper*)** | Trampa de aire vertical en derivación T de $1/4''$ con niple ciego de $15\text{ cm}$ para amortiguar rodillo. | 1 | **ALTA** | Armado en taller con niple y tapón de $1/4''$. |
| **F-07** | **Válvula Antirretorno (*Check*)** | Válvula check de $1/4''$ con resorte blando ($P_{\text{cracking}} \le 0.05\text{ bar}$). | 1 | Media | Casas de neumática / hidráulica en Salta. |

---

## ⚡ 4. Tablero Eléctrico de Control y Distribución (Riel DIN)

Para garantizar una presentación industrial, aislamiento térmico/electromagnético y máxima seguridad operativa:

### Arquitectura de Ubicación:
- **Externo al Tablero (Montado sobre la estructura de la planta)**:
  - Transformador 220V / 24VAC (debido a su peso de ~2 kg, vibración a 50 Hz e irradiación electromagnética).
  - Conjunto NEMA 34 acoplado al cabezal MBP-2000.
- **Interno al Tablero (Gabinete Estanco IP65)**:
  - Driver Leadshine DM860 (montado con espacio vertical para disipación de calor).
  - ESP32 + Shield ZS-1057 + Conversor ADS1115.
  - Driver L298N (para el motor de paletas del sedimentador).
  - Fuentes conmutadas auxiliares (12V y 5V) + Step-Down LM2596.
  - Protecciones eléctricas (Termomagnética + Portafusibles).
  - Borneras de distribución de Riel DIN.

### Lista de Compras para el Tablero (Distribuidores en Salta: Electro Alem, Crespo, D'Avanzo, Electro Salta):
1. **1x Gabinete Estanco Plástico IP65 con Tapa Transparente**: Medidas recomendadas $400 \times 300 \times 160\text{ mm}$ (marcas Roker serie PR, Genrod o Gabexel).
2. **1x Tramo de Riel DIN Simétrico de 35 mm** ($1\text{ metro}$, chapa cincada perforada).
3. **1x Interruptor Termomagnético Bipolar $2 \times 6\text{A}$ Curva C** (corte general de 220V a fuentes y trafo).
4. **2x Borneras Portafusible para Riel DIN con fusibles de vidrio $5 \times 20\text{ mm}$ (1A y 2A rápidos)** para líneas de 5V y 12V.
5. **Borneras de Paso tipo Clemas (sección $2.5\text{ mm}^2$ - marcas Zoloda / WAGO / genéricas)**:
   - **10 unidades Color Gris**: Distribución de señales digitales, I2C y entradas analógicas.
   - **6 unidades Color Azul**: Barras de masa común ($0\text{V}$ DC y GND lógico) unidas con peine puenteador.
   - **4 unidades Color Rojo / Naranja**: Distribución de positivos (+5V y +12V).
   - **2 unidades Color Verde / Amarillo**: Puesta a tierra de protección mecánica (PE) fijada directo al riel DIN.
6. **4x Prensaestopas de Poliamida (Pasa-cables estancos PG9 / PG11)** para el ingreso/egreso prolijo de mangueras de cables al fondo del gabinete.
7. **Punteras Huecas Crimpables (Ferrules)** de $0.5\text{ mm}^2$ y $1.0\text{ mm}^2$ para evitar pelos de cobre sueltos.

---

## 📐 5. Mapeo Definitivo de Puertos del Módulo Fresenius FX100

```
                        [ RETENTADO / CONCENTRADO ]
                        Puerto Axial Superior (ISO 8637)
                                     ▲
                                     │
                             ┌───────┴───────┐
    [ PERMEADO / FILTRADO ]  │               │
    Puerto Lateral Superior ─┤   Cartucho    │
    (Hansen Cilíndrico Fino) │ Fresenius FX100│
    Hacia Caudalímetro Perm  │               │
                             │ (Fibras       │
                             │  Capilares    │
    [ PUERTO LATERAL INF. ]  │  Helixone)    │
    Tapado Hermético (Ciego)─┤               │
                             └───────┬───────┘
                                     ▲
                                     │
                        [ ALIMENTACIÓN / FEED ]
                        Puerto Axial Inferior (ISO 8637)
                        Desde Salida Bomba MBP-2000
```

1. **Circuito de Flujo Cruzado (Capilares Internos)**:
   - Ingreso por el cono axial inferior (Alimentación con sensor $P_{\text{feed}}$).
   - Circulación longitudinal a alta velocidad por el interior de las fibras huecas.
   - Egreso por el cono axial superior (Retentado con sensor $P_{\text{ret}}$ hacia la válvula reguladora).
2. **Circuito de Filtración (Carcasa Externa)**:
   - El agua que pasa a través de la pared porosa de las fibras se recolecta en la cavidad externa.
   - Sale exclusivamente por el puerto lateral superior hacia el sensor $P_{\text{perm}}$ y el segundo caudalímetro YF-S401.
   - El puerto lateral inferior permanece clausurado con tapón hermético.

---

## 📌 6. Checklist de Compras y Tareas Inmediatas

- [ ] **Comprar en Casa de Electrónica de Salta**:
  - [ ] 3x Resistencias $4.7\text{ k}\Omega$ (1/4W)
  - [ ] 2x Resistencias $1.0\text{ k}\Omega$ (1/4W)
  - [ ] 2x Capacitores cerámicos multicapa $100\text{ nF}$ (104)
  - [ ] 3 metros de cable apantallado / mallado tipo LiYCY
- [ ] **Comprar en Casa de Electricidad Industrial de Salta (Electro Alem / D'Avanzo / Crespo)**:
  - [ ] Gabinete estanco IP65 plástico ($400 \times 300\text{ mm}$) con tapa transparente
  - [ ] 1 metro de Riel DIN 35 mm
  - [ ] 20 borneras de paso DIN ($2.5\text{ mm}^2$: 10 grises, 6 azules, 4 rojas, 2 tierra) + peines de puenteo
  - [ ] 1 termomagnética bipolar $2 \times 6\text{A}$ o $2 \times 10\text{A}$
  - [ ] 4 prensaestopas PG9/PG11
- [ ] **Comprar en Ferretería Industrial / Sanitarios de Salta (Av. Chile / Av. Paraguay)**:
  - [ ] 2 abrazaderas isofónicas con perfil de goma $\varnothing 60-70\text{ mm}$ con varilla roscada para el FX100
  - [ ] 3 Tés hembra de $1/4''$ (bronce niquelado) y 6 espigas para manguera de $1/4''$
  - [ ] Niple de $15\text{ cm}$ con tapón ciego de $1/4''$ (amortiguador de pulsaciones)
- [ ] **Comprar por Mercado Libre (Envíos a Salta)**:
  - [ ] 3x Transductores de presión $0 - 1.2\text{ bar}$ rosca $1/4''$ NPT (salida $0.5 - 4.5\text{ V}$)
  - [ ] 1x Set descartable de líneas de sangre para hemodiálisis (para extraer conectores ISO 8637)
