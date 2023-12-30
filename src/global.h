#ifndef GLOBAL_H
#define GLOBAL_H

#include <iostream>
#include <vector>
#include <string>
#include <assert.h>
#include <fstream>
using namespace std;
const string padding(30, '=');

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
};
struct VECTOR_2D
{
    float x;
    float y;
};
struct VECTOR_3D
{
    float x;
    float y;
    float z;
};
float float_mul(float a, float b) // perform float number multiplication
{
    float c = a * b;
    return c;
}
#endif