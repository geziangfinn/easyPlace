#include "parser.h"
#include "arghandler.h"
#include "qplace.h"
#include "eplace.h"
// #include "plot.h"
#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
    PlaceDB *placedb = new PlaceDB();
    Plotter *plotter = new Plotter(placedb);
    gArg.Init(argc, argv);

    if (argc < 2)
    {
        return 0;
    }
    if (strcmp(argv[1] + 1, "aux") == 0) // -aux, argv[1]=='-'
    {
        // bookshelf
        printf("Use BOOKSHELF placement format\n");

        string filename = argv[2];
        string::size_type pos = filename.rfind("/");
        string benchmarkName;
        if (pos != string::npos)
        {
            printf("    Path = %s\n", filename.substr(0, pos + 1).c_str());
            gArg.Override("path", filename.substr(0, pos + 1));

            int length = filename.length();

            benchmarkName = filename.substr(pos + 1, length - pos);

            int len = benchmarkName.length();
            if (benchmarkName.substr(len - 4, 4) == ".aux")
            {
                benchmarkName = benchmarkName.erase(len - 4, 4);
            }
            gArg.Override("benchmarkName", benchmarkName);
            cout << "    Benchmark: " << benchmarkName << endl;

            string plotPath;
            gArg.GetString("plotPath", &plotPath);

            plotPath += "/" + benchmarkName + "/";

            string cmd = "mkdir -p " + plotPath;
            system(cmd.c_str());

            plotter->setPlotPath(plotPath);
            gArg.Override("plotPath", plotPath);

            cout << "    Plot path: " << plotPath << endl;
        }
        BookshelfParser parser;
        parser.ReadFile(argv[2], *placedb);
    }
    placedb->showDBInfo();
    QPPlacer *qpplacer = new QPPlacer(placedb);
    qpplacer->setPlotter(plotter);
    qpplacer->quadraticPlacement();

    double initializationTime;
    double iterationTime;
    EPlacer_2D *eplacer = new EPlacer_2D(placedb);
    eplacer->setPlotter(plotter);
    eplacer->setTargetDensity(0.9);

    time_start(&initializationTime);
    eplacer->initialization();
    time_end(&initializationTime);

    time_start(&iterationTime);
    eplacer->binNodeDensityUpdate();
    eplacer->wirelengthGradientUpdate();
    eplacer->densityGradientUpdate();
    eplacer->totalGradientUpdate(1.0);
    time_end(&iterationTime);

    cout << "Initialization time: " << initializationTime << endl;
    cout << "Iteration time: " << iterationTime << endl;
}