#include "SimulatorView.h"
#include <QPainter>

SimulatorView::SimulatorView(QWidget *parent)
    : QWidget(parent),
    m_timer(new QTimer(this))
{
    connect(m_timer, &QTimer::timeout, this, &SimulatorView::updateSimulation);
    m_timer->start(100); // update every 100ms
}

void SimulatorView::setVehicles(const std::vector<Vehicule*>& vehicules) {
    m_vehicules = vehicules;
}

void SimulatorView::updateSimulation() {
    for (auto* v : m_vehicules) {
        v->update(0.1); // Simulate 0.1s step
    }
    update(); // trigger repaint
}

void SimulatorView::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white); // background

    painter.setBrush(Qt::red);
    for (auto* v : m_vehicules) {
        auto [lat, lon] = v->getPosition();
        int x = static_cast<int>(lon * 10); // scale for visualization
        int y = static_cast<int>(lat * 10);
        painter.drawEllipse(QPointF(x, y), 5, 5);
    }
}
