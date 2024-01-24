#include "placedb.h"
#include "global.h"

void PlaceDB::setCoreRegion()
{
    float bottom = dbSiteRows.front().bottom;
    float top = dbSiteRows.back().bottom + dbSiteRows.back().height;
    float left = dbSiteRows.front().start.x;
    float right = dbSiteRows.front().end.x;

    totalRowArea = 0;
    float curRowArea = 0;
    for (SiteRow curRow : dbSiteRows)
    {
        left = min(left, curRow.start.x);
        right = max(right, curRow.end.x);
        // printf( "right= %g\n", m_coreRgn.right );
        curRowArea = (curRow.end.x - curRow.start.x) * curRow.height;
        totalRowArea += curRowArea;
    }

    coreRegion.ll = POS_2D(left, bottom);
    coreRegion.ur = POS_2D(right, top);

    cout << "Set core region from site info: ";
    coreRegion.Print();
}

void PlaceDB::init_tiers()
{
}

Module *PlaceDB::addNode(int index, string name, float width, float height)
{
    Module *node = new Module(index, name, width, height);
    assert(index < dbNodes.size());
    assert(commonRowHeight > EPS); //! potential float precision problem!!
    node->isMacro = false;
    if (float_greater(height, commonRowHeight))
    {
        node->isMacro = true;
    }
    else if (float_greater(commonRowHeight, height))
    {
        printf("Cell %s height ERROR: %f\n", name, height);
        exit(-1);
    }
    dbNodes[index] = node; // memory was previously allocated
    return node;
}

Module *PlaceDB::addTerminal(int index, string name, float width, float height, bool isFixed, bool isNI)
{
    assert(width != 0 && height != 0);
    Module *terminal = new Module(index, name, width, height, isFixed, isNI);
    assert(index < dbTerminals.size());
    dbTerminals[index] = terminal;
    return terminal;
}

void PlaceDB::addNet(Net *net)
{
    assert(net->idx < dbNets.size());
    dbNets[net->idx] = net;
}

int PlaceDB::addPin(Module *masterModule, Net *masterNet, float xOffset, float yOffset)
{
    dbPins.push_back(new Pin(masterModule, masterNet, xOffset, yOffset));
    int pinId = (int)dbPins.size() - 1;
    dbPins[pinId]->setId(pinId);
    return pinId;
}

void PlaceDB::allocateNodeMemory(int n)
{
    dbNodes.resize(n);
}

void PlaceDB::allocateTerminalMemory(int n)
{
    dbTerminals.resize(n);
}

void PlaceDB::allocateNetMemory(int n)
{
    dbNets.resize(n);
}

void PlaceDB::allocatePinMemory(int n) // difference between resize and reserve: with resize we can use index to index an element in a vector after allocation and before the element was instantiated
{
    dbPins.reserve(n); //! reserve instead of resize
}

Module *PlaceDB::getModuleFromName(string name)
{
    map<string, Module *>::const_iterator ite = moduleMap.find(name);
    if (ite == moduleMap.end())
    {
        return NULL;
    }
    return ite->second;
}

void PlaceDB::setModuleLocation_2D(Module *module, float x, float y)
{
    //? use high precision comparison functions in global.h??
    //  check if x,y are legal(inside the chip)
    if (!module->isFixed)
    {
        if (x < coreRegion.ll.x)
        {
            x = coreRegion.ll.x;
        }
        if (x + module->width > coreRegion.ur.x)
        {
            x = coreRegion.ur.x - module->width - EPS;
        }
        if (y < coreRegion.ll.y)
        {
            y = coreRegion.ll.y;
        }
        if (y + module->height > coreRegion.ur.y)
        {
            y = coreRegion.ur.y - module->height - EPS;
        }
    }

    module->setLocation_2D(x, y);
}

void PlaceDB::setModuleCenter_2D(Module *module, float x, float y)
{
    //? use high precision comparison functions in global.h??
    // todo: check if x,y are legal(inside the chip)
    if (!module->isFixed)
    {
        if (x - 0.5 * module->width < coreRegion.ll.x)
        {
            // x = coreRegion.ll.x + 0.5 * module->width;
            x = coreRegion.ll.x + 0.5 * module->width + EPS;
        }
        if (x + 0.5 * module->width > coreRegion.ur.x)
        {
            // x = coreRegion.ur.x - 0.5 * module->width;
            x = coreRegion.ur.x - 0.5 * module->width - EPS;
        }
        if (y - 0.5 * module->height < coreRegion.ll.y)
        {
            // y = coreRegion.ll.y + 0.5 * module->height;
            y = coreRegion.ll.y + 0.5 * module->height + EPS;
        }
        if (y + 0.5 * module->height > coreRegion.ur.y)
        {
            // y = coreRegion.ur.y - 0.5 * module->height;
            y = coreRegion.ur.y - 0.5 * module->height - EPS;
        }
    }

    module->setCenter_2D(x, y);
}

void PlaceDB::setModuleOrientation(Module *module, int orientation)
{
    module->setOrientation(orientation);
}

void PlaceDB::setModuleLocation_2D_random(Module *module)
{
    assert(module);
    float x = rand();
    float y = rand();

    assert(coreRegion.ur.x > coreRegion.ll.x);
    assert(coreRegion.ur.y > coreRegion.ll.y);

    float potentialRegionWidth = coreRegion.ur.x - coreRegion.ll.x; // potential region for randomly place module
    float potentialRegionHeight = coreRegion.ur.y - coreRegion.ll.y;

    potentialRegionHeight -= module->getHeight();
    potentialRegionWidth -= module->getWidth();

    float RAND_MAX_INVERSE = (float)1.0 / RAND_MAX;
    x = x * RAND_MAX_INVERSE * potentialRegionWidth;
    y = y * RAND_MAX_INVERSE * potentialRegionHeight;

    setModuleLocation_2D(module, x, y);
}

double PlaceDB::calcHPWL() //! parallel this?
{
    double HPWL = 0;
    for (Net *curNet : dbNets)
    {
        HPWL += curNet->calcNetHPWL();
    }
    return HPWL;
}

double PlaceDB::calcWA_Wirelength_2D(VECTOR_2D invertedGamma)
{
    double WA = 0;
    for (Net *curNet : dbNets)
    {
        WA += curNet->calcWirelengthWA_2D(invertedGamma);
    }
    return WA;
}

double PlaceDB::calcNetBoundPins()
{
    double HPWL = 0;
    for (Net *curNet : dbNets)
    {
        HPWL += curNet->calcBoundPin();
    }
    return HPWL;
}

void PlaceDB::moveNodesCenterToCenter()
{
    POS_2D coreRegionCenter(0.5 * (coreRegion.ur.x + coreRegion.ll.x), 0.5 * (coreRegion.ur.y + coreRegion.ll.y));
    for (Module *curModule : dbNodes)
    {
        setModuleCenter_2D(curModule, coreRegionCenter.x, coreRegionCenter.y);
    }
}

void PlaceDB::setChipRegion_2D()
{
    chipRegion = coreRegion;
    for (Module *curTerminal : dbTerminals)
    {
        assert(curTerminal);
        chipRegion.ll.x = min(chipRegion.ll.x, curTerminal->getLL_2D().x);
        chipRegion.ll.y = min(chipRegion.ll.y, curTerminal->getLL_2D().y);
        // gmin.z = min(gmin.z, curTerminal->pmin.z);

        chipRegion.ur.x = max(chipRegion.ur.x, curTerminal->getUR_2D().x);
        chipRegion.ur.y = max(chipRegion.ur.y, curTerminal->getUR_2D().y);
        // gmax.z = max(gmax.z, curTerminal->pmax.z);
    }
    //!!!!!!!chipRegion.ll!=(0,0)
}

void PlaceDB::showDBInfo()
{
    float coreArea = coreRegion.getArea();
    float cellArea = 0;
    float macroArea = 0;
    float fixedArea = 0;
    float fixedAreaInCore = 0;
    float movableArea = 0;
    int macroCount = 0;
    int cellCount = 0;
    int terminalCount = dbTerminals.size();
    int netCount = dbNets.size();
    int pinCount = dbPins.size();
    int maxNetDegree = INT32_MIN;
    for (Module *curNode : dbNodes)
    {

        if (curNode->isMacro)
        {
            macroArea += curNode->getArea();
            macroCount++;
        }
        else
        {
            cellArea += curNode->getArea();
            cellCount++;
        }
    }
    movableArea = macroArea + cellArea;
    for (Module *curTerminal : dbTerminals)
    {
        fixedArea += curTerminal->getArea();
        fixedAreaInCore += getOverlapArea_2D(coreRegion.ll, coreRegion.ur, curTerminal->getLL_2D(), curTerminal->getUR_2D()); // notice that here we do not consider that some rows might have shorter width than the others
    }
    int pin2 = 0, pin3 = 0, pin10 = 0, pin100 = 0;
    for (Net *curNet : dbNets)
    {
        int curPinCount = curNet->getPinCount();
        if (curPinCount > maxNetDegree)
        {
            maxNetDegree = curPinCount;
        }
        if (curPinCount == 2)
            pin2++;
        else if (curPinCount < 10)
            pin3++;
        else if (curPinCount < 100)
            pin10++;
        else
            pin100++;
    }

    printf("\n<<<< DATABASE SUMMARIES >>>>\n\n");
    printf("         Core region: ");
    coreRegion.Print();
    printf("   Row Height/Number: %.0f / %d (site step %d)\n", commonRowHeight, dbSiteRows.size(), dbSiteRows[0].step);
    printf("           Core Area: %.0f (%g)\n", coreArea, coreArea);
    printf("           Cell Area: %.0f (%.2f%%)\n", cellArea, 100.0 * cellArea / coreArea);
    if (macroCount > 0)
    {
        printf("          Macro Area: %.0f (%.2f%%)\n", macroArea, 100.0 * macroArea / coreArea);
        printf("  Macro/(Macro+Cell): %.2f%%\n", 100.0 * macroArea / (macroArea + cellArea));
    }
    printf("        Movable Area: %.0f (%.2f%%)\n", movableArea, 100.0 * movableArea / coreArea);
    if (terminalCount > 0)
    {
        printf("          Fixed Area: %.0f (%.2f%%)\n", fixedArea, 100.0 * fixedArea / coreArea);
        printf("  Fixed Area in Core: %.0f (%.2f%%)\n", fixedAreaInCore, 100.0 * fixedAreaInCore / coreArea);
    }
    // printf( "   (Macro+Cell)/Core: %.2f%%\n", 100.0*(macroArea+cellArea)/coreArea );
    printf("     Placement Util.: %.2f%% (=move/freeSites)\n", 100.0 * movableArea / (coreArea - fixedAreaInCore));
    printf("        Core Density: %.2f%% (=usedArea/core)\n", 100.0 * (movableArea + fixedAreaInCore) / coreArea);
    // printf( "           Site Area: %.0f (%.0f)", totalSiteArea, coreArea-fixedAreaInCore );
    printf("              Cell #: %d (=%dk)\n", cellCount, (cellCount / 1000));
    printf("            Object #: %d (=%dk) (fixed: %d) (macro: %d)\n", macroCount+cellCount+terminalCount, (macroCount+cellCount+terminalCount) / 1000, terminalCount, macroCount);
    if (macroCount < 20)
    {
        for (Module *curNode : dbNodes)
            if (curNode->isMacro)
                printf(" Macro: %s\n", curNode->name);
    }
    printf("               Net #: %d (=%dk)\n", netCount, netCount / 1000);
    printf("               Max net degree=: %d\n", maxNetDegree);
    printf("                  Pin 2 (%d) 3-10 (%d) 11-100 (%d) 100- (%d)\n", pin2, pin3, pin10, pin100);
    printf("               Pin #: %d\n", pinCount);

    // printf( "               Pin #: %d (in: %d  out: %d  undefined: %d)\n", pinNum, inPinNum, outPinNum, undefPinNum );
    // double HPWL = calcHPWL();
    // printf("     Pin-to-Pin HPWL: %.0f (%g)\n", HPWL, HPWL);
}
