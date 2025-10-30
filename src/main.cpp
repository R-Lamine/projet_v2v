#include <QApplication>
#include <QMainWindow>
#include <QStatusBar>
#include <QProcessEnvironment>
#include <QObject>

#include "map_view.h"
#include "simulator.h"
#include "graph_builder.h"
#include "osm_reader.h"

#define DELTA_TIME 0.5
#define CAR_COUNT 10

int main(int argc, char** argv){
    QApplication app(argc, argv);

    // ----------------------
    //  Load OSM data
    OSMReader reader("../data/strasbourg.osm.pbf");
    reader.read();
    reader.printSummary();

    // 2. Construction du graphe
    GraphBuilder builder(reader.nodes, reader.ways);
    builder.buildGraph();
    builder.printSummary();
    const RoadGraph& graph = builder.getGraph();


    // Create main window
    // ----------------------
    QMainWindow win;
    win.setWindowTitle("V2V ");

    MapView* map = new MapView(&win);
    map->setTilesTemplate("https://tile.openstreetmap.org/{z}/{x}/{y}.png");
    map->setCenterLonLat(7.7521, 48.5734, 14);

    win.setCentralWidget(map);
    QObject::connect(map, &MapView::cursorInfoChanged, &win, [&](const QString& s){
        win.statusBar()->showMessage(s);
    });

    win.resize(1200, 800);
    win.show();

    // --- Create simulator ---
    Simulator simulator(const_cast<RoadGraph&>(graph), map);
    map->setSimulator(&simulator);
    QObject::connect(&simulator, &Simulator::ticked, map, [map](double){
        map->update();
    });


    //GENERATE RANDOM CARS
    std::vector<Vertex> vertices;
    for (auto vp = boost::vertices(graph); vp.first != vp.second; ++vp.first) {
        vertices.push_back(*vp.first);
    }

    const int NUM_CARS = 50;
    for (int i = 0; i < NUM_CARS; ++i) {
        Vertex start = vertices[rand() % vertices.size()];
        Vertex goal = vertices[rand() % vertices.size()];
        double speed = 13.9;           // 50 km/h in m/s
        double range = 100.0;          // transmission range
        double collisionDist = 5.0;    // 5 meters

        Vehicule* car = new Vehicule(i, graph, start, goal, speed, range, collisionDist);
        simulator.addVehicle(car);

        qDebug() << "Vehicle created:" << i << "start" << start << "goal" << goal;
        qDebug() << "Vehicle" << i << "position:" << car->getPosition() ;
    }


    simulator.start(50); // 20 FPS
    return app.exec();
}
