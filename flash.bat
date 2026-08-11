@echo off
title Aquarium OS Touch - Flasher

powershell -ExecutionPolicy Bypass -File "%~dp0flash_tool.ps1"

echo.
pause
