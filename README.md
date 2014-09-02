NewBioDiVinE
=============

This is NewBioDiVinE. The tool that combines model checking tool BioDiVinE 1.0 (provided by owcty, distr_map and 
owcty_reversed algorithms) with parameter estimation tool based on DiVinE (provided by pepmc algorithm). 
Both extended by new input model and in respect to it by new abstraction method that trasforms non-linear functions 
like Hill functions into piece-wise multi-affine functions which tools can work with. This tool is supplemented by 
graphical visualisation of computing state space based on XDot tool. This functionality provides predot algorithm.

For more informations see https://github.com/sybila/NewBioDiVinE

For compilation of this package follow the instructions:

    ./autogen
    
    make
    
For success in this step, you need: 
    MPI - 1.2 or higher
    Autoconf - 2.61 or higher
    Automake - 1.10.1 or higher
    gcc - 4.7 or higher
    XDot
    Graphviz
    Boost library (especially thread and system)
    hoard library
    
If you noticed some problems during configuration, contact us
(sybila@fi.muni.cz)
    
