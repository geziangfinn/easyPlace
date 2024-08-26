#include "qplace.h"
#include "parser.h"
#include "arghandler.h"
#include "eplace.h"
#include "opt.hpp"
#include "nesterov.hpp"
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
            if (!gArg.GetString("outputPath", &plotPath))
            {
                plotPath = "./";
            }

            plotPath += "/" + benchmarkName + "/Graphs/";

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
    QPPlacer *qpplacer = new QPPlacer(placedb);
    if (!gArg.CheckExist("noQP"))
    {
        qpplacer->quadraticPlacement();
    }

    if (gArg.CheckExist("addNoise"))
    {
        placedb->addNoise(); // the noise range is [-avgbinStep,avgbinStep]
    }

    double mGPTime;
    double mLGTime;
    double FILLERONLYtime;
    double cGPTime;

    EPlacer_2D *eplacer = new EPlacer_2D(placedb);

    float targetDensity;
    if (!gArg.GetFloat("targetDensity", &targetDensity))
    {
        targetDensity = 1.0;
    }

    eplacer->setTargetDensity(targetDensity);
    eplacer->initialization();

    FirstOrderOptimizer<VECTOR_3D> *opt = new EplaceNesterovOpt<VECTOR_3D>(eplacer);

    if (!gArg.CheckExist("nomGP"))
    {
        time_start(&mGPTime);
        opt->opt();
        time_end(&mGPTime);
    }

    cout << "mGP finished!\n";
    cout << "Final HPWL: " << int(placedb->calcHPWL()) << endl;
    cout << "mGP time: " << mGPTime << endl;
    PLOTTING::plotCurrentPlacement("mGP result", placedb);

    ///////////////////////////////////////////////////
    // legalization and detailed placement
    ///////////////////////////////////////////////////

    if (placedb->dbMacroCount > 0 && !gArg.CheckExist("nomLG"))
    {
        SAMacroLegalizer *macroLegalizer = new SAMacroLegalizer(placedb);
        macroLegalizer->setTargetDensity(targetDensity);
        cout << "Start mLG, total macro count: " << placedb->dbMacroCount << endl;
        time_start(&mLGTime);
        macroLegalizer->legalization();
        time_end(&mLGTime);

        PLOTTING::plotCurrentPlacement("mLG result", placedb);
        cout << "HPWL after mLG: " << int(placedb->calcHPWL()) << endl;
        cout << "mLG time: " << mLGTime << endl;
        // exit(0);

        if (!gArg.CheckExist("nocGP"))
        {
            eplacer->switch2FillerOnly();

            time_start(&FILLERONLYtime);
            opt->opt();
            time_end(&FILLERONLYtime);

            cout << "filler placement finished!\n";
            cout << "FILLERONLY time: " << mGPTime << endl;
            PLOTTING::plotCurrentPlacement("FILLERONLY result", placedb);

            eplacer->switch2cGP();

            time_start(&cGPTime);
            opt->opt();
            time_end(&cGPTime);

            cout << "cGP finished!\n";
            cout << "cGP Final HPWL: " << int(placedb->calcHPWL()) << endl;
            cout << "cGP time: " << mGPTime << endl;
            PLOTTING::plotCurrentPlacement("cGP result", placedb);
        }
    }

    cout << "Output bookshelf:\n";
    placedb->outputBookShelf();

    if (gArg.CheckExist("noLegal"))
    {
        return;
    }

    string legalizerPath;
    if (gArg.GetString("legalizerPath", &legalizerPath))
    {
        string outputAUXPath;
        string outputPLPath;
        string outputPath;
        string benchmarkName;

        gArg.GetString("outputAUX", &outputAUXPath);
        gArg.GetString("outputPL", &outputPLPath);
        gArg.GetString("outputPath", &outputPath);
        gArg.GetString("benchmarkName", &benchmarkName);
        string cmd = legalizerPath + "/ntuplace3" + " -aux " + outputAUXPath + " -loadpl " + outputPLPath + " -noglobal" + " -out " + outputPath + "/" + benchmarkName + " > " + outputPath + "/Results.txt";
        cout << RED << "Running legalizer: " << cmd << RESET << endl;
        system(cmd.c_str());
    }
    else if (gArg.GetString("internalLegal", &legalizerPath))
    {
        // for std cell design only, since we don't have macro legalizer for now
        cout << "Calling internal legalizer: " << endl;
        cout << "Global HPWL: " << int(placedb->calcHPWL()) << endl;
        AbacusLegalizer *legalizer = new AbacusLegalizer(placedb);
        legalizer->legalization();
        cout << "Legal HPWL: " << int(placedb->calcHPWL()) << endl;
        placedb->outputBookShelf();
        PLOTTING::plotCurrentPlacement("Cell legalized result", placedb);
    }
}