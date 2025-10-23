#include "osm_reader.h"
#include <iostream>
#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>

using namespace std;

/* =====================
   1️⃣ Classe interne : MyHandler
   ===================== */
class MyHandler : public osmium::handler::Handler {
public:
    vector<OSMNode>& nodes;
    vector<OSMWay>& ways;

    MyHandler(vector<OSMNode>& n, vector<OSMWay>& w) : nodes(n), ways(w) {}

    void node(const osmium::Node& n) noexcept {
        OSMNode node;
        node.id = n.id();
        node.lat = n.location().lat();
        node.lon = n.location().lon();
        nodes.push_back(node);
    }

    void way(const osmium::Way& w) noexcept {
        OSMWay way;
        way.id = w.id();

        for (const auto& nr : w.nodes()) {
            way.nodeRefs.push_back(nr.ref());
        }

        way.oneway = w.tags().has_tag("oneway", "yes");
        ways.push_back(way);
    }
};

/* =====================
   2️⃣ Méthodes de OSMReader
   ===================== */

OSMReader::OSMReader(const string& path) : filePath(path) {}

void OSMReader::read() {
    try {
        osmium::io::File file(filePath);
        osmium::io::Reader reader(file);

        MyHandler handler(nodes, ways);
        osmium::apply(reader, handler);
        reader.close();

        cout << "Lecture terminée sans erreur" << endl;
    } catch (const exception& e) {
        cerr << "Erreur lors de la lecture OSM : " << e.what() << endl;
    }
}

void OSMReader::printSummary() const {
    cout << "📄 Résumé du fichier :" << endl;
    cout << "  Nodes : " << nodes.size() << endl;
    cout << "  Ways  : " << ways.size() << endl;
}

