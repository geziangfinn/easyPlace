#ifndef PLACEDB_H
#define PLACEDB_H
#include "objects.h"
#include "plot.h"
class PlaceDB
{
public:
    PlaceDB()
    {
        layerCount = -1;
        moduleCount = -1;
        dbMacroCount = 0;
        commonRowHeight = -1;
        dbNodes.clear();
        dbTerminals.clear();
        dbPins.clear();
        dbNets.clear();
        dbSiteRows.clear();
        dbTiers.clear();
        coreRegion = CRect();
        chipRegion = CRect();
        totalRowArea = 0;
    };
    int layerCount;  // how many layers? this is for 3dic
    int moduleCount; // number of modules
    int dbMacroCount;
    int netCount;
    int pinCount;
    double commonRowHeight; //! rowHeight that all(most of the times) rows share

    CRect coreRegion;   // In 2d placement the core region is just the rectangle that encloses all placement rows(see setCoreRegion), in 3d it might be shrunk. coreRegion should be smaller than the whole chip
    CRect chipRegion;   // Chip Region is obtained with coreRegion and all terminal locations. adapect1 is a good example. This should only be used for plot.
    float totalRowArea; //! area of all placement rows, equal or less than coreRegion area, usually equals coreRegion area. calculated in setCoreRegion

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
    void addNet(Net *);
    int addPin(Module *, Net *, float, float);

    void allocateNodeMemory(int);
    void allocateTerminalMemory(int);
    void allocateNetMemory(int);
    void allocatePinMemory(int);

    Module *getModuleFromName(string);

    void setModuleLocation_2D(Module *, float, float);
    void setModuleCenter_2D(Module *, float, float);
    void setModuleCenter_2D(Module *, POS_3D);
    void setModuleCenter_2D(Module *, VECTOR_3D);
    void setModuleOrientation(Module *, int);
    void setModuleLocation_2D_random(Module *);
    void moveModule_2D(Module *, VECTOR_2D);
    void moveModule_2D(Module *, VECTOR_2D_INT);
    void randomPlacment();
    void addNoise();

    POS_3D getValidModuleCenter_2D(Module *module, float x, float y);

    double calcHPWL();
    double calcWA_Wirelength_2D(VECTOR_2D);
    double calcLSE_Wirelength_2D(VECTOR_2D);
    double calcNetBoundPins();
    double calcModuleHPWL(Module *);

    void moveNodesCenterToCenter(); // used for initial 2D quadratic placement

    void setChipRegion_2D();

    void showDBInfo();

    void outputBookShelf();
    void outputAUX();
    void outputNodes();
    void outputPL();
    void outputNets();
    void outputSCL();

    void plotCurrentPlacement(string);
};
#endif