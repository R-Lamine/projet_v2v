#include "spatial_grid.h"
#include "vehicule.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SpatialGrid::SpatialGrid() 
    : m_numMacroAntennas(0), m_microPerMacro(0) {}

SpatialGrid::~SpatialGrid() {
    clear();
}

void SpatialGrid::clear() {
    m_macroAntennas.clear();
    m_microAntennas.clear();
    m_vehicleToMicroAntenna.clear();
}

double SpatialGrid::distance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371000.0; // Rayon de la Terre en mètres
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
               std::cos(lat1 * M_PI / 180.0) * std::cos(lat2 * M_PI / 180.0) *
               std::sin(dLon / 2) * std::sin(dLon / 2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    return R * c;
}

void SpatialGrid::initialize(const std::vector<Vehicule*>& vehicles, 
                             int numMacroAntennas, 
                             int microPerMacro) {
    clear();
    
    if (vehicles.empty()) {
        std::cout << "Aucun véhicule pour initialiser la grille" << std::endl;
        return;
    }
    
    m_numMacroAntennas = numMacroAntennas;
    m_microPerMacro = microPerMacro;
    
    // Afficher seulement pour debug (première fois)
    static bool firstTime = true;
    if (firstTime) {
        std::cout << "\n=== Initialisation de la grille spatiale ===" << std::endl;
        std::cout << "Véhicules: " << vehicles.size() << std::endl;
        std::cout << "Grandes antennes: " << numMacroAntennas << std::endl;
        std::cout << "Petites antennes par grande: " << microPerMacro << std::endl;
        firstTime = false;
    }
    
    // Étape 1: Placer les grandes antennes selon la densité
    placeMacroAntennas(vehicles, numMacroAntennas);
    
    // Étape 2: Placer les petites antennes uniformément dans chaque grande
    placeMicroAntennas(microPerMacro);
    
    // Étape 3: Calculer les voisinages
    computeNeighborhoods();
    
    // Étape 4: Assigner les véhicules
    assignVehiclesToAntennas(vehicles);
    
    // Ne plus afficher les stats à chaque fois (trop verbeux)
    // printStats();
}

void SpatialGrid::placeMacroAntennas(const std::vector<Vehicule*>& vehicles, int numMacro) {
    // Algorithme K-means simplifié pour placer les antennes selon la densité
    
    // Calculer les limites de la zone
    double minLat = std::numeric_limits<double>::max();
    double maxLat = std::numeric_limits<double>::lowest();
    double minLon = std::numeric_limits<double>::max();
    double maxLon = std::numeric_limits<double>::lowest();
    
    std::vector<std::pair<double, double>> positions;
    
    // OPTIMISATION: Pour beaucoup de véhicules, échantillonner seulement 1000 positions
    int sampleSize = std::min(1000, (int)vehicles.size());
    int step = vehicles.size() / sampleSize;
    if (step < 1) step = 1;
    
    for (size_t i = 0; i < vehicles.size(); i += step) {
        auto* v = vehicles[i];
        if (!v) continue;
        auto [lat, lon] = v->getPosition();
        positions.push_back({lat, lon});
        
        minLat = std::min(minLat, lat);
        maxLat = std::max(maxLat, lat);
        minLon = std::min(minLon, lon);
        maxLon = std::max(maxLon, lon);
    }
    
    // Initialiser les centres uniformément dans la zone
    std::vector<std::pair<double, double>> centers;
    int sqrtNum = (int)std::ceil(std::sqrt(numMacro));
    for (int i = 0; i < numMacro; ++i) {
        int row = i / sqrtNum;
        int col = i % sqrtNum;
        double lat = minLat + (maxLat - minLat) * row / (sqrtNum - 1.0);
        double lon = minLon + (maxLon - minLon) * col / (sqrtNum - 1.0);
        centers.push_back({lat, lon});
    }
    
    // K-means: seulement 3 itérations pour la vitesse
    for (int iter = 0; iter < 3; ++iter) {
        // Assigner chaque véhicule au centre le plus proche
        std::vector<std::vector<std::pair<double, double>>> clusters(numMacro);
        
        for (const auto& [lat, lon] : positions) {
            int nearestCenter = 0;
            double minDist = std::numeric_limits<double>::max();
            
            for (int i = 0; i < numMacro; ++i) {
                double dist = distance(lat, lon, centers[i].first, centers[i].second);
                if (dist < minDist) {
                    minDist = dist;
                    nearestCenter = i;
                }
            }
            
            clusters[nearestCenter].push_back({lat, lon});
        }
        
        // Recalculer les centres (moyenne des positions)
        for (int i = 0; i < numMacro; ++i) {
            if (clusters[i].empty()) continue;
            
            double sumLat = 0.0, sumLon = 0.0;
            for (const auto& [lat, lon] : clusters[i]) {
                sumLat += lat;
                sumLon += lon;
            }
            centers[i] = {sumLat / clusters[i].size(), sumLon / clusters[i].size()};
        }
    }
    
    // Créer les grandes antennes aux centres calculés
    double avgRadius = distance(minLat, minLon, maxLat, maxLon) / (2.0 * std::sqrt(numMacro));
    
    for (int i = 0; i < numMacro; ++i) {
        MacroAntenna macro;
        macro.id = i;
        macro.centerLat = centers[i].first;
        macro.centerLon = centers[i].second;
        macro.radius = avgRadius * 1.5; // 50% de marge
        
        m_macroAntennas[i] = macro;
    }
}

void SpatialGrid::placeMicroAntennas(int microPerMacro) {
    int microId = 0;
    
    for (auto& [macroId, macro] : m_macroAntennas) {
        // Placer les petites antennes uniformément dans un cercle
        int itemsPerRow = (int)std::ceil(std::sqrt(microPerMacro));
        
        for (int i = 0; i < microPerMacro; ++i) {
            int row = i / itemsPerRow;
            int col = i % itemsPerRow;
            
            // Calculer l'offset par rapport au centre de la grande antenne
            double offsetLat = (row - itemsPerRow / 2.0) * (macro.radius / itemsPerRow) / 111000.0;
            double offsetLon = (col - itemsPerRow / 2.0) * (macro.radius / itemsPerRow) / 111000.0;
            
            MicroAntenna micro;
            micro.id = microId;
            micro.macroAntennaId = macroId;
            micro.centerLat = macro.centerLat + offsetLat;
            micro.centerLon = macro.centerLon + offsetLon;
            micro.radius = macro.radius / (2.0 * itemsPerRow);
            
            m_microAntennas[microId] = micro;
            macro.microAntennaIds.push_back(microId);
            
            microId++;
        }
    }
}

void SpatialGrid::computeNeighborhoods() {
    // Calculer les voisinages entre grandes antennes
    for (auto& [id1, macro1] : m_macroAntennas) {
        for (auto& [id2, macro2] : m_macroAntennas) {
            if (id1 >= id2) continue;
            
            double dist = distance(macro1.centerLat, macro1.centerLon, 
                                 macro2.centerLat, macro2.centerLon);
            
            // Voisines si distance < somme des rayons * 1.5 (marge)
            if (dist < (macro1.radius + macro2.radius) * 1.5) {
                macro1.neighborMacroIds.insert(id2);
                macro2.neighborMacroIds.insert(id1);
            }
        }
    }
    
    // OPTIMISATION CRITIQUE: Pour chaque micro, ne prendre QUE les micros 
    // où les véhicules peuvent potentiellement communiquer.
    // Distance max entre centres = portée transmission + 2 × rayon micro
    
    for (auto& [id1, micro1] : m_microAntennas) {
        for (auto& [id2, micro2] : m_microAntennas) {
            if (id1 >= id2) continue;
            
            // Calculer la distance entre les centres des micros
            double dist = distance(micro1.centerLat, micro1.centerLon, 
                                 micro2.centerLat, micro2.centerLon);
            
            // Voisines SEULEMENT si un véhicule de micro1 peut communiquer avec un de micro2
            // Distance max = portée + rayon1 + rayon2 (cas extrême: véhicules aux bords opposés)
            double maxCommDistance = m_maxTransmissionRange + micro1.radius + micro2.radius;
            
            if (dist < maxCommDistance) {
                micro1.neighborMicroIds.insert(id2);
                micro2.neighborMicroIds.insert(id1);
            }
        }
    }
}

void SpatialGrid::assignVehiclesToAntennas(const std::vector<Vehicule*>& vehicles) {
    // Effacer les anciennes assignations
    m_vehicleToMicroAntenna.clear();
    for (auto& [id, micro] : m_microAntennas) {
        micro.vehicleIds.clear();
    }
    
    // Assigner chaque véhicule à la petite antenne la plus proche
    for (auto* v : vehicles) {
        if (!v) continue;
        
        auto [lat, lon] = v->getPosition();
        int nearestMicro = findNearestMicroAntenna(lat, lon);
        
        if (nearestMicro >= 0) {
            m_vehicleToMicroAntenna[v->getId()] = nearestMicro;
            m_microAntennas[nearestMicro].vehicleIds.push_back(v->getId());
        }
    }
}

void SpatialGrid::assignVehicleToAntenna(Vehicule* vehicle) {
    if (!vehicle) return;
    if (m_microAntennas.empty()) return;  // Grille pas encore initialisée
    
    auto [lat, lon] = vehicle->getPosition();
    int nearestMicro = findNearestMicroAntenna(lat, lon);
    
    if (nearestMicro >= 0) {
        m_vehicleToMicroAntenna[vehicle->getId()] = nearestMicro;
        m_microAntennas[nearestMicro].vehicleIds.push_back(vehicle->getId());
    }
}

void SpatialGrid::removeVehicleFromAntenna(int vehicleId) {
    auto it = m_vehicleToMicroAntenna.find(vehicleId);
    if (it == m_vehicleToMicroAntenna.end()) return;
    
    int microId = it->second;
    m_vehicleToMicroAntenna.erase(it);
    
    // Retirer le véhicule de la liste de l'antenne
    auto microIt = m_microAntennas.find(microId);
    if (microIt != m_microAntennas.end()) {
        auto& vehicleIds = microIt->second.vehicleIds;
        vehicleIds.erase(std::remove(vehicleIds.begin(), vehicleIds.end(), vehicleId), vehicleIds.end());
    }
}

int SpatialGrid::findNearestMicroAntenna(double lat, double lon) const {
    // OPTIMISATION: D'abord trouver la macro antenne la plus proche (30 comparaisons)
    // Puis chercher dans ses micros (20 comparaisons) = 50 au lieu de 600!
    
    int nearestMacro = -1;
    double minMacroDist = std::numeric_limits<double>::max();
    
    for (const auto& [id, macro] : m_macroAntennas) {
        double dist = distance(lat, lon, macro.centerLat, macro.centerLon);
        if (dist < minMacroDist) {
            minMacroDist = dist;
            nearestMacro = id;
        }
    }
    
    if (nearestMacro < 0) return -1;
    
    // Chercher la micro antenne la plus proche dans cette macro (et macros voisines)
    int nearest = -1;
    double minDist = std::numeric_limits<double>::max();
    
    // Micros de la macro la plus proche
    for (int microId : m_macroAntennas.at(nearestMacro).microAntennaIds) {
        const auto& micro = m_microAntennas.at(microId);
        double dist = distance(lat, lon, micro.centerLat, micro.centerLon);
        if (dist < minDist) {
            minDist = dist;
            nearest = microId;
        }
    }
    
    // Aussi checker les micros des macros voisines (au cas où on est sur la bordure)
    for (int neighborMacroId : m_macroAntennas.at(nearestMacro).neighborMacroIds) {
        for (int microId : m_macroAntennas.at(neighborMacroId).microAntennaIds) {
            const auto& micro = m_microAntennas.at(microId);
            double dist = distance(lat, lon, micro.centerLat, micro.centerLon);
            if (dist < minDist) {
                minDist = dist;
                nearest = microId;
            }
        }
    }
    
    return nearest;
}

std::vector<int> SpatialGrid::getNearbyVehicles(int vehicleId) const {
    std::vector<int> nearby;
    
    auto it = m_vehicleToMicroAntenna.find(vehicleId);
    if (it == m_vehicleToMicroAntenna.end()) {
        return nearby; // Véhicule non trouvé
    }
    
    int microId = it->second;
    const auto& micro = m_microAntennas.at(microId);
    
    // Ajouter les véhicules de la même antenne
    for (int vId : micro.vehicleIds) {
        if (vId != vehicleId) {
            nearby.push_back(vId);
        }
    }
    
    // Ajouter les véhicules des antennes voisines
    for (int neighborId : micro.neighborMicroIds) {
        const auto& neighborMicro = m_microAntennas.at(neighborId);
        for (int vId : neighborMicro.vehicleIds) {
            nearby.push_back(vId);
        }
    }
    
    return nearby;
}

int SpatialGrid::getMicroAntennaId(int vehicleId) const {
    auto it = m_vehicleToMicroAntenna.find(vehicleId);
    return (it != m_vehicleToMicroAntenna.end()) ? it->second : -1;
}

int SpatialGrid::getMacroAntennaId(int vehicleId) const {
    int microId = getMicroAntennaId(vehicleId);
    if (microId < 0) return -1;
    
    return m_microAntennas.at(microId).macroAntennaId;
}

void SpatialGrid::printStats() const {
    std::cout << "\n=== Statistiques de la grille spatiale ===" << std::endl;
    std::cout << "Grandes antennes: " << m_macroAntennas.size() << std::endl;
    std::cout << "Petites antennes: " << m_microAntennas.size() << std::endl;
    std::cout << "Véhicules assignés: " << m_vehicleToMicroAntenna.size() << std::endl;
    
    // Statistiques par grande antenne
    for (const auto& [macroId, macro] : m_macroAntennas) {
        int totalVehicles = 0;
        for (int microId : macro.microAntennaIds) {
            totalVehicles += m_microAntennas.at(microId).vehicleIds.size();
        }
        std::cout << "Grande antenne " << macroId << ": " 
                  << macro.microAntennaIds.size() << " petites antennes, "
                  << totalVehicles << " véhicules" << std::endl;
    }
    std::cout << "========================================\n" << std::endl;
}

void SpatialGrid::updateNeighborhoods() {
    // Recalculer les voisinages entre antennes avec la nouvelle portée
    // D'abord effacer les anciens voisinages des micro antennes
    for (auto& [id, micro] : m_microAntennas) {
        micro.neighborMicroIds.clear();
    }
    
    // Recalculer seulement les voisinages des micro antennes
    // (les macro antennes ne dépendent pas de la portée de transmission)
    for (auto& [id1, micro1] : m_microAntennas) {
        for (auto& [id2, micro2] : m_microAntennas) {
            if (id1 >= id2) continue;
            
            double dist = distance(micro1.centerLat, micro1.centerLon, 
                                 micro2.centerLat, micro2.centerLon);
            
            double maxCommDistance = m_maxTransmissionRange + micro1.radius + micro2.radius;
            
            if (dist < maxCommDistance) {
                micro1.neighborMicroIds.insert(id2);
                micro2.neighborMicroIds.insert(id1);
            }
        }
    }
}