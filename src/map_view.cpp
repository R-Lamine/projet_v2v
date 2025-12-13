#include "map_view.h"
#include "vehicle_renderer.h"
#include "overlay_ui.h"
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
    
    connect(m_uiOverlay->zoomControls(), &ZoomControls::zoomIn, this, &MapView::zoomIn);
    connect(m_uiOverlay->zoomControls(), &ZoomControls::zoomOut, this, &MapView::zoomOut);
    
    // Connecter les toggles du panneau de paramètres
    auto* params = m_uiOverlay->bottomMenu()->parametersPanel();
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
    connect(params, &ParametersPanel::transmissionRangeChanged, this, [this](int range) {
        if (m_simulator) {
            // Mettre à jour le rayon de transmission de tous les véhicules
            for (auto* v : m_simulator->vehicles()) {
                v->setTransmissionRange(range);
            }
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
            for (auto* v : visibleVehicles) {
                auto [lat, lon] = v->getPosition();
                QPointF pt = lonLatToScreen(lon, lat);
                
                // Calculer le rayon en pixels
                double range = v->getTransmissionRange();
                double mpp = metersPerPixelAtLat(lat);
                double radiusPixels = range / mpp;
                
                // Dessiner un cercle semi-transparent pour le rayon (cyan doux, très léger)
                QPen rangePen(QColor(100, 200, 220, 80));  // Cyan clair, très très transparent
                rangePen.setWidth(1);
                p.setPen(rangePen);
                p.setBrush(QColor(100, 200, 220, 5));  // Remplissage quasi invisible
                p.drawEllipse(pt, radiusPixels, radiusPixels);
            }
        }

        // Dessiner les connexions uniquement si peu de véhicules visibles
        if (drawDetails && visibleVehicles.size() < 200) {
            // Dessiner d'abord les connexions transitives (lignes bleues pointillées) si activé
            if (m_showTransitiveConnections) {
                QPen transitivePen(QColor(147, 112, 219, 120));  // Violet moyen, semi-transparent
                transitivePen.setWidth(1);
                transitivePen.setStyle(Qt::DashLine);
                p.setPen(transitivePen);

                for (auto* v : visibleVehicles) {
                    auto directNeighbors = interfGraph.getDirectNeighbors(v->getId());
                    auto allReachable = interfGraph.getReachableVehicles(v->getId());
                    auto [lat1, lon1] = v->getPosition();
                    QPointF pt1 = lonLatToScreen(lon1, lat1);

                    
                    // Dessiner les connexions transitives (accessibles mais pas directs)
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
                QPen connectionPen(QColor(135, 206, 235, 150));  // Bleu clair, semi-transparent
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

        // Dessiner les véhicules comme des SVG orientés
        // Taille proportionnelle au zoom (plus grand quand on zoom)
        double baseSize = 16.0;  // Taille de base
        double zoomFactor = std::pow(1.15, m_zoom - 16);  // Zoom 14 = taille normale
        double vehicleSize = std::clamp(baseSize * zoomFactor, 6.0, 100.0);  // Min 6, Max 40 pixels
        
        for (auto* v : visibleVehicles) {
            auto [lat, lon] = v->getPosition();
            QPointF pt = lonLatToScreen(lon, lat);
            
            // Obtenir la direction du véhicule (heading)
            double heading = v->getHeading();
            
            // Générer une couleur aléatoire basée sur l'ID du véhicule pour cohérence
            QRandomGenerator gen(v->getId());
            QColor vehicleColor(
                gen.bounded(120, 220),  // R: 100-255
                gen.bounded(120, 220),  // G: 100-255
                gen.bounded(120, 220),  // B: 100-255
                255  // Opacité élevée (sur 255)
            );
            
            // Utiliser le VehicleRenderer pour dessiner le véhicule
            VehicleRenderer::drawVehicle(p, pt, heading, vehicleColor, vehicleSize);
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
        m_lastPos = ev->pos();
    }
}

void MapView::mouseMoveEvent(QMouseEvent* ev){
    if(m_dragging){
        QPoint d = ev->pos() - m_lastPos;
        m_offsetX -= d.x();
        m_offsetY -= d.y();
        m_lastPos = ev->pos();
        update();
    }
    double lon, lat;
    screenToLonLat(ev->pos(), lon, lat);

    // ✅ fix du message (évite "QString::arg: Argument missing")
    emit cursorInfoChanged(QString("Zoom %1  |  Lon %2  Lat %3")
        .arg(m_zoom).arg(lon,0,'f',5).arg(lat,0,'f',5));
}

void MapView::mouseReleaseEvent(QMouseEvent* ev){
    if(ev->button()==Qt::LeftButton) m_dragging = false;
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
