#pragma once
#include <boost/graph/adjacency_list.hpp>

// Données attachées à chaque sommet (node OSM)
struct VertexData {
    long id;        // identifiant OSM du point
    double lat;     // latitude
    double lon;     // longitude
};

// Données attachées à chaque arête (route entre deux sommets)
struct EdgeData {
    double distance;   // distance entre les deux sommets (en mètres)
    bool oneway;       // true si la route est à sens unique
    std::string type;  //highway type
};

// Définition du graphe Boost
// - boost::vecS : stockage des sommets et arêtes sous forme de tableaux
// - boost::undirectedS : routes bidirectionnelles par défaut
//   (on adaptera si besoin pour les routes à sens unique)
using RoadGraph = boost::adjacency_list<
    boost::vecS,
    boost::vecS,
    boost::undirectedS,
    VertexData,
    EdgeData
>;

// Types utiles pour manipuler le graphe
using Vertex = boost::graph_traits<RoadGraph>::vertex_descriptor;
using Edge   = boost::graph_traits<RoadGraph>::edge_descriptor;
