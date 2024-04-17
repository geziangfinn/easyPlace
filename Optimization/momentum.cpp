#include <assert.h>

#include "momentum.h"

template<typename T>
Momentum<T>::Momentum(T lr,T m):learining_rate(lr),momentum(m),iter_count(0){}

template<typename T>
void Momentum<T>::reset(){
    iter_count = 0;
}

template<typename T>
void Momentum<T>::opt_step(std::vector<T>& x,const std::vector<T>& dx){
    assert(x.size() == dx.size());
    uint32_t vec_len = x.size();

    // first iteration
    if(iter_count == 0){
        velocity.assign(x.size(),0);
    }
    
    for(uint32_t index = 0;index < x.size(); index++){
        T next_v = momentum * velocity[index] - learining_rate * dx[index];
        x[index] = x[index] + next_v;
    }
}



