#include "osm_reader.h"
#include "graph_builder.h"
#include <iostream>

int main() {
   // 1. Lecture du fichier OSM
   OSMReader reader("data/strasbourg.osm.pbf");
   reader.read();
   reader.printSummary();

   // 2. Construction du graphe à partir des données lues
   GraphBuilder builder(reader.nodes, reader.ways);
   builder.buildGraph();
   builder.printSummary();

   return 0;
}
