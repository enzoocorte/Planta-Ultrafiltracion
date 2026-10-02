// =============================================================================
// SHIM Arduino/ESP32 minimo para ejecutar caudalimetro.cpp en host (x86-64).
// NO reimplementa logica del firmware: solo provee los simbolos del API que
// caudalimetro.cpp/.h consumen. La logica auditada es la del archivo original.
// =============================================================================
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cmath>

// ---- Reloj virtual controlado por el arnes de prueba -----------------------
extern uint32_t g_micros;          // reloj virtual (envuelve a 2^32 como el real)
extern uint64_t g_micros_abs;      // mismo reloj, monotono absoluto (solo debug)

inline uint32_t micros() { return g_micros; }

// ---- GPIO / interrupciones --------------------------------------------------
#define INPUT_PULLUP 2
#define FALLING      2
#define HIGH         1
#define LOW          0

typedef void (*voidFuncPtr)(void);
typedef void (*voidInterruptPtr)(void*);

// Registro de la ISR instalada por attachInterruptArg (lo usa el arnes)
extern voidInterruptPtr g_isr_fn;
extern void*            g_isr_arg;

inline void pinMode(uint8_t, uint8_t) {}
inline int  digitalPinToInterrupt(uint8_t p) { return p; }
inline void attachInterruptArg(int, voidInterruptPtr fn, void* arg, int) {
  g_isr_fn  = fn;
  g_isr_arg = arg;
}

// ---- Seccion critica -------------------------------------------------------
// En simulacion mono-hilo la ISR se invoca de forma sincrona desde el arnes,
// por lo que la exclusion mutua es exacta con macros vacias. Se cuenta el
// numero de entradas para poder afirmar que el codigo SI las ejecuta.
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
extern unsigned long g_cs_entradas;
#define portENTER_CRITICAL(m)     do { g_cs_entradas++; } while (0)
#define portEXIT_CRITICAL(m)      do { } while (0)
#define portENTER_CRITICAL_ISR(m) do { g_cs_entradas++; } while (0)
#define portEXIT_CRITICAL_ISR(m)  do { } while (0)

#define IRAM_ATTR

// ---- Serial -----------------------------------------------------------------
struct SerialStub {
  void begin(unsigned long) {}
  void println(const char* s) { std::printf("%s\n", s); }
  void printf(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap);
  }
};
extern SerialStub Serial;
