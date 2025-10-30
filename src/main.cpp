#include <QApplication>
#include <QMainWindow>
#include <QStatusBar>
#include <QProcessEnvironment>

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

    // ===== Mode EN LIGNE (MapTiler) =====
    // export MAPTILER_KEY="ta_cle_api"
    const QString key = QProcessEnvironment::systemEnvironment().value("MAPTILER_KEY");
    if (!key.isEmpty()) {
        // ✅ URL correcte MapTiler (256 px tiles)
        map->setTilesTemplate("https://api.maptiler.com/maps/streets/256/{z}/{x}/{y}.png?key=" + key);

        // (Optionnel) identité + petit rate limit
        map->setNetworkIdentity("V2V-Simulator/1.0 (student@example.edu)",
                                 "https://university.example/course/v2v");
        map->setRequestRateLimitMs(80); // ~12 req/s

        map->setCenterLonLat(7.7521, 48.5734, 13); // Strasbourg
        win.statusBar()->showMessage("Online tiles via MapTiler • Molette=Zoom • Drag=Pan");
    } else {
        // ===== FALLBACK garanti (image locale) =====
        map->loadImage("data/strasbourg.png");
        win.statusBar()->showMessage("MAPTILER_KEY manquant → fallback image locale. export MAPTILER_KEY=YOUR_KEY pour la carte en ligne.", 8000);
    }
    map->setCenterLonLat(7.7521, 48.5734, 13);

    win.resize(1200, 800);
    win.show();
    return app.exec();
}
