#include "optimizer.h"

#include <cmath>
#include <assert.h>
#include <cstdio>

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

    lastIterModulePosition.resize(movableModuleSize);
    lastIterPreconditionedGradient.resize(movableModuleSize);
    preconditionedGradient.resize(movableModuleSize);
    nesterovReferencePosition.resize(movableModuleSize);
    WAPreconditioner.resize(movableModuleSize);
    // the initial reference position is the same as the module position
    for (uint32_t idx = 0; idx < movableModuleSize; idx++)
    {
        nesterovReferencePosition[idx] = placer->ePlaceNodesAndFillers[idx]->getCenter();
    }
    placer->binNodeDensityUpdate();
    placer->densityGradientUpdate();
    placer->wirelengthGradientUpdate();

    penaltyFactor = placer->penaltyFactorInitilization();

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

        CalcPreconditionedGradient();

        NesterovIter();
        placer->binNodeDensityUpdate();
        placer->densityGradientUpdate();
        placer->wirelengthGradientUpdate();

        UpdatePenaltyFactor();
        // need a function to calculate lambda in eplace
        placer->totalGradientUpdate(penaltyFactor);
        if (gArg.CheckExist("fullPlot") && iterCount % 10 == 0)
        {
            plotter->plotCurrentPlacement("Ite_" + to_string(iterCount));
        }
    }
}

void Optimizer::setPlotter(Plotter *_plotter)
{
    plotter = _plotter;
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
        VECTOR_2D lipshitzConstant = LipschitzConstantPrediction();
        stepSize.x = 1 / lipshitzConstant.x;
        stepSize.y = 1 / lipshitzConstant.y;
    }
    cout << "Step size: " << stepSize << endl
         << endl;

    float newOptimizationParameter = (1 + sqrt(4 * float_square(nesterovOptimizationParameter) + 1)) / 2; // ak+1
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

VECTOR_2D Optimizer::LipschitzConstantPrediction()
{

    assert(placer->ePlaceNodesAndFillers.size() == lastIterModulePosition.size());
    assert(placer->totalGradient.size() == lastIterPreconditionedGradient.size());

    VECTOR_2D lipschitzConstant;

    float squaredSumNumeratorX = 0;
    float squaredSumNumeratorY = 0;
    float squaredSumDenominatorX = 0;
    float squaredSumDenominatorY = 0;
    for (uint32_t idx = 0; idx < lastIterModulePosition.size(); idx++)
    {
        POS_3D curModulePosition = placer->ePlaceNodesAndFillers[idx]->getCenter();

        squaredSumNumeratorX += float_square(preconditionedGradient[idx].x - lastIterPreconditionedGradient[idx].x);
        squaredSumDenominatorX += float_square(curModulePosition.x - lastIterModulePosition[idx].x);

        squaredSumNumeratorY += float_square(preconditionedGradient[idx].y - lastIterPreconditionedGradient[idx].y);
        squaredSumDenominatorY += float_square(curModulePosition.y - lastIterModulePosition[idx].y);
    }

    lipschitzConstant.x = sqrt(squaredSumNumeratorX) / sqrt(squaredSumDenominatorX);
    lipschitzConstant.y = sqrt(squaredSumNumeratorY) / sqrt(squaredSumDenominatorY);

    return lipschitzConstant;
}

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