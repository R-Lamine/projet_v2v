#!/bin/bash
# =============================================================================
# Script de lancement pour ConnectedVehicles avec Docker
# =============================================================================

echo " ConnectedVehicles - Simulation V2V"
echo "======================================"

# Vérifier si Docker est installé
if ! command -v docker &> /dev/null; then
    echo " Docker n'est pas installé. Installez-le depuis https://docs.docker.com/get-docker/"
    exit 1
fi

# Vérifier si docker-compose est disponible
if ! command -v docker-compose &> /dev/null && ! docker compose version &> /dev/null; then
    echo " Docker Compose n'est pas installé."
    exit 1
fi

# Autoriser l'accès au serveur X11 pour Docker
echo " Configuration de l'affichage X11..."
xhost +local:docker 2>/dev/null || echo "  xhost non disponible, l'affichage pourrait ne pas fonctionner"

# Lancer l'application avec docker-compose
echo " Lancement de l'application..."
if docker compose version &> /dev/null; then
    docker compose up --build
else
    docker-compose up --build
fi

# Révoquer l'accès X11 après fermeture
xhost -local:docker 2>/dev/null
