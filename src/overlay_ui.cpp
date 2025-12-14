#include "overlay_ui.h"
#include "simulator.h"
#include "vehicule.h"
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsBlurEffect>
#include <QFile>
#include <QCoreApplication>
#include <QTransform>

// ============================================================================
// Styles CSS globaux
// ============================================================================
static const QString BUTTON_STYLE = R"(
    QPushButton {
        background-color: rgba(55, 65, 81, 0.9);
        border: 1px solid rgba(75, 85, 99, 0.5);
        border-radius: 8px;
        color: white;
        padding: 8px 16px;
        font-size: 13px;
        font-weight: 500;
    }
    QPushButton:hover {
        background-color: rgba(75, 85, 99, 0.95);
        border-color: rgba(99, 102, 241, 0.5);
    }
    QPushButton:pressed {
        background-color: rgba(55, 65, 81, 1);
    }
)";

static const QString PRIMARY_BUTTON_STYLE = R"(
    QPushButton {
        background-color: rgba(99, 102, 241, 0.9);
        border: none;
        border-radius: 8px;
        color: white;
        padding: 8px 20px;
        font-size: 13px;
        font-weight: 600;
    }
    QPushButton:hover {
        background-color: rgba(129, 140, 248, 0.95);
    }
    QPushButton:pressed {
        background-color: rgba(79, 70, 229, 1);
    }
)";

static const QString TOGGLE_ON_STYLE = R"(
    QPushButton {
        background-color: rgba(99, 102, 241, 0.9);
        border: none;
        border-radius: 12px;
        min-width: 44px;
        max-width: 44px;
        min-height: 24px;
        max-height: 24px;
    }
)";

static const QString TOGGLE_OFF_STYLE = R"(
    QPushButton {
        background-color: rgba(75, 85, 99, 0.7);
        border: none;
        border-radius: 12px;
        min-width: 44px;
        max-width: 44px;
        min-height: 24px;
        max-height: 24px;
    }
)";

static const QString SLIDER_STYLE = R"(
    QSlider::groove:horizontal {
        border: none;
        height: 4px;
        background: rgba(75, 85, 99, 0.6);
        border-radius: 2px;
    }
    QSlider::handle:horizontal {
        background: white;
        border: none;
        width: 16px;
        height: 16px;
        margin: -6px 0;
        border-radius: 8px;
    }
    QSlider::sub-page:horizontal {
        background: rgba(99, 102, 241, 0.9);
        border-radius: 2px;
    }
)";

static const QString ZOOM_BUTTON_STYLE = R"(
    QPushButton {
        background-color: rgba(30, 30, 35, 0.85);
        border: 1px solid rgba(75, 85, 99, 0.4);
        border-radius: 8px;
        color: white;
        font-size: 18px;
        font-weight: bold;
        min-width: 36px;
        max-width: 36px;
        min-height: 36px;
        max-height: 36px;
    }
    QPushButton:hover {
        background-color: rgba(50, 50, 55, 0.9);
        border-color: rgba(99, 102, 241, 0.5);
    }
)";

// Helper pour trouver les ressources
static QString findResource(const QString& filename) {
    QStringList paths = {
        filename,
        "data/" + filename,
        "../../data/" + filename,
        QCoreApplication::applicationDirPath() + "/data/" + filename,
        QCoreApplication::applicationDirPath() + "/../../data/" + filename
    };
    for (const QString& path : paths) {
        if (QFile::exists(path)) return path;
    }
    return filename;
}

// ============================================================================
// TopBar Implementation
// ============================================================================
TopBar::TopBar(QWidget* parent) : QWidget(parent) {
    setFixedHeight(56);
    setAttribute(Qt::WA_TranslucentBackground);
    setupUI();
}

void TopBar::setupUI() {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 8, 20, 8);
    layout->setSpacing(12);
    
    // Logo et titre
    auto* logoLayout = new QHBoxLayout();
    logoLayout->setSpacing(10);
    
    QLabel* logoLabel = new QLabel(this);
    QPixmap radioPix(findResource("Radio.png"));
    if (!radioPix.isNull()) {
        logoLabel->setPixmap(radioPix.scaled(28, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    logoLayout->addWidget(logoLabel);
    
    m_titleLabel = new QLabel("V2V OSM app", this);
    m_titleLabel->setStyleSheet("color: white; font-size: 18px; font-weight: 700; letter-spacing: 0.5px;");
    logoLayout->addWidget(m_titleLabel);
    
    // Badge de statut
    m_statusBadge = new QPushButton(this);
    m_statusBadge->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(16, 185, 129, 0.2);
            border: 1px solid rgba(16, 185, 129, 0.5);
            border-radius: 12px;
            color: #10b981;
            padding: 4px 12px;
            font-size: 12px;
            font-weight: 600;
        }
    )");
    m_statusBadge->setText("⚡ Running");
    m_statusBadge->setCursor(Qt::ArrowCursor);
    logoLayout->addWidget(m_statusBadge);
    
    layout->addLayout(logoLayout);
    layout->addStretch();
    
    // Info label (zoom, coordonnées) à gauche du bouton
    m_infoLabel = new QLabel(this);
    m_infoLabel->setStyleSheet(R"(
        color: rgba(255, 255, 255, 0.7);
        font-size: 12px;
        padding: 4px 12px;
    )");
    m_infoLabel->setText("Zoom 16 | Lon 7.75210 | Lat 48.57340");
    layout->addWidget(m_infoLabel);
    
    // Bouton toggle thème (dark/light)
    m_themeBtn = new QPushButton("Dark", this);
    m_themeBtn->setStyleSheet(BUTTON_STYLE);
    m_themeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_themeBtn, &QPushButton::clicked, this, [this]() {
        m_darkTheme = !m_darkTheme;
        m_themeBtn->setText(m_darkTheme ? "Dark" : "Light");
        emit themeToggled(m_darkTheme);
    });
    layout->addWidget(m_themeBtn);
    
    // Bouton toggle qualité (HQ/LQ)
    m_qualityBtn = new QPushButton("Fast", this);
    m_qualityBtn->setStyleSheet(BUTTON_STYLE);
    m_qualityBtn->setCursor(Qt::PointingHandCursor);
    connect(m_qualityBtn, &QPushButton::clicked, this, [this]() {
        m_highQuality = !m_highQuality;
        m_qualityBtn->setText(m_highQuality ? "HQ" : "Fast");
        emit qualityToggled(m_highQuality);
    });
    layout->addWidget(m_qualityBtn);
    
    // Bouton Pause/Continuer
    m_startPauseBtn = new QPushButton(this);
    m_startPauseBtn->setStyleSheet(PRIMARY_BUTTON_STYLE);
    m_startPauseBtn->setCursor(Qt::PointingHandCursor);
    connect(m_startPauseBtn, &QPushButton::clicked, this, &TopBar::startPauseClicked);
    layout->addWidget(m_startPauseBtn);
    
    updateButtonStates();
}

void TopBar::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    // Fond avec effet de flou (simulé avec dégradé)
    QLinearGradient grad(0, 0, 0, height());
    grad.setColorAt(0, QColor(17, 24, 39, 230));
    grad.setColorAt(1, QColor(17, 24, 39, 200));
    
    p.fillRect(rect(), grad);
    
    // Bordure subtile en bas
    p.setPen(QPen(QColor(75, 85, 99, 100), 1));
    p.drawLine(0, height()-1, width(), height()-1);
}

void TopBar::setRunning(bool running) {
    m_running = running;
    updateButtonStates();
}

void TopBar::updateButtonStates() {
    if (m_running) {
        m_statusBadge->setText("⚡ Running");
        m_statusBadge->setStyleSheet(R"(
            QPushButton {
                background-color: rgba(16, 185, 129, 0.2);
                border: 1px solid rgba(16, 185, 129, 0.5);
                border-radius: 12px;
                color: #10b981;
                padding: 4px 12px;
                font-size: 12px;
                font-weight: 600;
            }
        )");
        // Bouton Pause avec icône pause
        QPixmap pausePix(findResource("pause.png"));
        if (!pausePix.isNull()) {
            m_startPauseBtn->setIcon(QIcon(pausePix.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        }
        m_startPauseBtn->setText(" Pause");
    } else {
        m_statusBadge->setText("⏸ Paused");
        m_statusBadge->setStyleSheet(R"(
            QPushButton {
                background-color: rgba(245, 158, 11, 0.2);
                border: 1px solid rgba(245, 158, 11, 0.5);
                border-radius: 12px;
                color: #f59e0b;
                padding: 4px 12px;
                font-size: 12px;
                font-weight: 600;
            }
        )");
        // Bouton Continuer avec icône play
        QPixmap playPix(findResource("play.png"));
        if (!playPix.isNull()) {
            m_startPauseBtn->setIcon(QIcon(playPix.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        }
        m_startPauseBtn->setText(" Continuer");
    }
}

void TopBar::updateInfo(int zoom, double lon, double lat) {
    m_infoLabel->setText(QString("Zoom %1 | Lon %2 | Lat %3")
        .arg(zoom)
        .arg(lon, 0, 'f', 5)
        .arg(lat, 0, 'f', 5));
}

void TopBar::setDarkTheme(bool dark) {
    m_darkTheme = dark;
    m_themeBtn->setText(m_darkTheme ? "🌙 Dark" : "☀️ Light");
}

void TopBar::setHighQuality(bool hq) {
    m_highQuality = hq;
    m_qualityBtn->setText(m_highQuality ? "✨ HQ" : "⚡ Fast");
}

// ============================================================================
// ParametersPanel Implementation
// ============================================================================
ParametersPanel::ParametersPanel(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void ParametersPanel::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);
    
    // Titre avec icône (aligné à gauche comme StatsPanel)
    auto* titleLayout = new QHBoxLayout();
    QLabel* iconLabel = new QLabel(this);
    iconLabel->setStyleSheet("background: transparent; border: none;");
    QPixmap gearPix(findResource("Gear.png"));
    if (!gearPix.isNull()) {
        iconLabel->setPixmap(gearPix.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    titleLayout->addWidget(iconLabel);
    
    QLabel* titleLabel = new QLabel("Paramètres de la simulation", this);
    titleLabel->setStyleSheet("color: white; font-size: 14px; font-weight: 600; background: transparent; border: none;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    layout->addLayout(titleLayout);
    
    // Sliders
    layout->addWidget(createSliderRow("", "Nombres de véhicules", m_vehicleSlider, m_vehicleValue, 1, 10000, 2000, ""));
    layout->addWidget(createSliderRow("", "Vitesse des véhicules", m_speedSlider, m_speedValue, 1, 500, 50, " km/h"));
    layout->addWidget(createSliderRow("", "Grandes antennes", m_largeAntennaSlider, m_largeAntennaValue, 0, 50, 5, ""));
    layout->addWidget(createSliderRow("", "Petites antennes", m_smallAntennaSlider, m_smallAntennaValue, 0, 200, 20, ""));
    
    // Rayon de transmission
    layout->addWidget(createSliderRow("", "Rayon de transmission", m_rangeSlider, m_rangeValue, 10, 1000, 500, "m"));
    
    // Toggles
    layout->addWidget(createToggleRow("Afficher les connexions", m_connectionsToggle, true));
    layout->addWidget(createToggleRow("Afficher les rayons", m_rangesToggle, true));
    layout->addWidget(createToggleRow("Connexions transitives", m_transitiveToggle, false));
    layout->addWidget(createToggleRow("Afficher les routes", m_roadsToggle, false));
    
    // Connecter les signaux
    connect(m_vehicleSlider, &QSlider::valueChanged, this, &ParametersPanel::vehicleCountChanged);
    connect(m_vehicleSlider, &QSlider::sliderReleased, this, [this]() {
        emit vehicleCountReleased(m_vehicleSlider->value());
    });
    connect(m_speedSlider, &QSlider::valueChanged, this, &ParametersPanel::vehicleSpeedChanged);
    connect(m_largeAntennaSlider, &QSlider::valueChanged, this, &ParametersPanel::largeAntennaCountChanged);
    connect(m_smallAntennaSlider, &QSlider::valueChanged, this, &ParametersPanel::smallAntennaCountChanged);
    // Émettre antennaConfigReleased au relâchement des sliders d'antennes pour déclencher K-means
    connect(m_largeAntennaSlider, &QSlider::sliderReleased, this, [this]() {
        emit antennaConfigReleased(m_largeAntennaSlider->value(), m_smallAntennaSlider->value());
    });
    connect(m_smallAntennaSlider, &QSlider::sliderReleased, this, [this]() {
        emit antennaConfigReleased(m_largeAntennaSlider->value(), m_smallAntennaSlider->value());
    });
    connect(m_rangeSlider, &QSlider::valueChanged, this, &ParametersPanel::transmissionRangeChanged);
    connect(m_connectionsToggle, &QPushButton::toggled, this, &ParametersPanel::showConnectionsChanged);
    connect(m_rangesToggle, &QPushButton::toggled, this, &ParametersPanel::showRangesChanged);
    connect(m_transitiveToggle, &QPushButton::toggled, this, &ParametersPanel::showTransitiveChanged);
    connect(m_roadsToggle, &QPushButton::toggled, this, &ParametersPanel::showRoadsChanged);
    
    layout->addStretch();
}

QWidget* ParametersPanel::createSliderRow(const QString& icon, const QString& title,
                                          QSlider*& slider, QLabel*& valueLabel,
                                          int min, int max, int initial, const QString& suffix) {
    QWidget* row = new QWidget(this);
    row->setStyleSheet("background: transparent; border: none;");
    row->setMinimumHeight(36);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(16);
    
    QLabel* titleLabel = new QLabel(title, row);
    titleLabel->setStyleSheet("color: rgba(255,255,255,0.8); font-size: 13px; background: transparent; border: none;");
    titleLabel->setMinimumWidth(180);
    layout->addWidget(titleLabel);
    
    slider = new QSlider(Qt::Horizontal, row);
    slider->setRange(min, max);
    slider->setValue(initial);
    slider->setStyleSheet(SLIDER_STYLE);
    slider->setMinimumWidth(150);
    layout->addWidget(slider, 1);
    
    valueLabel = new QLabel(row);
    valueLabel->setStyleSheet(R"(
        background-color: rgba(99, 102, 241, 0.2);
        border: 1px solid rgba(99, 102, 241, 0.4);
        border-radius: 4px;
        color: #818cf8;
        padding: 4px 12px;
        font-size: 12px;
        font-weight: 600;
    )");
    valueLabel->setAlignment(Qt::AlignCenter);
    valueLabel->setMinimumWidth(60);
    valueLabel->setText(QString::number(initial) + suffix);
    layout->addWidget(valueLabel);
    
    connect(slider, &QSlider::valueChanged, [valueLabel, suffix](int val) {
        valueLabel->setText(QString::number(val) + suffix);
    });
    
    return row;
}

QWidget* ParametersPanel::createToggleRow(const QString& title, QPushButton*& toggle, bool initial) {
    QWidget* row = new QWidget(this);
    row->setStyleSheet("background: transparent; border: none;");
    row->setMinimumHeight(36);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(16);
    
    QLabel* titleLabel = new QLabel(title, row);
    titleLabel->setStyleSheet("color: rgba(255,255,255,0.8); font-size: 13px; background: transparent; border: none;");
    titleLabel->setMinimumWidth(180);
    layout->addWidget(titleLabel);
    layout->addStretch();
    
    toggle = new QPushButton(row);
    toggle->setCheckable(true);
    toggle->setChecked(initial);
    toggle->setStyleSheet(initial ? TOGGLE_ON_STYLE : TOGGLE_OFF_STYLE);
    toggle->setCursor(Qt::PointingHandCursor);
    layout->addWidget(toggle);
    
    connect(toggle, &QPushButton::toggled, [toggle](bool checked) {
        toggle->setStyleSheet(checked ? TOGGLE_ON_STYLE : TOGGLE_OFF_STYLE);
    });
    
    return row;
}

int ParametersPanel::getVehicleCount() const { return m_vehicleSlider->value(); }
int ParametersPanel::getTransmissionRange() const { return m_rangeSlider->value(); }
int ParametersPanel::getLargeAntennaCount() const { return m_largeAntennaSlider->value(); }
int ParametersPanel::getSmallAntennaCount() const { return m_smallAntennaSlider->value(); }
int ParametersPanel::getVehicleSpeed() const { return m_speedSlider->value(); }
bool ParametersPanel::showConnections() const { return m_connectionsToggle->isChecked(); }
bool ParametersPanel::showRanges() const { return m_rangesToggle->isChecked(); }
bool ParametersPanel::showTransitive() const { return m_transitiveToggle->isChecked(); }
bool ParametersPanel::showRoads() const { return m_roadsToggle->isChecked(); }

void ParametersPanel::setVehicleCount(int count) {
    // Bloquer les signaux pour éviter de déclencher une boucle
    m_vehicleSlider->blockSignals(true);
    m_vehicleSlider->setValue(count);
    m_vehicleValue->setText(QString::number(count));
    m_vehicleSlider->blockSignals(false);
}

// ============================================================================
// StatsPanel Implementation
// ============================================================================
StatsPanel::StatsPanel(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void StatsPanel::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(10);
    
    // Titre avec icône
    auto* titleLayout = new QHBoxLayout();
    QLabel* iconLabel = new QLabel(this);
    iconLabel->setStyleSheet("background: transparent; border: none;");
    QPixmap trendPix(findResource("TrendUp.png"));
    if (!trendPix.isNull()) {
        iconLabel->setPixmap(trendPix.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    titleLayout->addWidget(iconLabel);
    
    QLabel* titleLabel = new QLabel("Statistiques en temps réel", this);
    titleLabel->setStyleSheet("color: white; font-size: 14px; font-weight: 600; background: transparent; border: none;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    layout->addLayout(titleLayout);
    
    // Stats
    layout->addWidget(createStatRow("Véhicules actifs", m_activeVehicles));
    layout->addWidget(createStatRow("Véhicules connectés", m_connectedVehicles, "#10b981"));
    layout->addWidget(createStatRow("Connexions totales", m_totalConnections));
    layout->addWidget(createStatRow("Taux de connexion", m_connectionRate, "#f472b6"));
    layout->addWidget(createStatRow("Comparaisons/tick", m_comparisons, "#fbbf24"));
    layout->addWidget(createStatRow("Moy. voisins/véhicule", m_avgNeighbors, "#60a5fa"));
    layout->addWidget(createStatRow("Temps de calcul", m_buildTime, "#a78bfa"));
    
    layout->addStretch();
}

QWidget* StatsPanel::createStatRow(const QString& title, QLabel*& valueLabel, const QString& color) {
    QWidget* row = new QWidget(this);
    row->setStyleSheet("background: transparent; border: none;");
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    
    QLabel* titleLabel = new QLabel(title, row);
    titleLabel->setStyleSheet("color: rgba(255,255,255,0.6); font-size: 13px; background: transparent; border: none;");
    layout->addWidget(titleLabel);
    layout->addStretch();
    
    valueLabel = new QLabel("0", row);
    valueLabel->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: 600; background: transparent; border: none;").arg(color));
    valueLabel->setAlignment(Qt::AlignRight);
    layout->addWidget(valueLabel);
    
    return row;
}

void StatsPanel::updateStats(int activeVehicles, int connectedVehicles,
                             int totalConnections, double connectionRate,
                             int comparisons, double avgNeighbors, double buildTimeMs) {
    m_activeVehicles->setText(QString::number(activeVehicles));
    m_connectedVehicles->setText(QString::number(connectedVehicles));
    m_totalConnections->setText(QString::number(totalConnections));
    m_connectionRate->setText(QString::number(connectionRate, 'f', 0) + "%");
    
    // Formater le nombre de comparaisons (ex: 93.7K au lieu de 93785)
    if (comparisons >= 1000000) {
        m_comparisons->setText(QString::number(comparisons / 1000000.0, 'f', 1) + "M");
    } else if (comparisons >= 1000) {
        m_comparisons->setText(QString::number(comparisons / 1000.0, 'f', 1) + "K");
    } else {
        m_comparisons->setText(QString::number(comparisons));
    }
    
    m_avgNeighbors->setText(QString::number(avgNeighbors, 'f', 1));
    m_buildTime->setText(QString::number(buildTimeMs, 'f', 2) + " ms");
}

// ============================================================================
// BottomMenu Implementation
// ============================================================================
BottomMenu::BottomMenu(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedHeight(m_expandedHeight);
    setupUI();
}

void BottomMenu::setupUI() {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 0);  // Pas de marge en bas pour toucher le bord
    layout->setSpacing(16);
    
    // Panel gauche - Paramètres (avec wrapper pour le fond arrondi en haut seulement)
    QWidget* paramsWrapper = new QWidget(this);
    paramsWrapper->setStyleSheet(R"(
        background-color: rgba(17, 24, 39, 0.95);
        border: 1px solid rgba(75, 85, 99, 0.3);
        border-bottom: none;
        border-top-left-radius: 16px;
        border-top-right-radius: 16px;
        border-bottom-left-radius: 0px;
        border-bottom-right-radius: 0px;
    )");
    auto* paramsLayout = new QVBoxLayout(paramsWrapper);
    paramsLayout->setContentsMargins(0, 0, 0, 0);
    m_paramsPanel = new ParametersPanel(paramsWrapper);
    paramsLayout->addWidget(m_paramsPanel);
    layout->addWidget(paramsWrapper, 3);
    
    // Panel droit - Stats (avec wrapper pour le fond arrondi en haut seulement)
    QWidget* statsWrapper = new QWidget(this);
    statsWrapper->setStyleSheet(R"(
        background-color: rgba(17, 24, 39, 0.95);
        border: 1px solid rgba(75, 85, 99, 0.3);
        border-bottom: none;
        border-top-left-radius: 16px;
        border-top-right-radius: 16px;
        border-bottom-left-radius: 0px;
        border-bottom-right-radius: 0px;
    )");
    auto* statsLayout = new QVBoxLayout(statsWrapper);
    statsLayout->setContentsMargins(0, 0, 0, 0);
    m_statsPanel = new StatsPanel(statsWrapper);
    statsLayout->addWidget(m_statsPanel);
    layout->addWidget(statsWrapper, 2);
    
    // Animation sur la hauteur
    m_animation = new QPropertyAnimation(this, "geometry", this);
    m_animation->setDuration(300);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
}

void BottomMenu::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Fond semi-transparent - 100% largeur, jusqu'en bas
    painter.setBrush(QColor(17, 24, 39, 180));  // rgba(17, 24, 39, 0.7)
    painter.setPen(Qt::NoPen);
    // Coins arrondis en haut seulement
    QPainterPath path;
    path.addRoundedRect(rect(), 20, 20);
    // Ajouter un rectangle pour couvrir les coins bas
    path.addRect(0, height() - 20, width(), 20);
    painter.drawPath(path.simplified());
}

void BottomMenu::toggle() {
    if (!parentWidget()) return;
    
    m_expanded = !m_expanded;
    m_animation->stop();
    
    int parentHeight = parentWidget()->height();
    int parentWidth = parentWidget()->width();
    
    // Le menu a toujours la même hauteur (expandedHeight)
    // On anime seulement sa position Y (slide in/out)
    int currentY = geometry().top();
    int targetY = m_expanded ? (parentHeight - m_expandedHeight) : parentHeight;
    
    QRect startRect(0, currentY, parentWidth, m_expandedHeight);
    QRect endRect(0, targetY, parentWidth, m_expandedHeight);
    
    m_animation->setStartValue(startRect);
    m_animation->setEndValue(endRect);
    m_animation->start();
    
    emit expansionChanged(m_expanded);
}

// ============================================================================
// ZoomControls Implementation
// ============================================================================
ZoomControls::ZoomControls(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(44, 90);
    setupUI();
}

void ZoomControls::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    
    QPushButton* zoomInBtn = new QPushButton("+", this);
    zoomInBtn->setStyleSheet(ZOOM_BUTTON_STYLE);
    zoomInBtn->setCursor(Qt::PointingHandCursor);
    connect(zoomInBtn, &QPushButton::clicked, this, &ZoomControls::zoomIn);
    layout->addWidget(zoomInBtn);
    
    QPushButton* zoomOutBtn = new QPushButton("−", this);
    zoomOutBtn->setStyleSheet(ZOOM_BUTTON_STYLE);
    zoomOutBtn->setCursor(Qt::PointingHandCursor);
    connect(zoomOutBtn, &QPushButton::clicked, this, &ZoomControls::zoomOut);
    layout->addWidget(zoomOutBtn);
}

// ============================================================================
// UIOverlay Implementation
// ============================================================================
UIOverlay::UIOverlay(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setupUI();
}

void UIOverlay::setupUI() {
    // Pas de layout - positionnement manuel
    m_topBar = new TopBar(this);
    m_bottomMenu = new BottomMenu(this);
    m_zoomControls = new ZoomControls(this);
    
    // Bouton toggle pour le menu
    m_menuToggleBtn = new QPushButton(this);
    m_menuToggleBtn->setFixedSize(48, 48);
    m_menuToggleBtn->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(99, 102, 241, 0.9);
            border: none;
            border-radius: 24px;
        }
        QPushButton:hover {
            background-color: rgba(129, 140, 248, 0.95);
        }
    )");
    updateToggleButtonIcon(true);  // Menu ouvert par défaut
    m_menuToggleBtn->setCursor(Qt::PointingHandCursor);
    
    // Bouton supprimer véhicule (caché par défaut)
    m_deleteVehicleBtn = new QPushButton("Supprimer", this);
    m_deleteVehicleBtn->setFixedSize(120, 40);
    m_deleteVehicleBtn->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(99, 102, 241, 0.9);
            color: white;
            border: none;
            border-radius: 8px;
            font-weight: bold;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: rgba(129, 140, 248, 0.95);
        }
    )");
    m_deleteVehicleBtn->setCursor(Qt::PointingHandCursor);
    m_deleteVehicleBtn->hide();
    
    connect(m_deleteVehicleBtn, &QPushButton::clicked, this, &UIOverlay::deleteTrackedVehicle);
    
    connect(m_menuToggleBtn, &QPushButton::clicked, m_bottomMenu, &BottomMenu::toggle);
    connect(m_bottomMenu, &BottomMenu::expansionChanged, this, [this](bool expanded) {
        updateToggleButtonIcon(expanded);
    });
    
    // Repositionner pendant l'animation
    connect(m_bottomMenu->animation(), &QPropertyAnimation::valueChanged, this, [this]() {
        repositionElements();
    });
}

void UIOverlay::showDeleteVehicleButton(bool show) {
    m_deleteVehicleBtn->setVisible(show);
    if (show) {
        // Positionner au centre en haut
        m_deleteVehicleBtn->move((width() - m_deleteVehicleBtn->width()) / 2, 70);
    }
}

void UIOverlay::resizeEvent(QResizeEvent*) {
    repositionElements();
}

void UIOverlay::repositionElements() {
    // TopBar en haut
    m_topBar->setGeometry(0, 0, width(), 56);
    
    // BottomMenu - ne pas toucher à la géométrie pendant l'animation
    // L'animation gère elle-même la position du menu
    if (m_bottomMenu->animation()->state() != QAbstractAnimation::Running) {
        int menuHeight = m_bottomMenu->expandedHeight();
        int menuY = m_bottomMenu->isExpanded() ? (height() - menuHeight) : height();
        m_bottomMenu->setGeometry(0, menuY, width(), menuHeight);
    }
    
    // Bouton toggle en bas à droite - suit le haut du menu
    int menuTop = m_bottomMenu->geometry().top();
    int btnY = menuTop - 60;
    // S'assurer que le bouton ne dépasse pas en bas
    if (btnY > height() - 68) {
        btnY = height() - 68;
    }
    m_menuToggleBtn->move(width() - 68, btnY);
    
    // Zoom controls à droite
    m_zoomControls->move(width() - 56, height() / 2 - 45);
}

void UIOverlay::updateStats() {
    if (!m_simulator) return;
    
    const auto& vehicles = m_simulator->vehicles();
    int active = vehicles.size();
    int connected = 0;
    int totalConnections = 0;
    
    // Calculer les stats depuis le graphe d'interférence
    const auto& interfGraph = m_simulator->getInterferenceGraph();
    for (const auto* v : vehicles) {
        auto neighbors = interfGraph.getDirectNeighbors(v->getId());
        if (!neighbors.empty()) connected++;
        totalConnections += neighbors.size();
    }
    totalConnections /= 2; // Chaque connexion est comptée deux fois
    
    double rate = active > 0 ? (connected * 100.0 / active) : 0;
    
    // Statistiques de performance
    int comparisons = interfGraph.getLastComparisons();
    double avgNeighbors = interfGraph.getLastAvgNeighbors();
    double buildTimeMs = interfGraph.getLastBuildTimeMs();
    
    m_bottomMenu->statsPanel()->updateStats(active, connected, totalConnections, rate,
                                            comparisons, avgNeighbors, buildTimeMs);
}

void UIOverlay::updateMapInfo(int zoom, double lon, double lat) {
    if (m_topBar) {
        m_topBar->updateInfo(zoom, lon, lat);
    }
}

void UIOverlay::updateToggleButtonIcon(bool menuExpanded) {
    QString iconPath = findResource("down.png");
    QPixmap pix(iconPath);
    if (!pix.isNull()) {
        if (!menuExpanded) {
            // Menu fermé -> flèche vers le haut (rotation 180°)
            QTransform transform;
            transform.rotate(180);
            pix = pix.transformed(transform);
        }
        m_menuToggleBtn->setIcon(QIcon(pix.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        m_menuToggleBtn->setIconSize(QSize(24, 24));
    }
}
