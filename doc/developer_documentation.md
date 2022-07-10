# Stim Developer Documentation

This is documentation for programmers working with stim, e.g. how to build it.
These notes generally assume you are on a Linux system.

# Index

- [compatibility guarantees across versions](#compatibility)
- [releasing a new version](#release-checklist)
- [building `stim` command line tool](#build)
    - [with cmake](#build.cmake)
    - [with bazel](#build.bazel)
    - [with gcc](#build.gcc)
- [linking to `libstim` shared library](#link)
    - [with cmake](#link.cmake)
    - [with bazel](#link.bazel)
- [running C++ tests](#test)
    - [with cmake](#test.cmake)
    - [with bazel](#test.bazel)
- [running performance benchmarks](#perf)
    - [with cmake](#perf.cmake)
    - [with bazel](#perf.bazel)
    - [interpreting output from `stim_benchmark`](#perf.output)
    - [profiling with gcc and perf](#perf.profile)
- [creating a python dev environment](#venv)
- [running python unit tests](#test.pytest)
- [python packaging `stim`](#pypackage.stim)
    - [with cibuildwheels](#pypackage.stim.cibuildwheels)
    - [with bazel](#pypackage.stim.bazel)
    - [with python setup.py](#pypackage.stim.python)
    - [with pip install -e](#pypackage.stim.pip)
- [python packaging `stimcirq`](#pypackage.stimcirq)
    - [with python setup.py](#pypackage.stimcirq.python)
    - [with pip install -e](#pypackage.stimcirq.pip)
- [python packaging `sinter`](#pypackage.sinter)
    - [with python setup.py](#pypackage.sinter.python)
    - [with pip install -e](#pypackage.sinter.pip)
- [javascript packaging `stimjs`](#jspackage.stimjs)
    - [with emscripten](#jspackage.stimjs.emscripten)
- [autoformating code](#autoformat)
    - [with clang-format](#autoformat.clang-format)

# <a name="compatibility"></a>Compatibility guarantees across versions

A *bug* is bad behavior that wasn't intended. For example, the program crashing instead of returning empty results when sampling from an empty circuit would be a bug.

A *trap* is originally-intended behavior that leads to the user making mistakes. For example, allowing the user to take a number of shots that wasn't a multiple of 64 when using the data format `ptb64` was a trap because the shots were padded up to a multiple of 64 using zeroes (and these fake shots could then easily be treated as real by later analysis steps).

A *spandrel* is an implementation detail that has observable effects, but which is not really required to behave in that specific way. For example, when stim derives a detector error model from a circuit, the exact probability of an error instruction depends on minor details such as floating point error (which is sensitive to compiler optimizations). The exact floating point error that occurs is a spandrel.

- **Stim Python API**
    - The python API must maintain backwards compatibility from minor version to minor version (except for bug fixes, trap fixes, and spandrels). Violating this requires a new major version.
    - Trap fixes must be documented as breaking changes in the release notes.
    - The exact behavior of random seeding is a spandrel. The behavior must be consistent on a single machine for a single version, with the same seed always producing the same results, but it is **not** stable between minor versions. This is enforced by intentionally introducing changes for every single minor version.
- **Stim Command Line API**
    - The command land API must maintain backwards compatibility from minor version to minor version (except for bug fixes, trap fixes, and spandrels). Violating this requires a new major version.
    - Trap fixes must be documented as breaking changes in the release notes.
    - It is explicitly **not** allowed for a command to stop working due to, for example, a cleanup effort to make the commands more consistent. Violating this requires a new major version.
    - The exact behavior of random seeding is a spandrel. The behavior must be consistent on a single machine for a single version, with the same seed always producing the same results, but it is **not** stable between minor versions. This is enforced by intentionally introducing changes for every single minor version.
- **Stim C++ API**
    - The C++ API makes no compatibility guarantees. It may change arbitrarily and catastrophically from minor version to minor version.

# <a name="release-checklist"></a>Releasing a new version

- Create an off-main-branch release commit
    - [ ] `git checkout main -b SOMEBRANCHNAME`
    - [ ] Update version to `vX.Y.Z` in `setup.py` (at repo root)
    - [ ] Update version to `vX.Y.Z` in `glue/cirq/setup.py`
    - [ ] Update version to `vX.Y.Z` in `glue/sinter/setup.py`
    - [ ] Update version to `vX.Y.Z` in `glue/stimzx/setup.py`
    - [ ] `git commit -a -m "Bump to vX.Y.Z"`
    - [ ] `git tag vX.Y.Z`
    - [ ] Push tag to github
    - [ ] Check github `Actions` tab and confirm ci is running on the tag
    - [ ] Wait for ci to finish validating and producing artifacts for the tag
    - [ ] Get `stim` wheels [from cibuildwheels](#pypackage.stim.cibuildwheels) of this tag
    - [ ] Build `stimcirq` sdist on this tag [using python setup.py sdist](#pypackage.stimcirq.python)
    - [ ] Build `sinter` sdist on this tag [using python setup.py sdist](#pypackage.sinter.python)
    - [ ] Combine `stim`, `stimcirq`, and `sinter` package files into one directory
- Bump to next dev version on main branch
    - [ ] Update version to `vX.(Y+1).dev0` in all setup.py files
    - [ ] Update `INTENTIONAL_VERSION_SEED_INCOMPATIBILITY` in `src/stim/circuit/circuit.h`
    - [ ] Push to github as a branch and merge into main using a pull request
- Write release notes on github
    - [ ] In title, use two-word themeing of most important changes
    - [ ] Flagship changes section
    - [ ] Notable changes section
    - [ ] Include wheels/sdists as attachments
- Do these irreversible and public viewable steps last!
    - [ ] Upload wheels/sdists to pypi using `twine`
    - [ ] Publish the github release notes
    - [ ] Add gates reference page to wiki for the new version
    - [ ] Add python api reference page to wiki for the new version
    - [ ] Update main wiki page to point to latest reference pages
    - [ ] Tweet about the release


# <a name="build"></a>Building `stim` command line tool

The stim command line tool is a binary program `stim` that accepts commands like
`stim sample -shots 100 -in circuit.stim`
(see [the command line reference](usage_command_line.md)).
It can be built [with cmake](#build.cmake), [with bazel](#build.bazel), or
manually [with gcc](#build.gcc).

## <a name="build.cmake"></a>Building `stim` command line tool with cmake

```bash
# from the repository root:
cmake .
make stim

# output binary ends up at:
# ./out/stim
```

Vectorization can be controlled by passing the flag `-DSIMD_WIDTH` to `cmake`:

- `cmake . -DSIMD_WIDTH=256` means "use 256 bit avx operations" (forces `-mavx2`)
- `cmake . -DSIMD_WIDTH=128` means "use 128 bit sse operations" (forces `-msse2`)
- `cmake . -DSIMD_WIDTH=64` means "don't use simd operations" (no machine arch flags)
- `cmake .` means "use the best thing possible on this machine" (forces `-march=native`)

## <a name="build.bazel"></a>Building `stim` command line tool with bazel

```bash
bazel build stim
```

or, to build and run:

```bash
bazel run stim
```

## <a name="build.gcc"></a>Building `stim` command line tool with gcc

```bash
# from the repository root:
find src \
    | grep "\\.cc" \
    | grep -v "\\.\(test\|perf\|pybind\)\\.cc" \
    | xargs g++ -I src -pthread -std=c++11 -O3 -march=native

# output binary ends up at:
# ./a.out
```


# <a name="link"></a>Linking to `libstim` shared library

**!!!CAUTION!!!
Stim's C++ API is not kept stable!
Always pin to a specific version!
I *WILL* break your downstream code when I update stim if you don't!
The API is also not extensively documented;
what you can find in the headers is what you get.**

To use Stim functionality within your C++ program, you can build `libstim` and
link to it to gain direct access to underlying Stim types and methods.

If you want a stim API that promises backwards compatibility, use the python API.

## <a name="link.cmake"></a>Linking to `libstim` shared library with cmake

**!!!CAUTION!!!
Stim's C++ API is not kept stable!
Always pin to a specific version!
I *WILL* break your downstream code when I update stim if you don't!
The API is also not extensively documented;
what you can find in the headers is what you get.**

In your `CMakeLists.txt` file, use `FetchContent` to automatically fetch stim
from github when running `cmake .`:

```
# in CMakeLists.txt file

include(FetchContent)
FetchContent_Declare(stim
        GIT_REPOSITORY https://github.com/quantumlib/stim.git
        GIT_TAG v1.4.0)  # [[[<<<<<<< customize the version you want!!]]]
FetchContent_GetProperties(stim)
if(NOT stim_POPULATED)
  FetchContent_Populate(stim)
  add_subdirectory(${stim_SOURCE_DIR})
endif()
```

(Replace `v1.4.0` with another version tag as appropriate.)

For build targets that need to use stim functionality, add `libstim` to them
using `target_link_libraries`:

```
# in CMakeLists.txt file

target_link_libraries(some_cmake_target PRIVATE libstim)
```

In your source code, use `#include "stim.h"` to access stim types and functions:

```
// in a source code file

#include "stim.h"

stim::Circuit make_bell_pair_circuit() {
    return stim::Circuit(R"CIRCUIT(
        H 0
        CNOT 0 1
        M 0 1
        DETECTOR rec[-1] rec[-2]
    )CIRCUIT");
}
```

## <a name="link.bazel"></a>Linking to `libstim` shared library with bazel

**!!!CAUTION!!!
Stim's C++ API is not kept stable!
Always pin to a specific version!
I *WILL* break your downstream code when I update stim if you don't!
The API is also not extensively documented;
what you can find in the headers is what you get.**

In your `WORKSPACE` file, include stim's git repo using `git_repository`:

```
# in WORKSPACE file

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "stim",
    commit = "v1.4.0",
    remote = "https://github.com/quantumlib/stim.git",
)
```

(Replace `v1.4.0` with another version tag or commit SHA as appropriate.)

In your `BUILD` file, add `@stim//:stim_lib` to the relevant target's `deps`:

```
# in BUILD file

cc_binary(
    ...
    deps = [
        ...
        "@stim//:stim_lib",
        ...
    ],
)
```

In your source code, use `#include "stim.h"` to access stim types and functions:

```
// in a source code file

#include "stim.h"

stim::Circuit make_bell_pair_circuit() {
    return stim::Circuit(R"CIRCUIT(
        H 0
        CNOT 0 1
        M 0 1
        DETECTOR rec[-1] rec[-2]
    )CIRCUIT");
}
```


# <a name="test"></a>Running unit tests

Stim's code base includes a variety of types of tests, spanning over a few
packages and languages.

## <a name="test.cmake"></a>Running C++ unit tests with cmake

Unit testing with cmake requires the GTest library to be installed on your
system in a place that cmake can find it.
Follow the ["Standalone CMake Project" instructions from the GTest README](https://github.com/google/googletest/tree/master/googletest)
to get GTest installed correctly.

Run tests with address and memory sanitization, without compile time optimization:

```bash
# from the repository root:
cmake .
make stim_test
./out/stim_test
```

Run tests without sanitization, with compile time optimization:

```bash
# from the repository root:
cmake .
make stim_test_o3
./out/stim_test_o3
```

Stim supports 256 bit (AVX), 128 bit (SSE), and 64 bit (native) vectorization.
The type to use is chosen at compile time.
To force this choice (so that each case can be tested on one machine),
add `-DSIMD_WIDTH=256` or `-DSIMD_WIDTH=128` or `-DSIMD_WIDTH=64`
to the `cmake .` command.

## <a name="test.bazel"></a>Running C++ unit tests with bazel

```bash
# from the repository root:
bazel test stim_test
```

# <a name="perf"></a>Running performance benchmarks

## <a name="perf.cmake"></a>Running performance benchmarks with cmake

```bash
cmake .
make stim_benchmark
./out/stim_benchmark
```

## <a name="perf.cmake"></a>Running performance benchmarks with bazel

```bash
bazel run stim_benchmark
```

## <a name="perf.output"></a>Interpreting output from `stim_benchmark`

When you run `stim_benchmark` you will see output like:

```
[....................*....................] 460 ns (vs 450 ns) ( 21 GBits/s) simd_bits_randomize_10K
[...................*|....................]  24 ns (vs  20 ns) (400 GBits/s) simd_bits_xor_10K
[....................|>>>>*...............] 3.6 ns (vs 4.0 ns) (270 GBits/s) simd_bits_not_zero_100K
[....................*....................] 5.8 ms (vs 6.0 ms) ( 17 GBits/s) simd_bit_table_inplace_square_transpose_diam10K
[...............*<<<<|....................] 8.1 ms (vs 5.0 ms) ( 12 GOpQubits/s) FrameSimulator_depolarize1_100Kqubits_1Ksamples_per1000
[....................*....................] 5.3 ms (vs 5.0 ms) ( 18 GOpQubits/s) FrameSimulator_depolarize2_100Kqubits_1Ksamples_per1000
```

The bars on the left show how fast each task is running compared to baseline expectations (on my dev machine).
Each tick away from the center `|` is 1 decibel slower or faster (i.e. each `<` or `>` represents a factor of `1.26`).

Basically, if you see `[......*<<<<<<<<<<<<<|....................]` then something is *seriously* wrong, because the
code is running 25x slower than expected.

The benchmark binary supports a `--only=BENCHMARK_NAME` filter flag.
Multiple filters can be specified by separating them with commas `--only=A,B`.
Ending a filter with a `*` turns it into a prefix filter `--only=sim_*`.

## <a name="perf.profile"></a>Profiling with gcc and perf

```bash
find src \
    | grep "\\.cc" \
    | grep -v "\\.\(test\|perf\|pybind\)\\.cc" \
    | xargs g++ -I src -pthread -std=c++11 -O3 -march=native -g -fno-omit-frame-pointer
sudo perf record -g ./a.out  # [ADD STIM FLAGS FOR THE CASE YOU WANT TO PROFILE]
sudo perf report
```


## <a name="venv"></a>Creating a python dev environment

First, create a fresh python 3.6+ virtual environment using your favorite
method:
[`python -m venv`](https://docs.python.org/3/library/venv.html),
[`virtualenvwrapper`](https://virtualenvwrapper.readthedocs.io/en/latest/),
[`conda`](https://docs.conda.io/en/latest/),
or etc.

Second, build and install a stim wheel.
Follow the [python packaging stim](#pypackage.stim) instructions to
create a wheel.
I recommend [packaging with bazel](#pypackage.stim) because it is **BY FAR** the
fastest.
Once you have the wheel, run `pip install [that_wheel]`.
For example:

```base
# from the repository root in a python virtual environment
bazel build :stim_dev_wheel
pip uninstall stim --yes
pip install bazel-bin/stim-dev-py3-none-any.whl
```

Note that you need to repeat the above steps each time you make a change to
`stim`.

Third, use `pip install -e` to install development references to the pure-python
glue packages:

```
# install stimcirq dev reference:
pip install -e glue/cirq

# install sinter dev reference:
pip install -e glue/sample

# install stimzx dev reference:
pip install -e glue/zx
```

## <a name="test.pytest"></a>Running python unit tests

See [creating a python dev environment](#venv) for instructions on creating a
python virtual environment with your changes to stim installed.

Unit tests are run using `pytest`.
Examples in docstrings are tested using the `doctest_proper` script at the repo root,
which uses python's [`doctest`](https://docs.python.org/3/library/doctest.html) module
but ensures values added to a module at import time are also tested (instead of requiring
them to be [manually listed in a `__test__` property](https://docs.python.org/3/library/doctest.html#which-docstrings-are-examined)).

To test everything:

```bash
# from the repository root in a virtualenv with development wheels installed:
pytest src glue
./doctest_proper.py --module stim
./doctest_proper.py --module stimcirq --import cirq sympy
./doctest_proper.py --module sinter
./doctest_proper.py --module stimzx
```

Test only `stim`:

```bash
# from the repository root in a virtualenv with development wheels installed:
pytest src
./doctest_proper.py --module stim
```

Test only `stimcirq`:

```bash
# from the repository root in a virtualenv with development wheels installed:
pytest glue/cirq
./doctest_proper.py --module stimcirq --import cirq sympy
```

Test only `sinter`:

```bash
# from the repository root in a virtualenv with development wheels installed:
pytest glue/sample
./doctest_proper.py --module sinter
```

Test only `stimzx`:

```bash
# from the repository root in a virtualenv with development wheels installed:
pytest glue/zx
./doctest_proper.py --module stimzx
```


# <a name="pypackage.stim"></a>python packaging `stim`

Because stim is a C++ extension, it is non-trivial to create working
python packages for it.
To make cross-platform release wheels, we rely heavily on cibuildwheels.
To make development wheels, various other options are possible.

## <a name="pypackage.stim.cibuildwheels"></a>python packaging `stim` with cibuildwheels

When a commit is merged into the `main` branch of stim's GitHub repository,
there are GitHub actions that use [cibuildwheels](https://github.com/pypa/cibuildwheel)
to build wheels for all supported platforms.

cibuildwheels can also be invoked locally, assuming you have Docker installed, using a command like:

```bash
CIBW_BUILD=cp39-manylinux_x86_64 cibuildwheel --platform linux
# output goes into wheelhouse/
````

When these wheels are finished building, they are automatically uploaded to
pypi as a dev version of stim.
For release versions, the artifacts created by the github action must be
manually downloaded and uploaded using `twine`:

```bash
twine upload --username="${PROD_TWINE_USERNAME}" --password="${PROD_TWINE_PASSWORD}" artifacts_directory/*
```

## <a name="pypackage.stim.bazel"></a>python packaging `stim` with bazel

Bazel can be used to create dev versions of the stim python wheel:

```bash
# from the repository root:
bazel build stim_dev_wheel
# output is at bazel-bin/stim-dev-py3-none-any.whl
```

## <a name="pypackage.stim.python"></a>python packaging `stim` with python setup.py

Python can be used to create dev versions of the stim python wheel (very slow):

Binary distribution:

```bash
# from the repository root in a python venv with pybind11 installed:
python setup.py bdist
# output is at dist/*
```

Source distribution:

```bash
# from the repository root in a python venv with pybind11 installed:
python setup.py sdist
# output is at dist/*
```

## <a name="pypackage.stim.pip"></a>python packaging `stim` with pip install -e

You can directly install stim as a development python wheel by using pip (very slow):

```bash
# from the repository root
pip install -e .
# stim is now installed in current virtualenv as dev reference
```


# <a name="pypackage.stimcirq"></a>Python packaging `stimcirq`

## <a name="pypackage.stimcirq.python"></a>Python packaging `stimcirq` with python setup.py

```bash
# from repo root
cd glue/cirq
python setup.py sdist
cd -
# output in glue/cirq/dist/*
```

## <a name="pypackage.stimcirq.pip"></a>Python packaging `stimcirq` with pip install -e

```bash
# from repo root
pip install -e glue/cirq
# stimcirq is now installed in current virtualenv as dev reference
```

# <a name="pypackage.sinter"></a>Python packaging `sinter`

## <a name="pypackage.sinter.python"></a>Python packaging `sinter` with python setup.py

```bash
# from repo root
cd glue/sample
python setup.py sdist
cd -
# output in glue/sample/dist/*
```

## <a name="pypackage.sinter.pip"></a>Python packaging `sinter` with pip install -e

```bash
# from repo root
pip install -e glue/sample
# sinter is now installed in current virtualenv as dev reference
```

# <a name="pypackage.stimzx"></a>Python packaging `stimzx`

## <a name="pypackage.stimzx.python"></a>Python packaging `stimzx` with python setup.py

```bash
# from repo root
cd glue/zx
python setup.py sdist
cd -
# output in glue/zx/dist/*
```

## <a name="pypackage.stimzx.pip"></a>Python packaging `stimzx` with pip install -e

```bash
# from repo root
pip install -e glue/zx
# stimzx is now installed in current virtualenv as dev reference
```


# <a name="jspackage"></a>Javascript packaging `stimjs`

## <a name="jspackage.stimjs"></a>Javascript packaging `stimjs` with emscripten

Install and activate enscriptem (`emcc` must be in your PATH).
Example:

```bash
# [outside of repo]
git clone git@github.com:emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source emsdk_env.sh
```

Run the bash build script:

```bash
# [from repo root]
glue/javascript/build_wasm.sh
```

Outputs are the binary `out/stim.js` and the test runner `out/all_stim_tests.html`.
Run tests by opening in a browser and checking for an `All tests passed.` message in the browser console:

```bash
firefox out/all_stim_tests.html
```


# <a name="autoformat"></a>Autoformating code

## <a name="autoformat.clang-format"></a>Autoformating code with clang-format

Run the following command from the repo root to auto-format all C++ code:

```bash
find src | grep "\.\(cc\|h\)$" | xargs clang-format -i
```
