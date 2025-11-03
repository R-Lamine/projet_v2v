#include "interference_graph.h"
#include "vehicule.h"
#include <iostream>
#include <algorithm>

InterferenceGraph::InterferenceGraph() {}

InterferenceGraph::~InterferenceGraph() {
    clear();
}

void InterferenceGraph::clear() {
    m_adjacencyList.clear();
    m_transitiveClosure.clear();
}

void InterferenceGraph::buildGraph(const std::vector<Vehicule*>& vehicles) {
    // Étape 1: Effacer le graphe précédent
    clear();

    if (vehicles.empty()) {
        return;
    }

    // Étape 2: Initialiser les ensembles vides pour chaque véhicule
    for (auto* v : vehicles) {
        if (v) {
            m_adjacencyList[v->getId()] = std::unordered_set<int>();
        }
    }

    // Étape 3: Construire les connexions directes basées sur la portée de transmission
    // Pour chaque paire de véhicules, vérifier s'ils sont dans la portée l'un de l'autre
    for (size_t i = 0; i < vehicles.size(); ++i) {
        Vehicule* v1 = vehicles[i];
        if (!v1) continue;

        for (size_t j = i + 1; j < vehicles.size(); ++j) {
            Vehicule* v2 = vehicles[j];
            if (!v2) continue;

            // Calculer la distance entre les deux véhicules
            double distance = v1->calculateDist(*v2);

            // Vérifier si chaque véhicule est dans la portée de l'autre
            bool v1CanReachV2 = distance <= v1->getTransmissionRange();
            bool v2CanReachV1 = distance <= v2->getTransmissionRange();

            // Les deux doivent pouvoir se joindre (communication bidirectionnelle)
            if (v1CanReachV2 && v2CanReachV1) {
                // Ajouter la connexion bidirectionnelle
                m_adjacencyList[v1->getId()].insert(v2->getId());
                m_adjacencyList[v2->getId()].insert(v1->getId());
            }
        }
    }

    // Étape 4: Calculer la fermeture transitive
    // Si A peut communiquer avec B et B avec C, alors A peut communiquer avec C
    computeTransitiveClosure();

    // Étape 5: Mettre à jour les voisins de chaque véhicule
    // Maintenant on considère tous les véhicules accessibles, pas seulement les directs
    for (auto* v : vehicles) {
        if (!v) continue;

        v->clearNeighbors();
        
        // Obtenir tous les véhicules accessibles (directement ou indirectement)
        const auto& reachable = m_transitiveClosure[v->getId()];
        
        for (auto* other : vehicles) {
            if (!other || other == v) continue;
            
            // Si le véhicule est accessible, l'ajouter comme voisin
            if (reachable.find(other->getId()) != reachable.end()) {
                v->addNeighbor(other);
            }
        }
    }
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
