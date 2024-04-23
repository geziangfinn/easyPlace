#include "optimizer.h"
#include "Eigen/Dense"

#include <cmath>
#include <assert.h>
#include <cstdio>
#include <random>

float CalcLipschitzConstant_2D(const vector<POS_3D> &curV, const vector<POS_3D> &lastV, const vector<VECTOR_2D> &curPreconditionedGradient, const vector<VECTOR_2D> &lastPreconditionedGradient);
// void CopyToEigenVector(const vector<>& from,Eigen::VectorXf& to)

float getRandom(float begin,float end);

template <typename P, typename G>
void NSIter<P, G>::resize(uint32_t size)
{
    mainSolution.resize(size);
    referenceSolution.resize(size);
    gradient.resize(size);
}

// template <typename P, typename G>
// // const NSIter<P, G> &NSIter<P, G>::operator=(const NSIter &other)
// // {
// //     this->mainSolution.assign(other.mainSolution.begin(), other.mainSolution.end());
// //     this->referenceSolution.assign(other.referenceSolution.begin(), other.referenceSolution.end());
// //     this->gradient.assign(other.gradient.begin(), other.gradient.end());
// // }

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

    curNSIter.resize(movableModuleSize);
    lastNSIter.resize(movableModuleSize);
    preconditionedGradient.resize(movableModuleSize);
    // the initial reference position is the same as the module position
    for (uint32_t idx = 0; idx < movableModuleSize; idx++)
    {
        curNSIter.mainSolution[idx] = placer->ePlaceNodesAndFillers[idx]->getCenter();
        curNSIter.referenceSolution[idx] = curNSIter.mainSolution[idx];
    }
    PlacerStateInit();
    CalcPreconditionedGradient();
    curNSIter.gradient = preconditionedGradient;
}

void Optimizer::PlacerStateInit()
{
    placer->binNodeDensityUpdate();
    placer->densityGradientUpdate();
    placer->wirelengthGradientUpdate();
    penaltyFactor = placer->penaltyFactorInitilization();
    placer->totalGradientUpdate(penaltyFactor);
}

void Optimizer::PlacerStateUpdate()
{
    placer->binNodeDensityUpdate();
    placer->densityGradientUpdate();
    placer->wirelengthGradientUpdate();
}

void Optimizer::DoNesterovOpt(bool bb)
{
    if (bb){
        printf("Start Nesterov BB opt\n\n");
    }
    else{
        printf("Start Nesterov opt\n\n");
    }
    Init();
    while (!StopCondition())
    {
        if (verbose)
        {
            PrintStatistics();
        }

        if (bb){
            NesterovIterBB();
        }
        else{
            NesterovIter();
        }

        if (gArg.CheckExist("fullPlot") && iterCount % 10 == 0)
        {
            SetModulePosition_2D(curNSIter.mainSolution);
            placer->plotCurrentPlacement("Ite_" + to_string(iterCount));
            SetModulePosition_2D(curNSIter.referenceSolution);
        }
    }
    SetModulePosition_2D(curNSIter.mainSolution);
    printf("Final HPWL=%f\n",placer->db->calcHPWL());
}

void Optimizer::NesterovIter()
{
    uint32_t movableModuleSize = placer->ePlaceNodesAndFillers.size();
    float stepSize;
    NSIter<POS_3D, VECTOR_2D> newNSIter;
    newNSIter.resize(movableModuleSize);

    // calculate initial stepsize
    if (iterCount == 0)
    {
        stepSize = 0.01;
    }
    else
    {
        float lipshitzConstant = CalcLipschitzConstant_2D(curNSIter.referenceSolution, lastNSIter.referenceSolution, curNSIter.gradient, lastNSIter.gradient);
        stepSize = 1 / lipshitzConstant;
    }
    cout << "Step size: " << stepSize << endl
         << endl;

    // calculate optimization paramter
    float newOptimizationParameter = (1 + sqrt(4 * float_square(nesterovOptimizationParameter) + 1)) / 2; // ak+1

    // new module postion
    for (uint32_t idx = 0; idx < placer->ePlaceNodesAndFillers.size(); idx++)
    {
        POS_3D &newPosition = newNSIter.mainSolution[idx];
        POS_3D &newReferencePosition = newNSIter.referenceSolution[idx];

        VECTOR_2D gradient = curNSIter.gradient[idx];
        POS_3D curPosition = curNSIter.mainSolution[idx];
        POS_3D curReferencePosition = curNSIter.referenceSolution[idx];

        newPosition = placer->db->getValidModuleCenter_2D(placer->ePlaceNodesAndFillers[idx],
                                                          curReferencePosition.x + stepSize * gradient.x,
                                                          curReferencePosition.y + stepSize * gradient.y);
        newReferencePosition = placer->db->getValidModuleCenter_2D(placer->ePlaceNodesAndFillers[idx],
                                                                   newPosition.x + (nesterovOptimizationParameter - 1) * (newPosition.x - curPosition.x) / newOptimizationParameter,
                                                                   newPosition.y + (nesterovOptimizationParameter - 1) * (newPosition.y - curPosition.y) / newOptimizationParameter);

    }
    SetModulePosition_2D(newNSIter.referenceSolution);
    PlacerStateUpdate();

    if (gArg.CheckExist("bktrk"))
    {
        while (true)
        {
            placer->totalGradientUpdate(penaltyFactor);
            CalcPreconditionedGradient();
            newNSIter.gradient = preconditionedGradient;
            float newLipschitzConstant = CalcLipschitzConstant_2D(newNSIter.referenceSolution, curNSIter.referenceSolution, newNSIter.gradient, curNSIter.gradient);
            float newStepSize = 1 / newLipschitzConstant;
            if (BKTRK_EPS * stepSize <= newStepSize)
            {
                break;
            }
            printf("bktrk!\n");
            stepSize = newStepSize;
            for (uint32_t idx = 0; idx < placer->ePlaceNodesAndFillers.size(); idx++)
            {
                POS_3D &newPosition = newNSIter.mainSolution[idx];
                POS_3D &newReferencePosition = newNSIter.referenceSolution[idx];

                VECTOR_2D gradient = curNSIter.gradient[idx];
                POS_3D curPosition = curNSIter.mainSolution[idx];
                POS_3D curReferencePosition = curNSIter.referenceSolution[idx];

                newPosition = placer->db->getValidModuleCenter_2D(placer->ePlaceNodesAndFillers[idx],
                                                                  curReferencePosition.x + stepSize * gradient.x,
                                                                  curReferencePosition.y + stepSize * gradient.y);
                newReferencePosition = placer->db->getValidModuleCenter_2D(placer->ePlaceNodesAndFillers[idx],
                                                                           newPosition.x + (nesterovOptimizationParameter - 1) * (newPosition.x - curPosition.x) / newOptimizationParameter,
                                                                           newPosition.y + (nesterovOptimizationParameter - 1) * (newPosition.y - curPosition.y) / newOptimizationParameter);

                placer->db->setModuleCenter_2D(placer->ePlaceNodesAndFillers[idx], newReferencePosition.x, newReferencePosition.y);
            }
            PlacerStateUpdate();
        }
    }

    // update this iteration
    UpdatePenaltyFactor();
    placer->totalGradientUpdate(penaltyFactor);
    CalcPreconditionedGradient();
    newNSIter.gradient = preconditionedGradient;
    lastNSIter = curNSIter;
    curNSIter = newNSIter;
    nesterovOptimizationParameter = newOptimizationParameter;
    iterCount++;
}

void Optimizer::NesterovIterBB(){
    uint32_t movableModuleSize = placer->ePlaceNodesAndFillers.size();
    float stepSize;
    NSIter<POS_3D,VECTOR_2D> newNSIter;
    newNSIter.resize(movableModuleSize);

    // 
    if (iterCount == 0){
        stepSize = 0.01;
    }    
    else{
        float lipschitzConstant = CalcLipschitzConstant_2D(curNSIter.referenceSolution,lastNSIter.referenceSolution,curNSIter.gradient,lastNSIter.gradient);
        float lip_step = 1 / lipschitzConstant;
        Eigen::VectorXf s(2*movableModuleSize);
        Eigen::VectorXf y(2*movableModuleSize);
        // move them into eigen vector
        for (size_t i = 0; i < movableModuleSize; i++){
            s[2*i] = curNSIter.referenceSolution[i].x - lastNSIter.referenceSolution[i].x;
            s[2*i+1] = curNSIter.referenceSolution[i].y - lastNSIter.referenceSolution[i].y;
            y[2*i] = curNSIter.gradient[i].x - lastNSIter.gradient[i].x;
            y[2*i+1] = curNSIter.gradient[i].y - lastNSIter.gradient[i].y;
        }
        float s_norm = s.norm();
        float y_norm = y.norm();
        float s_dot_y = s.dot(y);
        float bb_long_step = pow(s_norm,2) / s_dot_y;
        float bb_short_step = s_dot_y / pow(y_norm,2);    
        if (bb_short_step > 0){
            stepSize = bb_short_step;
        }
        else{
            stepSize = min(s_norm/y_norm,lip_step);
        }
    }
    printf("stepSize : %f\n\n",stepSize);

    // calculate optimization paramter
    float newOptimizationParameter = (1 + sqrt(4 * float_square(nesterovOptimizationParameter) + 1)) / 2; // ak+1

    // perform one step
    for (uint32_t idx = 0; idx < placer->ePlaceNodesAndFillers.size(); idx++)
    {
        POS_3D &newPosition = newNSIter.mainSolution[idx];
        POS_3D &newReferencePosition = newNSIter.referenceSolution[idx];

        VECTOR_2D gradient = curNSIter.gradient[idx];
        POS_3D curPosition = curNSIter.mainSolution[idx];
        POS_3D curReferencePosition = curNSIter.referenceSolution[idx];
        newPosition = placer->db->getValidModuleCenter_2D(placer->ePlaceNodesAndFillers[idx],
                                                            curReferencePosition.x + stepSize * gradient.x ,
                                                            curReferencePosition.y + stepSize * gradient.y );
        newReferencePosition = placer->db->getValidModuleCenter_2D(placer->ePlaceNodesAndFillers[idx],
                                                                    newPosition.x + (nesterovOptimizationParameter - 1) * (newPosition.x - curPosition.x) / newOptimizationParameter,
                                                                    newPosition.y + (nesterovOptimizationParameter - 1) * (newPosition.y - curPosition.y) / newOptimizationParameter);

        placer->db->setModuleCenter_2D(placer->ePlaceNodesAndFillers[idx], newReferencePosition.x, newReferencePosition.y);
    }
    SetModulePosition_2D(newNSIter.referenceSolution);
    PlacerStateUpdate();
    UpdatePenaltyFactor();
    placer->totalGradientUpdate(penaltyFactor);
    CalcPreconditionedGradient();
    newNSIter.gradient = preconditionedGradient;
    lastNSIter = curNSIter;
    curNSIter = newNSIter;
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

// void Optimizer::NesterovIterBkTrk()
// {

//     CalcPreconditionedGradient();

//     uint32_t movableModuleSize = placer->ePlaceNodesAndFillers.size();
//     vector<VECTOR_2D> curIterPreconditionedGradient(movableModuleSize);
//     // save current position and preconditioned gradient
//     for (uint32_t idx = 0; idx < movableModuleSize; idx++)
//     {
//         curIterPreconditionedGradient[idx] = preconditionedGradient[idx];
//     }

//     float stepSizeX;
//     float stepSizeY;
//     if (iterCount == 0)
//     {
//         stepSizeX = 0.01;
//         stepSizeY = 0.01;
//     }
//     else
//     {
//         VECTOR_2D lipschitzConstant = CalcLipschitzConstant_2D(curIterReferencePosition, lastIterReferencePosition, curIterPreconditionedGradient, lastIterPreconditionedGradient);
//         stepSizeX = 1 / lipschitzConstant.x;
//         stepSizeY = 1 / lipschitzConstant.y;
//     }

//     float newOptimizationParameter = (1 + sqrt(4 * float_square(nesterovOptimizationParameter) + 1)) / 2; // ak+1
//     vector<POS_3D> newModulePosition(movableModuleSize);
//     vector<POS_3D> newReferencePosition(movableModuleSize); // vk+1_hat
//     for (uint32_t idx = 0; idx < movableModuleSize; idx++)
//     {
//         // update uk+1 and vk+1
//         newModulePosition[idx].x = curIterReferencePosition[idx].x + stepSizeX * curIterPreconditionedGradient[idx].x;
//         newModulePosition[idx].y = curIterReferencePosition[idx].y + stepSizeY * curIterPreconditionedGradient[idx].y;
//         newReferencePosition[idx].x = newModulePosition[idx].x + (nesterovOptimizationParameter - 1) * (newModulePosition[idx].x - curIterModulePosition[idx].x) / newOptimizationParameter;
//         newReferencePosition[idx].y = newModulePosition[idx].y + (nesterovOptimizationParameter - 1) * (newModulePosition[idx].y - curIterModulePosition[idx].y) / newOptimizationParameter;
//     }
//     SetModulePosition_2D(newReferencePosition);
//     placer->binNodeDensityUpdate();
//     placer->densityGradientUpdate();
//     placer->wirelengthGradientUpdate();
//     placer->totalGradientUpdate(penaltyFactor);
//     CalcPreconditionedGradient();
//     // x direction
//     while (true)
//     {
//         VECTOR_2D newLipschitzConstant = CalcLipschitzConstant_2D(newReferencePosition, curIterReferencePosition, preconditionedGradient, curIterPreconditionedGradient);
//         float newStepSizeX = 1 / newLipschitzConstant.x;
//         if (BKTRK_EPS * stepSizeX <= newStepSizeX)
//         {
//             break;
//         }
//         stepSizeX = newStepSizeX;
//         for (uint32_t idx = 0; idx < movableModuleSize; idx++)
//         {
//             // update uk+1 and vk+1
//             newModulePosition[idx].x = curIterReferencePosition[idx].x + stepSizeX * curIterPreconditionedGradient[idx].x;
//             newReferencePosition[idx].x = newModulePosition[idx].x + (nesterovOptimizationParameter - 1) * (newModulePosition[idx].x - curIterModulePosition[idx].x) / newOptimizationParameter;
//         }
//         SetModulePosition_2D(newReferencePosition);
//         placer->binNodeDensityUpdate();
//         placer->densityGradientUpdate();
//         placer->wirelengthGradientUpdate();
//         placer->totalGradientUpdate(penaltyFactor);
//         CalcPreconditionedGradient();
//     }
//     // y direction
//     while (true)
//     {
//         VECTOR_2D newLipschitzConstant = CalcLipschitzConstant_2D(newReferencePosition, curIterReferencePosition, preconditionedGradient, curIterPreconditionedGradient);
//         float newStepSizeY = 1 / newLipschitzConstant.y;
//         if (BKTRK_EPS * stepSizeY <= newStepSizeY)
//         {
//             break;
//         }
//         stepSizeY = newStepSizeY;
//         for (uint32_t idx = 0; idx < movableModuleSize; idx++)
//         {
//             // update uk+1 and vk+1
//             newModulePosition[idx].y = curIterReferencePosition[idx].y + stepSizeY * curIterPreconditionedGradient[idx].y;
//             newReferencePosition[idx].y = newModulePosition[idx].y + (nesterovOptimizationParameter - 1) * (newModulePosition[idx].y - curIterModulePosition[idx].y) / newOptimizationParameter;
//         }
//         SetModulePosition_2D(newReferencePosition);
//         placer->binNodeDensityUpdate();
//         placer->densityGradientUpdate();
//         placer->wirelengthGradientUpdate();
//         placer->totalGradientUpdate(penaltyFactor);
//         CalcPreconditionedGradient();
//     }
//     // update this iteration
//     nesterovOptimizationParameter = newOptimizationParameter;
//     lastIterReferencePosition = curIterReferencePosition;
//     curIterReferencePosition = newReferencePosition;
//     lastIterPreconditionedGradient = curIterPreconditionedGradient;
//     curIterModulePosition = newModulePosition;
//     iterCount++;
// }

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
        float connectedNetNum = placer->ePlaceNodesAndFillers[idx]->modulePins.size();
        // printf("Connected net num:%d, Pin count:%d\n",connectedNetNum,placer->ePlaceNodesAndFillers[idx]->modulePins.size());
        // assert(connectedNetNum == placer->ePlaceNodesAndFillers[idx]->modulePins.size());
        float charge = placer->ePlaceNodesAndFillers[idx]->getArea();
        float preconditioner = 1 / max(1.0f,(connectedNetNum + penaltyFactor * charge));
        preconditionedGradient[idx].x = preconditioner * placer->totalGradient[idx].x;
        preconditionedGradient[idx].y = preconditioner * placer->totalGradient[idx].y;
    }
}

void Optimizer::PrintStatistics()
{
    printf("Iter : %u\n", iterCount);
    printf("DensityOverflow = %.8f\n", placer->globalDensityOverflow);
    printf("PenaltyFactor = %.10f\n", penaltyFactor);
    printf("HPWL = %.4f\n\n", lastIterHPWL);
}

float CalcLipschitzConstant_2D(const vector<POS_3D> &curV, const vector<POS_3D> &lastV, const vector<VECTOR_2D> &curPreconditionedGradient, const vector<VECTOR_2D> &lastPreconditionedGradient)
{

    assert(curV.size() == lastV.size());
    assert(curPreconditionedGradient.size() == lastPreconditionedGradient.size());

    float lipschitzConstant;

    float squaredSumNumeratorX = 0;
    float squaredSumNumeratorY = 0;
    float squaredSumDenominatorX = 0;
    float squaredSumDenominatorY = 0;
    for (uint32_t idx = 0; idx < curV.size(); idx++)
    {
        // printf("cur_ref=%.8f,last_ref=%.8f,cur_grad=%.8f,last_grad=%.8f\n",curV[idx].x,lastV[idx].x,curPreconditionedGradient[idx].x,lastPreconditionedGradient[idx].x);
        squaredSumNumeratorX += float_square(curPreconditionedGradient[idx].x - lastPreconditionedGradient[idx].x);
        squaredSumDenominatorX += float_square(curV[idx].x - lastV[idx].x);

        squaredSumNumeratorY += float_square(curPreconditionedGradient[idx].y - lastPreconditionedGradient[idx].y);
        squaredSumDenominatorY += float_square(curV[idx].y - lastV[idx].y);
    }

    lipschitzConstant = sqrt(squaredSumNumeratorX + squaredSumNumeratorY) / sqrt(squaredSumDenominatorX + squaredSumDenominatorY);

    return lipschitzConstant;
}
