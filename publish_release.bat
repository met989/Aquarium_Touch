@echo off
echo %DATE% %TIME% - Eseguito %~nx0 >> "%~dp0operations.log"
echo ========================================
echo Avvio procedura di Release su GitHub
echo ========================================
echo.

python "%~dp0scripts\publish_release.py"

echo.