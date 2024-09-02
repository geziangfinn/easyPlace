#ifndef DETAILED_H
#define DETAILED_H
#include <global.h>
#include "placedb.h"
class ISMDP;
class ISMRow;
class LocalReorderingDP;
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
    void ISMSweep(int, int); // int windowSize, int windowOverlap
    void ISMRun(CRect);
    PlaceDB *placeDB;
    vector<ISMRow> ISMRows;
    int maxModuleCount; // max number of modules in ISM window(s) (not one ISMRun())
    bool doubleWindow;
    bool independentCells;

private:
    void initializeParams();
    void initializeISMRows();
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

class LocalReorderingDP
{
};
class GlobalSwapDP
{
};
#endif