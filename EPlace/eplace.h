#ifndef EPLACE_H
#define EPLACE_H

#include "global.h"
#include "objects.h"
#include "placedb.h"
#include "fft.h"
#include "plot.h"

#define DELTA_HPWL_REF 350000
#define PENALTY_MULTIPLIER_BASE 1.1
#define PENALTY_MULTIPLIER_UPPERBOUND 1.05
#define PENALTY_MULTIPLIER_LOWERBOUND 0.95

enum PLACEMENT_STAGE
{
    mGP,
    FILLERONLY,
    cGP

};

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

    vector<Module *> ePlaceFillers;         // for FILLERONLY placement stores all filler cells. I think filler cells shouldn't be stored in placedb. is this unnecessary when we have ePlaceNodesAndFillers?
    vector<Module *> ePlaceNodesAndFillers; //! contains nodes and fillers, for mGP
    vector<Module *> ePlaceCellsAndFillers; //! contains std cells and fillers, for cGP

    vector<VECTOR_3D> wirelengthGradient; // store wirelength gradient for nodes only(wirelength gradient for filler is always 0)
    vector<VECTOR_3D> densityGradient;    // store density gradient for fillers nodes (wirelength gradient for filler node is always 0)
    vector<VECTOR_3D> totalGradient;      // total gradient of objective function f including gradients of all components, for mGP
    vector<VECTOR_3D> fillerGradient;
    vector<VECTOR_3D> cGPGradient; // cell and fillers

    float targetDensity;         //!!!!!! so important
    float globalDensityOverflow; // !!!!! so important, tau
    VECTOR_2D invertedGamma;     // gamma of the wa wirelength model,here we actually use 1/gamma, following RePlAce. gamma is different for different dimension

    float lambda; // penalty factor
    double lastHPWL;

    int placementStage;
    int mGPIterationCount;

    void init()
    {
        bins.clear();
        binDimension.SetZero();
        binStep.SetZero();

        targetDensity = 0;
        globalDensityOverflow = 0;
        invertedGamma.SetZero();
        lambda = 0.0;
        lastHPWL = 0.0;

        ePlaceStdCellArea = 0;
        ePlaceMacroArea = 0;

        db = NULL;
        ePlaceFillers.clear();
        ePlaceNodesAndFillers.clear();

        wirelengthGradient.clear();
        densityGradient.clear();

        placementStage = mGP;
        mGPIterationCount=0;
        
    }
    void setTargetDensity(float);
    void setPlacementStage(int);

    void initialization();

    void fillerInitialization();
    void binInitialization();
    void gradientVectorInitialization();

    void binNodeDensityUpdate();  //! only consider density from movable modules(nodes) in this function, because terminal density only needed to be calculated once, in binInitializaton()
    void densityOverflowUpdate(); // called in wirelenghGradientUpdate()
    void wirelengthGradientUpdate();
    void densityGradientUpdate();

    void totalGradientUpdate();

    vector<VECTOR_3D> getGradient();
    vector<VECTOR_3D> getPosition();

    void setPosition(vector<VECTOR_3D>);

    void penaltyFactorInitilization(); // initialize lambda 0, see ePlace paper equation 35
    void updatePenaltyFactor();
    //! be aware of density scaling and local smooth in density calculation

    void switch2FillerOnly();
    void switch2cGP();

    void showInfo();
    void showInfoFinal();
    void plotCurrentPlacement(string);

private:
    vector<VECTOR_3D> getModulePositions(vector<Module *>);
};
#endif