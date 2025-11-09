#pragma once
#include <vector>
#include <string>

struct OSMNode {
    long id;
    double lat;
    double lon;
};

struct OSMWay {
    long id;
    std::vector<long> nodeRefs;
    bool oneway;
    std::string highwayType;
};

class OSMReader {
public:
    explicit OSMReader(const std::string& filePath);
    void read();             // lit et stocke les données
    void printSummary() const; // affiche un résumé

    std::vector<OSMNode> nodes;
    std::vector<OSMWay> ways;

private:
    std::string filePath;
};

