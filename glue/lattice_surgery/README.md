# Optimal Lattice Surgery Subroutine Compiler (OLSSCo)

## installation
In this directory `pip install .` This will also install a few packages that we need: 3 packaes from pypi `z3-solver`, `networkx`, and `stim`; 1 package from included files `stimzx` [(source)](https://github.com/quantumlib/Stim/tree/0fdddef863cfe777f3f2086a092ba99785725c07/glue/zx).

We have a dependency [kissat](https://github.com/arminbiere/kissat) which is a SAT solver, not a Python package. 
It is recommended to install it and find out the directory of the executable `kissat` because we will need it later.
Our compiler can be used without Kissat, in which case it just uses `z3-solver`, but on certain cases Kissat can offer big runtime improvements.

## how to use
See the [demo notebook in the docs directory](docs/demo.ipynb)