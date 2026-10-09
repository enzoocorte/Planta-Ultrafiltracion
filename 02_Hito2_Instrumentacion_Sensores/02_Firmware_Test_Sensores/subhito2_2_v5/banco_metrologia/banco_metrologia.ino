#include <Arduino.h>
#include "driver/gpio.h"
#include "esp_timer.h"
#include "config.h"
#include "caudalimetro.h"
#include "registro_ensayos.h"

// ==============================================================================
// BANCO DE COMPROBACIÓN METROLÓGICA (Auditoría GPT Astra - V5.1)
// - Programa aislado para verificar adquisición, filtros de ruido y registro
// - No acciona el motor (ENA deshabilitado) ni enciende radio Wi-Fi
// - Permite certificar el comportamiento del sensor y registrar CSV por Serial
// ==============================================================================

static Caudalimetro alimentacion(SENSOR_ALIM_CFG, "ALIMENTACION", true);
static Caudalimetro permeado(SENSOR_PERM_CFG, "PERMEADO", false);
static RegistroEnsayos registro;
static bool listo = false;

static void indicarResultado(const char* operacion, bool ok) {
  Serial.print(operacion);
  Serial.println(ok ? ": OK" : ": NO REALIZADO");
}

void setup() {
  // Mantener driver de motor en estado seguro de parada
  digitalWrite(PIN_ENA, HIGH);
  pinMode(PIN_ENA, OUTPUT);
  digitalWrite(PIN_PUL, LOW);
  pinMode(PIN_PUL, OUTPUT);
  digitalWrite(PIN_DIR, LOW);
  pinMode(PIN_DIR, OUTPUT);

  Serial.begin(115200);
  delay(500);

  Serial.println("\n==================================================");
  Serial.println(" BANCO DE COMPROBACIÓN METROLÓGICA (GPT Astra)   ");
  Serial.println(" No genera pasos de motor. Diagnóstico en reposo. ");
  Serial.println("==================================================");

  alimentacion.declararCalibrado(SENSOR_ALIM_CFG.calibracionDocumentada);
  permeado.declararCalibrado(SENSOR_PERM_CFG.calibracionDocumentada);

  const bool okA = alimentacion.begin();
  const bool okP = permeado.begin();
  if (!okA || !okP) {
    Serial.println("❌ Fallo inicializando sensores; motor detenido.");
    return;
  }

  // Estado inicial de diagnóstico en seco, sin totalizar volumen espurio
  alimentacion.setModoSeco(true);
  permeado.setModoSeco(true);
  listo = true;

  Serial.println("\nComandos disponibles vía Serial:");
  Serial.println("  d : Activar Modo Seco (fuera de sesión)");
  Serial.println("  h : Activar Modo Medición Húmedo (fuera de sesión)");
  Serial.println("  i : Iniciar sesión de ensayo");
  Serial.println("  f : Finalizar sesión de ensayo");
  Serial.println("  e : Exportar CSV metrológico (fuera de sesión)");
  Serial.println("  l : Limpiar registros en RAM (fuera de sesión)");
  Serial.println("  a : Limpiar alarmas de ruido seco");
  Serial.println("--------------------------------------------------\n");
}

void loop() {
  if (!listo) {
    delay(10);
    return;
  }

  constexpr bool bombaEmpuja = false;
  constexpr double rpm = 0.0;

  static uint32_t ultimaCaptura_ms = 0;
  const uint32_t ahora_ms = millis();
  if (ahora_ms - ultimaCaptura_ms >= 1000U) {
    ultimaCaptura_ms = ahora_ms;
    const auto ma = alimentacion.capturar(bombaEmpuja);
    const auto mp = permeado.capturar(bombaEmpuja);
    registro.tick(ma, mp, rpm);
  }

  if (Serial.available()) {
    const char comando = static_cast<char>(Serial.read());
    switch (comando) {
      case 'd':
      case 'h': {
        if (registro.activo()) {
          Serial.println("Finalice la sesión antes de cambiar de modo.");
          break;
        }
        const bool seco = (comando == 'd');
        alimentacion.setModoSeco(seco);
        permeado.setModoSeco(seco);
        Serial.println(seco ? "Modo seco activado." : "Modo medicion humedo activado.");
        break;
      }
      case 'i':
        indicarResultado("Inicio de sesión", registro.iniciar(alimentacion, permeado, rpm, bombaEmpuja));
        break;
      case 'f':
        indicarResultado("Fin de sesión", registro.finalizar(alimentacion, permeado, rpm, bombaEmpuja));
        break;
      case 'e':
        if (!registro.activo()) {
          registro.exportarCSV(Serial);
        } else {
          Serial.println("Finalice la sesión antes de exportar CSV.");
        }
        break;
      case 'l':
        if (!registro.activo()) {
          indicarResultado("Limpieza de registro", registro.limpiar());
        } else {
          Serial.println("Finalice la sesión antes de limpiar.");
        }
        break;
      case 'a':
        alimentacion.limpiarAlarmaSeco();
        permeado.limpiarAlarmaSeco();
        Serial.println("Alarmas de ruido seco limpiadas.");
        break;
      default:
        break;
    }
  }

  delay(1);
}
