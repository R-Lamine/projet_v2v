#pragma once
#include <vector>

// Classe représentant un véhicule dans la simulation V2V
class Vehicule {
public:
    // Constructeur
    Vehicule(int id, double lat, double lon, double range);

    // Getters
    int getId() const { return id; }
    double getLatitude() const { return latitude; }
    double getLongitude() const { return longitude; }
    double getTransmissionRange() const { return transmissionRange; }
    
    // Gestion des voisins (véhicules en portée de communication)
    const std::vector<Vehicule*>& getNeighbors() const { return neighbors; }
    void addNeighbor(Vehicule* v);
    void clearNeighbors();

    // Setters (pour déplacer le véhicule)
    void setPosition(double lat, double lon);
    void setTransmissionRange(double range);

private:
    int id;                           // Identifiant unique du véhicule
    double latitude;                  // Position latitude
    double longitude;                 // Position longitude
    double transmissionRange;         // Rayon de transmission en mètres (100-500m)
    std::vector<Vehicule*> neighbors; // Liste des véhicules en portée de communication
};
