#include "optimizer.h"

#include <cmath>
#include <assert.h>
#include <cstdio>

VECTOR_2D CalcLipschitzConstant_2D(const vector<POS_3D> &curV, const vector<POS_3D> &lastV, const vector<VECTOR_2D> &curPreconditionedGradient, const vector<VECTOR_2D> &lastPreconditionedGradient);

template <typename P,typename G>
NSIter<P,G>::NSIter(uint32_t size){
    mainSolution.resize(size);
    referenceSolution.resize(size);
    gradient.resize(size);
}

Optimizer::Optimizer(EPlacer_2D *placer, bool verbose)
{
    this->placer = placer;
    this->verbose = verbose;
}

void Optimizer::Init()
{
    uint32_t movableModuleSize = placer->ePlaceNodesAndFillers.size();
    iterCount = 0;
    nesterovOptimizationParameter = 1;
    lastIterHPWL = placer->db->calcHPWL();

    curNSIter = NSIter<POS_3D,VECTOR_2D>(movableModuleSize);
    lastNSIter = NSIter<POS_3D,VECTOR_2D>(movableModuleSize);
    // the initial reference position is the same as the module position
    for (uint32_t idx = 0; idx < movableModuleSize; idx++)
    {
        curNSIter.mainSolution[idx] = placer->ePlaceNodesAndFillers[idx]->getCenter();
        curNSIter.referenceSolution[idx] = curNSIter.mainSolution[idx];
    }
    PlacerStateInit();
}

void Optimizer::PlacerStateInit(){
    placer->binNodeDensityUpdate();
    placer->densityGradientUpdate();
    placer->wirelengthGradientUpdate();
    penaltyFactor = placer->penaltyFactorInitilization();
    placer->totalGradientUpdate(penaltyFactor);
}

void Optimizer::PlacerStateUpdate(){
    placer->binNodeDensityUpdate();
    placer->densityGradientUpdate();
    placer->wirelengthGradientUpdate();
    UpdatePenaltyFactor();
    placer->totalGradientUpdate(penaltyFactor);
}

void Optimizer::DoNesterovOpt()
{

    Init();
    while (!StopCondition())
    {
        if (verbose)
        {
            PrintStatistics();
        }

        NesterovIter();
        
        if (gArg.CheckExist("fullPlot") && iterCount % 10 == 0)
        {
            placer->plotCurrentPlacement("Ite_" + to_string(iterCount));
        }
    }
}

void Optimizer::NesterovIter()
{
    VECTOR_2D stepSize;
    if (iterCount == 0)
    {
        stepSize.x = 1;
        stepSize.y = 1;
    }
    else
    {
        VECTOR_2D lipshitzConstant = CalcLipschitzConstant_2D(curNSIter.referenceSolution,lastNSIter.referenceSolution,curNSIter.gradient,lastNSIter.gradient);
        stepSize.x = 1 / lipshitzConstant.x;
        stepSize.y = 1 / lipshitzConstant.y;
    }
    cout << "Step size: " << stepSize << endl << endl;

    float newOptimizationParameter = (1 + sqrt(4 * float_square(nesterovOptimizationParameter) + 1)) / 2; // ak+1
    if(gArg.CheckExist("bktrk")){
        
    }
    else{
        //update module postion
        for (uint32_t idx = 0; idx < placer->ePlaceNodesAndFillers.size(); idx++)
        {
            POS_3D newPosition, newReferencePosition;
            newPosition.SetZero();
            newReferencePosition.SetZero();
        }


    }

    for (uint32_t idx = 0; idx < placer->ePlaceNodesAndFillers.size(); idx++)
    {
        POS_3D newPosition;
        POS_3D newReferencePosition;
        newPosition.SetZero();
        newReferencePosition.SetZero();
        VECTOR_2D gradient = preconditionedGradient[idx];

        POS_3D curPosition = placer->ePlaceNodesAndFillers[idx]->getCenter(); // uk
        POS_3D curReferencePosition = nesterovReferencePosition[idx];         // vk

        // uk+1
        newPosition.x = curReferencePosition.x + stepSize.x * gradient.x;
        newPosition.y = curReferencePosition.y + stepSize.y * gradient.y;

        // vk+1
        newReferencePosition.x = newPosition.x + (nesterovOptimizationParameter - 1) * (newPosition.x - curPosition.x) / newOptimizationParameter;
        newReferencePosition.y = newPosition.y + (nesterovOptimizationParameter - 1) * (newPosition.y - curPosition.y) / newOptimizationParameter;

        // update this iteration
        lastIterModulePosition[idx] = curPosition;
        lastIterPreconditionedGradient[idx] = gradient;
        nesterovReferencePosition[idx] = newReferencePosition;
        if (isnanf(newPosition.x) || isnanf(newPosition.y))
        {
            cout << "\nnmb\n";
            exit(0);
        }
        placer->db->setModuleCenter_2D(placer->ePlaceNodesAndFillers[idx], newPosition.x, newPosition.y);
    }
    nesterovOptimizationParameter = newOptimizationParameter;
    iterCount++;
}

// VECTOR_2D Optimizer::LipschitzConstantPrediction()
// {

//     assert(placer->ePlaceNodesAndFillers.size() == curIterModulePosition.size());
//     assert(placer->totalGradient.size() == lastIterPreconditionedGradient.size());

//     VECTOR_2D lipschitzConstant;

//     float squaredSumNumeratorX = 0;
//     float squaredSumNumeratorY = 0;
//     float squaredSumDenominatorX = 0;
//     float squaredSumDenominatorY = 0;
//     for (uint32_t idx = 0; idx < curIterModulePosition.size(); idx++)
//     {
//         POS_3D curModulePosition = placer->ePlaceNodesAndFillers[idx]->getCenter();

//         squaredSumNumeratorX += float_square(preconditionedGradient[idx].x - lastIterPreconditionedGradient[idx].x);
//         squaredSumDenominatorX += float_square(curIterReferencePosition[idx].x - lastIterReferencePosition[idx].x);

//         squaredSumNumeratorY += float_square(preconditionedGradient[idx].y - lastIterPreconditionedGradient[idx].y);
//         squaredSumDenominatorY += float_square(curIterReferencePosition[idx].y - lastIterReferencePosition[idx].y);
//     }

//     lipschitzConstant.x = sqrt(squaredSumNumeratorX) / sqrt(squaredSumDenominatorX);
//     lipschitzConstant.y = sqrt(squaredSumNumeratorY) / sqrt(squaredSumDenominatorY);

//     return lipschitzConstant;
// }

bool Optimizer::StopCondition()
{
    float targetOverflow;
    if (gArg.CheckExist("targetOverflow"))
    {
        gArg.GetFloat("targetOverflow", &targetOverflow);
    }
    else
    {
        targetOverflow = 0.10;
        if (gArg.CheckExist("newblue5"))
        {
            targetOverflow = 0.102; // for ISPD2006 newblue 5
        }
    }
    return (placer->globalDensityOverflow < targetOverflow) || (iterCount > MAX_ITERATION);
}

void Optimizer::UpdatePenaltyFactor()
{
    float curHPWL = placer->db->calcHPWL();
    float multiplier = pow(PENALTY_MULTIPLIER_BASE, (-(curHPWL - lastIterHPWL) / DELTA_HPWL_REF + 1.0)); // see ePlace-3D code opt.cpp line 1523
    if (float_greater(multiplier, PENALTY_MULTIPLIER_UPPERBOUND))
    {
        multiplier = PENALTY_MULTIPLIER_UPPERBOUND;
    }
    if (float_less(multiplier, PENALTY_MULTIPLIER_LOWERBOUND))
    {
        multiplier = PENALTY_MULTIPLIER_LOWERBOUND;
    }
    penaltyFactor *= multiplier;
    // if (penaltyFactor < 0.00001)
    // {
    //     penaltyFactor = 0.00001;
    // }
    // else if (penaltyFactor > 10.0)
    // {
    //     penaltyFactor = 10.0;
    // }
    lastIterHPWL = curHPWL;
}

void Optimizer::NesterovIterBkTrk()
{

    CalcPreconditionedGradient();

    uint32_t movableModuleSize = placer->ePlaceNodesAndFillers.size();
    vector<VECTOR_2D> curIterPreconditionedGradient(movableModuleSize);
    // save current position and preconditioned gradient
    for (uint32_t idx = 0; idx < movableModuleSize; idx++)
    {
        curIterPreconditionedGradient[idx] = preconditionedGradient[idx];
    }

    float stepSizeX;
    float stepSizeY;
    if (iterCount == 0)
    {
        stepSizeX = 0.01;
        stepSizeY = 0.01;
    }
    else
    {
        VECTOR_2D lipschitzConstant = CalcLipschitzConstant_2D(curIterReferencePosition, lastIterReferencePosition, curIterPreconditionedGradient, lastIterPreconditionedGradient);
        stepSizeX = 1 / lipschitzConstant.x;
        stepSizeY = 1 / lipschitzConstant.y;
    }

    float newOptimizationParameter = (1 + sqrt(4 * float_square(nesterovOptimizationParameter) + 1)) / 2; // ak+1
    vector<POS_3D> newModulePosition(movableModuleSize);
    vector<POS_3D> newReferencePosition(movableModuleSize); // vk+1_hat
    for (uint32_t idx = 0; idx < movableModuleSize; idx++)
    {
        // update uk+1 and vk+1
        newModulePosition[idx].x = curIterReferencePosition[idx].x + stepSizeX * curIterPreconditionedGradient[idx].x;
        newModulePosition[idx].y = curIterReferencePosition[idx].y + stepSizeY * curIterPreconditionedGradient[idx].y;
        newReferencePosition[idx].x = newModulePosition[idx].x + (nesterovOptimizationParameter - 1) * (newModulePosition[idx].x - curIterModulePosition[idx].x) / newOptimizationParameter;
        newReferencePosition[idx].y = newModulePosition[idx].y + (nesterovOptimizationParameter - 1) * (newModulePosition[idx].y - curIterModulePosition[idx].y) / newOptimizationParameter;
    }
    SetModulePosition_2D(newReferencePosition);
    placer->binNodeDensityUpdate();
    placer->densityGradientUpdate();
    placer->wirelengthGradientUpdate();
    placer->totalGradientUpdate(penaltyFactor);
    CalcPreconditionedGradient();
    // x direction
    while (true)
    {
        VECTOR_2D newLipschitzConstant = CalcLipschitzConstant_2D(newReferencePosition, curIterReferencePosition, preconditionedGradient, curIterPreconditionedGradient);
        float newStepSizeX = 1 / newLipschitzConstant.x;
        if (BKTRK_EPS * stepSizeX <= newStepSizeX)
        {
            break;
        }
        stepSizeX = newStepSizeX;
        for (uint32_t idx = 0; idx < movableModuleSize; idx++)
        {
            // update uk+1 and vk+1
            newModulePosition[idx].x = curIterReferencePosition[idx].x + stepSizeX * curIterPreconditionedGradient[idx].x;
            newReferencePosition[idx].x = newModulePosition[idx].x + (nesterovOptimizationParameter - 1) * (newModulePosition[idx].x - curIterModulePosition[idx].x) / newOptimizationParameter;
        }
        SetModulePosition_2D(newReferencePosition);
        placer->binNodeDensityUpdate();
        placer->densityGradientUpdate();
        placer->wirelengthGradientUpdate();
        placer->totalGradientUpdate(penaltyFactor);
        CalcPreconditionedGradient();
    }
    // y direction
    while (true)
    {
        VECTOR_2D newLipschitzConstant = CalcLipschitzConstant_2D(newReferencePosition, curIterReferencePosition, preconditionedGradient, curIterPreconditionedGradient);
        float newStepSizeY = 1 / newLipschitzConstant.y;
        if (BKTRK_EPS * stepSizeY <= newStepSizeY)
        {
            break;
        }
        stepSizeY = newStepSizeY;
        for (uint32_t idx = 0; idx < movableModuleSize; idx++)
        {
            // update uk+1 and vk+1
            newModulePosition[idx].y = curIterReferencePosition[idx].y + stepSizeY * curIterPreconditionedGradient[idx].y;
            newReferencePosition[idx].y = newModulePosition[idx].y + (nesterovOptimizationParameter - 1) * (newModulePosition[idx].y - curIterModulePosition[idx].y) / newOptimizationParameter;
        }
        SetModulePosition_2D(newReferencePosition);
        placer->binNodeDensityUpdate();
        placer->densityGradientUpdate();
        placer->wirelengthGradientUpdate();
        placer->totalGradientUpdate(penaltyFactor);
        CalcPreconditionedGradient();
    }
    // update this iteration
    nesterovOptimizationParameter = newOptimizationParameter;
    lastIterReferencePosition = curIterReferencePosition;
    curIterReferencePosition = newReferencePosition;
    lastIterPreconditionedGradient = curIterPreconditionedGradient;
    curIterModulePosition = newModulePosition;
    iterCount++;
}

void Optimizer::SetModulePosition_2D(const vector<POS_3D> &pos)
{
    for (uint32_t idx = 0; idx < pos.size(); idx++)
    {
        placer->db->setModuleCenter_2D(placer->ePlaceNodesAndFillers[idx], pos[idx].x, pos[idx].y);
    }
}

void Optimizer::CalcPreconditionedGradient()
{
    for (uint32_t idx = 0; idx < placer->ePlaceNodesAndFillers.size(); idx++)
    {
        // get connected net number
        uint32_t connectedNetNum = placer->ePlaceNodesAndFillers[idx]->nets.size();
        uint32_t charge = placer->ePlaceNodesAndFillers[idx]->getArea();
        float preconditioner = 1 / (connectedNetNum + penaltyFactor * charge);
        preconditionedGradient[idx].x = preconditioner * placer->totalGradient[idx].x;
        preconditionedGradient[idx].y = preconditioner * placer->totalGradient[idx].y;
    }
}

void Optimizer::PrintStatistics()
{
    printf("Iter : %u\n", iterCount);
    printf("DensityOverflow = %.4f\n", placer->globalDensityOverflow);
    printf("PenaltyFactor = %.8f\n", penaltyFactor);
    printf("HPWL = %.4f\n\n", lastIterHPWL);
}

VECTOR_2D CalcLipschitzConstant_2D(const vector<POS_3D> &curV, const vector<POS_3D> &lastV, const vector<VECTOR_2D> &curPreconditionedGradient, const vector<VECTOR_2D> &lastPreconditionedGradient)
{

    assert(curV.size() == lastV.size());
    assert(curPreconditionedGradient.size() == lastPreconditionedGradient.size());

    VECTOR_2D lipschitzConstant;

    float squaredSumNumeratorX = 0;
    float squaredSumNumeratorY = 0;
    float squaredSumDenominatorX = 0;
    float squaredSumDenominatorY = 0;
    for (uint32_t idx = 0; idx < curV.size(); idx++)
    {
        squaredSumNumeratorX += float_square(curPreconditionedGradient[idx].x - lastPreconditionedGradient[idx].x);
        squaredSumDenominatorX += float_square(curV[idx].x - lastV[idx].x);

        squaredSumNumeratorY += float_square(curPreconditionedGradient[idx].y - lastPreconditionedGradient[idx].y);
        squaredSumDenominatorY += float_square(curV[idx].y - lastV[idx].y);
    }

    lipschitzConstant.x = sqrt(squaredSumNumeratorX) / sqrt(squaredSumDenominatorX);
    lipschitzConstant.y = sqrt(squaredSumNumeratorY) / sqrt(squaredSumDenominatorY);

    return lipschitzConstant;
}