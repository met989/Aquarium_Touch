@echo off
title Aquarium OS Touch - Flasher

powershell -ExecutionPolicy Bypass -File "%~dp0scripts/flash.ps1"

echo.
pause
