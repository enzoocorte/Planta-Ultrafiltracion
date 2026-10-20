# REGLAS LOCALES: ESPEJO DEL REPOSITORIO `Planta-Ultrafiltracion` (Hitos 0 a 5)

## 1. Propósito de esta Carpeta
Espejo local del repositorio GitHub de Enzo Corte (`enzoocorte/Planta-Ultrafiltracion`), estructurado por hitos:
- `00_General_y_P_ID_Planta/`: P&ID interactivo y montaje de tanques.
- `01_Hito1_Control_Accionamiento_NEMA34_DM860/`: Bomba MBP-2000, NEMA 34 y driver DM860.
- `02_Hito2_Instrumentacion_Sensores/`: Caudalímetros, calibración y datos `.csv`.
- `03_Hito3_Reactor_Sedimentador_Agitador/`: Protocolo Jar Test con penca de tuna (*Opuntia ficus-indica*) y control L298N.
- `04_Hito4_Integracion_Automatizacion_IoT/`: Enclavamientos y SCADA.
- `05_Hito5_Ensayos_Membrana_VidaUtil/`: Ensayos de Ley de Darcy, fouling y vida útil del dializador FX100.

## 2. Regla de Oro de Ahorro de Tokens
- **PROHIBIDO** leer la carpeta `Archivado/` o `Capitulo_4_Programacion_Firmware/03_Historial_Versiones_Arduino/`. Contienen versiones obsoletas (V1–V3) que consumen miles de tokens y confunden el contexto.
- Para verificar novedades recientes de Enzo en GitHub, consulta primero `get_file_contents` del MCP `github` sobre `02_Hito2_Instrumentacion_Sensores/02_Firmware_Test_Sensores/subhito2_2_v6/`.
