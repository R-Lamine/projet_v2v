#include "osm_reader.h"

int main() {
    OSMReader reader("data/strasbourg.osm.pbf");
    reader.read();
    reader.printSummary();
    return 0;
}

