#include "interference_test.h"
#include "communication_manager.h"
#include <iostream>

using namespace std;

// ==========================================================
// Constructeur : Initialise les véhicules de test
// ==========================================================
InterferenceTest::InterferenceTest() {
    // Véhicule 1 
    vehicles.emplace_back(1, 48.5839, 7.7455, 500.0);
    
    // Véhicule 2
    vehicles.emplace_back(2, 48.5818, 7.7509, 500.0);
    
    // Véhicule 3
    vehicles.emplace_back(3, 48.5847, 7.7345, 400.0);
    
    // Véhicule 4
    vehicles.emplace_back(4, 48.5805, 7.7404, 300.0);
}

// ==========================================================
// Utilitaire : Affiche un en-tête
// ==========================================================
void InterferenceTest::printHeader(const string& title) {
    cout << "\n=== " << title << " ===" << endl;
}

// ==========================================================
// Test 1 : Connexions initiales
// ==========================================================
void InterferenceTest::testInitialConnections() {
    printHeader("Test 1 : Calcul des connexions initiales");
    
    cout << "Nombre de véhicules : " << vehicles.size() << endl;
    
    CommunicationManager commManager(vehicles);
    commManager.updateInterferences();
    commManager.printSummary();
}

// ==========================================================
// Test 2 : Vérification de communication entre véhicules
// ==========================================================
void InterferenceTest::testCommunicationCheck() {
    printHeader("Test 2 : Vérification de communication");
    
    CommunicationManager commManager(vehicles);
    commManager.updateInterferences();
    
    if (commManager.canCommunicate(vehicles[0], vehicles[1])) {
        cout << "✓ Véhicule 1 et 2 PEUVENT communiquer" << endl;
    } else {
        cout << "✗ Véhicule 1 et 2 NE PEUVENT PAS communiquer" << endl;
    }
    
    if (commManager.canCommunicate(vehicles[0], vehicles[2])) {
        cout << "✓ Véhicule 1 et 3 PEUVENT communiquer" << endl;
    } else {
        cout << "✗ Véhicule 1 et 3 NE PEUVENT PAS communiquer" << endl;
    }
}

// ==========================================================
// Test 3 : Accès aux voisins
// ==========================================================
void InterferenceTest::testNeighborAccess() {
    printHeader("Test 3 : Accès direct aux voisins");
    
    CommunicationManager commManager(vehicles);
    commManager.updateInterferences();
    
    // Accès direct depuis le véhicule
    const vector<Vehicule*>& neighbors = vehicles[0].getNeighbors();
    cout << "Véhicule 1 a " << neighbors.size() << " voisin(s) :" << endl;
    for (const Vehicule* neighbor : neighbors) {
        cout << "  - Véhicule " << neighbor->getId() 
             << " (rayon: " << neighbor->getTransmissionRange() << "m)" << endl;
    }
}

// ==========================================================
// Test 4 : Mise à jour dynamique après déplacement
// ==========================================================
void InterferenceTest::testDynamicUpdate() {
    printHeader("Test 4 : Graphe dynamique - Déplacement d'un véhicule");
    
    CommunicationManager commManager(vehicles);
    commManager.updateInterferences();
    
    cout << "État AVANT déplacement :" << endl;
    cout << "  Voisins du véhicule 1 : " << vehicles[0].getNeighbors().size() << endl;
    cout << "  Voisins du véhicule 2 : " << vehicles[1].getNeighbors().size() << endl;
    
    // Déplacement du véhicule 2 vers une position éloignée
    cout << "\n>>> Déplacement du véhicule 2 loin du centre..." << endl;
    vehicles[1].setPosition(48.6000, 7.8000);
    
    // Recalcul du graphe (aspect DYNAMIQUE !)
    commManager.updateInterferences();
    
    cout << "\nÉtat APRÈS déplacement :" << endl;
    cout << "  Voisins du véhicule 1 : " << vehicles[0].getNeighbors().size() << endl;
    cout << "  Voisins du véhicule 2 : " << vehicles[1].getNeighbors().size() << endl;
    
    commManager.printSummary();
}

// ==========================================================
// Lance tous les tests
// ==========================================================
void InterferenceTest::runAllTests() {
    cout << "\n╔═══════════════════════════════════════════════════════╗" << endl;
    cout << "║  Simulation V2V - Tests du Graphe d'Interférences  ║" << endl;
    cout << "╚═══════════════════════════════════════════════════════╝" << endl;
    
    testInitialConnections();
    testCommunicationCheck();
    testNeighborAccess();
    testDynamicUpdate();
    
    cout << "\n✓ Tous les tests terminés !" << endl;
}

