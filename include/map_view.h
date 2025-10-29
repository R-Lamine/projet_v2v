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
    // ---- Fallback image ----
    QPixmap m_base;

    // ---- Tuiles XYZ ----
    QString m_tilesTemplate;
    QNetworkAccessManager m_net;
    QCache<QString, QPixmap> m_memCache;                   // LRU cache (clé = URL)
    QHash<TileKey, QPointer<QNetworkReply>> m_inflight;    // téléchargements en cours

    // ---- Vue ----
    int m_zoom = 13;
    double m_offsetX = 0.0; // monde->écran (pixels)
    double m_offsetY = 0.0;
    bool m_dragging = false;
    QPoint m_lastPos;

    // ---- Réseau (option) ----
    QString m_userAgent = "V2V-Simulator/1.0 (contact: student@example.edu)";
    QString m_referer   = "https://university.example/course/v2v";
    qint64  m_minRequestIntervalMs = 100;  // ~10 req/s max
    qint64  m_lastRequestMs        = 0;

    // ---- Utils ----
    static void lonlatToPixel(double lonDeg, double latDeg, int z, double& px, double& py);
    static void pixelToLonlat(double px, double py, int z, double& lonDeg, double& latDeg);
    void zoomAt(const QPoint& screenPos, double factor);
    void drawTiles(QPainter& p);
    void drawHUD(QPainter& p);
    void requestTile(int z,int x,int y);
    QString buildUrl(int z,int x,int y) const;
    void setCenterWorld(double px, double py, int zoom);

    void screenToLonLat(const QPoint& screenPos, double& lon, double& lat) const;
    double metersPerPixelAtLat(double latDeg) const;
};

