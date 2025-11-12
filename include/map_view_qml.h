#pragma once
#include <QQuickWidget>
#include <QQuickItem>
#include <QGeoCoordinate>
#include <QTimer>
#include <vector>

class Simulator;
class Vehicule;

class MapViewQML : public QQuickWidget {
    Q_OBJECT
    Q_PROPERTY(QVariantList vehiclePositions READ vehiclePositions NOTIFY vehiclePositionsChanged)
    Q_PROPERTY(QVariantList vehicleConnections READ vehicleConnections NOTIFY vehicleConnectionsChanged)
    Q_PROPERTY(QVariantList vehicleTransitiveConnections READ vehicleTransitiveConnections NOTIFY vehicleTransitiveConnectionsChanged)
    Q_PROPERTY(QVariantList vehicleRanges READ vehicleRanges NOTIFY vehicleRangesChanged)

public:
    explicit MapViewQML(QWidget* parent = nullptr);
    
    // Setters
    void setSimulator(Simulator* sim);
    void setCenterLonLat(double lon, double lat, int zoom);
    
    // Getters pour QML
    QVariantList vehiclePositions() const;
    QVariantList vehicleConnections() const;
    QVariantList vehicleTransitiveConnections() const;
    QVariantList vehicleRanges() const;
    
    // Getter pour le simulateur
    int zoomLevel() const { return m_zoom; }
    double centerLon() const { return m_centerLon; }
    double centerLat() const { return m_centerLat; }

signals:
    void cursorInfoChanged(const QString& text);
    void vehiclePositionsChanged();
    void vehicleConnectionsChanged();
    void vehicleTransitiveConnectionsChanged();
    void vehicleRangesChanged();

public slots:
    void updateVehicles();
    void onMapCenterChanged(double lat, double lon);
    void onMapZoomChanged(double zoom);

private:
    Simulator* m_simulator = nullptr;
    QTimer* m_updateTimer = nullptr;
    
    int m_zoom = 16;
    double m_centerLon = 7.7521;
    double m_centerLat = 48.5734;
    
    void setupQmlContext();
};
