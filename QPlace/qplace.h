#ifndef QPLACE_H
#define QPLACE_H
//! qplace: quadratic placement, used to create an initial placement for ePlace
#include <Eigen/SparseCore>
#include "global.h"
#include "placedb.h"
using Eigen::VectorXf;
typedef Eigen::SparseMatrix<float, Eigen::RowMajor> SMatrix;
typedef Eigen::Triplet<float> T;
#define MIN_DISTANCE 25.0 /* 10.0 */ /* 5.0 */ /* 1.0 */

class QPPlacer;

class QPPlacer
{
public:
    QPPlacer()
    {
        db = NULL;
    }
    QPPlacer(PlaceDB *_db)
    {
        db = _db;
    }
    PlaceDB *db;
    void quadraticPlacement();

    void createSparseMatrix(SMatrix &X_A, SMatrix &Y_A, VectorXf &X_x, VectorXf &Y_x, VectorXf &X_b, VectorXf &Y_b); //! solve Ax=b for X and Y coordinates. Create A,x and b, see kraftwerk2 for more details
    void updateModuleLocation(VectorXf &X_x, VectorXf &Y_x);
};
#endif