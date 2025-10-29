#include "simulator.h"
#include <iostream>

Simulator::Simulator(const RoadGraph& g) : graph(g) {}   //initializes member field "graph" with g

void Simulator::addCar(Vertex start, Vertex goal) {
    vehicules.push_back(new Vehicule(vehicules.size(), graph, start, goal, 30.0, 0.0, 5));
}

void Simulator::update(double deltaTime) {
    for (auto* v : vehicules)
        v->update(deltaTime);
}

void Simulator::printStatus() const {
    for (const auto* v : vehicules) {
        auto [lat, lon] = v->getPosition();
        std::cout << "Vehicule " << v->getId()
                  << " at (" << lat << ", " << lon << ")\n";
    }
}

Simulator::~Simulator() {
    for (auto* v : vehicules)
        delete v;
}

