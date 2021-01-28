# Stim

Stim is an extremely fast simulator for non-adaptive quantum stabilizer circuits.
Stim is based on the stabilizer tableau representation introduced in
[Scott Aaronson et al's CHP simulator](https://arxiv.org/abs/quant-ph/0406196).
Stim makes three key improvements over CHP.

First, the stabilizer tableau that is being tracked is inverted.
The tableau tracked by Stim indexes how each qubit's X and Z observables at the current time map to compound observables
at the start of time (instead of mapping from the start of time to the current time).
This is done so that the sign of the tracked observables directly store the measurement result to return
when a measurement is deterministic.
As a result, deterministic measurements can be completed in linear time instead of quadratic time.

Second, when producing multiple samples, the initial stabilizer simulation is executed without noise in order
to create a reference sample.
Once a reference sample from the circuit is available, all that is needed is to track a Pauli frame through the circuit,
using the original sample as a template whose results are flipped or not flipped by the passing Pauli frame.
As long as all errors are probabilistic Pauli operations, and as long as 50/50 probability Z errors are placed after
every reset and every measurement, the Pauli frame can track these errors and the resulting samples will come from the
same distribution as a full stabilizer simulation.
This ensures every gate has a worst case complexity of O(1), instead of O(n) or O(n^2).

Third, data is laid out in a cache friendly way and operated on using vectorized 256-bit-wide SIMD instructions.
This makes key operations extremely fast.
For example, Stim can multiply a Pauli string with a *hundred billion terms* into another in *under a second*.
Pauli string multiplication is a key bottleneck operation when updating a stabilizer tableau.
Tracking Pauli frames can also benefit from vectorization, by combining them into batches and computing thousands of
samples at a time.

# Usage (command line)

Stim reads a quantum circuit from `stdin` and writes measurement results to `stdout`.
The input format is a series of lines, each starting with an optional gate (e.g. `CNOT 0 1`)
and ending with an optional comment prefixed by `#`.
See below for a list of supported gates.
The default output format is a string of "0" and "1" characters, indicating measurement results.
The order of the results is the same as the order of the measurement commands in the input.

## Examples

Bit flip and measure a qubit:

```bash
$ echo "
  X 0
  M 0
" | stim
```

```
1
```

Create and measure a GHZ state, five times:

```bash
$ echo "
  H 0
  CNOT 0 1 0 2
  M 0 1 2
" | stim -shots=5
```

```
111
111
000
111
000
```

Sample several runs of a small noisy surface code:

```bash
$ echo "
  REPEAT 20 {
    DEPOLARIZE1(0.001) 0 1 2 3 4 5 6 7 8
    H 3 5
    DEPOLARIZE1(0.001) 0 1 2 3 4 5 6 7 8
    CNOT 4 1 3 6 5 8
    DEPOLARIZE1(0.001) 0 1 2 3 4 5 6 7 8
    CNOT 2 1 8 7 3 4
    DEPOLARIZE1(0.001) 0 1 2 3 4 5 6 7 8
    CNOT 0 1 6 7 5 4
    DEPOLARIZE1(0.001) 0 1 2 3 4 5 6 7 8
    CNOT 4 7 3 0 5 2
    DEPOLARIZE1(0.001) 0 1 2 3 4 5 6 7 8
    H 3 5
    DEPOLARIZE1(0.001) 0 1 2 3 4 5 6 7 8
    M 1 7 3 5
    DEPOLARIZE1(0.001) 0 1 2 3 4 5 6 7 8
    R 1 7 3 5
  }
  DEPOLARIZE1(0.001) 0 1 2 3 4 5 6 7 8
  M 0 2 4 6 8
" | stim -shots=10
```

```
0010001000100010001000100010001000100010001000100010001000100010011011000101010111010
0000000000000000000000000000000000000000000000000000000000001001100110011001100110011
0010001000100010001000100010001000100010001000100110001000100010001000100010001010110
0001000100010001000100010001000100010001000100010001000100010001000100010001000101101
0010001000100010001000100010001000100010001000100011001100110011001100110011001110110
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0010001000100010001000100010001000100010001000101000100010001000100010001000001000000
0010001000100010101000100010001000110001000100010001000100000001000100010001000100000
0010001000100010001000100010001000100010001100100010101010101010101010101010101010011
0000000000000000000000000000000000000000000001000100010001001000100011001100110000100
```

## Command line flags

- `-shots=#`:
    Run the circuit multiple times instead of one time.
- `-frame0`:
    Assume that a valid reference sample is for all measurements to return 0, so that the frame simulator can start
    running immediately (without waiting for a reference sample from the tableau simulator).
- `-repl`:
    Print measurement results interactively, with line separators, instead of all in one big blob the end of the circuit.
    Incompatible with several other flags.
- `-out=FILEPATH`:
    Specifies where to write results.
    If not specified, the `stdout` pipe is used.
    Specifying the output file in this way may be more performant than redirecting `stdout` to a file on the command line.
- `-format=[name]`: Output format to use.
    - `01` (default):
        Human readable format.
        Prints all the measurements from one shot before moving on to the next.
        Prints '0' or '1' for each measurement.
        Prints '\n' at the end of each shot.
    - `b8`:
        Binary format.
        Writes all the measurements from one shot before moving on to the next.
        The number of measurements is padded up to a multiple of 8 using fake measurements that returned 0.
        Measurements are combined into groups of 8.
        The measurement results for a group are bit packed into a byte, ordered from least significant bit to most significant bit.
        The byte for the first measurement group is printed, then the second, and so forth for all groups in order.
        There is no separator between shots (other than the fake measurement padding).
    - `ptb64`:
        Partially transposed binary format.
        The number of shots is padded up to a multiple of 64 using fake shots where all measurement results are 0.
        Shots are combined into groups of 64.
        All the measurements from one shot group are written before moving on to the next group.
        Within a shot group, each of the circuit measurements has 64 results (one from each shot).
        These 64 bits of information are packed into 8 bytes, ordered from first byte to last byte and then least significant bit to most significant bit.
        The 8 bytes for the first measurement are output, then the 8 bytes for the next, and so forth for all measurements.
        There is no separator between shot groups (other than the fake shot padding).

## Supported Gates

Note: all gates support broadcasting over multiple targets.
For example, `H 0 1 2` will apply a Hadamard gate to qubits 0, 1, and 2.
Two qubit gates broadcast over each aligned pair of targets.
For example, `CNOT 0 1 2 3` will apply `CNOT 0 1` and also `CNOT 2 3`.

### Single qubit gates

- `Z`: Pauli Z gate. Phase flip.
- `Y`: Pauli Y gate.
- `X`: Pauli X gate. Bit flip.
- `H` (alternate name `H_XZ`): Hadamard gate. Swaps the X and Z axes. Unitary equals (X + Z) / sqrt(2).
- `H_XY`: Variant of the Hadamard gate that swaps the X and Y axes (instead of X and Z). Unitary equals (X + Y) / sqrt(2).
- `H_YZ`: Variant of the Hadamard gate that swaps the Y and Z axes (instead of X and Z). Unitary equals (Y + Z) / sqrt(2).
- `S` (alternate name `SQRT_Z`): Principle square root of Z gate. Equal to `diag(1, i)`.
- `S_DAG` (alternate name `SQRT_Z_DAG`): Adjoint square root of Z gate. Equal to `diag(1, -i)`.
- `SQRT_Y`: Principle square root of Y gate. Equal to `H_YZ*S*H_YZ`.
- `SQRT_Y_DAG`: Adjoint square root of Y gate. Equal to `H_YZ*S_DAG*H_YZ`.
- `SQRT_X`: Principle square root of X gate. Equal to `H*S*H`.
- `SQRT_X_DAG`: Adjoint square root of X gate. Equal to `H*S_DAG*H`.
- `I`: Identity gate. Does nothing. Why is this even here? Probably out of a misguided desire for closure.

### Two qubit gates

- `SWAP`: Swaps two qubits.
- `ISWAP`: Swaps two qubits while phasing the ZZ observable by i. Equal to `SWAP * CZ * (S tensor S)`.
- `ISWAP_DAG`: Swaps two qubits while phasing the ZZ observable by -i. Equal to `SWAP * CZ * (S_DAG tensor S_DAG)`.
- `CNOT` (alternate names `CX`, `ZCX`): Controlled NOT operation. Qubit pairs are in name order (first qubit is the control, second is the target).
- `CY` (alternate name `ZCY`): Controlled Y operation. Qubit pairs are in name order (first qubit is the control, second is the target).
- `CZ` (alternate name `ZCZ`): Controlled Z operation.
- `YCZ`: Y-basis-controlled Z operation. Qubit pairs are in name order.
- `YCY`: Y-basis-controlled Y operation.
- `YCX`: Y-basis-controlled X operation. Qubit pairs are in name order.
- `XCZ`: X-basis-controlled Z operation. Qubit pairs are in name order.
- `XCY`: X-basis-controlled Y operation. Qubit pairs are in name order.
- `XCX`: X-basis-controlled X operation.

### Non-unitary gates

- `M`:
    Z-basis measurement.
    Examples: `M 0`, `M 2 1`, `M 0 !3 1 2`. 
    Collapses the target qubits and reports their values.
    Prefixing a target with a `!` indicates that the measurement result should be inverted when reported.
    In the tableau simulator, this operation may require a transpose and so is more efficient when grouped
    (e.g. prefer `M 0 1 \n H 0` over `M 0 \n H 0 \n M 1`).
- `R`:
    Reset to |0>.
    Examples: `R 0`, `R 2 1`, `R 0 3 1 2`.
    Silently measures the target qubits and bit flips them if they're in the |1> state.
    Equivalently, discards the target qubits for zero'd qubits.
    In the tableau simulator, this operation may require a transpose and so is more efficient when grouped
    (e.g. prefer `R 0 1 \n X 0` over `R 0 \n X 0 \ nR 1`).
- `DEPOLARIZE1(p)`:
    Single qubit depolarizing error.
    Examples: `DEPOLARIZE1(0.001) 1`, `DEPOLARIZE1(0.0003) 0 2 4 6`.
    With probability `p`, applies independent single-qubit depolarizing kicks to the given qubits.
    A single-qubit depolarizing kick is `X`, `Y`, or `Z` chosen uniformly at random.
- `DEPOLARIZE2(p)`:
    Two qubit depolarizing error.
    Examples: `DEPOLARIZE2(0.001) 0 1`, `DEPOLARIZE2(0.0003) 0 2 4 6`.
    With probability `p`, applies independent two-qubit depolarizing kicks to the given qubit pairs.
    A two-qubit depolarizing kick is
    `IX`, `IY`, `IZ`, `XI`, `XX`, `XY`, `XZ`, `YI`, `YX`, `YY`, `YZ`, `ZI`, `ZX`, `ZY`, `ZZ`
    chosen uniformly at random.

### Other.

- `TICK`: Optional command indicating the end of a layer of gates.
    May be ignored, may force processing of internally queued operations and flushing of queued measurement results.
- `REPEAT N { ... }`: Repeats the instructions in its body N times.


# Building

```bash
cmake . -DSIMD_WIDTH=256
make stim
# output file: out/stim
```

By default, stim vectorizes loops using 256 bit wide avx2 instructions.
Some systems don't support these instructions.
Passing `-DSIMD_WIDTH=128` into cmake will use 128 bit wide SSE instructions instead.
Passing `-DSIMD_WIDTH=64` into cmake will use plain `uint64_t` values for everything.

# Testing

Unit testing requires GTest to be installed on your system and discoverable by CMake.
Follow the ["Standalone CMake Project" from the GTest README](https://github.com/google/googletest).

```bash
cmake .
make stim_test
./out/stim_test
```

# Benchmarking

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

The benchmark binary supports a `-only=BENCHMARK_NAME` filter flag.
Multiple filters can be specified by separating them with commas `-only=A,B`.
Ending a filter with a `*` turns it into a prefix filter `-only=sim_*`.

# Manual Build

Emergency `cmake`+`make` bypass:

```bash
find src | grep "\\.cc" | grep -v "\\.test\\.cc" | grep -v "\\.perf\\.cc" | xargs g++ -pthread -std=c++20 -march=native -O3
# output file: ./a.out
```
