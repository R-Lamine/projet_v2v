#include "simulator.h"

#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>
#include <QtConcurrent>
#include <limits>
#include <algorithm>

// Fonction statique pour calculer le graphe dans un thread séparé
static InterferenceGraph calculateGraphAsync(const std::vector<VehicleSnapshot>& snapshots, 
                                              bool computeTransitive,
                                              const AntennaNeighborhood& antennaInfo) {
    InterferenceGraph tempGraph;
    tempGraph.enableTransitiveClosure(computeTransitive);
    tempGraph.buildGraphFromSnapshots(snapshots, &antennaInfo);
    return tempGraph;
}

Simulator::Simulator(RoadGraph& graph, MapView* mapView, QObject* parent)
    :graph(graph), m_mapView(mapView), QObject(parent)
{
    // initialize elapsed timer
    m_elapsed.start();

    // setup the QTimer
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &Simulator::onTick);
    
    // Initialiser la liste des vertices pour la création dynamique
    for (auto vp = boost::vertices(graph); vp.first != vp.second; ++vp.first) {
        m_vertices.push_back(*vp.first);
    }
    
    // Créer le watcher pour les calculs asynchrones
    m_futureWatcher = new QFutureWatcher<InterferenceGraph>(this);
    connect(m_futureWatcher, &QFutureWatcher<InterferenceGraph>::finished, 
            this, &Simulator::onGraphCalculationFinished);
}

Simulator::~Simulator() {
    stop();
    
    // Attendre la fin du calcul en cours
    if (m_futureWatcher && m_futureWatcher->isRunning()) {
        m_futureWatcher->waitForFinished();
    }
}

void Simulator::start(int tickIntervalMs) {
    m_tickIntervalMs = tickIntervalMs;
    m_running = true;
    m_paused = false;
    m_elapsed.restart();
    
    // Initialiser le compteur d'ID pour les nouveaux véhicules
    if (!m_vehicles.empty()) {
        int maxId = 0;
        for (const auto* vehicle : m_vehicles) {
            maxId = std::max(maxId, vehicle->getId());
        }
        m_nextVehicleId = maxId + 1;
    }
    
    // Initialiser la grille spatiale UNE SEULE FOIS au démarrage
    // Utilise les valeurs par défaut de l'UI: 5 grandes antennes, 20 petites par grande
    if (!m_vehicles.empty()) {
        m_interferenceGraph.initializeSpatialGrid(m_vehicles, 5, 20);
    }
    
    m_timer->start(tickIntervalMs);
    emit simulationStarted();
}

void Simulator::pause() {
    m_paused = true;
    m_timer->stop();
    emit simulationPaused();
}

void Simulator::resume() {
    m_paused = false;
    m_elapsed.restart();
    m_timer->start(m_tickIntervalMs);
    emit simulationResumed();
}

void Simulator::togglePause() {
    if (m_paused) {
        resume();
    } else {
        pause();
    }
}

void Simulator::clearVehicles() {
    for (Vehicule* v : m_vehicles) {
        delete v;
    }
    m_vehicles.clear();
}

void Simulator::reset() {
    pause();
    clearVehicles();
    m_interferenceGraph.clear();
    if (m_mapView) {
        m_mapView->update();
    }
}

void Simulator::stop() {
    m_running = false;
    m_timer->stop();
    emit simulationStopped();
}

void Simulator::onTick() {
    double deltaTime = m_elapsed.restart() / 1000.0; // seconds
    deltaTime *= m_speedMultiplier;

    // Mise à jour de la position des véhicules
    for (Vehicule* v : m_vehicles) {
        if(v) v->update(deltaTime);
    }

    // Lancer le calcul du graphe en arrière-plan si pas déjà en cours
    if (!m_calculationInProgress && !m_vehicles.empty()) {
        startGraphCalculation();
    }

    emit ticked(deltaTime);
}

void Simulator::startGraphCalculation() {
    if (m_calculationInProgress) return;
    
    // Créer les snapshots des véhicules avec leurs infos d'antenne
    std::vector<VehicleSnapshot> snapshots;
    snapshots.reserve(m_vehicles.size());
    
    // Récupérer la grille spatiale pour les infos d'antennes
    const SpatialGrid& spatialGrid = m_interferenceGraph.getSpatialGrid();
    
    for (size_t i = 0; i < m_vehicles.size(); ++i) {
        const Vehicule* v = m_vehicles[i];
        if (v) {
            auto [lat, lon] = v->getPosition();
            int microAntennaId = spatialGrid.getMicroAntennaId(v->getId());
            snapshots.push_back({
                v->getId(),
                lon,
                lat,
                v->getTransmissionRange(),
                microAntennaId
            });
        }
    }
    
    if (snapshots.empty()) return;
    
    // Créer les infos de voisinage d'antennes
    AntennaNeighborhood antennaInfo;
    
    // Remplir les véhicules par antenne (utiliser l'index dans snapshots)
    for (size_t i = 0; i < snapshots.size(); ++i) {
        int antennaId = snapshots[i].microAntennaId;
        if (antennaId >= 0) {
            antennaInfo.vehiclesPerAntenna[antennaId].push_back(i);
        }
    }
    
    // Copier les voisinages d'antennes depuis la grille spatiale
    const auto& microAntennas = spatialGrid.getMicroAntennas();
    for (const auto& [antennaId, micro] : microAntennas) {
        antennaInfo.neighborAntennas[antennaId] = micro.neighborMicroIds;
    }
    
    m_calculationInProgress = true;
    
    // Récupérer le flag de fermeture transitive
    bool computeTransitive = m_interferenceGraph.isTransitiveClosureEnabled();
    
    // Lancer le calcul dans un thread séparé
    QFuture<InterferenceGraph> future = QtConcurrent::run(calculateGraphAsync, snapshots, computeTransitive, antennaInfo);
    m_futureWatcher->setFuture(future);
}

void Simulator::onGraphCalculationFinished() {
    // Récupérer le résultat et le copier dans le graphe principal
    InterferenceGraph result = m_futureWatcher->result();
    m_interferenceGraph.copyFrom(result);
    
    m_calculationInProgress = false;
    
    // Redessiner la vue
    if (m_mapView) {
        m_mapView->update();
    }
}

void Simulator::addVehicle(Vehicule* v) {
    if(v) {
        m_vehicles.push_back(v);
        // Assigner le nouveau véhicule à son antenne
        m_interferenceGraph.assignVehicleToAntenna(v);
        emit vehicleCountChanged(m_vehicles.size());
    }
}

bool Simulator::removeVehicle(Vehicule* v) {
    if (!v) return false;
    
    auto it = std::find(m_vehicles.begin(), m_vehicles.end(), v);
    if (it != m_vehicles.end()) {
        // Retirer le véhicule de son antenne avant de le supprimer
        m_interferenceGraph.removeVehicleFromAntenna(v->getId());
        m_vehicles.erase(it);
        delete v;
        // Le graphe sera recalculé au prochain tick par le worker thread
        emit vehicleCountChanged(m_vehicles.size());
        return true;
    }
    return false;
}

Vehicule* Simulator::createVehicleNear(double lon, double lat) {
    if (m_vertices.empty()) return nullptr;
    
    // Trouver le vertex le plus proche de la position cliquée
    Vertex nearestVertex = m_vertices[0];
    double minDist = std::numeric_limits<double>::max();
    
    // Parcourir tous les vertices pour trouver le plus proche
    for (const Vertex& v : m_vertices) {
        double vLat = graph[v].lat;
        double vLon = graph[v].lon;
        double dist = (vLon - lon) * (vLon - lon) + (vLat - lat) * (vLat - lat);  // Distance² (plus rapide)
        if (dist < minDist) {
            // Vérifier la validité seulement pour les candidats proches
            if (Vehicule::isValidVertex(v, graph) && Vehicule::hasValidOutgoingEdge(v, graph)) {
                minDist = dist;
                nearestVertex = v;
            }
        }
    }
    
    // Choisir un goal aléatoire
    Vertex goal = m_vertices[rand() % m_vertices.size()];
    int attempts = 0;
    while ((!Vehicule::isValidVertex(goal, graph) || !Vehicule::hasValidOutgoingEdge(goal, graph)) && attempts < 100) {
        goal = m_vertices[rand() % m_vertices.size()];
        attempts++;
    }
    
    // Créer le véhicule
    double speed = 14;          // 50 km/h en m/s
    double range = 500.0;
    double collisionDist = 5.0;
    
    Vehicule* car = new Vehicule(m_nextVehicleId++, graph, nearestVertex, goal,
                                 speed, range, collisionDist);
    // Utiliser addVehicle pour assigner le véhicule à une antenne
    addVehicle(car);
    
    return car;
}

void Simulator::setVehicleCount(int count) {
    int currentCount = m_vehicles.size();
    
    if (count == currentCount) {
        return; // Pas de changement
    }
    
    if (count < currentCount) {
        // Supprimer des véhicules
        int toRemove = currentCount - count;
        for (int i = 0; i < toRemove && !m_vehicles.empty(); ++i) {
            Vehicule* v = m_vehicles.back();
            // Retirer le véhicule de son antenne avant de le supprimer
            m_interferenceGraph.removeVehicleFromAntenna(v->getId());
            m_vehicles.pop_back();
            delete v;
        }
    } else {
        // Ajouter des véhicules
        int toAdd = count - currentCount;
        
        for (int i = 0; i < toAdd; ++i) {
            if (m_vertices.empty()) break;
            
            // Choisir aléatoirement start et goal
            Vertex start = m_vertices[rand() % m_vertices.size()];
            Vertex goal = m_vertices[rand() % m_vertices.size()];
            
            // Vérifier que les vertices sont valides
            while (!Vehicule::isValidVertex(start, graph) && 
                   !Vehicule::hasValidOutgoingEdge(start, graph)) {
                start = m_vertices[rand() % m_vertices.size()];
            }
            
            while (!Vehicule::isValidVertex(goal, graph) && 
                   !Vehicule::hasValidOutgoingEdge(goal, graph)) {
                goal = m_vertices[rand() % m_vertices.size()];
            }
            
            // Créer le nouveau véhicule avec les mêmes paramètres
            double speed = 14;          // 50 km/h in m/s
            double range = 500.0;        // transmission range
            double collisionDist = 5.0;   // 5 meters
            
            Vehicule* car = new Vehicule(m_nextVehicleId++, graph, start, goal, 
                                        speed, range, collisionDist);
            addVehicle(car);
        }
    }
    
    // Le graphe sera recalculé au prochain tick par le worker thread
    
    // Notifier le changement de nombre de véhicules
    emit vehicleCountChanged(m_vehicles.size());
}

void Simulator::placeAntennas(int numLarge, int numSmall) {
    if (m_vehicles.empty()) {
        std::cout << "[Simulator] Pas de véhicules pour placer les antennes" << std::endl;
        return;
    }
    
    std::cout << "[Simulator] Placement des antennes: " << numLarge << " grandes, " 
              << numSmall << " petites par grande" << std::endl;
    
    // Réinitialiser la grille avec les nouveaux paramètres
    m_interferenceGraph.reinitializeSpatialGrid(m_vehicles, numLarge, numSmall);
    
    // Le graphe sera recalculé au prochain tick par le worker thread
}