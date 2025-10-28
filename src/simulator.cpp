#include "simulator.h"
#include <iostream>

Simulator::Simulator(const RoadGraph& g) : graph(g) {}   //initializes member field "graph" with g
Simulator::~Simulator() = default;

void Simulator::addCar(Vertex start, Vertex goal) {
    vehicules.emplace_back(vehicules.size(), graph, start, goal, 20.0, 0.0, 5);
}

void Simulator::update(double deltaTime) {
    for (auto& v : vehicules)
        v.update(deltaTime);
}

void Simulator::printStatus() const {
    for (const auto& v : vehicules) {
        auto [lat, lon] = v.getPosition();
        std::cout << "Vehicule " << v.getId()
                  << " at (" << lat << ", " << lon << ")\n";
    }
}
