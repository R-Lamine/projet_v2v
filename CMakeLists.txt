# ===============================
#  Configuration minimale requise
# ===============================
cmake_minimum_required(VERSION 3.16)

# ===============================
#  Nom du projet et langage utilisé
# ===============================
project(ConnectedVehicles LANGUAGES CXX)

# ===============================
#  Version du standard C++
# ===============================
# C++17 est requis pour Boost, libosmium et les fonctionnalités modernes
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# For QWidget
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ===============================
#  Inclusion des fichiers d'en-tête
# ===============================
# Indique à CMake que les fichiers .h se trouvent dans le dossier include/
include_directories(include)

# ===============================
#  Recherche des bibliothèques nécessaires
# ===============================
# 1. Boost : gestion du graphe (Boost Graph Library)
# 2. PROJ : calculs de distance géographique
# 3. libosmium : lecture de fichiers OpenStreetMap

find_package(Boost REQUIRED)

# ===============================
#  Qt for GUI Visualization
# ===============================
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)


# PROJ (lib de projection géographique)
include_directories(/usr/include)
link_libraries(proj)

# libosmium
include_directories(/usr/include/osmium)


# ===============================
#  Récupération automatique des fichiers source
# ===============================
# CMake va compiler tous les fichiers source présents dans src/
file(GLOB SOURCES src/*.cpp)

# ===============================
#  Création de l’exécutable
# ===============================
# Le binaire s’appellera ConnectedVehicles
add_executable(ConnectedVehicles ${SOURCES})


########## debug Widgets
qt6_wrap_cpp(MOC_SOURCES include/SimulatorView.h)
target_sources(ConnectedVehicles PRIVATE ${MOC_SOURCES})

# ===============================
#  Liaison avec les bibliothèques externes
# ===============================
# On lie le projet aux bibliothèques trouvées (Boost, PROJ)
target_link_libraries(ConnectedVehicles
    ${Boost_LIBRARIES}
    proj
    bz2
    z
    expat
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
)


# ===============================
#  Informations de débogage (optionnel)
# ===============================
message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
message(STATUS "PROJ library: ${PROJ_LIBRARIES}")
