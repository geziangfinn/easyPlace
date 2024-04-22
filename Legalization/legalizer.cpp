#include "legalizer.h"

void AbacusLegalizer::legalization()
{
    initialization();
    int cellCount = dbCells.size();
    cout << cellCount << endl;
    for (Module *curCell : dbCells)
    {
        // cout<<curCell->name<<endl;
        double cost = DOUBLE_MAX; // current minimum cost
        //! assume subrows are sorted by y coordinate(non decreasing)
        // 1. find closest row
        int subrowCount = subrows.size();
        // cout<<subrowCount<<endl;
        int bestRowIndex = -1;
        int closestRowIndex = 0;
        for (AbacusRow curRow : subrows)
        {
            if (float_lessorequal(fabs(curRow.bottom - curCell->getLL_2D().y), 0.5 * curRow.height)) //? double check here
            {
                break;
            }
            closestRowIndex++;
        }
        // cout <<closestRowIndex<<endl;
        //! 2. search in two directions(higher rows and lower rows) for the best row
        //? optimize here?
        // first search in higher rows
        for (int i = closestRowIndex; i < subrowCount; i++)
        {
            // cout<<"higher "<<i<<endl;
            if (float_greater(subrows[i].width + curCell->getWidth(), subrows[i].end.x - subrows[i].start.x))
            {
                continue;
            }
            double costLowerBound = fabs(curCell->getLL_2D().y - subrows[i].bottom); // cost for moving cell in y direction only, //?quadratic or not?
            // determine cost to move a cell to this row!
            if (float_greaterorequal(costLowerBound, cost))
            {
                break;
            }
            double newCost = placeRow(curCell, i, ABACUS_TRIAL);
            if (float_lessorequal(newCost, cost))
            {
                bestRowIndex = i;
                cost = newCost;
            }
        }
        // then search in lower rows
        for (int i = closestRowIndex - 1; i >= 0; i--)
        {
            // cout<<"lower "<<i<<endl;
            if (i == 0)
            {
                break;
            }
            if (float_greater(subrows[i].width + curCell->getWidth(), subrows[i].end.x - subrows[i].start.x))
            {
                continue;
            }
            double costLowerBound = fabs(curCell->getLL_2D().y - subrows[i].bottom); // cost for moving cell in y direction only
            if (float_greaterorequal(costLowerBound, cost))
            {
                break;
            }
            double newCost = placeRow(curCell, i, ABACUS_TRIAL);
            if (float_lessorequal(newCost, cost))
            {
                bestRowIndex = i;
                cost = newCost;
            }
        }
        assert(bestRowIndex != -1);
        // 3. place cell to the best row
        placeRow(curCell, bestRowIndex, ABACUS_FINAL);
    }

    //! 4. write cell locations! In placeRow we only determine cluster's best position, without writing cell location

    for (AbacusRow curSubrow : subrows)
    {
        for (AbacusCellCluster curCluster : curSubrow.clusters)
        {
            int x = curCluster.x;
            for (Module *curCell : curCluster.cells)
            {
                placeDB->setModuleLocation_2D(curCell, x, curSubrow.bottom);
                x += curCell->getWidth();
                assert(x <= curSubrow.end.x);
            }
        }
    }
}

void AbacusLegalizer::initialization()
{
    // segmentFaultCP("cp2");
    initializeCells();
    // segmentFaultCP("cp3");
    initializeObstacles();
    segmentFaultCP("cp4");
    initializeSubrows();
}

void AbacusLegalizer::initializeCells()
{
    for (Module *curNode : placeDB->dbNodes)
    {
        assert(curNode);
        if (!curNode->isMacro)
        {
            dbCells.push_back(curNode);
        }
    }
    // segmentFaultCP("cp5");
    sort(dbCells.begin(), dbCells.end(), [=](Module *a, Module *b)
         { return float_less(a->getLL_2D().x, b->getLL_2D().x); });
    // segmentFaultCP("cp6");
    //? what's the potential bug when one cell is completely inside another? Should think about it.
}

void AbacusLegalizer::initializeObstacles()
{
    //! ignore terminals that are outside the core region
    //! overlap between macros should be eliminated first!
    for (Module *curTerminal : placeDB->dbTerminals)
    {
        if (float_greater(placeDB->coreRegion.ur.y, curTerminal->getLL_2D().y) && float_less(placeDB->coreRegion.ll.y, curTerminal->getUR_2D().y)) // overlap in y direction
        {
            if (float_greater(placeDB->coreRegion.ur.x, curTerminal->getLL_2D().x) && float_less(placeDB->coreRegion.ll.x, curTerminal->getUR_2D().x)) // overlap in x direction
            {
                Obstacle newObstacle;
                newObstacle.ll = curTerminal->getLL_2D();
                newObstacle.ur = curTerminal->getUR_2D();
                obstacles.push_back(newObstacle);
            }
        }
    }

    for (Module *curNode : placeDB->dbNodes)
    {
        if (curNode->isMacro)
        {
            Obstacle newObstacle;
            newObstacle.ll = curNode->getLL_2D();
            newObstacle.ur = curNode->getUR_2D();
            obstacles.push_back(newObstacle);
        }
    }
    //! sort obstacles by x coordinate first
    sort(obstacles.begin(), obstacles.end(), [=](Obstacle a, Obstacle b)
         {
            if (!float_equal(a.ll.x , b.ll.x))
            {
                return float_less(a.ll.x ,b.ll.x);
                
            }
            else if (!float_equal(a.ll.y , b.ll.y))
            {
                return float_less(a.ll.y ,b.ll.y);
            }
            else
            {
                // terminals/macros overlap!!
                cerr<<"TERMINALS/MACROS OVERLAP!\n";
                exit(0);
            } });
}

void AbacusLegalizer::initializeSubrows()
{
    double siteStep = placeDB->dbSiteRows.front().step; //! assume step for all rows are identical!

    for (SiteRow curRow : placeDB->dbSiteRows) // generate subrows from dbRows
    {
        float currentX = curRow.start.x;
        float currentRowBottom = curRow.start.y; // start.y == curRow.bottom
        float currentRowTop = curRow.bottom + curRow.height;

        assert(curRow.bottom == curRow.start.y);
        for (Obstacle curObstacle : obstacles)
        {
            // assume all obstacles are inside the core region
            // ! and are ordered from left to right!

            // assuming row and obstacles are like this:
            // O for obstacle and --- for row
            // ---------------------
            //  OOO  OOOO  OO    OOOO  OOOOO
            //! curRow.end.x might be to the left of an obstacle's right boundary! although in almost all cases, curRow.end.x is to the right of all obstacles(only consider obstacles in the core region, see initializeObstacles)

            if (float_greaterorequal(curRow.start.x, curObstacle.ur.x)) // not overlap in x direction, following obstacles might overlap
            {
                continue;
            }
            if (float_greaterorequal(curObstacle.ll.x, curRow.end.x)) // not overlap in x direction, following obstacles will not overlap as well
            {
                break;
            }

            if (float_greater(currentRowTop, curObstacle.ll.y) && float_less(currentRowBottom, curObstacle.ur.y)) // overlap in y direction
            {
                // end.x might be less than start.x when:
                //    ------
                //   OOOOO
                AbacusRow newRow;
                newRow.start.x = currentX;
                newRow.start.y = currentRowBottom;

                newRow.end.x = curObstacle.ll.x; //!
                newRow.end.y = currentRowBottom;

                newRow.bottom = currentRowBottom;
                newRow.height = curRow.height;
                newRow.step = siteStep;

                currentX = curObstacle.ur.x;

                subrows.push_back(newRow);
            }
            if (float_greaterorequal(currentX, curRow.end.x))
            {
                break;
            }
        }
        // add the last subrow when row and obstacles are like this:
        // O for obstacle and --- for row
        // ---------------------
        //  OOO  OOOO  OOOO
        // or like this:
        // -------
        //           OOO
        if (float_less(currentX, curRow.end.x))
        {
            AbacusRow newRow;
            newRow.start.x = currentX;
            newRow.start.y = currentRowBottom;

            newRow.end.x = curRow.end.x; //!
            newRow.end.y = currentRowBottom;

            newRow.bottom = currentRowBottom;
            newRow.height = curRow.height;
            newRow.step = siteStep;

            subrows.push_back(newRow);
        }
    }

    for (auto iter = subrows.begin(); iter != subrows.end();)
    {
        //! align subrows to sites after generating subrows, see ntuplace: FixFreeSiteBySiteStep(). Here we need to update the end and start of a site row, so end.x-start.x is an positive integer multiple of site step(site width)
        //? should start.x and end.x be integers too??? check ntuplace
        double subrowWidth = iter->end.x - iter->start.x;
        // subRowWidth might be less than 0 when:
        //    ------
        //   OOOOO
        if (float_less(subrowWidth, siteStep)) // assume iter->step > 0!
        {
            iter = subrows.erase(iter);
        }
        else
        {
            double newLeft = ceil((iter->start.x - placeDB->coreRegion.ll.x) / siteStep) * siteStep + placeDB->coreRegion.ll.x;
            double newRight = floor((iter->end.x - placeDB->coreRegion.ll.x) / siteStep) * siteStep + placeDB->coreRegion.ll.x;
            double newSubrowWidth = newRight - newLeft;
            assert(newSubrowWidth >= siteStep);
            if (float_greater(newSubrowWidth, 0.0))
            {
                iter->start.x = newLeft;
                iter->end.x = newRight;
            }
            else if (float_less(newSubrowWidth, 0.0))
            {
                // newSubrowWidth should >= 0.0
                cerr << "sub row new width < 0 when it should not\n";
                exit(0);
            }
            iter++;
        }
    }

    // sort subrows by y coordinate!
    sort(subrows.begin(), subrows.end(), [=](AbacusRow a, AbacusRow b)
         {
         if (!float_equal(a.bottom , b.bottom))
            {
                return float_less(a.bottom , b.bottom);
            }
            else if (!float_equal(a.start.x , b.start.x))
            {
                return float_less(a.start.x ,b.start.x);
            }
            else
            {
                // terminals/macros overlap!!
                cerr<<"SUBROWS OVERLAP!\n";
                exit(0);
            } });
}

double AbacusLegalizer::placeRow(Module *cell, int bestRow, bool trial)
{
    AbacusRow *tempRowPointer;
    AbacusRow tempRow;
    if (trial == ABACUS_TRIAL)
    {
        // cout<<"try "<< cell->name<<"at"<< bestRow<<endl;
        tempRow = subrows[bestRow];
        tempRowPointer = &tempRow;
        // for trial place, would not actually modify subrows
    }
    else if (trial == ABACUS_FINAL)
    {
        tempRowPointer = &subrows[bestRow];
        tempRowPointer->width += cell->getWidth();
        // final place
    }
    else
    {
        cerr << "Not trial nor final\n";
        exit(0);
    }
    segmentFaultCP("K1");
    if (tempRowPointer->clusters.empty() || float_lessorequal(tempRowPointer->clusters.back().x + tempRowPointer->clusters.back().w, cell->getLL_2D().x))
    {
        AbacusCellCluster newCluster;
        newCluster.index = tempRowPointer->clusters.size();                                                //! 0 for the first cluster
        newCluster.x = int((int(cell->getLL_2D().x + 0.5) / tempRowPointer->step) * tempRowPointer->step); // !! need to make sure that position of clusters align with sites!!!!
        //? site step: double instead of int, potential precision problems? although step is almost always 1 (never seen any case in which step != 1 so far)
        //! boundary check
        if (float_less(newCluster.x, tempRowPointer->start.x))
        {
            newCluster.x = tempRowPointer->start.x;
        }
        if (float_greater(newCluster.x + cell->getWidth(), tempRowPointer->end.x))
        {
            newCluster.x = tempRowPointer->end.x - cell->getWidth();
        }

        tempRowPointer->clusters.push_back(newCluster);
        tempRowPointer->addCell(newCluster.index, cell); // addCell: actually always add cell to the last cluster, that is, cluster.back()
        tempRowPointer->collapse(newCluster.index);      //! this is missing in the abacus paper!
    }
    else
    {
        // cout<<cell->name<<"at here\n";
        tempRowPointer->addCell(tempRowPointer->clusters.back().index, cell); // addCell: actually always add cell to the last cluster, that is, cluster.back()
        tempRowPointer->collapse(tempRowPointer->clusters.back().index);
    }
    //! operate with tempRowPointer
    // todo:calculate cell location and return cost!
    // here, cell must be in the last cluster
    float x = tempRowPointer->clusters.back().x; // !! need to make sure that position of clusters align with sites!!!! Align in collapse and when creating new clusters
    for (Module *curCell : tempRowPointer->clusters.back().cells)
    {
        if (curCell == cell)
        {
            double cost = sqrt((curCell->getLL_2D().x - x) * (curCell->getLL_2D().x - x) + (curCell->getLL_2D().y - tempRowPointer->bottom) * (curCell->getLL_2D().y - tempRowPointer->bottom));
            return cost;
            //? sqrt or not?
        }
        else
        {
            x += curCell->getWidth();
        }
    }
    cerr << "cell not found in cluster??\n";
    exit(0);
}

void AbacusRow::addCell(int clusterIndex, Module *cell)
{
    assert(clusterIndex < clusters.size());
    AbacusCellCluster &c = clusters[clusterIndex];
    c.cells.push_back(cell);
    c.e++;
    c.q += cell->getLL_2D().x - c.w;
    c.w += cell->getWidth();
}

void AbacusRow::addCluster(int predecessorIndex, int clusterIndex)
{
    // cout<<predecessorIndex<<" fafa "<<clusterIndex<<endl;
    // cout<<clusters.size()<<endl;
    AbacusCellCluster &cPrime = clusters[predecessorIndex];
    AbacusCellCluster &c = clusters[clusterIndex];
    cPrime.cells.insert(cPrime.cells.end(), c.cells.begin(), c.cells.end());
    cPrime.e += c.e;
    cPrime.q += c.q - c.e * cPrime.w;
    cPrime.w += c.w;
}

void AbacusRow::collapse(int clusterIndex)
{
    assert(clusterIndex < clusters.size());
    AbacusCellCluster &c = clusters[clusterIndex];
    // place c
    c.x = (int)(1.0 * c.q / c.e / step) * step;
    if (float_less(c.x, start.x))
    {
        c.x = start.x;
    }
    if (float_greater(c.x + c.w, end.x))
    {
        c.x = end.x - c.w;
    }
    int predecessorIndex = clusterIndex - 1;
    // cout<<"pindex here is: "<<predecessorIndex<<endl;
    if (predecessorIndex >= 0)
    {
        AbacusCellCluster cPrime = clusters[predecessorIndex];
        if (cPrime.x + cPrime.w > c.x) // actually comparing int here
        {
            addCluster(predecessorIndex, clusterIndex);
            assert(clusters[clusterIndex].index == clusterIndex); //!
            clusters.erase(clusters.begin() + clusterIndex);

            collapse(predecessorIndex);
        }
    }
}

void SAMacroLegalizer::legalization()
{
    initialization();
    for (int j = 0; j < jLimit; j++) // outter loop(mLG iteration, see ePlace-MS paper)
    {
        //! 1. initialize parameters(t and r), this part of code is basically copied from RePlAce, macro.cpp

        float sa_init_neg_rate =
            0.03 * pow(beta, (float)j);
        // 3% cost increase will be
        // accepted in 50% probability

        float sa_last_neg_rate = 0.0001 * pow((float)j, (float)j);
        // 0.01% cost increase will be
        // accepted in 50% probability

        float sa_init_t = sa_init_neg_rate / log(2.0); // based on the equation that
                                                       // exp(-1.0*sa_init_neg_rate/
                                                       // sa_init_t) = 0.5

        SAtemperature = sa_init_t;
        // 0.1 ; // 400 ; // from Howard's ICCCAS 13 paper

        SAtemperatureCoef = pow((sa_last_neg_rate / sa_init_neg_rate),
                                1.0 / (float)kLimit); // make sure sa_t(last) equals
                                                      // its expected value
        VECTOR_2D sa_n;
        VECTOR_2D sa_ncof;
        VECTOR_2D max_sa_r;
        sa_n.x = sa_n.y = sqrt(float(dbMacros.size()));

        sa_ncof.x = 0.05 * pow(beta, (float)j);
        sa_ncof.y = 0.05 * pow(beta, (float)j);

        max_sa_r.x = placeDB->coreRegion.getWidth() / sa_n.x * sa_ncof.x;
        max_sa_r.y = placeDB->coreRegion.getHeight() / sa_n.y * sa_ncof.y;

        r.x = max_sa_r.x;
        r.y = max_sa_r.y;

        sa_r_stp.x = (max_sa_r.x - 1.0) / (float)kLimit;
        sa_r_stp.y = (max_sa_r.y - 1.0) / (float)kLimit;
        //! 2. SA iterations
        SAMacroLegalization();
        //! 3. update parameters
        miuO *= beta;
        // no update for miuD

        if (overlapFree)
        {
            break;
        }
    }
}

void SAMacroLegalizer::SAMacroLegalization()
{
    int innerLoopCount = dbMacros.size();
    for (int k = 0; k < kLimit; k++) // inner loop(SA iteration), see ePlace-MS paper
    {
        for (int i = 0; i < innerLoopCount; i++) // there is one more for in RePlAce code, why???
        {
            SAperturb();
            if (overlapFree)
            {
                break;
            }
        }
        // update temperature and r
        SAtemperature *= SAtemperatureCoef;
        r.x -= sa_r_stp.x;
        r.y -= sa_r_stp.y;
        if (overlapFree)
        {
            break;
        }
    }
}

void SAMacroLegalizer::initialization()
{
    initializeMacros();
    initializeBins();
    initializeCost();
    initializeSAparams();
}

void SAMacroLegalizer::initializeMacros()
{

    for (Module *curNode : placeDB->dbNodes)
    {
        assert(curNode);
        if (curNode->isMacro)
        {
            dbMacros.push_back(curNode);
            totalMacroArea += std::round(curNode->getArea()); // followed RePlAce code
            //! Discretization of macro coordinates, followed RePlAce code
            POS_2D legalLL;
            legalLL.x = (int)(curNode->getLL_2D().x + 0.5); // set macro center to integer here, but why according to row height?
            legalLL.y = ((int)((curNode->getLL_2D().y + 0.5 * placeDB->commonRowHeight - placeDB->coreRegion.ll.y /* 1.0 * ROW_Y0 */) / (double)placeDB->commonRowHeight)) * placeDB->commonRowHeight + (int)(placeDB->coreRegion.ll.y + 0.5) /* ROW_Y0 */;
            placeDB->setModuleLocation_2D(curNode, legalLL.x, legalLL.y);
        }
        else
        {
            totalCellArea += std::round(curNode->getArea());
        }
    }
}

void SAMacroLegalizer::initializeBins()
{
    segmentFaultCP("mLGbinInit");
    //! Bins are allocated on coreRegion! RePlAce code bin.cpp line 337 and bookshelfIO.cpp, also see ePlace paper
    ////////////////////////////////////////////////////////////////
    // calculate bin dimension and size
    ////////////////////////////////////////////////////////////////
    //! this code for bin dimension calculation is copied directly from RePlAce
    segmentFaultCP("mLGbinDim");

    int nodeCount = placeDB->dbNodes.size();
    float nodeArea = (totalCellArea + totalMacroArea);

    float coreRegionWidth = placeDB->coreRegion.getWidth();
    float coreRegionHeight = placeDB->coreRegion.getHeight();
    float coreRegionArea = float_mul(coreRegionWidth, coreRegionHeight);

    float averageNodeArea = 1.0 * nodeArea / nodeCount;
    float idealBinArea = averageNodeArea / targetDensity;
    // float idealBinArea = averageNodeArea / 1.0;

    int idealBinCount = INT_CONVERT(coreRegionArea / idealBinArea);

    bool isUpdate = false;
    // bin dimension upper bound: 1024 rather than 2048
    // for (int i = 1; i <= 10; i++)
    // { //! 4*4,8*8,16*16,32*32..., 2048*2048
    for (int i = 1; i < 10; i++)
    { //! 4*4,8*8,16*16,32*32..., 1024*1024
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

    cout << BLUE << "Macro legalizer bin dimension: " << binDimension << "\ncoreRegion width: " << coreRegionWidth << "\ncoreRegion height: " << coreRegionHeight << RESET << endl;

    binStep.x = float_div(coreRegionWidth, binDimension.x);
    binStep.y = float_div(coreRegionHeight, binDimension.y);

    cout << BLUE << "Macro legalizer bin step: " << binStep << RESET << endl;
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

    double addBinTime;
    double terminalDensityTime;
    double baseDensityTime;

    time_start(&addBinTime);

    for (int i = 0; i < binDimension.x; i++)
    {
        bins[i].resize(binDimension.y);
        for (int j = 0; j < binDimension.y; j++)
        {
            bins[i][j] = new SAMacroLegalizationBin_2D();
            // cout<<"adding bin "<<i<<","<<j<<endl;
            //!!! +db->coreRegion.ll.x to get coordinates!!
            bins[i][j]->ll.x = i * binStep.x + placeDB->coreRegion.ll.x;
            bins[i][j]->ll.y = j * binStep.y + placeDB->coreRegion.ll.y;

            bins[i][j]->width = binStep.x;
            bins[i][j]->height = binStep.y;

            bins[i][j]->ur.x = bins[i][j]->ll.x + bins[i][j]->width;
            bins[i][j]->ur.y = bins[i][j]->ll.y + bins[i][j]->height;

            bins[i][j]->area = binStep.x * binStep.y;

            bins[i][j]->center.x = bins[i][j]->ll.x + (float)0.5 * bins[i][j]->width;
            bins[i][j]->center.y = bins[i][j]->ll.y + (float)0.5 * bins[i][j]->height;
        }
    }

    time_end(&addBinTime);
    cout << "mLG bin add time: " << addBinTime << endl;
    ////////////////////////////////////////////////////////////////
    // terminal density calculation, calculate here because they are terminals and only needed to be considered once
    ////////////////////////////////////////////////////////////////
    segmentFaultCP("mLGterminalArea");
    VECTOR_2D_INT binStartIdx;
    VECTOR_2D_INT binEndIdx;

    time_start(&terminalDensityTime);

    for (Module *curTerminal : placeDB->dbTerminals)
    {
        //! only consider terminals inside the coreRegion
        //! assume no terminal would have a part inside the coreRegion and a part outside the coreRegion
        if (curTerminal->getLL_2D().x < placeDB->coreRegion.ll.x || curTerminal->getLL_2D().x + curTerminal->getWidth() > placeDB->coreRegion.ur.x)
        {
            continue;
        }
        if (curTerminal->getLL_2D().y < placeDB->coreRegion.ll.y || curTerminal->getLL_2D().y + curTerminal->getHeight() > placeDB->coreRegion.ur.y)
        {
            continue;
        }

        binStartIdx.x = INT_DOWN((curTerminal->getLL_2D().x - placeDB->coreRegion.ll.x) / binStep.x);
        binEndIdx.x = INT_DOWN((curTerminal->getUR_2D().x - placeDB->coreRegion.ll.x) / binStep.x);

        binStartIdx.y = INT_DOWN((curTerminal->getLL_2D().y - placeDB->coreRegion.ll.y) / binStep.y);
        binEndIdx.y = INT_DOWN((curTerminal->getUR_2D().y - placeDB->coreRegion.ll.y) / binStep.y);

        // cout << curTerminal->getLL_2D() << curTerminal->getUR_2D() << endl;
        // cout << binStep << " " << db->coreRegion.ll << endl;
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
                bins[i][j]->terminalArea += targetDensity * getOverlapArea_2D(bins[i][j]->ll, bins[i][j]->ur, curTerminal->getLL_2D(), curTerminal->getUR_2D());
            }
        }
    }

    time_end(&terminalDensityTime);
    cout << "Macro legalizer terminal density time: " << terminalDensityTime << endl;
    ////////////////////////////////////////////////////////////////
    // base density calculation, also only needed to be considered once
    ////////////////////////////////////////////////////////////////
    segmentFaultCP("mLGbaseArea");

    time_start(&baseDensityTime);

    for (int i = 0; i < binDimension.x; i++)
    {
        for (int j = 0; j < binDimension.y; j++)
        {
            float curBinAvailableArea = 0; // overlap area between current bin and placement rows
            for (SiteRow curRow : placeDB->dbSiteRows)
            {
                curBinAvailableArea += getOverlapArea_2D(bins[i][j]->ll, bins[i][j]->ur, curRow.getLL_2D(), curRow.getUR_2D());
            }
            // debugOutput("Bin area", bins[i][j]->area);
            // debugOutput("Available area", curBinAvailableArea);
            if (float_equal(bins[i][j]->area, curBinAvailableArea))
            {
                bins[i][j]->baseArea = 0;
            }
            else
            {
                bins[i][j]->baseArea = targetDensity * (bins[i][j]->area - curBinAvailableArea); //! follow RePlAce bin.cpp line 433
            }
        }
    }
    time_end(&baseDensityTime);
    cout << "Base density time: " << baseDensityTime << endl;

    ////////////////////////////////////////////////////////////////
    // cell density calculation, calculate here because cells are fixed during macro legalization and only needed to be considered once
    ////////////////////////////////////////////////////////////////
    segmentFaultCP("cellArea");
    for (Module *curNode : placeDB->dbNodes) // ePlaceNodes: nodes and filler nodes
    {
        CRect rectForCurNode;
        rectForCurNode.ll = curNode->getLL_2D();
        rectForCurNode.ur = curNode->getUR_2D();

        VECTOR_2D_INT binStartIdx; // binStartIdx: index the index of the first bin that has overlap with a cell on X/Y direction
        VECTOR_2D_INT binEndIdx;
        binStartIdx.x = INT_DOWN((rectForCurNode.ll.x - placeDB->coreRegion.ll.x) / binStep.x);
        binEndIdx.x = INT_DOWN((rectForCurNode.ur.x - placeDB->coreRegion.ll.x) / binStep.x);

        binStartIdx.y = INT_DOWN((rectForCurNode.ll.y - placeDB->coreRegion.ll.y) / binStep.y);
        binEndIdx.y = INT_DOWN((rectForCurNode.ur.y - placeDB->coreRegion.ll.y) / binStep.y);

        if (!(binStartIdx.x >= 0))
        {
            cout << "Module pos: " << rectForCurNode.ll << " " << placeDB->coreRegion.ll << endl;
        }
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

                float overlapArea = getOverlapArea_2D(bins[i][j]->ll, bins[i][j]->ur, rectForCurNode.ll, rectForCurNode.ur);
                if (curNode->isMacro)
                {
                    bins[i][j]->macroArea += targetDensity * overlapArea; // macro area is never used??
                }
                else
                {
                    bins[i][j]->cellArea += overlapArea;
                }
            }
        }
    }

    for (int i = 0; i < binDimension.x; i++)
    {

        for (int j = 0; j < binDimension.y; j++)
        {
            bins[i][j]->nonMacroDensity = (bins[i][j]->cellArea + bins[i][j]->terminalArea + bins[i][j]->baseArea) / bins[i][j]->area;
        }
    }
}

void SAMacroLegalizer::initializeCost()
{
    totalHPWL = placeDB->calcHPWL();

    totalCellAreaCovered = getCellAreaCoveredByAllMacros();

    totalMacroOverlap = totalMacroArea - getAreaCoveredByMacros(); // SA legalization terminates when total macro overlap == 0;
}

void SAMacroLegalizer::initializeSAparams()
{
    jLimit = 1000;
    kLimit = 1000;

    miuD = totalHPWL / totalCellAreaCovered;
    miuO = (totalHPWL + totalCellAreaCovered * miuD) / (float)totalMacroOverlap;

    beta = 1.5;
}

int SAMacroLegalizer::getAreaCoveredByMacros()
{
    // todo: use segment tree to solve this problem(which is actually a classic OJ problem)

    RectangleAreaSolution solution;
    vector<vector<int>> rectangles;
    //! x and y should be integers here!
    for (Module *curMacro : dbMacros)
    {
        vector<int> llAndur;
        llAndur.push_back(curMacro->getLL_2D().x);
        llAndur.push_back(curMacro->getLL_2D().y);
        llAndur.push_back(curMacro->getUR_2D().x);
        llAndur.push_back(curMacro->getUR_2D().y);
        rectangles.push_back(llAndur);
    }
    return solution.rectangleArea(rectangles);
}

float SAMacroLegalizer::getCellAreaCoveredByAllMacros()
{
    // follow the RePlAce code
    // an approximation of cell area covered by macros, because it would take too much time to get a precise value. This approximation can be very precise when bins are small enough
    float cost = 0.0;
    for (Module *curMacro : dbMacros)
    {
        cost += getCellAreaCoveredByMacro(curMacro);
    }
    return cost;
}

float SAMacroLegalizer::getCellAreaCoveredByMacro(Module *curNode)
{
    float cost = 0;

    CRect rectForCurNode;
    rectForCurNode.ll = curNode->getLL_2D();
    rectForCurNode.ur = curNode->getUR_2D();

    VECTOR_2D_INT binStartIdx; // binStartIdx: index the index of the first bin that has overlap with a cell on X/Y direction
    VECTOR_2D_INT binEndIdx;
    binStartIdx.x = INT_DOWN((rectForCurNode.ll.x - placeDB->coreRegion.ll.x) / binStep.x);
    binEndIdx.x = INT_DOWN((rectForCurNode.ur.x - placeDB->coreRegion.ll.x) / binStep.x);

    binStartIdx.y = INT_DOWN((rectForCurNode.ll.y - placeDB->coreRegion.ll.y) / binStep.y);
    binEndIdx.y = INT_DOWN((rectForCurNode.ur.y - placeDB->coreRegion.ll.y) / binStep.y);

    if (!(binStartIdx.x >= 0))
    {
        cout << "Module pos: " << rectForCurNode.ll << " " << placeDB->coreRegion.ll << endl;
    }
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

            cost += getOverlapArea_2D(bins[i][j]->ll, bins[i][j]->ur, rectForCurNode.ll, rectForCurNode.ur) * bins[i][j]->nonMacroDensity;
        }
    }
    return cost;
}

int SAMacroLegalizer::getMacroOverlapArea(Module *curMacro)
{
    int totalOverlap = 0;
    for (Module *otherMacro : dbMacros)
    {
        if (otherMacro == curMacro)
        {
            continue;
        }
        totalOverlap += getOverlapArea_2D(curMacro->getLL_2D(), curMacro->getUR_2D(), otherMacro->getLL_2D(), otherMacro->getUR_2D());
    }
    return totalOverlap;
}

float SAMacroLegalizer::getMacroCost(Module *curMacro, float &HPWL, float &cellCovered, float &macroOverlap)
{
    HPWL = placeDB->calcModuleHPWL(curMacro);
    cellCovered = getCellAreaCoveredByMacro(curMacro);
    macroOverlap = getMacroOverlapArea(curMacro);
    return HPWL + cellCovered * miuD + macroOverlap * miuO;
}

void SAMacroLegalizer::SAperturb()
{
    // this function randomly choose a macro and perturb its coordinate

    //! 1. choose a macro randomly
    int randomMacroIndex = 0;
    int rnd_idx = 0;
    double drnd_idx = 0;
    int mac_idx = 0;

    rnd_idx = rand();
    drnd_idx = (double)rnd_idx / RAND_MAX;
    randomMacroIndex = (int)(drnd_idx * (double)dbMacros.size());
    assert(randomMacroIndex < dbMacros.size());
    Module *chosenOne = dbMacros[randomMacroIndex];

    //! 2. move this macro randomly, first, decide the move vector(delta of coordinates)
    VECTOR_2D_INT delta;

    POS_3D curCenter = chosenOne->getCenter();

    VECTOR_2D_INT rnd;
    VECTOR_2D drnd;
    rnd.x = rand();
    rnd.y = rand();
    float RAND_MAX_INVERSE = (float)1.0 / RAND_MAX;
    drnd.x = (float)rnd.x * RAND_MAX_INVERSE - 0.5;
    drnd.y = (float)rnd.y * RAND_MAX_INVERSE - 0.5;

    delta.x = round(drnd.x * r.x) * u.x;
    delta.y = round(drnd.y * r.y / placeDB->commonRowHeight) * placeDB->commonRowHeight * u.y;

    float curHPWLCost;
    float curCellCoveredCost;
    float curMacroOverlapCost;
    float curCost = getMacroCost(chosenOne, curHPWLCost, curCellCoveredCost, curMacroOverlapCost);

    placeDB->moveModule_2D(chosenOne, delta);

    float newHPWLCost;
    float newCellCoveredCost;
    float newMacroOverlapCost;
    float newCost = getMacroCost(chosenOne, newHPWLCost, newCellCoveredCost, newMacroOverlapCost);

    bool accept = acceptPerturb(curCost, newCost);

    if (accept)
    {
        totalHPWL += newHPWLCost - curHPWLCost;
        totalCellAreaCovered += newCellCoveredCost - curCellCoveredCost;
        totalMacroOverlap += newMacroOverlapCost - curMacroOverlapCost;

        if (totalMacroOverlap <= 0 && newMacroOverlapCost <= 0 && curMacroOverlapCost > 0)
        {
            totalMacroOverlap = totalMacroArea - getAreaCoveredByMacros();
            if (totalMacroOverlap <= 0)
            {
                overlapFree = true;
            }
        }
    }
    else
    {
        placeDB->setModuleCenter_2D(chosenOne, curCenter.x, curCenter.y);
    }
}

bool SAMacroLegalizer::acceptPerturb(float oldCost, float newCost)
{
    int rnd = 0;
    float drnd = 0;

    float expValue = 0;
    float costDelta = (newCost - oldCost) / oldCost;

    rnd = rand();
    drnd = (float)rnd / RAND_MAX;

    expValue = std::exp(-1.0 * costDelta / SAtemperature);

    if (drnd < expValue)
    {
        return true;
    }
    else
    {
        return false;
    }
}
