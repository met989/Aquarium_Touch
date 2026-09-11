@echo off
echo %DATE% %TIME% - Eseguito %~nx0 >> "%~dp0operations.log"
title Aquarium OS Touch - Builder
echo ========================================
echo Compilazione del progetto e creazione del file .bin...
echo ========================================
echo.

"%USERPROFILE%\.platformio\penv\Scripts\pio.exe" run

echo.
echo ========================================
echo Operazione completata. Se la compilazione ha avuto successo,
echo il nuovo firmware e' stato salvato nella cartella release/.
echo ========================================