# 🛒 LISTA DE COMPRAS Y TALLER: HARDWARE MECÁNICO Y ELECTRÓNICO (AGITADOR 25L)
## Hito 3: Montaje del Agitador sobre Tapa de Acero Inoxidable y Driver BTS7960
**Proyecto:** Planta Piloto de Ultrafiltración FX100 & Reactor Sedimentador  
**Referencia:** Dictamen Técnico Cruzado Antigravity & Argon (Octubre 2026)  
**Destinatarios:** Ing. Enzo Corte, Antonella Guitián & Owen Cañizares

---

## 🎯 1. Objetivo de la Lista
Presentar en una sola planilla consolidada todos los elementos mecánicos, de ferretería y de electrónica de potencia que deben adquirirse o conseguirse en taller para armar el sistema de agitación del reactor de 25L sin forzar componentes y con confiabilidad industrial.

---

## 📋 2. Planilla de Compras y Materiales

### A. Actuador (Motorreductor 12V DC)
| Ítem | Componente | Cant. | Especificación Técnica | Dónde Conseguir |
| :--- | :--- | :---: | :--- | :--- |
| **A.1 (Comercial)** | **Motorreductor Sinfín-Corona 5840-31ZY** | 1 | 12V DC, **100 RPM**, torque nominal $25\text{ kg}\cdot\text{cm}$, torque stall $>70\text{ kg}\cdot\text{cm}$, eje macizo de **8 mm con plano D**. Autoblocante. | Mercado Libre / Distribuidores de robótica y automatización. |
| **A.2 (Reciclado)** | **Motor de Limpiaparabrisas Automotriz** | 1 | 12V DC, dos velocidades (~50 y ~90 RPM), torque $30 - 60\text{ kg}\cdot\text{cm}$. Brida 3 agujeros M6. | Taller mecánico, desarmadero automotriz habilitado, repuestera. |

> [!WARNING]
> **Prueba obligatoria con multímetro si usan Limpiaparabrisas:**  
> Medir continuidad entre los cables y la carcasa metálica del motor. Si la carcasa tiene continuidad con el polo negativo, aislar el montaje del tanque usando bujes y arandelas de goma/nylon. Usar el borne de **alta velocidad** para modular por PWM.

---

### B. Tren Mecánico y Soporte en Tapa
| Ítem | Componente | Cant. | Especificación Técnica | Función |
| :--- | :--- | :---: | :--- | :--- |
| **B.1** | **Chumacera con Brida KFL08** | 1 | Rodamiento autoalineante de pared para **eje Ø 8 mm**, fundición de zinc o acero inoxidable con 2 prisioneros allen. | Absorbe el 100% de la carga axial y radial de la paleta. Evita romper el motor. |
| **B.2** | **Acoplamiento Elástico tipo Spider / Mandíbula** | 1 | Diámetro exterior 20–25 mm, acople flexible con elastómero central. **Boca A: 8 mm / Boca B: 8 mm** (o según eje del motor). | Une el motor con el eje absorbiendo desalineaciones angulares. |
| **B.3** | **Placa de Refuerzo Sándwich (Grilón / Nylon)** | 1 | Placa de **120 x 120 mm**, espesor **8 a 10 mm** (o planchuela de inox 3 mm). | Se atornilla contra la tapa para rigidizar la chapa, anular vibraciones y aislar. |
| **B.4** | **Separadores / Standoffs Torreta** | 3 o 4 | Espárragos o separadores hexagonales **M6 x 45–50 mm** de acero cincado o inox. | Elevan el motor sobre la tapa dejando espacio libre para el acople elástico. |
| **B.5** | **Retén Antisalpicadura / Polvo** | 1 | Retén de labio de nitrilo Ø interior 8 mm (o arandela ajustada de Teflón/PTFE). | Se coloca debajo de la tapa para proteger la chumacera del vapor del agua. |
| **B.6** | **Tornillería Inox** | 1 juego | Tornillos M5 y M6 con tuercas autofrenantes y arandelas planas/grower en acero inoxidable AISI 304. | Fijación de la chumacera y la placa a la tapa del sedimentador. |

---

### C. Electrónica de Potencia y Conectividad
| Ítem | Componente | Cant. | Especificación Técnica | Función |
| :--- | :--- | :---: | :--- | :--- |
| **C.1** | **Driver MOSFET BTS7960 (IBT-2)** | 1 | Módulo de **43 A**, compatible con lógica 3.3V nativa del ESP32, disipador de aluminio incluido. | Reemplaza al obsoleto L298N. Cero caída de tensión, frenado activo. |
| **C.2** | **Conector Circular Industrial GX16 (4 Pines)** | 1 par | Ficha tipo aviación de 4 contactos a rosca metálica: 1 ficha chasis hembra + 1 ficha aérea macho. | Conexión rápida y estanca entre el maletín y el sedimentador. |
| **C.3** | **Cable Tipo Taller / TPR Envainado** | 2.5 m | Manguera flexible redonda negra de **4 x 1.0 mm² o 4 x 1.5 mm²**. | Lleva 2 hilos de potencia al motor (12V PWM) y 2 hilos de señal a la boya. |
| **C.4** | **Resistencia y Capacitor Anti-Ruido** | 1 c/u | Resistencia fija de **$2.2\text{ k}\Omega$** (1/4 W) + Capacitor cerámico **$100\text{ nF}$ (0.1 µF / 50V)**. | Filtro hardware en el maletín en `GPIO 32` para inmunizar la boya de nivel. |

---

## 🛠️ 3. Pasos de Mecanizado y Montaje en Taller

```
                              SECUENCIA DE ARMADO
                              ===================

  1. PERFORACIÓN EXCÉNTRICA:
     Marcar el agujero del eje en la tapa DESPLAZADO 25 a 35 mm del centro geométrico
     (para romper la simetría y anular el vórtice central sin soldar deflectores).
     Perforar con mecha copa o escalonada a Ø 12 - 14 mm.

  2. MONTAJE SÁNDWICH DE GRILÓN Y CHUMACERA:
     • Colocar la placa de Grilón (120x120 mm) sobre la cara exterior de la tapa.
     • Perforar los agujeros de anclaje de la chumacera KFL08 atravesando el Grilón y la tapa.
     • Atornillar la chumacera KFL08 con tornillos pasantes M5/M6 de inox y tuercas autofrenantes.

  3. INSTALACIÓN DE LA TORRETA Y MOTOR:
     • Fijar los 3 o 4 separadores M6 de 50 mm sobre la placa de Grilón.
     • Calzar el acoplamiento elástico spider en el eje del motor y en el eje de la paleta.
     • Ajustar los prisioneros allen sobre el plano D del eje.
     • Atornillar el motor a la parte superior de los separadores.

  4. REGULACIÓN DE ALTURA DE LA PALETA:
     • Deslizar el eje a través de la chumacera KFL08.
     • Verificar que la paleta quede ubicada entre 5 y 8 cm por encima de la línea donde
       comienza el fondo cónico de 60°.
     • Ajustar con fuerza los 2 prisioneros allen de la chumacera KFL08 para fijar la posición axial.
```

---

## 📍 4. Comercios Sugeridos en Salta Capital

1. **Rodamientos y Chumaceras KFL08 / Acoples elásticos:**
   * **Rodamientos Salta:** Av. San Martín / Pellegrini.
   * **Casa del Rodamiento:** Calle Ituzaingó / Pellegrini.
   * **Mercado Libre:** Búsqueda: *"Chumacera KFL08 8mm"* y *"Acople elástico mandíbula 8mm a 8mm"*.
2. **Placa de Grilón / Nylon y Tornillería Inox:**
   * **Bulonera Salta:** Av. Chile o Av. San Martín (bulonería y plásticos técnicos).
   * **Ferreterías Industriales:** Bulones de acero inoxidable AISI 304 M5 y M6.
3. **Electrónica (BTS7960, Fichas GX16, Cable TPR, Capacitor 100nF):**
   * **Electrónica Salta / Megatone Componentes / Microelectrónica:** Calle Alvarado / Urquiza.
   * **Mercado Libre:** Búsqueda: *"Driver BTS7960 43A"* y *"Conector GX16 4 pines macho hembra"*.
