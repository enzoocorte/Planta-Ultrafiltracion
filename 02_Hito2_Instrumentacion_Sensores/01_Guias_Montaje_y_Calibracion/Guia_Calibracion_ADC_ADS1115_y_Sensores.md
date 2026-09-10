# 📐 GUÍA DE CALIBRACIÓN Y SEÑALES: ADC ADS1115 (16 BITS), TDS, TEMPERATURA Y CAUDAL

Esta guía detalla el procesamiento matemático de señales, la calibración de los sensores y la lectura en alta resolución mediante el conversor I2C ADS1115.

---

## 🎯 Entregable Concreto
* **Curvas de calibración y algoritmos de conversión validados**: Medición de temperatura en $^\circ\text{C}$ ($\pm 0.5^\circ\text{C}$), TDS en $\text{ppm}$ compensado térmicamente a $25^\circ\text{C}$, y caudales $Q_p$ y $Q_c$ en $\text{L/min}$ con factor de $98\text{ pulsos/L}$.
* **Lectura I2C del conversor ADS1115 operativa**: Adquisición de señales analógicas de 16 bits sin ruido eléctrico.

---

## 🔬 1. Ecuaciones de Calibración Matemática

### A. Conversor ADC ADS1115 (16 Bits en Bus I2C)
* Con ganancia $\text{GAIN\_ONE}$ ($\pm 4.096\text{V}$), el conversor entrega 32768 cuentas en rango positivo:
  $$\text{Resolución LSB} = \frac{4096\text{ mV}}{32768} = 0.125\text{ mV por cuenta}$$
  $$V_{\text{medido}} (\text{V}) = \text{Lectura ADS1115} \times 0.000125\text{ V}$$

### B. Sensor de Calidad de Agua (TDS) con Compensación Térmica
Para cumplir la norma de agua segura del Código Alimentario Argentino (CAA), el voltaje de la sonda debe normalizarse a $25^\circ\text{C}$:
$$V_{25} = \frac{V_{\text{medido}}}{1.0 + 0.02 \times (T_{\text{agua}} - 25.0)}$$
$$\text{TDS} (\text{ppm}) = (133.42 \times V_{25}^3 - 255.86 \times V_{25}^2 + 857.39 \times V_{25}) \times 0.5$$

### C. Caudalímetros de Turbina YF-S401
$$Q (\text{L/min}) = \frac{\text{Pulsos acumulados en 1 segundo}}{98.0}$$
$$\text{Volumen Total} (\text{L}) = \sum \left(\frac{Q (\text{L/min})}{60} \times \Delta t (\text{s})\right)$$

---

## 📋 Lista de Verificación (Checklist de Calibración)
- [ ] Módulo ADS1115 conectado por I2C al ESP32: `GPIO 21` (SDA), `GPIO 22` (SCL), `VDD` a 5V/3.3V y `GND` común.
- [ ] Escaneo I2C ejecutado con éxito detectando el ADS1115 en la dirección `0x48`.
- [ ] Resistencia de pull-up de $4.7\text{ k}\Omega$ conectada entre el cable amarillo (Datos) del DS18B20 y los $3.3\text{V}$.
- [ ] Lectura de temperatura del DS18B20 en el Monitor Serie coherente con la temperatura ambiente del laboratorio ($\approx 20 - 25^\circ\text{C}$).
- [ ] Calibración del sensor TDS con solución patrón de $1413\,\mu\text{S/cm}$ o agua potable de red conocida ($\approx 120 - 250\text{ ppm}$).
- [ ] Calibración gravimétrica del caudalímetro: Medir con probeta graduada durante 1 minuto a 60 RPM y verificar que coincida con el volumen registrado por el firmware ($\pm 5\%$).
