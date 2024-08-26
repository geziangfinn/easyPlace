#include "parser.h"
#include "arghandler.h"
#include "qplace.h"
#include "eplace.h"
// #include "optimizer.h"
#include "legalizer.h"
#include "plot.h"
#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
    BookshelfParser parser;
    PlaceDB *placedb = new PlaceDB();
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
            if (!gArg.GetString("plotPath", &plotPath))
            {
                plotPath = "./";
            }

            plotPath += "/" + benchmarkName + "/";

            string cmd = "rm -rf " + plotPath;
            system(cmd.c_str());
            cmd = "mkdir -p " + plotPath;
            system(cmd.c_str());

            gArg.Override("plotPath", plotPath);

            cout << "    Plot path: " << plotPath << endl;
        }

        parser.ReadFile(argv[2], *placedb);
    }
    placedb->showDBInfo();

    string outputPath;
    string benchmarkName;
    placedb->outputBookShelf();
    gArg.GetString("outputPath", &outputPath);
    gArg.GetString("benchmarkName", &benchmarkName);

    parser.ReadPLFile(outputPath + "/" + benchmarkName + "-global.pl", *placedb, false);
    PLOTTING::plotCurrentPlacement("global_RESULT",placedb);
    AbacusLegalizer legalizer(placedb);
    legalizer.legalization();
    PLOTTING::plotCurrentPlacement("LEGALIZED_RESULT",placedb);
    placedb->outputBookShelf();
}