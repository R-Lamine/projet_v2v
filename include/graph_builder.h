#pragma once
#include <unordered_map>
#include <vector>
#include "graph_types.h"
#include "osm_reader.h"

// Classe responsable de construire le graphe routier
// à partir des données OSM (nodes et ways)
class GraphBuilder {
public:
    // Constructeur : reçoit les données lues par OSMReader
    GraphBuilder(const std::vector<OSMNode>& nodes,
                 const std::vector<OSMWay>& ways);

    // Construit le graphe complet (sommets + arêtes)
    void buildGraph();

    // Affiche un petit résumé (nombre de sommets, d'arêtes, etc.)
    void printSummary() const;

    // Retourne une référence constante vers le graphe construit
    const RoadGraph& getGraph() const { return graph; }

private:
    // Données sources provenant du fichier OSM
    const std::vector<OSMNode>& nodes;
    const std::vector<OSMWay>& ways;

    // Graphe routier résultant
    RoadGraph graph;

    // Correspondance entre ID OSM et Vertex Boost
    std::unordered_map<long, Vertex> osmIdToVertex;
};
