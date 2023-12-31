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
float float_mul(float a, float b) // perform float number multiplication
{
    float c = a * b;
    return c;
}

bool float_greater(float a, float b)// return true if a > b
{
    return a-b>EPS;
}
#endif