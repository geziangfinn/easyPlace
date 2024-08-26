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
class Interval;

class Net
{
public:
    Net()
    {
        init();
    }
    Net(int index)
    {
        init();
        idx = index;
    }
    int idx;
    vector<Pin *> netPins;
    //! boundPin pointers, used for quadratic placement utilizing bound2bound net model
    Pin *boundPinXmin;
    Pin *boundPinXmax;
    Pin *boundPinYmin;
    Pin *boundPinYmax;
    Pin *boundPinZmin; // for 3D
    Pin *boundPinZmax; // for 3D

    VECTOR_3D numeratorMax_WA;
    VECTOR_3D denominatorMax_WA;
    VECTOR_3D numeratorMin_WA;
    VECTOR_3D denominatorMin_WA;

    VECTOR_3D sumMax_LSE;
    VECTOR_3D sumMin_LSE;

    void init()
    {
        idx = 0;
        netPins.clear();
        clearBoundPins();
        numeratorMax_WA.SetZero();
        denominatorMin_WA.SetZero();
        numeratorMax_WA.SetZero();
        denominatorMin_WA.SetZero();
    }
    void addPin(Pin *);
    int getPinCount();
    void allocateMemoryForPin(int);
    double calcNetHPWL();
    double calcBoundPin();
    void clearBoundPins();
    double calcWirelengthWA_2D(VECTOR_2D);
    double calcWirelengthLSE_2D(VECTOR_2D);
    VECTOR_2D getWirelengthGradientWA_2D(VECTOR_2D, Pin *);
    VECTOR_2D getWirelengthGradientLSE_2D(VECTOR_2D, Pin *);
};

class Pin
{
public:
    Pin()
    {
        init();
    }
    Pin(Module *masterModule, Net *masterNet, float x, float y)
        : direction(PIN_DIRECTION_UNDEFINED)
    {
        init();
        offset = POS_2D(x, y);
        setModule(masterModule);
        setNet(masterNet);
    }
    void init()
    {
        idx = -1;
        module = NULL;
        net = NULL;
        offset.SetZero();
        absolutePos.SetZero();
        direction = -1;
        eMin_WA.SetZero();
        eMax_WA.SetZero();
        expZeroFlgMax_WA.SetZero();
        expZeroFlgMin_WA.SetZero();
    }
    int idx;
    Module *module;
    Net *net;
    POS_2D offset;
    POS_3D absolutePos;
    int direction; // 0 output  1 input  -1 not-define
    POS_3D getAbsolutePos();
    POS_3D fetchAbsolutePos(); // currently for mLG only

    VECTOR_3D eMin_WA;               // e^[(Xmin-Xi)/gamma] in WA model (X/Y/Z)
    VECTOR_3D eMax_WA;               // e^[(Xi-Xmax)/gamma] in WA model (X/Y/Z)
    VECTOR_3D_BOOL expZeroFlgMax_WA; // indicate if (Xi-Xmax)/gamma is too small that e^[(Xi-Xmax)/gamma] == 0
    VECTOR_3D_BOOL expZeroFlgMin_WA; // similar

    VECTOR_3D eMin_LSE;               // e^[(Xmin-Xi)/gamma] in LSE model
    VECTOR_3D eMax_LSE;               // e^[(Xi-Xmax)/gamma] in LSE model
    VECTOR_3D_BOOL expZeroFlgMax_LSE; // indicate if (Xi-Xmax)/gamma is too small that e^[(Xi-Xmax)/gamma] == 0
    VECTOR_3D_BOOL expZeroFlgMin_LSE; // similar

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

    Module(int _index, string _name, float _width = 0, float _height = 0, bool _isFixed = false, bool _isNI = false)
    {
        Init();
        name = _name;
        width = _width;
        height = _height;
        area = float_mul(width, height); //! area calculated here!
        isFixed = _isFixed;
        isNI = _isNI; // 2022-05-13 (frank)
        idx = _index;
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
        isMacro = false;
        isFiller = false;
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
    POS_3D getCenter() { return center; }
    POS_2D getLL_2D();
    POS_2D getUR_2D();
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
        start.SetZero();
        end.SetZero();
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
        intervals.clear();
        start.SetZero();
        end.SetZero();
    }

    double bottom;              // The bottom y coordinate of this SiteRow of sites
    double height;              // The height of this SiteRow of sites
    double step;                // The minimum x step of SiteRow.	by indark
    POS_2D start;               // lower left coor of this row;
    POS_2D end;                 //! lower right coor of this row;
    ORIENT orientation;         // donnie 2006-04-23  N (0) or S (1)
    vector<Interval> intervals; //!
    // double site_spacing;// site spacing in bookshelf format, equals to site width
    POS_2D getLL_2D();
    POS_2D getUR_2D();
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
        os << "SiteRow start: " << row.start << " SiteRow end:" << row.end;
        return os;
    }
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

class Interval
{
    // horizontal intervals of placement siterows, only store x coordinate, instead of POS_2D
    // there are intervals in rows because of macros or pre-defined dead zones.
    // interval is not an empty! empty is the available space in a row that is not covered by std cells
    // and interval is calculated without considering std cell locations
public:
    Interval()
    {
        SetZero();
    }
    Interval(float _start, float _end)
    {
        start = _start;
        end = _end;
    }
    inline void SetZero()
    {
        start = end = 0.0; //!! 0.0!!!!
    }
    float start;
    float end;
};
#endif