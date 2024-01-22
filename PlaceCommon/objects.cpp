#include <objects.h>

void Net::addPin(Pin *pin)
{
    netPins.push_back(pin);
}

int Net::getPinCount()
{
    return netPins.size();
}

void Net::allocateMemoryForPin(int n)
{
    netPins.reserve(n);
}

double Net::calcNetHPWL()
{
    double maxX = DOUBLE_MIN;
    double minX = DOUBLE_MAX;
    double maxY = DOUBLE_MIN;
    double minY = DOUBLE_MAX;
    double maxZ = DOUBLE_MIN;
    double minZ = DOUBLE_MAX;

    double curX;
    double curY;
    double curZ;
    double HPWL;
    for (Pin *curPin : netPins)
    {
        curX = curPin->getAbsolutePos().x;
        curY = curPin->getAbsolutePos().y;
        curZ = curPin->getAbsolutePos().z;
        minX = min(minX, curX);
        maxX = max(maxX, curX);
        minY = min(minY, curY);
        maxY = max(maxY, curY);
        minZ = min(minZ, curZ);
        maxZ = max(maxY, curZ);
    }
    if (!gArg.CheckExist("3DIC"))
    {
        assert(maxZ == minZ == 0);
    }
    HPWL = ((maxX - minX) + (maxY - minY));
    return HPWL;
}

double Net::calcBoundPin()
{
    double maxX = DOUBLE_MIN;
    double minX = DOUBLE_MAX;
    double maxY = DOUBLE_MIN;
    double minY = DOUBLE_MAX;
    double maxZ = DOUBLE_MIN;
    double minZ = DOUBLE_MAX;

    double curX;
    double curY;
    double curZ;
    double HPWL;

    for (Pin *curPin : netPins)
    {
        curX = curPin->getAbsolutePos().x;
        curY = curPin->getAbsolutePos().y;
        curZ = curPin->getAbsolutePos().z;

        if (curX < minX)
        {
            minX = curX;
            boundPinXmin = curPin;
        }

        if (curX > maxX)
        {
            maxX = curX;
            boundPinXmax = curPin;
        }

        if (curY < minY)
        {
            minY = curY;
            boundPinYmin = curPin;
        }

        if (curY > maxY)
        {
            maxY = curY;
            boundPinYmax = curPin;
        }

        if (curZ < minZ)
        {
            minZ = curZ;
            boundPinZmin = curPin;
        }

        if (curZ > maxZ)
        {
            maxZ = curZ;
            boundPinZmax = curPin;
        }
    }
    if (!gArg.CheckExist("3DIC"))
    {
        assert(maxZ == minZ == 0);
    }
    HPWL = ((maxX - minX) + (maxY - minY) + (maxZ - minZ));
    return HPWL;
}

void Net::clearBoundPins()
{
    boundPinXmax = NULL;
    boundPinXmin = NULL;
    boundPinYmax = NULL;
    boundPinYmin = NULL;
    boundPinZmax = NULL;
    boundPinZmin = NULL;
}

VECTOR_2D Net::getWirelengthGradientWA_2D(Pin *)
{
    return VECTOR_2D();
}

POS_3D Pin::getAbsolutePos()
{
    POS_3D absPos;
    // module->calcCenter();//?
    absPos.x = module->getCenter().x + offset.x;
    absPos.y = module->getCenter().y + offset.y;
    absPos.z = module->getCenter().z;
    return absPos;
}

void Pin::setId(int index)
{
    idx = index;
}

void Pin::setNet(Net *_net)
{
    net = _net;
}

void Pin::setModule(Module *_module)
{
    module = _module;
}

void Pin::setDirection(int _direction)
{
    direction = _direction;
}

void Module::addPin(Pin *_pin)
{
    modulePins.push_back(_pin);
}

POS_2D Module::getLL_2D()
{
    POS_2D ll_2D;
    ll_2D.x = coor.x;
    ll_2D.y = coor.y;
    return ll_2D;
}

POS_2D Module::getUR_2D()
{
    POS_2D ur_2D;

    ur_2D.x = coor.x;
    ur_2D.y = coor.y;

    ur_2D.x += width;
    ur_2D.y += height;

    assert(width != 0 && height != 0);
    return ur_2D;
}

void Module::setOrientation(int _oritentation)
{
    orientation = _oritentation;
}

//! need to check if coor is out side of the chip!!! but should be done in placeDB
void Module::setLocation_2D(float _x, float _y, float _z)
{
    coor.x = _x;
    coor.y = _y;
    coor.z = _z;
    // update center
    center.x = coor.x + (float)0.5 * width; //! be careful of float problems
    center.y = coor.y + (float)0.5 * height;
    center.z = coor.z;
}

void Module::setCenter_2D(float _x, float _y, float _z)
{
    center.x = _x;
    center.y = _y;
    center.z = _z;
    // update coor
    coor.x = center.x - (float)0.5 * width; //! be careful of float problems
    coor.y = center.y - (float)0.5 * height;
    coor.z = center.z;
}

POS_2D SiteRow::getLL_2D()
{
    return start;
}

POS_2D SiteRow::getUR_2D()
{
    POS_2D ur_2D = end;
    ur_2D.y += height;
    return ur_2D;
}
