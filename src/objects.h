#ifndef OBJECTS_H
#define OBJECTS_H
#include "global.h"
const int PIN_DIRECTION_OUT = 0;
const int PIN_DIRECTION_IN = 1;
const int PIN_DIRECTION_UNDEFINED = -1;
// todo: use maps for indexing by name
class Net
{
public:
    Net()
    {
        idx = 0;
        netPins.clear();
    }
    Net(int index)
    {
        idx = index;
    }
    int idx;
    vector<Pin *> netPins;
    void addPin(Pin *);
    int getPinCount();
};

class Pin
{
public:
    Pin()
    {
        idx = 0;
    }
    Pin(float x, float y, int index = 0)
        : direction(PIN_DIRECTION_UNDEFINED)
    {
        offset = POS_2D(x, y);
        idx = index;
    }
    int idx;
    Module *module;
    Net *net;
    POS_2D offset;
    int direction; // 0 output  1 input  -1 not-define
    POS_3D getAbsolutePos();
    void setNetId(int);
    void setModuleId(int);
    void setDirection(int);
};

class Module
{
public:
    Module()
    {
        Init();
    }

    Module(string name, float width = 0, float height = 0, bool isFixed = false, bool isNI = false)
    {
        Init();
        name = name;
        width = width;
        height = height;
        area = width * height;
        isFixed = isFixed;
        isNI = isNI; // 2022-05-13 (frank)
        assert(area >= 0);
    }
    int idx;
    Tier *tier;
    POS_3D coor;   // coor for coordinate
    POS_3D center; // coor of center
    string name;
    float width;
    float height;
    float area;
    float orientation;
    bool isMacro;
    // short int m_lefCellId; // 2006-06-22 (donnie) LEF Cell ID
    // short int m_libCellId; // 2006-06-19 (donnie) Liberty Cell ID
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
    void calcCenter()
    {
        center.x = coor.x + (float)0.5 * width; //! be careful of float problems
        center.y = coor.y + (float)0.5 * height;
    }
    void calcArea()
    {
        area = float_mul(width, height);
    }
    string getName() { return name; }
    float getWidth() { return width; }
    float getHeight() { return height; }
    float getX() { return coor.x; }
    float getY() { return coor.y; }
    float getArea() { return area; }
    short int getOrientation() { return orientation; }
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
    }

    SiteRow(double _bottom, double _height, double _step) : bottom(_bottom),
                                                            height(_height),
                                                            step(_step),
                                                            orient(OR_N)
    {
    }

    double bottom; // The bottom y coordinate of this SiteRow of sites
    double height; // The height of this SiteRow of sites
    double step;   // The minimum x step of SiteRow.	by indark
    POS_2D start;  // lower left coor of this row;
    POS_2D end;    // lower right coor of this row;

    vector<double> intervals; //!
    ORIENT orient;            // donnie 2006-04-23  N (0) or S (1)

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
        // return false;
    }
};

class CRect
{
public:
    CRect(double top = 0, double bottom = 0, double left = 0, double right = 0)
    {
        this->left = left;
        this->right = right;
        this->top = top;
        this->bottom = bottom;
    }
    void Print()
    {
        cout << "(" << left << "," << bottom << ")-(" << right << "," << top << ")\n";
    }
    double left, right, top, bottom;
};

class Tier // one 2D plane(or one 2D chip)
{
public:
    Tier()
    {
        ll.SetZero();
        ur.SetZero();
        layerNumber = -1;
        modules.clear();
        siteRows.clear();
    }
    POS_2D ll; // ll: lower left
    POS_2D ur; // ur: upper right
    int layerNumber;
    vector<Module *> modules;
    vector<SiteRow *> siteRows;
    vector<Module *> terminals; // terminals: module that can't be moved
};
#endif