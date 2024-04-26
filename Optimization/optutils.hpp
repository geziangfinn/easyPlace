#ifndef OPT_UTILS_HPP
#define OPT_UTILS_HPP

#include <vector>
#include <cstddef>
#include <assert.h>
#include "global.h"
/*
    T should override + - * / operators
*/
template <typename T>
float calc_lipschitz_constant(const std::vector<T> &xt, const std::vector<T> &xt_1, const std::vector<T> &dxt, const std::vector<T> &dxt_1)
{
    assert(xt.size() == xt_1.size());
    assert(dxt.size() == dxt_1.size());
    float squared_sum_denominator = 0;
    float squared_sum_numerator = 0;
    for (size_t idx = 0; idx < xt.size(); idx++)
    {
        squared_sum_numerator += (dxt[idx] - dxt_1[idx]) * (dxt[idx] - dxt_1[idx]);
        squared_sum_denominator += (xt[idx] - xt_1[idx]) * (xt[idx] - xt_1[idx]);
    
    }
    return sqrt(squared_sum_numerator) / sqrt(squared_sum_denominator);
}

#endif