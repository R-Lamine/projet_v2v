#include "simulator.h"

#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>

Simulator::Simulator(RoadGraph& graph, MapView* mapView, QObject* parent)
    :graph(graph), m_mapView(mapView), QObject(parent)
{
    // initialize elapsed timer
    m_elapsed.start();

    // setup the QTimer
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &Simulator::onTick);
}

Simulator::~Simulator() {
    stop();
}

void Simulator::start(int tickIntervalMs) {
    m_tickIntervalMs = tickIntervalMs;
    m_elapsed.restart();
    
    // Initialiser la grille spatiale UNE SEULE FOIS au démarrage
    // Ceci évite de refaire le K-means à chaque tick
    if (!m_vehicles.empty()) {
        m_interferenceGraph.initializeSpatialGrid(m_vehicles);
    }
    
    m_timer->start(tickIntervalMs);
    emit simulationStarted();
}

void Simulator::pause() {
    m_timer->stop();
    emit simulationPaused();
}

void Simulator::resume() {
    m_elapsed.restart();
    m_timer->start(m_tickIntervalMs);
    emit simulationResumed();
}

void Simulator::stop() {
    m_timer->stop();
    emit simulationStopped();
}

void Simulator::onTick() {
    double deltaTime = m_elapsed.restart() / 1000.0; // seconds
    deltaTime *= m_speedMultiplier;

    static int tickCount = 0;
    tickCount++;

    // Mise à jour de la position des véhicules
    for (Vehicule* v : m_vehicles) {
        if(v) v->update(deltaTime);
    }

    // Avec beaucoup de véhicules (>1000), ne reconstruire le graphe que rarement
    int rebuildInterval = 10; // Par défaut 500ms
    if (m_vehicles.size() > 500) {
        rebuildInterval = 20; // 5 secondes pour >500 véhicules
    }
    if (m_vehicles.size() > 1000) {
        rebuildInterval = 40; // 10 secondes pour >1000 véhicules
    }
    if (m_vehicles.size() > 2000) {
        rebuildInterval = 100; // 30 secondes pour >2000 véhicules - limite les freezes
    }
    
    if (tickCount % rebuildInterval == 0) {
        m_interferenceGraph.buildGraph(m_vehicles);
    }

    emit ticked(deltaTime);
}

void Simulator::addVehicle(Vehicule* v) {
    if(v) m_vehicles.push_back(v);
}











