#include "placedb.h"

void PlaceDB::setCoreRegion()
{
    float bottom = dbSiteRows.front().bottom;
    float top = dbSiteRows.back().bottom + dbSiteRows.back().height;
    float left = dbSiteRows.front().start.x;
    float right = dbSiteRows.front().end.x;

    for (SiteRow curRow : dbSiteRows)
    {
        left = min(left, curRow.start.x);
        right = max(right, curRow.end.x);
        // printf( "right= %g\n", m_coreRgn.right );
    }

    coreRegion.ll = POS_2D(left, bottom);
    coreRegion.ur = POS_2D(right, top);

    cout << "Set core region from site info: ";
    coreRegion.Print();

    // Add fixed blocks to fill "non-sites"
    // int number = CreateDummyFixedBlock();
    // cout << "Add " << number << " fixed blocks\n";

    // number = CreateDummyBlock();
    // cout << "Add " << number << " dummy blocks\n";
}

void PlaceDB::init_tiers()
{
}

Module *PlaceDB::addNode(int index, string name, float width, float height)
{
    Module *node = new Module(index, name, width, height);
    assert(index<dbNodes.size());
    assert(commonRowHeight > EPS); //! potential float precision problem!!
    if (float_greater(height, commonRowHeight))
    {
        node->isMacro = true;
    }
    else
    {
        printf("Cell %s height ERROR: %f\n", name, height);
    }
    dbNodes[index] = node; // memory was previously allocated
    return node;
}

Module *PlaceDB::addTerminal(int index, string name, float width, float height, bool isFixed, bool isNI)
{
    Module *terminal = new Module(index, name, width, height, isFixed, isNI);
    assert(index<dbTerminals.size());
    dbTerminals[index] = terminal;
    return terminal;
}

void PlaceDB::addNet(Net * net)
{
    assert(net->idx<dbNets.size());
    dbNets[net->idx] = net;
}

int PlaceDB::addPin(Module *masterModule, Net* masterNet, float xOffset, float yOffset)
{
    dbPins.push_back(new Pin(masterModule,masterNet, xOffset, yOffset));
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

void PlaceDB::allocatePinMemory(int n) // difference between resize and reserve: we can use index to index an element in a vector after allocation and before the element was instantiated
{
    dbPins.reserve(n);//! reserve instead of resize
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
