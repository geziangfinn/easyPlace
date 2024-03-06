# ePlace
trying to reimplement ePlace and ePlace-3D according to ePlace(TODAES) and ePlace3D()
TODO: consider orientation of nodes and terminals
Problem when using Eigen and CImg(which relys on X11)!!!!!
Switch to Cairo? But it seems Cairo also requires X11.

./Optimization/ePlace -aux ../testcase/ISPD2005/adaptec4/adaptec4.aux -targetDensity 1.0 -plotPath ../result_LSE -LSE -fullPlot -targetOverflow 0.1 -legalizerPath ../Legalizer -outputPath ....
see: https://blog.csdn.net/ABC_ORANGE/article/details/128012887