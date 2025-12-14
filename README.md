# V2V Communication Simulator

## Description

Ce projet implémente un simulateur de communication Vehicle-to-Vehicle (V2V) sur une carte OpenStreetMap. L'application permet de visualiser en temps réel le déplacement de véhicules sur un réseau routier et d'analyser leurs capacités de communication sans fil basées sur la portée de transmission.

Le simulateur utilise des données cartographiques réelles (format OSM PBF) pour construire un graphe routier sur lequel les véhicules se déplacent de manière autonome. Un graphe d'interférence modélise les connexions possibles entre véhicules, avec support de la fermeture transitive pour représenter les communications multi-sauts.

---

## Lancement rapide avec Docker

**Prérequis** : [Docker](https://docs.docker.com/get-docker/) installé sur votre machine.

### Sur Linux (3 commandes)
```bash
git clone https://github.com/R-Lamine/projet_v2v.git
cd projet_v2v
./run.sh
```

### Sur Windows avec WSL2 (3 commandes)

**Prérequis** : WSL2 installé (Windows 10/11). Si ce n'est pas le cas, ouvrez PowerShell en admin et exécutez `wsl --install`, puis redémarrez.

```powershell
git clone https://github.com/R-Lamine/projet_v2v.git
cd projet_v2v
.\run-windows.bat
```

> **Note** : Sur Windows 11, WSLg affiche automatiquement les applications graphiques Linux. Sur Windows 10, il faut installer Docker Desktop avec le backend WSL2.

### Alternative : lancement manuel
```bash
# 1. Cloner le projet
git clone https://github.com/R-Lamine/projet_v2v.git && cd projet_v2v

# 2. Autoriser l'affichage X11 (Linux uniquement)
xhost +local:docker

# 3. Lancer avec Docker Compose
docker compose up --build
```

---

## Table des matières

1. [Fonctionnalités](#fonctionnalités)
2. [Architecture du projet](#architecture-du-projet)
3. [Dépendances](#dépendances)
4. [Compilation](#compilation)
5. [Utilisation](#utilisation)
6. [Structure des fichiers](#structure-des-fichiers)
7. [Description des modules](#description-des-modules)
8. [Algorithmes implémentés](#algorithmes-implémentés)
9. [Interface utilisateur](#interface-utilisateur)
10. [Contrôles clavier](#contrôles-clavier)

---

## Fonctionnalités

- Chargement et parsing de fichiers OpenStreetMap (format PBF)
- Construction automatique d'un graphe routier à partir des données OSM
- Simulation de 1 à 3000 véhicules en temps réel
- Graphe d'interférence pour modéliser les communications V2V
- Fermeture transitive pour les communications multi-sauts
- Optimisation spatiale par grille hiérarchique (algorithme K-means)
- Visualisation cartographique avec tuiles XYZ (thèmes sombre et clair)
- Interface utilisateur moderne avec panneaux de contrôle
- Statistiques en temps réel (connexions, comparaisons, performances)

---

## Architecture du projet

```
projet_v2v/
├── include/                    # Fichiers d'en-tête (.h)
│   ├── graph_types.h          # Définitions des types Boost Graph
│   ├── osm_reader.h           # Lecteur de fichiers OSM
│   ├── graph_builder.h        # Constructeur du graphe routier
│   ├── vehicule.h             # Classe Véhicule
│   ├── simulator.h            # Moteur de simulation
│   ├── interference_graph.h   # Graphe d'interférence V2V
│   ├── spatial_grid.h         # Grille spatiale optimisée
│   ├── map_view.h             # Widget de visualisation carte
│   ├── overlay_ui.h           # Interface utilisateur overlay
│   └── vehicle_renderer.h     # Rendu SVG des véhicules
├── src/                        # Fichiers sources (.cpp)
│   ├── main.cpp               # Point d'entrée de l'application
│   ├── osm_reader.cpp         # Implémentation du lecteur OSM
│   ├── graph_builder.cpp      # Implémentation du constructeur
│   ├── vehicule.cpp           # Logique de déplacement véhicule
│   ├── simulator.cpp          # Boucle de simulation
│   ├── interference_graph.cpp # Calcul des interférences
│   ├── spatial_grid.cpp       # Algorithme K-means et grille
│   ├── map_view.cpp           # Rendu de la carte et véhicules
│   ├── overlay_ui.cpp         # Composants UI Qt
│   └── vehicle_renderer.cpp   # Rendu vectoriel véhicules
├── data/                       # Données
│   └── strasbourg.osm.pbf     # Carte OpenStreetMap
├── CMakeLists.txt             # Configuration CMake
└── README.md                  # Ce fichier
```

---

## Dépendances

### Bibliothèques requises

| Bibliothèque | Version | Usage |
|--------------|---------|-------|
| Qt5 ou Qt6 | 5.15+ / 6.x | Interface graphique, réseau, SVG |
| Boost | 1.71+ | Structures de graphe (adjacency_list) |
| libosmium | 2.15+ | Parsing des fichiers OSM PBF |
| PROJ | 6.0+ | Projections géographiques |
| zlib | - | Compression (dépendance osmium) |
| bz2 | - | Compression (dépendance osmium) |
| expat | - | Parsing XML (dépendance osmium) |

### Installation des dépendances (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y \
    build-essential cmake \
    qtbase5-dev qttools5-dev libqt5svg5-dev \
    libboost-all-dev \
    libosmium2-dev \
    libproj-dev \
    zlib1g-dev libbz2-dev libexpat1-dev
```

---

## Compilation

### Configuration et compilation

```bash
# Créer le répertoire de build
mkdir -p build && cd build

# Configurer avec CMake
cmake ..

# Compiler
make -j$(nproc)

# Ou avec Ninja (plus rapide)
cmake -G Ninja ..
ninja
```

### Exécution

```bash
./ConnectedVehicles
```

Note : L'exécutable doit pouvoir accéder au fichier `data/strasbourg.osm.pbf` (chemin relatif `../../data/` depuis le répertoire de build).

---

## Utilisation

Au lancement, l'application :

1. Charge les données OSM de Strasbourg
2. Construit le graphe routier (sommets = intersections, arêtes = routes)
3. Génère 2000 véhicules positionnés aléatoirement
4. Démarre la simulation à 20 FPS

L'interface permet de :
- Naviguer sur la carte (glisser-déposer, molette pour zoomer)
- Ajuster le nombre de véhicules (1 à 3000)
- Configurer les antennes pour l'optimisation spatiale
- Visualiser les connexions entre véhicules
- Afficher les rayons de transmission
- Activer/désactiver les connexions transitives

---

## Structure des fichiers

### Fichiers d'en-tête (include/)

#### graph_types.h
Définit les types fondamentaux du graphe routier basé sur Boost Graph Library :
- `VertexData` : données d'un sommet (id OSM, latitude, longitude)
- `EdgeData` : données d'une arête (distance, sens unique, type de route)
- `RoadGraph` : alias pour `boost::adjacency_list` configuré pour le réseau routier

#### osm_reader.h
Classe `OSMReader` pour parser les fichiers OSM PBF :
- Structures `OSMNode` et `OSMWay` pour stocker les données brutes
- Méthode `read()` utilisant libosmium pour le parsing

#### graph_builder.h
Classe `GraphBuilder` pour construire le graphe Boost :
- Conversion des données OSM vers le graphe
- Calcul des distances géodésiques (formule de Haversine)
- Gestion des routes à sens unique

#### vehicule.h
Classe `Vehicule` représentant un véhicule dans la simulation :
- Position sur une arête du graphe
- Navigation vers un objectif
- Gestion de la portée de transmission
- Calcul de la direction (heading) pour le rendu

#### simulator.h
Classe `Simulator` (QObject) orchestrant la simulation :
- Gestion du cycle de vie (start, pause, stop)
- Mise à jour des véhicules à chaque tick
- Reconstruction du graphe d'interférence
- Gestion dynamique du nombre de véhicules

#### interference_graph.h
Classe `InterferenceGraph` modélisant les communications V2V :
- Liste d'adjacence pour les connexions directes
- Fermeture transitive par BFS
- Optimisation par grille spatiale

#### spatial_grid.h
Classe `SpatialGrid` pour l'optimisation des calculs de distance :
- Structures `MacroAntenna` et `MicroAntenna`
- Algorithme K-means pour le placement des antennes
- Réduction de la complexité de O(n²) à O(n)

#### map_view.h
Classe `MapView` (QWidget) pour le rendu cartographique :
- Affichage de tuiles XYZ (OpenStreetMap, CartoDB)
- Cache de tuiles par thème
- Rendu des véhicules et connexions
- Gestion des interactions utilisateur

#### overlay_ui.h
Classes d'interface utilisateur :
- `TopBar` : barre supérieure avec contrôles principaux
- `ParametersPanel` : sliders et toggles de configuration
- `StatsPanel` : statistiques en temps réel
- `BottomMenu` : conteneur des panneaux avec animation
- `ZoomControls` : boutons de zoom
- `UIOverlay` : orchestrateur de l'interface

---

## Description des modules

### Module OSM (osm_reader, graph_builder)

Le module OSM est responsable du chargement des données cartographiques :

1. **OSMReader** parse le fichier PBF en utilisant libosmium
   - Extrait les noeuds (coordonnées GPS)
   - Extrait les ways (séquences de noeuds formant des routes)
   - Filtre par type de route (highway=*)

2. **GraphBuilder** construit le graphe routier
   - Crée un sommet Boost pour chaque noeud référencé
   - Crée une arête pour chaque segment de route
   - Calcule la distance en mètres via la formule de Haversine

### Module Simulation (simulator, vehicule)

Le moteur de simulation gère le déplacement des véhicules :

1. **Simulator** utilise un QTimer pour les ticks réguliers (50ms)
   - Appelle `update()` sur chaque véhicule
   - Reconstruit le graphe d'interférence
   - Émet un signal `ticked()` pour le rendu

2. **Vehicule** se déplace le long des arêtes du graphe
   - Interpolation linéaire sur l'arête courante
   - Sélection aléatoire de la prochaine arête à l'intersection
   - Inversion de direction quand l'objectif est atteint

### Module Communication (interference_graph, spatial_grid)

Le graphe d'interférence modélise les capacités de communication :

1. **Construction du graphe** (à chaque tick)
   - Deux véhicules sont voisins si leur distance < portée de transmission
   - Sans optimisation : O(n²) comparaisons
   - Avec grille spatiale : O(n) comparaisons moyennes

2. **Fermeture transitive** (optionnelle)
   - BFS depuis chaque véhicule pour trouver tous les véhicules atteignables
   - Permet de modéliser les communications multi-sauts

3. **Grille spatiale hiérarchique**
   - Niveau 1 : Macro-antennes placées par K-means
   - Niveau 2 : Micro-antennes subdivisées dans chaque macro-zone
   - Seuls les véhicules dans les zones voisines sont comparés

### Module Rendu (map_view, vehicle_renderer, overlay_ui)

Le rendu utilise le système de peinture Qt :

1. **MapView** dessine la carte et les entités
   - Tuiles XYZ chargées depuis le réseau ou le cache
   - Projection Web Mercator (EPSG:3857)
   - Rendu adaptatif selon le niveau de zoom

2. **VehicleRenderer** dessine les véhicules
   - Mode SVG pour zoom >= 13 (haute qualité)
   - Mode points pour zoom < 13 (performance)
   - Coloration cohérente basée sur l'ID

3. **UIOverlay** superpose l'interface utilisateur
   - Fond semi-transparent avec flou
   - Animation fluide des panneaux

---

## Algorithmes implémentés

### Algorithme K-means pour le placement des antennes

```
Entrée: Liste de véhicules V, nombre d'antennes K
Sortie: Positions optimales des K antennes

1. Initialiser K centres aléatoirement parmi les positions des véhicules
2. Répéter jusqu'à convergence (max 50 itérations):
   a. Assigner chaque véhicule au centre le plus proche
   b. Recalculer chaque centre comme barycentre de ses véhicules
   c. Si aucun centre n'a bougé, terminer
3. Retourner les K centres comme positions d'antennes
```

Complexité : O(n * K * iterations)

### Algorithme BFS pour la fermeture transitive

```
Entrée: Graphe d'adjacence G, sommet source s
Sortie: Ensemble des sommets atteignables depuis s

1. Créer une file Q et un ensemble Visités
2. Enfiler s, marquer s comme visité
3. Tant que Q n'est pas vide:
   a. Défiler un sommet u
   b. Pour chaque voisin v de u:
      - Si v non visité: marquer visité, enfiler v
4. Retourner Visités
```

Complexité : O(V + E) par sommet source

### Optimisation par grille spatiale

```
Entrée: Véhicule v, grille G
Sortie: Véhicules candidats pour la comparaison de distance

1. Trouver la micro-antenne M contenant v
2. Obtenir les micro-antennes voisines de M
3. Collecter tous les véhicules de M et ses voisines
4. Retourner cette liste (au lieu de tous les véhicules)
```

Réduction : de O(n) à O(n/k) comparaisons en moyenne, où k est le nombre de micro-antennes.

---

## Interface utilisateur

### TopBar (Barre supérieure)

| Élément | Description |
|---------|-------------|
| Logo + Titre | Identifiant de l'application |
| Badge de statut | Indique si la simulation est en cours ou en pause |
| Informations carte | Niveau de zoom et coordonnées du centre |
| Bouton Thème | Bascule entre thème sombre et clair |
| Bouton Qualité | Bascule entre mode rapide et haute qualité |
| Bouton Pause/Play | Contrôle de la simulation |

### ParametersPanel (Panneau de paramètres)

| Paramètre | Plage | Description |
|-----------|-------|-------------|
| Nombre de véhicules | 1 - 3000 | Ajuste dynamiquement la flotte |
| Grandes antennes | 0 - 50 | Macro-cellules pour K-means |
| Petites antennes | 0 - 200 | Micro-cellules par macro |
| Rayon de transmission | 10 - 1000 m | Portée de communication |
| Afficher connexions | On/Off | Lignes entre véhicules connectés |
| Afficher rayons | On/Off | Cercles de portée de transmission |
| Connexions transitives | On/Off | Active le calcul multi-sauts |

### StatsPanel (Panneau de statistiques)

| Statistique | Description |
|-------------|-------------|
| Véhicules actifs | Nombre total de véhicules |
| Véhicules connectés | Véhicules ayant au moins un voisin |
| Connexions totales | Nombre d'arêtes dans le graphe |
| Taux de connexion | Pourcentage de véhicules connectés |
| Comparaisons/tick | Nombre de calculs de distance par tick |
| Moy. voisins/véhicule | Degré moyen du graphe |
| Temps de calcul | Durée de construction du graphe |

---

## Contrôles clavier

| Touche | Action |
|--------|--------|
| Flèches | Déplacer la vue |
| +/- | Zoomer/Dézoomer |
| Espace | Pause/Reprendre la simulation |
| T | Activer/Désactiver les connexions transitives |
| B | Basculer thème sombre/clair |
| L | Basculer mode qualité basse/haute |


feat: Calcul asynchrone du graphe d'interférence avec optimisation par antennes

## Résumé
Implémentation du calcul du graphe d'interférence dans un thread séparé
pour éviter le freeze de l'UI, avec utilisation du système d'antennes
hiérarchique pour optimiser les comparaisons de distance O(n²) → O(n).

## Modifications principales

### Thread-safe async calculation (QtConcurrent)
- Ajout de VehicleSnapshot {id, lon, lat, transmissionRange, microAntennaId}
  pour copier les données des véhicules de manière thread-safe
- Ajout de AntennaNeighborhood pour passer la topologie des antennes au thread
- Nouveau QFutureWatcher<InterferenceGraph> pour surveiller le calcul async
- buildGraphFromSnapshots() utilise les snapshots au lieu des pointeurs directs

### Optimisation par antennes
- buildGraphFromSnapshots() utilise maintenant les voisinages d'antennes
  au lieu de comparer tous les véhicules entre eux
- Seuls les véhicules dans la même antenne ou les antennes voisines sont comparés
- Réduction drastique du nombre de comparaisons par tick

### Synchronisation de la portée de transmission
- updateTransmissionRange() recalcule les voisinages d'antennes quand
  la portée est modifiée via l'UI
- map_view.cpp appelle cette méthode sur transmissionRangeChanged

### Assignation dynamique des véhicules aux antennes
- SpatialGrid::assignSingleVehicle() et removeSingleVehicle() pour
  l'ajout/suppression individuelle de véhicules
- InterferenceGraph::assignVehicleToAntenna() et removeVehicleFromAntenna()
- Simulator::addVehicle() assigne automatiquement le véhicule à une antenne
- Simulator::removeVehicle() et setVehicleCount() retirent les véhicules
  de leurs antennes
- createVehicleNear() utilise maintenant addVehicle() pour l'assignation

## Fichiers modifiés
- include/spatial_grid.h: Méthodes d'assignation individuelle
- src/spatial_grid.cpp: Implémentation assignSingleVehicle/removeSingleVehicle
- include/interference_graph.h: VehicleSnapshot, AntennaNeighborhood, nouvelles méthodes
- src/interference_graph.cpp: buildGraphFromSnapshots avec antennes, updateTransmissionRange
- include/simulator.h: addVehicle, removeVehicle, QFutureWatcher
- src/simulator.cpp: Async calculation, gestion des antennes dans add/remove
- src/map_view.cpp: Appel updateTransmissionRange sur changement de portée