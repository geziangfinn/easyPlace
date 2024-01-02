#ifndef PARSER_H
#define PARSER_H
#include "placedb.h"
#include "global.h"
class BookshelfParser
{
public:
    int ReadFile(string file, PlaceDB &db);
    int ReadPLFile(string file, PlaceDB &db);

private:
    int ReadSCLFile(string file, PlaceDB &db);
    int ReadNodesFile(string file, PlaceDB &db);
    int ReadNetsFile(string file, PlaceDB &db);

};

class LEFDEFParser
{
    // maybe in the future...
};

#endif