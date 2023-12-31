#include <objects.h>

void Net::addPin(Pin * pin)
{
    netPins.push_back(pin);
    pin->setNet(this);
}

int Net::getPinCount()
{
    return netPins.size();
}

void Net::allocateMemoryForPin(int n)
{
    netPins.reserve(n);
}

POS_3D Pin::getAbsolutePos()
{
    POS_3D absPos;
    //module->calcCenter();//?
    absPos.x = module->center.x + offset.x;
    absPos.y = module->center.y + offset.y;
    absPos.z = module->center.z;
}

void Pin::setId(int index)
{
    idx=index;
}

void Module::addPin(Pin * pin)
{
    modulePins.push_back(pin);
}
