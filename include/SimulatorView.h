#ifndef SIMULATORVIEW_H
#define SIMULATORVIEW_H

#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <QObject>
#include <QPainter>
#include "vehicule.h"
#include "map_view.h"

class SimulatorView : public QWidget {
    Q_OBJECT

public:
    explicit SimulatorView(MapView* map, QWidget *parent = nullptr);          //explicit prevents automatic conversions like this "SimulatorView view = nullptr;"

    void setVehicles(const std::vector<Vehicule*>& vehicules);

    ~SimulatorView() = default;

protected:
    void paintEvent(QPaintEvent* event) override;           //protected so that it can be reimplemented internally (override)

private slots:
    void updateSimulation();            //slots and signals are the way objects communicate
                                        //a slot is a funct that reacts to an emitted signal when smthn changes

private:
    std::vector<Vehicule*> m_vehicules;
    QTimer* m_timer;
    MapView* m_map;
};

#endif // SIMULATORVIEW_H
