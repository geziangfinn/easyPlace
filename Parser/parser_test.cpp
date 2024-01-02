#include "parser.h"
#include "arghandler.h"
#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
    PlaceDB placedb;

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
        if (pos != string::npos)
        {
            printf("    Path = %s\n", filename.substr(0, pos + 1).c_str());
            gArg.Override("path", filename.substr(0, pos + 1));
        }

        BookshelfParser parser;
        parser.ReadFile(argv[2], placedb);
    }
}