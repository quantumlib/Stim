# Stim

Stim is a fast simulator for quantum stabilizer circuits.
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
Once a reference sample from the circuit is available, more samples can be derived by propagating Pauli frames through the circuit,
using the original sample as a template whose results are flipped or not flipped by the passing Pauli frame.
As long as all errors are probabilistic Pauli operations, and as long as 50/50 probability Z errors are placed after
every reset and every measurement, the derived samples will come from the same distribution as a full stabilizer simulation.
This ensures every gate has a worst case complexity of O(1), instead of O(n) or O(n^2).

Third, data is laid out in a cache friendly way and operated on using vectorized 256-bit-wide SIMD instructions.
This makes key operations fast.
For example, Stim can multiply a Pauli string with a *hundred billion terms* into another in *under a second*.
Pauli string multiplication is a key bottleneck operation when updating a stabilizer tableau.
Tracking Pauli frames can also benefit from vectorization, by batching the frames into groups of hundreds that are
all operated on simultaneously by individual CPU instructions.

# Building

See the [developer documentation](dev/README.md).

# Usage (python)

See the [python documentation](glue/python/README.md).

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

- **`--help`**:
    Print usage examples and exit.

- **`--repl`**:
    **Interactive mode**.
    Print measurement results interactively as a circuit is typed into stdin.
- **`--sample`** or **`--sample=#`**:
    **Measurement sampling mode**.
    Output measurement results from the given circuit.
    If an integer argument is specified, run that many shots of the circuit.

    - **`--frame0`**:
        Significantly improve the performance of measurement sampling mode by asserting that it is possible to take a sample
        from the given circuit where all measurement results are 0.
        Allows the frame simulator to start immediately, without waiting for a reference sample from the tableau simulator.
        If this assertion is wrong, the output samples can be corrected by xoring them against a valid sample from the circuit.

- **`--detect`** or **`--detect=#`**:
    Detection event sampling mode.
    Outputs whether or not measurement sets specified by `DETECTOR` instructions have been flipped by noise.
    Assumes (does not verify) that all `DETECTOR` instructions corresponding to measurement sets with deterministic parity.
    See also `--append_observables`.
    If an integer argument is specified, run that many shots of the circuit.

    - **`--append_observables`**:
        In addition to outputting the values of detectors, output the values of logical observables
        built up using `OBSERVABLE_INCLUDE` instructions.
        Put these observables' values into the detection event output as if they were additional detectors at the end of the circuit.

- **`--detector_hypergraph`**:
    **Detector error model creation mode**.
    Determines the detectors and logical observables flipped by error channels in the input circuit.
    Outputs lines like `error(p) D2 D3 L5`, which means that there is an independent error
    mechanism that occurs with probability `p` that flips detector 2, detector 3, and logical observable 5.
    Detectors are numbered, starting from 0, based on the order they appear in the circuit.
    Observables are numbered based on the observable indices specified in the circuit.
    
    Doesn't support `ELSE_CORRELATED_ERROR`.
    
    - **`--find_reducible_errors`**:
    
        When applying a compound error channel (e.g. a depolarization channel), check if each case can be
        reduced to the single-detector and double-detector cases. For example, if the `X` and `Y` parts of
        the depolarizing channel each produce one detection event (e.g. `D5` and `D6`), and the `Z` part
        produces both those detections events, then instead of the `Z` part becoming `error(p) D5 D6` it
        will become `reducible_error(p) D5 ^ D6`.
    
        For example, this mode will automatically decompose errors that cross between the X and Z detector
        graphs of a surface code into components on just the X graph and components on just the Z graph. And
        it does so in a basis-independent fashion (so e.g. it will still do the right thing when using an XY
        or XZZX surface code instead of an XZ surface code.)
    
        This mode currently requires that every case of a compound error channel can be reduced to
        single-detector components accompanied by at most two double-detector components.    

- **`--gen=surface_code|repetition_code|color_code`**:
    **Circuit generation mode**.
    Generates stim circuit files for common circuit constructions.
    Includes annotations for detectors, logical observables, and the passage of time.
    Has configurable noise.

    - **`--task=[name]`**: What the generated circuit should do; the experiment it should run.
    Different error correcting codes support different tasks.

        - `memory` (repetition_code):
            Initialize a logical `|0>`,
            preserve it against noise for the given number of rounds,
            then measure.
        - `rotated_memory_x` (surface_code):
            Initialize a logical `|+>` in a rotated surface code,
            preserve it against noise for the given number of rounds,
            then measure in the X basis.
        - `rotated_memory_z` (surface_code):
            Initialize a logical `|0>` in a rotated surface code,
            preserve it against noise for the given number of rounds,
            then measure in the X basis.
        - `unrotated_memory_x` (surface_code):
            Initialize a logical `|+>` in an unrotated surface code,
            preserve it against noise for the given number of rounds,
            then measure in the Z basis.
        - `unrotated_memory_z` (surface_code):
            Initialize a logical `|0>` in an unrotated surface code,
            preserve it against noise for the given number of rounds,
            then measure in the Z basis.
        - `memory_xyz` (color_code):
            Initialize a logical `|0>`,
            preserve it against noise for the given number of rounds,
            then measure.
            Use a color code that alternates between measuring X, then Y, then Z stabilizers.

    - **`--distance=#`**:
        The minimum number of physical errors needed to cause a logical error.
        Determines how large the generated circuit has to be.

    - **`--rounds=#`**:
        The number of times the circuit's measurement qubits are measured.
        Different tasks have different minimum numbers of rounds.

    - **`--after_clifford_depolarization=p`**:
        Adds a `DEPOLARIZE1(p)` operation after every single-qubit Clifford operation
        and a `DEPOLARIZE2(p)` noise operation after every two-qubit Clifford operation.
        Must be a number between 0 and 1.
        When set to 0, the noise operations are not inserted.
        Defaults to 0.

    - **`--after_reset_flip_probability=p`**:
        Adds an `X_ERROR(p)` after `R` (`RZ`) and `RY` operations, and a `Z_ERROR(p)` after `RX` operations.
        Must be a number between 0 and 1.
        When set to 0, the noise operations are not inserted.
        Defaults to 0.

    - **`--before_measure_flip_probability=p`**:
        Adds an `X_ERROR(p)` before `M` (`MZ`) and `MY` operations, and a `Z_ERROR(p)` before `MX` operations.
        Must be a number between 0 and 1.
        When set to 0, the noise operations are not inserted.
        Defaults to 0.

    - **`--before_round_data_depolarization=p`**
        Phenomenological noise.
        Adds a `DEPOLARIZE1(p)` operation to each data qubit at the start of each round of stabilizer measurements.
        Must be a number between 0 and 1.
        When set to 0, the noise operations are not inserted.
        Defaults to 0.

- **`--in=FILEPATH`**:
    Specifies a file to read a circuit from.
    If not specified, the `stdin` pipe is used.

- **`--out=FILEPATH`**:
    Specifies a file to create or overwrite with results.
    If not specified, the `stdout` pipe is used.

- **`--out_format=[name]`**: Output format to use.
    Requires measurement sampling mode or detection sample mode.
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
        There is no separator between shots (other than the fake zero sample padding up to a multiple of 8).
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
      Note the sixth `0x00` due to the fake appended True.

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
    
    (The reason the measurement record indexing is relative to the present is so that it can be used in loops.)

- **Broadcasting**:
    Most gates support broadcasting over multiple targets.
    For example, `H 0 1 2` will broadcast a Hadamard gate over qubits `0`, `1`, and `2`.
    Two qubit gates can also broadcast, and do so over aligned pair of targets.
    For example, `CNOT 0 1 2 3` will apply `CNOT 0 1` and then `CNOT 2 3`.
    Broadcasting is always evaluated in left-to-right order.

### Single qubit gates

- **`Z`**: Pauli Z gate. Phase flip.
- **`Y`**: Pauli Y gate.
- **`X`**: Pauli X gate. Bit flip.
- **`H`** (alternate name **`H_XZ`**): Hadamard gate. Swaps the X and Z axes. Unitary equals (X + Z) / sqrt(2).
- **`H_XY`**: Variant of the Hadamard gate that swaps the X and Y axes (instead of X and Z). Unitary equals (X + Y) / sqrt(2).
- **`H_YZ`**: Variant of the Hadamard gate that swaps the Y and Z axes (instead of X and Z). Unitary equals (Y + Z) / sqrt(2).
- **`S`** (alternate name **`SQRT_Z`**): Principle square root of Z gate. Equal to `diag(1, i)`.
- **`S_DAG`** (alternate name **`SQRT_Z_DAG`**): Adjoint square root of Z gate. Equal to `diag(1, -i)`.
- **`SQRT_Y`**: Principle square root of Y gate. Equal to `H_YZ*S*H_YZ`.
- **`SQRT_Y_DAG`**: Adjoint square root of Y gate. Equal to `H_YZ*S_DAG*H_YZ`.
- **`SQRT_X`**: Principle square root of X gate. Equal to `H*S*H`.
- **`SQRT_X_DAG`**: Adjoint square root of X gate. Equal to `H*S_DAG*H`.
- **`C_XYZ`**: Right handed period 3 axis cycling gate. Sends X -> Y -> Z -> X. Rotates +120 degrees around X+Y+Z.
- **`C_ZYX`**: Left handed period 3 axis cycling gate. Sends Z -> Y -> X -> Z. Rotates -120 degrees around X+Y+Z. Inverse of `C_XYZ`.
- **`I`**: Identity gate. Does nothing. Why is this even here? Probably out of a misguided desire for closure.

### Two qubit gates

- **`SWAP`**: Swaps two qubits.
- **`ISWAP`**: Swaps two qubits while phasing the ZZ observable by i. Equal to `SWAP * CZ * (S tensor S)`.
- **`ISWAP_DAG`**: Swaps two qubits while phasing the ZZ observable by -i. Equal to `SWAP * CZ * (S_DAG tensor S_DAG)`.
- **`CNOT`** (alternate names **`CX`**, **`ZCX`**):
    Controlled NOT operation.
    Qubit pairs are in name order (first qubit is the control, second is the target).
    This gate can be controlled by on the measurement record.
    Examples: unitary `CNOT 1 2`, feedback `CNOT rec[-1] 4`.
- **`CY`** (alternate name **`ZCY`**):
    Controlled Y operation.
    Qubit pairs are in name order (first qubit is the control, second is the target).
    This gate can be controlled by on the measurement record.
    Examples: unitary `CY 1 2`, feedback `CY rec[-1] 4`.
- **`CZ`** (alternate name **`ZCZ`**):
    Controlled Z operation.
    This gate can be controlled by on the measurement record.
    Examples: unitary `CZ 1 2`, feedback `CZ rec[-1] 4` or `CZ 4 rec[-1]`.
- **`YCZ`**:
    Y-basis-controlled Z operation (i.e. the reversed-argument-order controlled-Y).
    Qubit pairs are in name order.
    This gate can be controlled by on the measurement record.
    Examples: unitary `YCZ 1 2`, feedback `YCZ 4 rec[-1]`.
- **`YCY`**: Y-basis-controlled Y operation.
- **`YCX`**: Y-basis-controlled X operation. Qubit pairs are in name order.
- **`XCZ`**:
    X-basis-controlled Z operation (i.e. the reversed-argument-order controlled-not).
    Qubit pairs are in name order.
    This gate can be controlled by on the measurement record.
    Examples: unitary `XCZ 1 2`, feedback `XCZ 4 rec[-1]`.
- **`XCY`**: X-basis-controlled Y operation. Qubit pairs are in name order.
- **`XCX`**: X-basis-controlled X operation.

### Collapsing gates

- **`M`** (alternate name **`MZ`**):
    Z-basis measurement.
    Examples: `M 0`, `M 2 !3 5`.
    Projects the target qubits into `|0>` or `|1>`and reports their values (false=`|0>`, true=`|1>`).
    Prefixing a target with a `!` indicates that the measurement result should be inverted when reported.
- **`MX`**:
    X-basis measurement.
    Examples: `MX 0`, `MX 2 !3 5`.
    Projects the target qubits into `|+>` or `|->`and reports their values (false=`|+>`, true=`|->`).
    Prefixing a target with a `!` indicates that the measurement result should be inverted when reported.
- **`MY`**:
    Y-basis measurement.
    Examples: `MY 0`, `MY 2 !3 5`.
    Projects the target qubits into `|i>` or `|-i>`and reports their values (false=`|i>`, true=`|-i>`).
    Prefixing a target with a `!` indicates that the measurement result should be inverted when reported.
- **`R`** (alternate name **`RZ`**):
    Reset to `|0>`.
    Examples: `R 0`, `R 2 1`, `R 0 3 1 2`.
    Silently measures the target qubits in the Z basis and applies an `X` to the ones found to be in the `|1>` state.
- **`RX`**:
    Reset to `|+>`.
    Examples: `RX 0`, `RX 2 5 3`.
    Silently measures the target qubits in the X basis and applies a `Z` to the ones found to be in the `|->` state.
- **`RY`**:
    Reset to `|i>`.
    Examples: `RY 0`, `RY 2 5 3`.
    Silently measures the target qubits in the Y basis and applies an `X` to the ones found to be in the `|-i>` state.
- **`MR`** (alternate name **`MRZ`**):
    Z-basis demolition measurement.
    A measurement combined with a reset.
    Examples: `MR 0`, `MR 2 !5 3`.
    Note that `MR 0 0` is equivalent to `M 0` then `R 0` then `M 0` then `R 0`, not to `M 0 0` then `R 0 0`.
    Prefixing a target with a `!` indicates that the measurement result should be inverted when reported
    (it does not change that the qubit is reset to `|0>`).
- **`MRX`**:
    X-basis demolition measurement.
    A measurement combined with a reset.
    Examples: `MRX 0`, `MRX 2 !5 3`.
    Note that `MRX 0 0` is equivalent to `MX 0` then `RX 0` then `MX 0` then `RX 0`, not to `MX 0 0` then `RX 0 0`.
    Prefixing a target with a `!` indicates that the measurement result should be inverted when reported
    (it does not change that the qubit is reset to `|+>`).
- **`MRY`**:
    Y-basis demolition measurement.
    A measurement combined with a reset.
    Examples: `MRY 0`, `MRY 2 !5 3`.
    Note that `MRY 0 0` is equivalent to `MY 0` then `RY 0` then `MY 0` then `RY 0`, not to `MY 0 0` then `RY 0 0`.
    Prefixing a target with a `!` indicates that the measurement result should be inverted when reported
    (it does not change that the qubit is reset to `|i>`).

### Noise Gates

- **`DEPOLARIZE1(p)`**:
    Single qubit depolarizing error.
    Examples: `DEPOLARIZE1(0.001) 1`, `DEPOLARIZE1(0.0003) 0 2 4 6`.
    With probability `p`, applies independent single-qubit depolarizing kicks to the given qubits.
    A single-qubit depolarizing kick is `X`, `Y`, or `Z` chosen uniformly at random.
- **`DEPOLARIZE2(p)`**:
    Two qubit depolarizing error.
    Examples: `DEPOLARIZE2(0.001) 0 1`, `DEPOLARIZE2(0.0003) 0 2 4 6`.
    With probability `p`, applies independent two-qubit depolarizing kicks to the given qubit pairs.
    A two-qubit depolarizing kick is
    `IX`, `IY`, `IZ`, `XI`, `XX`, `XY`, `XZ`, `YI`, `YX`, `YY`, `YZ`, `ZI`, `ZX`, `ZY`, `ZZ`
    chosen uniformly at random.
- **`X_ERROR(p)`**:
    Single-qubit probabilistic X error.
    Examples: `X_ERROR(0.001) 0 1`.
    For each target qubit, independently applies an X gate With probability `p`.
- **`Y_ERROR(p)`**:
    Single-qubit probabilistic Y error.
    Examples: `Y_ERROR(0.001) 0 1`.
    For each target qubit, independently applies a Y gate With probability `p`.
- **`Z_ERROR(p)`**:
    Single-qubit probabilistic Z error.
    Examples: `Z_ERROR(0.001) 0 1`.
    For each target qubit, independently applies a Z gate With probability `p`.
- **`CORRELATED_ERROR(p)`** (alternate name **`E`**)
    See `ELSE_CORRELATED_ERROR`.
    `CORRELATED_ERROR` is equivalent to `ELSE_CORRELATED_ERROR` except that
    `CORRELATED_ERROR` starts by clearing the "correlated error occurred" flag.
- **`ELSE_CORRELATED_ERROR(p)`**:
    Pauli product error cases.
    Probabilistically applies a Pauli product error with probability `p`,
    unless the "correlated error occurred" flag is already set.
    Sets the "correlated error occurred" flag if the error is applied.
    Example:

        # With 40% probability, uniformly pick X1*Y2 or Z2*Z3 or X1*Y2*Z3.
        CORRELATED_ERROR(0.2) X1 Y2
        ELSE_CORRELATED_ERROR(0.25) Z2 Z3
        ELSE_CORRELATED_ERROR(0.33333333333) X1 Y2 Z3

### Annotations

- **`DETECTOR`**:
    Asserts that a set of measurements have a deterministic result,
    and that this result changing can be used to detect errors.
    Ignored in measurement sampling mode.
    In detection sampling mode, a detector produces a sample indicating if it was inverted by  noise or not.
    Example: `DETECTOR rec[-1] rec[-2]`.
- **`OBSERVABLE_INCLUDE(k)`**:
    Adds measurement results to a logical observable.
    A logical observable's measurement result is the parity of all physical measurement results added to it.
    Behaves similarly to a Detector, except observables can be built up incrementally over the entire circuit.
    Ignored in measurement sampling mode.
    In detection sampling mode, a logical observable can produce a sample indicating if it was inverted by  noise or not.
    These samples are dropped or put before or after detector samples, depending on command line flags.
    Examples: `OBSERVABLE_INCLUDE(0) rec[-1] rec[-2]`, `OBSERVABLE_INCLUDE(3) rec[-7]`.

### Other

- **`TICK`**:
    Indicates the end of a layer of gates, or that time is advancing.
    Used by `stimcirq` to preserve the "moment structure" of cirq circuits converted to/from stim circuits.
    Examples: `TICK`, `TICK`, and of course `TICK`.
    
- **`REPEAT N { ... }`**:
    Repeats the instructions in its body N times.
    The implementation-defined maximum value of N is 9223372036854775807.
    Example:
    ```
    REPEAT 2 {
        CNOT 0 1
        CNOT 2 1
        M 1
    }
    REPEAT 10000000 {
        CNOT 0 1
        CNOT 2 1
        M 1
        DETECTOR rec[-1] rec[-3]
    }
    ```
