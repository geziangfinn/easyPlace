#ifndef LEGALIZER_H
#define LEGALIZER_H
#include "global.h"
#include "objects.h"
#include "placedb.h"
class Obstacle : public CRect
{

};
class CellCluster{
    // cluster of std cells, used in abacus
	CellCluster()
    {
        Init();
    }
    void Init()
    {
        cells.clear();
    }
    vector<Module*> cells;//cells should be ordered by x coordinate(non decreasing)
	int index;
    // x,e,w,q of a cluster, see the abacus paper
    float x;
	float e;
	float w;
	float q;
};
class AbacusLegalizer
{
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
    vector<SiteRow> subrows;  // generated from dbSiteRows and Terminals, this is actually used during legalization
    vector<Obstacle> obstacles; // include macro and terminals, macros should be legalized first!
    vector<Module*> dbCells;// all std cells, sorted by x coordinate(non decreasing order)
    void legalization();

private:
    // set these functions private because they are called in other member functions only
    // follow the abacus paper
    void initialization();
    void initializeCells();
    void initializeObstacles();// obstacles include macros and terminals, macros should be legalized first!
    void initializeSubrows();
    void placeRow();
    void addCell();
    void addCluster();
    void collapse();
};
#endif