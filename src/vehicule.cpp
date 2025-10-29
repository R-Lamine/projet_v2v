#include "vehicule.h"
#include "graph_builder.h"
#include <iostream>

//Constructor
Vehicule::Vehicule(int id, const RoadGraph& graph, Vertex start, Vertex goal, double speed, double range, double collisionDist)
    : id(id),
    graph(graph),
    start(start),
    goal(goal),
    currVertex(start),
    transmissionRange(range),
    speed(speed),
    collisionDist(collisionDist)
{}


//Destructor
Vehicule::~Vehicule(void) {}


/**
 * @brief Vehicule::update, select next Edge randomly for now, will use shortest part later (Djikstra)
 * @param deltaTime
 */
void Vehicule::update(double deltaTime) {
    if (destReached) return;
    if (currVertex == goal) {
        destReached = true;
        return;
    }

    // If we have no valid current edge, try to pick one from currVertex
    if (edgeLength <= 0.0) {
        auto [edgeIteratorStart, edgeIteratorEnd] = boost::out_edges(currVertex, graph);
        if (edgeIteratorStart == edgeIteratorEnd)
        {
            destReached = true;
            return;
        }

        // Pick a random outgoing edge instead of always the first
        std::advance(edgeIteratorStart, rand() % std::distance(edgeIteratorStart, edgeIteratorEnd));
        currEdge = *edgeIteratorStart;
        nextVertex = boost::target(currEdge, graph);
        edgeLength = graph[currEdge].distance;
        positionOnEdge = 0.0;

        // set initial speed if needed
        //if (speed <= 0.0) speed = 13.9; // e.g. 50 km/h in m/s
    }

    // advance
    positionOnEdge += speed * deltaTime;

    // reached end
    if (positionOnEdge >= edgeLength) {
        // compute overshoot
        double overshoot = positionOnEdge - edgeLength;

        // arrive at next vertex
        currVertex = nextVertex;
        if (currVertex == goal) {
            destReached = true;
            return;
        }

        // pick next outgoing edge randomly
        auto [edgeIteratorStart, edgeIteratorEnd] = boost::out_edges(currVertex, graph);
        if (edgeIteratorStart == edgeIteratorEnd) {
            // dead end or destination
            destReached = true;
            positionOnEdge = edgeLength; // clamp
            return;
        }

        std::advance(edgeIteratorStart, rand() % std::distance(edgeIteratorStart, edgeIteratorEnd));
        currEdge = *edgeIteratorStart;
        nextVertex = boost::target(currEdge, graph);
        edgeLength = graph[currEdge].distance;

        // start on new edge using overshoot
        positionOnEdge = std::min(overshoot, edgeLength);
    }
}


std::pair<double,double> Vehicule::getPosition() const {
    if (edgeLength <= 0.0) {
        const auto& vd = graph[currVertex];
        return {vd.lat, vd.lon};
    }

    Vertex s = boost::source(currEdge, graph);
    Vertex t = boost::target(currEdge, graph);
    const auto& sd = graph[s];
    const auto& td = graph[t];

    double tparam = positionOnEdge / edgeLength;
    if (tparam < 0) tparam = 0;
    if (tparam > 1) tparam = 1;

    double lat = sd.lat + tparam * (td.lat - sd.lat);
    double lon = sd.lon + tparam * (td.lon - sd.lon);
    return {lat, lon};
}

double Vehicule::calculateDist(Vehicule from) const{
    auto [lat1, lon1] = getPosition();
    auto [lat2, lon2] = from.getPosition();

    return GraphBuilder::distance(lat1, lon1, lat2, lon2);

}

void Vehicule::avoidCollision() {
    for (Vehicule* v : neighbors) {
        double dist = calculateDist(*v);
        if (dist <= collisionDist) {
            speed *= slowFactor;
        }
    }
}
