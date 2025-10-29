#include <QApplication>
#include <QMainWindow>
#include <QStatusBar>
#include "map_view.h"
#include "simulator.h"
#include "SimulatorView.h"
#include "graph_builder.h"
#include "osm_reader.h"

int main(int argc, char** argv){
    QApplication app(argc, argv);

    // ===== Load OSM & build graph =====
    OSMReader reader("../data/strasbourg.osm.pbf");
    reader.read();

    GraphBuilder builder(reader.nodes, reader.ways);
    builder.buildGraph();
    RoadGraph graph = builder.getGraph();

    Simulator sim(graph);
    sim.addCar(0, 2);
    sim.addCar(1, 2);

    // ===== Create main window =====
    QMainWindow win;
    win.setWindowTitle("V2V Simulator + OSM Map");

    // Map
    MapView* map = new MapView(&win);
    map->setTilesTemplate("https://tile.openstreetmap.org/{z}/{x}/{y}.png");
    map->setCenterLonLat(7.7521, 48.5734, 13);

    // Vehicles overlay
    SimulatorView* simView = new SimulatorView(map, map); // child of map to overlay
    simView->setVehicles(sim.getVehicules());
    simView->resize(map->size());
    simView->show();

    win.setCentralWidget(map);

    win.resize(1200, 800);
    win.show();

    return app.exec();
}
