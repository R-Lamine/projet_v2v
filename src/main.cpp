#include "osm_reader.h"
#include "interference_test.h"

int main() {
    // ==========================================================
    // Lire les données OSM 
    // ==========================================================
    // OSMReader reader("../data/strasbourg.osm.pbf");
    // reader.read();
    // reader.printSummary();
    
    // ==========================================================
    // Tester le système de communication Partie 4
    // ==========================================================
    InterferenceTest test;
    test.runAllTests();

    return 0;
}