#ifndef PLACEDB_H
#define PLACEDB_H
#include "objects.h"
class PlaceDB
{
public:
    PlaceDB()
    {
        layerCount = -1;
        moduleCount=-1;
        commonRowHeight=-1;
        dbNodes.clear();
        dbTerminals.clear();
        dbPins.clear();
        dbNets.clear();
        dbSiteRows.clear();
        dbTiers.clear();
        coreRegion=CRect();
    };
    int layerCount;  // how many layers? this is for 3dic
    int moduleCount; // number of modules
    int netCount;
    int pinCount;
    double commonRowHeight; //! rowHeight that all(most of the times) rows share
    CRect coreRegion; // In 2d placement the core region is just the rectangle representing the chip, in 3d it might be shrunk 
    // vector<Module *> dbModules;

    //! dbXxs: vector for storing Xxs
    vector<Module *> dbNodes; // nodes include std cells and macros
    vector<Module *> dbTerminals;
    vector<Pin *> dbPins;
    vector<Net *> dbNets;
    vector<SiteRow> dbSiteRows;
    vector<Tier *> dbTiers;

    map<string, Module *> moduleMap; // map module name to module pointer(module include nodes and terminals)
    
    void setCoreRegion();
    void init_tiers();
    Module *addNode(int index, string name, float width, float height); // (frank) 2022-05-13 consider terminal_NI
    Module *addTerminal(int index, string name, float width, float height, bool isFixed, bool isNI);
    void addNet(Net*);
    int addPin(Module*,Net*,float,float);
    void allocateNodeMemory(int);
    void allocateTerminalMemory(int);
    void allocateNetMemory(int);
    void allocatePinMemory(int);
    Module* getModuleFromName(string);
};
#endif