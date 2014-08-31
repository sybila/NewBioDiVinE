BioDiVinE-1.1
=============

This is the package containing BioDiVinE-1.1, new version of BioDiVinE-1.0 tool (including DiVinE/Library and DiVinE/ToolSet.)

This branch of DiVinE focuses on modeling and analysis of dynamic models of biological systems
including sigmoidal switches and Hill functions, that are transformed into ramp functions
by piece-wise multi-affine abstraction.

Currently this is an experimental branch of DiVinE that includes the basic kernel 
of DiVinE (the DiVinE library) which extended with state-space generator for biological 
systems. Note that only some of the DiVinE tools are included 
(in modified version to work with the biological extension).

For more informations see https://github.com/martindemko/BioDiVinE-1.1

For compilation of this package follow the instructions:

    ./autogen
    
    make
    
    For success in this step, you need Autoconf 2.61 and Automake 1.10.1 and gcc 4.7
    If you noticed some problems during ./configure, contact us
    (sybila@fi.muni.cz)
    
