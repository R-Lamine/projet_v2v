import QtQuick 2.15
import QtLocation 5.15
import QtPositioning 5.15

Rectangle {
    id: root
    color: "#1e1e1e"
    
    // Propriétés exposées
    property var mapItem: map
    
    // Fonction pour centrer la carte (appelée depuis C++)
    function setCenter(lat, lon, zoom) {
        map.center = QtPositioning.coordinate(lat, lon)
        map.zoomLevel = zoom
    }
    
    Plugin {
        id: mapPlugin
        name: "osm"
        
        // Configuration pour de meilleures performances
        PluginParameter {
            name: "osm.mapping.providersrepository.disabled"
            value: "false"
        }
        PluginParameter {
            name: "osm.mapping.cache.directory"
            value: "/tmp/osm_cache"
        }
        PluginParameter {
            name: "osm.mapping.cache.memory.size"
            value: "100"
        }
        PluginParameter {
            name: "osm.mapping.cache.disk.size"
            value: "50000"
        }
    }
    
    Map {
        id: map
        anchors.fill: parent
        plugin: mapPlugin
        center: QtPositioning.coordinate(48.5734, 7.7521)
        zoomLevel: 16
        
        // Activer le zoom et le pan
        gesture.enabled: true
        gesture.acceptedGestures: MapGestureArea.PanGesture | MapGestureArea.PinchGesture
        
        // Propriétés pour le drag
        property bool isDragging: false
        property real lastX: 0
        property real lastY: 0
        
        // Notifier C++ des changements
        onCenterChanged: {
            if (mapView) {
                mapView.onMapCenterChanged(center.latitude, center.longitude)
            }
        }
        
        onZoomLevelChanged: {
            if (mapView) {
                mapView.onMapZoomChanged(zoomLevel)
            }
        }
        
        // MouseArea pour contrôles additionnels
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            
            onPressed: (mouse) => {
                if (mouse.button === Qt.LeftButton) {
                    map.isDragging = true
                    map.lastX = mouse.x
                    map.lastY = mouse.y
                }
            }
            
            onReleased: {
                map.isDragging = false
            }
            
            onWheel: (wheel) => {
                if (wheel.angleDelta.y > 0) {
                    map.zoomLevel = Math.min(20, map.zoomLevel + 1)
                } else {
                    map.zoomLevel = Math.max(0, map.zoomLevel - 1)
                }
            }
        }
        
        // Dessiner les rayons de transmission (cercles jaunes)
        Repeater {
            model: mapView ? mapView.vehicleRanges : []
            
            MapCircle {
                center: QtPositioning.coordinate(modelData.lat, modelData.lon)
                radius: modelData.range
                color: "transparent"
                border.color: "#ffff00"
                border.width: 3
                opacity: 0.8
            }
        }
        
        // Dessiner les connexions transitives (lignes bleues pointillées)
        Repeater {
            model: mapView ? mapView.vehicleTransitiveConnections : []
            
            MapPolyline {
                line.color: "#0096ff"
                line.width: 2
                opacity: 0.6
                path: [
                    QtPositioning.coordinate(modelData.lat1, modelData.lon1),
                    QtPositioning.coordinate(modelData.lat2, modelData.lon2)
                ]
            }
        }
        
        // Dessiner les connexions directes (lignes vertes épaisses)
        Repeater {
            model: mapView ? mapView.vehicleConnections : []
            
            MapPolyline {
                line.color: "#00ff00"
                line.width: 3
                opacity: 0.9
                path: [
                    QtPositioning.coordinate(modelData.lat1, modelData.lon1),
                    QtPositioning.coordinate(modelData.lat2, modelData.lon2)
                ]
            }
        }
        
        // Dessiner les véhicules (cercles rouges)
        Repeater {
            model: mapView ? mapView.vehiclePositions : []
            
            MapQuickItem {
                coordinate: QtPositioning.coordinate(modelData.lat, modelData.lon)
                anchorPoint.x: vehicleMarker.width / 2
                anchorPoint.y: vehicleMarker.height / 2
                
                sourceItem: Rectangle {
                    id: vehicleMarker
                    width: 12
                    height: 12
                    radius: 6
                    color: "#ff0000"
                    border.color: "#8b0000"
                    border.width: 2
                }
            }
        }
    }
    
    // Contrôles de navigation (panneau en haut à droite)
    Rectangle {
        anchors {
            right: parent.right
            top: parent.top
            margins: 16
        }
        width: 100
        height: 120
        color: "#ffffffee"
        radius: 8
        border.color: "#888888"
        border.width: 1
        
        Column {
            anchors.centerIn: parent
            spacing: 8
            
            Rectangle {
                width: 80
                height: 30
                color: "#4CAF50"
                radius: 4
                
                Text {
                    anchors.centerIn: parent
                    text: "+"
                    font.pixelSize: 18
                    font.bold: true
                    color: "white"
                }
                
                MouseArea {
                    anchors.fill: parent
                    onClicked: map.zoomLevel = Math.min(20, map.zoomLevel + 1)
                    cursorShape: Qt.PointingHandCursor
                }
            }
            
            Rectangle {
                width: 80
                height: 30
                color: "#f44336"
                radius: 4
                
                Text {
                    anchors.centerIn: parent
                    text: "-"
                    font.pixelSize: 18
                    font.bold: true
                    color: "white"
                }
                
                MouseArea {
                    anchors.fill: parent
                    onClicked: map.zoomLevel = Math.max(0, map.zoomLevel - 1)
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }
    }
    
    // HUD info (en haut à gauche)
    Rectangle {
        anchors {
            left: parent.left
            top: parent.top
            margins: 12
        }
        width: infoText.width + 16
        height: infoText.height + 16
        color: "#000000cc"
        radius: 6
        
        Text {
            id: infoText
            anchors.centerIn: parent
            text: "Zoom " + map.zoomLevel.toFixed(1) + 
                  "  |  Lon " + map.center.longitude.toFixed(5) + 
                  "  Lat " + map.center.latitude.toFixed(5)
            font.pixelSize: 12
            color: "white"
        }
    }
}
