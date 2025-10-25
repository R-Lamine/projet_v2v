#include "vehicule.h"

// ==========================================================
// Constructeur
// ==========================================================
Vehicule::Vehicule(int id, double lat, double lon, double range)
    : id(id), latitude(lat), longitude(lon), transmissionRange(range) {
}

// ==========================================================
// Modifie la position du véhicule
// ==========================================================
void Vehicule::setPosition(double lat, double lon) {
    latitude = lat;
    longitude = lon;
}

// ==========================================================
// Modifie le rayon de transmission
// ==========================================================
void Vehicule::setTransmissionRange(double range) {
    transmissionRange = range;
}

// ==========================================================
// Ajoute un voisin à la liste
// ==========================================================
void Vehicule::addNeighbor(Vehicule* v) {
    neighbors.push_back(v);
}

// ==========================================================
// Efface la liste des voisins (avant recalcul)
// ==========================================================
void Vehicule::clearNeighbors() {
    neighbors.clear();
}
