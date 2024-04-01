#include "legalizer.h"

void AbacusLegalizer::legalization()
{
    initialization();
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
}

void AbacusLegalizer::initializeSubrows()
{
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
                // terminals overlap!!
                cerr<<"TERMINALS OVERLAP!\n";
                exit(0);
            } });

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
                SiteRow newRow;
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
            SiteRow newRow;
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
}