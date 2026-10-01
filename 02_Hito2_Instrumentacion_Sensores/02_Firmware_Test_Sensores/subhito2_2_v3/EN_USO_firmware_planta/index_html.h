#pragma once
#include <Arduino.h>

// ==============================================================================
// DECLARACIÓN DE LA INTERFAZ WEB SCADA (Flash PROGMEM)
// El contenido completo se encuentra en index_html.cpp.
// Esto permite compilación incremental en GCC/Clang: no se reprocesa el HTML
// cuando se modifica la lógica de la bomba o el firmware en el .ino.
// ==============================================================================

extern const char INDEX_HTML[] PROGMEM;
