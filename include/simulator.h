#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QMutex>
#include <vector>
#include <iostream>
#include <atomic>

#include "vehicule.h"
#include "map_view.h"
#include "graph_builder.h"
#include "interference_graph.h"

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
    void togglePause(); // toggle between pause and resume
    void reset(); // reset simulation to initial state
    bool isRunning() const { return m_running && !m_paused; }

    //vehicle management
    void addVehicle(Vehicule* v); // takes ownership
    bool removeVehicle(Vehicule* v);
    void clearVehicles();
    void setVehicleCount(int count); // Dynamically add or remove vehicles to match count
    Vehicule* createVehicleNear(double lon, double lat); // Create a vehicle near a position
    
    // Antenna management
    void placeAntennas(int numLarge, int numSmall); // Run K-means to place antennas

    // Simulation parameters
    void setSpeedMultiplier(double m);
    double speedMultiplier() const;
    void setCollisionDetectionEnabled(bool e);

    // Read-only access for rendering / UI
    const std::vector<Vehicule*>& vehicles() const { return m_vehicles; }

    // Access to interference graph for visualization
    const InterferenceGraph& interferenceGraph() const { return m_interferenceGraph; }
    InterferenceGraph& interferenceGraph() { return m_interferenceGraph; }
    const InterferenceGraph& getInterferenceGraph() const { return m_interferenceGraph; }

   const RoadGraph& getGraph() const {return graph;}

signals:
    void simulationStarted();
    void simulationPaused();
    void simulationResumed();
    void simulationStopped();
    void vehicleCountChanged(int count);  // Émis quand le nombre de véhicules change

    // Emitted after each tick (after vehicles updated) to repaint
    void ticked(double deltaTimeSeconds);


public slots:
    // slot used by internal timer
    void onTick();
    
private slots:
    void onGraphCalculationFinished();


private:
    // Internal step logic: advances all vehicles by deltaTime
    void updateSimulation(double deltaSeconds);
    
    // Lance le calcul du graphe en arrière-plan
    void startGraphCalculation();

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
    InterferenceGraph m_interferenceGraph;
    
    // Pour le calcul asynchrone du graphe
    QFutureWatcher<InterferenceGraph>* m_futureWatcher = nullptr;
    std::atomic<bool> m_calculationInProgress{false};
    
    // Pour la création dynamique de véhicules
    std::vector<Vertex> m_vertices;
    int m_nextVehicleId = 0;
};


#endif
