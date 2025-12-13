#pragma once

#include <QPointF>
#include <QPainter>
#include <QPixmap>
#include <QImage>
#include <QSvgRenderer>
#include <cmath>
#include <memory>
#include <QString>

/**
 * @brief Classe responsable du rendu des véhicules sous forme de SVG
 * 
 * Cette classe gère le dessin des véhicules comme de petites voitures orientées
 * selon leur direction de déplacement. Utilise une image SVG chargée depuis le disque.
 */
class VehicleRenderer {
public:
    VehicleRenderer() = default;
    ~VehicleRenderer() = default;

    /**
     * @brief Définit le chemin vers l'image SVG (lazy initialization)
     * @param svgPath Chemin vers le fichier SVG (ex: "data/top_view_icon.svg")
     */
    static void setSvgPath(const QString& svgPath);

    /**
     * @brief Dessine un véhicule à une position et orientation données
     * 
     * @param painter Le QPainter à utiliser pour le dessin
     * @param position Position du centre du véhicule (en pixels écran)
     * @param heading Direction du véhicule en degrés (0° = vers le haut)
     * @param color Couleur du véhicule
     * @param size Taille approximative du véhicule en pixels
     */
    static void drawVehicle(QPainter& painter, 
                           const QPointF& position,
                           double heading,
                           const QColor& color,
                           double size = 12.0);

private:
    /**
     * @brief Initialise le SVG la première fois qu'il est nécessaire
     */
    static void lazyInitialize();

    /**
     * @brief Convertit les degrés en radians
     */
    static constexpr double degreesToRadians(double degrees) {
        return degrees * M_PI / 180.0;
    }
    
    static std::unique_ptr<QSvgRenderer> svgRenderer;
    static QString cachedSvgPath;
    static bool initialized;
};
