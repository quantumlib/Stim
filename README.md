# Stim

Stim is a fast simulator for non-adaptive quantum stabilizer circuits.
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
This makes key operations fast.
For example, Stim can multiply a Pauli string with a *hundred billion terms* into another in *under a second*.
Pauli string multiplication is a key bottleneck operation when updating a stabilizer tableau.
Tracking Pauli frames can also benefit from vectorization, by combining them into batches and computing thousands of
samples at a time.

# Usage (python)

Stim can be installed into a python 3 environment using pip:

```bash
pip install stim
```

Once stim is installed, you can `import stim` and use it.
There are two supported use cases: interactive usage and high speed sampling.

You can use the Tableau simulator in an interactive fashion:

```python
import stim

s = stim.TableauSimulator()

# Create a GHZ state.
s.h(0)
s.cnot(0, 1)
s.cnot(0, 2)

# Measure the GHZ state.
print(s.measure_many(0, 1, 2))  # [False, False, False] or [True, True, True]
```

Alternatively, you can compile a circuit and then begin generating samples from it:

```python
import stim

# Create a circuit that measures a large GHZ state.
c = stim.Circuit()
c.append_operation("H", [0])
for k in range(1, 30):
    c.append_operation("CNOT", [0, k])
c.append_operation("M", range(30))

# Compile the circuit into a high performance sampler.
sampler = c.compile_sampler()

# Collect a batch of samples.
# Note: the ideal batch size, in terms of speed per sample, is roughly 1024.
# Smaller batches are slower because they are not sufficiently vectorized.
# Bigger batches are slower because they use more memory.
batch = sampler.sample(1024)
print(type(batch))  # numpy.ndarray
print(batch.dtype)  # numpy.uint8
print(batch.shape)  # (1024, 30)
print(batch)
# Prints something like:
# [[1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1]
#  [0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
#  [1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1]
#  ...
#  [1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1]
#  [1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1]
#  [0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
#  [1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1]]
```

The circuit can also include noise:

```python
import stim
import numpy as np

c = stim.Circuit()
c.append_operation("X_ERROR", [0], 0.1)
c.append_operation("Y_ERROR", [1], 0.2)
c.append_operation("Z_ERROR", [2], 0.3)
c.append_operation("DEPOLARIZE1", [3], 0.4)
c.append_operation("DEPOLARIZE2", [4, 5], 0.5)
c.append_operation("M", [0, 1, 2, 3, 4, 5])

batch = c.compile_sampler().sample(2**20)
print(np.mean(batch, axis=0).round(3))
# Prints something like:
# [0.1   0.2   0.    0.267 0.267 0.266]
```

You can also sample detection events using `stim.Circuit.compile_detector_sampler`.


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
echo "
  X 0
  M 0
" | ./stim --sample
```

```
1
```

Create and measure a GHZ state, five times:

```bash
echo "
  H 0
  CNOT 0 1 0 2
  M 0 1 2
" | ./stim --sample=5
```

```
111
111
000
111
000
```

Sample several runs of a small noisy surface code with phenomenological type noise:

```bash
echo "
  REPEAT 20 {
    DEPOLARIZE1(0.001) 0 1 2 3 4 5 6 7 8
    H 3 5
    CNOT 4 1 3 6 5 8
    CNOT 2 1 8 7 3 4
    CNOT 0 1 6 7 5 4
    CNOT 4 7 3 0 5 2
    H 3 5
    M 1 7 3 5
    R 1 7 3 5
  }
  M 0 2 4 6 8
" | ./stim --sample=10
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

Sample detection events in a repetition code with circuit level noise,
include two qubit depolarizing noise when performing a CNOT.
Instead of listing all 0s and 1s, print the locations of the 1s in each line:

```bash
echo "
  M 1 3 5 7
  REPEAT 100 {
    DEPOLARIZE2(0.001) 0 1 2 3 4 5 6 7
    DEPOLARIZE1(0.001) 8
    CNOT 0 1 2 3 4 5 6 7
    DEPOLARIZE2(0.001) 8 7 6 5 4 3 2 1
    DEPOLARIZE1(0.001) 0
    CNOT 8 7 6 5 4 3 2 1
    DEPOLARIZE1(0.001) 0 1 2 3 4 5 6 7 8
    MR 1 3 5 7
    # Parity measurements should stay consistent over time.
    DETECTOR rec[-1] rec[-5]
    DETECTOR rec[-2] rec[-6]
    DETECTOR rec[-3] rec[-7]
    DETECTOR rec[-4] rec[-8]
  }
  M 0 2 4 6 8
  # Data measurements should agree with parity measurements.
  DETECTOR rec[-1] rec[-2] rec[-6]
  DETECTOR rec[-2] rec[-3] rec[-7]
  DETECTOR rec[-3] rec[-4] rec[-8]
  DETECTOR rec[-4] rec[-5] rec[-9]
  # Any one of the data qubit measurements can be the logical measurement result.
  OBSERVABLE_INCLUDE(0) rec[-1]
" | ./stim --detect=10 --out_format=hits
```

```
85,89
83


98,103,242,243
125,129,241,245
144,152,153,176,180,238,242
162,166
147
204
```

Compute the circuit's detector hypergraph (the graph whose nodes are detectors/observables and
whose edges are errors grouped into equivalence classes based on which detectors and observables they invert).
Output the graph's hyper edges as a series of lines like `error(probability) D0 D1 L2`:

```bash
echo "
  M 0 1
  H 0
  CNOT 0 1
  DEPOLARIZE1(0.01) 0
  X_ERROR(0.1) 1
  CNOT 0 1
  H 0
  M 0 1
  DETECTOR rec[-1] rec[-3]
  DETECTOR rec[-2] rec[-4]
" | ./stim --detector_hypergraph
```

```
error(0.1026756153132975941) D0
error(0.003344519141621982161) D0 D1
error(0.003344519141621982161) D1
```


## Command line flags

- `--help`:
    Prints usage examples and exits.

### Modes

Only one mode can be specified.

- `--repl`:
    Interactive mode.
    Print measurement results interactively as a circuit is typed into stdin.
    Note that this mode, unlike the other modes, prints output before operations
    that mutate the measurement record (e.g. `CNOT rec[-1] rec[-2]`) are applied.
- `--sample` or `--sample=#`:
    Measurement sampling mode.
    Output measurement results from the given circuit.
    If an integer argument is specified, run that many shots of the circuit.
- `--detect` or `--detect=#`:
    Detection event sampling mode.
    Outputs whether or not measurement sets specified by `DETECTOR` instructions have been flipped by noise.
    Assumes (does not verify) that all `DETECTOR` instructions corresponding to measurement sets with deterministic parity.
    See also `--prepend_observables`, `--append_observables`.
    If an integer argument is specified, run that many shots of the circuit.
- `--detector_hypergraph`:
    Detector graph creation mode.
    Computes equivalence classes of errors based on the detectors and observables that the error inverts.
    Each equivalence class is a hyper edge in the graph, and is weighted with a probability such that independently sampling
    each edge and inverting the associated detectors is equivalent to sampling from the original circuit.
    The output is given as series of lines like `error(probability) D1 D2 L3` where `D#` is a detector and `L#` is
    a logical observable.

### Modifiers

Modifiers tweak how a mode runs.
Not all modifiers apply to all modes.

- `--frame0`:
    Significantly improve the performance of measurement sampling mode by asserting that it is possible to take a sample
    from the given circuit where all measurement results are 0.
    Allows the frame simulator to start immediately, without waiting for a reference sample from the tableau simulator.
    If this assertion is wrong, the output samples can be corrected by xoring them against a valid sample from the circuit.
    Requires measurement sampling mode.
- `--append_observables`:
    Requires detection event sampling mode.
    In addition to outputting the values of detectors, output the values of logical observables
    built up using `OBSERVABLE_INCLUDE` instructions.
    Put these observables' values into the detection event output as if they were additional detectors at the end of the circuit.
- `--prepend_observables`:
    Requires detection event sampling mode.
    In addition to outputting the values of detectors, output the values of logical observables
    built up using `OBSERVABLE_INCLUDE` instructions.
    Put these observables' values into the detection event output as if they were additional detectors at the start of the circuit
- `--in=FILEPATH`:
    Specifies a file to read a circuit from.
    If not specified, the `stdin` pipe is used.
    Incompatible with interactive mode.
- `--out=FILEPATH`:
    Specifies a file to create or overwrite with results.
    If not specified, the `stdout` pipe is used.
    Incompatible with interactive mode.
- `--out_format=[name]`: Output format to use.
    Incompatible with interactive mode.
    Definition: a "sample" is one measurement result in measurement sampling mode or one detector/observable result in detection event sampling mode.
    Definition: a "shot" is composed of all of the samples from a circuit.
    Definition: a "sample location" is a measurement gate in measurement sampling mode, or a detector/observable in detection event sampling mode.
    - `01` (default):
        Human readable ASCII format.
        Prints all the samples from one shot before moving on to the next.
        Prints '0' or '1' for each sample.
        Prints '\n' at the end of each shot.
        Example all-true ASCII data (for 10 measurements, 4 shots):
        ```
        1111111111
        1111111111
        1111111111
        1111111111
        ```
    - `hits`:
        Human readable ASCII format.
        Writes the decimal indices of samples equal to 1, separated by a comma.
        Shots are separated by a newline.
        This format is more useful in `--detect` mode, where `1`s are rarer.
        Example all-true output data (for 10 measurements, 4 shots):
        ```
        0,1,2,3,4,5,6,7,8,9
        0,1,2,3,4,5,6,7,8,9
        0,1,2,3,4,5,6,7,8,9
        0,1,2,3,4,5,6,7,8,9
        ```
    - `dets`:
        Human readable ASCII format.
        Similar to `hits`, except each line is prefixed by `shot `, hits are separated by spaces, and each hit is
        prefixed by a character indicating its type (`M` for measurement, `D` for detector, `L` for logical observable).
        Example output data (for 3 detectors, 2 observables):
        ```
        shot
        shot L0 L1 D0 D1 D2
        shot L0 D0
        ```
    - `b8`:
        Binary format.
        Writes all the samples from one shot before moving on to the next.
        The number of samples is padded up to a multiple of 8 using fake samples set to 0.
        Samples are combined into groups of 8.
        The sample results for a group are bit packed into a byte, ordered from least significant bit to most significant bit.
        The byte for the first sample group is printed, then the second, and so forth for all groups in order.
        There is no separator between shots (other than the fake zero sample padding).
        Example all-true output hex data (for 10 measurements, 4 shots):
        ```
        FF 30 FF 30 FF 30 FF 30
        ```
    - `ptb64`:
        Partially transposed binary format.
        The number of shots is padded up to a multiple of 64 using fake shots where all samples are 0.
        Shots are combined into groups of 64.
        All the samples from one shot group are written before moving on to the next group.
        Within a shot group, each of the circuit sample locations has 64 results (one from each shot).
        These 64 bits of information are packed into 8 bytes, ordered from first file byte to last file byte and then least significant bit to most significant bit.
        The 8 bytes for the first sample location are output, then the 8 bytes for the next, and so forth for all sample locations.
        There is no separator between shot groups (the reader must know how many measurements are expected).
        Example all-true output hex data (for 3 measurements, 81 shots):
        ```
        FF FF FF FF FF FF FF FF
        FF FF FF FF FF FF FF FF
        FF FF FF FF FF FF FF FF
        FF FF F1 00 00 00 00 00
        FF FF F1 00 00 00 00 00
        FF FF F1 00 00 00 00 00
        ```
    - `r8`:
        Binary run-length format.
        Each byte is the length of a run of samples that were all False.
        If the run length is non-maximal (less than 255), the next measurement result is a 1.
        For example, `0x00` means `[1]`, `0x03` means `[0, 0, 0, 1]`,
        `0xFE` means `[0] * 254 + [1]`,
        and `0xFF`   means `[0] * 255`.
        A fake "True" sample is appended to the end of each shot, and the data for a shot ends on the byte that decodes
        to produce this fake appended True.
        Note that this means the reader must know how many measurement results are expected,
        and that the data will never end with a `0xFF`.
        There is no separator between shots, other than padding implicit in appending the fake "True" samples.
        Example data for an all-false shot then an all-true shot then an all-false shot (with 5 measurements per shot):
        ```
        0x05 0x00 0x00 0x00 0x00 0x00 0x00 0x05
        ```
      Note the extra `0x00` due to the fake appended True.
- `--distance=#`:
    Distance to use in circuit generation mode.
    Defaults to 3.
- `--rounds=#`:
    Number of rounds to use in circuit generation mode.
    Defaults to the same as distance.
- `--noise_level=%f`:
    Strength of depolarizing noise, from 0 to 1, to insert into generated circuits.
    Defaults to 0 (none).

## Supported Gates

### General facts about all gates.

- **Qubit Targets**:
    Qubits are referred to by non-negative integers.
    There is a qubit `0`, a qubit `1`, and so forth (up to an implemented-defined maximum of `16777215`).
    For example, the line `X 2` says to apply an `X` gate to qubit `2`.
    Beware that touching qubit `999999` implicitly tells simulators to resize their internal state to accommodate a million qubits.

- **Measurement Record Targets**:
    Measurement results are referred to by `rec[-#]` arguments, where the index within
    the square brackets uses python-style negative indices to refer to the end of the
    growing measurement record.
    For example, `CNOT rec[-1] 3` says "toggle qubit `3` if the most recent measurement returned `True`
    and `CZ 1 rec[-2]` means "phase flip qubit `1` if the second most recent measurement returned `True`.
    There is implementation-defined maximum lookback of `-16777215` when accessing the measurement record.
    Non-negative indices are not permitted.

- **Broadcasting**:
    Most gates support broadcasting over multiple targets.
    For example, `H 0 1 2` will broadcast a Hadamard gate over qubits `0`, `1`, and `2`.
    Two qubit gates can also broadcast, and do so over aligned pair of targets.
    For example, `CNOT 0 1 2 3` will apply `CNOT 0 1` and then `CNOT 2 3`.
    Broadcasting is always evaluated in left-to-right order.

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
- `CNOT` (alternate names `CX`, `ZCX`):
    Controlled NOT operation.
    Qubit pairs are in name order (first qubit is the control, second is the target).
    This gate can be controlled by on the measurement record.
    Examples: unitary `CNOT 1 2`, feedback `CNOT rec[-1] 4`.
- `CY` (alternate name `ZCY`):
    Controlled Y operation.
    Qubit pairs are in name order (first qubit is the control, second is the target).
    This gate can be controlled by on the measurement record.
    Examples: unitary `CY 1 2`, feedback `CY rec[-1] 4`.
- `CZ` (alternate name `ZCZ`):
    Controlled Z operation.
    This gate can be controlled by on the measurement record.
    Examples: unitary `CZ 1 2`, feedback `CZ rec[-1] 4` or `CZ 4 rec[-1]`.
- `YCZ`:
    Y-basis-controlled Z operation (i.e. the reversed-argument-order controlled-Y).
    Qubit pairs are in name order.
    This gate can be controlled by on the measurement record.
    Examples: unitary `YCZ 1 2`, feedback `YCZ 4 rec[-1]`.
- `YCY`: Y-basis-controlled Y operation.
- `YCX`: Y-basis-controlled X operation. Qubit pairs are in name order.
- `XCZ`:
    X-basis-controlled Z operation (i.e. the reversed-argument-order controlled-not).
    Qubit pairs are in name order.
    This gate can be controlled by on the measurement record.
    Examples: unitary `XCZ 1 2`, feedback `XCZ 4 rec[-1]`.
- `XCY`: X-basis-controlled Y operation. Qubit pairs are in name order.
- `XCX`: X-basis-controlled X operation.

### Collapsing gates

- `M`:
    Z-basis measurement.
    Examples: `M 0`, `M 2 1`, `M 0 !3 1 2`.
    Collapses the target qubits and reports their values (optionally flipped).
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
- `MR`:
    Z-basis measurement and reset.
    Examples: `MR 0`, `MR 2 1`, `MR 0 !3 1 2`.
    Collapses the target qubits, reports their values (optionally flipped), then resets them to the |0> state.
    Prefixing a target with a `!` indicates that the measurement result should be inverted when reported.
    (The ! does not change that the qubit is reset to |0>.)
    In the tableau simulator, this operation may require a transpose and so is more efficient when grouped
    (e.g. prefer `MR 0 1 \n H 0` over `MR 0 \n H 0 \n MR 1`).

### Noise Gates

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
- `X_ERROR(p)`:
    Single-qubit probabilistic X error.
    Examples: `X_ERROR(0.001) 0 1`.
    For each target qubit, independently applies an X gate With probability `p`.
- `Y_ERROR(p)`:
    Single-qubit probabilistic Y error.
    Examples: `Y_ERROR(0.001) 0 1`.
    For each target qubit, independently applies a Y gate With probability `p`.
- `Z_ERROR(p)`:
    Single-qubit probabilistic Z error.
    Examples: `Z_ERROR(0.001) 0 1`.
    For each target qubit, independently applies a Z gate With probability `p`.
- `CORRELATED_ERROR(p)` (alternate name `E`) and `ELSE_CORRELATED_ERROR(p)`:
    Pauli product error cases.
    Probabilistically applies a Pauli product error with probability `p`,
    unless the "correlated error occurred" flag is already set.
    `CORRELATED_ERROR` is equivalent to `ELSE_CORRELATED_ERROR` except that
    `CORRELATED_ERROR` starts by clearing the "correlated error occurred" flag.
    Both operations set the "correlated error occurred" flag if they apply their error.
    Example:

        # With 40% probability, uniformly pick X1*Y2 or Z2*Z3 or X1*Y2*Z3.
        CORRELATED_ERROR(0.2) X1 Y2
        ELSE_CORRELATED_ERROR(0.25) Z2 Z3
        ELSE_CORRELATED_ERROR(0.33333333333) X1 Y2 Z3

### Annotations

- `DETECTOR`:
    Asserts that a set of measurements have a deterministic result,
    and that this result changing can be used to detect errors.
    Ignored in measurement sampling mode.
    In detection sampling mode, a detector produces a sample indicating if it was inverted by  noise or not.
    Example: `DETECTOR rec[-1] rec[-2]`.
- `OBSERVABLE_INCLUDE(k)`:
    Adds physical measurement locations to a specified logical observable.
    The logical measurement result is the parity of all physical measurements added to it.
    Behaves similarly to a Detector, except observables can be built up globally over the entire circuit instead of being defined locally.
    Ignored in measurement sampling mode.
    In detection sampling mode, a logical observable can produce a sample indicating if it was inverted by  noise or not.
    These samples are dropped or put before or after detector samples, depending on command line flags.
    Examples: `OBSERVABLE_INCLUDE(0) rec[-1] rec[-2]`, `OBSERVABLE_INCLUDE(3) rec[-7]`.

### Other

- `TICK`: Optional command indicating the end of a layer of gates.
    May be ignored, may force processing of internally queued operations and flushing of queued measurement results.
- `REPEAT N { ... }`: Repeats the instructions in its body N times.


# Building

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

### Python Package Build

Environment requirements:

```bash
pip install -y pybind11 cibuildwheel
```

Build source distribution (fallback for missing binary wheels):

```bash
python setup.py sdist
```

Output in `dist` directory.

Build manylinux binary distributions (takes 30+ minutes):

```bash
python -m cibuildwheel --output-dir wheelhouse --platform=linux
```

Output in `wheelhouse` directory.

Build `stim_cirq` package:

```bash
cd glue/cirq
python setup.py sdist
```

Output in `glue/cirq/dist` directory.

# Testing

### Run tests using CMAKE

Unit testing with CMAKE requires GTest to be installed on your system and discoverable by CMake.
Follow the ["Standalone CMake Project" from the GTest README](https://github.com/google/googletest/tree/master/googletest).

Run tests with address and memory sanitization, but without optimizations:

```bash
cmake .
make stim_test
./out/stim_test
```

To force AVX vectorization, SSE vectorization, or no vectorization
pass `-DSIMD_WIDTH=256` or `-DSIMD_WIDTH=128` or -DSIMD_WIDTH=64` to the `cmake` command.

Run tests with optimizations without sanitization:

```bash
cmake .
make stim_test_o3
./out/stim_test_o3
```

### Run tests using Bazel

Run tests with whatever settings Bazel feels like using:

```bash
bazel :stim_test
```

### Run python binding tests

In a fresh virtual environment:

```bash
pip install -e .
pip install -y numpy pytest
python -m pytest src
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

The benchmark binary supports a `--only=BENCHMARK_NAME` filter flag.
Multiple filters can be specified by separating them with commas `--only=A,B`.
Ending a filter with a `*` turns it into a prefix filter `--only=sim_*`.
