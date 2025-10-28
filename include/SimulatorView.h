#ifndef SIMULATORVIEW_H
#define SIMULATORVIEW_H

#include <QWidget>
#include <QTimer>
#include <QPainter>
#include "vehicule.h"

class SimulatorView : public QWidget {
    Q_OBJECT

public:
    explicit SimulatorView(QWidget *parent = nullptr);

    void setVehicles(const std::vector<Vehicule>& vehicules);

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void updateSimulation();

private:
    std::vector<Vehicule*> m_vehicules;
    QTimer* m_timer;
};

#endif // SIMULATORVIEW_H
