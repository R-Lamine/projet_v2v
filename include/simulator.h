#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <vector>
#include <iostream>

#include "vehicule.h"
#include "map_view.h"
#include "graph_builder.h"

class Simulator : public QObject {
    Q_OBJECT

public:
    explicit Simulator(RoadGraph& graph, MapView* mapView, QObject* parent = nullptr);
    ~Simulator() override;

    // Controls
    void start(int tickIntervalMs = 50); // default 20 FPS
    void stop(); // stops and resets timer
    void pause(); // pauses (timer stops but state preserved)
    void resume(); // resumes after pause
    void stepOnce(); // perform a single simulation

    //vehicle management
    void addVehicle(Vehicule* v); // takes ownership
    bool removeVehicle(Vehicule* v);
    void clearVehicles();

    // Simulation parameters
    void setSpeedMultiplier(double m);
    double speedMultiplier() const;
    void setCollisionDetectionEnabled(bool e);

    // Read-only access for rendering / UI
    const std::vector<Vehicule*>& vehicles() const { return m_vehicles; }

signals:
    void simulationStarted();
    void simulationPaused();
    void simulationResumed();
    void simulationStopped();

    // Emitted after each tick (after vehicles updated) to repaint
    void ticked(double deltaTimeSeconds);


public slots:
    // slot used by internal timer
    void onTick();


private:
    // Internal step logic: advances all vehicles by deltaTime
    void updateSimulation(double deltaSeconds);

private:
    const RoadGraph& graph;
    MapView* m_mapView;

    QTimer* m_timer;
    QElapsedTimer m_elapsed;    //to compute deltaTime between ticks (tick = update)
    int m_tickIntervalMs = 50;  //evry 50 ms
    double m_speedMultiplier = 1.0;
    bool m_running = false;
    bool m_paused = false;
    bool m_collisionDetectionEnabled = true;

    std::vector<Vehicule*> m_vehicles;
};


#endif
