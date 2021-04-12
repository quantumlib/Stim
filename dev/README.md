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

# Build stim python package

Ensure python environment dependencies are present:

```bash
pip install -y pybind11
```

Create a source distribution:

```bash
python setup.py sdist
```

Output is in the `dist` directory, and can be uploaded using `twine`.

```bash
twine upload --username="${PROD_TWINE_USERNAME}" --password="${PROD_TWINE_PASSWORD}" dist/[CREATED_FILE_GOES_HERE]
```

# Build stimcirq python package

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
bazel :stim_test
```

### Run stim python package tests

In a clean virtual environment:

```bash
pip install pytest
pip install -e .
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
