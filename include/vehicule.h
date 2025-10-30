#ifndef VEHICULE_H
#define VEHICULE_H

#include "graph_types.h"
#include <vector>
#include <utility>
#include <cmath>


class Vehicule {

public:
    Vehicule(int id, const RoadGraph& graph, Vertex start, Vertex goal, double speed,
             double range, double collisionDist);

    ~Vehicule();

    /**
     * @brief Updates the vehicle's position along its current edge.
     * @param deltaTime Time step for movement update (seconds).
     */
    void update(double deltaTime);

    /**
     * @brief Reduces speed if any neighbor is too close (collision avoidance).
     */
    void avoidCollision();
    void printStatus() const;

    /**
     * @brief Calculates Euclidean distance to another vehicle.
     * @param from Another vehicle to measure distance from.
     * @return Euclidean distance between vehicles.
     */
    double calculateDist(Vehicule from) const;
    std::pair<double, double> getPosition() const;

    //getters
    int getId() const { return id; }
    double getTransmissionRange() const { return transmissionRange; }
    const std::vector<Vehicule*>& getNeighbors() const { return neighbors;}
    //setter
    void setTransmissionRange(double range) { transmissionRange = range; }

    void addNeighbor(Vehicule* v) { neighbors.push_back(v); }
    void clearNeighbors() { neighbors.clear(); }


private:

    int id;
    const RoadGraph& graph;         ///< Reference to shared road graph

    Vertex start;
    Vertex goal;

    Vertex nextVertex;
    Edge currEdge;
    double collisionDist;
    double transmissionRange;
    double speed;

    //default values
    Vertex currVertex;
    double edgeLength = 0.0;      ///< Cach of graph[currEdge].distance
    double positionOnEdge = 0.0;     ///< Distance along the current edge
    bool destReached = false;
    double slowFactor = 0.8;        ///< Speed reduction factor when avoiding collision

    std::vector<Vehicule*> neighbors;
    std::vector<Vertex> route;      ///< Optional route (sequence of vertices)
};

#endif
