#ifndef DETAILED_H
#define DETAILED_H
#include <global.h>
#include "placedb.h"
class ISMRow;
class ISMDP;

class DetailedPlacer
{
};

class ISMDP // ISMDP: independent set matching based detailed placement
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
    }

    PlaceDB *placeDB;
    vector<ISMRow> ISMRows;

private:
    void initilization();
    void initializeISMRows();
};

class ISMRow
{
public:
    map<double, double> m_empties; // x-coordinate,length
    map<double, int> m_rowmodule;  // x-coordinate,module ID, map instead of vector
    //? consider: use double or int as the key of the above 2 maps? I can use int for now, it should be easy to switch to double if necessary.
};
#endif