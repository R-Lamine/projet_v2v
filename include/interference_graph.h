#ifndef INTERFERENCE_GRAPH_H
#define INTERFERENCE_GRAPH_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>

class Vehicule;

/**
 * @brief Graphe d'interférence pour gérer la communication entre véhicules
 * 
 * Ce graphe utilise une liste d'adjacence pour représenter les connexions
 * entre véhicules. Deux véhicules sont connectés si:
 * 1. Ils sont dans la portée de transmission l'un de l'autre (connexion directe)
 * 2. Ils peuvent communiquer via d'autres véhicules (connexion transitive)
 *    Si A communique avec B et B avec C, alors A et C peuvent aussi communiquer
 */
class InterferenceGraph {
public:
    InterferenceGraph();
    ~InterferenceGraph();

    /**
     * @brief Construit le graphe d'interférence basé sur les positions des véhicules
     * @param vehicles Liste de tous les véhicules dans la simulation
     * 
     * Étape 1: Connexions directes basées sur la portée de transmission
     * Étape 2: Calcul de la fermeture transitive pour les connexions indirectes
     */
    void buildGraph(const std::vector<Vehicule*>& vehicles);

    /**
     * @brief Efface toutes les connexions du graphe
     */
    void clear();

    /**
     * @brief Vérifie si deux véhicules peuvent communiquer (directement ou indirectement)
     * @param id1 ID du premier véhicule
     * @param id2 ID du deuxième véhicule
     * @return true si les véhicules peuvent communiquer, false sinon
     */
    bool canCommunicate(int id1, int id2) const;

    /**
     * @brief Obtient tous les véhicules avec lesquels un véhicule peut communiquer
     * @param vehicleId ID du véhicule
     * @return Ensemble des IDs des véhicules accessibles
     */
    std::unordered_set<int> getReachableVehicles(int vehicleId) const;

    /**
     * @brief Obtient les voisins directs d'un véhicule (portée de transmission)
     * @param vehicleId ID du véhicule
     * @return Ensemble des IDs des voisins directs
     */
    std::unordered_set<int> getDirectNeighbors(int vehicleId) const;

    /**
     * @brief Obtient le nombre de véhicules dans le graphe
     */
    int getVehicleCount() const { return m_adjacencyList.size(); }

    /**
     * @brief Affiche les statistiques du graphe (pour debug)
     */
    void printStats() const;

private:
    /**
     * @brief Calcule la fermeture transitive du graphe
     * Utilise un BFS pour trouver tous les véhicules accessibles depuis chaque véhicule
     */
    void computeTransitiveClosure();

    /**
     * @brief Effectue un BFS pour trouver tous les nœuds accessibles depuis un nœud source
     * @param startId ID du véhicule de départ
     * @return Ensemble des IDs des véhicules accessibles
     */
    std::unordered_set<int> bfsReachable(int startId) const;

private:
    // Liste d'adjacence pour les connexions directes (basées sur la portée)
    std::unordered_map<int, std::unordered_set<int>> m_adjacencyList;

    // Fermeture transitive: pour chaque véhicule, tous les véhicules accessibles
    // (directement ou indirectement)
    std::unordered_map<int, std::unordered_set<int>> m_transitiveClosure;
};

#endif // INTERFERENCE_GRAPH_H
