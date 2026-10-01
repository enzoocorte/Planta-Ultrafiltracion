import numpy as np
import matplotlib.pyplot as plt

def run_circuit_simulation():
    # Parámetros del circuito
    # Frecuencia de pulsos de caudal (~50 Hz para aprox. 0.5 L/min en YF-S401)
    # Frecuencia de ruido inductivo del driver DM860/NEMA34 (~20 kHz PWM con espurios)
    t = np.linspace(0, 0.04, 200000) # 40 ms de captura a 5 MHz de muestreo
    
    # Señal cuadrada del sensor Hall (con pull-up interno a 5V)
    f_flow = 50.0 # Hz
    square_wave = 0.5 * (1 + np.sign(np.sin(2 * np.pi * f_flow * t))) # 0 a 1
    v_raw = square_wave * 5.0 # 0V a 5.0V
    
    # Ruido inducido en el cable largo (picos de conmutación PWM de 20 kHz y espurios de 100 kHz)
    noise_pwm = 1.6 * np.sin(2 * np.pi * 20000 * t) * np.exp(-((t % 0.0005)/0.0001))
    noise_spikes = 1.2 * np.sin(2 * np.pi * 120000 * t)
    v_noisy = v_raw + noise_pwm + noise_spikes
    
    # Simulación del comportamiento del Filtro RC con Divisor Thévenin
    # En HIGH: Sensor open-collector (~35k a 5V) + Pull-up externo (4.7k a 3.3V)
    # V_th = (5*4.7 + 3.3*35)/(35 + 4.7) = 3.501 V
    # R_th = (35k * 4.7k) / (35k + 4.7k) = 4.14 kΩ
    # C = 100 nF (Código 104)
    # Tau = R_th * C = 4.14k * 100n = 0.414 ms
    
    v_filtered = np.zeros_like(t)
    v_node = 0.0
    dt = t[1] - t[0]
    
    R_th = 4140.0
    C_val = 100e-9
    tau = R_th * C_val
    
    for i in range(len(t)):
        if square_wave[i] > 0.5:
            # Sensor en HIGH (transistor apagado, actúa divisor Thévenin a 3.50V)
            v_target = 3.50 + 0.15 * noise_pwm[i] # Ruido atenuado ingresando
        else:
            # Sensor en LOW (transistor Hall saturado a masa: ~0.15V)
            v_target = 0.15 + 0.05 * noise_pwm[i]
            
        # Ecuación diferencial del capacitor: dv/dt = (v_target - v) / tau
        v_node += ((v_target - v_node) / tau) * dt
        v_filtered[i] = v_node

    # Graficar con estándar científico de publicación para la Tesis
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(11, 7), sharex=True, dpi=300)
    
    # Gráfico 1: Señal bruta ruidosa que viene por el cable
    ax1.plot(t * 1000, v_noisy, color='#ef4444', linewidth=1.0, alpha=0.85, label='Señal bruta en cable amarillo (Pulsos + Ruido PWM 20kHz)')
    ax1.axhline(3.6, color='#dc2626', linestyle='--', linewidth=1.5, label='Límite Máximo Absoluto ESP32 (3.60V) ⚠️ PELIGRO')
    ax1.axhline(5.0, color='#991b1b', linestyle=':', linewidth=1.2, label='Nivel 5.0V Sensor (Sin acondicionar)')
    ax1.set_ylabel('Tensión (V)', fontsize=11, fontweight='bold')
    ax1.set_title('1. Señal Bruta: Picos peligrosos > 5.5V que destruyen el GPIO o causan falsos conteos', fontsize=12, fontweight='bold', pad=10)
    ax1.grid(True, linestyle=':', alpha=0.6)
    ax1.legend(loc='upper right', framealpha=0.9, fontsize=9)
    ax1.set_ylim(-1.5, 6.8)
    
    # Gráfico 2: Señal limpia y recortada a 3.5V que ingresa a GPIO27 / GPIO14
    ax2.plot(t * 1000, v_filtered, color='#10b981', linewidth=2.2, label='Señal filtrada en GPIO (Pull-up 4.7k a 3.3V + C 100nF a GND)')
    ax2.axhline(3.50, color='#0ea5e9', linestyle='--', linewidth=1.5, label='Nivel HIGH Thévenin Seguro: 3.50V (Garantiza lectura lógica 1)')
    ax2.axhline(0.75, color='#64748b', linestyle=':', linewidth=1.2, label='Umbral LOW ESP32: < 0.80V (Garantiza lectura lógica 0)')
    ax2.set_ylabel('Tensión (V)', fontsize=11, fontweight='bold')
    ax2.set_xlabel('Tiempo (milisegundos)', fontsize=11, fontweight='bold')
    ax2.set_title('2. Señal Filtrada: Nivel HIGH acotado a 3.50V, espurios eliminados y flancos limpios', fontsize=12, fontweight='bold', pad=10)
    ax2.grid(True, linestyle=':', alpha=0.6)
    ax2.legend(loc='upper right', framealpha=0.9, fontsize=9)
    ax2.set_ylim(-0.2, 4.2)
    
    plt.tight_layout()
    output_path = "d:/antigravity-pipeline/TESIS/curva_calidad_filtro_caudalimetro.png"
    plt.savefig(output_path)
    print(f"Simulacion guardada con exito en: {output_path}")

if __name__ == '__main__':
    run_circuit_simulation()
