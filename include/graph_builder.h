#pragma once

#include <vector>
#include <unordered_map>
#include <boost/graph/adjacency_list.hpp>
#include "graph_types.h"    // pour RoadGraph, Vertex, Edge
#include "osm_reader.h"    // pour OSMNode, OSMWay

class GraphBuilder {
public:
    // Constructeur : reçoit les nodes et ways déjà lus depuis OSMReader
    GraphBuilder(const std::vector<OSMNode>& n, const std::vector<OSMWay>& w);

    // Construit le graphe à partir des données OSM
    void buildGraph();

    // Calcule la distance géographique entre deux points (en mètres)
    static double distance(double lat1, double lon1, double lat2, double lon2);

    // Affiche un résumé du graphe (nombre de sommets et d’arêtes)
    void printSummary() const;

    // Retourne une référence constante vers le graphe
    const RoadGraph& getGraph() const { return graph; }

private:
    const std::vector<OSMNode>& nodes;  // Référence vers les nœuds OSM
    const std::vector<OSMWay>& ways;    // Référence vers les routes OSM
    RoadGraph graph;                    // Le graphe Boost construit
    std::unordered_map<long, Vertex> idToVertex; // lien entre ID OSM et sommet Boost
};
