#include "legalizer.h"

void AbacusLegalizer::legalization()
{
    initialization();
    for (Module *curCell : dbCells)
    {
        double cost = DOUBLE_MAX; // current minimum cost
        //! assume subrows are sorted by y coordinate(non decreasing)
        // 1. find closest row
        int subrowCount = subrows.size();
        int bestRowIndex = -1;
        int closestRowIndex = 0;
        for (AbacusRow curRow : subrows)
        {
            if (float_less(fabs(curRow.bottom - curCell->getLL_2D().y), 0.5 * curRow.height)) //? double check here
            {
                break;
            }
            closestRowIndex++;
        }
        //! 2. search in two directions(higher rows and lower rows) for the best row
        //? optimize here?
        // first search in higher rows
        for (int i = closestRowIndex; i < subrowCount; i++)
        {
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
    initializeCells();
    initializeObstacles();
    initializeSubrows();
}

void AbacusLegalizer::initializeCells()
{
    for (Module *curNode : placeDB->dbNodes)
    {
        if (!curNode->isMacro)
        {
            dbCells.push_back(curNode);
        }
    }
    sort(dbCells.begin(), dbCells.end(), [=](Module *a, Module *b)
         { return float_lessorequal(a->getLL_2D().x, b->getLL_2D().x); });

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
        }
    }

    // todo sort subrows by y coordinate!
}

double AbacusLegalizer::placeRow(Module *cell, int bestRow, bool trial)
{
    AbacusRow *tempRowPointer;
    AbacusRow tempRow;
    if (trial == ABACUS_TRIAL)
    {
        tempRow = subrows[bestRow];
        tempRowPointer = &tempRow;
        // for trial place, would not actually modify subrows
    }
    else if (trial == ABACUS_FINAL)
    {
        tempRowPointer = &subrows[bestRow];
        // final place
    }
    else
    {
        cerr << "Not trial nor final\n";
        exit(0);
    }

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
    }
    else
    {
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
    int predecessorIndex = clusterIndex--;
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
