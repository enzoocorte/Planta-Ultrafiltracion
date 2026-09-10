# 🎮 SIMULADOR INTERACTIVO DEL ACCIONAMIENTO NEMA 34 & DRIVER DM860

Este simulador permite al equipo visualizar la respuesta dinámica, la rampa de aceleración y el comportamiento térmico del motor NEMA 34 antes y durante las pruebas físicas en el laboratorio.

---

## 🎯 Entregable Concreto
* **Entorno de simulación mecatrónica en tiempo real**: Ejecutable directamente en cualquier navegador web sin necesidad de compilar ni instalar librerías.
* **Validación gráfica del interruptor SW4**: Demostración visual interactiva de por qué SW4 en `OFF` mantiene el motor frío ($\approx 28-35^\circ\text{C}$) mientras que en `ON` sobrecalienta a $>80^\circ\text{C}$.

---

## 🚀 Cómo Ejecutar el Simulador
1. Abre el archivo [`simulador_bomba.html`](./simulador_bomba.html) haciendo doble clic en él o arrastrándolo a Google Chrome / Microsoft Edge.
2. Utiliza los controles:
   * **Botón INICIAR / DETENER**: Observa la rampa suave de subida y bajada ($35\text{ RPM/s}$).
   * **Slider de RPM**: Modifica la velocidad entre $0\text{ y }120\text{ RPM}$ y comprueba la frecuencia sintetizada en Hz.
   * **Botón SW4**: Alterna entre `SW4 = OFF` (óptimo) y `SW4 = ON` para observar el calentamiento térmico modelado.

---

## 📋 Lista de Verificación (Checklist de Validación)
- [ ] Archivo `simulador_bomba.html` abierto y renderizando correctamente el cabezal de la bomba y el tren de pulsos.
- [ ] Verificación de la frecuencia de cálculo: a $30\text{ RPM} = 800\text{ Hz}$, a $60\text{ RPM} = 1600\text{ Hz}$.
- [ ] Comprensión gráfica del tiempo de rampa: el motor tarda $\approx 1.7\text{ segundos}$ en acelerar de 0 a 60 RPM.
- [ ] Validación del interruptor SW4: verificación de la reducción al 50% de corriente en reposo.
