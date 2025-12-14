@echo off
REM =============================================================================
REM Script de lancement pour ConnectedVehicles avec Docker (Windows)
REM =============================================================================

echo  ConnectedVehicles - Simulation V2V
echo ======================================

REM Vérifier si Docker est en cours d'exécution
docker info >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo  Docker n'est pas en cours d'exécution. Lancez Docker Desktop.
    pause
    exit /b 1
)

REM Lancer l'application
echo  Lancement de l'application...
docker compose up --build

pause
