#include <vector>
#include <cstdint>

#include "opt.h"

template<typename T>
class Momentum :public FirstOrderOptimizer<T>{
public:
    Momentum(T lr,T m);
    void reset();
    void opt_step(std::vector<T>& x,const std::vector<T>& dx);
private:
    T learining_rate;
    T momentum;
    std::vector<T> velocity;
};