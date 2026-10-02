// =============================================================================
// ARNES DE VERIFICACION METROLOGICA — Caudalimetro (firmware v4, UNSa)
//
// Compila y EJECUTA el caudalimetro.cpp ORIGINAL del firmware v4 (sin tocarlo)
// sobre un reloj virtual de 32 bits, para comprobar numericamente:
//   T1  Exactitud en regimen permanente (periodo reciproco, n>=2)
//   T2  No atenuacion por la "cota fisica superior" en regimen permanente
//   T3  Decaimiento tras parada: forma 1/t, escalon final y su magnitud
//   T4  Inmunidad al rollover de micros() (cruce REAL de 2^32 us)
//   T5  Exactitud dimensional de K [Hz/(L/min)] y de V[L] = n/(K*60)
//   T6  Limite de deteccion (regla de cero duro a los 3 s)
//   T7  Blanking anti-rebote (1500 us) ante rafaga EMI
//   T8  Perdida de resolucion del acumulador float _vol
// =============================================================================
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include "caudalimetro.h"

// ---- Globales del shim ------------------------------------------------------
uint32_t g_micros     = 0;
uint64_t g_micros_abs = 0;
voidInterruptPtr g_isr_fn  = nullptr;
void*            g_isr_arg = nullptr;
unsigned long    g_cs_entradas = 0;
SerialStub       Serial;

// ---- Reloj virtual ----------------------------------------------------------
// g_micros es SIEMPRE el truncamiento a 32 bits del tiempo absoluto, tal como
// ocurre en el ESP32 real. Cualquier cruce por 2^32 es un rollover genuino.
static void setAbs(uint64_t t) { g_micros_abs = t; g_micros = (uint32_t)t; }
static void pulse() { if (g_isr_fn) g_isr_fn(g_isr_arg); }

static int g_fallos = 0;
static void check(bool ok, const char* que, const char* det) {
  std::printf("   [%s] %-42s %s\n", ok ? " OK " : "FALLO", que, det);
  if (!ok) g_fallos++;
}

// Emite los flancos de un tren de periodo T_us que caen en [t_from, t_to)
static unsigned long emitir(uint64_t t_from, uint64_t t_to, double T_us, double* fase) {
  unsigned long n = 0;
  while (*fase < (double)t_to) {
    if (*fase >= (double)t_from) { setAbs((uint64_t)llround(*fase)); pulse(); n++; }
    *fase += T_us;
  }
  return n;
}

int main() {
  char b[320];

  std::printf("=====================================================================\n");
  std::printf(" VERIFICACION EN HOST — caudalimetro.cpp v4 ejecutado sin modificar\n");
  std::printf(" K_ALIMENTACION=%.2f Hz/(L/min)   K_PERMEADO=%.2f Hz/(L/min)\n",
              K_ALIMENTACION, K_PERMEADO);
  std::printf(" FILTRO_RUIDO_US=%u us    Q_MAX_FISICO_MLMIN=%.0f mL/min\n",
              (unsigned)FILTRO_RUIDO_US, Q_MAX_FISICO_MLMIN);
  std::printf("=====================================================================\n\n");

  // ===========================================================================
  std::printf("T1/T2  REGIMEN PERMANENTE — exactitud y no-atenuacion por la cota\n");
  std::printf("   Q_true     f_true    n/ventana   f_med     Q_med      err      cota recorta?\n");
  std::printf("  [mL/min]     [Hz]                 [Hz]     [mL/min]    [%%]\n");
  // ===========================================================================
  {
    const double qTrue[] = {6.47, 18.2, 250, 680, 1360, 4200};
    for (double qt : qTrue) {
      Caudalimetro c(14, K_ALIMENTACION, "TEST", true);
      c.begin();
      double f  = qt * K_ALIMENTACION / 1000.0;
      double Tu = 1e6 / f;
      double fase = 1000000.0;
      uint64_t t_end = 1000000;
      double last_q = 0;
      unsigned long nvent = 0;
      for (int w = 0; w < 40; w++) {
        uint64_t nxt = t_end + 1000000ULL;
        nvent = emitir(t_end, nxt, Tu, &fase);
        setAbs(nxt); t_end = nxt;
        c.actualizar(1.0f, true);
        last_q = c.caudal_mLmin();
      }
      double err = (last_q - qt) / qt * 100.0;
      bool recorta = (c.frecuencia_Hz() < f * 0.999);
      std::printf("  %8.2f  %8.3f   %9lu  %8.3f  %9.2f  %+7.3f   %s\n",
                  qt, f, nvent, c.frecuencia_Hz(), last_q, err,
                  recorta ? "SI <-- PROBLEMA" : "no");
    }
    std::printf("\n");
  }

  // ===========================================================================
  std::printf("T3  PARADA — decaimiento de f tras cortar el tren de pulsos\n");
  // ===========================================================================
  {
    Caudalimetro c(14, K_ALIMENTACION, "TEST", true);
    c.begin();
    double f = 105.14, Tu = 1e6 / f, fase = 200000000.0;
    uint64_t t_end = 200000000;
    for (int w = 0; w < 5; w++) {
      uint64_t nxt = t_end + 1000000ULL;
      emitir(t_end, nxt, Tu, &fase);
      setAbs(nxt); t_end = nxt;
      c.actualizar(1.0f, true);
    }
    std::printf("   Regimen previo: f=%.2f Hz  Q=%.1f mL/min  (objetivo 680)\n",
                c.frecuencia_Hz(), c.caudal_mLmin());
    std::printf("   ventana   t_sin_flanco[s]    f[Hz]     Q[mL/min]\n");
    double q1s = 0.0;
    for (int k = 1; k <= 6; k++) {
      uint64_t nxt = t_end + 1000000ULL;
      setAbs(nxt); t_end = nxt;
      c.actualizar(1.0f, true);
      double q = c.caudal_mLmin();
      std::printf("   %7d   %14d   %9.5f  %10.3f\n", k, k, c.frecuencia_Hz(), q);
      if (k == 1) q1s = q;
    }
    snprintf(b, sizeof b, "1er segundo tras STOP = %.1f mL/min (%.0f%% del regimen); cero recien en la ventana 3",
             q1s, q1s / 680.0 * 100.0);
    check(q1s < 0.05 * 680.0, "Decaimiento 'suave y asintotico' declarado en el comentario", b);
    std::printf("\n");
  }

  // ===========================================================================
  std::printf("T4  ROLLOVER — cruce REAL de micros() por 2^32 durante el ensayo\n");
  std::printf("   ventana      micros()      f_med[Hz]   err[%%]\n");
  // ===========================================================================
  {
    Caudalimetro c(14, K_ALIMENTACION, "TEST", true);
    c.begin();
    const uint64_t inicio = 4294967296ULL - 3000000ULL;   // 3 s antes del cruce
    double f = 105.14, Tu = 1e6 / f, fase = (double)inicio;
    uint64_t t_end = inicio;
    bool cruzo = false; double peor = 0; int ventanaCruce = -1;
    for (int w = 1; w <= 10; w++) {
      uint64_t nxt = t_end + 1000000ULL;
      emitir(t_end, nxt, Tu, &fase);
      setAbs(nxt); t_end = nxt;
      c.actualizar(1.0f, true);
      if (g_micros_abs >= 4294967296ULL && !cruzo) { cruzo = true; ventanaCruce = w; }
      double e = (c.frecuencia_Hz() - f) / f * 100.0;
      if (w >= 3 && std::fabs(e) > std::fabs(peor)) peor = e;
      std::printf("   %7d   %10u   %9.3f   %+.3f%s\n", w, g_micros, c.frecuencia_Hz(), e,
                  (w == ventanaCruce) ? "   <-- cruce 2^32 en esta ventana" : "");
    }
    snprintf(b, sizeof b, "peor desvio tras el cruce = %+.3f %% (ventana del cruce: %d)",
             peor, ventanaCruce);
    check(cruzo && std::fabs(peor) < 0.5, "Inmunidad al rollover (resta uint32_t)", b);
    std::printf("\n");
  }

  // ===========================================================================
  std::printf("T5  DIMENSIONES — K [Hz/(L/min)] y V[L] = n/(K*60)\n");
  // ===========================================================================
  {
    Caudalimetro c(14, K_ALIMENTACION, "TEST", true);
    c.begin();
    c.resetVolumen();
    // Se emite EXACTAMENTE la cantidad de pulsos que la teoria dice que hay en 1 L
    const double V = 1.0;
    double f  = 0.680 * K_ALIMENTACION;          // 105.14 Hz a 680 mL/min
    double Tu = 1e6 / f;
    unsigned long objetivo = (unsigned long)llround(K_ALIMENTACION * 60.0 * V);  // 9277
    double fase = 10000000.0; (void)fase;
    unsigned long nTot = 0;
    for (unsigned long i = 0; i < objetivo; i++) {
      setAbs(10000000ULL + (uint64_t)llround(i * Tu));
      pulse(); nTot++;
    }
    setAbs(10000000ULL + (uint64_t)llround((objetivo - 1) * Tu) + 1ULL);
    c.actualizar(1.0f, true);

    snprintf(b, sizeof b, "emitidos=%lu   teoricos K*60*V=%.1f   dif=%+.1f",
             nTot, K_ALIMENTACION * 60.0 * V, (double)nTot - K_ALIMENTACION * 60.0 * V);
    check(std::fabs((double)nTot - K_ALIMENTACION * 60.0 * V) <= 1.0, "pulsos por Litro = K * 60", b);

    snprintf(b, sizeof b, "V=%.6f L (objetivo 1.000000, err %+.4f %%)",
             c.volumen_L(), (c.volumen_L() - V) / V * 100.0);
    check(std::fabs(c.volumen_L() - V) / V < 2e-4, "V[L] = n / (K*60)", b);

    double q = f * 1000.0 / K_ALIMENTACION;
    snprintf(b, sizeof b, "Q = f*1000/K = %.3f mL/min (objetivo 680.000)", q);
    check(std::fabs(q - 680.0) < 0.01, "Q[mL/min] = f * 1000 / K", b);

    // mismo chequeo para el sensor de permeado
    double qperm = 5.50 * 1000.0 / K_PERMEADO;
    snprintf(b, sizeof b, "K_perm=%.2f: f=5.50 Hz -> Q=%.3f mL/min (objetivo 100.000)",
             K_PERMEADO, qperm);
    check(std::fabs(qperm - 100.0) < 0.01, "Coherencia K_PERMEADO", b);
    std::printf("\n");
  }

  // ===========================================================================
  std::printf("T6  LIMITE DE DETECCION — regla de cero duro a los 3 s\n");
  // ===========================================================================
  {
    for (int s = 0; s < 2; s++) {
      float K = (s == 0) ? K_ALIMENTACION : K_PERMEADO;
      Caudalimetro c(14, K, "TEST", true);
      c.begin();
      double Tu = 3600000.0;                    // f = 0.2778 Hz
      double f  = 1e6 / Tu;
      double fase = 60000000.0;
      uint64_t t_end = 60000000;
      int ceros = 0;
      for (int w = 0; w < 15; w++) {
        uint64_t nxt = t_end + 1000000ULL;
        emitir(t_end, nxt, Tu, &fase);
        setAbs(nxt); t_end = nxt;
        c.actualizar(1.0f, true);
        if (c.frecuencia_Hz() == 0.0f) ceros++;
      }
      double Qmin = (1.0 / 3.0) * 1000.0 / K;
      snprintf(b, sizeof b,
               "K=%7.2f  f_real=%.4f Hz  Q_real=%6.2f mL/min  ->  %2d/15 ventanas leen 0"
               "   |   Q_min teorico (f=0.333Hz) = %.2f mL/min",
               K, f, f * 1000.0 / K, ceros, Qmin);
      std::printf("   %s\n", b);
    }
    std::printf("\n");
  }

  // ===========================================================================
  std::printf("T7  BLANKING — rafaga EMI: 5 flancos separados 400 us\n");
  // ===========================================================================
  {
    Caudalimetro c(14, K_ALIMENTACION, "TEST", true);
    c.begin();
    for (int i = 0; i < 5; i++) { setAbs(90000000ULL + (uint64_t)i * 400ULL); pulse(); }
    setAbs(91000000ULL); pulse();               // un flanco legitimo 1 s despues
    setAbs(91000000ULL); c.actualizar(1.0f, true);
    snprintf(b, sizeof b, "f=%.4f Hz  Q=%.2f mL/min  (la rafaga NO debe inflar el caudal)",
             c.frecuencia_Hz(), c.caudal_mLmin());
    check(c.caudal_mLmin() < 50.0, "Rafaga EMI rechazada por blanking", b);
    std::printf("\n");
  }

  // ===========================================================================
  std::printf("T8  ACUMULADOR float _vol — donde se pierde resolucion\n");
  // ===========================================================================
  {
    for (int s = 0; s < 2; s++) {
      float K = (s == 0) ? K_ALIMENTACION : K_PERMEADO;
      float inc = 1.0f / (K * 60.0f);
      float vol = 0.0f; unsigned long n = 0;
      while (vol + inc > vol) { vol += inc; n++; }
      std::printf("   K=%7.2f  incremento=%.4e L/pulso -> _vol se congela en %.1f L (%lu pulsos)\n",
                  K, inc, vol, n);
    }
  }

  // ===========================================================================
  std::printf("T9  EMI SOSTENIDA justo por encima del blanking (dt >= 1500 us)\n");
  // ===========================================================================
  {
    const double femi[] = {600.0, 660.0, 666.0};
    for (double fe : femi) {
      Caudalimetro c(14, K_ALIMENTACION, "TEST", true);
      c.begin();
      double Tu = 1e6 / fe, fase = 100000000.0;
      uint64_t t_end = 100000000;
      for (int w = 0; w < 6; w++) {
        uint64_t nxt = t_end + 1000000ULL;
        emitir(t_end, nxt, Tu, &fase);
        setAbs(nxt); t_end = nxt;
        c.actualizar(1.0f, true);
      }
      snprintf(b, sizeof b, "EMI a %5.0f Hz -> Q=%.0f mL/min ; Q_MAX_FISICO=%.0f -> %s",
               fe, c.caudal_mLmin(), Q_MAX_FISICO_MLMIN,
               (c.caudal_mLmin() > Q_MAX_FISICO_MLMIN) ? "descartada" : "ACEPTADA como caudal real");
      std::printf("   %s\n", b);
    }
    std::printf("   Techo del blanking: f_max = 1e6/%u = %.1f Hz -> Q_alim_max = %.0f mL/min"
                "  (< Q_MAX_FISICO %.0f => el filtro de plausibilidad NUNCA dispara en alimentacion)\n",
                (unsigned)FILTRO_RUIDO_US, 1e6 / FILTRO_RUIDO_US,
                (1e6 / FILTRO_RUIDO_US) * 1000.0 / K_ALIMENTACION, Q_MAX_FISICO_MLMIN);
    std::printf("\n");
  }

  // ===========================================================================
  std::printf("T10  ASENTAMIENTO DE LA EMA (alpha=0.4, ventana 1 s) desde reposo\n");
  // ===========================================================================
  {
    Caudalimetro c(14, K_ALIMENTACION, "TEST", true);
    c.begin();
    double f = 105.14, Tu = 1e6 / f, fase = 300000000.0;
    uint64_t t_end = 300000000;
    int n1 = -1, n01 = -1, n001 = -1;
    double e1 = -1.0;
    for (int w = 1; w <= 40; w++) {
      uint64_t nxt = t_end + 1000000ULL;
      emitir(t_end, nxt, Tu, &fase);
      setAbs(nxt); t_end = nxt;
      c.actualizar(1.0f, true);
      double e = std::fabs(c.caudal_mLmin() - 680.0) / 680.0 * 100.0;
      if (w == 1) e1 = e;
      if (n1 < 0 && e < 1.0)   n1 = w;
      if (n01 < 0 && e < 0.1)  n01 = w;
      if (n001 < 0 && e < 0.01) n001 = w;
    }
    std::printf("   error tras  1 ventana: %6.3f %%   (residuo teorico de la EMA = 0.6^n)\n", e1);
    std::printf("   ventanas para err<1%%: %d s   err<0.1%%: %d s   err<0.01%%: %d s\n", n1, n01, n001);
    snprintf(b, sizeof b, "asentamiento a 0.1%% requiere %d s de regimen (tau=%.2f s)",
             n01, -1.0 / std::log(0.6));
    check(n01 > 0 && n01 <= 15, "Tiempo de asentamiento de la EMA", b);
    std::printf("\n");
  }

  // ===========================================================================
  std::printf("T11  RIZO PERISTALTICO (3 rodillos a 120 deg) — rechazo del filtro\n");
  // ===========================================================================
  {
    const double rpm = 50.0;                       // 680 mL/min de referencia
    const double f0  = 0.680 * K_ALIMENTACION;     // 105.14 Hz
    const double m   = 0.30;                       // modulacion +/-30 % de caudal instantaneo
    const double fr  = 3.0 * rpm / 60.0;           // 2.5 Hz de rizo
    Caudalimetro c(14, K_ALIMENTACION, "TEST", true);
    c.begin();
    uint64_t t = 400000000ULL; setAbs(t);
    double faseAbs = 0.0;                          // fase acumulada del tren (en ciclos)
    double vmin = 1e9, vmax = -1e9;
    for (int w = 0; w < 20; w++) {
      uint64_t fin = t + 1000000ULL;
      while (true) {
        // tiempo del proximo flanco: integral de f(t)
        double trel = (double)(t - 400000000ULL);
        // avance por paso pequeno hasta acumular 1 ciclo
        double acc = 0.0, tt = trel;
        while (acc < 1.0) {
          double step = 20e-6;
          acc += f0 * (1.0 + m * std::sin(2.0 * M_PI * fr * tt)) * step;
          tt  += step;
        }
        uint64_t nxt = t + (uint64_t)llround((tt - trel) * 1e6);
        if (nxt >= fin) break;
        setAbs(nxt); pulse();
        t = nxt;
        (void)faseAbs;
      }
      setAbs(fin); t = fin;
      c.actualizar(1.0f, true);
      if (w >= 12) { double q = c.caudal_mLmin(); if (q < vmin) vmin = q; if (q > vmax) vmax = q; }
    }
    double pk = (vmax - vmin) / 2.0, med = (vmax + vmin) / 2.0;
    snprintf(b, sizeof b, "rizo de entrada +/-%.0f%% -> salida Q=%.1f +/-%.2f mL/min (=%.2f%%)",
             m * 100.0, med, pk, pk / med * 100.0);
    check(pk / med < 0.02, "Rechazo del rizo peristaltico", b);
    std::printf("\n");
  }

  std::printf("\n   portENTER_CRITICAL/…_ISR ejecutados: %lu veces\n", g_cs_entradas);
  std::printf("\n=====================================================================\n");
  std::printf(" RESULTADO: %d chequeo(s) fallido(s)\n", g_fallos);
  std::printf("=====================================================================\n");
  return g_fallos ? 1 : 0;
}
