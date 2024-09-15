#include "detailed.h"
void DetailedPlacer::detailedPlacement()
{
    initialization();
    runISM();
    // runGlobalSwap();
    // runLocalReordering();
}
void DetailedPlacer::initialization()
{
    placedb->removeBlockedSite(); // intervals updated for detailed placement here
}
void DetailedPlacer::runISM()
{
    ISMDP *ismDetailedPlacer = new ISMDP(placedb);
    ismDetailedPlacer->initialization();
    // ismDetailedPlacer->independentCells = false;

    double totalTime;
    time_start(&totalTime);

    double initialHPWL = placedb->calcHPWL();

    double previousHPWL = initialHPWL;

    double detailTimeStart = seconds();

    double totalDetailTime = 0;
    cout << "\nRunning ISM for detailed palcement...\n";
    flush(cout);

    // placedb.SaveBlockLocation();

    // the following are 3 adjustable parameters
    double timeLimit = 28800; // 28800= 8 hours, adjustable
    double stop = -0.2;       // adjustable parameter
    double windowSize = 20;   // in row height, adjustable, (20x row height for now)
    // assert(stop < 0);

    totalDetailTime = seconds() - detailTimeStart;

    double accumulatedTimeStart = seconds(); // donnie 2006-03-13

    int index = 0;
    while (totalDetailTime < timeLimit)
    {
        // placedb.SaveBlockLocation();
        double oneIteTimeStart = seconds();

        // de.MAXWINDOW = param.de_MW; // MAXWINDOW reset here!
        // de.MAXMODULE = param.de_MM;

        // int run_para1 = param.de_window - i; // de_window = 20, this is window size
        int windowSize = windowSize - index;
        if (windowSize < 15)
        {
            windowSize = 15 + index % 5;
        }

        int windowOverlap = 2 + (int)(index / 2);
        if (windowOverlap > 8)
        {
            windowOverlap = 8;
        }
        // cout << windowSize << " " << windowOverlap << endl;

        ismDetailedPlacer->ISMSweep(windowSize, windowOverlap);

        double currentHPWL = placedb->calcHPWL();
        // double wlx = placedb.GetHPWLp2p();
        double oneIteTime = double(seconds() - oneIteTimeStart);
        double accumulatedTime = seconds() - accumulatedTimeStart;

        printf(" iteration:%2d HPWL=%.0f (%.3f%%)(%.3f%%)   time: %d sec   all: %d sec\n",
               index, currentHPWL, 100.0 * (currentHPWL / previousHPWL - 1.0), 100.0 * (currentHPWL / initialHPWL - 1.0),
               (int)oneIteTime, (int)accumulatedTime);

        fflush(stdout);

        if ((100.0 * (currentHPWL / previousHPWL - 1.0)) > stop * 3 && ismDetailedPlacer->independentCells)
        {
            break;
        }

        if ((100.0 * (currentHPWL / previousHPWL - 1.0)) > stop) // by donnie
        {
            if (ismDetailedPlacer->doubleWindow == false)
            {
                ismDetailedPlacer->doubleWindow = true;
                ismDetailedPlacer->independentCells = true;
                cout << "Consider independent cells now\n"; // ! start independent
            }
        }

        totalDetailTime = seconds() - detailTimeStart;
        index++;
        previousHPWL = currentHPWL;
    }
    time_end(&totalTime);
    double finalHPWL = placedb->calcHPWL();
    cout << "ISM result: Pin-to-pin HPWL= " << finalHPWL << " (" << 100.0 * (finalHPWL / initialHPWL - 1.0) << "%)\n";
    cout << "ISM total runtime:" << totalTime << "\n";
}

void DetailedPlacer::runLocalReordering()
{
    LocalReorderingDP *lrDetailedPlacer = new LocalReorderingDP(placedb);
    lrDetailedPlacer->initialization();
}

void ISMDP::ISMSweep(int windowSize, int windowOverlap) // a square window, width=height=windowSize
{
    if (windowOverlap >= windowSize)
    {
        windowOverlap = 0;
    }

    // fplan->CalcHPWL();
    // double wl1 = fplan->GetHPWLp2p();

    int x_num = (int)(placeDB->coreRegion.getWidth() / (placeDB->commonRowHeight));
    int y_num = (int)(placeDB->coreRegion.getHeight() / (placeDB->commonRowHeight));
    //	cout<<"\n total bins:"<<total<<"\n";
    for (int i = 0; i < y_num; i = i + windowSize - windowOverlap) // window slide across the chip region, overlap means overlap between windows
    {
        for (int j = 0; j < x_num; j = j + windowSize - windowOverlap)
        {
            CRect curWindow;

            curWindow.ll.x = placeDB->coreRegion.ll.x + j * placeDB->commonRowHeight;
            curWindow.ll.y = placeDB->coreRegion.ll.y + i * placeDB->commonRowHeight;
            curWindow.ur.x = curWindow.ll.x + windowSize * placeDB->commonRowHeight;
            curWindow.ur.y = curWindow.ll.y + windowSize * placeDB->commonRowHeight;

            // cout << curWindow;
            // ISMRun(fplan->m_coreRgn.left + j * placeDB->commonRowHeight, fplan->m_coreRgn.bottom + i * placeDB->commonRowHeight,
            //        window * placeDB->commonRowHeight, window * placeDB->commonRowHeight);
            ISMRun(curWindow);
        }
    }
}

void ISMDP::ISMRun(CRect window)
{

    // 1.build modules size set in the window
    // 2.build slot and modules vector, if it's length > maxModuleCount , cut it.
    // 3.calc cost matrix
    // 4.run bipartite matching
    // 5.save result

    int startRowIndex = max(placeDB->y2RowIndex(window.ll.y), 0); // lowest row

    double endRowBottom = min(double(window.ur.y), placeDB->coreRegion.ur.y - placeDB->commonRowHeight);
    int endRowIndex = placeDB->y2RowIndex(endRowBottom); // highest row

    multimap<double, Module *> moduleMap; // use multimap for auto sorting (by module width)

    list<Module *> moduleList; // list<module ID>

    // build module size map, with all modules in the window
    for (int i = startRowIndex; i <= endRowIndex; i++) // i< endRowIndex in ntuplace
    {
        // map<double, int>::iterator iter;// curModule
        // map<double, int>::iterator startModule, endModule;// begin and end in the original code

        auto startModule = ISMRows[i].rowModules.lower_bound(window.ll.x); //! rowModules initialized in initializeISMRows()
        auto endModule = ISMRows[i].rowModules.lower_bound(window.ur.x);

        for (auto curModuleIter = startModule; curModuleIter != endModule; curModuleIter++)
        {
            // if (curModuleIter->second->getHeight() == placeDB->commonRowHeight)
            if (!(curModuleIter->second->isMacro)) // macros are ignored
            {                                      //! adding all cells in the window
                moduleMap.insert(pair<double, Module *>(curModuleIter->second->getWidth(), curModuleIter->second));
            }
        }
        // ISMRows[i].showModule();
        // cout << "modulecountcp1: " << moduleMap.size() << endl;
    }

    if (doubleWindow == true) // enable double window, pick cells from 2 windows
    {
        for (int i = startRowIndex; i < endRowIndex; i++)
        {
            map<double, int>::iterator iter;
            // modified by Jin 20070726
            auto startModule = ISMRows[i].rowModules.lower_bound(window.ur.x + 5 * window.getWidth()); //?? double window?
            auto endModule = ISMRows[i].rowModules.lower_bound(window.ur.x + 6 * window.getWidth());   //??
            for (auto curModuleIter = startModule; curModuleIter != endModule; curModuleIter++)
            {
                // if (curModuleIter->second->getHeight() == placeDB->commonRowHeight)
                if (!(curModuleIter->second->isMacro)) // macros are ignored
                {
                    moduleMap.insert(pair<double, Module *>(curModuleIter->second->getWidth(), curModuleIter->second));
                }
            }
        }
    }

    //! moduleList stores ALL modules in current window(windows), sorted by module width, decreaingly (sorted increasingly in moduleMap)
    for (auto rIter = moduleMap.rbegin(); rIter != moduleMap.rend(); rIter++)
    {
        moduleList.push_back(rIter->second);
    }

    while (moduleList.size() != 0)
    {
        // slots and modules: assign all modules to all slots, slots.size()==modules.size()?
        vector<POS_2D> slots;
        vector<Module *> modules;
        slots.reserve(maxModuleCount);
        modules.reserve(maxModuleCount);

        int insertedModuleCount = 0;
        const double maxWIDTH = (*moduleList.begin())->getWidth(); // WIDTH == largest width

        // The following 2 vectors are connected: removedRowIndexes[i] = the row that removedModules[i] is located in
        vector<Module *> removedModules;
        removedModules.reserve(maxModuleCount);

        vector<int> removedRowIndexes;
        removedRowIndexes.reserve(maxModuleCount);

        // The following 3 vectors are connected:
        // empty: space left after inserting a cell with width less than maxWIDTH, emptyX[i]: start of the empty,
        // emptyWidth[i]: length of the empty, emptyRowIndex[i]: row in which the ith empty located

        vector<double> emptyX;
        emptyX.reserve(maxModuleCount);

        vector<double> emptyWidth;
        emptyWidth.reserve(maxModuleCount);

        vector<int> emptyRowID;
        emptyRowID.reserve(maxModuleCount);

        for (auto curModuleIter = moduleList.begin(); curModuleIter != moduleList.end();)
        {
            Module *curModule = *(curModuleIter);
            POS_2D curModulePos = curModule->getLL_2D();
            bool inserted = false;
            int rowID = placeDB->y2RowIndex(curModulePos.y);

            if (curModule->getWidth() == maxWIDTH)
            {
                bool connection = false;
                if (independentCells == true) // independent set
                {
                    for (Module *storedModule : modules)
                    {
                        if (placeDB->isConnected(storedModule, curModule) == true)
                        {
                            connection = true;
                        }
                    }
                }
                if (connection == false)
                {
                    modules.push_back(curModule);
                    POS_2D slot;
                    slot.x = curModulePos.x;
                    slot.y = curModulePos.y;
                    slots.push_back(slot);
                    // remove_module(*iter,rowID);
                    removedModules.push_back(curModule);
                    removedRowIndexes.push_back(rowID);
                    inserted = true;
                }
            }
            else if (curModule->getWidth() < maxWIDTH) // width < WIDTH, need to check if there is space for a slot of maxWIDTH
            {
                auto spaceNextToCell = ISMRows[rowID].rowSpaces.upper_bound(curModulePos.x);
                if (spaceNextToCell != ISMRows[rowID].rowSpaces.end())
                {
                    if (spaceNextToCell->first == curModule->getWidth() + curModulePos.x) //?? when would this happen for a legalized design???? if this won't happen, then ISM actually only consider cells with same size?wozhi
                    {
                        if ((curModule->getWidth() + spaceNextToCell->second) >= maxWIDTH) // the space occupied by the module + the space next to the module can hold a module of maximum size(maxWIDTH)
                        {
                            bool connection = false;
                            if (independentCells == true)
                            {
                                // for (int m = 0; m < (int)modules.size(); m++)
                                // {
                                //     if (placeDB->isConnected(modules[m], curModule) == true)
                                //     {
                                //         connection = true;
                                //     }
                                // }
                                for (Module *storedModule : modules)
                                {
                                    if (placeDB->isConnected(storedModule, curModule) == true)
                                    {
                                        connection = true;
                                    }
                                }
                            }
                            if (connection == false)
                            {
                                double tailX = curModule->getWidth() + curModulePos.x;
                                double tailWidth = maxWIDTH - curModule->getWidth(); // space left EMPTY after removing the cell with width less than maxWidth

                                modules.push_back(curModule);
                                POS_2D slot;
                                slot.x = curModulePos.x;
                                slot.y = curModulePos.y;

                                slots.push_back(slot);
                                // remove_module(*iter,rowID);
                                removedModules.push_back(curModule);
                                removedRowIndexes.push_back(rowID);
                                emptyX.push_back(tailX); //! need to understand empty! 2024.8.21
                                emptyWidth.push_back(tailWidth);
                                emptyRowID.push_back(rowID);
                                ISMRows[rowID].removeTail(tailX, tailWidth); //!!! remove tail here, but add space later, why remove tail? for whitespace handling?
                                inserted = true;
                            }
                        }
                    }
                }
            }

            if (inserted == true)
            {
                auto moduleIter2 = curModuleIter;
                curModuleIter++;
                moduleList.erase(moduleIter2);
                insertedModuleCount++;
                if (insertedModuleCount >= maxModuleCount)
                {
                    break;
                }
            }
            else
            {
                curModuleIter++;
            }
        }
        // Handle whitespace and add extra slots

        if (insertedModuleCount < maxWindow)
        {
            int emptySlotCount = 0;
            for (int i = startRowIndex; i < endRowIndex; i++)
            {
                for (auto curRowSpaceIter = ISMRows[i].rowSpaces.lower_bound(window.ll.x);
                     curRowSpaceIter != ISMRows[i].rowSpaces.lower_bound(window.ur.x);
                     curRowSpaceIter++)
                {
                    int slotCount = (int)(curRowSpaceIter->second / maxWIDTH);
                    for (int j = 0; j < slotCount; j++)
                    {
                        modules.push_back(NULL); // is this ok?
                        POS_2D slot;
                        slot.x = curRowSpaceIter->first + j * maxWIDTH;
                        slot.y = ISMRows[i].ll.y;

                        assert(placeDB->coreRegion.ll.y + i * placeDB->commonRowHeight == ISMRows[i].ll.y);

                        slots.push_back(slot); //! how was the vector 'position'used?
                        emptySlotCount++;
                        if (emptySlotCount + insertedModuleCount >= maxWindow)
                        {
                            break;
                        }
                    }
                }
            }
        }

        for (int i = 0; i < emptyX.size(); i++)
        {
            ISMRows[emptyRowID[i]].insertSpace(emptyX[i], emptyWidth[i]); // reinsert tail
        }
        // remove all selected modules, and then assign all modules (stored in the vector modules) to all available positions, which are stored in the vector position
        for (int i = 0; i < removedModules.size(); i++)
        {
            ISMRows[removedRowIndexes[i]].removeModule(removedModules[i]);
        }

        // 3. eatablish bimatching object, calc cost matrix
        int deg = modules.size(); // matrix degree
        // bimatching matrix(deg);
        lap2 matrix(deg);

        // calc cost matrix
        for (int i = 0; i < deg; i++)
        {
            if (modules[i] == NULL) // (donnie) whitespace?
            {
                for (int j = 0; j < deg; j++)
                {
                    matrix.put(i, j, 0);
                }
            }
            else
            {
                // bool set=false;
                POS_2D bestPos = slots[0];
                double bestWirelength = DOUBLE_MAX;
                // double ox=fplan->m_modules[modules[i]].m_x;
                // double oy=fplan->m_modules[modules[i]].m_y;
                // CNetLengthCalc netcalc(*fplan, modules[i]);
                // netcalc.init();

                for (int j = 0; j < deg; j++)
                {
                    placeDB->setModuleLocation_2D(modules[i], slots[j].x, slots[j].y);
                    double wl = placeDB->calcModuleHPWLsafe(modules[i]);
                    // cerr<<" "<<wl;
                    // double wl=0;
                    // for(int k=0;k<(int)fplan->m_modules[modules[i]].m_netsId.size();k++)
                    //{
                    //   wl = wl+fplan->GetNetLength(fplan->m_modules[modules[i]].m_netsId[k]);
                    // }
                    // cerr<<","<<wl;
                    // if(wl!=wl2)
                    //	cerr<<" "<<wl<<","<<wl2;
                    // force module find empty space ^^  ==>FOOL!
                    // if( (modules[j]!=modules[i])&&(modules[j]!=-1))
                    //	wl=wl+100000;
                    if (wl < bestWirelength)
                    {
                        bestWirelength = wl;
                        bestPos = slots[j];
                    }
                    // matrix.costs.put(i,j,wl);
                    matrix.put(i, j, wl);
                }
                // fplan->SetModuleLocation(modules[i],ox,oy);
                placeDB->setModuleLocation_2D(modules[i], bestPos.x, bestPos.y); // is it necessary to set now? 2024.8.27
            }
        }

        //	matrix.show();

        // 4.run matching
        // matrix.find();
        matrix.lapSolve();
        vector<int> result;
        matrix.getResult(result);

        // 5.save result
        for (int i = 0; i < deg; i++)
        {
            if (modules[i] == NULL)
            {
                break;
            }

            int pos = placeDB->y2RowIndex(slots[result[i]].y);

            if (!ISMRows[pos].insertModule(slots[result[i]].x, modules[i])) //!!module reinserted here!!
            {
                if (!gArg.CheckExist("nocheck")) // (donnie) 2007-03-25
                {
                    // cout << "\ninsert" << pos << " x:" << position[result[i]].x
                    //      << " w:" << fplan->m_modules[modules[i]].m_width
                    //      << " mod:" << modules[i]
                    //      << " name:" << fplan->m_modules[modules[i]].m_name << "\n";
                    // m_de_row[pos].showspace();
                }
            }
            placeDB->setModuleLocation_2D(modules[i], slots[result[i]].x, slots[result[i]].y);
        }
        // for(int i=0;i<deg;i++)
        //{
        //	int pos=y2rowID(position[i].y);
        //	if(!m_de_row[pos].insert_module(position[i].x,fplan->m_modules[modules[i]].m_width,modules[i]))
        //	{
        //		cout<<"\ninsert"<<pos<<" x:"<<position[i].x<<" w:"<<fplan->m_modules[i].m_width<<" mod:"<<modules[i]<<" name:"<<fplan->m_modules[modules[i]].m_name<<"\n";
        //		m_de_row[pos].showspace();
        //	}
        //	fplan->SetModuleLocation(modules[i],position[result[i]].x,position[result[i]].y);
        // }
    }
}

void ISMDP::initialization()
{
    // placeDB->removeBlockedSite();
    initializeParams();
    initializeISMRows();
}

void ISMDP::initializeParams()
{
    // see ntuplace3, not sure if maxWindow=90, maxModuleCount=128, or maxWindow=maxModuleCount=64
    doubleWindow = false;
    independentCells = false;
    maxWindow = 90;
    maxModuleCount = 128;
}

void ISMDP::initializeISMRows()
{
    double ISMRowLength = placeDB->coreRegion.getWidth();
    cout << "initialize ISM rows: \n";

    // 1.create ISMRows
    for (SiteRow curRow : placeDB->dbSiteRows)
    {
        ISMRow newRow(placeDB->coreRegion.ll.x, curRow.bottom, ISMRowLength);
        for (Interval curInterval : curRow.intervals)
        {
            // cout<<"father: "<<curInterval.start<<" "<<curInterval.getLength()<<endl;
            newRow.rowSpaces[curInterval.start] = curInterval.getLength();
            // cout<<"son: "<<newRow.rowSpaces[curInterval.start]<<endl;
        }
        ISMRows.push_back(newRow);
    }

    // 2. insert cells to ISMRows
    for (Module *curModule : placeDB->dbNodes) // all nodes, macros and cells
    {

        int coveredRowCount = 1;
        POS_2D curModulePos = curModule->getLL_2D();
        float moduleHeight = curModule->getHeight();
        if (moduleHeight > placeDB->commonRowHeight) //?? macro included???
        {
            coveredRowCount = (int)(moduleHeight / placeDB->commonRowHeight);
            if (placeDB->commonRowHeight * coveredRowCount < moduleHeight)
            {
                coveredRowCount++; // for macros
            }
        }

        int pos = placeDB->y2RowIndex(curModulePos.y);

        for (int i = 0; i < coveredRowCount; i++)
        {
            ISMRows[pos + i].insertModule(curModulePos.x, curModule);
        }
    }

    // placeDB->showRows();

    // for (ISMRow curRow : ISMRows)
    // {
    //     curRow.showSpace();
    //     curRow.showModule();
    // }
}

bool ISMRow::insertModule(double x, Module *curModule)
{
    //! both moduleX and moduleWidth should be integer for a legalized placement
    double moduleX = round(x);
    double moduleWidth = round(curModule->getWidth());
    //@modified by Jin 20070727

    auto rowSpaceIter = rowSpaces.upper_bound(moduleX);
    if (rowSpaceIter == rowSpaces.begin())
    {
        // cout<<"wtf?\n";
        return false; // x<all empty site's start point, shouldn't happen for a legalized placement?
    }
    else
    {
        --rowSpaceIter; // upper_bound returns the first element greater than key
        if ((rowSpaceIter->second - (moduleX - rowSpaceIter->first)) < moduleWidth)
        {
            // cout<<rowSpaceIter->second<<" "<<moduleX<<" "<<rowSpaceIter->first<<" "<<moduleWidth<<endl;
            return false; // space can't contain this cell
        }
        else
        {
            double l2 = moduleX - rowSpaceIter->first;
            if (l2 == 0)
            {
                if (((rowSpaceIter->first + rowSpaceIter->second) - (moduleX + moduleWidth)) != 0)
                {
                    rowSpaces[moduleX + moduleWidth] = (rowSpaceIter->first + rowSpaceIter->second) - (moduleX + moduleWidth);
                }
                rowSpaces.erase(rowSpaceIter->first); // space updated here. space initilization in the constructor didn't consider modules, here the spaces are modified considering cells
            }
            else
            {

                if ((moduleX + moduleWidth) < (rowSpaceIter->first + rowSpaceIter->second))
                {
                    rowSpaces[moduleX + moduleWidth] = (rowSpaceIter->first + rowSpaceIter->second) - (moduleX + moduleWidth);
                }
                rowSpaceIter->second = l2; // update length of space, which is l2.
            }
            rowModules[moduleX] = curModule;
            // cout<<"inserted\n"<<curModule->idx<<endl;
            return true;
        }
    }
}

void ISMRow::removeModule(Module *module)
{

    //?SHOULD NOT use double as the index of map

    bool flag = insertSpace(module->getLL_2D().x, module->getWidth());
    double targetModuleX = round(module->getLL_2D().x);

    auto curRowModuleIter = rowModules.find(targetModuleX);
    if (curRowModuleIter == rowModules.end())
    {
        printf("Warning: mID %d not found\n", module->idx);
    }
    else if (curRowModuleIter->second != module)
    {
        printf("Warning: mID %d does not equal ite->second %d\n", module->idx, curRowModuleIter->second);
    }
    else
    {
        rowModules.erase(curRowModuleIter); // cell will be reinserted after ISM
    }
    //@modified by Jin 20070727
}

bool ISMRow::removeTail(double x, double width)
{

    x = round(x);
    width = round(width);

    auto spaceIter = rowSpaces.upper_bound(x);
    if (spaceIter == rowSpaces.begin())
        return false; // x<all empty site's start point

    spaceIter--;
    if ((spaceIter->second - (x - spaceIter->first)) < width) // (iter->first+iter->second)-(x+w) < 0, x+w is not convered by the empty indexed by iter
        return false;

    double l2 = x - spaceIter->first;

    if (l2 == 0) // x == iter->first
    {
        if (((spaceIter->first + spaceIter->second) - (x + width)) != 0) // (iter->first+iter->second)-(x+w) > 0
        {
            rowSpaces[x + width] = (spaceIter->first + spaceIter->second) - (x + width);
        }
        rowSpaces.erase(spaceIter->first); //? why
    }
    else
    { // what is this situation????
        if ((x + width) < (spaceIter->first + spaceIter->second))
        {

            rowSpaces[x + width] = (spaceIter->first + spaceIter->second) - (x + width);
        }
        spaceIter->second = l2;
    }

    return true;
}

bool ISMRow::insertSpace(double x, double width)
{
    // merge a spaces with existing spaces

    x = round(x);
    width = round(width);

    bool front; // true: no space in front of (and abut) the space to be added
    bool end;   // true: no space behind after (and abut) the space to be added

    auto curSpaceIter = rowSpaces.lower_bound(x);
    if (curSpaceIter == rowSpaces.begin())
    {
        front = true;
    }
    else
    {
        curSpaceIter--;
        if ((curSpaceIter->first + curSpaceIter->second) == x)
        {
            front = false;
        }
        else
        {
            front = true;
        }
    }
    curSpaceIter = rowSpaces.lower_bound(x);

    if (curSpaceIter == rowSpaces.end())
    {
        end = true;
    }
    else
    {
        if (curSpaceIter->first == (x + width))
        {
            end = false;
        }
        else
        {
            end = true;
        }
    }

    if ((front == true) && (end == true))
    {
        rowSpaces[x] = width;
    }
    else if ((front == true) && (end == false))
    {
        curSpaceIter = rowSpaces.lower_bound(x);
        rowSpaces[x] = width + curSpaceIter->second;
        rowSpaces.erase(curSpaceIter->first); // or erase(curSpaceIter) ?
    }
    else if ((front == false) && (end == true))
    {
        curSpaceIter = rowSpaces.lower_bound(x);
        curSpaceIter--;
        rowSpaces[curSpaceIter->first] = curSpaceIter->second + width;
    }
    else // front==false && end == false
    {
        curSpaceIter = rowSpaces.lower_bound(x);
        double spaceBehindX = curSpaceIter->first;
        double spaceBehindWidth = curSpaceIter->second;
        curSpaceIter--;
        rowSpaces[curSpaceIter->first] = curSpaceIter->second + width + spaceBehindWidth;
        rowSpaces.erase(spaceBehindX);
    }
    return true;
}

void ISMRow::showSpace()
{
    cout << "\n=====ISM ROW SPACE ===\n";

    for (auto iter = rowSpaces.begin(); iter != rowSpaces.end(); iter++)
    {
        // modified by Jin 20070727
        printf("[%.10f,%.10f] ", iter->first, iter->second);
        // cout<<" ["<<iter->first<<","<<iter->second<<"] ";
        // modified by Jin 20070727
    }
    cout << '\n';
}

void ISMRow::showModule()
{
    cout << "\n=====ISM ROW Module ===\n";

    for (auto iter = rowModules.begin(); iter != rowModules.end(); iter++)
    {
        cout << " [" << iter->first << "," << iter->second->idx << "] ";
    }
    cout << '\n';
}

int lap2::lapSolve()
{
    // input:
    // degree        - problem size
    // cost - cost matrix

    // output:
    // assignment     - column assigned to row in solution
    // assignment     - row assigned to column in solution
    // u          - dual variables, row reduction numbers
    // v          - dual variables, column reduction numbers
    bool unassignedfound;
    int i, imin, numfree = 0, prvnumfree, f, i0, k, freerow;
    int j, j1, j2, endofpath, last = 0, low, up;
    int min = 0, h, umin, usubmin, v2;
    j2 = 0;
    endofpath = 0;

    vector<int> free;
    free.resize(degree, 0);
    vector<int> collist;
    collist.resize(degree, 0);
    vector<int> matches;
    matches.resize(degree, 0);
    vector<int> d;
    d.resize(degree, 0);
    vector<int> pred;
    pred.resize(degree, 0);

    vector<int> colsol;
    colsol.resize(degree, 0);

    vector<int> u;
    u.resize(degree, 0);

    vector<int> v;
    v.resize(degree, 0);

    // COLUMN REDUCTION
    for (j = degree - 1; j >= 0; j--) // reverse order gives better results.
    {
        // find minimum cost over rows.
        min = cost[0][j];
        imin = 0;
        for (i = 1; i < degree; i++)
            if (cost[i][j] < min)
            {
                min = cost[i][j];
                imin = i;
            }
        v[j] = min;

        if (++matches[imin] == 1)
        {
            // init assignment if minimum row assigned for first time.
            assignment[imin] = j;
            colsol[j] = imin;
        }
        else
            colsol[j] = -1; // row already assigned, column not assigned.
    }

    // REDUCTION TRANSFER
    for (i = 0; i < degree; i++)
        if (matches[i] == 0) // fill list of unassigned 'free' rows.
            free[numfree++] = i;
        else if (matches[i] == 1) // transfer reduction from rows that are assigned once.
        {
            j1 = assignment[i];
            min = INF;
            for (j = 0; j < degree; j++)
                if (j != j1)
                    if (cost[i][j] - v[j] < min)
                        min = cost[i][j] - v[j];
            v[j1] = v[j1] - min;
        }

    // AUGMENTING ROW REDUCTION
    int loopcnt = 0; // do-loop to be done twice.
    do
    {
        loopcnt++;

        // scan all free rows.
        // in some cases, a free row may be replaced with another one to be scanned next.
        k = 0;
        prvnumfree = numfree;
        numfree = 0; // start list of rows still free after augmenting row reduction.
        while (k < prvnumfree)
        {
            i = free[k];
            k++;

            // find minimum and second minimum reduced cost over columns.
            umin = cost[i][0] - v[0];
            j1 = 0;
            usubmin = INF;
            for (j = 1; j < degree; j++)
            {
                h = cost[i][j] - v[j];
                if (h < usubmin)
                    if (h >= umin)
                    {
                        usubmin = h;
                        j2 = j;
                    }
                    else
                    {
                        usubmin = umin;
                        umin = h;
                        j2 = j1;
                        j1 = j;
                    }
            }

            i0 = colsol[j1];
            if (umin < usubmin)
                // change the reduction of the minimum column to increase the minimum
                // reduced cost in the row to the subminimum.
                v[j1] = v[j1] - (usubmin - umin);
            else             // minimum and subminimum equal.
                if (i0 >= 0) // minimum column j1 is assigned.
                {
                    // swap columns j1 and j2, as j2 may be unassigned.
                    j1 = j2;
                    i0 = colsol[j2];
                }

            // (re-)assign i to j1, possibly de-assigning an i0.
            assignment[i] = j1;
            colsol[j1] = i;

            if (i0 >= 0) // minimum column j1 assigned earlier.
                if (umin < usubmin)
                    // put in current k, and go back to that k.
                    // continue augmenting path i - j1 with i0.
                    free[--k] = i0;
                else
                    // no further augmenting reduction possible.
                    // store i0 in list of free rows for next phase.
                    free[numfree++] = i0;
        }
    } while (loopcnt < 2); // repeat once.

    // AUGMENT SOLUTION for each free row.
    for (f = 0; f < numfree; f++)
    {
        freerow = free[f]; // start row of augmenting path.

        // Dijkstra shortest path algorithm.
        // runs until unassigned column added to shortest path tree.
        for (j = 0; j < degree; j++)
        {
            d[j] = cost[freerow][j] - v[j];
            pred[j] = freerow;
            collist[j] = j; // init column list.
        }

        low = 0; // columns in 0..low-1 are ready, now none.
        up = 0;  // columns in low..up-1 are to be scanned for current minimum, now none.
                 // columns in up..degree-1 are to be considered later to find new minimum,
                 // at this stage the list simply contains all columns
        unassignedfound = false;
        do
        {
            if (up == low) // no more columns to be scanned for current minimum.
            {
                last = low - 1;

                // scan columns for up..degree-1 to find all indices for which new minimum occurs.
                // store these indices between low..up-1 (increasing up).
                min = d[collist[up++]];
                for (k = up; k < degree; k++)
                {
                    j = collist[k];
                    h = d[j];
                    if (h <= min)
                    {
                        if (h < min) // new minimum.
                        {
                            up = low; // restart list at index low.
                            min = h;
                        }
                        // new index with same minimum, put on undex up, and extend list.
                        collist[k] = collist[up];
                        collist[up++] = j;
                    }
                }

                // check if any of the minimum columns happens to be unassigned.
                // if so, we have an augmenting path right away.
                for (k = low; k < up; k++)
                    if (colsol[collist[k]] < 0)
                    {
                        endofpath = collist[k];
                        unassignedfound = true;
                        break;
                    }
            }

            if (!unassignedfound)
            {
                // update 'distances' between freerow and all unscanned columns, via next scanned column.
                j1 = collist[low];
                low++;
                i = colsol[j1];
                h = cost[i][j1] - v[j1] - min;

                for (k = up; k < degree; k++)
                {
                    j = collist[k];
                    v2 = cost[i][j] - v[j] - h;
                    if (v2 < d[j])
                    {
                        pred[j] = i;
                        if (v2 == min) // new column found at same minimum value
                            if (colsol[j] < 0)
                            {
                                // if unassigned, shortest augmenting path is complete.
                                endofpath = j;
                                unassignedfound = true;
                                break;
                            }
                            // else add to list to be scanned right away.
                            else
                            {
                                collist[k] = collist[up];
                                collist[up++] = j;
                            }
                        d[j] = v2;
                    }
                }
            }
        } while (!unassignedfound);

        // update column prices.
        for (k = 0; k <= last; k++)
        {
            j1 = collist[k];
            v[j1] = v[j1] + d[j1] - min;
        }

        // reset row and column assignments along the alternating path.
        do
        {
            i = pred[endofpath];
            colsol[endofpath] = i;
            j1 = endofpath;
            endofpath = assignment[i];
            assignment[i] = j1;
        } while (i != freerow);
    }

    // calculate optimal cost.
    int lapcost = 0;
    for (i = 0; i < degree; i++)
    {
        j = assignment[i];
        lapcost = lapcost + cost[i][j];
    }
    //  this->assignment=assignment;

    return lapcost;
}

void LocalReorderingDP::initialization()
{
    initializeSegments();
}

void LocalReorderingDP::solve()
{
    
}

void LocalReorderingDP::initializeSegments()
{
    //! 1. create segments from intervals
    for (SiteRow curRow : placeDB->dbSiteRows)
    {
        for (Interval curInterval : curRow.intervals)
        {
            lrSegments.push_back(LRSegment(curRow.bottom, curInterval.start, curInterval.end));
        }
    }

    //! 2. add modules to segments
    //  printf("Init BB...");
    fflush(stdout);

    // Add all non-Macro modules into segments
    for (Module *curModule : placeDB->dbNodes)
    {
        if (curModule->isMacro) // Skip Macro modules
        {
            continue;
        }
        POS_2D curModulePos = curModule->getLL_2D();

        // Find the corresponding segment of this module, and insert the module id into the segment
        LRSegment compSeg(curModulePos.y, curModulePos.x, curModulePos.x + curModule->getWidth());
        auto iteFindSegment = lower_bound(lrSegments.begin(), lrSegments.end(), compSeg, compareLRSegment());//!!!! potential bug
        if (iteFindSegment != lrSegments.end() && iteFindSegment->bottom == curModulePos.y)
        {
            iteFindSegment->addModule(curModule);
        }
        else
        {
            fprintf(stderr, "Warning: InitFroBBCellSwap(), cannot find legal segment for "
                            "module '%s' at (%.2f, %.2f) w: %.2f h: %.2f\n",
                    curModule->name.c_str(),
                    curModulePos.x, curModulePos.y, curModule->getWidth(), curModule->getHeight());
            if (iteFindSegment != lrSegments.end())
            {
                fprintf(stderr, "   iteFindSegment  : bottom %.2f left: %.2f right: %.2f\n", iteFindSegment->bottom, iteFindSegment->start, iteFindSegment->end);
                fprintf(stderr, "   iteFindSegment-1: bottom %.2f left: %.2f right: %.2f\n", (iteFindSegment - 1)->bottom, (iteFindSegment - 1)->start, (iteFindSegment - 1)->end);
            }
        }

        // Warning: this module is not on any segments
        if (iteFindSegment == lrSegments.end())
        {
            cerr << "Warning: Module " << curModule->idx << " is not on any segments" << endl;
        }
    }

    // printf( "..s.." );
    fflush(stdout);
    // Sort the module id's by their x coordinates (increasingly)
    for (auto iteSegment = lrSegments.begin(); iteSegment != lrSegments.end(); iteSegment++)//!! potential bug regarding the lambda expression for comparing Module*
    {
        sort(iteSegment->segModules.begin(), iteSegment->segModules.end(), [=](Module *a, Module *b)
             { return float_less(a->getLL_2D().x, b->getLL_2D().x); });
    }

    // printf("done\n");
    fflush(stdout);
}
