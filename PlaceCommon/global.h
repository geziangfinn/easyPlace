#ifndef GLOBAL_H
#define GLOBAL_H

#include <iostream>
#include <vector>
#include <string>
#include <assert.h>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <map>
#include <sys/time.h>
#include <sys/resource.h>
#include "string.h"
#include "arghandler.h"
using namespace std;
const string padding(30, '=');
#define EPS 1.0E-15 // for float number comparison
#define DOUBLE_MAX __DBL_MAX__
#define DOUBLE_MIN __DBL_MIN__

//! The followings are colors used for log(in shell)
#define RESET "\033[0m"
#define BLACK "\033[30m"              /* Black */
#define RED "\033[31m"                /* Red */
#define GREEN "\033[32m"              /* Green */
#define YELLOW "\033[33m"             /* Yellow */
#define BLUE "\033[34m"               /* Blue */
#define MAGENTA "\033[35m"            /* Magenta */
#define CYAN "\033[36m"               /* Cyan */
#define WHITE "\033[37m"              /* White */
#define BOLDBLACK "\033[1m\033[30m"   /* Bold Black */
#define BOLDRED "\033[1m\033[31m"     /* Bold Red */
#define BOLDGREEN "\033[1m\033[32m"   /* Bold Green */
#define BOLDYELLOW "\033[1m\033[33m"  /* Bold Yellow */
#define BOLDBLUE "\033[1m\033[34m"    /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m" /* Bold Magenta */
#define BOLDCYAN "\033[1m\033[36m"    /* Bold Cyan */
#define BOLDWHITE "\033[1m\033[37m"   /* Bold White */

enum ORIENT
{
    OR_N,
    OR_W,
    OR_S,
    OR_E,
    OR_FN,
    OR_FW,
    OR_FS,
    OR_FE
    //         0    1    2    3    4     5     6     7
    //         |    -    |    -    |     -     |     -
};
inline int orientInt(char *c) // transform orientation string to int (enum ORIENT)
{
    assert(strlen(c) <= 2);
    if (c[0] == 'F')
    {
        switch (c[1])
        {
        case 'N':
            return 4;
        case 'W':
            return 5;
        case 'S':
            return 6;
        case 'E':
            return 7;
        }
    }
    else
    {
        switch (c[0])
        {
        case 'N':
            return 0;
        case 'W':
            return 1;
        case 'S':
            return 2;
        case 'E':
            return 3;
        }
    }
    return 0; // Use "N" if any problem occurs.
}
struct POS_2D // POS means postition, POS_2D can be used to store coordinates, offsets
{
    float x;
    float y;
    POS_2D() { SetZero(); };
    POS_2D(float _x, float _y)
    {
        x = _x;
        y = _y;
    }
    inline void SetZero()
    {
        x = y = 0;
    }
    friend inline std::ostream &operator<<(std::ostream &os, const POS_2D &pos)
    {
        os << "(" << pos.x << "," << pos.y << ")";
        return os;
    }
};
struct POS_3D
{
    float x;
    float y;
    float z;
    POS_3D() { SetZero(); };
    POS_3D(float _x, float _y, float _z)
    {
        x = _x;
        y = _y;
        z = _z;
    }
    inline void SetZero()
    {
        x = y = z = 0;
    }
    friend inline std::ostream &operator<<(std::ostream &os, const POS_3D &pos)
    {
        os << "(" << pos.x << "," << pos.y << "," << pos.z << ")";
        return os;
    }
};
struct VECTOR_2D
{
    float x;
    float y;
    friend inline std::ostream &operator<<(std::ostream &os, const VECTOR_2D &vec)
    {
        os << "[" << vec.x << "," << vec.y << "]"; // [] for vectors and () for pos
        return os;
    }
};
struct VECTOR_3D
{
    float x;
    float y;
    float z;
    friend inline std::ostream &operator<<(std::ostream &os, const VECTOR_3D &vec)
    {
        os << "[" << vec.x << "," << vec.y << "," << vec.z << "]"; // [] for vectors and () for pos
        return os;
    }
};
inline float float_mul(float a, float b) // perform float number multiplication
{
    float c = a * b;
    return c;
}

inline bool float_greater(float a, float b) // return true if a > b
{
    return a - b > 1.0 * EPS;
}

inline bool float_less(float a, float b) // return true if a < b
{
    return a - b < -1.0 * EPS;
}
inline bool float_equal(float a, float b)
{
    return fabs(a-b) < EPS;
}

inline bool float_lessorequal(float a, float b)
{
    return float_less(a, b) || float_equal(a, b);
}

inline bool float_greaterorequal(float a, float b)
{
    return float_greater(a, b) || float_equal(a, b);
}

inline double seconds()
{
#ifdef WIN32 // Windows
    struct __timeb64 tstruct;
    _ftime64(&tstruct);
    return (double)tstruct.time + 0.001 * tstruct.millitm;
#else // Linux
    rusage time;
    getrusage(RUSAGE_SELF, &time);
    // return (double)(1.0*time.ru_utime.tv_sec+0.000001*time.ru_utime.tv_usec);	// user time

    return (double)(1.0 * time.ru_utime.tv_sec + 0.000001 * time.ru_utime.tv_usec + // user time +
                    1.0 * time.ru_stime.tv_sec + 0.000001 * time.ru_stime.tv_usec); // system time
#endif

    // clock() loop is about 72min. (or 4320 sec)
    // return double(clock())/CLOCKS_PER_SEC;
}

inline void segmentFaultCP(string checkpointname)
{
    if(gArg.CheckExist("segDebug"))
    {
            cout<<endl<<padding<<checkpointname<<padding<<endl;
    }

}

#endif