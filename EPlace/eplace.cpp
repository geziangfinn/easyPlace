#include "eplace.h"

void EPlacer_2D::fillerInitialization()
{
    //!//////////////////////////////////////////////////////////////
    // calculate whitespace area
    /////////////////////////////////////////////////////////////////

    //! whitespace: area of placement rows - overlap area between placement rows and terminals see RePlace opt.cpp whitespace_init()
    float whitespaceArea = 0;
    float totalOverLapArea = 0; // overlap area between placement rows and terminals

    for (Module *curTerminal : db->dbTerminals)
    {
        CRect terminal;
        terminal.ll = curTerminal->getLL_2D();
        terminal.ur = curTerminal->getUR_2D();

        for (SiteRow curRow : db->dbSiteRows)
        {
            CRect placementRow;
            placementRow.ll = curRow.getLL_2D();
            placementRow.ur = curRow.getUR_2D();
            totalOverLapArea += getOverlapArea_2D(terminal, placementRow);
        }
    }

    whitespaceArea = db->totalRowArea - totalOverLapArea;

    ////////////////////////////////////////////////////////////////
    // calculate node area
    ////////////////////////////////////////////////////////////////

    float nodeAreaScaled = 0; // node = std cells + movable macros
    float stdcellArea = 0;
    float macroArea = 0;

    for (Module *curNode : db->dbNodes)
    {
        assert(curNode->getArea() > 0);
        if (curNode->isMacro)
        {
            macroArea += curNode->getArea();
        }
        else
        {
            stdcellArea += curNode->getArea();
        }
    }

    nodeAreaScaled = stdcellArea + macroArea * targetDensity; // see ePlace paper equation (13), Am is nodeArea here

    ////////////////////////////////////////////////////////////////
    // calculate filler area
    ////////////////////////////////////////////////////////////////

    float totalFillerArea = 0;                                         // area of all fillers, how to calculate: see ePlace paper (13)
    totalFillerArea = whitespaceArea * targetDensity - nodeAreaScaled; //! see ePlace paper (13)

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

    avg90NodeArea = avg90TotalArea / ((float)(maxIdx - minIdx));

    float fillerArea = avg90NodeArea; //! use average area of the middle 90% of all nodes as filler area!
    float fillerHeight = db->commonRowHeight;
    float fillerWidth = float_div(fillerArea, fillerHeight);

    ////////////////////////////////////////////////////////////////
    // add fillers
    ////////////////////////////////////////////////////////////////
    
    int fillerCount=(int)(totalFillerArea / fillerArea + 0.5);//!



    //??? macro area should *= target density when calculating Am in(13)? see RePlAce code opt.cpp line 86 But terminal area wasn't *= target density when calculating Aws??? why implement as this for now
    // macro density scaling in density computation: RePlace bin.cpp line 1853
    // terminal(fixed macro) density scaling in density computation?: RePlace bin.cpp line 480
    //!! It seems that in RePlAce, bins are allocated on coreRegion!
    // density scaling when calculating Aws?
}
