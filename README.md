# Stim

Stim is an extremely fast simulator for non-adaptive quantum stabilizer circuits.
Stim is based on a stabilizer tableau representation,
analogous to [Scott Aaronson's CHP simulator](https://arxiv.org/abs/quant-ph/0406196).
However, Stim makes three improvements each with large performance consequences.

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
This optimization ensures every gate has a worst case complexity of O(1), instead of O(n) or O(n^2).

Third, the simulator states are operated on using 256-bit-wide SIMD instructions.
This makes basic building block operations extremely fast, particularly considering that the relevant operations tend
to be bitwise XORs, ANDs, and ORs (which have very low clock-per-instruction costs).
For example, applying a CNOT operation to a ten thousand qubit tableau takes less than a microsecond.
The tableau is transposed as needed to keep operations fast: column major order for unitary operations and row major
order for non-deterministic measurements.
(The overhead of transposing means that non-deterministic measurements are more efficient when grouped together.)
In the case of the Pauli frame simulator, thousands of samples are computed simultaneously so that operations can be
vectorized across the samples.


# Usage (command line)

Stim reads a quantum circuit from `stdin` and writes measurement results to `stdout`.
The input format is a series of lines, each starting with an optional gate (e.g. `CNOT 0 1`)
and ending with an optional comment prefixed by `#`.
See below for a list of supported gates.
The default output format is a string of "0" and "1" characters, indicating measurement results.
The order of the results is the same as the order of the measurement commands in the input.

## Example

```bash
$ echo -e "H 0 \n CNOT 0 1 \n M 0 \n M 1 \n" | stim
11
```

## Command line flags

- `-repl`:
    Print measurement results interactively, with line separators, instead of all in one big blob the end of the circuit.
    Incompatible with several other flags.
- `-out=FILEPATH`:
    Specifies where to write results.
    If not specified, the `stdout` pipe is used.
    Specifying the output file in this way may be more performant than redirecting `stdout` to a file on the command line.
- `-shots=#`:
    Run the circuit multiple times instead of one time.
- `-format=ascii|bin_LE8|RAW`: Output format to use.
    - `ascii` (default):
        Human readable format.
        Prints "0" or "1" for each measurement.
        Prints "\n" at the end of each shot.
    - `bin_LE8`:
        Faster binary format.
        Each byte holds 8 measurement results, ordered from least significant bit to most significant bit.
        The number of measurement results from the circuit is increased to a multiple of 8 by padding with 0s.
        There is no separator between shots (other than the padding).


Here is a list of the supported no-qubit commands:

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
- `CNOT` (alternate name `CX`): Controlled NOT operation. Qubit pairs are in name order (first qubit is the control, second is the target).
- `CY`: Controlled Y operation. Qubit pairs are in name order (first qubit is the control, second is the target).
- `CZ`: Controlled Z operation. Insensitive to qubit pair order.
- `YCZ`: Y-basis-controlled Z operation. Qubit pairs are in name order (first qubit is the control, second is the target).
- `YCY`: Y-basis-controlled Y operation. Insensitive to qubit pair order.
- `YCX`: Y-basis-controlled X operation. Qubit pairs are in name order (first qubit is the control, second is the target).
- `XCZ`: X-basis-controlled Z operation. Qubit pairs are in name order (first qubit is the control, second is the target).
- `XCY`: X-basis-controlled Y operation. Qubit pairs are in name order (first qubit is the control, second is the target).
- `XCX`: X-basis-controlled X operation. Insensitive to qubit pair order.

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
- `REPEATLAST(window) iterations_including_initial`: Meta command flattened while parsing.
    Repeats the last `window` gates a total of `iterations` times (including the initial execution).
    In particular, note that `REPEATLAST(window) 1` is equivalent to having no `REPEATLAST` instruction at all.


# Building

```bash
cmake .
make stim
# output file: out/stim
```

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
slower [....................*....................] faster dB | 460 ns (vs 450 ns) ( 21 GBits/s) simd_bits_randomize_10K
slower [...................*|....................] faster dB |  24 ns (vs  20 ns) (400 GBits/s) simd_bits_xor_10K
slower [....................|>>>>*...............] faster dB | 3.6 ns (vs 4.0 ns) (270 GBits/s) simd_bits_not_zero_100K
slower [....................*....................] faster dB | 5.8 ms (vs 6.0 ms) ( 17 GBits/s) simd_bit_table_inplace_square_transpose_diam10K
slower [...............*<<<<|....................] faster dB | 8.1 ms (vs 5.0 ms) ( 12 GOpQubits/s) FrameSimulator_depolarize1_100Kqubits_1Ksamples_per1000
slower [....................*....................] faster dB | 5.3 ms (vs 5.0 ms) ( 18 GOpQubits/s) FrameSimulator_depolarize2_100Kqubits_1Ksamples_per1000
```

The bars on the left show how fast each task is running compared to baseline expectations (on my dev machine).
Each tick away from the center `|` is 1 decibel slower or faster (i.e. each `<` or `>` represents a factor of `1.26`).

Basically, if you see `[......*<<<<<<<<<<<<<|....................]` then something is *seriously* wrong, because the
code is running 25x slower than expected.

The benchmark binary supports a `-only=TASK_NAME_1,TASK_NAME_2,...` flag.

# Manual Build

Emergency `cmake`+`make` bypass:

```bash
find src | grep "\\.cc" | grep -v "\\.test\\.cc" | grep -v "\\.perf\\.cc" | xargs g++ -pthread -std=c++20 -march=native -O3
# output file: ./a.out
```
