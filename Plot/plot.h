#ifndef PLOT_H
#define PLOT_H
#include "global.h"
#include "placedb.h"
#include "CImg.h"
const unsigned char Blue[] = {120, 200, 255},
                    Black[] = {0, 0, 0},
                    Red[] = {255, 0, 0};
class Plotter
{
public:
    Plotter()
    {
        db = NULL;
    }
    Plotter(PlaceDB *_db)
    {
        db = _db;
    }
    PlaceDB *db;
    string plotPath;
    void setPlotPath(string);
    void plotCurrentPlacement(string imgName);
    int getX(float, float);
    int getY(float, float);
};

#endif