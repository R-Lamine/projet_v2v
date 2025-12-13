#include "vehicle_renderer.h"
#include <QPainterPath>
#include <QRandomGenerator>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <iostream>

// Variables statiques
std::unique_ptr<QSvgRenderer> VehicleRenderer::svgRenderer = nullptr;
QString VehicleRenderer::cachedSvgPath;
bool VehicleRenderer::initialized = false;

void VehicleRenderer::setSvgPath(const QString& path) {
    cachedSvgPath = path;
    initialized = false;  // Reset initialization flag so lazy init happens on first draw
}

void VehicleRenderer::lazyInitialize() {
    if (initialized || cachedSvgPath.isEmpty()) {
        return;
    }
    
    // Essayer plusieurs chemins possibles
    QStringList possiblePaths;
    
    // 1. Chemin absolu tel quel
    possiblePaths << cachedSvgPath;
    
    // 2. Relatif au répertoire de l'exécutable
    QString appDir = QCoreApplication::applicationDirPath();
    possiblePaths << appDir + "/" + cachedSvgPath;
    possiblePaths << appDir + "/../../" + cachedSvgPath;
    
    // 3. Chemins hardcodés pour le développement
    possiblePaths << "/home/soki/files/M1/S1/reseau/projet_v2v/" + cachedSvgPath;
    
    QString foundPath;
    for (const QString& path : possiblePaths) {
        if (QFileInfo::exists(path)) {
            foundPath = path;
            std::cout << "SVG trouvé: " << path.toStdString() << std::endl;
            break;
        }
    }
    
    if (foundPath.isEmpty()) {
        std::cerr << "Erreur: SVG introuvable. Chemins essayés:" << std::endl;
        for (const QString& path : possiblePaths) {
            std::cerr << "  - " << path.toStdString() << std::endl;
        }
        initialized = true;
        return;
    }
    
    svgRenderer = std::make_unique<QSvgRenderer>(foundPath);
    
    if (!svgRenderer->isValid()) {
        std::cerr << "Erreur: SVG invalide: " << foundPath.toStdString() << std::endl;
        svgRenderer = nullptr;
    } else {
        std::cout << "SVG chargé avec succès: " << foundPath.toStdString() << std::endl;
    }
    
    initialized = true;
}

void VehicleRenderer::drawVehicle(QPainter& painter,
                                 const QPointF& position,
                                 double heading,
                                 const QColor& color,
                                 double size) {
    // Lazy initialization on first draw
    if (!initialized) {
        lazyInitialize();
    }
    
    painter.save();
    painter.translate(position);
    // Le SVG pointe vers le haut (nord), heading 0° = nord
    // Qt rotation: 0° = droite (est), donc on doit ajuster
    painter.rotate(heading);

    if (svgRenderer && svgRenderer->isValid()) {
        // Rendu du SVG avec couleur
        QRectF svgRect(-size / 2, -size / 2, size, size);
        
        // Créer un pixmap pour le rendu
        int pixelSize = qMax(1, static_cast<int>(size * 2));  // Plus grande résolution
        QPixmap vehiclePixmap(pixelSize, pixelSize);
        vehiclePixmap.fill(Qt::transparent);
        
        // Renderer le SVG sur le pixmap
        QPainter pixmapPainter(&vehiclePixmap);
        pixmapPainter.setRenderHint(QPainter::Antialiasing);
        svgRenderer->render(&pixmapPainter);
        pixmapPainter.end();
        
        // Coloriser: remplacer les pixels noirs par la couleur
        QImage img = vehiclePixmap.toImage();
        for (int y = 0; y < img.height(); ++y) {
            for (int x = 0; x < img.width(); ++x) {
                QColor pixel = img.pixelColor(x, y);
                if (pixel.alpha() > 0) {
                    // Garder l'alpha, appliquer la couleur
                    img.setPixelColor(x, y, QColor(color.red(), color.green(), color.blue(), pixel.alpha()));
                }
            }
        }
        
        // Dessiner l'image colorisée
        painter.drawImage(svgRect, img);
    } else {
        // Fallback: dessiner une forme de voiture simple
        painter.setBrush(QBrush(color));
        painter.setPen(QPen(color.darker(150), 1));
        
        // Corps de la voiture (rectangle arrondi)
        QRectF body(-size/3, -size/2, size*2/3, size);
        painter.drawRoundedRect(body, size/6, size/6);
        
        // Indicateur avant (triangle)
        QPainterPath arrow;
        arrow.moveTo(0, -size/2);
        arrow.lineTo(-size/6, -size/3);
        arrow.lineTo(size/6, -size/3);
        arrow.closeSubpath();
        painter.setBrush(color.lighter(150));
        painter.drawPath(arrow);
    }

    painter.restore();
}
