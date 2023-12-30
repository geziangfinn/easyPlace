#ifndef PLACEDB_H
#define PLACEDB_H
#include "objects.h"
class PlaceDB
{
public:
    PlaceDB()
    {
        layerCount = -1;
        dbModules.clear();
        dbPins.clear();
        dbNets.clear();
        dbTiers.clear();
    };
    int layerCount; // how many layers? this is for 3dic
    vector<Module *> dbModules;
    vector<Pin *> dbPins;
    vector<Net *> dbNets;
    vector<Tier *> dbTiers;
    void init_tiers();
    inline void addModule(string name, float width, float height, bool isFixed, bool isNI); // (frank) 2022-05-13 consider terminal_NI
    void addNet();
    void addPin();
    void CreateModuleNameMap();
    void ClearModuleNameMap(); 
};
#endif