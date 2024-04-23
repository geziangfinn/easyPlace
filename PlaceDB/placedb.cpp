#include <random>

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
    // assert(width != 0 && height != 0);
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

POS_3D PlaceDB::getValidModuleCenter_2D(Module *module, float x, float y)
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
    POS_3D validPosition;
    validPosition.x = x;
    validPosition.y = y;
    return validPosition;
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

void PlaceDB::randomPlacment()
{
    for (Module *curModule : dbNodes)
    {
        setModuleLocation_2D_random(curModule);
    }
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

double PlaceDB::calcLSE_Wirelength_2D(VECTOR_2D invertedGamma)
{
    double LSE = 0;
    for (Net *curNet : dbNets)
    {
        LSE += curNet->calcWirelengthLSE_2D(invertedGamma);
    }
    return LSE;
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
    printf("            Object #: %d (=%dk) (fixed: %d) (macro: %d)\n", macroCount + cellCount + terminalCount, (macroCount + cellCount + terminalCount) / 1000, terminalCount, macroCount);
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

void PlaceDB::outputBookShelf()
{
    string outputFilePath;
    if (!gArg.GetString("outputPath", &outputFilePath))
    {
        outputFilePath = "./";
    }

    string benchmarkName;
    gArg.GetString("benchmarkName", &benchmarkName);

    outputFilePath = outputFilePath + "/" + benchmarkName + "/";

    string cmd = "mkdir -p " + outputFilePath;
    system(cmd.c_str());
    cout << cmd << endl;

    gArg.Override("outputPath", outputFilePath);

    outputAUX();
    outputNodes();
    outputNets();
    outputPL();
    outputSCL();
}

void PlaceDB::outputAUX()
{
    string outputFilePath;
    gArg.GetString("outputPath", &outputFilePath);

    string benchmarkName;
    gArg.GetString("benchmarkName", &benchmarkName);

    outputFilePath += benchmarkName;
    outputFilePath += "-out.aux";

    cout << "Output AUX file:" << outputFilePath << endl;

    gArg.Override("outputAUX", outputFilePath);

    ofstream out(outputFilePath);
    if (!out)
    {
        cerr << "Cannot open output file\n";
        return;
    }

    out << "RowBasedPlacement : "
        << benchmarkName << "-out.nodes "
        << benchmarkName << "-out.nets "
        << benchmarkName << "-out.wts "
        << benchmarkName << "-out.pl "
        << benchmarkName << "-out.scl \n\n";
}

void PlaceDB::outputNodes()
{

    string outputFilePath;
    gArg.GetString("outputPath", &outputFilePath);

    string benchmarkName;
    gArg.GetString("benchmarkName", &benchmarkName);

    outputFilePath += benchmarkName;
    outputFilePath += "-out.nodes";

    cout << "Output Nodes file:" << outputFilePath << endl;

    FILE *out;
    out = fopen(outputFilePath.c_str(), "w");
    if (!out)
    {
        cerr << "Cannot open output file\n";
        return;
    }

    fprintf(out, "UCLA nodes 1.0\n\n");
    fprintf(out, "NumNodes : %d\n", moduleCount);
    fprintf(out, "NumTerminals : %d\n\n", dbTerminals.size());

    // non-terminal
    for (Module *curNode : dbNodes)
    {
        double w = curNode->getWidth();
        double h = curNode->getHeight();

        fprintf(out, " %30s %10.0f %10.0f\n",
                curNode->name.c_str(),
                w,
                h);
    }

    // terminal
    for (Module *curTerminal : dbTerminals)
    {
        double w = curTerminal->getWidth();
        double h = curTerminal->getHeight();

        if (curTerminal->isNI)
        {
            fprintf(out, " %10s %10.0f %10.0f terminal_NI\n",
                    curTerminal->name.c_str(),
                    w,
                    h);
        }
        else
        {
            fprintf(out, " %10s %10.0f %10.0f terminal\n",
                    curTerminal->name.c_str(),
                    w,
                    h);
        }
    }
    fprintf(out, "\n\n");
    fclose(out);
}

void PlaceDB::outputPL()
{
    string outputFilePath;
    gArg.GetString("outputPath", &outputFilePath);

    string benchmarkName;
    gArg.GetString("benchmarkName", &benchmarkName);

    outputFilePath += benchmarkName;
    outputFilePath += "-out.pl";
    gArg.Override("outputPL", outputFilePath);

    cout << "Output PL file:" << outputFilePath << endl;

    // out "pl"
    FILE *out = fopen(outputFilePath.c_str(), "w");
    fprintf(out, "UCLA pl 1.0\n\n");

    char *orientN = "N";

    for (Module *curNode : dbNodes)
    {
        fprintf(out, "%s\t%.0f\t%.0f : %s",
                curNode->name.c_str(),
                curNode->getLL_2D().x,
                curNode->getLL_2D().y,
                orientN);
        fprintf(out, "\n");
    }
    for (Module *curTerminal : dbTerminals)
    {
        fprintf(out, "%s\t%.0f\t%.0f : %s",
                curTerminal->name.c_str(),
                curTerminal->getLL_2D().x,
                curTerminal->getLL_2D().y,
                orientN);

        if (curTerminal->isNI)
            fprintf(out, " /FIXED_NI\n");
        else
            fprintf(out, " /FIXED\n");
    }
    fprintf(out, "\n\n");

    fclose(out);
}

void PlaceDB::outputNets()
{
    string outputFilePath;
    gArg.GetString("outputPath", &outputFilePath);

    string benchmarkName;
    gArg.GetString("benchmarkName", &benchmarkName);

    outputFilePath += benchmarkName;
    outputFilePath += "-out.nets";

    cout << "Output Nets file:" << outputFilePath << endl;

    FILE *out;
    out = fopen(outputFilePath.c_str(), "w");
    if (!out)
    {
        cerr << "Cannot open output file\n";
        return;
    }

    fprintf(out, "UCLA nets 1.0\n\n");
    fprintf(out, "NumNets : %d\n", dbNets.size());
    fprintf(out, "NumPins : %d\n", dbPins.size());
    for (Net *curNet : dbNets)
    {
        fprintf(out, "NetDegree : %d\n", curNet->netPins.size());
        for (Pin *curPin : curNet->netPins)
        {
            fprintf(out, " %10s B : %.2f %.2f\n",

                    curPin->module->name.c_str(),
                    curPin->offset.x,
                    curPin->offset.y);
        }
    }
    fprintf(out, "\n");
    fclose(out);
}

void PlaceDB::outputSCL()
{
    string outputFilePath;
    gArg.GetString("outputPath", &outputFilePath);

    string benchmarkName;
    gArg.GetString("benchmarkName", &benchmarkName);

    outputFilePath += benchmarkName;
    outputFilePath += "-out.scl";

    cout << "Output SCL file:" << outputFilePath << endl;

    FILE *out;
    out = fopen(outputFilePath.c_str(), "w");
    if (!out)
    {
        cerr << "Cannot open output file\n";
        return;
    }

    fprintf(out, "UCLA scl 1.0\n");
    fprintf(out, "# Created       :\n");
    fprintf(out, "# User          :\n\n");
    fprintf(out, "NumRows : %d\n\n", dbSiteRows.size());

    char *ori[2] = {"N", "Y"};
    for (SiteRow curRow : dbSiteRows)
    {
        double step = curRow.step;
        if (step == 0)
            step = 1.0;
        fprintf(out, "CoreRow Horizontal\n");
        fprintf(out, " Coordinate    : %8.0f\n", curRow.bottom);
        fprintf(out, " Height        : %8.0f\n", curRow.height);
        fprintf(out, " Sitewidth     : %8.0f\n", step);
        fprintf(out, " Sitespacing   : %8.0f\n", step);
        fprintf(out, " Siteorient    : 1\n"); //%s\n", ori[i % 2] );
        fprintf(out, " Sitesymmetry  : 1\n");
        fprintf(out, " SubrowOrigin  : %8.0f Numsites : %8.0f\n",
                curRow.start.x, (curRow.end.x - curRow.start.x) / curRow.step);

        fprintf(out, "End\n");
    }
    fprintf(out, "\n");
    fclose(out);
}

void PlaceDB::plotCurrentPlacement(string imageName)
{
    string plotPath;
    if (!gArg.GetString("plotPath", &plotPath))
    {
        plotPath = "./";
    }

    float chipRegionWidth = chipRegion.ur.x - chipRegion.ll.x;
    float chipRegionHeight = chipRegion.ur.y - chipRegion.ll.y;

    int minImgaeLength = 1000;

    int imageHeight;
    int imageWidth;

    float opacity = 0.7;
    int xMargin = 30, yMargin = 30;

    if (chipRegionWidth < chipRegionHeight)
    {
        imageHeight = 1.0 * chipRegionHeight / (chipRegionWidth / minImgaeLength);
        imageWidth = minImgaeLength;
    }
    else
    {
        imageWidth = 1.0 * chipRegionWidth / (chipRegionHeight / minImgaeLength);
        imageHeight = minImgaeLength;
    }

    CImg<unsigned char> img(imageWidth + 2 * xMargin, imageHeight + 2 * yMargin, 1, 3, 255);

    float unitX = imageWidth / chipRegionWidth,
          unitY = imageHeight / chipRegionHeight;

    for (Module *curTerminal : dbTerminals)
    {
        assert(curTerminal);
        // ignore pin's location
        if (curTerminal->isNI)
        {
            continue;
        }
        int x1 = getX(chipRegion.ll.x, curTerminal->getLL_2D().x, unitX) + xMargin;
        int x2 = getX(chipRegion.ll.x, curTerminal->getUR_2D().x, unitX) + xMargin;
        int y1 = getY(chipRegionHeight, chipRegion.ll.y, curTerminal->getLL_2D().y, unitY) + yMargin;
        int y2 = getY(chipRegionHeight, chipRegion.ll.y, curTerminal->getUR_2D().y, unitY) + yMargin;
        img.draw_rectangle(x1, y1, x2, y2, Blue, opacity);
    }

    for (Module *curNode : dbNodes)
    {
        assert(curNode);
        int x1 = getX(chipRegion.ll.x, curNode->getLL_2D().x, unitX) + xMargin;
        int x2 = getX(chipRegion.ll.x, curNode->getUR_2D().x, unitX) + xMargin;
        int y1 = getY(chipRegionHeight, chipRegion.ll.y, curNode->getLL_2D().y, unitY) + yMargin;
        int y2 = getY(chipRegionHeight, chipRegion.ll.y, curNode->getUR_2D().y, unitY) + yMargin;
        if (curNode->isMacro)
        {
            img.draw_rectangle(x1, y1, x2, y2, Orange, opacity);
        }
        else
        {
            img.draw_rectangle(x1, y1, x2, y2, Red, opacity);
        }
    }

    img.draw_text(50, 50, imageName.c_str(), Black, NULL, 1, 30);
    img.save_bmp(string(plotPath + imageName + string(".bmp")).c_str());
    cout << "INFO: BMP HAS BEEN SAVED: " << imageName + string(".bmp") << endl;
}

void PlaceDB::addNoise()
{
    static std::mt19937 gen(0); // 0:random seed, fixed for debugging

    for (auto node : dbNodes)
    {
        VECTOR_2D range;
        range.x = 0.5 * 0.025 * node->getWidth();
        range.y = 0.5 * 0.025 * node->getHeight();

        std::uniform_real_distribution<float> disx(-range.x, range.x);
        std::uniform_real_distribution<float> disy(-range.y, range.y);
        
        POS_3D pos = node->center;
        pos.x += disx(gen);
        pos.y += disy(gen);
        node->setCenter_2D(pos.x, pos.y);
    }
}