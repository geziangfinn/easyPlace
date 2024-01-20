#include "global.h"
#include "objects.h"
#include "placedb.h"
class Bin_2D;
class Bin_3D; // call it cube?
class EPlacer;
class Bin_2D
{
public:
    Bin_2D()
    {
        init();
    }
    POS_2D center;
    POS_2D ll;
    POS_2D ur;
    float width;
    float height;
    float area;

    // density equals nodeDensity+terminalDensity+baseDensity
    float nodeDensity;     //! density due to movable modules, use this to calculate overflow! see ePlace paper equation(37)
    float terminalDensity; // terminalDensity and baseDensity are calculated in binInitialization
    float baseDensity;     // see virtual area in RePlAce, baseDensity: area inside a bin but not inside a placement row

    // todo: add virtual area?

    VECTOR_2D E; // electric field, see eplace paper
    float phi;   // electric potential, see eplace paper
    void init()
    {
        center.SetZero();
        ll.SetZero();
        ur.SetZero();
        nodeDensity = 0;
        terminalDensity = 0;
        baseDensity = 0;
        E.SetZero();
        phi = 0;
        area = 0;
        width = 0;
        height = 0;
    }
    float getWidth() { return width; }
    float getHeight() { return height; }
    float getArea()
    {
        area = width * height;
        return width * height;
    }
};

class EPlacer_2D
{
public:
    EPlacer_2D()
    {
        init();
    }

    EPlacer_2D(PlaceDB *_db)
    {
        db = _db;
    }
    vector<vector<Bin_2D *>> bins;

    float ePlaceStdCellArea; // calculated in fillerInitialization
    float ePlaceMacroArea;

    VECTOR_2D_INT binDimension; // How many bins in X/Y direction
    VECTOR_2D binStep;          // length of a bin in X/Y direction

    PlaceDB *db;
    vector<Module *> ePlaceFillers; // stores all filler cells. I think filler cells shouldn't be stored in placedb.
    vector<Module *> ePlaceNodes;   //! contains nodes and fillers

    float targetDensity; //!!!!!! so important
    void init()
    {
        bins.clear();
        binDimension.SetZero();
        binStep.SetZero();
        targetDensity = 0;

        ePlaceStdCellArea = 0;
        ePlaceMacroArea = 0;

        db = NULL;
        ePlaceFillers.clear();
        ePlaceNodes.clear();
    }
    void setTargetDensity(float);
    void fillerInitialization();
    void binInitialization();
    void binNodeDensityUpdate(); //! only consider density from movable modules(nodes) in this function, because terminal density only needed to be calculated once, in binInitializaton()

    //! be aware of density scaling and local smooth in density calculation
};