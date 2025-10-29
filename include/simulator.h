#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "vehicule.h"


class Simulator {
public:
    Simulator(const RoadGraph& g);
    ~Simulator();

    void addCar(Vertex start, Vertex goal);
    void update(double deltaTime);
    void printStatus() const;

    //getters
    const std::vector<Vehicule*> getVehicules() const {return vehicules;};

    //test function
    //RoadGraph createTestGraph();

private:
    const RoadGraph& graph;
    std::vector<Vehicule*> vehicules;
};


#endif
