#include "global.h"
#include "objects.h"
#include "placedb.h"
#include "fft.h"
class Bin_2D;
class Bin_3D; // call it cube?
class EPlacer_2D;
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

    //! here Density actually means Area(electric quantity) rather than electric density, the true density should be calculated as: area/bin area,
    //! and here the densities are actually the overlap area between each component and bin
    float nodeDensity;     //! density due to movable modules, use this to calculate overflow! see ePlace paper equation(37)
    float fillerDensity;   //! density due to fillers, use this and node density to do nesterov optimization
    float terminalDensity; // terminalDensity and baseDensity are calculated in binInitialization
    float baseDensity;     // see bin->virt_area in RePlAce, baseDensity: area inside a bin but not inside a placement row
    // todo: add virtual area?

    VECTOR_2D E; // electric field, see eplace paper
    float phi;   // electric potential, see eplace paper
    void init()
    {
        center.SetZero();
        ll.SetZero();
        ur.SetZero();
        nodeDensity = 0;
        fillerDensity = 0;
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
        init();
        db = _db;
    }
    vector<vector<Bin_2D *>> bins;

    float ePlaceStdCellArea; // calculated in fillerInitialization
    float ePlaceMacroArea;

    VECTOR_2D_INT binDimension; // How many bins in X/Y direction
    VECTOR_2D binStep;          // length of a bin in X/Y direction

    PlaceDB *db;
    vector<Module *> ePlaceFillers;         // stores all filler cells. I think filler cells shouldn't be stored in placedb. is this unnecessary when we have ePlaceNodesAndFillers?
    vector<Module *> ePlaceNodesAndFillers; //! contains nodes and fillers

    vector<VECTOR_2D> wirelengthGradient; // store wirelength gradient for nodes only(wirelength gradient for filler is always 0)
    vector<VECTOR_2D> densityGradient;    // store density gradient for fillers nodes (wirelength gradient for filler node is always 0)
    vector<VECTOR_2D> totalGradient;// total gradient of objective function f including gradients of all components

    float targetDensity;         //!!!!!! so important
    float globalDensityOverflow; // !!!!! so important, tau
    VECTOR_2D invertedGamma;         // gamma of the wa wirelength model,here we actually use 1/gamma, following RePlAce. gamma is different for different dimension

    void init()
    {
        bins.clear();
        binDimension.SetZero();
        binStep.SetZero();

        targetDensity = 0;
        globalDensityOverflow = 0;
        invertedGamma .SetZero();

        ePlaceStdCellArea = 0;
        ePlaceMacroArea = 0;

        db = NULL;
        ePlaceFillers.clear();
        ePlaceNodesAndFillers.clear();

        wirelengthGradient.clear();
        densityGradient.clear();
    }
    void setTargetDensity(float);

    void initialization();

    void fillerInitialization();
    void binInitialization();
    void gradientVectorInitialization();

    void densityOverflowUpdate();
    void wirelengthGradientUpdate();
    void densityGradientUpdate();

    void binNodeDensityUpdate(); //! only consider density from movable modules(nodes) in this function, because terminal density only needed to be calculated once, in binInitializaton()

    //! be aware of density scaling and local smooth in density calculation
};