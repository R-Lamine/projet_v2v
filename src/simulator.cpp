#include "simulator.h"

#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>
#include <limits>
#include <algorithm>

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
}

Simulator::~Simulator() {
    stop();
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

    static int tickCount = 0;
    tickCount++;

    // Mise à jour de la position des véhicules
    for (Vehicule* v : m_vehicles) {
        if(v) v->update(deltaTime);
    }

    // Avec beaucoup de véhicules (>1000), ne reconstruire le graphe que rarement
    int rebuildInterval = 10; // Par défaut 500ms
    if (m_vehicles.size() > 500) {
        rebuildInterval = 20;
    }
    if (m_vehicles.size() > 1000) {
        rebuildInterval = 40; // 2.5 secondes
    }
    if (m_vehicles.size() > 2000) {
        rebuildInterval = 100; // 5 secondes
    }
    
    if (tickCount % rebuildInterval == 0) {
        m_interferenceGraph.buildGraph(m_vehicles);
    }

    emit ticked(deltaTime);
}

void Simulator::addVehicle(Vehicule* v) {
    if(v) {
        m_vehicles.push_back(v);
        emit vehicleCountChanged(m_vehicles.size());
    }
}

bool Simulator::removeVehicle(Vehicule* v) {
    if (!v) return false;
    
    auto it = std::find(m_vehicles.begin(), m_vehicles.end(), v);
    if (it != m_vehicles.end()) {
        m_vehicles.erase(it);
        delete v;
        m_interferenceGraph.buildGraph(m_vehicles);
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
    m_vehicles.push_back(car);
    // Pas de buildGraph ici - sera fait au prochain tick
    emit vehicleCountChanged(m_vehicles.size());
    
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
    
    // Reconstruire le graphe d'interférence
    m_interferenceGraph.buildGraph(m_vehicles);
    
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
    
    // Reconstruire le graphe d'interférence avec la nouvelle grille
    m_interferenceGraph.buildGraph(m_vehicles);
}