@echo off
echo ========================================
echo Salvataggio del progetto in corso...
echo ========================================

git add .
git commit -m "Salvataggio del %DATE% %TIME%"
git push

echo ========================================
echo Fatto! Progetto salvato con successo.
echo ========================================
