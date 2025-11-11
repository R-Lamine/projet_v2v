#include "interference_graph.h"
#include "vehicule.h"
#include <iostream>
#include <algorithm>
#include <chrono>

InterferenceGraph::InterferenceGraph() 
    : m_useSpatialGrid(true), m_gridInitialized(false), m_computeTransitive(true) {}

InterferenceGraph::~InterferenceGraph() {
    clear();
}

void InterferenceGraph::clear() {
    m_adjacencyList.clear();
    m_transitiveClosure.clear();
    m_vehicleMap.clear();
    m_spatialGrid.clear();
}

void InterferenceGraph::buildGraph(const std::vector<Vehicule*>& vehicles) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Effacer seulement les listes d'adjacence, pas la grille spatiale
    m_adjacencyList.clear();
    m_transitiveClosure.clear();
    m_vehicleMap.clear();

    if (vehicles.empty()) {
        return;
    }

    // Construire la map pour accès rapide et initialiser les ensembles
    for (auto* v : vehicles) {
        if (v) {
            m_vehicleMap[v->getId()] = v;
            m_adjacencyList[v->getId()] = std::unordered_set<int>();
        }
    }

    // Grille spatiale : si déjà initialisée, juste réassigner les véhicules
    if (m_useSpatialGrid && m_gridInitialized && vehicles.size() >= 20) {
        m_spatialGrid.assignVehiclesToAntennas(vehicles);
    }

    // Construire les connexions directes
    if (m_useSpatialGrid && vehicles.size() >= 20) {
        buildGraphWithSpatialGrid(vehicles);
    } else {
        buildGraphClassic(vehicles);
    }

    // Calculer la fermeture transitive (optimisée avec grille spatiale) si activé
    if (m_computeTransitive) {
        computeTransitiveClosure();

        // Mettre à jour les voisins de chaque véhicule
        for (auto* v : vehicles) {
            if (!v) continue;

            v->clearNeighbors();
            const auto& reachable = m_transitiveClosure[v->getId()];
            
            for (auto* other : vehicles) {
                if (!other || other == v) continue;
                if (reachable.find(other->getId()) != reachable.end()) {
                    v->addNeighbor(other);
                }
            }
        }
    }
    // Sinon, ne rien faire - pas de mise à jour des voisins

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    // Toujours afficher pour debug avec beaucoup de véhicules
    if (vehicles.size() > 1000) {
        std::cout << "[buildGraph] " << vehicles.size() << " véhicules, " 
                  << duration.count() / 1000.0 << " ms ("
                  << (m_useSpatialGrid && vehicles.size() >= 20 ? "optimisé" : "classique") << ")" << std::endl;
    }
}

void InterferenceGraph::buildGraphClassic(const std::vector<Vehicule*>& vehicles) {
    // Méthode O(n²): comparer toutes les paires
    for (size_t i = 0; i < vehicles.size(); ++i) {
        Vehicule* v1 = vehicles[i];
        if (!v1) continue;

        for (size_t j = i + 1; j < vehicles.size(); ++j) {
            Vehicule* v2 = vehicles[j];
            if (!v2) continue;

            double distance = v1->calculateDist(*v2);
            bool v1CanReachV2 = distance <= v1->getTransmissionRange();
            bool v2CanReachV1 = distance <= v2->getTransmissionRange();

            if (v1CanReachV2 && v2CanReachV1) {
                m_adjacencyList[v1->getId()].insert(v2->getId());
                m_adjacencyList[v2->getId()].insert(v1->getId());
            }
        }
    }
}

void InterferenceGraph::buildGraphWithSpatialGrid(const std::vector<Vehicule*>& vehicles) {
    // Méthode optimisée O(n × k): comparer uniquement avec les voisins spatiaux
    int totalComparisons = 0;
    int totalNearby = 0;
    
    for (auto* v1 : vehicles) {
        if (!v1) continue;

        // Obtenir les véhicules proches via la grille spatiale
        std::vector<int> nearbyIds = m_spatialGrid.getNearbyVehicles(v1->getId());
        totalNearby += nearbyIds.size();
        
        for (int nearbyId : nearbyIds) {
            // Éviter les doublons (on traite chaque paire une seule fois)
            if (nearbyId <= v1->getId()) continue;
            
            totalComparisons++;
            
            auto it = m_vehicleMap.find(nearbyId);
            if (it == m_vehicleMap.end()) continue;
            
            Vehicule* v2 = it->second;
            if (!v2) continue;

            double distance = v1->calculateDist(*v2);
            bool v1CanReachV2 = distance <= v1->getTransmissionRange();
            bool v2CanReachV1 = distance <= v2->getTransmissionRange();

            if (v1CanReachV2 && v2CanReachV1) {
                m_adjacencyList[v1->getId()].insert(v2->getId());
                m_adjacencyList[v2->getId()].insert(v1->getId());
            }
        }
    }
    
    std::cout << "[buildGraphWithSpatialGrid] " << totalComparisons << " comparaisons de distance effectuées"
              << " (moyenne " << (totalNearby / vehicles.size()) << " voisins par véhicule)" << std::endl;
}

void InterferenceGraph::computeTransitiveClosure() {
    // Pour chaque véhicule, calculer tous les véhicules accessibles
    // en utilisant un parcours en largeur (BFS)
    
    m_transitiveClosure.clear();
    
    for (const auto& [vehicleId, neighbors] : m_adjacencyList) {
        m_transitiveClosure[vehicleId] = bfsReachable(vehicleId);
    }
}

std::unordered_set<int> InterferenceGraph::bfsReachable(int startId) const {
    std::unordered_set<int> visited;
    std::queue<int> toVisit;
    
    toVisit.push(startId);
    visited.insert(startId);
    
    while (!toVisit.empty()) {
        int currentId = toVisit.front();
        toVisit.pop();
        
        // Vérifier si ce véhicule existe dans le graphe
        auto it = m_adjacencyList.find(currentId);
        if (it == m_adjacencyList.end()) {
            continue;
        }
        
        // Explorer tous les voisins directs
        for (int neighborId : it->second) {
            // Si on n'a pas encore visité ce voisin
            if (visited.find(neighborId) == visited.end()) {
                visited.insert(neighborId);
                toVisit.push(neighborId);
            }
        }
    }
    
    // Retirer le nœud de départ de l'ensemble des accessibles
    visited.erase(startId);
    
    return visited;
}

bool InterferenceGraph::canCommunicate(int id1, int id2) const {
    // Vérifier si id2 est dans l'ensemble des véhicules accessibles depuis id1
    auto it = m_transitiveClosure.find(id1);
    if (it == m_transitiveClosure.end()) {
        return false;
    }
    
    return it->second.find(id2) != it->second.end();
}

std::unordered_set<int> InterferenceGraph::getReachableVehicles(int vehicleId) const {
    auto it = m_transitiveClosure.find(vehicleId);
    if (it != m_transitiveClosure.end()) {
        return it->second;
    }
    return std::unordered_set<int>();
}

std::unordered_set<int> InterferenceGraph::getDirectNeighbors(int vehicleId) const {
    auto it = m_adjacencyList.find(vehicleId);
    if (it != m_adjacencyList.end()) {
        return it->second;
    }
    return std::unordered_set<int>();
}

void InterferenceGraph::printStats() const {
    std::cout << "\n=== Statistiques du Graphe d'Interférence ===" << std::endl;
    std::cout << "Nombre de véhicules: " << m_adjacencyList.size() << std::endl;
    
    int totalDirectConnections = 0;
    int totalTransitiveConnections = 0;
    
    for (const auto& [id, neighbors] : m_adjacencyList) {
        totalDirectConnections += neighbors.size();
    }
    
    for (const auto& [id, reachable] : m_transitiveClosure) {
        totalTransitiveConnections += reachable.size();
    }
    
    // Diviser par 2 car chaque connexion directe est comptée deux fois (bidirectionnelle)
    std::cout << "Connexions directes: " << totalDirectConnections / 2 << std::endl;
    std::cout << "Connexions totales (avec transitivité): " << totalTransitiveConnections / 2 << std::endl;
    
    // Afficher quelques exemples de véhicules avec leurs connexions
    int count = 0;
    for (const auto& [id, reachable] : m_transitiveClosure) {
        if (count++ >= 5) break; // Afficher seulement les 5 premiers
        
        auto directNeighbors = getDirectNeighbors(id);
        std::cout << "Véhicule " << id << ": " 
                  << directNeighbors.size() << " voisins directs, "
                  << reachable.size() << " véhicules accessibles" << std::endl;
    }
    std::cout << "==========================================\n" << std::endl;
}

void InterferenceGraph::initializeSpatialGrid(const std::vector<Vehicule*>& vehicles) {
    if (!m_useSpatialGrid || vehicles.size() < 20) {
        std::cout << "[InterferenceGraph] Pas assez de véhicules pour la grille spatiale" << std::endl;
        return;
    }
    
    if (m_gridInitialized) {
        std::cout << "[InterferenceGraph] Grille spatiale déjà initialisée" << std::endl;
        return;
    }
    
    std::cout << "[InterferenceGraph] Initialisation de la grille spatiale (K-means)..." << std::endl;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    int numMacro = 10;
    int numMicro = 10;
    
    if (vehicles.size() > 500) {
        numMacro = 20;
        numMicro = 15;
    }
    if (vehicles.size() > 2000) {
        numMacro = 30;
        numMicro = 20;
    }
    
    m_spatialGrid.initialize(vehicles, numMacro, numMicro);
    m_gridInitialized = true;
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "[InterferenceGraph] Grille initialisée en " << duration.count() 
              << " ms avec " << numMacro << " macro et " 
              << numMicro << " micro antennes par macro" << std::endl;
}
