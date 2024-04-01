#ifndef LEGALIZER_H
#define LEGALIZER_H
#include "global.h"
#include "objects.h"
#include "placedb.h"
#define ABACUS_TRIAL true
#define ABACUS_FINAL false
class Obstacle : public CRect
{
};
class AbacusCellCluster
{
public:
    // cluster of std cells, used in abacus
    AbacusCellCluster()
    {
        Init();
    }
    void Init()
    {
        cells.clear();
        index=-1;
        x=0.0;
        e=0.0;
        w=0.0;
        q=0.0;
    }
    vector<Module *> cells; // cells should be ordered by x coordinate(non decreasing)
    int index;// its index in the vector clusters of class AbacusRow
    // x,e,w,q of a cluster, see the abacus paper
    float x;
    float e;
    float w;
    float q;
};

class AbacusRow : public SiteRow
{
public:
    AbacusRow()
    {
        clusters.clear();
        //lastClusterIndex=-1;
    }
    vector<AbacusCellCluster> clusters;
    void addCell(int,Module*);
    void addCluster(int,int);
    void collapse(int);
};

class AbacusLegalizer
{
public:
    AbacusLegalizer()
    {
        Init();
    }
    AbacusLegalizer(PlaceDB *_db)
    {
        Init();
        placeDB = _db;
    }
    void Init()
    {
        placeDB = NULL;
    }
    PlaceDB *placeDB;
    vector<AbacusRow> subrows;  // generated from dbSiteRows and Terminals, this is actually used during legalization
    vector<Obstacle> obstacles; // include macro and terminals, macros should be legalized first!
    vector<Module *> dbCells;   // all std cells, sorted by x coordinate(non decreasing order)
    void legalization();

private:
    // set these functions private because they are called in other member functions only
    // follow the abacus paper
    void initialization();
    void initializeCells();
    void initializeObstacles(); // obstacles include macros and terminals, macros should be legalized first!
    void initializeSubrows();
    double placeRow(Module *, int, bool);
};

class MacroLegalizer
{
    // todo: build a macro legalizer
};
#endif