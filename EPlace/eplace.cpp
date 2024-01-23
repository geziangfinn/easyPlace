#include "eplace.h"

void EPlacer_2D::setTargetDensity(float target)
{
    targetDensity = target;
    cout << padding << "Target density set: " << padding << endl;
}

void EPlacer_2D::initialization()
{
    fillerInitialization();
    binInitialization();
    gradientVectorInitialization();
}

void EPlacer_2D::fillerInitialization()
{
    segmentFaultCP("fillerInit");
    ////////////////////////////////////////////////////////////////
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

    nodeAreaScaled = stdcellArea + macroArea * targetDensity; // see ePlace paper equation (13), Am is nodeArea here. or see RePlAce code opt.cpp line 86, total_modu_area equals nodeAreaScaled here
    //??? macro area should *= target density when calculating Am in(13)? see RePlAce code opt.cpp line 86 But terminal area wasn't *= target density when calculating Aws??? implement as this for now

    ////////////////////////////////////////////////////////////////
    // calculate filler area
    ////////////////////////////////////////////////////////////////

    float totalFillerArea = 0;                                         // area of all fillers, how to calculate: see ePlace paper (13)
    totalFillerArea = whitespaceArea * targetDensity - nodeAreaScaled; //! see ePlace paper (13)

    int nodeCount = db->dbNodes.size();

    vector<float> nodeArea; // sort node according to area with this because we don't want to sort dbNodes
    nodeArea.resize(nodeCount);

    for (int i = 0; i < nodeCount; i++)
    {
        nodeArea[i] = db->dbNodes[i]->getArea();
        // use this one below if we want no problems:
        // nodeArea[i]=db->dbNodes[i]->calcArea();
    }

    sort(nodeArea.begin(), nodeArea.end()); //! sort by area

    float avg80TotalArea = 0;
    float avg80NodeArea = 0;
    int minIdx = (int)(0.10 * (float)nodeCount); //! for calculating average area of the middle 80% of all nodes(cells and macros)
    int maxIdx = (int)(0.90 * (float)nodeCount);

    for (int i = minIdx; i < maxIdx; i++)
    {
        avg80TotalArea += nodeArea[i];
    }

    avg80NodeArea = avg80TotalArea / ((float)(maxIdx - minIdx));

    float fillerArea = avg80NodeArea; //! use average area of the middle 90% of all nodes as filler area! see ePlace paper, filler insertion
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
        Module *curFiller = new Module(i + nodeCount, name, fillerWidth, fillerHeight, false, false);
        curFiller->isFiller = true; //!
        ePlaceFillers[i] = curFiller;

        db->setModuleLocation_2D_random(curFiller);
    }

    ePlaceNodesAndFillers = db->dbNodes;
    ePlaceNodesAndFillers.insert(ePlaceNodesAndFillers.end(), ePlaceFillers.begin(), ePlaceFillers.end()); //! fillers are stored after nodes in the vector
    // macro density scaling in density computation: RePlace bin.cpp line 1853
    // terminal(fixed macro) density scaling in density computation?: RePlace bin.cpp line 480

    // density scaling when calculating Aws?
}

void EPlacer_2D::binInitialization()
{
    segmentFaultCP("binInit");
    //! Bins are allocated on coreRegion! RePlAce code bin.cpp line 337 and bookshelfIO.cpp, also see ePlace paper
    ////////////////////////////////////////////////////////////////
    // calculate bin dimension and size
    ////////////////////////////////////////////////////////////////
    //! this code for bin dimension calculation is copied directly from RePlAce
    segmentFaultCP("binDim");
    int nodeCount = db->dbNodes.size();
    float nodeArea = (ePlaceStdCellArea + ePlaceMacroArea);

    float coreRegionWidth = db->coreRegion.getWidth();
    float coreRegionHeight = db->coreRegion.getHeight();
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

    cout << "Bin dimension: " << binDimension;

    binStep.x = float_div(coreRegionWidth, binDimension.x);
    binStep.y = float_div(coreRegionHeight, binDimension.y);

    ////////////////////////////////////////////////////////////////
    // add bins
    ////////////////////////////////////////////////////////////////
    segmentFaultCP("addBin");
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
            bins[i][j] = new Bin_2D();
            // cout<<"adding bin "<<i<<","<<j<<endl;
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
    segmentFaultCP("terminalDensity");
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

        cout << curTerminal->getLL_2D() << curTerminal->getUR_2D() << endl;
        cout << binStep << " " << db->coreRegion.ll << endl;
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
    segmentFaultCP("baseDensity");
    for (int i = 0; i < binDimension.x; i++)
    {
        for (int j = 0; j < binDimension.y; j++)
        {
            float curBinAvailableArea = 0; // overlap area between current bin and placement rows
            for (SiteRow curRow : db->dbSiteRows)
            {
                curBinAvailableArea += getOverlapArea_2D(bins[i][j]->ll, bins[i][j]->ur, curRow.getLL_2D(), curRow.getUR_2D());
            }
            debugOutput("Bin area", bins[i][j]->area);
            debugOutput("Available area", curBinAvailableArea);
            if (float_equal(bins[i][j]->area, curBinAvailableArea))
            {
                bins[i][j]->baseDensity = 0;
            }
            else
            {
                bins[i][j]->baseDensity = targetDensity * (bins[i][j]->area - curBinAvailableArea); //! follow RePlAce bin.cpp line 433
            }
        }
    }
}

void EPlacer_2D::gradientVectorInitialization()
{
    wirelengthGradient.resize(db->dbNodes.size()); // fillers has no wirelength gradient
    densityGradient.resize(ePlaceNodesAndFillers.size());
    totalGradient.resize(ePlaceNodesAndFillers.size());
}

void EPlacer_2D::densityOverflowUpdate()
{
    float globalOverflowArea = 0;
    float nodeAreaScaled = ePlaceStdCellArea + ePlaceMacroArea * targetDensity;
    float invertedBinArea = 1.0 / (binStep.x * binStep.y); // 1/bin area
    for (int i = 0; i < binDimension.x; i++)
    {
        for (int j = 0; j < binDimension.y; j++)
        {
            // nodeDensity+terminalDensity+baseDensity because filler density are not included
            globalOverflowArea += max(float(0.0), (bins[i][j]->nodeDensity + bins[i][j]->terminalDensity + bins[i][j]->baseDensity) * invertedBinArea - targetDensity) * bins[i][j]->area;
        }
    }
    globalDensityOverflow = globalOverflowArea / nodeAreaScaled; // see RePlAce code bin.cpp line 1183, opt.cpp line 86. And nodeAreaScaled in fillerInitialization() in this file
}

void EPlacer_2D::wirelengthGradientUpdate()
{
    ////////////////////////////////////////////////////////////////
    //! Step1: calculate gamma, see ePlace paper equation 38, RePlace code wlen.cpp line 141
    ////////////////////////////////////////////////////////////////
    // first, calculate tau(density overflow)
    densityOverflowUpdate();

    //! now calculate gamma with the updated tau, here we actually calculate 1/gamma for furthurer calculation
    VECTOR_2D baseWirelengthCoef;
    baseWirelengthCoef.x = baseWirelengthCoef.y = 0.125;     // 0.125=1/8.0, 8.0:see ePlace paper equation 38. Notice that baseWirelngthCoef
                                                             // is wcof00_org in RePlace code wlen.cpp,
                                                             // and is tuned according to input benchmark in RePlAce main.cpp
    baseWirelengthCoef.x = baseWirelengthCoef.x / binStep.x; // binStep: wb in ePlace paper equation 38, 1/8/wb=1/8.0wb
    baseWirelengthCoef.y = baseWirelengthCoef.y / binStep.y;

    if (globalDensityOverflow > 1.0)
    {
        baseWirelengthCoef.x *= 0.1;
        baseWirelengthCoef.y *= 0.1;
    }
    else if (globalDensityOverflow < 0.1)
    {
        baseWirelengthCoef.x *= 10.0;
        baseWirelengthCoef.y *= 10.0;
    }
    else
    {
        float temp;
        temp = 1.0 / pow(10.0, (globalDensityOverflow - 0.1) * 20 / 9.0 - 1.0); //! see eplace paper equation 38
        baseWirelengthCoef.x *= temp;                                           //!(1/8.0wb)*(1/10^(k*tau+b)), where k=20/9 and b=-11/9
        baseWirelengthCoef.y *= temp;
    }

    invertedGamma = baseWirelengthCoef;

    ////////////////////////////////////////////////////////////////
    //! Step2: calculate wirelength density for each nodes (not filler nodes)
    ////////////////////////////////////////////////////////////////
    // When using weighted-average wirelength model we would need X/Y/Z max and min in a net,
    // so update X/Y/Z max and min in all nets first, see ntuplace3D paper page 6: Stable Weighted-Average Wirelength Model
    // Also the numerators and denominators are pre-calculated for all nets for further use
    double HPWL = db->calcNetBoundPins();
    double WA = db->calcWA_Wirelength_2D(invertedGamma);

    int index = 0;
    for (Module *curNode : db->dbNodes) // use ePlaceNodesAndFillers?
    {
        assert(curNode->idx == index);
        wirelengthGradient[index].SetZero(); //! clear before updating
        for (Pin *curPin : curNode->modulePins)
        {
            VECTOR_2D gradient;
            gradient = curPin->net->getWirelengthGradientWA_2D(invertedGamma, curPin);
            wirelengthGradient[index].x += gradient.x;
            wirelengthGradient[index].y += gradient.y;
            // get the wirelength gradient of this pin
        }
        index++;
    }
}

void EPlacer_2D::densityGradientUpdate()
{
    ////////////////////////////////////////////////////////////////
    //! Step1: obtain electric field(e) through FFT
    ////////////////////////////////////////////////////////////////
    replace::FFT_2D fft(binDimension.x, binDimension.y, binStep.x, binStep.y);
    float invertedBinArea = 1.0 / (binStep.x * binStep.y);
    for (int i = 0; i < binDimension.x; i++)
    {
        for (int j = 0; j < binDimension.y; j++)
        {
            float eDensity = bins[i][j]->nodeDensity + bins[i][j]->baseDensity + bins[i][j]->fillerDensity + bins[i][j]->terminalDensity; // consider filler area(density) here
            eDensity *= invertedBinArea;
            fft.updateDensity(i, j, eDensity);
        }
    }
    fft.doFFT();
    for (int i = 0; i < binDimension.x; i++)
    {
        for (int j = 0; j < binDimension.y; j++)
        {
            auto eForcePair = fft.getElectroForce(i, j);
            bins[i][j]->E.x = eForcePair.first;
            bins[i][j]->E.y = eForcePair.second;
            // std::cout<<"bin eforce x and y "<<Bins[i][j].Eforce.x<<" "<<Bins[i][j].Eforce.y<<std::endl;
            float electroPhi = fft.getElectroPhi(i, j);
            bins[i][j]->phi = electroPhi;
        }
        // sumPhi_ += electroPhi*static_cast<float>(bin->nonPlaceArea()+bin->instPlacedArea()+bin->fillerArea());
    }

    ////////////////////////////////////////////////////////////////
    //! Step2: calculate density(potential) gradient, see ePlace paper equation 16, RePlace charge.cpp line 451
    ////////////////////////////////////////////////////////////////
    int nodeCount = db->dbNodes.size();
    int index = 0;
    for (Module *curNode : ePlaceNodesAndFillers)
    {
        assert(index == curNode->idx);
        //! clear before updating
        densityGradient[index].SetZero();

        VECTOR_2D localSmoothLengthScale; // see ePlace paper page 15 or RePlace opt.cpp line 1460
        localSmoothLengthScale.x = 1;
        localSmoothLengthScale.y = 1;

        CRect rectForCurNode;
        rectForCurNode.ll = curNode->getLL_2D();
        rectForCurNode.ur = curNode->getUR_2D();

        //! beware: local smooth on x and y dimension, also needed here!
        //! binStart and binEnd should be calculated with inflated cell width and height, see replace charge.cpp line 408
        POS_3D cellCenter = curNode->getCenter();

        if (!curNode->isMacro)
        {
            if (float_less(curNode->getWidth(), binStep.x))
            {
                localSmoothLengthScale.x = curNode->getWidth() / binStep.x;
                rectForCurNode.ll.x = cellCenter.x - 0.5 * binStep.x;
                rectForCurNode.ur.x = cellCenter.x + 0.5 * binStep.x;
            }
            if (float_less(curNode->getHeight(), binStep.y))
            {
                localSmoothLengthScale.y = curNode->getHeight() / binStep.y;
                rectForCurNode.ll.y = cellCenter.y - 0.5 * binStep.y;
                rectForCurNode.ur.y = cellCenter.y + 0.5 * binStep.y;
            }
        }

        VECTOR_2D_INT binStartIdx; // binStartIdx: index the index of the first bin that has overlap with a cell on X/Y direction
        VECTOR_2D_INT binEndIdx;
        binStartIdx.x = INT_DOWN((rectForCurNode.ll.x - db->coreRegion.ll.x) / binStep.x);
        binEndIdx.x = INT_DOWN((rectForCurNode.ur.x - db->coreRegion.ll.x) / binStep.x);

        binStartIdx.y = INT_DOWN((rectForCurNode.ll.y - db->coreRegion.ll.y) / binStep.y);
        binEndIdx.y = INT_DOWN((rectForCurNode.ur.y - db->coreRegion.ll.y) / binStep.y);

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
                float overlapArea = localSmoothLengthScale.x * localSmoothLengthScale.y * getOverlapArea_2D(bins[i][j]->ll, bins[i][j]->ur, rectForCurNode.ll, rectForCurNode.ur);

                densityGradient[index].x += overlapArea * bins[i][j]->E.x;
                densityGradient[index].y += overlapArea * bins[i][j]->E.y;
            }
        }

        index++;
    }
}

void EPlacer_2D::totalGradientUpdate(float lambda)
{
    int index = 0;
    for (Module *curNodeOrFiller : ePlaceNodesAndFillers)
    {
        totalGradient[index].SetZero();
        assert(index == curNodeOrFiller->idx);
        // calculate -gradient here
        if (curNodeOrFiller->isFiller)
        {
            // wirelength gradient of fillers should == 0
            totalGradient[index].x = lambda * densityGradient[index].x;
            totalGradient[index].y = lambda * densityGradient[index].y;
        }
        else
        {
            totalGradient[index].x = lambda * densityGradient[index].x - wirelengthGradient[index].x;
            totalGradient[index].y = lambda * densityGradient[index].y - wirelengthGradient[index].y;
        }

        index++;
    }
}

void EPlacer_2D::binNodeDensityUpdate()
{
    //!!!! clear nodeDensity for each bin before update!
    for (int i = 0; i < binDimension.x; i++)
    {
        for (int j = 0; j < binDimension.y; j++)
        {
            bins[i][j]->nodeDensity = 0;
            bins[i][j]->fillerDensity = 0;
        }
    }

    segmentFaultCP("nodeDensity");
    for (Module *curNode : ePlaceNodesAndFillers) // ePlaceNodes: nodes and filler nodes
    {
        bool localSmooth = false;
        bool macroDensityScaling = false; // density scaling, see ePlace paper

        VECTOR_2D localSmoothLengthScale; // see ePlace paper page 15 or RePlace opt.cpp line 1460
        localSmoothLengthScale.x = 1;
        localSmoothLengthScale.y = 1;

        CRect rectForCurNode;
        rectForCurNode.ll = curNode->getLL_2D();
        rectForCurNode.ur = curNode->getUR_2D();

        if (!curNode->isMacro)
        {
            //! beware: local smooth on x and y dimension!
            //! binStart and binEnd should be calculated with inflated cell width and height, see replace bin.cpp line 1807
            POS_3D cellCenter = curNode->getCenter();

            if (float_less(curNode->getWidth(), binStep.x))
            {
                localSmoothLengthScale.x = curNode->getWidth() / binStep.x;
                rectForCurNode.ll.x = cellCenter.x - 0.5 * binStep.x;
                rectForCurNode.ur.x = cellCenter.x + 0.5 * binStep.x;
            }
            if (float_less(curNode->getHeight(), binStep.y))
            {
                localSmoothLengthScale.y = curNode->getHeight() / binStep.y;
                rectForCurNode.ll.y = cellCenter.y - 0.5 * binStep.y;
                rectForCurNode.ur.y = cellCenter.y + 0.5 * binStep.y;
            }
        }
        else
        {
            macroDensityScaling = true;
        }
        VECTOR_2D_INT binStartIdx; // binStartIdx: index the index of the first bin that has overlap with a cell on X/Y direction
        VECTOR_2D_INT binEndIdx;
        binStartIdx.x = INT_DOWN((rectForCurNode.ll.x - db->coreRegion.ll.x) / binStep.x);
        binEndIdx.x = INT_DOWN((rectForCurNode.ur.x - db->coreRegion.ll.x) / binStep.x);

        binStartIdx.y = INT_DOWN((rectForCurNode.ll.y - db->coreRegion.ll.y) / binStep.y);
        binEndIdx.y = INT_DOWN((rectForCurNode.ur.y - db->coreRegion.ll.y) / binStep.y);

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

                float overlapArea = getOverlapArea_2D(bins[i][j]->ll, bins[i][j]->ur, rectForCurNode.ll, rectForCurNode.ur);
                if (macroDensityScaling)
                {
                    bins[i][j]->nodeDensity += targetDensity * overlapArea;
                }
                else
                {
                    if (curNode->isFiller)
                    {
                        //? does filler need localSmooth?
                        bins[i][j]->fillerDensity += localSmoothLengthScale.x * localSmoothLengthScale.y * overlapArea;
                    }
                    else
                    {
                        bins[i][j]->nodeDensity += localSmoothLengthScale.x * localSmoothLengthScale.y * overlapArea;
                    }
                }
            }
        }
    }
}
