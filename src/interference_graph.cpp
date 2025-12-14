#include "interference_graph.h"
#include "vehicule.h"
#include <iostream>
#include <algorithm>
#include <chrono>

InterferenceGraph::InterferenceGraph() 
    : m_useSpatialGrid(true), m_gridInitialized(false), m_computeTransitive(false) {}

InterferenceGraph::~InterferenceGraph() {
    clear();
}

void InterferenceGraph::clear() {
    m_adjacencyList.clear();
    m_transitiveClosure.clear();
    m_vehicleMap.clear();
    m_spatialGrid.clear();
}

void InterferenceGraph::copyFrom(const InterferenceGraph& other) {
    m_adjacencyList = other.m_adjacencyList;
    m_transitiveClosure = other.m_transitiveClosure;
    // On ne copie pas m_vehicleMap car il contient des pointeurs
    // On ne copie pas m_spatialGrid car elle est initialisée séparément
    m_lastBuildTimeMs = other.m_lastBuildTimeMs;
    m_lastComparisons = other.m_lastComparisons;
    m_lastAvgNeighbors = other.m_lastAvgNeighbors;
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
    
    // Stocker le temps de build
    m_lastBuildTimeMs = duration.count() / 1000.0;
    
    // Toujours afficher pour debug avec beaucoup de véhicules
    if (vehicles.size() > 1000) {
        std::cout << "[buildGraph] " << vehicles.size() << " véhicules, " 
                  << duration.count() / 1000.0 << " ms ("
                  << (m_useSpatialGrid && vehicles.size() >= 20 ? "optimisé" : "classique") << ")" << std::endl;
    }
}

void InterferenceGraph::buildGraphFromSnapshots(const std::vector<VehicleSnapshot>& snapshots,
                                                  const AntennaNeighborhood* antennaInfo) {
    // Cette méthode est thread-safe car elle travaille sur des copies
    auto startTime = std::chrono::high_resolution_clock::now();
    
    m_adjacencyList.clear();
    m_transitiveClosure.clear();
    m_vehicleMap.clear();

    if (snapshots.empty()) {
        return;
    }

    // Initialiser les ensembles pour chaque véhicule
    for (const auto& snap : snapshots) {
        m_adjacencyList[snap.id] = std::unordered_set<int>();
    }

    int comparisons = 0;
    
    // Si on a les infos d'antennes, utiliser l'optimisation par antennes
    if (antennaInfo != nullptr && !antennaInfo->vehiclesPerAntenna.empty()) {
        // Créer un index pour accéder aux snapshots par leur ID
        std::unordered_map<int, size_t> idToIndex;
        for (size_t i = 0; i < snapshots.size(); ++i) {
            idToIndex[snapshots[i].id] = i;
        }
        
        // Pour chaque antenne, comparer les véhicules de cette antenne entre eux
        // et avec les véhicules des antennes voisines
        for (const auto& [antennaId, vehicleIndices] : antennaInfo->vehiclesPerAntenna) {
            // 1. Comparer les véhicules de la même antenne entre eux
            for (size_t i = 0; i < vehicleIndices.size(); ++i) {
                size_t idx1 = vehicleIndices[i];
                if (idx1 >= snapshots.size()) continue;
                const auto& v1 = snapshots[idx1];
                
                for (size_t j = i + 1; j < vehicleIndices.size(); ++j) {
                    size_t idx2 = vehicleIndices[j];
                    if (idx2 >= snapshots.size()) continue;
                    const auto& v2 = snapshots[idx2];
                    
                    comparisons++;
                    
                    // Calcul rapide de distance (approximation euclidienne)
                    double dLat = (v2.lat - v1.lat) * 111000.0;
                    double dLon = (v2.lon - v1.lon) * 111000.0 * std::cos(v1.lat * M_PI / 180.0);
                    double distance = std::sqrt(dLat * dLat + dLon * dLon);
                    
                    if (distance <= v1.transmissionRange && distance <= v2.transmissionRange) {
                        m_adjacencyList[v1.id].insert(v2.id);
                        m_adjacencyList[v2.id].insert(v1.id);
                    }
                }
            }
            
            // 2. Comparer avec les véhicules des antennes voisines
            auto neighborIt = antennaInfo->neighborAntennas.find(antennaId);
            if (neighborIt != antennaInfo->neighborAntennas.end()) {
                for (int neighborAntennaId : neighborIt->second) {
                    // Ne comparer que si notre ID est plus petit (éviter doublons)
                    if (neighborAntennaId <= antennaId) continue;
                    
                    auto neighborVehiclesIt = antennaInfo->vehiclesPerAntenna.find(neighborAntennaId);
                    if (neighborVehiclesIt == antennaInfo->vehiclesPerAntenna.end()) continue;
                    
                    for (size_t idx1 : vehicleIndices) {
                        if (idx1 >= snapshots.size()) continue;
                        const auto& v1 = snapshots[idx1];
                        
                        for (size_t idx2 : neighborVehiclesIt->second) {
                            if (idx2 >= snapshots.size()) continue;
                            const auto& v2 = snapshots[idx2];
                            
                            comparisons++;
                            
                            double dLat = (v2.lat - v1.lat) * 111000.0;
                            double dLon = (v2.lon - v1.lon) * 111000.0 * std::cos(v1.lat * M_PI / 180.0);
                            double distance = std::sqrt(dLat * dLat + dLon * dLon);
                            
                            if (distance <= v1.transmissionRange && distance <= v2.transmissionRange) {
                                m_adjacencyList[v1.id].insert(v2.id);
                                m_adjacencyList[v2.id].insert(v1.id);
                            }
                        }
                    }
                }
            }
        }
    } else {
        // Fallback: O(n²) classique si pas d'infos d'antennes
        for (size_t i = 0; i < snapshots.size(); ++i) {
            const auto& v1 = snapshots[i];

            for (size_t j = i + 1; j < snapshots.size(); ++j) {
                const auto& v2 = snapshots[j];
                comparisons++;
                
                double dLat = (v2.lat - v1.lat) * 111000.0;
                double dLon = (v2.lon - v1.lon) * 111000.0 * std::cos(v1.lat * M_PI / 180.0);
                double distance = std::sqrt(dLat * dLat + dLon * dLon);

                if (distance <= v1.transmissionRange && distance <= v2.transmissionRange) {
                    m_adjacencyList[v1.id].insert(v2.id);
                    m_adjacencyList[v2.id].insert(v1.id);
                }
            }
        }
    }

    // Calculer la fermeture transitive si activé
    if (m_computeTransitive) {
        computeTransitiveClosure();
    }

    m_lastComparisons = comparisons;
    m_lastAvgNeighbors = snapshots.size() > 0 ? static_cast<double>(comparisons * 2) / snapshots.size() : 0.0;

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    m_lastBuildTimeMs = duration.count() / 1000.0;
}

void InterferenceGraph::buildGraphClassic(const std::vector<Vehicule*>& vehicles) {
    // Méthode O(n²): comparer toutes les paires
    int comparisons = 0;
    
    for (size_t i = 0; i < vehicles.size(); ++i) {
        Vehicule* v1 = vehicles[i];
        if (!v1) continue;

        for (size_t j = i + 1; j < vehicles.size(); ++j) {
            Vehicule* v2 = vehicles[j];
            if (!v2) continue;

            comparisons++;
            double distance = v1->calculateDist(*v2);
            bool v1CanReachV2 = distance <= v1->getTransmissionRange();
            bool v2CanReachV1 = distance <= v2->getTransmissionRange();

            if (v1CanReachV2 && v2CanReachV1) {
                m_adjacencyList[v1->getId()].insert(v2->getId());
                m_adjacencyList[v2->getId()].insert(v1->getId());
            }
        }
    }
    
    // Stocker les statistiques (méthode classique)
    m_lastComparisons = comparisons;
    m_lastAvgNeighbors = vehicles.size() > 0 ? static_cast<double>(vehicles.size() - 1) : 0.0;
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
    
    // Stocker les statistiques
    m_lastComparisons = totalComparisons;
    m_lastAvgNeighbors = vehicles.size() > 0 ? static_cast<double>(totalNearby) / vehicles.size() : 0.0;
    
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

void InterferenceGraph::initializeSpatialGrid(const std::vector<Vehicule*>& vehicles, int numMacro, int numMicro) {
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
    
    // Si paramètres = 0, utiliser les valeurs automatiques
    if (numMacro == 0) {
        numMacro = 10;
        if (vehicles.size() > 500) numMacro = 20;
        if (vehicles.size() > 2000) numMacro = 30;
    }
    if (numMicro == 0) {
        numMicro = 10;
        if (vehicles.size() > 500) numMicro = 15;
        if (vehicles.size() > 2000) numMicro = 20;
    }
    
    // Utiliser la portée de transmission du premier véhicule (tous ont la même)
    double maxRange = 500.0;  // Valeur par défaut
    if (!vehicles.empty() && vehicles[0]) {
        maxRange = vehicles[0]->getTransmissionRange();
    }
    m_spatialGrid.setMaxTransmissionRange(maxRange);
    
    m_spatialGrid.initialize(vehicles, numMacro, numMicro);
    m_gridInitialized = true;
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "[InterferenceGraph] Grille initialisée en " << duration.count() 
              << " ms avec " << numMacro << " macro et " 
              << numMicro << " micro antennes par macro (portée max: " << maxRange << "m)" << std::endl;
}

void InterferenceGraph::reinitializeSpatialGrid(const std::vector<Vehicule*>& vehicles, int numMacro, int numMicro) {
    if (!m_useSpatialGrid || vehicles.size() < 20) {
        std::cout << "[InterferenceGraph] Pas assez de véhicules pour la grille spatiale" << std::endl;
        return;
    }
    
    std::cout << "[InterferenceGraph] Réinitialisation de la grille spatiale (K-means)..." << std::endl;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Utiliser la portée de transmission du premier véhicule (tous ont la même)
    double maxRange = 500.0;  // Valeur par défaut
    if (!vehicles.empty() && vehicles[0]) {
        maxRange = vehicles[0]->getTransmissionRange();
    }
    m_spatialGrid.setMaxTransmissionRange(maxRange);
    
    // Force la réinitialisation
    m_gridInitialized = false;
    m_spatialGrid.initialize(vehicles, numMacro, numMicro);
    m_gridInitialized = true;
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "[InterferenceGraph] Grille réinitialisée en " << duration.count() 
              << " ms avec " << numMacro << " macro et " 
              << numMicro << " micro antennes par macro (portée max: " << maxRange << "m)" << std::endl;
}

void InterferenceGraph::updateTransmissionRange(double range) {
    if (!m_gridInitialized) {
        return;  // La grille n'est pas encore initialisée
    }
    
    // Mettre à jour la portée dans la grille spatiale
    m_spatialGrid.setMaxTransmissionRange(range);
    
    // Recalculer les voisinages entre antennes
    m_spatialGrid.updateNeighborhoods();
}

void InterferenceGraph::assignVehicleToAntenna(Vehicule* vehicle) {
    if (!m_gridInitialized || !vehicle) {
        return;
    }
    m_spatialGrid.assignVehicleToAntenna(vehicle);
}

void InterferenceGraph::removeVehicleFromAntenna(int vehicleId) {
    if (!m_gridInitialized) {
        return;
    }
    m_spatialGrid.removeVehicleFromAntenna(vehicleId);
}