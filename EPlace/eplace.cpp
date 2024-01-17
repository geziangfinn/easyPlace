#include "eplace.h"

void EPlacer_2D::fillerInitialization()
{
    int nodeCount = db->dbNodes.size();

    int minIdx = (int)(0.05 * (float)nodeCount); //! for calculating average area of the middle 90% of all nodes(cells and macros)
    int maxIdx = (int)(0.95 * (float)nodeCount);

    vector<float> nodeArea; // sort node according to area with this because we don't want to sort dbNodes
    nodeArea.resize(nodeCount);

    for (int i = 0; i < nodeCount; i++)
    {
        nodeArea[i] = db->dbNodes[i]->getArea();
        // use this one below if we want no problems:
        // nodeArea[i]=db->dbNodes[i]->calcArea();
    }

    sort(nodeArea.begin(), nodeArea.end()); //! sort by area

    float avg90TotalArea = 0;
    float avg90NodeArea = 0;

    for (int i = minIdx; i < maxIdx; i++)
    {
        avg90TotalArea += nodeArea[i];
    }

    avg90NodeArea=avg90TotalArea/((float)(maxIdx - minIdx));

    float fillerArea=avg90NodeArea;//! use average area of the middle 90% of all nodes as filler area!
    float fillerHeight=db->commonRowHeight;
    float fillerWidth=float_div(fillerArea,fillerHeight);

    float totalFillerArea=0;// area of all fillers, how to calculate: see ePlace paper (13)

}
