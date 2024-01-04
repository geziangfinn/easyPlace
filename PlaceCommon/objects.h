#ifndef OBJECTS_H
#define OBJECTS_H
#include "global.h"
const int PIN_DIRECTION_OUT = 0;
const int PIN_DIRECTION_IN = 1;
const int PIN_DIRECTION_UNDEFINED = -1;
// todo: use maps for indexing by name
class Module;
class SiteRow;
class Row;
class Pin;
class Net;
class Tier;
class CRect;
class PlaceDB;

class Net
{
public:
    Net()
    {
        idx = 0;
        clearBoundPins();
    }
    Net(int index)
    {
        idx = index;
        netPins.clear();
        clearBoundPins();
    }
    int idx;
    vector<Pin *> netPins;
    //! boundPin pointers, used for quadratic placement utilizing bound2bound net model
    Pin *boundPinXmin;
    Pin *boundPinXmax;
    Pin *boundPinYmin;
    Pin *boundPinYmax;

    void addPin(Pin *);
    int getPinCount();
    void allocateMemoryForPin(int);
    double calcNetHPWL();
    double calcBoundPin();
    void clearBoundPins();
};

class Pin
{
public:
    Pin()
    {
        idx = -1;
        module = NULL;
        net = NULL;
        offset.SetZero();
        direction = -1;
    }
    Pin(Module *masterModule, Net *masterNet, float x, float y)
        : direction(PIN_DIRECTION_UNDEFINED)
    {
        offset = POS_2D(x, y);
        setModule(masterModule);
        setNet(masterNet);
    }
    int idx;
    Module *module;
    Net *net;
    POS_2D offset;
    int direction; // 0 output  1 input  -1 not-define
    POS_3D getAbsolutePos();
    void setId(int);
    void setNet(Net *);
    void setModule(Module *);
    void setDirection(int);
};

class Module
{
public:
    friend class PlaceDB;
    Module()
    {
        Init();
    }

    Module(int index, string name, float width = 0, float height = 0, bool isFixed = false, bool isNI = false)
    {
        Init();
        name = name;
        width = width;
        height = height;
        area = width * height;
        isMacro = false;
        isFixed = isFixed;
        isNI = isNI; // 2022-05-13 (frank)
        assert(area >= 0);
    }
    int idx;
    Tier *tier;
    string name;
    float width;
    float height;
    float area;
    float orientation;
    bool isMacro;
    bool isFixed;
    bool isNI;
    bool isFiller;
    vector<Pin *> modulePins;
    vector<Net *> nets;
    void Init()
    {
        idx = -1;
        coor.SetZero();
        center.SetZero();
        width = 0;
        height = 0;
        area = 0;
        orientation = 0;
        isFixed = false;
        isNI = false;
        tier = NULL;
    }
    float calcArea()
    {
        area = float_mul(width, height);
        return area;
    }
    void addPin(Pin *);
    string getName() { return name; }
    float getWidth() { return width; }
    float getHeight() { return height; }
    POS_3D getLocation() { return coor; }
    POS_3D getCenter(){return center;}
    float getArea() { return area; }
    short int getOrientation() { return orientation; }
    void setOrientation(int);

private:
    //! these 2 functions should only be called in db->setModuleCenter/Location!!!
    POS_3D coor;   // coor for coordinate
    POS_3D center; // coor of center, be aware that center should be recalculated every time the module is moved, or before HPWL calculation
    void setLocation_2D(float, float, float = 0);
    void setCenter_2D(float, float, float = 0);
};

class Row
{ // an abstract row
    Row()
    {
        bottom = 0;
        height = 0;
        step = 0;
        start.SetZero();
        end.SetZero();
    }

    Row(double _bottom, double _height, double _step) : bottom(_bottom),
                                                        height(_height),
                                                        step(_step)
    {
    }
    double bottom;
    double height;
    double step;
    POS_2D start;
    POS_2D end;
};

class SiteRow // a place row
{
public:
    SiteRow()
    {
        intervals.clear();
        bottom = 0;
        height = 0;
        step = 0;
        start.SetZero();
        end.SetZero();
        orientation = OR_N;
    }

    SiteRow(double _bottom, double _height, double _step) : bottom(_bottom),
                                                            height(_height),
                                                            step(_step),
                                                            orientation(OR_N)
    {
    }

    double bottom;         // The bottom y coordinate of this SiteRow of sites
    double height;         // The height of this SiteRow of sites
    double step;           // The minimum x step of SiteRow.	by indark
    POS_2D start;          // lower left coor of this row;
    POS_2D end;            // lower right coor of this row;
    ORIENT orientation;    // donnie 2006-04-23  N (0) or S (1)
    vector<Row> intervals; //!
    // double site_spacing;// site spacing in bookshelf format, equals to site width

    bool Lesser(SiteRow &r1, SiteRow &r2)
    {
        return (r1.bottom < r2.bottom);
    }
    bool Greater(SiteRow &r1, SiteRow &r2)
    {
        return (r1.bottom > r2.bottom);
    }

    bool isInside(const double &x, const double width)
    {
        // vector<double>::const_iterator ite;
        // for (ite = interval.begin(); ite != interval.end(); ite += 2)
        // {
        //     if (*ite > x)
        //         return false;
        //     // cout << "  sites(" << *ite << " " << *(ite+1) << ") ";
        //     if (*ite <= x && *(ite + 1) >= x)
        //         return true;
        // }
        return false;
    }
    friend inline std::ostream &operator<<(std::ostream &os, const SiteRow &row)
    {
        os << "Row start: " << row.start << " Row end:" << row.end;
        return os;
    }
};

class CRect
{
public:
    CRect()
    {
        Init();
    }
    void Print()
    {
        cout << "lower left: " << ll << " upper right: " << ur << "\n";
    }
    void Init()
    {
        ll.SetZero();
        ur.SetZero();
    }
    POS_2D ll; // ll: lower left point
    POS_2D ur; // ur: upper right point
};

class Tier // one 2D plane(or one 2D chip)
{
public:
    Tier()
    {
        coreRegion.Init();
        layerNumber = -1;
        modules.clear();
        siteRows.clear();
    }
    CRect coreRegion;
    int layerNumber; // which layer in a 3dic
    double rowHeight;
    vector<Module *> modules;
    vector<SiteRow *> siteRows;
    vector<Module *> terminals; // terminals: module that can't be moved
};
#endif