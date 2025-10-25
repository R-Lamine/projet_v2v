#include "communication_manager.h"
#include <iostream>
#include <cmath>

// Rayon de la Terre en mètres (pour le calcul de distance)
const double EARTH_RADIUS = 6371000.0;

// ==========================================================
// Constructeur
// ==========================================================
CommunicationManager::CommunicationManager(std::vector<Vehicule>& v) 
    : vehicles(v) {
}

// ==========================================================
// Calcule la distance entre deux véhicules (formule haversine)
// ==========================================================
double CommunicationManager::calculateDistance(const Vehicule& v1, const Vehicule& v2) const {
    // Récupère les coordonnées
    double lat1 = v1.getLatitude();
    double lon1 = v1.getLongitude();
    double lat2 = v2.getLatitude();
    double lon2 = v2.getLongitude();

    // Conversion en radians
    double lat1_rad = lat1 * M_PI / 180.0;
    double lat2_rad = lat2 * M_PI / 180.0;
    double delta_lat = (lat2 - lat1) * M_PI / 180.0;
    double delta_lon = (lon2 - lon1) * M_PI / 180.0;

    // Formule de haversine pour calculer la distance sur une sphère
    double a = std::sin(delta_lat / 2.0) * std::sin(delta_lat / 2.0) +
               std::cos(lat1_rad) * std::cos(lat2_rad) *
               std::sin(delta_lon / 2.0) * std::sin(delta_lon / 2.0);
    
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    
    // Distance en mètres
    return EARTH_RADIUS * c;
}

// ==========================================================
// Met à jour les connexions entre véhicules
// ==========================================================
void CommunicationManager::updateInterferences() {
    // Vide les listes de voisins de chaque véhicule
    for (Vehicule& v : vehicles) {
        v.clearNeighbors();
    }

    int totalConnections = 0;

    // Pour chaque paire de véhicules
    for (size_t i = 0; i < vehicles.size(); ++i) {
        for (size_t j = i + 1; j < vehicles.size(); ++j) {
            Vehicule& v1 = vehicles[i];
            Vehicule& v2 = vehicles[j];

            // Calcule la distance entre les deux véhicules
            double distance = calculateDistance(v1, v2);

            // Vérifie si ils peuvent communiquer
            // Condition : distance <= min des deux rayons de transmission
            double minRange = std::min(v1.getTransmissionRange(), 
                                      v2.getTransmissionRange());

            if (distance <= minRange) {
                // Ajoute la connexion dans les deux sens
                v1.addNeighbor(&v2);
                v2.addNeighbor(&v1);
                totalConnections++;
            }
        }
    }

    std::cout << "Connexions mises à jour : " 
              << totalConnections << " connexion(s) établie(s)" << std::endl;
}

// ==========================================================
// Retourne les voisins (véhicules en portée) d'un véhicule
// ==========================================================
std::vector<Vehicule*> CommunicationManager::getNeighbors(const Vehicule& v) {
    // Retourne directement la liste stockée dans le véhicule
    return v.getNeighbors();
}

// ==========================================================
// Vérifie si deux véhicules peuvent communiquer
// ==========================================================
bool CommunicationManager::canCommunicate(const Vehicule& v1, const Vehicule& v2) {
    // Cherche v2 dans la liste des voisins de v1
    const std::vector<Vehicule*>& neighbors = v1.getNeighbors();
    
    for (const Vehicule* neighbor : neighbors) {
        if (neighbor->getId() == v2.getId()) {
            return true;
        }
    }
    
    return false;
}

// ==========================================================
// Affiche un résumé des connexions
// ==========================================================
void CommunicationManager::printSummary() const {
    std::cout << "\n=== Résumé des communications V2V ===" << std::endl;
    std::cout << "Nombre de véhicules : " << vehicles.size() << std::endl;
    
    int totalConnections = 0;
    int connectedVehicles = 0;
    
    for (const Vehicule& v : vehicles) {
        size_t neighborCount = v.getNeighbors().size();
        if (neighborCount > 0) {
            connectedVehicles++;
            totalConnections += neighborCount;
        }
    }
    
    // Divisé par 2 car chaque connexion est comptée deux fois
    std::cout << "Nombre de véhicules connectés : " << connectedVehicles << std::endl;
    std::cout << "Nombre total de connexions : " << totalConnections / 2 << std::endl;
    
    // Détail par véhicule
    std::cout << "\nDétail des connexions :" << std::endl;
    for (const Vehicule& v : vehicles) {
        const std::vector<Vehicule*>& neighbors = v.getNeighbors();
        if (neighbors.size() > 0) {
            std::cout << "  Véhicule " << v.getId() << " : " 
                      << neighbors.size() << " voisin(s)" << std::endl;
        }
    }
}
