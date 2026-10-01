@echo off
title Subir Firmware a ESP32 (PlatformIO Upload)
echo ===================================================
echo   FLASHEANDO FIRMWARE AL ESP32 POR PUERTO SERIE COM
echo ===================================================
set "PROJECT_DIR=%~dp0..\01_Firmware_ESP32_Produccion\firmware_esp32_platformio"
"C:\Users\owenc\AppData\Local\Programs\Python\Python312\Scripts\pio.exe" run -d "%PROJECT_DIR%" -t upload
echo ===================================================
pause
