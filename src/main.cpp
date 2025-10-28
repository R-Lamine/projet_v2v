#include "simulator.h"
#include "SimulatorView.h"

#include <QApplication>

// TEST METHOD
RoadGraph createTestGraph() {
    RoadGraph g;

    // Add vertices
    Vertex v0 = boost::add_vertex(VertexData{0, 0.0}, g);
    Vertex v1 = boost::add_vertex(VertexData{0, 1.0}, g);
    Vertex v2 = boost::add_vertex(VertexData{1, 1.0}, g);

    // Add edges
    boost::add_edge(v0, v1, EdgeData{1.0}, g);
    boost::add_edge(v1, v2, EdgeData{1.5}, g);
    boost::add_edge(v0, v2, EdgeData{2.0}, g);

    return g;
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    RoadGraph g = createTestGraph(); // create your mock graph first
    Simulator sim(g);                // pass graph to constructor

    sim.addCar(0, 2); // start at vertex 0, goal vertex 2
    sim.addCar(1, 2); // start at vertex 1, goal vertex 2

    for (int t = 0; t < 5; ++t) {
        sim.update(1.0); // advance 1 second
        sim.printStatus();
    }

    SimulatorView view;
    view.setVehicles(sim.getVehicules());
    view.resize(800, 600);
    view.show();

    return app.exec();
}

