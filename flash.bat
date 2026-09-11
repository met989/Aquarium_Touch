@echo off
echo %DATE% %TIME% - Eseguito %~nx0 >> "%~dp0operations.log"
title Aquarium OS Touch - Flasher

python "%~dp0scripts\flash.py"

echo.
pause
