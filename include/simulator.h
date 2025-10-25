#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "vehicule.h"

class Simulation {
public:
    Simulation(const RoadGraph& g);

    void addCar(Vertex start, Vertex goal);
    void update(double deltaTime);
    void printStatus() const;

private:
    const RoadGraph& graph;
    std::vector<Car> cars;
};


#endif
