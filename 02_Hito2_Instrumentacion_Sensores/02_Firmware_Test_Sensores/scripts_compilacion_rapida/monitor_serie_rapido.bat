@echo off
title Monitor Serie ESP32 (115200 bps)
echo ===================================================
echo   INICIANDO MONITOR SERIE A 115200 BAUDIOS
echo ===================================================
"C:\Users\owenc\AppData\Local\Programs\Python\Python312\Scripts\pio.exe" device monitor -b 115200
pause
