#ifndef INTERFERENCE_GRAPH_TEST_H
#define INTERFERENCE_GRAPH_TEST_H

#include <string>
#include <vector>
#include "vehicule.h"
#include "interference_graph.h"
#include "graph_types.h"

/**
 * @brief Classe de tests unitaires pour le graphe d'interférence
 * 
 * Teste différents scénarios :
 * - Connexions directes
 * - Fermeture transitive (A→B→C)
 * - Edge cases (graphe vide, véhicule isolé, etc.)
 */
class InterferenceGraphTest {
public:
    InterferenceGraphTest();
    ~InterferenceGraphTest();

    /**
     * @brief Lance tous les tests
     * @return true si tous les tests passent, false sinon
     */
    bool runAllTests();

    /**
     * @brief Affiche un rapport détaillé des résultats
     */
    void printReport() const;

private:
    // Tests unitaires individuels
    bool testEmptyGraph();
    bool testSingleVehicle();
    bool testTwoVehiclesInRange();
    bool testTwoVehiclesOutOfRange();
    bool testTransitiveConnection();
    bool testChainOfVehicles();
    bool testDisconnectedGroups();
    bool testAsymmetricRange();
    bool testCompleteGraph();
    bool testStarTopology();

    // Fonctions utilitaires
    void printTestHeader(const std::string& testName) const;
    void printTestResult(const std::string& testName, bool passed);
    bool checkCondition(const std::string& condition, bool result);
    
    // Création de véhicules de test avec positions fixes
    Vehicule* createTestVehicle(int id, double lat, double lon, double range);
    void cleanupVehicles(std::vector<Vehicule*>& vehicles);

private:
    // Statistiques des tests
    int m_totalTests;
    int m_passedTests;
    int m_failedTests;
    
    // Graphe de test (simple, non routier)
    RoadGraph m_testGraph;
    
    // Stockage des résultats détaillés
    std::vector<std::pair<std::string, bool>> m_testResults;
};

#endif // INTERFERENCE_GRAPH_TEST_H
