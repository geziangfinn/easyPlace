# ePlace
trying to reimplement ePlace and ePlace-3D according to ePlace(TODAES) and ePlace3D()

TODO: consider orientation of nodes and terminals

Problem when using Eigen and CImg(which relys on X11)!!!!!


Switch to Cairo? But it seems Cairo also requires X11.

see: https://blog.csdn.net/ABC_ORANGE/article/details/128012887

example command: ./ePlace -aux ../testcase/ISPD2005/adaptec4/adaptec4.aux -targetDensity 1.0 -plotPath YOUR_PLOTPATH -fullPlot -targetOverflow 0.1 -legalizerPath YOUR_LEGALIZERPATH -outputPath YOUR_OUTPUTPATH

Refer to the RePlAce code here:
https://github.com/geziangfinn/RePlAce_bookshelf branch: eplace_reference