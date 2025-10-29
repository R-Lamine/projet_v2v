#ifndef VEHICULE_H
#define VEHICULE_H

#include "graph_types.h"
#include <vector>
#include <utility>
#include <cmath>


class Vehicule {

public:

    /**
     * @brief Constructs a Vehicule.
     * @param id Unique vehicle ID.
     * @param graph Reference to a shared RoadGraph.
     * @param start and Goal vertex.
     * @param speed of vehicule movement.
     * @param range Transmission range for V2V communication.
     * @param collisionDist to avoid collisions.
     */
    Vehicule(int id, const RoadGraph& graph, Vertex start, Vertex goal, double speed,
             double range, double collisionDist);

    /// Destructor
    ~Vehicule();

    // ===============================
    // Movement & simulation methods
    // ===============================

    /**
     * @brief Updates the vehicle's position along its current edge.
     * @param deltaTime Time step for movement update (seconds).
     */
    void update(double deltaTime);

    /**
     * @brief Reduces speed if any neighbor is too close (collision avoidance).
     */
    void avoidCollision();

    /**
     * @brief Prints vehicle status (ID, position, speed).
     */
    void printStatus() const;

    /**
     * @brief Calculates Euclidean distance to another vehicle.
     * @param from Another vehicle to measure distance from.
     * @return Euclidean distance between vehicles.
     */
    double calculateDist(Vehicule from) const;

    /**
     * @brief Gets the current geographic position of the vehicle.
     * @return Pair of latitude and longitude (lat, lon).
     */
    std::pair<double, double> getPosition() const;

    // ===============================
    // Communication (V2V)
    // ===============================

    /// Returns the vehicle's transmission range.
    double getTransmissionRange() const { return transmissionRange; }

    /// Sets the vehicle's transmission range.
    void setTransmissionRange(double range) { transmissionRange = range; }

    /// Returns the list of neighboring vehicles.
    const std::vector<Vehicule*>& getNeighbors() const { return neighbors; }

    /// Adds a neighbor for V2V communication.
    void addNeighbor(Vehicule* v) { neighbors.push_back(v); }

    /// Clears the list of neighbors.
    void clearNeighbors() { neighbors.clear(); }

    /// Returns the vehicle ID.
    int getId() const { return id; }

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
