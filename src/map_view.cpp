#include "map_view.h"
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
#include <algorithm>
#include <cmath>

#include "simulator.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static inline double deg2rad(double d){ return d * M_PI / 180.0; }
static inline double rad2deg(double r){ return r * 180.0 / M_PI; }

MapView::MapView(QWidget* parent)
    : QWidget(parent), m_memCache(1024) {
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(30,30,30));
    setPalette(pal);

    connect(&m_net, &QNetworkAccessManager::finished, this, [this](QNetworkReply* rep){
        rep->deleteLater();
    });
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

    //Draw vehicules on map
    if (m_simulator) {
        const auto& vehicles = m_simulator->vehicles();
        const auto& interfGraph = m_simulator->interferenceGraph();

        // Dessiner d'abord les rayons de transmission (cercles jaunes)
        for (auto* v : vehicles) {
            if (!v) continue;
            
            auto [lat, lon] = v->getPosition();
            QPointF pt = lonLatToScreen(lon, lat);
            
            // Calculer le rayon en pixels
            double range = v->getTransmissionRange(); // en mètres
            double mpp = metersPerPixelAtLat(lat);
            double radiusPixels = range / mpp;
            
            // Dessiner un cercle semi-transparent pour le rayon
            QPen rangePen(QColor(255, 255, 0, 255)); // Jaune semi-transparent
            rangePen.setWidth(3); // Épaisseur augmentée à 3 pixels
            p.setPen(rangePen);
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(pt, radiusPixels, radiusPixels);
        }

        // Dessiner d'abord les connexions transitives (lignes bleues pointillées)
        QPen transitivePen(QColor(0, 150, 255, 255)); // Bleu semi-transparent
        transitivePen.setWidth(3);
        transitivePen.setStyle(Qt::DashLine); // Ligne pointillée
        p.setPen(transitivePen);

        for (auto* v : vehicles) {
            if (!v) continue;
            
            auto directNeighbors = interfGraph.getDirectNeighbors(v->getId());
            auto allReachable = interfGraph.getReachableVehicles(v->getId());
            auto [lat1, lon1] = v->getPosition();
            QPointF pt1 = lonLatToScreen(lon1, lat1);

            // Dessiner les connexions transitives (accessibles mais pas directs)
            for (int reachableId : allReachable) {
                // Si c'est un voisin direct, on le saute (sera dessiné après)
                if (directNeighbors.find(reachableId) != directNeighbors.end()) {
                    continue;
                }
                
                // Trouver le véhicule accessible
                for (auto* reachable : vehicles) {
                    if (reachable && reachable->getId() == reachableId) {
                        auto [lat2, lon2] = reachable->getPosition();
                        QPointF pt2 = lonLatToScreen(lon2, lat2);
                        
                        // Ne dessiner la ligne qu'une seule fois
                        if (v->getId() < reachableId) {
                            p.drawLine(pt1, pt2);
                        }
                        break;
                    }
                }
            }
        }

        // Dessiner ensuite les connexions directes (lignes vertes épaisses)
        QPen connectionPen(QColor(0, 255, 0, 255)); // Vert visible
        connectionPen.setWidth(2);
        p.setPen(connectionPen);

        // Dessiner les connexions directes
        for (auto* v : vehicles) {
            if (!v) continue;
            
            auto directNeighbors = interfGraph.getDirectNeighbors(v->getId());
            auto [lat1, lon1] = v->getPosition();
            QPointF pt1 = lonLatToScreen(lon1, lat1);

            for (int neighborId : directNeighbors) {
                // Trouver le véhicule voisin
                for (auto* neighbor : vehicles) {
                    if (neighbor && neighbor->getId() == neighborId) {
                        auto [lat2, lon2] = neighbor->getPosition();
                        QPointF pt2 = lonLatToScreen(lon2, lat2);
                        
                        // Ne dessiner la ligne qu'une seule fois (éviter les doublons)
                        if (v->getId() < neighborId) {
                            p.drawLine(pt1, pt2);
                        }
                        break;
                    }
                }
            }
        }

        // Dessiner les véhicules (points rouges) par-dessus tout
        for (auto* v : vehicles) {
            if (!v) continue;
            
            auto [lat, lon] = v->getPosition();
            QPointF pt = lonLatToScreen(lon, lat);
            
            // Cercle rouge pour le véhicule
            p.setBrush(QBrush(Qt::red));
            p.setPen(QPen(Qt::darkRed, 2));
            p.drawEllipse(pt, 6, 6);  // Point rouge pour le véhicule
        }
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
        default: QWidget::keyPressEvent(ev); break;
    }
}

void MapView::resizeEvent(QResizeEvent*){}

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

    if(QPixmap* cached = m_memCache.object(url)){
        return;
    }

    TileKey key{z,x,y};
    if(m_inflight.contains(key)) return;

    if(url.startsWith("file://")){
        const QString path = QUrl(url).toLocalFile();
        if(QFileInfo::exists(path)){
            QPixmap* px = new QPixmap();
            if(px->load(path)){
                m_memCache.insert(url, px);
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
                m_memCache.insert(url, px);
                update();
            } else delete px;
        }
        rep->deleteLater();
    });
}

void MapView::drawTiles(QPainter& p){
    const int T = 256;
    const int n = 1 << m_zoom;

    int x0 = int(std::floor(m_offsetX / T));
    int y0 = int(std::floor(m_offsetY / T));
    int nx = int(std::ceil((m_offsetX + width()) / T)) - x0;
    int ny = int(std::ceil((m_offsetY + height()) / T)) - y0;

    p.fillRect(rect(), QColor(20,20,20));

    for(int dy=0; dy<=ny; ++dy){
        for(int dx=0; dx<=nx; ++dx){
            int tx = x0 + dx;
            int ty = y0 + dy;

            int txWrap = ((tx % n) + n) % n;
            if(ty < 0 || ty >= n) continue;

            const QString url = buildUrl(m_zoom, txWrap, ty);
            QPixmap* cached = m_memCache.object(url);
            const QRectF target(tx*T - m_offsetX, ty*T - m_offsetY, T, T);

            if(!cached){
                requestTile(m_zoom, txWrap, ty);
                p.fillRect(target, QColor(60,60,60));
            } else {
                p.drawPixmap(target, *cached, QRectF(0,0,T,T));
            }
        }
    }
}

void MapView::drawHUD(QPainter& p){
    const int margin = 12;
    const int pad = 8;
    QFont f = p.font();
    f.setPointSizeF(f.pointSizeF()*0.95);
    p.setFont(f);

    double lonC, latC;
    screenToLonLat(QPoint(width()/2, height()/2), lonC, latC);
    QString info = QString("Zoom %1  |  Centre Lon %2  Lat %3")
        .arg(m_zoom).arg(lonC,0,'f',5).arg(latC,0,'f',5);

    QFontMetrics fm(p.font());
    int w = fm.horizontalAdvance(info) + 2*pad;
    int h = fm.height() + 2*pad;
    QRect box(width()-w-margin, margin, w, h);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0,0,0,140));
    p.drawRoundedRect(box, 6, 6);
    p.setPen(Qt::white);
    p.drawText(box.adjusted(pad,pad,-pad,-pad), Qt::AlignLeft|Qt::AlignVCenter, info);

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
    return std::cos(deg2rad(latDeg)) * 2.0 * M_PI * R / (256.0 * (1<<m_zoom));
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
