#ifndef NESTEROV_HPP
#define NESTEROV_HPP

#include <vector>
#include <cstdint>
#include <cstddef>

#include "global.h"
#include "eplace.h"
#include "optutils.hpp"
#include "Eigen/Dense"

#define MAX_ITERATION 1000
#define BKTRK_EPS 0.95

template <typename T>
class NSIter
{
public:
    void resize(size_t length);
    std::vector<T> main_solution;
    std::vector<T> reference_solution;
    std::vector<T> gradient;
};

template <typename T>
void NSIter<T>::resize(size_t length)
{
    main_solution.resize(length);
    reference_solution.resize(length);
    gradient.resize(length);
}

template <typename T>
class EplaceNesterovOpt : public FirstOrderOptimizer<T>
{
public:
    EplaceNesterovOpt(EPlacer_2D *placer) : placer(placer){};
    // void opt();
private:
    bool stop_condition();
    void opt_step();
    void init();
    void wrap_up();
    void opt_step_bb();
    void opt_step_bktrk();
    void opt_step_vanilla();
    EPlacer_2D *placer;
    NSIter<T> cur_iter, last_iter;
    size_t iter_count;
    float NS_opt_param; // ak
};

template <typename T>
bool EplaceNesterovOpt<T>::stop_condition()
{
    float targetOverflow;
    if (gArg.CheckExist("targetOverflow"))
    {
        gArg.GetFloat("targetOverflow", &targetOverflow);
    }
    else
    {
        targetOverflow = 0.1f;
    }

    float cGPtargetOverflow = 0.07f;

    bool judge;
    switch (placer->placementStage)
    {
    case mGP:
        judge = (placer->globalDensityOverflow < targetOverflow) || (iter_count > MAX_ITERATION);
        if (judge)
        {
            placer->mGPIterationCount = iter_count;
        }
        // return (placer->globalDensityOverflow < targetOverflow) || (iter_count > MAX_ITERATION);
        return judge;
        break;
    case FILLERONLY:
        return iter_count >= 20;
        break;
    case cGP:
        return (placer->globalDensityOverflow < cGPtargetOverflow) || (iter_count > MAX_ITERATION);
        break;
    default:
        cerr << "INCORRECT PLACEMENT STAGE!\n";
        exit(0);
    }

    // if (placer->placementStage == mGP)
    // {
    //     bool judge = (placer->globalDensityOverflow < targetOverflow) || (iter_count > MAX_ITERATION);
    //     if (judge)
    //     {
    //         placer->mGPIterationCount = iter_count;
    //     }
    //     // return (placer->globalDensityOverflow < targetOverflow) || (iter_count > MAX_ITERATION);
    //     return judge;
    // }
    // else if (placer->placementStage == FILLERONLY)
    // {
    //     return iter_count >= 20;
    // }
    // else if (placer->placementStage == cGP)
    // {
    //     return (placer->globalDensityOverflow < cGPtargetOverflow) || (iter_count > MAX_ITERATION);
    // }
    // else
    // {
    //     cerr << "INCORRECT PLACEMENT STAGE!\n";
    //     exit(0);
    // }

    // return (placer->globalDensityOverflow < targetOverflow) || (iter_count > MAX_ITERATION);
}

template <typename T>
void EplaceNesterovOpt<T>::opt_step()
{
    printf("Iter %d\n", iter_count);

    if (gArg.CheckExist("bb"))
    {
        opt_step_bb();
    }
    else if (gArg.CheckExist("bktrk"))
    {
        opt_step_bktrk();
    }
    else
    {
        opt_step_vanilla();
    }
    placer->updatePenaltyFactor();
    placer->showInfo();

    switch (placer->placementStage)
    {
    case mGP:
        if (iter_count % 10 == 0 && gArg.CheckExist("fullPlot"))
        {
            placer->plotCurrentPlacement("mGP Iter-" + to_string(iter_count));
        }
        break;
    case FILLERONLY:
        if (gArg.CheckExist("fullPlot"))
        {
            placer->plotCurrentPlacement("FILLERONLY Iter-" + to_string(iter_count));
        }
        break;
    case cGP:
        if (gArg.CheckExist("fullPlot"))
        {
            placer->plotCurrentPlacement("cGP Iter-" + to_string(iter_count));
        }
        break;
    default:
        cerr << "INCORRECT PLACEMENT STAGE!\n";
        exit(0);
    }

    iter_count++;
}

// template <typename T>
// void EplaceNesterovOpt<T>::reset(){
//     iter_count = 0;
// }

template <typename T>
void EplaceNesterovOpt<T>::init()
{
    iter_count = 0;
    NS_opt_param = 1;
    cur_iter.main_solution = placer->getPosition();
    printf("main length %d\n", cur_iter.main_solution.size());
}

template <typename T>
void EplaceNesterovOpt<T>::wrap_up()
{
    placer->setPosition(cur_iter.main_solution);
}

template <>
void EplaceNesterovOpt<VECTOR_3D>::opt_step_bb()
{
    cur_iter.reference_solution = placer->getPosition();
    placer->totalGradientUpdate(); //?
    cur_iter.gradient = placer->getGradient();
    float step_size;
    NSIter<VECTOR_3D> new_iter;
    size_t length = cur_iter.main_solution.size();
    new_iter.resize(length);
    if (iter_count == 0)
    {
        step_size = 0.01;
    }
    else
    {
        float lipschitz_constant = calc_lipschitz_constant(cur_iter.reference_solution, last_iter.reference_solution, cur_iter.gradient, last_iter.gradient);
        float lip_step = 1 / lipschitz_constant;
        Eigen::VectorXf s(3 * length);
        Eigen::VectorXf y(3 * length);
        // move them into eigen vector
        for (size_t i = 0; i < length; i++)
        {
            s[3 * i] = cur_iter.reference_solution[i].x - last_iter.reference_solution[i].x;
            s[3 * i + 1] = cur_iter.reference_solution[i].y - last_iter.reference_solution[i].y;
            s[3 * i + 2] = cur_iter.reference_solution[i].z - last_iter.reference_solution[i].z;
            y[3 * i] = cur_iter.gradient[i].x - last_iter.gradient[i].x;
            y[3 * i + 1] = cur_iter.gradient[i].y - last_iter.gradient[i].y;
            y[3 * i + 2] = cur_iter.gradient[i].z - last_iter.gradient[i].z;
        }
        float s_norm = s.norm();
        float y_norm = y.norm();
        float s_dot_y = s.dot(y);
        float bb_long_step = pow(s_norm, 2) / s_dot_y;
        float bb_short_step = s_dot_y / pow(y_norm, 2);
        if (bb_short_step > 0)
        {
            step_size = bb_short_step;
        }
        else
        {
            step_size = min(s_norm / y_norm, lip_step);
        }
        printf("lip=%f,bb_short=%f,bb_long=%f\n", lip_step, bb_short_step, bb_long_step);
    }
    printf("stepSize : %f\n", step_size);

    float new_NS_opt_param = (1 + sqrt(4 * float_square(NS_opt_param) + 1)) / 2; // ak+1

    // perform one step
    for (size_t idx = 0; idx < length; idx++)
    {
        VECTOR_3D &new_position = new_iter.main_solution[idx];
        VECTOR_3D &new_reference_position = new_iter.reference_solution[idx];

        VECTOR_3D gradient = cur_iter.gradient[idx];
        VECTOR_3D cur_position = cur_iter.main_solution[idx];
        VECTOR_3D cur_reference_position = cur_iter.reference_solution[idx];
        new_position = cur_reference_position + gradient * step_size;
        new_reference_position = new_position + (new_position - cur_position) * ((NS_opt_param - 1) / new_NS_opt_param);
    }
    placer->setPosition(new_iter.reference_solution);
    last_iter = cur_iter;
    cur_iter = new_iter;
    NS_opt_param = new_NS_opt_param;
}

template <>
void EplaceNesterovOpt<VECTOR_3D>::opt_step_bktrk()
{
    cur_iter.reference_solution = placer->getPosition();
    placer->totalGradientUpdate(); //?
    cur_iter.gradient = placer->getGradient();
    float step_size;
    NSIter<VECTOR_3D> new_iter;
    size_t length = cur_iter.main_solution.size();
    new_iter.resize(length);

    // initial step size
    if (iter_count == 0)
    {
        step_size = 0.01;
    }
    else
    {
        float lipschitz_constant = calc_lipschitz_constant(cur_iter.reference_solution, last_iter.reference_solution, cur_iter.gradient, last_iter.gradient);
        step_size = 1 / lipschitz_constant;
    }

    printf("stepSize : %f\n", step_size);

    float new_NS_opt_param = (1 + sqrt(4 * float_square(NS_opt_param) + 1)) / 2; // ak+1

    while (true)
    {
        // first move cells
        for (size_t idx = 0; idx < length; idx++)
        {
            VECTOR_3D &new_position = new_iter.main_solution[idx];
            VECTOR_3D &new_reference_position = new_iter.reference_solution[idx];

            VECTOR_3D gradient = cur_iter.gradient[idx];
            VECTOR_3D cur_position = cur_iter.main_solution[idx];
            VECTOR_3D cur_reference_position = cur_iter.reference_solution[idx];
            new_position = cur_reference_position + gradient * step_size;
            new_reference_position = new_position + (new_position - cur_position) * ((NS_opt_param - 1) / new_NS_opt_param);
        }
        placer->setPosition(new_iter.reference_solution);
        placer->totalGradientUpdate();
        new_iter.gradient = placer->getGradient();
        float new_lipschitz_constant = calc_lipschitz_constant(new_iter.reference_solution, cur_iter.reference_solution, new_iter.gradient, cur_iter.gradient);
        float new_step_size = 1 / new_lipschitz_constant;
        if (BKTRK_EPS * step_size <= new_step_size)
        {
            break;
        }
        printf("bktrk!\n");
        step_size = new_step_size;
    }
    // update iter variable
    last_iter = cur_iter;
    cur_iter = new_iter;
    NS_opt_param = new_NS_opt_param;
}

template <>
void EplaceNesterovOpt<VECTOR_3D>::opt_step_vanilla()
{
    cur_iter.reference_solution = placer->getPosition();
    placer->totalGradientUpdate(); //?
    cur_iter.gradient = placer->getGradient();
    float step_size;
    NSIter<VECTOR_3D> new_iter;
    size_t length = cur_iter.main_solution.size();
    new_iter.resize(length);
    if (iter_count == 0)
    {
        step_size = 0.01;
    }
    else
    {
        float lipschitz_constant = calc_lipschitz_constant(cur_iter.reference_solution, last_iter.reference_solution, cur_iter.gradient, last_iter.gradient);
        step_size = 1 / lipschitz_constant;
    }
    printf("stepSize : %f\n", step_size);

    float new_NS_opt_param = (1 + sqrt(4 * float_square(NS_opt_param) + 1)) / 2; // ak+1

    // perform one step
    for (size_t idx = 0; idx < length; idx++)
    {
        VECTOR_3D &new_position = new_iter.main_solution[idx];
        VECTOR_3D &new_reference_position = new_iter.reference_solution[idx];

        VECTOR_3D gradient = cur_iter.gradient[idx];
        VECTOR_3D cur_position = cur_iter.main_solution[idx];
        VECTOR_3D cur_reference_position = cur_iter.reference_solution[idx];
        new_position = cur_reference_position + gradient * step_size;
        new_reference_position = new_position + (new_position - cur_position) * ((NS_opt_param - 1) / new_NS_opt_param);
    }
    placer->setPosition(new_iter.reference_solution);
    last_iter = cur_iter;
    cur_iter = new_iter;
    NS_opt_param = new_NS_opt_param;
}

#endif