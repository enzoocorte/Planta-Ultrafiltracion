@echo off
title Compilador Ultrarrapido ESP32 (PlatformIO)
echo ===================================================
echo   COMPILANDO FIRMWARE ESP32 CON CACHE INCREMENTAL
echo ===================================================
set "PROJECT_DIR=%~dp0..\01_Firmware_ESP32_Produccion\firmware_esp32_platformio"
"C:\Users\owenc\AppData\Local\Programs\Python\Python312\Scripts\pio.exe" run -d "%PROJECT_DIR%"
echo ===================================================
pause
