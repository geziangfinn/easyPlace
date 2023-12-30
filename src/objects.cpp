#include <objects.h>

void Net::addPin(Pin * pin)
{
    assert(pin);
    netPins.push_back(pin);
}

int Net::getPinCount()
{
    return netPins.size();
}

POS_3D Pin::getAbsolutePos()
{
    POS_3D absPos;
    absPos.x=module->center.x+offset.x;
    absPos.y=module->center.y+offset.y;
    absPos.z=module->center.z;
}
