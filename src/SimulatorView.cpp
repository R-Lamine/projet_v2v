#include "SimulatorView.h"

SimulatorView::SimulatorView(MapView* map, QWidget *parent)
    : QWidget(parent), m_map(map)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SimulatorView::updateSimulation);
    m_timer->start(100); // update every 100ms
}

/*
SimulatorView::~SimulatorView() {
    m_vehicules.clear();
}
*/
void SimulatorView::setVehicles(const std::vector<Vehicule*>& vehicules) {
    m_vehicules = vehicules;
}
//we pass thevector by reference and not a copy

void SimulatorView::updateSimulation() {
    for (auto* v : m_vehicules) {
        v->update(0.1); // Simulate 0.1s step
    }
    update(); // trigger repaint
}

void SimulatorView::paintEvent(QPaintEvent*) {
    if (!m_map) return;
    QPainter painter(this);

    //painter.fillRect(rect(), Qt::white); // background

    painter.setBrush(Qt::red);
    for (auto v : m_vehicules) {
        auto [lat, lon] = v->getPosition();

        double px, py;
        MapView::lonlatToPixel(lon, lat, m_map->zoomLevel(), px, py);
        /*
        int x = static_cast<int>(lon * 10); // scale for visualization
        int y = static_cast<int>(lat * 10);
        */
        painter.drawEllipse(QPointF(px, py), 5, 5);
    }
}
