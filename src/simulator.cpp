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

    // Mise à jour de la position des véhicules
    for (Vehicule* v : m_vehicles) {
        if(v) v->update(deltaTime);
    }

    // Reconstruction du graphe d'interférence avec les nouvelles positions
    m_interferenceGraph.buildGraph(m_vehicles);

    emit ticked(deltaTime);
}

void Simulator::addVehicle(Vehicule* v) {
    if(v) m_vehicles.push_back(v);
}











