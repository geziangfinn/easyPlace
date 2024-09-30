#include "qplace.h"
#include "parser.h"
#include "arghandler.h"
#include "eplace.h"
#include "opt.hpp"
#include "nesterov.hpp"
#include "legalizer.h"
#include "detailed.h"
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

            string outputFilePath;
            if (!gArg.GetString("outputPath", &outputFilePath))
            {
                outputFilePath = "./";
            }
            outputFilePath = outputFilePath + "/" + benchmarkName + "/";
            gArg.Override("outputPath", outputFilePath);

            string plotPath = outputFilePath + "Graphs/";
            gArg.Override("plotPath", plotPath);

            string cmd = "rm -rf " + outputFilePath;
            system(cmd.c_str());

            cmd = "mkdir -p " + outputFilePath;
            system(cmd.c_str());

            cmd = "rm -rf " + plotPath;
            system(cmd.c_str());

            cmd = "mkdir -p " + plotPath;
            system(cmd.c_str());

            cout << "    Plot path: " << plotPath << endl;
        }
        parser.ReadFile(argv[2], *placedb);
    }
    placedb->showDBInfo();
    string plPath;
    if (gArg.GetString("loadpl", &plPath))
    {
        //! modules will be moved to center in QP, so if QP is not skipped, loading module locations from an existing pl file is meaningless
        parser.ReadPLFile(plPath, *placedb, false);
    }

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
        cout << "mGP started!\n";

        time_start(&mGPTime);
        opt->opt();
        time_end(&mGPTime);

        cout << "mGP finished!\n";
        cout << "Final HPWL: " << int(placedb->calcHPWL()) << endl;
        cout << "mGP time: " << mGPTime << endl;
        PLOTTING::plotCurrentPlacement("mGP result", placedb);
    }

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
        cout << "mLG finished. HPWL after mLG: " << int(placedb->calcHPWL()) << endl;
        cout << "mLG time: " << mLGTime << endl;
        // exit(0);

        if (!gArg.CheckExist("nocGP"))
        {
            eplacer->switch2FillerOnly();
            cout << "filler placement started!\n";

            time_start(&FILLERONLYtime);
            opt->opt();
            time_end(&FILLERONLYtime);

            cout << "filler placement finished!\n";
            cout << "FILLERONLY time: " << mGPTime << endl;
            PLOTTING::plotCurrentPlacement("FILLERONLY result", placedb);

            eplacer->switch2cGP();
            cout << "cGP started!\n";

            time_start(&cGPTime);
            opt->opt();
            time_end(&cGPTime);

            cout << "cGP finished!\n";
            cout << "cGP Final HPWL: " << int(placedb->calcHPWL()) << endl;
            cout << "cGP time: " << mGPTime << endl;
            PLOTTING::plotCurrentPlacement("cGP result", placedb);
        }
    }

    placedb->outputBookShelf("eGP",false); // output, files will be used for legalizers such as ntuplace3

    if (!gArg.CheckExist("noLegal"))
    {
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

            string cmd = legalizerPath + "/ntuplace3" + " -aux " + outputAUXPath + " -loadpl " + outputPLPath + " -noglobal" + " -out " + outputPath + benchmarkName + "-ntu" + " > " + outputPath + "ntuplace3-log.txt";

            cout << RED << "Running legalizer and detailed placer: " << cmd << RESET << endl;
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
            PLOTTING::plotCurrentPlacement("Cell legalized result", placedb);

            placedb->outputBookShelf("eLG",true);
        }
        else
        {
            cout<<"Legalization not done!!!\n";
            exit(0);
        }
    }

    if (gArg.CheckExist("internalDP"))// currently only works after internal legalization
    {
        cout << "Calling internal detailed placement: " << endl;
        cout << "HPWL before detailed placement: " << int(placedb->calcHPWL()) << endl;

        DetailedPlacer *detailedPlacer = new DetailedPlacer(placedb);
        detailedPlacer->detailedPlacement();

        cout << "HPWL after detailed placement: " << int(placedb->calcHPWL()) << endl;
        PLOTTING::plotCurrentPlacement("Detailed placement result", placedb);

        placedb->outputBookShelf("eDP",true);
    }
}