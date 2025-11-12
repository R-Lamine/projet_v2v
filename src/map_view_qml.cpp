#include "map_view_qml.h"
#include "simulator.h"
#include "vehicule.h"
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QUrl>
#include <QDebug>

MapViewQML::MapViewQML(QWidget* parent)
    : QQuickWidget(parent) {
    
    // Configuration du widget QML
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    
    // Timer pour mettre à jour l'affichage régulièrement
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(50); // 20 FPS
    connect(m_updateTimer, &QTimer::timeout, this, &MapViewQML::updateVehicles);
    
    // Charger le fichier QML
    setupQmlContext();
    setSource(QUrl(QStringLiteral("qrc:/MapComponent.qml")));
    
    if (status() == QQuickWidget::Error) {
        qWarning() << "Erreur lors du chargement du QML";
        for (const auto& err : errors()) {
            qWarning() << err.toString();
        }
    }
}

void MapViewQML::setupQmlContext() {
    // Exposer ce widget au contexte QML
    rootContext()->setContextProperty("mapView", this);
}

void MapViewQML::setSimulator(Simulator* sim) {
    m_simulator = sim;
    if (m_simulator) {
        // Connecter le signal du simulateur pour rafraîchir l'affichage
        connect(m_simulator, &Simulator::ticked, this, &MapViewQML::updateVehicles);
        m_updateTimer->start();
    }
}

void MapViewQML::setCenterLonLat(double lon, double lat, int zoom) {
    m_centerLon = lon;
    m_centerLat = lat;
    m_zoom = zoom;
    
    // Appeler la fonction QML pour centrer la carte
    QQuickItem* rootItem = rootObject();
    if (rootItem) {
        QMetaObject::invokeMethod(rootItem, "setCenter",
            Q_ARG(QVariant, lat),
            Q_ARG(QVariant, lon),
            Q_ARG(QVariant, zoom));
    }
}

QVariantList MapViewQML::vehiclePositions() const {
    QVariantList positions;
    
    if (!m_simulator) {
        return positions;
    }
    
    const auto& vehicles = m_simulator->vehicles();
    for (const auto* vehicle : vehicles) {
        if (!vehicle) continue;
        
        auto [lat, lon] = vehicle->getPosition();
        
        QVariantMap vehicleData;
        vehicleData["id"] = vehicle->getId();
        vehicleData["lat"] = lat;
        vehicleData["lon"] = lon;
        
        positions.append(vehicleData);
    }
    
    return positions;
}

QVariantList MapViewQML::vehicleConnections() const {
    QVariantList connections;
    
    if (!m_simulator) {
        return connections;
    }
    
    const auto& vehicles = m_simulator->vehicles();
    const auto& interfGraph = m_simulator->interferenceGraph();
    
    for (const auto* vehicle : vehicles) {
        if (!vehicle) continue;
        
        auto directNeighbors = interfGraph.getDirectNeighbors(vehicle->getId());
        auto [lat1, lon1] = vehicle->getPosition();
        
        for (int neighborId : directNeighbors) {
            // Éviter les doublons (ne dessiner qu'une fois par paire)
            if (vehicle->getId() >= neighborId) continue;
            
            // Trouver le véhicule voisin
            for (const auto* neighbor : vehicles) {
                if (neighbor && neighbor->getId() == neighborId) {
                    auto [lat2, lon2] = neighbor->getPosition();
                    
                    QVariantMap connection;
                    connection["lat1"] = lat1;
                    connection["lon1"] = lon1;
                    connection["lat2"] = lat2;
                    connection["lon2"] = lon2;
                    
                    connections.append(connection);
                    break;
                }
            }
        }
    }
    
    return connections;
}

QVariantList MapViewQML::vehicleTransitiveConnections() const {
    QVariantList connections;
    
    if (!m_simulator) {
        return connections;
    }
    
    const auto& vehicles = m_simulator->vehicles();
    const auto& interfGraph = m_simulator->interferenceGraph();
    
    for (const auto* vehicle : vehicles) {
        if (!vehicle) continue;
        
        auto directNeighbors = interfGraph.getDirectNeighbors(vehicle->getId());
        auto allReachable = interfGraph.getReachableVehicles(vehicle->getId());
        auto [lat1, lon1] = vehicle->getPosition();
        
        for (int reachableId : allReachable) {
            // Ignorer les voisins directs (ils sont dans l'autre liste)
            if (directNeighbors.find(reachableId) != directNeighbors.end()) {
                continue;
            }
            
            // Éviter les doublons
            if (vehicle->getId() >= reachableId) continue;
            
            // Trouver le véhicule accessible
            for (const auto* reachable : vehicles) {
                if (reachable && reachable->getId() == reachableId) {
                    auto [lat2, lon2] = reachable->getPosition();
                    
                    QVariantMap connection;
                    connection["lat1"] = lat1;
                    connection["lon1"] = lon1;
                    connection["lat2"] = lat2;
                    connection["lon2"] = lon2;
                    
                    connections.append(connection);
                    break;
                }
            }
        }
    }
    
    return connections;
}

QVariantList MapViewQML::vehicleRanges() const {
    QVariantList ranges;
    
    if (!m_simulator) {
        return ranges;
    }
    
    const auto& vehicles = m_simulator->vehicles();
    for (const auto* vehicle : vehicles) {
        if (!vehicle) continue;
        
        auto [lat, lon] = vehicle->getPosition();
        
        QVariantMap rangeData;
        rangeData["lat"] = lat;
        rangeData["lon"] = lon;
        rangeData["range"] = vehicle->getTransmissionRange();
        
        ranges.append(rangeData);
    }
    
    return ranges;
}

void MapViewQML::updateVehicles() {
    // Notifier QML que les données ont changé
    emit vehiclePositionsChanged();
    emit vehicleConnectionsChanged();
    emit vehicleTransitiveConnectionsChanged();
    emit vehicleRangesChanged();
}

void MapViewQML::onMapCenterChanged(double lat, double lon) {
    m_centerLat = lat;
    m_centerLon = lon;
    
    emit cursorInfoChanged(QString("Zoom %1  |  Centre Lon %2  Lat %3")
        .arg(m_zoom).arg(lon, 0, 'f', 5).arg(lat, 0, 'f', 5));
}

void MapViewQML::onMapZoomChanged(double zoom) {
    m_zoom = static_cast<int>(zoom);
    
    emit cursorInfoChanged(QString("Zoom %1  |  Centre Lon %2  Lat %3")
        .arg(m_zoom).arg(m_centerLon, 0, 'f', 5).arg(m_centerLat, 0, 'f', 5));
}
