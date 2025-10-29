#include "simulator.h"
#include "SimulatorView.h"
#include "osm_reader.h"
#include "graph_builder.h"

#include <QApplication>

int main(int argc, char *argv[]) {
   // 1. Lecture du fichier OSM
   OSMReader reader("../data/strasbourg.osm.pbf");
   reader.read();
   reader.printSummary();

   // 2. Construction du graphe à partir des données lues
   GraphBuilder builder(reader.nodes, reader.ways);
   builder.buildGraph();
   builder.printSummary();

    QApplication app(argc, argv);
    RoadGraph graph = builder.getGraph();
    Simulator sim(graph);                // pass graph to constructor

    sim.addCar(0, 2); // start at vertex 0, goal vertex 2
    sim.addCar(1, 2); // start at vertex 1, goal vertex 2

    for (int t = 0; t < 5; ++t) {
        sim.update(1.0); // advance 1 second
        sim.printStatus();
    }

/*
    SimulatorView view;
    view.setVehicles(sim.getVehicules());
    view.resize(800, 600);
    view.show();
*/
    return app.exec();
}
