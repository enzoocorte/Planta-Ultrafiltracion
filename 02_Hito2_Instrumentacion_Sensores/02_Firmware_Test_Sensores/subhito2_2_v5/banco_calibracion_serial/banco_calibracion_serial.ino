#include <Arduino.h>
#include <cmath>
#include <esp_arduino_version.h>

#include "config.h"
#include "caudalimetro.h"
#include "registro_ensayos.h"

#if ESP_ARDUINO_VERSION_MAJOR != 3
#error "Esta variante requiere Arduino-ESP32 3.x"
#endif

// Cambiar a true despues de la verificacion electrica documentada del driver DM860.
constexpr bool INTERFAZ_DRIVER_VERIFICADA = true;

// Deben coincidir con la interfaz y el driver reales (catodo comun: ENA HIGH = inhibido)
constexpr uint8_t NIVEL_INHIBIR = HIGH;
constexpr uint8_t NIVEL_HABILITAR = LOW;

constexpr float RPM_PRUEBA = 25.0f;
constexpr uint32_t ESPERA_HABILITACION_MS = 250;
constexpr uint32_t PERIODO_CONTROL_MS = 20;
constexpr uint32_t MAX_RETRASO_CONTROL_MS = 250;
constexpr uint32_t PERIODO_MUESTRA_MS = 1000;

static_assert(RPM_PRUEBA >= RPM_MIN &&
              RPM_PRUEBA <= RPM_MAX,
              "RPM_PRUEBA fuera de rango");

static_assert(PULSOS_POR_REV == 3200,
              "Revisar el control para otros micropasos");

Caudalimetro alim(SENSOR_ALIM_CFG, "ALIMENTACION", true);
Caudalimetro perm(SENSOR_PERM_CFG, "PERMEADO", false);
RegistroEnsayos registro;

bool listo = false;
bool fallo = false;
bool pwmAdjunto = false;
bool marchaSolicitada = false;
bool esperandoHabilitacion = false;

float rpmRampa = 0.0f;
uint32_t frecuenciaStep = 0;

uint32_t tControl = 0;
uint32_t tMuestra = 0;
uint32_t tHabilitacion = 0;

// Es la velocidad equivalente de la frecuencia STEP configurada.
// NO es una medicion del eje.
float rpmStep() {
  return 60.0f * static_cast<float>(frecuenciaStep) /
         static_cast<float>(PULSOS_POR_REV);
}

bool motorParado() {
  return !marchaSolicitada &&
         !esperandoHabilitacion &&
         rpmRampa == 0.0f &&
         frecuenciaStep == 0;
}

void cerrarVentana() {
  if (!registro.activo()) return;

  const bool impulsa = frecuenciaStep != 0;
  if (!registro.finalizar(alim, perm, rpmStep(), impulsa)) {
    registro.marcarFalloControl();
    fallo = true;
    digitalWrite(PIN_ENA, NIVEL_INHIBIR);
  }
}

void fallar(const char* causa) {
  // Solicitar inhibicion antes de cualquier salida por Serial.
  digitalWrite(PIN_ENA, NIVEL_INHIBIR);

  fallo = true;
  marchaSolicitada = false;
  esperandoHabilitacion = false;
  rpmRampa = 0.0f;

  if (pwmAdjunto) {
    // Si falla, no se puede garantizar por software el corte STEP.
    // La inhibicion y la parada fisica siguen siendo necesarias.
    (void)ledcWrite(PIN_PUL, 0);
  }

  frecuenciaStep = 0;

  if (registro.activo()) {
    registro.marcarFalloControl();
    cerrarVentana();
  }

  Serial.print("# FALLO_ENCLAVADO: ");
  Serial.println(causa);
}

bool aplicarRPM(float rpm) {
  if (rpm < 1.0f) {
    if (!ledcWrite(PIN_PUL, 0)) {
      fallar("No se pudo poner STEP a duty cero");
      return false;
    }

    frecuenciaStep = 0;
    return true;
  }

  const uint32_t pedida = static_cast<uint32_t>(
      lroundf(rpm * PULSOS_POR_REV / 60.0f));

  if (pedida != frecuenciaStep) {
    const uint32_t obtenida =
        ledcChangeFrequency(PIN_PUL, pedida, 10);

    if (obtenida == 0) {
      fallar("Fallo al configurar frecuencia STEP");
      return false;
    }

    frecuenciaStep = obtenida;
  }

  if (!ledcWrite(PIN_PUL, 512)) {
    fallar("Fallo al configurar duty STEP");
    return false;
  }

  return true;
}

void iniciarMotor() {
  if (!listo || fallo || !motorParado() || registro.activo()) {
    Serial.println("# START rechazado: estado no disponible");
    return;
  }

  if (!INTERFAZ_DRIVER_VERIFICADA) {
    Serial.println("# START bloqueado: verificar interfaz driver");
    return;
  }

  if (!ledcWrite(PIN_PUL, 0)) {
    fallar("No se pudo asegurar STEP en reposo");
    return;
  }

  // Direccion fija; no existe inversion en esta variante.
  digitalWrite(PIN_DIR, LOW);
  digitalWrite(PIN_ENA, NIVEL_HABILITAR);

  marchaSolicitada = true;
  esperandoHabilitacion = true;
  tHabilitacion = millis();
  tControl = tHabilitacion;

  Serial.println("# Motor: arranque solicitado a 25 RPM");
}

void solicitarParada() {
  marchaSolicitada = false;

  if (esperandoHabilitacion) {
    esperandoHabilitacion = false;
  }

  Serial.println("# Motor: parada solicitada");
}

void actualizarMotor(uint32_t ahora) {
  const uint32_t transcurrido = ahora - tControl;
  if (transcurrido < PERIODO_CONTROL_MS) return;

  tControl = ahora;

  if (fallo || !listo) return;

  const bool ocupado =
      marchaSolicitada || esperandoHabilitacion ||
      frecuenciaStep != 0;

  if (ocupado && transcurrido > MAX_RETRASO_CONTROL_MS) {
    fallar("Retraso excesivo del control");
    return;
  }

  if (esperandoHabilitacion) {
    if (ahora - tHabilitacion < ESPERA_HABILITACION_MS) {
      return;
    }

    esperandoHabilitacion = false;
  }

  const float dt = transcurrido / 1000.0f;

  if (marchaSolicitada) {
    if (rpmRampa < 1.0f) rpmRampa = 1.0f;

    rpmRampa = fminf(
        RPM_PRUEBA,
        rpmRampa + ACEL_ARRANQUE_RPM_S * dt);
  } else {
    rpmRampa = fmaxf(
        0.0f,
        rpmRampa - FRENADO_PARADA_RPM_S * dt);

    if (rpmRampa < 1.0f) rpmRampa = 0.0f;
  }

  if (!aplicarRPM(rpmRampa)) return;

  if (!marchaSolicitada && frecuenciaStep == 0) {
    digitalWrite(PIN_ENA, NIVEL_INHIBIR);

    // Si el operador no cerro antes con F, FIN queda definido
    // por el fin de STEP. No incluye todo posible flujo residual.
    cerrarVentana();
  }
}

void iniciarVentana() {
  if (fallo || !listo || registro.activo() ||
      !marchaSolicitada || esperandoHabilitacion ||
      fabsf(rpmRampa - RPM_PRUEBA) > 0.01f ||
      frecuenciaStep == 0) {
    Serial.println("# INICIO rechazado: esperar regimen comandado estabilizado");
    return;
  }

  if (!registro.iniciar(alim, perm, rpmStep(), true)) {
    fallar("No se pudo iniciar el registro");
    return;
  }

  Serial.println("# Ventana INICIO registrada (Cronometro y probeta en marcha)");
}

void exportar() {
  if (!motorParado() || registro.activo()) {
    Serial.println("# Exportacion rechazada: detener primero el motor");
    return;
  }

  Serial.println("# variante=banco_reducido");
  Serial.println("# rpm=equivalente_STEP_no_medicion_eje");
  Serial.println("# FALLO_CONTROL=bit_2_del_campo_estado");
  Serial.println("# FIN_por_F_o_fin_STEP;ver_protocolo_de_recogida");

  if (!registro.exportarCSV(Serial)) {
    Serial.println("# ERROR: exportacion no completada");
  }
}

void atenderComando(char c) {
  switch (c) {
    case 'S':
    case 's':
      iniciarMotor();
      break;

    case 'I':
    case 'i':
      iniciarVentana();
      break;

    case 'F':
    case 'f':
      if (!registro.activo()) {
        Serial.println("# No existe ventana activa");
      } else {
        cerrarVentana();
        Serial.println("# Ventana FIN procesada (Retirar probeta / detener cronometro)");
      }
      break;

    case 'P':
    case 'p':
      solicitarParada();
      break;

    case 'X':
    case 'x':
      fallar("Parada de software solicitada");
      break;

    case 'E':
    case 'e':
      exportar();
      break;

    case 'C':
    case 'c':
      if (!motorParado() || registro.activo()) {
        Serial.println("# Limpieza rechazada: motor o ventana activos");
      } else if (registro.limpiar()) {
        Serial.println("# Registro limpio");
      }
      break;

    case '\r':
    case '\n':
    case ' ':
      break;

    default:
      Serial.println("# Comandos: S (Start) | I (Inicio probeta) | F (Fin probeta) | P (Parar) | X (Emergencia) | E (Exportar CSV) | C (Limpiar)");
      break;
  }
}

void setup() {
  // Estado electrico seguro durante reset/arranque
  digitalWrite(PIN_ENA, NIVEL_INHIBIR);
  pinMode(PIN_ENA, OUTPUT);

  digitalWrite(PIN_PUL, LOW);
  pinMode(PIN_PUL, OUTPUT);

  digitalWrite(PIN_DIR, LOW);
  pinMode(PIN_DIR, OUTPUT);

  Serial.begin(115200);

  // K iniciales son provisionales para esta campana
  const bool ka = alim.setK(K_ALIMENTACION);
  const bool kp = perm.setK(K_PERMEADO);

  const bool okAlim = alim.begin();
  const bool okPerm = perm.begin();

  pwmAdjunto = ledcAttach(PIN_PUL, 800, 10);
  const bool pwmReposo =
      pwmAdjunto && ledcWrite(PIN_PUL, 0);

  listo = ka && kp && okAlim && okPerm &&
          registro.disponible() && pwmReposo;

  tControl = millis();
  tMuestra = tControl;

  if (!listo) {
    fallar("Inicializacion incompleta");
    return;
  }

  Serial.println("\n# ==========================================================");
  Serial.println("# BANCO DE CALIBRACION METROLOGICA V5.2 — SERIAL MONITOR");
  Serial.println("# Planta Piloto Ultrafiltracion - UNSa 2026");
  Serial.println("# ==========================================================");
  Serial.println("# Baudios: 115200 | Consigna fija: 25.0 RPM");
  Serial.println("# S: arrancar motor a 25 RPM (estabilizar)");
  Serial.println("# I: inicio recogida probeta (tara de ventana)");
  Serial.println("# F: fin recogida probeta (cierra ventana metrologica)");
  Serial.println("# P: parar rampa motor | X: parada de emergencia");
  Serial.println("# E: exportar CSV metrologico | C: limpiar buffer");
  Serial.println("# ==========================================================");
}

void loop() {
  uint32_t ahora = millis();
  actualizarMotor(ahora);

  // Un caracter por vuelta: sin readString ni esperas de linea.
  if (Serial.available() > 0) {
    atenderComando(static_cast<char>(Serial.read()));
  }

  ahora = millis();

  if (ahora - tMuestra >= PERIODO_MUESTRA_MS) {
    tMuestra = ahora;

    const bool impulsa = frecuenciaStep != 0;
    const auto ma = alim.capturar(impulsa);
    const auto mp = perm.capturar(impulsa);

    registro.tick(ma, mp, rpmStep());

    if (registro.activo() && registro.incompleto()) {
      fallar("Registro incompleto: revisar capacidad o temporizacion");
    }
  }
}
