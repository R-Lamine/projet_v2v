#include "map_view.h"
#include "vehicle_renderer.h"
#include "overlay_ui.h"
#include "vehicule.h"
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QFileInfo>
#include <QUrl>
#include <QNetworkRequest>
#include <QDateTime>
#include <QtMath>
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>

#include "simulator.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static inline double deg2rad(double d){ return d * M_PI / 180.0; }
static inline double rad2deg(double r){ return r * 180.0 / M_PI; }

MapView::MapView(QWidget* parent)
    : QWidget(parent), m_darkCache(1024), m_lightCache(1024) {
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(30,30,30));
    setPalette(pal);

    // Définir le chemin du SVG (chargement paresseux lors du premier dessin)
    VehicleRenderer::setSvgPath("../../data/car-top-view-icon.svg");

    // Initialiser le template de tuiles avec le thème sombre par défaut
    m_tilesTemplate = m_darkTilesTemplate;

    // Créer l'overlay UI
    m_uiOverlay = new UIOverlay(this);
    m_uiOverlay->raise();  // S'assurer qu'il est au-dessus de la carte
    
    // Connecter les signaux de l'overlay
    connect(m_uiOverlay->topBar(), &TopBar::startPauseClicked, this, [this]() {
        if (m_simulator) {
            m_simulator->togglePause();
            m_uiOverlay->topBar()->setRunning(m_simulator->isRunning());
        }
    });
    
    // Connecter le toggle thème (dark/light)
    connect(m_uiOverlay->topBar(), &TopBar::themeToggled, this, [this](bool dark) {
        m_darkTheme = dark;
        m_tilesTemplate = m_darkTheme ? m_darkTilesTemplate : m_lightTilesTemplate;
        std::cout << "[MapView] Thème " << (m_darkTheme ? "sombre" : "clair") << std::endl;
        update();
    });
    
    // Connecter le toggle qualité (HQ/Fast)
    connect(m_uiOverlay->topBar(), &TopBar::qualityToggled, this, [this](bool hq) {
        m_lowQualityMode = !hq;  // lowQuality = inverse de highQuality
        std::cout << "[MapView] Mode low quality " << (m_lowQualityMode ? "activé" : "désactivé") << std::endl;
        update();
    });
    
    connect(m_uiOverlay->zoomControls(), &ZoomControls::zoomIn, this, &MapView::zoomIn);
    connect(m_uiOverlay->zoomControls(), &ZoomControls::zoomOut, this, &MapView::zoomOut);
    
    // Connecter les toggles du panneau de paramètres
    auto* params = m_uiOverlay->bottomMenu()->parametersPanel();
    // Utiliser sliderReleased pour éviter les freezes pendant le sliding
    connect(params, &ParametersPanel::vehicleCountReleased, this, [this](int count) {
        if (m_simulator) {
            m_simulator->setVehicleCount(count);
            update();
        }
    });
    connect(params, &ParametersPanel::showConnectionsChanged, this, [this](bool show) {
        m_drawDirectConnections = show;
        update();
    });
    connect(params, &ParametersPanel::showRangesChanged, this, [this](bool show) {
        m_showRanges = show;
        update();
    });
    connect(params, &ParametersPanel::showTransitiveChanged, this, [this](bool show) {
        m_showTransitiveConnections = show;
        if (m_simulator) {
            m_simulator->interferenceGraph().enableTransitiveClosure(show);
        }
        update();
    });
    connect(params, &ParametersPanel::showRoadsChanged, this, [this](bool show) {
        m_showRoads = show;
        update();
    });
    connect(params, &ParametersPanel::transmissionRangeChanged, this, [this](int range) {
        if (m_simulator) {
            // Mettre à jour le rayon de transmission de tous les véhicules
            for (auto* v : m_simulator->vehicles()) {
                v->setTransmissionRange(range);
            }
            // Mettre à jour les voisinages d'antennes avec la nouvelle portée
            m_simulator->interferenceGraph().updateTransmissionRange(range);
            update();
        }
    });
    
    // Connexion pour la vitesse des véhicules
    connect(params, &ParametersPanel::vehicleSpeedChanged, this, [this](int speedKmh) {
        if (m_simulator) {
            // Convertir km/h en m/s (1 km/h = 1/3.6 m/s)
            double speedMs = speedKmh / 3.6;
            for (auto* v : m_simulator->vehicles()) {
                v->setSpeed(speedMs);
            }
        }
    });
    
    // Connexion pour le placement automatique des antennes (K-means) au relâchement des sliders
    connect(params, &ParametersPanel::antennaConfigReleased, this, [this](int numLarge, int numSmall) {
        if (m_simulator) {
            m_simulator->placeAntennas(numLarge, numSmall);
            update();
        }
    });

    connect(&m_net, &QNetworkAccessManager::finished, this, [this](QNetworkReply* rep){
        rep->deleteLater();
    });
}

void MapView::setSimulator(Simulator* sim) {
    m_simulator = sim;
    if (m_uiOverlay) {
        m_uiOverlay->setSimulator(sim);
        // Synchroniser l'état initial de l'UI avec le simulateur
        if (sim) {
            m_uiOverlay->topBar()->setRunning(sim->isRunning());
            
            // Connecter les signaux du simulateur pour mettre à jour l'UI
            connect(sim, &Simulator::simulationStarted, this, [this]() {
                m_uiOverlay->topBar()->setRunning(true);
            });
            connect(sim, &Simulator::simulationPaused, this, [this]() {
                m_uiOverlay->topBar()->setRunning(false);
            });
            connect(sim, &Simulator::simulationResumed, this, [this]() {
                m_uiOverlay->topBar()->setRunning(true);
            });
            connect(sim, &Simulator::simulationStopped, this, [this]() {
                m_uiOverlay->topBar()->setRunning(false);
            });
            
            // Mettre à jour le slider quand le nombre de véhicules change
            connect(sim, &Simulator::vehicleCountChanged, this, [this](int count) {
                auto* params = m_uiOverlay->bottomMenu()->parametersPanel();
                params->setVehicleCount(count);
            });
            
            // Connecter le bouton supprimer véhicule
            connect(m_uiOverlay, &UIOverlay::deleteTrackedVehicle, this, [this]() {
                if (m_trackedVehicle && m_simulator) {
                    Vehicule* toDelete = m_trackedVehicle;
                    m_trackedVehicle = nullptr;
                    m_followingVehicle = false;
                    m_uiOverlay->showDeleteVehicleButton(false);
                    m_simulator->removeVehicle(toDelete);
                }
            });
        }
    }
}

void MapView::zoomIn() {
    zoomAt(QPoint(width()/2, height()/2), 2.0);
}

void MapView::zoomOut() {
    zoomAt(QPoint(width()/2, height()/2), 0.5);
}

bool MapView::loadImage(const QString& path){
    QPixmap px;
    if(!px.load(path)){
        return false;
    }
    m_base = std::move(px);
    update();
    return true;
}

void MapView::setTilesTemplate(const QString& pattern){
    m_tilesTemplate = pattern;
    update();
}

void MapView::setCenterWorld(double px, double py, int zoom){
    m_zoom = std::clamp(zoom, 0, 20);
    m_offsetX = px - width()/2.0;
    m_offsetY = py - height()/2.0;
    update();
}

void MapView::setCenterLonLat(double lonDeg, double latDeg, int zoom){
    double px, py;
    lonlatToPixel(lonDeg, latDeg, std::clamp(zoom,0,20), px, py);
    setCenterWorld(px, py, zoom);
}

void MapView::paintEvent(QPaintEvent*){
    // Vérifier que le véhicule suivi existe toujours
    if (m_followingVehicle && m_trackedVehicle && m_simulator) {
        const auto& vehicles = m_simulator->vehicles();
        bool found = std::find(vehicles.begin(), vehicles.end(), m_trackedVehicle) != vehicles.end();
        if (!found) {
            // Le véhicule a été supprimé
            m_trackedVehicle = nullptr;
            m_followingVehicle = false;
            if (m_uiOverlay) m_uiOverlay->showDeleteVehicleButton(false);
        }
    }
    
    // Update camera to follow tracked vehicle
    if (m_followingVehicle && m_trackedVehicle) {
        auto [vLat, vLon] = m_trackedVehicle->getPosition();  // getPosition retourne {lat, lon}
        double px, py;
        lonlatToPixel(vLon, vLat, m_zoom, px, py);
        m_offsetX = px - width() / 2.0;
        m_offsetY = py - height() / 2.0;
    }
    
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if(!m_tilesTemplate.isEmpty()){
        drawTiles(p);
    } else if(!m_base.isNull()){
        p.translate(-m_offsetX, -m_offsetY);
        p.drawPixmap(QPointF(0,0), m_base);
    } else {
        const int step = 64;
        QPen grid(QColor(80,80,80));
        p.setPen(grid);
        for(int x=0; x< 4096; x+=step)
            p.drawLine(x - (int)m_offsetX, 0 - (int)m_offsetY, x - (int)m_offsetX, 4096 - (int)m_offsetY);
        for(int y=0; y< 4096; y+=step)
            p.drawLine(0 - (int)m_offsetX, y - (int)m_offsetY, 4096 - (int)m_offsetX, y - (int)m_offsetY);
    }

    // Dessiner les routes du graphe si activé
    if (m_showRoads && m_simulator) {
        // Pré-calculer les routes valides une seule fois
        if (!m_roadsPrecomputed) {
            const auto& graph = m_simulator->getGraph();
            m_validRoads.clear();
            m_validRoads.reserve(boost::num_edges(graph) / 2);  // Estimation
            
            for (auto ep = boost::edges(graph); ep.first != ep.second; ++ep.first) {
                Edge e = *ep.first;
                if (!Vehicule::isValidRoad(graph[e].type)) continue;
                
                Vertex s = boost::source(e, graph);
                Vertex t = boost::target(e, graph);
                
                RoadSegment seg;
                seg.lon1 = graph[s].lon;
                seg.lat1 = graph[s].lat;
                seg.lon2 = graph[t].lon;
                seg.lat2 = graph[t].lat;
                m_validRoads.push_back(seg);
            }
            m_roadsPrecomputed = true;
        }
        
        // Calculer les limites visibles
        double minLon, minLat, maxLon, maxLat;
        screenToLonLat(QPoint(0, height()), minLon, minLat);
        screenToLonLat(QPoint(width(), 0), maxLon, maxLat);
        
        // Style des routes
        p.setPen(QPen(QColor(138, 43, 226, 100), 2));
        p.setRenderHint(QPainter::Antialiasing, false);  // Désactiver AA pour performance
        
        // Dessiner les routes visibles
        for (const auto& seg : m_validRoads) {
            // Test de visibilité rapide
            if ((seg.lon1 < minLon && seg.lon2 < minLon) || 
                (seg.lon1 > maxLon && seg.lon2 > maxLon) ||
                (seg.lat1 < minLat && seg.lat2 < minLat) || 
                (seg.lat1 > maxLat && seg.lat2 > maxLat)) {
                continue;
            }
            
            QPointF p1 = lonLatToScreen(seg.lon1, seg.lat1);
            QPointF p2 = lonLatToScreen(seg.lon2, seg.lat2);
            p.drawLine(p1, p2);
        }
    }

    // ----------- DEBUG --------
    // draw edges (ways) on map to see which are detected
    // QPen pen(Qt::red, 10);
    // p.setPen(pen);
    // p.drawPoint(width()/2, height()/2);

    const auto& graph = m_simulator->getGraph();
    int drawn = 0;
    /*/ Draw edges first
    for (auto ep = boost::edges(graph); ep.first != ep.second; ++ep.first) {
        Edge e = *ep.first;
        Vertex s = boost::source(e, graph);
        Vertex t = boost::target(e, graph);

        // récupère les coordonnées lat/lon
        double lat1 = graph[s].lat;
        double lon1 = graph[s].lon;
        double lat2 = graph[t].lat;
        double lon2 = graph[t].lon;

        // convertit en pixels
        QPointF p1 = lonLatToScreen(lon1, lat1);
        QPointF p2 = lonLatToScreen(lon2, lat2);

        // couleur et épaisseur selon type de route
        QColor color;
        int width = 3; // default thickness
        const std::string& type = graph[e].type;


        if (type == "motorway" || type == "motorway_link")  {
            drawn ++;
            color = QColor(255, 0, 0);  // bright red
            width = 4;
        } else if (type == "trunk" || type == "trunk_link") {
            drawn ++;
            color = QColor(255, 128, 0); // orange
            width = 3;
        } else if (type == "primary" || type == "primary_link") {
            drawn ++;
            color = QColor(255, 255, 0); // yellow
            width = 3;
        } else if (type == "secondary" || type == "secondary_link") {
            drawn ++;
            color = QColor(0, 0, 255);   // blue
            width = 2;
        } else if (type == "tertiary" ) {
            drawn ++;
            color = QColor(0, 255, 0);   // green
            width = 1;
        } else { color = QColor(128, 128, 128); // gray for footway, unknown, etc.
            width = 1; }

        QPen pen(color, width*2 ,  Qt::SolidLine);
        pen.setCapStyle(Qt::RoundCap); // nicer line endings
        p.setPen(pen);

        p.drawLine(p1, p2);

    }

    // Draw nodes on top
    int drawnNodes = 0;
    QBrush nodeBrush(Qt::red);
    p.setBrush(nodeBrush);
    p.setPen(Qt::NoPen);

    for (auto vp = boost::vertices(graph); vp.first != vp.second; ++vp.first) {
        Vertex v = *vp.first;
        QPointF pos = lonLatToScreen(graph[v].lat, graph[v].lon);

        // draw small red square centered on node
        drawnNodes ++;
        int size = std::max(4.0, 8.0 / (1 << m_zoom));
        QRectF rect(pos.x() - size / 2, pos.y() - size / 2, size, size);
        p.drawRect(rect);
    }
    */


    //Draw vehicules on map
    if (m_simulator) {
        const auto& vehicles = m_simulator->vehicles();
        const auto& interfGraph = m_simulator->interferenceGraph();
        
        // Calculer les limites de l'écran pour ne dessiner que les véhicules visibles
        double minLat, maxLat, minLon, maxLon;
        screenToLonLat(0, 0, minLon, maxLat);  // Coin haut-gauche
        screenToLonLat(width(), height(), maxLon, minLat);  // Coin bas-droit
        
        // Filtrer les véhicules visibles
        std::vector<Vehicule*> visibleVehicles;
        for (auto* v : vehicles) {
            if (!v) continue;
            auto [lat, lon] = v->getPosition();
            if (lat >= minLat && lat <= maxLat && lon >= minLon && lon <= maxLon) {
                visibleVehicles.push_back(v);
            }
        }
        
        // Seuil: ne dessiner les détails que s'il y a moins de 500 véhicules visibles
        bool drawDetails = visibleVehicles.size() < 500;

        // Dessiner les rayons de transmission si activé et peu de véhicules visibles
        if (m_showRanges && drawDetails) {
            QColor rangeColor = m_darkTheme ? QColor(100, 200, 220, 80) : QColor(0, 180, 255, 110); // cyan vif
            QColor rangeFill = m_darkTheme ? QColor(100, 200, 220, 5) : QColor(0, 180, 255, 40);
            for (auto* v : visibleVehicles) {
                auto [lat, lon] = v->getPosition();
                QPointF pt = lonLatToScreen(lon, lat);
                double range = v->getTransmissionRange();
                double mpp = metersPerPixelAtLat(lat);
                double radiusPixels = range / mpp;
                QPen rangePen(rangeColor);
                rangePen.setWidth(1);
                p.setPen(rangePen);
                p.setBrush(rangeFill);
                p.drawEllipse(pt, radiusPixels, radiusPixels);
            }
        }

        // Dessiner les connexions si peu de véhicules visibles (augmenté à 500)
        if (drawDetails && visibleVehicles.size() < 500) {
            // Dessiner d'abord les connexions transitives (lignes bleues pointillées) si activé
            if (m_showTransitiveConnections) {
                QColor transitiveColor = m_darkTheme ? QColor(147, 112, 219, 120) : QColor(180, 0, 255, 140); // violet vif
                QPen transitivePen(transitiveColor);
                transitivePen.setWidth(1);
                transitivePen.setStyle(Qt::DashLine);
                p.setPen(transitivePen);
                for (auto* v : visibleVehicles) {
                    auto directNeighbors = interfGraph.getDirectNeighbors(v->getId());
                    auto allReachable = interfGraph.getReachableVehicles(v->getId());
                    auto [lat1, lon1] = v->getPosition();
                    QPointF pt1 = lonLatToScreen(lon1, lat1);
                    for (int reachableId : allReachable) {
                        if (directNeighbors.find(reachableId) != directNeighbors.end()) {
                            continue;
                        }
                        for (auto* reachable : vehicles) {
                            if (reachable && reachable->getId() == reachableId) {
                                auto [lat2, lon2] = reachable->getPosition();
                                QPointF pt2 = lonLatToScreen(lon2, lat2);
                                if (v->getId() < reachableId) {
                                    p.drawLine(pt1, pt2);
                                }
                                break;
                            }
                        }
                    }
                }
            }

            // Dessiner ensuite les connexions directes (lignes bleues) si activé
            if (m_drawDirectConnections) {
                QColor connectionColor = m_darkTheme ? QColor(135, 206, 235, 150) : QColor(0, 120, 255, 200); // bleu vif
                QPen connectionPen(connectionColor);
                connectionPen.setWidth(2);
                p.setPen(connectionPen);
                for (auto* v : visibleVehicles) {
                    auto directNeighbors = interfGraph.getDirectNeighbors(v->getId());
                    auto [lat1, lon1] = v->getPosition();
                    QPointF pt1 = lonLatToScreen(lon1, lat1);
                    for (int neighborId : directNeighbors) {
                        for (auto* neighbor : vehicles) {
                            if (neighbor && neighbor->getId() == neighborId) {
                                auto [lat2, lon2] = neighbor->getPosition();
                                QPointF pt2 = lonLatToScreen(lon2, lat2);
                                if (v->getId() < neighborId) {
                                    p.drawLine(pt1, pt2);
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Dessiner les antennes DERRIÈRE les véhicules
        if (interfGraph.isSpatialOptimizationEnabled()) {
            const auto& spatialGrid = interfGraph.getSpatialGrid();
            const auto& macroAntennas = spatialGrid.getMacroAntennas();
            const auto& microAntennas = spatialGrid.getMicroAntennas();
            
            // D'abord dessiner les petites antennes (plus discrètes)
            for (const auto& [id, micro] : microAntennas) {
                QPointF center = lonLatToScreen(micro.centerLon, micro.centerLat);
                
                // Dessiner seulement le triangle, pas la zone
                QPen centerPen(QColor(255, 140, 0, 255)); // Orange opaque
                centerPen.setWidth(1);
                p.setPen(centerPen);
                p.setBrush(QBrush(QColor(255, 165, 0, 200)));
                
                QPointF microTriangle[3] = {
                    QPointF(center.x(), center.y() - 5),       // Haut
                    QPointF(center.x() - 4, center.y() + 4),   // Bas gauche
                    QPointF(center.x() + 4, center.y() + 4)    // Bas droit
                };
                p.drawPolygon(microTriangle, 3);
            }
            
            // Ensuite dessiner les grandes antennes
            for (const auto& [id, macro] : macroAntennas) {
                QPointF center = lonLatToScreen(macro.centerLon, macro.centerLat);
                
                // Dessiner seulement le triangle et le label, pas la zone
                QPen centerPen(QColor(0, 255, 255, 255)); // Cyan opaque
                centerPen.setWidth(3);
                p.setPen(centerPen);
                p.setBrush(QBrush(QColor(0, 255, 255)));
                
                QPointF triangle[3] = {
                    QPointF(center.x(), center.y() - 12),
                    QPointF(center.x() - 10, center.y() + 10),
                    QPointF(center.x() + 10, center.y() + 10)
                };
                p.drawPolygon(triangle, 3);
                
                // Afficher l'ID de l'antenne avec fond noir pour visibilité
                p.setPen(Qt::black);
                p.setBrush(QBrush(QColor(0, 0, 0, 180)));
                QRectF textBg(center.x() + 10, center.y() - 10, 30, 20);
                p.drawRect(textBg);
                
                p.setPen(Qt::white);
                QFont font = p.font();
                font.setPointSize(11);
                font.setBold(true);
                p.setFont(font);
                p.drawText(center.x() + 14, center.y() + 5, QString("A%1").arg(id));
            }
        }

        // Dessiner les véhicules
        // Si zoom <= 12, dessiner en simples points colorés pour performance
        // Sinon, utiliser les SVG orientés
        if (m_zoom <= 12) {
            // Mode points simples pour zoom faible
            double pointSize = std::max(2.0, 3.0 + (m_zoom - 8) * 0.5);  // 2-5 pixels selon zoom
            for (auto* v : visibleVehicles) {
                auto [lat, lon] = v->getPosition();
                QPointF pt = lonLatToScreen(lon, lat);
                QRandomGenerator gen(v->getId());
                QColor vehicleColor = m_darkTheme
                    ? QColor(gen.bounded(120, 220), gen.bounded(120, 220), gen.bounded(120, 220), 255)
                    : QColor(gen.bounded(200, 255), gen.bounded(80, 255), gen.bounded(0, 255), 255); // couleurs vives
                p.setPen(Qt::NoPen);
                p.setBrush(vehicleColor);
                p.drawEllipse(pt, pointSize, pointSize);
            }
        } else {
            // Mode SVG orientés pour zoom >= 13
            double baseSize = 16.0;  // Taille de base
            double zoomFactor = std::pow(1.15, m_zoom - 16);  // Zoom 14 = taille normale
            double vehicleSize = std::clamp(baseSize * zoomFactor, 6.0, 100.0);  // Min 6, Max 40 pixels
            for (auto* v : visibleVehicles) {
                auto [lat, lon] = v->getPosition();
                QPointF pt = lonLatToScreen(lon, lat);
                double heading = v->getHeading();
                QRandomGenerator gen(v->getId());
                QColor vehicleColor = m_darkTheme
                    ? QColor(gen.bounded(120, 220), gen.bounded(120, 220), gen.bounded(120, 220), 255)
                    : QColor(gen.bounded(200, 255), gen.bounded(80, 255), gen.bounded(0, 255), 255); // couleurs vives
                VehicleRenderer::drawVehicle(p, pt, heading, vehicleColor, vehicleSize);
            }
        }
    }

    // Mettre à jour les stats et infos de l'UI overlay
    if (m_uiOverlay) {
        m_uiOverlay->updateStats();
        
        // Mettre à jour les infos de carte (zoom, position)
        double lonC, latC;
        screenToLonLat(QPoint(width()/2, height()/2), lonC, latC);
        m_uiOverlay->updateMapInfo(m_zoom, lonC, latC);
    }

    drawHUD(p);
}

void MapView::zoomAt(const QPoint& screenPos, double factor){
    int newZ = m_zoom + (factor > 1.0 ? +1 : -1);
    newZ = std::clamp(newZ, 0, 20);
    if(newZ == m_zoom) return;

    double wx_before = m_offsetX + screenPos.x();
    double wy_before = m_offsetY + screenPos.y();

    double scale = std::pow(2.0, newZ - m_zoom);
    m_offsetX = wx_before*scale - screenPos.x();
    m_offsetY = wy_before*scale - screenPos.y();
    m_zoom = newZ;
    update();
}

void MapView::wheelEvent(QWheelEvent* ev){
    const double steps = ev->angleDelta().y() / 120.0;
    if(steps > 0) zoomAt(ev->position().toPoint(), 2.0);
    else if(steps < 0) zoomAt(ev->position().toPoint(), 0.5);
}

void MapView::mousePressEvent(QMouseEvent* ev){
    if(ev->button()==Qt::LeftButton){
        m_dragging = true;
        m_didDrag = false;
        m_lastPos = ev->pos();
        m_pressPos = ev->pos();
    }
}

void MapView::mouseMoveEvent(QMouseEvent* ev){
    if(m_dragging){
        QPoint d = ev->pos() - m_lastPos;
        
        // Vérifier si on a bougé significativement (> 5 pixels)
        if (!m_didDrag) {
            int totalMove = (ev->pos() - m_pressPos).manhattanLength();
            if (totalMove > 5) {
                m_didDrag = true;
                // Stop following vehicle when user drags the map
                if (m_followingVehicle) {
                    m_followingVehicle = false;
                    m_trackedVehicle = nullptr;
                    if (m_uiOverlay) m_uiOverlay->showDeleteVehicleButton(false);
                }
            }
        }
        
        if (m_didDrag) {
            m_offsetX -= d.x();
            m_offsetY -= d.y();
            update();
        }
        m_lastPos = ev->pos();
    }
    double lon, lat;
    screenToLonLat(ev->pos(), lon, lat);

    // fix du message (évite "QString::arg: Argument missing")
    emit cursorInfoChanged(QString("Zoom %1  |  Lon %2  Lat %3")
        .arg(m_zoom).arg(lon,0,'f',5).arg(lat,0,'f',5));
}

void MapView::mouseReleaseEvent(QMouseEvent* ev){
    if(ev->button()==Qt::LeftButton) {
        if (!m_didDrag) {
            // C'était un clic simple, pas un drag
            double clickLon, clickLat;
            screenToLonLat(m_pressPos, clickLon, clickLat);
            
            // Seuil adaptatif selon le zoom
            double metersPerPx = metersPerPixelAtLat(clickLat);
            double clickRadiusPx = 20.0;
            double clickRadiusMeters = clickRadiusPx * metersPerPx;
            double CLICK_THRESHOLD = clickRadiusMeters / 111000.0;
            
            Vehicule* closestVehicle = nullptr;
            double minDist = CLICK_THRESHOLD;
            
            for (auto* v : m_simulator->vehicles()) {
                auto [vLat, vLon] = v->getPosition();
                double dist = std::hypot(vLon - clickLon, vLat - clickLat);
                if (dist < minDist) {
                    minDist = dist;
                    closestVehicle = v;
                }
            }
            
            if (closestVehicle) {
                // Clic sur un véhicule existant -> le suivre
                m_trackedVehicle = closestVehicle;
                m_followingVehicle = true;
                if (m_uiOverlay) m_uiOverlay->showDeleteVehicleButton(true);
            } else {
                // Clic sur zone vide -> créer un véhicule sans le suivre
                m_simulator->createVehicleNear(clickLon, clickLat);
            }
        }
        m_dragging = false;
    }
}

void MapView::keyPressEvent(QKeyEvent* ev){
    const int step = 128;
    switch(ev->key()){
        case Qt::Key_Plus:
        case Qt::Key_Equal:      zoomAt(QPoint(width()/2, height()/2), 2.0);   break;
        case Qt::Key_Minus:
        case Qt::Key_Underscore: zoomAt(QPoint(width()/2, height()/2), 0.5);   break;
        case Qt::Key_Left:       m_offsetX -= step; update();                  break;
        case Qt::Key_Right:      m_offsetX += step; update();                  break;
        case Qt::Key_Up:         m_offsetY -= step; update();                  break;
        case Qt::Key_Down:       m_offsetY += step; update();                  break;
        case Qt::Key_T: {
            // Toggle transitive connections display and computation
            m_showTransitiveConnections = !m_showTransitiveConnections;
            if (m_simulator) {
                auto& interfGraph = m_simulator->interferenceGraph();
                interfGraph.enableTransitiveClosure(m_showTransitiveConnections);
                std::cout << "[MapView] Connexions transitives " 
                          << (m_showTransitiveConnections ? "activées" : "désactivées") << std::endl;
            }
            update();
            break;
        }
        case Qt::Key_L: {
            // Toggle low quality tiles mode
            m_lowQualityMode = !m_lowQualityMode;
            std::cout << "[MapView] Mode low quality " 
                      << (m_lowQualityMode ? "activé" : "désactivé") << std::endl;
            // Sync UI button
            if (m_uiOverlay) {
                m_uiOverlay->topBar()->setHighQuality(!m_lowQualityMode);
            }
            update();
            break;
        }
        case Qt::Key_B: {
            // Toggle dark/light theme
            m_darkTheme = !m_darkTheme;
            m_tilesTemplate = m_darkTheme ? m_darkTilesTemplate : m_lightTilesTemplate;
            // Pas besoin de vider le cache - chaque thème a son propre cache !
            std::cout << "[MapView] Thème " 
                      << (m_darkTheme ? "sombre" : "clair") << std::endl;
            // Sync UI button
            if (m_uiOverlay) {
                m_uiOverlay->topBar()->setDarkTheme(m_darkTheme);
            }
            update();
            break;
        }
        default: QWidget::keyPressEvent(ev); break;
    }
}

void MapView::resizeEvent(QResizeEvent*) {
    // Redimensionner l'overlay pour couvrir toute la vue
    if (m_uiOverlay) {
        m_uiOverlay->setGeometry(0, 0, width(), height());
    }
}

QString MapView::buildUrl(int z,int x,int y) const{
    QString u = m_tilesTemplate;
    u.replace("{z}", QString::number(z));
    u.replace("{x}", QString::number(x));
    u.replace("{y}", QString::number(y));
    return u;
}

void MapView::requestTile(int z,int x,int y){
    if(m_tilesTemplate.isEmpty()) return;
    const QString url = buildUrl(z,x,y);

    if(QPixmap* cached = getActiveCache().object(url)){
        return;
    }

    TileKey key{z,x,y};
    if(m_inflight.contains(key)) return;

    if(url.startsWith("file://")){
        const QString path = QUrl(url).toLocalFile();
        if(QFileInfo::exists(path)){
            QPixmap* px = new QPixmap();
            if(px->load(path)){
                getActiveCache().insert(url, px);
                update();
            } else delete px;
        }
        return;
    }

    const qint64 now  = QDateTime::currentMSecsSinceEpoch();
    const qint64 wait = m_minRequestIntervalMs - (now - m_lastRequestMs);
    if (wait > 0) {
        QTimer::singleShot(int(wait), this, [this, z, x, y](){ requestTile(z, x, y); });
        return;
    }
    m_lastRequestMs = now;

    QNetworkRequest req{ QUrl{url} };
    req.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);
    req.setRawHeader("Referer", m_referer.toUtf8());
    req.setRawHeader("Cache-Control", "max-age=86400");

    QNetworkReply* rep = m_net.get(req);
    m_inflight.insert(key, rep);

    connect(rep, &QNetworkReply::finished, this, [this, url, key, rep](){
        m_inflight.remove(key);
        if(rep->error()==QNetworkReply::NoError){
            QByteArray data = rep->readAll();
            QPixmap* px = new QPixmap();
            if(px->loadFromData(data)){
                getActiveCache().insert(url, px);
                update();
            } else delete px;
        }
        rep->deleteLater();
    });
}

void MapView::drawTiles(QPainter& p){
    const int T = 256;
    
    // Choisir le niveau de zoom des tuiles selon le mode
    int tileZoom;
    if (m_lowQualityMode) {
        // Mode low quality : limiter les niveaux de zoom
        if (m_zoom >= 13) {
            tileZoom = 13;
        } else if (m_zoom >= 10) {
            tileZoom = 10;
        } else if (m_zoom >= 8) {
            tileZoom = 8;
        } else if (m_zoom >= 4) {
            tileZoom = 4;
        } else {
            tileZoom = (int)m_zoom;  // Zoom normal en dessous de 4
        }
    } else {
        // Mode normal : utiliser le zoom actuel
        tileZoom = (int)m_zoom;
    }
    
    const int n = 1 << tileZoom;
    
    // Adapter les coordonnées en fonction de la différence de zoom
    int zoomDiff = (int)m_zoom - tileZoom;
    int scale = 1 << std::abs(zoomDiff);
    
    int scaledOffsetX, scaledOffsetY;
    if (zoomDiff >= 0) {
        // m_zoom >= tileZoom : diviser
        scaledOffsetX = m_offsetX / scale;
        scaledOffsetY = m_offsetY / scale;
    } else {
        // m_zoom < tileZoom : multiplier
        scaledOffsetX = m_offsetX * scale;
        scaledOffsetY = m_offsetY * scale;
    }

    int x0 = int(std::floor(scaledOffsetX / T));
    int y0 = int(std::floor(scaledOffsetY / T));
    int nx = int(std::ceil((scaledOffsetX + width() / scale) / T)) - x0;
    int ny = int(std::ceil((scaledOffsetY + height() / scale) / T)) - y0;

    p.fillRect(rect(), QColor(20,20,20));

    for(int dy=0; dy<=ny; ++dy){
        for(int dx=0; dx<=nx; ++dx){
            int tx = x0 + dx;
            int ty = y0 + dy;

            int txWrap = ((tx % n) + n) % n;
            if(ty < 0 || ty >= n) continue;

            const QString url = buildUrl(tileZoom, txWrap, ty);
            QPixmap* cached = getActiveCache().object(url);
            
            // Afficher la tuile à la taille correcte selon le zoom actuel
            QRectF target(tx*T*scale - m_offsetX, ty*T*scale - m_offsetY, T*scale, T*scale);

            if(!cached){
                requestTile(tileZoom, txWrap, ty);
                p.fillRect(target, QColor(60,60,60));
            } else {
                p.drawPixmap(target, *cached, QRectF(0,0,T,T));
            }
        }
    }
}

void MapView::drawHUD(QPainter& p){
    // Les infos sont maintenant affichées dans la TopBar de l'UI overlay
    // On garde juste l'échelle en bas à gauche
    
    double lonC, latC;
    screenToLonLat(QPoint(width()/2, height()/2), lonC, latC);

    double mpp = metersPerPixelAtLat(latC);
    static const int niceVals[] = {5,10,20,50,100,200,500,1000,2000,5000,10000,20000};
    int bestM = 100;
    int targetPx = 150;
    for(int v : niceVals){
        int px = int(v / mpp);
        if(px <= targetPx) bestM = v;
    }
    int barPx = int(bestM / mpp);

    int bx = 12;
    int by = height() - 12 - 20;
    
    // Décaler l'échelle vers le haut si le menu du bas est visible
    if (m_uiOverlay && m_uiOverlay->bottomMenu()->isExpanded()) {
        by = height() - m_uiOverlay->bottomMenu()->expandedHeight() - 30;
    }
    
    p.setPen(QPen(Qt::white, 2));
    p.drawLine(bx, by, bx+barPx, by);
    p.drawLine(bx, by-5, bx, by+5);
    p.drawLine(bx+barPx, by-5, bx+barPx, by+5);
    QString lbl = (bestM>=1000) ? QString("%1 km").arg(bestM/1000) : QString("%1 m").arg(bestM);
    p.drawText(bx, by-8, lbl);
}

void MapView::screenToLonLat(const QPoint& screenPos, double& lon, double& lat) const{
    double wx = m_offsetX + screenPos.x();
    double wy = m_offsetY + screenPos.y();
    pixelToLonlat(wx, wy, m_zoom, lon, lat);
}
QPointF MapView::lonLatToScreen(double lon, double lat) const {
    double px, py;
    lonlatToPixel(lon, lat, m_zoom, px, py); // the inverse of pixelToLonlat
   return QPointF(px - m_offsetX, py - m_offsetY);
}

double MapView::metersPerPixelAtLat(double latDeg) const{
    const double R = 6378137.0;
    return std::cos(deg2rad(latDeg)) * 2.0 * M_PI * R / (256.0 * (1<<(int)m_zoom));
}

void MapView::lonlatToPixel(double lonDeg, double latDeg, int z, double& px, double& py){
    const double n = std::pow(2.0, z);
    const double latRad = deg2rad(latDeg);
    px = (lonDeg + 180.0) / 360.0 * 256.0 * n;
    py = (1.0 - std::log(std::tan(latRad) + 1.0/std::cos(latRad)) / M_PI) / 2.0 * 256.0 * n;
}

void MapView::pixelToLonlat(double px, double py, int z, double& lonDeg, double& latDeg){
    const double n = std::pow(2.0, z);
    lonDeg = px / (256.0 * n) * 360.0 - 180.0;
    const double y = M_PI * (1.0 - 2.0 * py / (256.0 * n));
    latDeg = rad2deg(std::atan(0.5*(std::exp(y) - std::exp(-y))));
}



//getters
double MapView::centerLon() const {
    double lon, lat;
    pixelToLonlat(width() / 2.0 - m_offsetX, height() / 2.0 - m_offsetY, m_zoom, lon, lat);
    return lon;
}

double MapView::centerLat() const {
    double lon, lat;
    pixelToLonlat(width() / 2.0 - m_offsetX, height() / 2.0 - m_offsetY, m_zoom, lon, lat);
    return lat;
}
