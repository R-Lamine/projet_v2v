#pragma once
#include "vehicule.h"
#include "communication_manager.h"
#include <vector>
#include <string>

// Classe pour tester le système de communication V2V
class InterferenceTest {
public:
    // Constructeur : crée des véhicules de test
    InterferenceTest();

    // Lance tous les tests
    void runAllTests();

private:
    std::vector<Vehicule> vehicles;
    
    // Tests individuels
    void testInitialConnections();
    void testCommunicationCheck();
    void testNeighborAccess();
    void testDynamicUpdate();
    
    // Utilitaire
    void printHeader(const std::string& title);
};
