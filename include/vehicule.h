#ifndef VEHICULE_H
#define VEHICULE_H

#include "graph_types.h"

#include <vector>
#include <utility>

//using namespace std;

class Car {

public:

    //Constructor
    Car(int id, const RoadGraph& graph, Vertex start);

    //Destructor
    ~Car();

    void update(double deltaTime);
    void printStatus() const;
    std::pair<double, double> getPosition() const;


private:

    /*/Constructor
    Car(int id, const RoadGraph graph, Vertex start, Vertex dest)
        :id{id}, graph{graph}, currVertex{start}, destReached{false} {};
    */

    int id;
    const RoadGraph& graph;     //reference to the shared graph

    Vertex currVertex;
    Vertex nextVertex;
    Edge currEdge;

    double positionOnEdge;   //distance from current vertex
    double speed;            //m/s
    double edgeLength = 0.0;     // cache of graph[currEdge].distance
    bool destReached = false;

    //optional
    std::vector<Vertex> route;  //list of vertex to follow

};


#endif
