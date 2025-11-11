#ifndef SPATIAL_GRID_H
#define SPATIAL_GRID_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <cmath>

class Vehicule;

/**
 * @brief Structure représentant une petite antenne (micro-cellule)
 */
struct MicroAntenna {
    int id;
    int macroAntennaId;  // ID de l'antenne parente
    double centerLat;
    double centerLon;
    double radius;  // Rayon de couverture
    std::vector<int> vehicleIds;  // Véhicules dans cette zone
    std::set<int> neighborMicroIds;  // Petites antennes voisines
};

/**
 * @brief Structure représentant une grande antenne (macro-cellule)
 */
struct MacroAntenna {
    int id;
    double centerLat;
    double centerLon;
    double radius;  // Rayon de couverture
    std::vector<int> microAntennaIds;  // IDs des petites antennes
    std::set<int> neighborMacroIds;  // Grandes antennes voisines
};

/**
 * @brief Grille spatiale hiérarchique pour optimiser les calculs de distance
 * 
 * Utilise une hiérarchie à deux niveaux (grandes et petites antennes)
 * pour réduire le nombre de comparaisons de distance entre véhicules.
 */
class SpatialGrid {
public:
    SpatialGrid();
    ~SpatialGrid();

    /**
     * @brief Initialise la grille spatiale basée sur les positions des véhicules
     * @param vehicles Liste des véhicules
     * @param numMacroAntennas Nombre de grandes antennes (défaut: 10)
     * @param microPerMacro Nombre de petites antennes par grande antenne (défaut: 10)
     */
    void initialize(const std::vector<Vehicule*>& vehicles, 
                   int numMacroAntennas = 10, 
                   int microPerMacro = 10);

    /**
     * @brief Assigne chaque véhicule à sa petite antenne la plus proche
     * @param vehicles Liste des véhicules
     */
    void assignVehiclesToAntennas(const std::vector<Vehicule*>& vehicles);

    /**
     * @brief Obtient tous les véhicules proches (même antenne + voisines)
     * @param vehicleId ID du véhicule
     * @return IDs des véhicules dans la zone élargie
     */
    std::vector<int> getNearbyVehicles(int vehicleId) const;

    /**
     * @brief Obtient l'ID de la petite antenne d'un véhicule
     */
    int getMicroAntennaId(int vehicleId) const;

    /**
     * @brief Obtient l'ID de la grande antenne d'un véhicule
     */
    int getMacroAntennaId(int vehicleId) const;

    /**
     * @brief Efface toutes les données
     */
    void clear();

    /**
     * @brief Affiche des statistiques sur la grille
     */
    void printStats() const;

    // Accesseurs pour la visualisation
    const std::unordered_map<int, MacroAntenna>& getMacroAntennas() const { return m_macroAntennas; }
    const std::unordered_map<int, MicroAntenna>& getMicroAntennas() const { return m_microAntennas; }

private:
    /**
     * @brief Place les grandes antennes selon la densité de véhicules (K-means)
     */
    void placeMacroAntennas(const std::vector<Vehicule*>& vehicles, int numMacro);

    /**
     * @brief Place les petites antennes uniformément dans chaque grande antenne
     */
    void placeMicroAntennas(int microPerMacro);

    /**
     * @brief Calcule les antennes voisines
     */
    void computeNeighborhoods();

    /**
     * @brief Trouve l'antenne la plus proche d'une position
     */
    int findNearestMicroAntenna(double lat, double lon) const;

    /**
     * @brief Calcule la distance géographique entre deux points (en mètres)
     */
    static double distance(double lat1, double lon1, double lat2, double lon2);

private:
    std::unordered_map<int, MacroAntenna> m_macroAntennas;
    std::unordered_map<int, MicroAntenna> m_microAntennas;
    std::unordered_map<int, int> m_vehicleToMicroAntenna;  // vehicleId -> microAntennaId
    
    // Paramètres
    int m_numMacroAntennas;
    int m_microPerMacro;
};

#endif // SPATIAL_GRID_H
