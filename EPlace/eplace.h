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
    float density;
    VECTOR_2D E; // electric field, see eplace paper
    float phi;   // electric potential, see eplace paper
    void init()
    {
        center.SetZero();
        ll.SetZero();
        ur.SetZero();
        density = 0;
        E.SetZero();
        phi = 0;
    }
};

class EPlacer_2D
{
public:
    EPlacer_2D()
    {
        bins.clear();
        binDimensionX = 0;
        binDimensionY = 0;
        targetDensity = 0;
        db = NULL;
    }
    vector<vector<Bin_2D>> bins;
    int binDimensionX;
    int binDimensionY;
    PlaceDB *db;
    vector<Module *> fillers;     // stores all filler cells. I think filler cells shouldn't be stored in placedb.
    vector<Module *> ePlaceNodes; //! contains nodes and fillers
    void fillerInitialization();
    float targetDensity; //!!!!!! so important
    // todo: watch out density scaling and local smooth in density calculation
};