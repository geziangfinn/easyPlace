#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "eplace.h"
#include "global.h"
#include "plot.h"

#define DELTA_HPWL_REF 350000
#define PENALTY_MULTIPLIER_BASE 1.1
#define PENALTY_MULTIPLIER_UPPERBOUND 1.05
#define PENALTY_MULTIPLIER_LOWERBOUND 0.95
#define MAX_ITERATION 3000
#define BKTRK_EPS 0.95

// p is the type of position , G is the type of gradient
template <typename P, typename G>
class NSIter
{
public:
    NSIter(){};
    const NSIter<P, G> &operator=(const NSIter &other);
    void resize(uint32_t size);
    vector<P> mainSolution;
    vector<P> referenceSolution;
    vector<G> gradient;
};

class Optimizer
{
public:
    Optimizer();
    Optimizer(EPlacer_2D *placer, bool verbose);
    void DoNesterovOpt();

private:
    bool verbose;
    EPlacer_2D *placer;
    NSIter<POS_3D, VECTOR_2D> curNSIter, lastNSIter;
    vector<VECTOR_2D> preconditionedGradient;
    float nesterovOptimizationParameter;
    float penaltyFactor;
    float lastIterHPWL;
    uint32_t iterCount;
    void SetModulePosition_2D(const vector<POS_3D> &pos);
    void Init();
    void NesterovIter();
    //    VECTOR_2D LipschitzConstantPrediction();
    bool StopCondition();
    void CalcPreconditionedGradient();
    void NesterovIterBkTrk();
    void UpdatePenaltyFactor();
    void PlacerStateInit();
    void PlacerStateUpdate();
    void PrintStatistics();
};

#endif