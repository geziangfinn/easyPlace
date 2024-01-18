#include "placedb.h"
#include "global.h"

void PlaceDB::setCoreRegion()
{
    float bottom = dbSiteRows.front().bottom;
    float top = dbSiteRows.back().bottom + dbSiteRows.back().height;
    float left = dbSiteRows.front().start.x;
    float right = dbSiteRows.front().end.x;
    
    totalRowArea=0;
    float curRowArea=0;
    for (SiteRow curRow : dbSiteRows)
    {
        left = min(left, curRow.start.x);
        right = max(right, curRow.end.x);
        // printf( "right= %g\n", m_coreRgn.right );
        curRowArea=(curRow.end.x-curRow.start.x)*curRow.height;
        totalRowArea+=curRowArea;
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

double PlaceDB::calcHPWL() //! parallel this?
{
    double HPWL = 0;
    for (Net *curNet : dbNets)
    {
        HPWL += curNet->calcNetHPWL();
    }
    return HPWL;
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
        //gmin.z = min(gmin.z, curTerminal->pmin.z);

        chipRegion.ur.x = max(chipRegion.ur.x, curTerminal->getUR_2D().x);
        chipRegion.ur.y = max(chipRegion.ur.y, curTerminal->getUR_2D().y);
        //gmax.z = max(gmax.z, curTerminal->pmax.z);
    }
}
