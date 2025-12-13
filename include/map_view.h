#pragma once
#include <QWidget>
#include <QPixmap>
#include <QPointF>
#include <QHash>
#include <QCache>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QTimer>
#include <vector>

class Simulator;
class UIOverlay;
class Vehicule;

struct TileKey {
    int z;
    int x;
    int y;
    bool operator==(const TileKey& o) const noexcept { return z==o.z && x==o.x && y==o.y; }
};
inline uint qHash(const TileKey &k, uint seed=0) noexcept {
    return qHash((k.z*73856093) ^ (k.x*19349663) ^ (k.y*83492791), seed);
}

class MapView : public QWidget {
    Q_OBJECT
public:
    explicit MapView(QWidget* parent=nullptr);

    // Image test (fallback offline)
    bool loadImage(const QString& path);

    // Schéma de tuiles XYZ: "https://.../{z}/{x}/{y}.png" ou "file:///.../{z}/{x}/{y}.png"
    void setTilesTemplate(const QString& pattern);

    // Centrer sur lon/lat (Web Mercator), zoom entier [0..20]
    void setCenterLonLat(double lonDeg, double latDeg, int zoom);

    // (Option) identité réseau et simple rate-limit si tu utilises un serveur qui l'exige
    void setNetworkIdentity(const QString& ua, const QString& ref) { m_userAgent = ua; m_referer = ref; }
    void setRequestRateLimitMs(qint64 ms) { m_minRequestIntervalMs = ms; }

    //getter
    int zoomLevel() const { return m_zoom; }
    double centerLon() const;
    double centerLat() const;
    double getOffsetX() const {return m_offsetX;}
    double getOffsetY() const {return m_offsetY;}

    //setter
    void setSimulator(Simulator* sim);

    //util
    static void lonlatToPixel(double lonDeg, double latDeg, int z, double& px, double& py);
    
    // UI Overlay
    UIOverlay* uiOverlay() { return m_uiOverlay; }
    
    // Zoom public pour l'overlay
    void zoomIn();
    void zoomOut();

signals:
    void cursorInfoChanged(const QString& text);

protected:
    void paintEvent(QPaintEvent* ev) override;
    void wheelEvent(QWheelEvent* ev) override;
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;
    void keyPressEvent(QKeyEvent* ev) override;
    void resizeEvent(QResizeEvent* ev) override;

private:
    // --
    Simulator* m_simulator = nullptr;
    UIOverlay* m_uiOverlay = nullptr;

    // ---- Fallback image ----
    QPixmap m_base;

    // ---- Display toggles ----
    bool m_showTransitiveConnections = false;
    bool m_drawDirectConnections = true;
    bool m_showRanges = true;
    bool m_showRoads = false;
    
    // ---- Cache pour les routes (optimisation) ----
    struct RoadSegment {
        double lon1, lat1, lon2, lat2;
    };
    std::vector<RoadSegment> m_validRoads;  // Pré-calculé une fois
    bool m_roadsPrecomputed = false;

    // ---- Toggle low quality tiles mode ----
    bool m_lowQualityMode = true;

    // ---- Toggle dark theme ----
    bool m_darkTheme = true;
    QString m_darkTilesTemplate = "https://basemaps.cartocdn.com/dark_all/{z}/{x}/{y}.png";
    QString m_lightTilesTemplate = "https://tile.openstreetmap.org/{z}/{x}/{y}.png";

    // ---- Tuiles XYZ ----
    QString m_tilesTemplate;
    QNetworkAccessManager m_net;
    QCache<QString, QPixmap> m_darkCache;                  // Cache pour tuiles sombres
    QCache<QString, QPixmap> m_lightCache;                 // Cache pour tuiles claires
    QHash<TileKey, QPointer<QNetworkReply>> m_inflight;    // téléchargements en cours
    
    // Helper pour obtenir le cache actif selon le thème
    QCache<QString, QPixmap>& getActiveCache() { return m_darkTheme ? m_darkCache : m_lightCache; }

    // ---- Vue ----
    int m_zoom = 13;
    double m_offsetX = 0.0; // monde->écran (pixels)
    double m_offsetY = 0.0;
    bool m_dragging = false;
    bool m_didDrag = false;  // True si on a bougé pendant le drag
    QPoint m_lastPos;
    QPoint m_pressPos;  // Position initiale du clic
    
    // ---- Suivi de véhicule ----
    Vehicule* m_trackedVehicle = nullptr;  // Véhicule suivi par la caméra
    bool m_followingVehicle = false;       // True si on suit un véhicule

    // ---- Réseau (option) ----
    QString m_userAgent = "V2V-Simulator/1.0 (contact: student@example.edu)";
    QString m_referer   = "https://university.example/course/v2v";
    qint64  m_minRequestIntervalMs = 100;  // ~10 req/s max
    qint64  m_lastRequestMs        = 0;

    // ---- Utils ----

    static void pixelToLonlat(double px, double py, int z, double& lonDeg, double& latDeg);
    void zoomAt(const QPoint& screenPos, double factor);
    void drawTiles(QPainter& p);
    void drawHUD(QPainter& p);
    void requestTile(int z,int x,int y);
    QString buildUrl(int z,int x,int y) const;
    void setCenterWorld(double px, double py, int zoom);

    void screenToLonLat(const QPoint& screenPos, double& lon, double& lat) const;
    void screenToLonLat(int x, int y, double& lon, double& lat) const {
        screenToLonLat(QPoint(x, y), lon, lat);
    }
    QPointF lonLatToScreen(double lon, double lat) const;
    double metersPerPixelAtLat(double latDeg) const;
};

