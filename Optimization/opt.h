#include <vector>
#include <cstdint>

template<typename T>
class FirstOrderOptimizer{
public:
    virtual void reset();
    virtual void opt_step(std::vector<T>& x,const std::vector<T>& dx);
private:
    uint32_t iter_count;
};