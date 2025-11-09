#include "osm_reader.h"
#include <iostream>
#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>

using namespace std;

// ==========================================================
// Classe interne : MyHandler
// Sert à traiter les objets lus dans le fichier OSM
// ==========================================================
class MyHandler : public osmium::handler::Handler {
public:
    vector<OSMNode>& nodes;
    vector<OSMWay>& ways;

    // Constructeur : on récupère les références vers les vecteurs
    MyHandler(vector<OSMNode>& n, vector<OSMWay>& w) : nodes(n), ways(w) {}

    // Appelée automatiquement pour chaque <node> du fichier OSM
    void node(const osmium::Node& n) noexcept {
        OSMNode node;
        node.id = n.id();
        node.lat = n.location().lat();
        node.lon = n.location().lon();
        nodes.push_back(node);
    }

    // Appelée automatiquement pour chaque <way> (route ou chemin)
    void way(const osmium::Way& w) noexcept {
        OSMWay way;
        way.id = w.id();

        // On stocke la liste des identifiants de nœuds qui composent la route
        for (const auto& nr : w.nodes()) {
            way.nodeRefs.push_back(nr.ref());
        }

        // On vérifie si la route est à sens unique
        way.oneway = w.tags().has_tag("oneway", "yes");

        // type de route
        if (w.tags().has_key("highway")) {
            way.highwayType = w.tags().get_value_by_key("highway");
        } else {
            way.highwayType = "unknown";
        }
       // std::cout << "Way ID: " << way.id << " type: " << way.highwayType << std::endl;

        // On ajoute la route à la liste des ways
        ways.push_back(way);
    }
};

// ==========================================================
// Méthodes de la classe OSMReader
// ==========================================================

// Constructeur : enregistre le chemin du fichier
OSMReader::OSMReader(const string& path) : filePath(path) {}

// Lecture complète du fichier OSM
void OSMReader::read() {
    try {
        osmium::io::File file(filePath);
        osmium::io::Reader reader(file);

        MyHandler handler(nodes, ways);

        // Lance la lecture : appelle handler.node() et handler.way() automatiquement
        osmium::apply(reader, handler);

        reader.close();
        cout << "Lecture terminée sans erreur" << endl;
    } catch (const exception& e) {
        cerr << "Erreur lors de la lecture OSM : " << e.what() << endl;
    }
}

// Affiche un résumé des données chargées
void OSMReader::printSummary() const {
    cout << "Résumé du fichier :" << endl;
    cout << "  Nodes : " << nodes.size() << endl;
    cout << "  Ways  : " << ways.size() << endl;
}
