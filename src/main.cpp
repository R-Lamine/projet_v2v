#include <QApplication>
#include <QMainWindow>
#include <QStatusBar>
#include <QProcessEnvironment>
#include "map_view.h"

int main(int argc, char** argv){
    QApplication app(argc, argv);
    QMainWindow win;
    win.setWindowTitle("V2V - Étape 1 (Online tiles with fallback)");

    auto* view = new MapView();
    win.setCentralWidget(view);
    QObject::connect(view, &MapView::cursorInfoChanged, &win, [&](const QString& s){
        win.statusBar()->showMessage(s);
    });

    // ===== Mode EN LIGNE (MapTiler) =====
    // export MAPTILER_KEY="ta_cle_api"
    const QString key = QProcessEnvironment::systemEnvironment().value("MAPTILER_KEY");
    if (!key.isEmpty()) {
        // ✅ URL correcte MapTiler (256 px tiles)
        view->setTilesTemplate("https://api.maptiler.com/maps/streets/256/{z}/{x}/{y}.png?key=" + key);

        // (Optionnel) identité + petit rate limit
        view->setNetworkIdentity("V2V-Simulator/1.0 (student@example.edu)",
                                 "https://university.example/course/v2v");
        view->setRequestRateLimitMs(80); // ~12 req/s

        view->setCenterLonLat(7.7521, 48.5734, 13); // Strasbourg
        win.statusBar()->showMessage("Online tiles via MapTiler • Molette=Zoom • Drag=Pan");
    } else {
        // ===== FALLBACK garanti (image locale) =====
        view->loadImage("data/strasbourg.png");
        win.statusBar()->showMessage("MAPTILER_KEY manquant → fallback image locale. export MAPTILER_KEY=YOUR_KEY pour la carte en ligne.", 8000);
    }

    win.resize(1200, 800);
    win.show();
    return app.exec();
}

