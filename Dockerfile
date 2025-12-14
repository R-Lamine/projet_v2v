# =============================================================================
# Dockerfile pour ConnectedVehicles - Simulation V2V
# =============================================================================

FROM ubuntu:22.04

# Éviter les prompts interactifs pendant l'installation
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Paris

# Installer les dépendances système
RUN apt-get update && apt-get install -y \
    # Compilation
    build-essential \
    cmake \
    ninja-build \
    # Qt5
    qtbase5-dev \
    qtdeclarative5-dev \
    libqt5svg5-dev \
    libqt5concurrent5 \
    # Boost
    libboost-all-dev \
    # Libosmium et ses dépendances
    libosmium2-dev \
    libprotozero-dev \
    libbz2-dev \
    zlib1g-dev \
    libexpat1-dev \
    # PROJ pour les projections géographiques
    libproj-dev \
    proj-bin \
    # X11 pour l'affichage graphique
    libxcb-xinerama0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-shape0 \
    libxcb-xfixes0 \
    libxkbcommon-x11-0 \
    libgl1-mesa-glx \
    libgl1-mesa-dri \
    libegl1-mesa \
    x11-apps \
    && rm -rf /var/lib/apt/lists/*

# Créer le répertoire de travail
WORKDIR /app

# Copier les fichiers du projet
COPY . /app

# Compiler le projet
RUN mkdir -p build && cd build && \
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release .. && \
    ninja

# Créer un lien symbolique pour l'exécutable
RUN ln -sf /app/build/ConnectedVehicles /usr/local/bin/ConnectedVehicles

# Définir le répertoire de travail pour l'exécution
WORKDIR /app/build

# Point d'entrée
CMD ["./ConnectedVehicles"]
