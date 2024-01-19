#include "eplace.h"

void EPlacer_2D::setTargetDensity(float target)
{
    targetDensity = target;
    cout << padding << "Target density set: " << padding << endl;
}

void EPlacer_2D::fillerInitialization()
{
    //!//////////////////////////////////////////////////////////////
    // calculate whitespace area
    /////////////////////////////////////////////////////////////////

    //! whitespace: area of placement rows - overlap area between placement rows and terminals see RePlace opt.cpp whitespace_init() and ePlace Paper
    float whitespaceArea = 0;
    float totalOverLapArea = 0; // overlap area between placement rows and terminals

    for (Module *curTerminal : db->dbTerminals)
    {
        if (curTerminal->isNI) //!
        {
            continue;
        }
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

    ePlaceStdCellArea = stdcellArea;
    ePlaceMacroArea = macroArea;

    nodeAreaScaled = stdcellArea + macroArea * targetDensity; // see ePlace paper equation (13), Am is nodeArea here
    //??? macro area should *= target density when calculating Am in(13)? see RePlAce code opt.cpp line 86 But terminal area wasn't *= target density when calculating Aws??? implement as this for now

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
    // add fillers and set filler locations randomly
    ////////////////////////////////////////////////////////////////

    int fillerCount = (int)(totalFillerArea / fillerArea + 0.5); //!

    ePlaceFillers.resize(fillerCount);

    float leftMost;
    float rightMost;

    for (int i = 0; i < fillerCount; i++)
    {
        string name = "f" + to_string(i);
        Module *curFiller = new Module(i, name, fillerWidth, fillerHeight, false, false);
        curFiller->isFiller = true;
        ePlaceFillers[i] = curFiller;

        db->setModuleLocation_2D_random(curFiller);
    }

    ePlaceNodes = db->dbNodes;
    ePlaceNodes.insert(ePlaceNodes.end(), ePlaceFillers.begin(), ePlaceFillers.end());
    // macro density scaling in density computation: RePlace bin.cpp line 1853
    // terminal(fixed macro) density scaling in density computation?: RePlace bin.cpp line 480

    // density scaling when calculating Aws?
}

void EPlacer_2D::binInitialization()
{
    //! Bins are allocated on coreRegion! RePlAce code bin.cpp line 337 and bookshelfIO.cpp, also see ePlace paper
    ////////////////////////////////////////////////////////////////
    // calculate bin dimension and size
    ////////////////////////////////////////////////////////////////
    //! this code for bin dimension calculation is copied directly from RePlAce
    int nodeCount = db->dbNodes.size();
    float nodeArea = (ePlaceStdCellArea + ePlaceMacroArea);

    float coreRegionWidth = db->coreRegion.ur.x - db->coreRegion.ll.x;
    float coreRegionHeight = db->coreRegion.ur.y - db->coreRegion.ur.x;
    float coreRegionArea = float_mul(coreRegionWidth, coreRegionHeight);

    float averageNodeArea = 1.0 * nodeArea / nodeCount;
    float idealBinArea = averageNodeArea / targetDensity;

    int idealBinCount = INT_CONVERT(coreRegionArea / idealBinArea);

    bool isUpdate = false;
    for (int i = 1; i <= 10; i++)
    { //! 4*4,8*8,16*16,32*32...
        if ((2 << i) * (2 << i) <= idealBinCount &&
            (2 << (i + 1)) * (2 << (i + 1)) > idealBinCount)
        {
            binDimension.x = binDimension.y = 2 << i;
            isUpdate = true;
            break;
        }
    }
    if (!isUpdate)
    {
        binDimension.x = binDimension.y = 1024; //!
    }

    binStep.x = float_div(coreRegionWidth, binDimension.x);
    binStep.y = float_div(coreRegionHeight, binDimension.y);

    ////////////////////////////////////////////////////////////////
    // add bins
    ////////////////////////////////////////////////////////////////
    // x stored in first dimension of vector
    //! bin index:
    //! # 5
    //! # 4
    //! # 3
    //! # 2
    //! # 1
    //! # 0
    //! #  0 1 2 3 4 5

    //?
    bins.resize(binDimension.x);

    for (int i = 0; i < binDimension.x; i++)
    {
        bins[i].resize(binDimension.y);
        for (int j = 0; j < binDimension.y; j++)
        {
            //!!! +db->coreRegion.ll.x to get coordinates!!
            bins[i][j]->ll.x = i * binStep.x + db->coreRegion.ll.x;
            bins[i][j]->ll.y = j * binStep.y + db->coreRegion.ll.y;

            bins[i][j]->width = binStep.x;
            bins[i][j]->height = binStep.y;

            bins[i][j]->ur.x = bins[i][j]->ll.x + bins[i][j]->width;
            bins[i][j]->ur.y = bins[i][j]->ll.y + bins[i][j]->height;

            bins[i][j]->area = binStep.x * binStep.y;

            bins[i][j]->center.x = bins[i][j]->ll.x + (float)0.5 * bins[i][j]->width;
            bins[i][j]->center.y = bins[i][j]->ll.y + (float)0.5 * bins[i][j]->height;
        }
    }

    ////////////////////////////////////////////////////////////////
    // terminal density calculation, calculate here because they are terminals and only needed to be considered once
    ////////////////////////////////////////////////////////////////
    VECTOR_2D_INT binStartIdx;
    VECTOR_2D_INT binEndIdx;

    for (Module *curTerminal : db->dbTerminals)
    {
        //! only consider terminals inside the coreRegion
        //! assume no terminal would have a part inside the coreRegion and a part outside the coreRegion
        if (curTerminal->getLL_2D().x < db->coreRegion.ll.x || curTerminal->getLL_2D().x + curTerminal->getWidth() > db->coreRegion.ur.x)
        {
            continue;
        }
        if (curTerminal->getLL_2D().y < db->coreRegion.ll.y || curTerminal->getLL_2D().y + curTerminal->getHeight() > db->coreRegion.ur.y)
        {
            continue;
        }

        binStartIdx.x = INT_DOWN((curTerminal->getLL_2D().x - db->coreRegion.ll.x) / binStep.x);
        binEndIdx.x = INT_DOWN((curTerminal->getUR_2D().x - db->coreRegion.ll.x) / binStep.x);

        binStartIdx.y = INT_DOWN((curTerminal->getLL_2D().y - db->coreRegion.ll.y) / binStep.y);
        binEndIdx.y = INT_DOWN((curTerminal->getUR_2D().y - db->coreRegion.ll.y) / binStep.y);

        assert(binStartIdx.x >= 0);
        assert(binEndIdx.x >= 0);
        assert(binStartIdx.y >= 0);
        assert(binEndIdx.y >= 0);

        if (binEndIdx.y >= binDimension.y)
        {
            binEndIdx.y = binDimension.y - 1;
        }

        if (binEndIdx.x >= binDimension.x)
        {
            binEndIdx.x = binDimension.x - 1;
        }

        for (int i = binStartIdx.x; i <= binEndIdx.x; i++)
        {
            for (int j = binStartIdx.y; j <= binEndIdx.y; j++)
            {
                //! beware: density scaling!
                bins[i][j]->terminalDensity += targetDensity * getOverlapArea_2D(bins[i][j]->ll, bins[i][j]->ur, curTerminal->getLL_2D(), curTerminal->getUR_2D());
            }
        }
    }
    ////////////////////////////////////////////////////////////////
    // base density calculation, also only needed to be considered once
    ////////////////////////////////////////////////////////////////
}

void EPlacer_2D::binDensityUpdate()
{
    VECTOR_2D_INT binStartIdx;
    VECTOR_2D_INT binEndIdx;

    // binStartIdx: index the index of the first bin that has overlap with a cell on X/Y direction

    for (Module *curNode : ePlaceNodes)
    {
        binStartIdx.x = INT_DOWN((curNode->getLL_2D().x - db->coreRegion.ll.x) / binStep.x);
        binEndIdx.x = INT_DOWN((curNode->getUR_2D().x - db->coreRegion.ll.x) / binStep.x);

        binStartIdx.y = INT_DOWN((curNode->getLL_2D().y - db->coreRegion.ll.y) / binStep.y);
        binEndIdx.y = INT_DOWN((curNode->getUR_2D().y - db->coreRegion.ll.y) / binStep.y);

        assert(binStartIdx.x >= 0);
        assert(binEndIdx.x >= 0);
        assert(binStartIdx.y >= 0);
        assert(binEndIdx.y >= 0);

        if (binEndIdx.y >= binDimension.y)
        {
            binEndIdx.y = binDimension.y - 1;
        }

        if (binEndIdx.x >= binDimension.x)
        {
            binEndIdx.x = binDimension.x - 1;
        }

        //! beware: local smooth and density scaling!
        for (int i = binStartIdx.x; i <= binEndIdx.x; i++)
        {
            for (int j = binStartIdx.y; j <= binEndIdx.y; j++)
            {
            }
        }
    }
}
