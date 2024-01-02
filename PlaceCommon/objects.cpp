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
    return absPos;
}

void Pin::setId(int index)
{
    idx=index;
}

void Pin::setNet(Net *_net)
{
    net=_net;
}

void Pin::setModule(Module *_module)
{
    module=_module;
}

void Pin::setDirection(int _direction)
{
    direction=_direction;
}

void Module::addPin(Pin * pin)
{
    modulePins.push_back(pin);
}

void Module::setLocation(float x, float y, float z)
{
    coor.x=x;
    coor.y=y;
    coor.z=z;
}

void Module::setOrientation(int _oritentation)
{
    orientation=_oritentation;
}
