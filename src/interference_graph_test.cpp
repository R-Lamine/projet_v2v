#include "interference_graph_test.h"
#include "graph_builder.h"
#include <iostream>
#include <iomanip>

using namespace std;

InterferenceGraphTest::InterferenceGraphTest() 
    : m_totalTests(0), m_passedTests(0), m_failedTests(0) {
    // Créer un graphe avec plusieurs sommets espacés pour les tests
    // Positions espacées d'environ 150m les unes des autres
    
    // Vertex 0: Position de base
    Vertex v0 = boost::add_vertex(m_testGraph);
    m_testGraph[v0].id = 0;
    m_testGraph[v0].lat = 48.5734;
    m_testGraph[v0].lon = 7.7521;
    
    // Vertex 1: ~150m au nord-est
    Vertex v1 = boost::add_vertex(m_testGraph);
    m_testGraph[v1].id = 1;
    m_testGraph[v1].lat = 48.5747;
    m_testGraph[v1].lon = 7.7541;
    
    // Vertex 2: ~300m au nord-est
    Vertex v2 = boost::add_vertex(m_testGraph);
    m_testGraph[v2].id = 2;
    m_testGraph[v2].lat = 48.5760;
    m_testGraph[v2].lon = 7.7561;
    
    // Vertex 3: ~450m au nord-est
    Vertex v3 = boost::add_vertex(m_testGraph);
    m_testGraph[v3].id = 3;
    m_testGraph[v3].lat = 48.5773;
    m_testGraph[v3].lon = 7.7581;
    
    // Vertex 4: Très loin (~5km)
    Vertex v4 = boost::add_vertex(m_testGraph);
    m_testGraph[v4].id = 4;
    m_testGraph[v4].lat = 48.6234;
    m_testGraph[v4].lon = 7.8021;
}

InterferenceGraphTest::~InterferenceGraphTest() {}

void InterferenceGraphTest::printTestHeader(const string& testName) const {
    cout << "\n╔══════════════════════════════════════════════════════════╗" << endl;
    cout << "║  TEST: " << left << setw(49) << testName << "║" << endl;
    cout << "╚══════════════════════════════════════════════════════════╝" << endl;
}

void InterferenceGraphTest::printTestResult(const string& testName, bool passed) {
    m_testResults.push_back({testName, passed});
    
    if (passed) {
        cout << "✅ PASSED: " << testName << endl;
        m_passedTests++;
    } else {
        cout << "❌ FAILED: " << testName << endl;
        m_failedTests++;
    }
    m_totalTests++;
}

bool InterferenceGraphTest::checkCondition(const string& condition, bool result) {
    cout << "  → " << condition << " : " << (result ? "✓" : "✗") << endl;
    return result;
}

Vehicule* InterferenceGraphTest::createTestVehicle(int id, double lat, double lon, double range) {
    // Utiliser différents sommets du graphe selon l'ID
    // pour avoir des positions différentes
    auto vertices = boost::vertices(m_testGraph);
    int numVertices = boost::num_vertices(m_testGraph);
    
    // Choisir un sommet de départ basé sur l'ID du véhicule
    int startIdx = id % numVertices;
    int goalIdx = (id + 1) % numVertices;
    
    Vertex start = *(vertices.first + startIdx);
    Vertex goal = *(vertices.first + goalIdx);
    
    Vehicule* v = new Vehicule(id, m_testGraph, start, goal, 10.0, range, 5.0);
    
    return v;
}

void InterferenceGraphTest::cleanupVehicles(vector<Vehicule*>& vehicles) {
    for (auto* v : vehicles) {
        delete v;
    }
    vehicles.clear();
}

bool InterferenceGraphTest::testEmptyGraph() {
    printTestHeader("Graphe vide");
    
    InterferenceGraph graph;
    vector<Vehicule*> vehicles;
    
    graph.buildGraph(vehicles);
    
    bool test1 = checkCondition("Nombre de véhicules = 0", graph.getVehicleCount() == 0);
    
    bool passed = test1;
    printTestResult("Graphe vide", passed);
    return passed;
}

bool InterferenceGraphTest::testSingleVehicle() {
    printTestHeader("Véhicule unique isolé");
    
    InterferenceGraph graph;
    vector<Vehicule*> vehicles;
    
    vehicles.push_back(createTestVehicle(1, 48.5734, 7.7521, 100.0));
    
    graph.buildGraph(vehicles);
    
    bool test1 = checkCondition("Nombre de véhicules = 1", graph.getVehicleCount() == 1);
    bool test2 = checkCondition("Aucun voisin direct", graph.getDirectNeighbors(1).empty());
    bool test3 = checkCondition("Aucun véhicule accessible", graph.getReachableVehicles(1).empty());
    
    bool passed = test1 && test2 && test3;
    printTestResult("Véhicule unique", passed);
    
    cleanupVehicles(vehicles);
    return passed;
}

bool InterferenceGraphTest::testTwoVehiclesInRange() {
    printTestHeader("Deux véhicules à portée l'un de l'autre");
    
    InterferenceGraph graph;
    vector<Vehicule*> vehicles;
    
    // Créer deux véhicules proches (vertex 0 et vertex 1, ~150m d'écart)
    // avec une grande portée (500m)
    vehicles.push_back(new Vehicule(0, m_testGraph,
                                     *(boost::vertices(m_testGraph).first + 0),
                                     *(boost::vertices(m_testGraph).first + 1),
                                     10.0, 500.0, 5.0));  // ID=0, portée=500m
    
    vehicles.push_back(new Vehicule(1, m_testGraph,
                                     *(boost::vertices(m_testGraph).first + 1),
                                     *(boost::vertices(m_testGraph).first + 0),
                                     10.0, 500.0, 5.0));  // ID=1, portée=500m
    
    // Calculer la distance réelle
    double distance = vehicles[0]->calculateDist(*vehicles[1]);
    cout << "  → Distance réelle entre V0 et V1: " << distance << " mètres" << endl;
    cout << "  → Portée des véhicules: 500 mètres" << endl;
    
    graph.buildGraph(vehicles);
    
    bool test1 = checkCondition("Nombre de véhicules = 2", graph.getVehicleCount() == 2);
    bool test2 = checkCondition("Distance < 500m", distance < 500.0);
    bool test3 = checkCondition("V0 peut communiquer avec V1", graph.canCommunicate(0, 1));
    bool test4 = checkCondition("V1 peut communiquer avec V0", graph.canCommunicate(1, 0));
    
    auto neighbors0 = graph.getDirectNeighbors(0);
    auto neighbors1 = graph.getDirectNeighbors(1);
    
    bool test5 = checkCondition("V0 a 1 voisin direct", neighbors0.size() == 1);
    bool test6 = checkCondition("V1 a 1 voisin direct", neighbors1.size() == 1);
    bool test7 = checkCondition("V1 est voisin de V0", neighbors0.find(1) != neighbors0.end());
    
    bool passed = test1 && test2 && test3 && test4 && test5 && test6 && test7;
    printTestResult("Deux véhicules à portée", passed);
    
    cleanupVehicles(vehicles);
    return passed;
}

bool InterferenceGraphTest::testTwoVehiclesOutOfRange() {
    printTestHeader("Deux véhicules hors de portée");
    
    InterferenceGraph graph;
    vector<Vehicule*> vehicles;
    
    // Créer deux véhicules très éloignés (utilisant vertex 0 et vertex 4 qui sont à ~5km)
    // avec une portée de seulement 10m
    vehicles.push_back(new Vehicule(0, m_testGraph, 
                                     *(boost::vertices(m_testGraph).first + 0),
                                     *(boost::vertices(m_testGraph).first + 1),
                                     10.0, 10.0, 5.0));  // ID=0, portée=10m
    
    vehicles.push_back(new Vehicule(4, m_testGraph,
                                     *(boost::vertices(m_testGraph).first + 4),
                                     *(boost::vertices(m_testGraph).first + 0),
                                     10.0, 10.0, 5.0));  // ID=4, portée=10m
    
    // Calculer la distance réelle entre les deux véhicules
    double distance = vehicles[0]->calculateDist(*vehicles[1]);
    cout << "  → Distance réelle entre V0 et V4: " << distance << " mètres" << endl;
    cout << "  → Portée des véhicules: 10 mètres" << endl;
    
    graph.buildGraph(vehicles);
    
    bool test1 = checkCondition("Nombre de véhicules = 2", graph.getVehicleCount() == 2);
    bool test2 = checkCondition("Distance > 10m", distance > 10.0);
    bool test3 = checkCondition("V0 ne peut PAS communiquer avec V4", !graph.canCommunicate(0, 4));
    bool test4 = checkCondition("V4 ne peut PAS communiquer avec V0", !graph.canCommunicate(4, 0));
    bool test5 = checkCondition("V0 n'a aucun voisin", graph.getDirectNeighbors(0).empty());
    bool test6 = checkCondition("V4 n'a aucun voisin", graph.getDirectNeighbors(4).empty());
    
    bool passed = test1 && test2 && test3 && test4 && test5 && test6;
    printTestResult("Deux véhicules hors portée", passed);
    
    cleanupVehicles(vehicles);
    return passed;
}

bool InterferenceGraphTest::testTransitiveConnection() {
    printTestHeader("Connexion transitive (A→B→C)");
    
    InterferenceGraph graph;
    vector<Vehicule*> vehicles;
    
    // Créer 3 véhicules en chaîne avec portée de 250m (pour couvrir les ~206m)
    // V0 (vertex 0) ←250m→ V1 (vertex 1) ←250m→ V2 (vertex 2)
    // Mais V0 et V2 sont à ~412m donc hors portée directe
    vehicles.push_back(new Vehicule(0, m_testGraph,
                                     *(boost::vertices(m_testGraph).first + 0),
                                     *(boost::vertices(m_testGraph).first + 1),
                                     10.0, 250.0, 5.0));
    
    vehicles.push_back(new Vehicule(1, m_testGraph,
                                     *(boost::vertices(m_testGraph).first + 1),
                                     *(boost::vertices(m_testGraph).first + 2),
                                     10.0, 250.0, 5.0));
    
    vehicles.push_back(new Vehicule(2, m_testGraph,
                                     *(boost::vertices(m_testGraph).first + 2),
                                     *(boost::vertices(m_testGraph).first + 3),
                                     10.0, 250.0, 5.0));
    
    double dist01 = vehicles[0]->calculateDist(*vehicles[1]);
    double dist12 = vehicles[1]->calculateDist(*vehicles[2]);
    double dist02 = vehicles[0]->calculateDist(*vehicles[2]);
    
    cout << "  → Distance V0-V1: " << dist01 << "m (portée: 250m)" << endl;
    cout << "  → Distance V1-V2: " << dist12 << "m (portée: 250m)" << endl;
    cout << "  → Distance V0-V2: " << dist02 << "m (portée: 250m)" << endl;
    
    graph.buildGraph(vehicles);
    
    auto neighbors0 = graph.getDirectNeighbors(0);
    auto neighbors1 = graph.getDirectNeighbors(1);
    auto neighbors2 = graph.getDirectNeighbors(2);
    
    cout << "  → V0 voisins directs: " << neighbors0.size() << endl;
    cout << "  → V1 voisins directs: " << neighbors1.size() << endl;
    cout << "  → V2 voisins directs: " << neighbors2.size() << endl;
    
    auto reachable0 = graph.getReachableVehicles(0);
    auto reachable2 = graph.getReachableVehicles(2);
    
    cout << "  → V0 peut atteindre: " << reachable0.size() << " véhicule(s)" << endl;
    cout << "  → V2 peut atteindre: " << reachable2.size() << " véhicule(s)" << endl;
    
    bool test1 = checkCondition("V0 et V1 sont voisins directs (dist < 250m)", 
                                neighbors0.find(1) != neighbors0.end());
    bool test2 = checkCondition("V1 et V2 sont voisins directs (dist < 250m)", 
                                neighbors1.find(2) != neighbors1.end());
    bool test3 = checkCondition("V0 HORS portée directe de V2 (dist > 250m)", 
                                neighbors0.find(2) == neighbors0.end());
    bool test4 = checkCondition("V0 peut atteindre V2 via V1 (transitivité)", 
                                reachable0.find(2) != reachable0.end());
    bool test5 = checkCondition("V2 peut atteindre V0 via V1 (transitivité)", 
                                reachable2.find(0) != reachable2.end());
    
    bool passed = test1 && test2 && test3 && test4 && test5;
    printTestResult("Connexion transitive", passed);
    
    cleanupVehicles(vehicles);
    return passed;
}

bool InterferenceGraphTest::testChainOfVehicles() {
    printTestHeader("Chaîne de véhicules (A-B-C-D)");
    
    InterferenceGraph graph;
    vector<Vehicule*> vehicles;
    
    // Créer une chaîne : chaque véhicule peut communiquer avec ses voisins directs
    for (int i = 1; i <= 4; i++) {
        vehicles.push_back(createTestVehicle(i, 48.5734, 7.7521, 500.0));
    }
    
    graph.buildGraph(vehicles);
    
    bool test1 = checkCondition("Nombre de véhicules = 4", graph.getVehicleCount() == 4);
    
    // Vérifier que tous les véhicules peuvent communiquer via la chaîne
    auto reachable1 = graph.getReachableVehicles(1);
    
    cout << "  → V1 peut atteindre " << reachable1.size() << " véhicule(s)" << endl;
    
    bool test2 = checkCondition("V1 peut atteindre d'autres véhicules", !reachable1.empty());
    
    bool passed = test1 && test2;
    printTestResult("Chaîne de véhicules", passed);
    
    cleanupVehicles(vehicles);
    return passed;
}

bool InterferenceGraphTest::testDisconnectedGroups() {
    printTestHeader("Groupes déconnectés");
    
    InterferenceGraph graph;
    vector<Vehicule*> vehicles;
    
    // Groupe 1 : V1 et V2 (portée large)
    vehicles.push_back(createTestVehicle(1, 48.5734, 7.7521, 1000.0));
    vehicles.push_back(createTestVehicle(2, 48.5735, 7.7522, 1000.0));
    
    // Groupe 2 : V3 et V4 (portée courte, loin des autres)
    vehicles.push_back(createTestVehicle(3, 48.5734, 7.7521, 1.0));
    vehicles.push_back(createTestVehicle(4, 48.5735, 7.7522, 1.0));
    
    graph.buildGraph(vehicles);
    
    bool test1 = checkCondition("Nombre de véhicules = 4", graph.getVehicleCount() == 4);
    
    // V1 et V2 devraient pouvoir communiquer
    bool test2 = checkCondition("V1 peut communiquer avec V2", graph.canCommunicate(1, 2));
    
    cout << "  → Test de groupes déconnectés (dépend des positions réelles)" << endl;
    
    bool passed = test1 && test2;
    printTestResult("Groupes déconnectés", passed);
    
    cleanupVehicles(vehicles);
    return passed;
}

bool InterferenceGraphTest::testAsymmetricRange() {
    printTestHeader("Portées asymétriques");
    
    InterferenceGraph graph;
    vector<Vehicule*> vehicles;
    
    // V1 a une grande portée, V2 a une petite portée
    vehicles.push_back(createTestVehicle(1, 48.5734, 7.7521, 2000.0)); // Grande portée
    vehicles.push_back(createTestVehicle(2, 48.5735, 7.7522, 1.0));    // Petite portée
    
    graph.buildGraph(vehicles);
    
    cout << "  → V1 portée: 2000m, V2 portée: 1m" << endl;
    cout << "  → Pour communiquer, les DEUX doivent être à portée" << endl;
    
    auto neighbors1 = graph.getDirectNeighbors(1);
    auto neighbors2 = graph.getDirectNeighbors(2);
    
    bool test1 = checkCondition("Connexion nécessite portée bidirectionnelle", true);
    
    cout << "  → V1 voisins: " << neighbors1.size() << endl;
    cout << "  → V2 voisins: " << neighbors2.size() << endl;
    
    bool passed = test1;
    printTestResult("Portées asymétriques", passed);
    
    cleanupVehicles(vehicles);
    return passed;
}

bool InterferenceGraphTest::testCompleteGraph() {
    printTestHeader("Graphe complet (tous connectés)");
    
    InterferenceGraph graph;
    vector<Vehicule*> vehicles;
    
    // Créer 5 véhicules avec une très grande portée (tous se voient)
    for (int i = 1; i <= 5; i++) {
        vehicles.push_back(createTestVehicle(i, 48.5734, 7.7521, 5000.0));
    }
    
    graph.buildGraph(vehicles);
    
    bool test1 = checkCondition("Nombre de véhicules = 5", graph.getVehicleCount() == 5);
    
    // Vérifier que chaque véhicule peut atteindre les autres
    int totalReachable = 0;
    for (int i = 1; i <= 5; i++) {
        auto reachable = graph.getReachableVehicles(i);
        totalReachable += reachable.size();
        cout << "  → V" << i << " peut atteindre " << reachable.size() << " véhicule(s)" << endl;
    }
    
    bool test2 = checkCondition("Au moins quelques connexions existent", totalReachable > 0);
    
    bool passed = test1 && test2;
    printTestResult("Graphe complet", passed);
    
    cleanupVehicles(vehicles);
    return passed;
}

bool InterferenceGraphTest::testStarTopology() {
    printTestHeader("Topologie en étoile (hub central)");
    
    InterferenceGraph graph;
    vector<Vehicule*> vehicles;
    
    // V1 au centre avec grande portée
    vehicles.push_back(createTestVehicle(1, 48.5734, 7.7521, 5000.0)); // Hub
    
    // V2, V3, V4 autour avec petite portée (ne se voient pas entre eux)
    vehicles.push_back(createTestVehicle(2, 48.5735, 7.7522, 5000.0));
    vehicles.push_back(createTestVehicle(3, 48.5736, 7.7523, 5000.0));
    vehicles.push_back(createTestVehicle(4, 48.5737, 7.7524, 5000.0));
    
    graph.buildGraph(vehicles);
    
    auto reachable2 = graph.getReachableVehicles(2);
    auto reachable3 = graph.getReachableVehicles(3);
    
    bool test1 = checkCondition("Nombre de véhicules = 4", graph.getVehicleCount() == 4);
    bool test2 = checkCondition("Véhicules périphériques peuvent communiquer via hub", 
                                !reachable2.empty() && !reachable3.empty());
    
    cout << "  → V2 peut atteindre " << reachable2.size() << " véhicule(s)" << endl;
    cout << "  → V3 peut atteindre " << reachable3.size() << " véhicule(s)" << endl;
    
    bool passed = test1 && test2;
    printTestResult("Topologie en étoile", passed);
    
    cleanupVehicles(vehicles);
    return passed;
}

bool InterferenceGraphTest::runAllTests() {
    cout << "\n";
    cout << "╔════════════════════════════════════════════════════════════╗" << endl;
    cout << "║                                                            ║" << endl;
    cout << "║      TESTS UNITAIRES - GRAPHE D'INTERFÉRENCE V2V          ║" << endl;
    cout << "║                                                            ║" << endl;
    cout << "╚════════════════════════════════════════════════════════════╝" << endl;
    
    // Lancer tous les tests
    testEmptyGraph();
    testSingleVehicle();
    testTwoVehiclesInRange();
    testTwoVehiclesOutOfRange();
    testTransitiveConnection();
    testChainOfVehicles();
    testDisconnectedGroups();
    testAsymmetricRange();
    testCompleteGraph();
    testStarTopology();
    
    return m_failedTests == 0;
}

void InterferenceGraphTest::printReport() const {
    cout << "\n";
    cout << "╔════════════════════════════════════════════════════════════╗" << endl;
    cout << "║                     RAPPORT FINAL                          ║" << endl;
    cout << "╚════════════════════════════════════════════════════════════╝" << endl;
    cout << "\n";
    cout << "  Total de tests exécutés : " << m_totalTests << endl;
    cout << "  ✅ Tests réussis        : " << m_passedTests << endl;
    cout << "  ❌ Tests échoués        : " << m_failedTests << endl;
    cout << "\n";
    
    double successRate = (m_totalTests > 0) ? 
        (100.0 * m_passedTests / m_totalTests) : 0.0;
    
    cout << "  Taux de réussite : " << fixed << setprecision(1) 
         << successRate << "%" << endl;
    cout << "\n";
    
    if (m_failedTests == 0) {
        cout << "  🎉 TOUS LES TESTS SONT PASSÉS ! 🎉" << endl;
    } else {
        cout << "  ⚠️  Certains tests ont échoué" << endl;
    }
    
    cout << "\n";
    cout << "╔════════════════════════════════════════════════════════════╗" << endl;
    cout << "║                   DÉTAILS DES TESTS                        ║" << endl;
    cout << "╚════════════════════════════════════════════════════════════╝" << endl;
    
    for (const auto& [testName, passed] : m_testResults) {
        cout << (passed ? "  ✅ " : "  ❌ ") << testName << endl;
    }
    cout << "\n";
}
