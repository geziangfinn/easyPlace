#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "eplace.h"
#include "global.h"
#include "plot.h"

#define DELTA_HPWL_REF 35000000
#define PENALTY_MULTIPLIER_BASE 1.1
#define PENALTY_MULTIPLIER_UPPERBOUND 1.1
#define PENALTY_MULTIPLIER_LOWERBOUND 0.75
#define MAX_ITERATION 3000

class Optimizer 
{
public:
    Optimizer();
    Optimizer(EPlacer_2D* placer,bool verbose);
    void DoNesterovOpt();
    void setPlotter(Plotter*);

private:
    Plotter * plotter;
    bool verbose;
    EPlacer_2D* placer;
    vector<POS_3D> lastIterModulePosition;
    vector<VECTOR_2D> lastIterTotalGradient;
    vector<VECTOR_2D> preconditionedGradient;
    vector<POS_3D> nesterovReferencePosition;
    float nesterovOptimizationParameter;
    float penaltyFactor;
    float lastIterHPWL;
    vector<float> WAPreconditioner;
    uint32_t iterCount;
    void Init();
    void NesterovIter();
    VECTOR_2D LipschitzConstantPrediction();
    bool StopCondition();
    void Preconditioning();
    void CalcPreconditionedGradient();
    void UpdatePenaltyFactor();
    void PrintStatistics();

};

#endif