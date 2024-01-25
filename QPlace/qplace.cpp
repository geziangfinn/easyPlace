#include "qplace.h"
#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <Eigen/IterativeLinearSolvers>
#include <unsupported/Eigen/IterativeSolvers>
using namespace Eigen;
void QPPlacer::setPlotter(Plotter *_plotter)
{
    plotter = _plotter;
}
void QPPlacer::quadraticPlacement()
{
    // TESTING, one HPWL can be removed in the future
    db->moveNodesCenterToCenter(); //!!!!!!

    double HPWL = db->calcHPWL();

    double HPWL2 = db->calcNetBoundPins();

    assert(HPWL == HPWL2);

    int nodeCount = db->dbNodes.size();
    int maxIterationNumber;

    if (gArg.CheckExist("IPiteCount"))
    {
        gArg.GetInt("IPiteCount", &maxIterationNumber);
    }
    else
    {
        maxIterationNumber = 20;
    }

    float xError, yError;
    float target_error = 0.000001;

    printf("INFO:  initial HPWL: %.6lf\n", HPWL);
    cout << "INFO:  The matrix size is " << nodeCount << endl; //! matrix size: only nodes(movable modules)

    setNbThreads(8); //! a parameter(numThreads) fixed here

    // BCGSTAB settings
    SMatrix X_A(nodeCount, nodeCount), Y_A(nodeCount, nodeCount);            // A for Ax=b
    VectorXf X_x(nodeCount), Y_x(nodeCount), X_b(nodeCount), Y_b(nodeCount); // x and b for Ax=b
    double qp_time;
    for (int i = 0;; i++)
    {

        time_start(&qp_time);
        createSparseMatrix(X_A, Y_A, X_x, Y_x, X_b, Y_b);

        BiCGSTAB<SMatrix, IdentityPreconditioner> solver;
        solver.setMaxIterations(100);

        solver.compute(X_A);
        X_x = solver.solveWithGuess(X_b, X_x);
        xError = solver.error();

        solver.compute(Y_A);
        Y_x = solver.solveWithGuess(Y_b, Y_x);
        yError = solver.error();

        updateModuleLocation(X_x, Y_x);
        HPWL = db->calcNetBoundPins();

        if (gArg.CheckExist("debug") || gArg.CheckExist("fullPlot"))
        {
            assert(plotter);
            plotter->plotCurrentPlacement("Initial placement iteration " + to_string(i));
        }

        time_end(&qp_time);
        printf("INFO:  at iteration number %3d,  CG Error %.6lf,  HPWL %.6lf,  CPUtime %.2lf\n", i, max(xError, yError), HPWL, qp_time);

        if (fabs(xError) < target_error && fabs(yError) < target_error && i > 4)
        {
            break;
        }
        if (i >= maxIterationNumber)
        {
            break;
        }
    }
}

void QPPlacer::createSparseMatrix(SMatrix &X_A, SMatrix &Y_A, VectorXf &X_x, VectorXf &Y_x, VectorXf &X_b, VectorXf &Y_b)
{
    vector<T> tripletListX, tripletListY; // create matrix A with triples, a triple：(i,j,v), means Aij=v
    tripletListX.reserve(10000000);
    tripletListY.reserve(10000000);
    int nodeCount = db->dbNodes.size();

    Module *curNode = NULL;
    for (int i = 0; i < nodeCount; i++)
    {
        curNode = db->dbNodes[i];
        assert(curNode);
        assert(curNode->idx == i);
        X_x(i) = curNode->getCenter().x; //!
        Y_x(i) = curNode->getCenter().y;
        X_b(i) = Y_b(i) = 0;
    }

    for (Net *curNet : db->dbNets)
    {
        assert(curNet);
        int pinCount = curNet->netPins.size();
        Pin *pin1;
        Pin *pin2;
        float constant1 = 1.0 / ((float)pinCount - 1.0); // see kraftwerk2 equation (8), here we follow eplace and use 1 instead of 2
        for (int j = 0; j < pinCount; j++)
        {
            pin1 = curNet->netPins[j];
            assert(pin1);
            for (int k = j + 1; k < pinCount; k++)
            {
                pin2 = curNet->netPins[k];
                assert(pin2);
                if (pin1->module == pin2->module)
                {
                    continue;
                }

                if (pin1 == curNet->boundPinXmin || pin1 == curNet->boundPinXmax || pin2 == curNet->boundPinXmin || pin2 == curNet->boundPinXmax)
                {
                    float distanceX = fabs(pin1->getAbsolutePos().x - pin2->getAbsolutePos().x);

                    float weightX = 0.0f; // see kraftwerk2 equation (8), here we follow eplace and use 1 instead of 2
                    if (float_greaterorequal(distanceX, MIN_DISTANCE))
                    {
                        weightX = constant1 / distanceX;
                    }
                    else
                    {
                        weightX = constant1 / MIN_DISTANCE;
                    }
                    if (weightX < 0)
                        printf("ERROR WEIGHT\n");

                    if (!pin1->module->isFixed && !pin2->module->isFixed) // both are movable modules
                    {
                        tripletListX.push_back(T(pin1->module->idx, pin1->module->idx, weightX));
                        tripletListX.push_back(T(pin2->module->idx, pin2->module->idx, weightX));

                        tripletListX.push_back(
                            T(pin1->module->idx, pin2->module->idx, (-1.0) * weightX));
                        tripletListX.push_back(
                            T(pin2->module->idx, pin1->module->idx, (-1.0) * weightX));

                        X_b(pin1->module->idx) += (-1.0) * weightX * ((pin1->offset.x) - (pin2->offset.x));
                        X_b(pin2->module->idx) += (-1.0) * weightX * ((pin2->offset.x) - (pin1->offset.x));
                    }

                    else if (pin1->module->isFixed && !pin2->module->isFixed) // 1 is terminal, 2 is movable
                    {
                        tripletListX.push_back(T(pin2->module->idx, pin2->module->idx, weightX));
                        X_b(pin2->module->idx) += weightX * (pin1->getAbsolutePos().x - (pin2->offset.x));
                    }

                    else if (!pin1->module->isFixed && pin2->module->isFixed) // 2 is terminal, 1 is movable
                    {
                        tripletListX.push_back(T(pin1->module->idx, pin1->module->idx, weightX));
                        X_b(pin1->module->idx) += weightX * (pin2->getAbsolutePos().x - (pin1->offset.x));
                    }
                }
                if (pin2 == curNet->boundPinYmin || pin1 == curNet->boundPinYmax || pin2 == curNet->boundPinYmin || pin2 == curNet->boundPinYmax)
                {
                    float distanceY = fabs(pin1->getAbsolutePos().y - pin2->getAbsolutePos().y);

                    float weightY = 0.0f; // see kraftwerk2 equation (8), here we follow eplace and use 1 instead of 2
                    if (float_greaterorequal(distanceY, MIN_DISTANCE))
                    {
                        weightY = constant1 / distanceY;
                    }
                    else
                    {
                        weightY = constant1 / MIN_DISTANCE;
                    }
                    if (weightY < 0)
                        printf("ERROR WEIGHT\n");

                    if (!pin1->module->isFixed && !pin2->module->isFixed) // both are movable modules
                    {
                        tripletListY.push_back(T(pin1->module->idx, pin1->module->idx, weightY));
                        tripletListY.push_back(T(pin2->module->idx, pin2->module->idx, weightY));

                        tripletListY.push_back(
                            T(pin1->module->idx, pin2->module->idx, (-1.0) * weightY));
                        tripletListY.push_back(
                            T(pin2->module->idx, pin1->module->idx, (-1.0) * weightY));

                        Y_b(pin1->module->idx) += (-1.0) * weightY * ((pin1->offset.y) - (pin2->offset.y));
                        Y_b(pin2->module->idx) += (-1.0) * weightY * ((pin2->offset.y) - (pin1->offset.y));
                    }

                    else if (pin1->module->isFixed && !pin2->module->isFixed) // 1 is terminal, 2 is movable
                    {
                        tripletListY.push_back(T(pin2->module->idx, pin2->module->idx, weightY));
                        Y_b(pin2->module->idx) += weightY * (pin1->getAbsolutePos().y - (pin2->offset.y));
                    }

                    else if (!pin1->module->isFixed && pin2->module->isFixed) // 2 is terminal, 1 is movable
                    {
                        tripletListY.push_back(T(pin1->module->idx, pin1->module->idx, weightY));
                        Y_b(pin1->module->idx) += weightY * (pin2->getAbsolutePos().y - (pin1->offset.y));
                    }
                }
            }
        }
    }

    X_A.setFromTriplets(tripletListX.begin(), tripletListX.end());
    Y_A.setFromTriplets(tripletListY.begin(), tripletListY.end());
}

void QPPlacer::updateModuleLocation(VectorXf &X_x, VectorXf &Y_x)
{
    //! do indexs match?
    int nodeCount = db->dbNodes.size();
    Module *curNode;
    for (int i = 0; i < nodeCount; i++)
    {
        curNode = db->dbNodes[i];
        assert(curNode);
        assert(curNode->idx == i);
        db->setModuleCenter_2D(curNode, X_x(i), Y_x(i));
    }
}
