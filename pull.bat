@echo off
echo %DATE% %TIME% - Eseguito %~nx0 >> "%~dp0operations.log"
echo ========================================
echo Lettura del progetto in corso...
echo ========================================

git pull

echo ========================================
echo Fatto! Progetto letto.
echo ========================================
