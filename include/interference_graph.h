#ifndef INTERFERENCE_GRAPH_H
#define INTERFERENCE_GRAPH_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <mutex>
#include "spatial_grid.h"

class Vehicule;

// Structure pour stocker un snapshot des positions des véhicules (thread-safe)
struct VehicleSnapshot {
    int id;
    double lon;
    double lat;
    double transmissionRange;
    int microAntennaId;  // ID de la petite antenne à laquelle ce véhicule appartient
};

// Structure pour stocker les infos de voisinage d'antennes (thread-safe)
struct AntennaNeighborhood {
    std::unordered_map<int, std::vector<int>> vehiclesPerAntenna;  // microAntennaId -> liste des indices de véhicules
    std::unordered_map<int, std::set<int>> neighborAntennas;        // microAntennaId -> set des antennes voisines
};

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
     * @brief Construit le graphe à partir de snapshots (thread-safe)
     * @param snapshots Copies des positions des véhicules
     * @param antennaInfo Infos sur les antennes et leurs voisinages (optionnel)
     */
    void buildGraphFromSnapshots(const std::vector<VehicleSnapshot>& snapshots,
                                  const AntennaNeighborhood* antennaInfo = nullptr);

    /**
     * @brief Copie les données d'un autre graphe (pour synchronisation thread-safe)
     * @param other Graphe source à copier
     */
    void copyFrom(const InterferenceGraph& other);

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

    /**
     * @brief Active ou désactive l'optimisation par grille spatiale
     * @param enable true pour activer, false pour désactiver
     */
    void enableSpatialOptimization(bool enable) { m_useSpatialGrid = enable; }

    /**
     * @brief Vérifie si l'optimisation spatiale est active
     */
    bool isSpatialOptimizationEnabled() const { return m_useSpatialGrid; }

    /**
     * @brief Obtient une référence constante à la grille spatiale
     */
    const SpatialGrid& getSpatialGrid() const { return m_spatialGrid; }

    /**
     * @brief Initialise la grille spatiale une fois pour toutes (K-means)
     * @param vehicles Liste des véhicules pour calculer la disposition optimale
     * @param numMacro Nombre de grandes antennes (0 = auto)
     * @param numMicro Nombre de petites antennes par grande (0 = auto)
     * 
     * Cette méthode doit être appelée UNE SEULE FOIS au début de la simulation.
     * Elle calcule les positions des antennes selon la densité des véhicules.
     */
    void initializeSpatialGrid(const std::vector<Vehicule*>& vehicles, int numMacro = 0, int numMicro = 0);
    
    /**
     * @brief Force la réinitialisation de la grille spatiale (K-means)
     * Utilisé quand l'utilisateur change le nombre d'antennes
     */
    void reinitializeSpatialGrid(const std::vector<Vehicule*>& vehicles, int numMacro, int numMicro);

    /**
     * @brief Assigne un véhicule à son antenne la plus proche
     * À appeler quand un nouveau véhicule est ajouté
     */
    void assignVehicleToAntenna(Vehicule* vehicle);
    
    /**
     * @brief Retire un véhicule de son antenne
     * À appeler quand un véhicule est supprimé
     */
    void removeVehicleFromAntenna(int vehicleId);

    /**
     * @brief Met à jour la portée de transmission pour les calculs de voisinage
     * @param range Nouvelle portée en mètres
     * 
     * Cette méthode recalcule quelles antennes sont voisines en fonction
     * de la nouvelle portée de transmission.
     */
    void updateTransmissionRange(double range);

    /**
     * @brief Active ou désactive le calcul de la fermeture transitive
     * @param enable true pour activer, false pour désactiver
     */
    void enableTransitiveClosure(bool enable) { m_computeTransitive = enable; }

    /**
     * @brief Vérifie si la fermeture transitive est activée
     */
    bool isTransitiveClosureEnabled() const { return m_computeTransitive; }

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

    /**
     * @brief Construit le graphe avec la méthode classique O(n²)
     * @param vehicles Liste de tous les véhicules
     */
    void buildGraphClassic(const std::vector<Vehicule*>& vehicles);

    /**
     * @brief Construit le graphe avec optimisation spatiale O(n × k)
     * @param vehicles Liste de tous les véhicules
     */
    void buildGraphWithSpatialGrid(const std::vector<Vehicule*>& vehicles);

private:
    // Liste d'adjacence pour les connexions directes (basées sur la portée)
    std::unordered_map<int, std::unordered_set<int>> m_adjacencyList;

    // Fermeture transitive: pour chaque véhicule, tous les véhicules accessibles
    // (directement ou indirectement)
    std::unordered_map<int, std::unordered_set<int>> m_transitiveClosure;

    // Grille spatiale pour optimisation des calculs de distance
    SpatialGrid m_spatialGrid;
    bool m_useSpatialGrid;
    bool m_gridInitialized;  // Flag pour savoir si la grille a été initialisée
    bool m_computeTransitive; // Flag pour activer/désactiver la fermeture transitive

    // Map pour accéder rapidement aux véhicules par ID
    std::unordered_map<int, Vehicule*> m_vehicleMap;
    
    // Statistiques de performance (mis à jour à chaque buildGraph)
    mutable int m_lastComparisons = 0;
    mutable double m_lastAvgNeighbors = 0.0;
    mutable double m_lastBuildTimeMs = 0.0;
    
public:
    // Getters pour les statistiques de performance
    int getLastComparisons() const { return m_lastComparisons; }
    double getLastAvgNeighbors() const { return m_lastAvgNeighbors; }
    double getLastBuildTimeMs() const { return m_lastBuildTimeMs; }
};

#endif // INTERFERENCE_GRAPH_H
