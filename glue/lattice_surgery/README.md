# Lattice Surgery Subroutine Synthesizer (LaSsynth)
A lattice surgery subroutine (LaS) is a confined volume with a set of ports.
Within this volume, lattice surgery merges and splits are performed.
The function of a LaS is characterized by a set of stabilizers on these ports.

The lattice surgery subroutine synthesizer (LaSsynth) uses SAT/SMT solvers to synthesize LaS given the volume, the ports, and the stabilizers.
LaSsynth outputs a textual representation of LaS (LaSRe) which is a JSON file with filename extension `.lasre`.
LaSsynth can also generate 3D modelling files in the [GLTF](https://www.khronos.org/gltf/) format from LaSRe files.

The main ideas of this project is provided in the paper [A SAT Scalpel for Lattice Surgery](http://arxiv.org/abs/2404.18369) by Tan, Niu, and Gidney.
For files specific to the paper, please refer to [its Zenodo archive](https://zenodo.org/doi/10.5281/zenodo.11051465).

## Installation
It is recommended to create a virtual Python environment. Once inside the environment, in this directory, `pip install .`
Apart from LaSsynth, this will install a few packages that we need: 
  - `z3-solver` version `4.12.1.0`, from pip
  - `networkx` default version, from pip
  - `stim` default version, from pip
  - `stimzx` from files included in sirectory `./stimzx/`. We copied these files from [here](https://github.com/quantumlib/Stim/tree/0fdddef863cfe777f3f2086a092ba99785725c07/glue/zx).
  - `ipykernel` default version, from pip, to view the demo Jupyter notebook.

We have a dependency [kissat](https://github.com/arminbiere/kissat) which is a SAT solver, not a Python package. 
It is recommended to install it and find out the directory of the executable `kissat` because we will need it later.
LaSsynth can be used without Kissat, in which case it just uses `z3-solver`, but on certain cases Kissat can offer big runtime improvements.

## How to use
See the [demo notebook in the docs directory](docs/demo.ipynb)

## Cite this work
```bibtex
@inproceedings{tan-niu-gidney_lattice_surgery,
    author = {Tan, Daniel Bochen and Niu, Murphy Yuezhen and Gidney, Craig},
	title = {A {SAT} Scalpel for Lattice Surgery: Representation and Synthesis of Subroutines for Surface-Code Fault-Tolerant Quantum Computing},
	shorttitle = {A {SAT} Scalpel for Lattice Surgery},
	booktitle = {2024 ACM/IEEE 51st Annual International Symposium on Computer Architecture ({ISCA})},
	year = {2024},
    url = {http://arxiv.org/abs/2404.18369},
}
```