#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

class Simulator;

/**
 * @brief Barre supérieure avec effet de flou et contrôles principaux
 */
class TopBar : public QWidget {
    Q_OBJECT
public:
    explicit TopBar(QWidget* parent = nullptr);
    
    void setRunning(bool running);
    bool isRunning() const { return m_running; }
    void updateInfo(int zoom, double lon, double lat);
    
    void setDarkTheme(bool dark);
    void setHighQuality(bool hq);

signals:
    void startPauseClicked();
    void themeToggled(bool dark);
    void qualityToggled(bool highQuality);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QLabel* m_titleLabel;
    QPushButton* m_statusBadge;
    QLabel* m_infoLabel;
    QPushButton* m_startPauseBtn;
    QPushButton* m_themeBtn;
    QPushButton* m_qualityBtn;
    bool m_running = false;
    bool m_darkTheme = true;
    bool m_highQuality = false;
    
    void setupUI();
    void updateButtonStates();
};

/**
 * @brief Panneau de paramètres (gauche du menu inférieur)
 */
class ParametersPanel : public QWidget {
    Q_OBJECT
public:
    explicit ParametersPanel(QWidget* parent = nullptr);
    
    int getVehicleCount() const;
    int getTransmissionRange() const;
    int getLargeAntennaCount() const;
    int getSmallAntennaCount() const;
    int getVehicleSpeed() const;
    bool showConnections() const;
    bool showRanges() const;
    bool showTransitive() const;
    bool showRoads() const;
    
    void setVehicleCount(int count);  // Met à jour le slider sans émettre de signal

signals:
    void vehicleCountChanged(int count);
    void vehicleCountReleased(int count);  // Émis uniquement au relâchement du slider
    void transmissionRangeChanged(int range);
    void largeAntennaCountChanged(int count);
    void smallAntennaCountChanged(int count);
    void antennaConfigReleased(int numLarge, int numSmall);  // K-means auto au relâchement
    void vehicleSpeedChanged(int speed);  // Vitesse des véhicules (1-100 km/h)
    void showConnectionsChanged(bool show);
    void showRangesChanged(bool show);
    void showTransitiveChanged(bool show);
    void showRoadsChanged(bool show);

private:
    QSlider* m_vehicleSlider;
    QLabel* m_vehicleValue;
    QSlider* m_largeAntennaSlider;
    QLabel* m_largeAntennaValue;
    QSlider* m_smallAntennaSlider;
    QLabel* m_smallAntennaValue;
    QSlider* m_rangeSlider;
    QLabel* m_rangeValue;
    QSlider* m_speedSlider;
    QLabel* m_speedValue;
    QPushButton* m_connectionsToggle;
    QPushButton* m_rangesToggle;
    QPushButton* m_transitiveToggle;
    QPushButton* m_roadsToggle;
    
    void setupUI();
    QWidget* createSliderRow(const QString& icon, const QString& title, 
                             QSlider*& slider, QLabel*& valueLabel,
                             int min, int max, int initial, const QString& suffix);
    QWidget* createToggleRow(const QString& title, QPushButton*& toggle, bool initial);
};

/**
 * @brief Panneau de statistiques (droite du menu inférieur)
 */
class StatsPanel : public QWidget {
    Q_OBJECT
public:
    explicit StatsPanel(QWidget* parent = nullptr);
    
    void updateStats(int activeVehicles, int connectedVehicles, 
                     int totalConnections, double connectionRate,
                     int comparisons, double avgNeighbors, double buildTimeMs);

private:
    QLabel* m_activeVehicles;
    QLabel* m_connectedVehicles;
    QLabel* m_totalConnections;
    QLabel* m_connectionRate;
    QLabel* m_comparisons;
    QLabel* m_avgNeighbors;
    QLabel* m_buildTime;
    
    void setupUI();
    QWidget* createStatRow(const QString& title, QLabel*& valueLabel, const QString& color = "white");
};

/**
 * @brief Menu inférieur rétractable
 */
class BottomMenu : public QWidget {
    Q_OBJECT
public:
    explicit BottomMenu(QWidget* parent = nullptr);
    
    void toggle();
    bool isExpanded() const { return m_expanded; }
    
    ParametersPanel* parametersPanel() { return m_paramsPanel; }
    StatsPanel* statsPanel() { return m_statsPanel; }
    QPropertyAnimation* animation() { return m_animation; }
    int expandedHeight() const { return m_expandedHeight; }

signals:
    void expansionChanged(bool expanded);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    ParametersPanel* m_paramsPanel;
    StatsPanel* m_statsPanel;
    QPropertyAnimation* m_animation;
    bool m_expanded = true;
    int m_expandedHeight = 380;
    int m_collapsedHeight = 0;
    
    void setupUI();
};

/**
 * @brief Boutons de zoom flottants (droite)
 */
class ZoomControls : public QWidget {
    Q_OBJECT
public:
    explicit ZoomControls(QWidget* parent = nullptr);

signals:
    void zoomIn();
    void zoomOut();

private:
    void setupUI();
};

/**
 * @brief Overlay principal qui contient tous les éléments UI
 */
class UIOverlay : public QWidget {
    Q_OBJECT
public:
    explicit UIOverlay(QWidget* parent = nullptr);
    
    TopBar* topBar() { return m_topBar; }
    BottomMenu* bottomMenu() { return m_bottomMenu; }
    ZoomControls* zoomControls() { return m_zoomControls; }
    
    void setSimulator(Simulator* sim) { m_simulator = sim; }
    void updateStats();
    void updateMapInfo(int zoom, double lon, double lat);
    
    // Gestion du bouton supprimer véhicule
    void showDeleteVehicleButton(bool show);
    
signals:
    void deleteTrackedVehicle();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    TopBar* m_topBar;
    BottomMenu* m_bottomMenu;
    ZoomControls* m_zoomControls;
    QPushButton* m_menuToggleBtn;
    QPushButton* m_deleteVehicleBtn;  // Bouton pour supprimer le véhicule suivi
    Simulator* m_simulator = nullptr;
    
    void setupUI();
    void repositionElements();
    void updateToggleButtonIcon(bool menuExpanded);
};
