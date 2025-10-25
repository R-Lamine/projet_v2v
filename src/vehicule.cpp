#include "vehicule.h"

//Destructor
Car::~Car(void) {}

void Car::update(double deltaTime) {
    if (destReached) return;

    // If we have no valid current edge, try to pick one from currVertex
    if (edgeLength <= 0.0) {
        auto [ei, ei_end] = boost::out_edges(currVertex, graph);
        if (ei == ei_end) { destReached = true; return; }
        currEdge = *ei;
        nextVertex = boost::target(currEdge, graph);
        edgeLength = graph[currEdge].distance;
        positionOnEdge = 0.0;
        // set initial speed if needed
        if (speed <= 0.0) speed = 13.9; // e.g. 50 km/h in m/s
    }

    // advance
    positionOnEdge += speed * deltaTime;

    // reached or passed end of edge?
    if (positionOnEdge >= edgeLength) {
        // compute overshoot
        double overshoot = positionOnEdge - edgeLength;

        // arrive at next vertex
        currVertex = nextVertex;

        // pick next outgoing edge (simple policy: first available)
        auto [ei, ei_end] = boost::out_edges(currVertex, graph);
        if (ei == ei_end) {
            // dead end or destination
            destReached = true;
            positionOnEdge = edgeLength; // clamp
            return;
        }

        // pick the first outgoing edge (replace with route logic later)
        currEdge = *ei;
        nextVertex = boost::target(currEdge, graph);
        edgeLength = graph[currEdge].distance;

        // start on new edge using overshoot
        positionOnEdge = std::min(overshoot, edgeLength);
    }
}


std::pair<double,double> Car::getPosition() const {
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
