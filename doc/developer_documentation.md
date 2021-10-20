# Stim Developer Documentation

This is documentation for programmers working with stim, e.g. how to build it.
These notes generally assume you are on a Linux system.

## Build stim command line tool

### CMake Build

```bash
cmake .
make stim
# ./out/stim
```

To control the vectorization (e.g. this is done for testing),
use `cmake . -DSIMD_WIDTH=256` (implying `-mavx2`)
or `cmake . -DSIMD_WIDTH=128` (implying `-msse2`)
or `cmake . -DSIMD_WIDTH=64` (implying no machine architecture flag).
If `SIMD_WIDTH` is not specified, `-march=native` is used.

### Bazel Build

```bash
bazel build stim
# bazel run stim
```

### Manual Build

```bash
find src | grep "\\.cc" | grep -v "\\.\(test\|perf\|pybind\)\\.cc" | xargs g++ -pthread -std=c++11 -O3 -march=native
# ./a.out
```

# Profile stim command line tool

```bash
find src | grep "\\.cc" | grep -v "\\.\(test\|perf\|pybind\)\\.cc" | xargs g++ -pthread -std=c++11 -O3 -march=native -g -fno-omit-frame-pointer
sudo perf record -g ./a.out  # [ADD STIM FLAGS FOR THE CASE YOU WANT TO PROFILE]
sudo perf report
```

# Run benchmarks

```bash
cmake .
make stim_benchmark
./out/stim_benchmark
```

This will output results like:

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

# Build all python packages

From the repo root, in a python 3.9+ environment:

```bash
./glue/python/create_sdists.sh VERSION_SPECIFIER_GOES_HERE
```

Output is in the `dist` directory from the repo root, and can be uploaded using `twine`.

The version specifier should be like `v1.5.0`, or `v1.5.dev0` for development versions.

Note that the script actually overwrites the version strings in various `setup.py` files.

# Manual build stim python package

Ensure python environment dependencies are present:

```bash
pip install pybind11
```

Create a source distribution:

```bash
python setup.py sdist
```

Output is in the `dist` directory, and can be uploaded using `twine`.

```bash
twine upload --username="${PROD_TWINE_USERNAME}" --password="${PROD_TWINE_PASSWORD}" dist/[CREATED_FILE_GOES_HERE]
```

# Manual build stimcirq python package

Create a source distribution:

```bash
cd glue/cirq
python setup.py sdist
cd ../..
```

Output is in the `glue/cirq/dist` directory, and can be uploaded using `twine`.

```bash
twine upload --username="${PROD_TWINE_USERNAME}" --password="${PROD_TWINE_PASSWORD}" glue/cirq/dist/[CREATED_FILE_GOES_HERE]
```

# Build javascript bindings

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

# Testing

### Run C++ tests using CMAKE

Unit testing with CMAKE requires GTest to be installed on your system and discoverable by CMake.
Follow the ["Standalone CMake Project" from the GTest README](https://github.com/google/googletest/tree/master/googletest).

Run tests with address and memory sanitization, but without optimizations:

```bash
cmake .
make stim_test
./out/stim_test
```

To force AVX vectorization, SSE vectorization, or no vectorization
pass `-DSIMD_WIDTH=256` or `-DSIMD_WIDTH=128` or `-DSIMD_WIDTH=64` to the `cmake` command.

Run tests with optimizations without sanitization:

```bash
cmake .
make stim_test_o3
./out/stim_test_o3
```

### Run C++ tests using Bazel

Run tests with whatever settings Bazel feels like using:

```bash
bazel test :stim_test
```

### Run stim python package tests

In a clean virtual environment:

```bash
pip install pytest
pip install -e .
python setup.py install  # Workaround for https://github.com/pypa/setuptools/issues/230
python -m pytest src
python -c "import stim; import doctest; assert doctest.testmod(stim).failed == 0"
```

### Run stimcirq python package tests

In a clean virtual environment:

```bash
pip install pytest
pip install -e glue/cirq
python -m pytest glue/cirq
python -c "import stimcirq; import doctest; assert doctest.testmod(stimcirq).failed == 0"
```

## Static Linking (cmake)

**WARNING**.
Stim's C++ API is not stable.
It may change in incompatible ways from version to version.
There's also no API reference, although the basics are somewhat similar to the python API.
You will have to rely on reading method signatures and comments to discover functionality.

Assuming that's acceptable to you...

In your `CMakeLists.txt` file, use `FetchContent` to automatically fetch stim from github:

```
include(FetchContent)
FetchContent_Declare(stim
        GIT_REPOSITORY https://github.com/quantumlib/stim.git
        GIT_TAG v1.4.0)
FetchContent_GetProperties(stim)
if(NOT stim_POPULATED)
  FetchContent_Populate(stim)
  add_subdirectory(${stim_SOURCE_DIR})
endif()
```

(Replace `v1.4.0` with another version tag as appropriate.)

Then link to `libstim` in targets using stim:

```
target_link_libraries(some_target_defined_in_your_cmake_file PRIVATE libstim)
```

And finally `#include "stim.h"` source files to use stim types and functions:

```
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

## Static Linking (bazel)

**WARNING**.
Stim's C++ API is not stable.
It may change in incompatible ways from version to version.
There's also no API reference, although the basics are somewhat similar to the python API.
You will have to rely on reading method signatures and comments to discover functionality.

Assuming that's acceptable to you...

In your `WORKSPACE` file, include stim's git repo using `git_repository`:

```
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "stim",
    commit = "v1.4.0",
    remote = "https://github.com/quantumlib/stim.git",
)
```

(Replace `v1.4.0` with another version tag or commit SHA as appropriate.)

Then in your `BUILD` file add `@stim//:stim_lib` to the relevant target's `deps`:

```
cc_binary(
    ...
    deps = [
        ...
        "@stim//:stim_lib",
        ...
    ],
)
```

And finally `#include "stim.h"` in source files to use stim types and functions:

```
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

### Auto-format Code

Run the following command from the repo root to auto-format all C++ code:

```bash
find src | grep "\.\(cc\|h\)$" | xargs clang-format -i
```
