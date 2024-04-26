#ifndef OPT_HPP
#define OPT_HPP

#include <vector>
#include <cstdint>
#include <cstddef>

template<typename T>
class FirstOrderOptimizer{
public:
    void opt();
private:
    virtual void init()=0;
    virtual void opt_step()=0;
    virtual bool stop_condition()=0;
};

template <typename T>
void FirstOrderOptimizer<T>::opt(){
    init();
    while(!stop_condition()){
        opt_step();
    }
}

#endif