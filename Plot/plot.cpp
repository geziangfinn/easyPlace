#include "plot.h"

int getX(float regionLLx,float x, float unitX)
{
    return (x - regionLLx) * unitX;
}

// the Y-axis must be mirrored
int getY(float regionHeight,float regionLLy,float y, float unitY)
{
    return (regionHeight - (y - regionLLy)) * unitY; //?
    // return (chipRegionHeight - y) * unitY;//?
}