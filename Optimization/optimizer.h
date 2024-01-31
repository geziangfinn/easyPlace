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

class Optimizer 
{
public:
    Optimizer();
    Optimizer(EPlacer_2D* placer,bool verbose);
    void DoNesterovOpt();

private:
    bool verbose;
    EPlacer_2D* placer;
    vector<POS_3D> curIterModulePosition; //uk
    vector<POS_3D> lastIterReferencePosition;  //vk-1
    vector<POS_3D> curIterReferencePosition; //vk
    vector<VECTOR_2D> lastIterPreconditionedGradient; //pre_vk-1
    vector<VECTOR_2D> preconditionedGradient;
    float nesterovOptimizationParameter;
    float penaltyFactor;
    float lastIterHPWL;
    uint32_t iterCount;
    void SetModulePosition_2D(const vector<POS_3D>& pos);
    void Init();
    void NesterovIter();
//    VECTOR_2D LipschitzConstantPrediction();
    bool StopCondition();
    void CalcPreconditionedGradient();
    void NesterovIterBkTrk();
    void UpdatePenaltyFactor();
    void PrintStatistics();

};

#endif