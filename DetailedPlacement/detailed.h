#ifndef DETAILED_H
#define DETAILED_H
#include <global.h>
#include "placedb.h"
class ISMDP;
class ISMRow;
class LocalReorderingDP;
class LRSegment;
class LRSolution;
class LRSolutionIterator;
class LRSolver;
class GlobalSwapDP;

class DetailedPlacer
{
public:
    DetailedPlacer()
    {
        Init();
    }
    DetailedPlacer(PlaceDB *_db)
    {
        Init();
        placedb = _db;
    }
    void Init()
    {
        placedb = NULL;
    }
    PlaceDB *placedb;
    void detailedPlacement();

private:
    void initialization();
    void runISM();
    void runGlobalSwap();
    void runLocalReordering(); // branch and bound cell swap = local reordering
};

class ISMDP // ISMDP: Independent Set Matching based Detailed Placement
{
public:
    ISMDP()
    {
        Init();
    }
    ISMDP(PlaceDB *_db)
    {
        Init();
        placeDB = _db;
    }
    void Init()
    {
        placeDB = NULL;
        ISMRows.clear();
        maxModuleCount = 0;
        doubleWindow = false;
        independentCells = false;
    }
    void initialization();

    //! core function, see grid_run() in ntuplace3
    void ISMSweep(int, int); // int windowSize, int windowOverlap

    PlaceDB *placeDB;
    vector<ISMRow> ISMRows;
    int maxWindow;      // see ntuplace class de_Detail.MAXWINDOW
    int maxModuleCount; // max number of modules in ISM window(s) (not one ISMRun()), see ntuplace class de_Detail.MAXMODULE
    bool doubleWindow;
    bool independentCells;

private:
    void initializeParams();
    void initializeISMRows();
    void ISMRun(CRect);
};

class ISMRow
{
public:
    ISMRow()
    {
        Init();
    }
    ISMRow(double _x = 0, double _y = 0, double _length = 0)
    {
        Init();
        ll.x = _x;
        ll.y = _y;
        length = _length;
    }
    void Init()
    {
        rowSpaces.clear();
        rowModules.clear();
    }

    bool insertModule(double, Module *); // see ntuplace, detail.cpp insert_module()
    void removeModule(Module *);
    bool removeTail(double, double);  // double: x coordiante, double: width, see ntuplace, detail.cpp, remove_empty()
    bool insertSpace(double, double); // double: x coordiante, double: width, see ntuplace, detail.cpp add_empty()

    void showSpace();
    void showModule();

    POS_2D ll; // ll: lower left
    double length;
    map<double, double> rowSpaces;    // key: x-coordinate, value: length of the space of the , space is actually similar to interval, it is just inconvenient to operate directly the the intervals stored in placedb
    map<double, Module *> rowModules; // key: x-coordinate, value: module
    //? consider: use double or int as the key of the above 2 maps? I can use int for now, it should be easy to switch to double if necessary.
    // use int as key is ok because the site step is usually integer, and x-coordinate is usually integer multiples of site step
};

class lap2 // lap: linear assignment problem
{
public:
    lap2(int _deg)
    {
        degree = _deg;
        cost.resize(degree);
        for (unsigned int i = 0; i < cost.size(); ++i)
        {
            cost[i].resize(degree, 0);
        }
        assignment.resize(degree, 0);
        INF = INT_MAX;
        verbose = false;
    }
    ~lap2(void) {};

    void put(const int &i, const int &j, double wl)
    {
        cost[i][j] = static_cast<int>(wl);
    }

    int INF;
    int degree;               // int m_deg;
    vector<vector<int>> cost; // vector<vector<int>> m_cost;
    vector<int> assignment;   // vector<int> m_assignment;
    int lapSolve();
    void getResult(vector<int> &v)
    {
        v = this->assignment;
    }
    bool verbose;
};

class LocalReorderingDP // local reorder == bb cell swap
{
public:
    LocalReorderingDP()
    {
        Init();
    }
    LocalReorderingDP(PlaceDB *_db)
    {
        Init();
        placeDB = _db;
    }
    void Init()
    {
        placeDB = NULL;
        lrSegments.clear();
    }
    PlaceDB *placeDB;
    vector<LRSegment> lrSegments;
    void initialization();
    //! core function
    void solve(int, int, int); // int windowSize, int overlapSize, int iterationNumber

private:
    void initializeSegments();
    bool solveForBestOrder(vector<Module *>::iterator, vector<Module *>::iterator); // todo: implement me
};

class LRSegment // The placeable interval in each row is called a segment, pretty much like an abacus row. Initialized with placedb->dbSiteRows.intervals
{               // LR:local reordering
public:
    LRSegment()
    {
        Init();
    }
    LRSegment(double _bottom, double _start, double _end)
    {
        Init();
        bottom = _bottom;
        start = _start;
        end = _end;
    }
    void Init()
    {
        bottom = -1;
        start = -1;
        end = -1;
        segModules.clear();
    }

    double bottom;               // The bottom y coordinate of this row of sites
    double start, end;           // The left and right coordinates of this row of sites
    vector<Module *> segModules; // Record the  modules on this segment
    void addModule(Module *module)
    {
        segModules.push_back(module);
    }
};

class compareLRSegment
{
public:
    bool operator()(const LRSegment &s, const LRSegment &compSeg)
    {
        if (s.bottom == compSeg.bottom)
        {
            if (s.start <= compSeg.start)
            {
                return s.end < compSeg.end;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return s.bottom < compSeg.bottom;
        }
    }
};

class LRSolution // one solution (ordering) of cells in a local reorder window
{
public:
    LRSolution(PlaceDB *_db)
    {
        Init();
        placedb = _db;
    }
    void Init()
    {
        placedb = NULL;
        solutionCost = 0;
        insertedCells.clear();
        uninsertedCells.clear();
        whiteSpaceWidth = 0;
        currentX = -1;
        xLocations.clear();
        netModuleCount.clear();
    }
    // LRSolution( const LRSolution& sol );

    bool isComplete() { return (uninsertedCells.size() == 0); } // insertedCells + uninsertedCells == all modules in the window
    double getCost()
    {
        return solutionCost;
    }
    LRSolution *clone();
    void print();
    void initializeNetModuleCount();
    void recalculateCost(); // Recalculate the total wirelength in insertedCells if this solution is complete

    PlaceDB *placedb;
    double solutionCost;            // For incrementally compute the solution cost, see m_bound in ntuplace3
    list<Module *> insertedCells;   // inserted modules (locations are determined in this solution)
    list<Module *> uninsertedCells; // to be inserted modules (locations not determined in this solution, NULL is whitespace)
    double whiteSpaceWidth;         // The width of the whitespace
    double currentX;                // left most x coordinate of the whole segment of cells

    list<double> xLocations; // Record the x coordinate of each module in insertedCells

    map<int, int> netModuleCount; // number of modules in each related net of the modules in the window, key: net id, do not use Net* as key
};

class LRSolutionIterator
{
public:
    LRSolutionIterator(const LRSolution *sol)
        : pointedSolution(sol)
    {
        reset();
    }
    void reset()
    {
        moduleIterator = pointedSolution->uninsertedCells.begin();
    }
    bool isDone()
    {
        return moduleIterator == pointedSolution->uninsertedCells.end();
    }
    LRSolution *createSuccessorSolution();

    list<Module *>::const_iterator moduleIterator; // The iterator points to current uninserted cell in m_sol
    const LRSolution *pointedSolution;
};

class LRSolver
{
public:
    LRSolver() : bestSolution(NULL), bestCost(DOUBLE_MAX) {}
    ~LRSolver() { delete bestSolution; }

    LRSolution *bestSolution;
    double bestCost;

    LRSolution solve(LRSolution *);

    void setBestSolution(LRSolution *);

    void updateBestSolution(LRSolution *);

    void depthFirstSolve(LRSolution *);
};

class GlobalSwapDP
{
};
#endif