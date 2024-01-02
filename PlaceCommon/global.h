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
using namespace std;
const string padding(30, '=');
#define EPS 1.0E-15 // for float number comparison
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
inline int orientInt(char* c)// transform orientation string to int (enum ORIENT)
{
    assert( strlen(c) <= 2 );
    if( c[0] == 'F' )
    {
	switch( c[1] )
	{
	    case 'N': return 4;
	    case 'W': return 5;
	    case 'S': return 6;
	    case 'E': return 7;
	}
    }
    else
    {
	switch( c[0] )
	{
	    case 'N': return 0;
	    case 'W': return 1;
	    case 'S': return 2;
	    case 'E': return 3;
	}
    }
    return 0;	// Use "N" if any problem occurs.
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

inline bool float_greater(float a, float b)// return true if a > b
{
    return a-b>EPS;
}
#endif