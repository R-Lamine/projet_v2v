#pragma once
#include <vector>
#include "vehicule.h"

// Classe responsable de gérer les communications entre véhicules
// Met à jour dynamiquement qui peut communiquer avec qui
class CommunicationManager {
public:
    // Constructeur : reçoit la liste des véhicules à gérer
    CommunicationManager(std::vector<Vehicule>& vehicles);

    // Recalcule toutes les connexions (à appeler après chaque déplacement)
    void updateInterferences();

    // Retourne tous les véhicules à portée de transmission d'un véhicule donné
    std::vector<Vehicule*> getNeighbors(const Vehicule& v);

    // Vérifie si deux véhicules peuvent communiquer entre eux
    bool canCommunicate(const Vehicule& v1, const Vehicule& v2);

    // Affiche un résumé des connexions
    void printSummary() const;

private:
    // Référence vers la liste des véhicules
    std::vector<Vehicule>& vehicles;

    // Calcule la distance géographique entre deux véhicules (en mètres)
    double calculateDistance(const Vehicule& v1, const Vehicule& v2) const;
};
