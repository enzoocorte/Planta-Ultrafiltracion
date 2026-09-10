# 🚰 GUÍA DE MONTAJE HIDRÁULICO DE SENSORES

Esta guía detalla la ubicación física recomendada para cada sensor en el circuito hidráulico de la planta de ultrafiltración.

---

## 🗺️ 1. Diagrama de Ubicación de Instrumentación

```
                        ESQUEMA DE UBICACIÓN FÍSICA DE SENSORES
                        =======================================

   [ SEDIMENTADOR ]
         │
         ├──► [ Sonda TDS 1 (Agua Cruda / Alimentación) ]
         │
         ▼
   [ BOMBA PERISTÁLTICA MBP-2000 ]
         │
         ▼
   [ MÓDULO ULTRAFILTRACIÓN FRESENIUS FX100 ]
         │
         ├──────────────────────────────────────────┐
         │ (Línea de Permeado - Agua Filtrada)      │ (Línea de Retentado / Concentrado)
         ▼                                          ▼
   [ Caudalímetro YF-S401 (Permeado Qp) ]     [ Caudalímetro YF-S401 (Retentado Qc) ]
         │                                          │
         ├─► [ Sonda DS18B20 (Temp Permeado) ]      ▼
         ├─► [ Sonda TDS 2 (Calidad Permeado) ]   [ Válvula Reguladora de Retentado ]
         ▼                                          │
   [ TANQUE DE RECOLECCIÓN PERMEADO ]               ▼
                                              [ RECIRCULACIÓN O DESCARTE ]
```

---

## 🔧 2. Consejos Prácticos de Instalación

1. **Sentido de Flujo en Caudalímetros YF-S401**:
   * En el cuerpo plástico de cada caudalímetro hay una **flecha grabada** que indica la dirección del agua. Instalar siempre con la flecha apuntando en el sentido del flujo.
   * Mantener el caudalímetro en posición horizontal para evitar burbujas atrapadas en la cámara de la turbina.
2. **Inmersión de las Sondas (DS18B20 y TDS)**:
   * La sonda de acero inoxidable del DS18B20 y los electrodos de la sonda TDS deben quedar permanentemente sumergidos en la corriente líquida sin tocar las paredes metálicas de los recipientes para evitar lecturas falsas de conductividad.

---

## 🎯 Entregable Concreto de esta Guía
* **Línea hidráulica instrumentada al 100%**: Caudalímetro de permeado ($Q_p$), caudalímetro de retentado ($Q_c$), sonda sumergible DS18B20 y sensor TDS montados sin fugas de agua y con la flecha de flujo correctamente orientada.

---

## 📋 Lista de Verificación (Checklist de Avance)
- [ ] Caudalímetro de Permeado YF-S401 intercalado en la manguera cristal de salida del FX100 con la flecha en dirección al tanque de agua tratada.
- [ ] Caudalímetro de Retentado YF-S401 intercalado en la línea de concentrado con su respectiva válvula de estrangulamiento.
- [ ] Abrazaderas plásticas o zunchos colocados en las espigas de los caudalímetros para evitar desacoples por pulsos de presión.
- [ ] Sonda DS18B20 asegurada en la cámara de medición de permeado, completamente sumergida.
- [ ] Sonda TDS sumergida sin rozar los electrodos contra superficies metálicas.
- [ ] Prueba estática de estanqueidad: Circulación de agua a 30 RPM durante 5 minutos con 0 goteos en las uniones.
