#include "vehicule.h"
#include "graph_builder.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

bool Vehicule::isValidRoad(const std::string& type) {
    static const std::vector<std::string> valid = {
        "motorway","trunk","primary","secondary","tertiary",
        "motorway_link","trunk_link","primary_link","secondary_link",
        "unclassified"
    };
    return std::find(valid.begin(), valid.end(), type) != valid.end();
}

bool Vehicule::isValidVertex(Vertex v, const RoadGraph& graph) {
    auto [itStart, itEnd] = boost::out_edges(v, graph);
    // vertex must have at least one valid outgoing edge
    for (auto it = itStart; it != itEnd; ++it) {
        const Edge e = *it;
        if (Vehicule::isValidRoad(graph[e].type)) return true;
    }
    return false;
}

bool Vehicule::hasValidOutgoingEdge(Vertex v, const RoadGraph& graph) {
    auto [itStart, itEnd] = boost::out_edges(v, graph);
    for (auto it = itStart; it != itEnd; ++it) {
        const Edge e = *it;
        if (isValidRoad(graph[e].type)) {
            return true; // found at least one valid road
        }
    }
    return false; // no valid outgoing road — not usable
}


void Vehicule::DestReached() {
    std::swap(start, goal);
    edgeLength = 0.0;
}

Vertex Vehicule::pickNextEdge() {
    auto [itStart, itEnd] = boost::out_edges(currVertex, graph);

    std::vector<Edge> validEdges;
    Edge backEdge = Edge();   // placeholder for the edge going back to previous vertex
    bool hasBackEdge = false;

    for (auto it = itStart; it != itEnd; ++it) {
        Edge e = *it;
        Vertex target = boost::target(e, graph);

        if (!isValidRoad(graph[e].type)) continue;

        if (target == previousVertex) {
            // remember this edge in case we have no other choice
            backEdge = e;
            hasBackEdge = true;
        } else {
            validEdges.push_back(e);
        }
    }

    if (validEdges.empty()) {
        if (hasBackEdge) {
            validEdges.push_back(backEdge); // only option is to go back
        } else {
            // truly stuck: swap start/goal
            std::swap(start, goal);
            nextVertex = start;
            edgeLength = 0.0;
            return nextVertex;
        }
    }

    // pick random valid edge (avoiding immediate backtracking if possible)
    currEdge = validEdges[rand() % validEdges.size()];
    previousVertex = currVertex;  // remember current as previous
    nextVertex = boost::target(currEdge, graph);
    edgeLength = graph[currEdge].distance;
    positionOnEdge = 0.0;

    return nextVertex;
}

void Vehicule::update(double deltaTime) {
    if (currVertex == goal) {
        DestReached();
        return;
    }

    // If no edge is selected yet
    if (edgeLength <= 0.0) {
        pickNextEdge();
    }

    // Stocker la position actuelle avant le mouvement
    auto [prevLat, prevLon] = getPosition();

    // advance along the current edge
    positionOnEdge += speed * deltaTime;

    // Récupérer la nouvelle position après le mouvement
    auto [currLat, currLon] = getPosition();
    
    // Calculer le heading basé sur le vecteur de mouvement réel
    double dLat = currLat - prevLat;
    double dLon = currLon - prevLon;
    
    // Vérifier qu'il y a eu un mouvement significatif
    if (std::abs(dLat) > 1e-10 || std::abs(dLon) > 1e-10) {
        // Calculer l'angle : 0° = north, 90° = east
        double angleRad = std::atan2(dLon, dLat);
        targetHeading = angleRad * 180.0 / M_PI;
        
        // Normaliser l'angle entre 0 et 360
        if (targetHeading < 0) {
            targetHeading += 360.0;
        }
        
        // Lisser le heading pour éviter les tremblements
        double angleDiff = targetHeading - currentHeading;
        if (angleDiff > 180.0) {
            angleDiff -= 360.0;
        } else if (angleDiff < -180.0) {
            angleDiff += 360.0;
        }
        
        currentHeading += angleDiff * headingSmoothingFactor;
        
        // Normaliser currentHeading
        if (currentHeading < 0) {
            currentHeading += 360.0;
        } else if (currentHeading >= 360.0) {
            currentHeading -= 360.0;
        }
    }

    // check if we've reached or overshot the end of the edge
    while (positionOnEdge >= edgeLength) {
        double overshoot = positionOnEdge - edgeLength;
        previousVertex = currVertex;  // remember where we came from
        currVertex = nextVertex;

        if (currVertex == goal) {
            DestReached();
            return;
        }

        pickNextEdge();  // choose next edge
        positionOnEdge = overshoot; // carry remaining distance to next edge
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
