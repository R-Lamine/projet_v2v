#include "graph_builder.h"
#include <iostream>
#include <cmath>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/iteration_macros.hpp>

using namespace std;

// Constructeur : initialise les références vers les données OSM
GraphBuilder::GraphBuilder(const vector<OSMNode>& n, const vector<OSMWay>& w)
    : nodes(n), ways(w) {}

// Construit le graphe à partir des données OSM
void GraphBuilder::buildGraph() {
    // Étape 1 : ajout de tous les sommets à partir des nodes OSM
    for (const auto& n : nodes) {
        Vertex v = boost::add_vertex(graph);
        graph[v].id = n.id;
        graph[v].lat = n.lat;
        graph[v].lon = n.lon;
        idToVertex[n.id] = v;
    }

    // Étape 2 : création des arêtes à partir des ways
    for (const auto& way : ways) {
        for (size_t i = 1; i < way.nodeRefs.size(); ++i) {
            long id1 = way.nodeRefs[i - 1];
            long id2 = way.nodeRefs[i];

            // Vérifie que les deux sommets existent dans le graphe
            if (idToVertex.count(id1) && idToVertex.count(id2)) {
                Vertex v1 = idToVertex[id1];
                Vertex v2 = idToVertex[id2];

                double dist = distance(graph[v1].lat, graph[v1].lon,
                                       graph[v2].lat, graph[v2].lon);

                Edge e; bool inserted;
                tie(e, inserted) = boost::add_edge(v1, v2, graph);

                if (inserted) {
                    graph[e].distance = dist;
                    graph[e].oneway = way.oneway;
                }
            }
        }
    }

    cout << "Graphe construit avec succès." << endl;
}

// Calcule la distance géographique entre deux points (formule de Haversine)
double GraphBuilder::distance(double lat1, double lon1, double lat2, double lon2) const {
    const double R = 6371000.0; // rayon de la Terre en mètres
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double a = sin(dLat / 2) * sin(dLat / 2) +
               cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
               sin(dLon / 2) * sin(dLon / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return R * c;
}

// Affiche un résumé du graphe construit
void GraphBuilder::printSummary() const {
    cout << "Résumé du graphe :" << endl;
    cout << "  Nombre de sommets : " << boost::num_vertices(graph) << endl;
    cout << "  Nombre d'arêtes   : " << boost::num_edges(graph) << endl;
}
