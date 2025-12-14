@echo off
REM =============================================================================
REM Script de lancement pour ConnectedVehicles avec Docker (Windows via WSL2)
REM =============================================================================

echo ========================================
echo   ConnectedVehicles - Simulation V2V
echo ========================================
echo.

REM Verifier si WSL est disponible
wsl --version >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERREUR] WSL n'est pas installe.
    echo.
    echo Pour installer WSL, ouvrez PowerShell en admin et executez:
    echo   wsl --install
    echo.
    echo Puis redemarrez votre PC.
    pause
    exit /b 1
)

echo [INFO] Lancement via WSL2...
echo.

REM Lancer le script Linux via WSL
wsl bash -c "cd $(wsl wslpath '%~dp0') && ./run.sh"

pause
