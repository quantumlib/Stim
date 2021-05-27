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

    `--help gates` lists all available gates.

    `--help [gatename]` prints information about a gate.

- **`--repl`**:
    **Interactive mode**.
    Print measurement results interactively as a circuit is typed into stdin.
- **`--sample`** or **`--sample=#`**:
    **Measurement sampling mode**.
    Output measurement results from the given circuit.
    If an integer argument is specified, run that many shots of the circuit.
    The maximum shot count is 9223372036854775807.

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
    The maximum shot count is 9223372036854775807.

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
    
    - **`--fold_loops`**:
        Allows the output error model to contain `REPEAT # {}` blocks and `TICK #` instructions.
        This makes it feasible to get error models for circuits with REPEAT blocks that repeat
        instructions millions or trillions of times.

        When interpreting the output error model, TICK instructions increase an accumulator
        which is added into all later detector ids. For example, the following three error models
        are equivalent:

        1. ```
           error(0.1) D1 L2
           error(0.1) D5 L2
           error(0.1) D9 L2
           ```
        2. ```
           error(0.1) D1 L2
           TICK 3
           error(0.1) D2 L2
           TICK 2
           error(0.1) D4 L2
           ```
        3. ```
           REPEAT 3 {
               error(0.1) D1 L2
               TICK 4
           }
           ```

        When `--fold_loops` is enabled, stim attempts to turn circuit REPEAT blocks into error model REPEAT blocks.
        It does so by using a ["tortoise and hare"](https://en.wikipedia.org/wiki/Cycle_detection#Floyd's_tortoise_and_hare)
        period finding algorithm. Stim has state tracking which detectors and observables have sensitivities
        overlapping the X and Z observables of each qubit at the end of each loop iteration. If this state recurs,
        loop folding succeeds.
        (This is slightly more complicated than it sounds, because each loop iteration introduces
        new detectors.
        The actual collision being checked for is a collision when all detector IDs are shifted to account for the new
        detectors introduced during each iteration.)

        Note that a loop might have a recurrence period that's more than 1 iteration.
        For example, in the color code memory_xyz circuit generated by stim, the recurrence period is 3 iterations of
        the main loop (because the loop applies C_XYZ gates to the data qubits and C_XYZ has period 3).

        CAUTION: Although logical observables can "cross" the loop without preventing loop folding, detectors
        CANNOT. If there is any detector introduced after the loop, whose sensitivity region extends to before the loop,
        loop folding will fail. This is disastrous for loops with repetition counts in the billions, because in that
        case loop folding is the difference between the error analysis finishing in seconds instead of days.

    - **`--find_reducible_errors`**:
        Allows the output error model to contain `reducible_error` instructions.

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
        Different tasks place different constraints on the code distances (e.g. sometimes the code distance must be odd).

    - **`--rounds=#`**:
        The number of times the circuit's measurement qubits are measured.
        Different tasks have different minimum numbers of rounds.
        Maximum rounds is 9223372036854775807.
        Different tasks place different constraints on the number of rounds (e.g. usually can't be zero).

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

## Index

- [CNOT](#CNOT)
- [CORRELATED_ERROR](#CORRELATED_ERROR)
- [CX](#CX)
- [CY](#CY)
- [CZ](#CZ)
- [C_XYZ](#C_XYZ)
- [C_ZYX](#C_ZYX)
- [DEPOLARIZE1](#DEPOLARIZE1)
- [DEPOLARIZE2](#DEPOLARIZE2)
- [DETECTOR](#DETECTOR)
- [E](#E)
- [ELSE_CORRELATED_ERROR](#ELSE_CORRELATED_ERROR)
- [H](#H)
- [H_XY](#H_XY)
- [H_XZ](#H_XZ)
- [H_YZ](#H_YZ)
- [I](#I)
- [ISWAP](#ISWAP)
- [ISWAP_DAG](#ISWAP_DAG)
- [M](#M)
- [MR](#MR)
- [MRX](#MRX)
- [MRY](#MRY)
- [MRZ](#MRZ)
- [MX](#MX)
- [MY](#MY)
- [MZ](#MZ)
- [OBSERVABLE_INCLUDE](#OBSERVABLE_INCLUDE)
- [R](#R)
- [REPEAT](#REPEAT)
- [RX](#RX)
- [RY](#RY)
- [RZ](#RZ)
- [S](#S)
- [SQRT_X](#SQRT_X)
- [SQRT_X_DAG](#SQRT_X_DAG)
- [SQRT_Y](#SQRT_Y)
- [SQRT_Y_DAG](#SQRT_Y_DAG)
- [SQRT_Z](#SQRT_Z)
- [SQRT_Z_DAG](#SQRT_Z_DAG)
- [SWAP](#SWAP)
- [S_DAG](#S_DAG)
- [TICK](#TICK)
- [X](#X)
- [XCX](#XCX)
- [XCY](#XCY)
- [XCZ](#XCZ)
- [X_ERROR](#X_ERROR)
- [Y](#Y)
- [YCX](#YCX)
- [YCY](#YCY)
- [YCZ](#YCZ)
- [Y_ERROR](#Y_ERROR)
- [Z](#Z)
- [ZCX](#ZCX)
- [ZCY](#ZCY)
- [ZCZ](#ZCZ)
- [Z_ERROR](#Z_ERROR)

## Pauli Gates

- <a name="I"></a>**`I`**
    
    Identity gate.
    Does nothing to the target qubits.
    
    - Example:
    
        ```
        I 5
        I 42
        I 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +X
        Z -> +Z
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: 
        Angle: 0 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ]
        [    , +1  ]
        ```
        
    
- <a name="X"></a>**`X`**
    
    Pauli X gate.
    The bit flip gate.
    
    - Example:
    
        ```
        X 5
        X 42
        X 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +X
        Z -> -Z
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +X
        Angle: 180 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [    , +1  ]
        [+1  ,     ]
        ```
        
    
- <a name="Y"></a>**`Y`**
    
    Pauli Y gate.
    
    - Example:
    
        ```
        Y 5
        Y 42
        Y 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> -X
        Z -> -Z
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +Y
        Angle: 180 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [    ,   -i]
        [  +i,     ]
        ```
        
    
- <a name="Z"></a>**`Z`**
    
    Pauli Z gate.
    The phase flip gate.
    
    - Example:
    
        ```
        Z 5
        Z 42
        Z 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> -X
        Z -> +Z
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +Z
        Angle: 180 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ]
        [    , -1  ]
        ```
        
    
## Single Qubit Clifford Gates

- <a name="C_XYZ"></a>**`C_XYZ`**
    
    Right handed period 3 axis cycling gate, sending X -> Y -> Z -> X.
    
    - Example:
    
        ```
        C_XYZ 5
        C_XYZ 42
        C_XYZ 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +Y
        Z -> +X
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +X+Y+Z
        Angle: 120 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1-i, -1-i]
        [+1-i, +1+i] / 2
        ```
        
    
- <a name="C_ZYX"></a>**`C_ZYX`**
    
    Left handed period 3 axis cycling gate, sending Z -> Y -> X -> Z.
    
    - Example:
    
        ```
        C_ZYX 5
        C_ZYX 42
        C_ZYX 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +Z
        Z -> +Y
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +X+Y+Z
        Angle: -120 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1+i, +1+i]
        [-1+i, +1-i] / 2
        ```
        
    
- <a name="H"></a>**`H`**
    
    Alternate name: <a name="H_XZ"></a>`H_XZ`
    
    The Hadamard gate.
    Swaps the X and Z axes.
    A 180 degree rotation around the X+Z axis.
    
    - Example:
    
        ```
        H 5
        H 42
        H 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +Z
        Z -> +X
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +X+Z
        Angle: 180 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  , +1  ]
        [+1  , -1  ] / sqrt(2)
        ```
        
    
- <a name="H_XY"></a>**`H_XY`**
    
    A variant of the Hadamard gate that swaps the X and Y axes (instead of X and Z).
    A 180 degree rotation around the X+Y axis.
    
    - Example:
    
        ```
        H_XY 5
        H_XY 42
        H_XY 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +Y
        Z -> -Z
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +X+Y
        Angle: 180 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [    , +1-i]
        [+1+i,     ] / sqrt(2)
        ```
        
    
- <a name="H_YZ"></a>**`H_YZ`**
    
    A variant of the Hadamard gate that swaps the Y and Z axes (instead of X and Z).
    A 180 degree rotation around the Y+Z axis.
    
    - Example:
    
        ```
        H_YZ 5
        H_YZ 42
        H_YZ 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> -X
        Z -> +Y
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +Y+Z
        Angle: 180 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,   -i]
        [  +i, -1  ] / sqrt(2)
        ```
        
    
- <a name="S"></a>**`S`**
    
    Alternate name: <a name="SQRT_Z"></a>`SQRT_Z`
    
    Principle square root of Z gate.
    Phases the amplitude of |1> by i.
    
    - Example:
    
        ```
        S 5
        S 42
        S 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +Y
        Z -> +Z
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +Z
        Angle: 90 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ]
        [    ,   +i]
        ```
        
    
- <a name="SQRT_X"></a>**`SQRT_X`**
    
    Principle square root of X gate.
    Phases the amplitude of |-> by i.
    Equivalent to `H` then `S` then `H`.
    
    - Example:
    
        ```
        SQRT_X 5
        SQRT_X 42
        SQRT_X 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +X
        Z -> -Y
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +X
        Angle: 90 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1+i, +1-i]
        [+1-i, +1+i] / 2
        ```
        
    
- <a name="SQRT_X_DAG"></a>**`SQRT_X_DAG`**
    
    Adjoint square root of X gate.
    Phases the amplitude of |-> by -i.
    Equivalent to `H` then `S_DAG` then `H`.
    
    - Example:
    
        ```
        SQRT_X_DAG 5
        SQRT_X_DAG 42
        SQRT_X_DAG 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +X
        Z -> +Y
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +X
        Angle: -90 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1-i, +1+i]
        [+1+i, +1-i] / 2
        ```
        
    
- <a name="SQRT_Y"></a>**`SQRT_Y`**
    
    Principle square root of Y gate.
    Phases the amplitude of |-i> by i.
    Equivalent to `S` then `H` then `S` then `H` then `S_DAG`.
    
    - Example:
    
        ```
        SQRT_Y 5
        SQRT_Y 42
        SQRT_Y 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> -Z
        Z -> +X
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +Y
        Angle: 90 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1+i, -1-i]
        [+1+i, +1+i] / 2
        ```
        
    
- <a name="SQRT_Y_DAG"></a>**`SQRT_Y_DAG`**
    
    Principle square root of Y gate.
    Phases the amplitude of |-i> by -i.
    Equivalent to `S` then `H` then `S_DAG` then `H` then `S_DAG`.
    
    - Example:
    
        ```
        SQRT_Y_DAG 5
        SQRT_Y_DAG 42
        SQRT_Y_DAG 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +Z
        Z -> -X
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +Y
        Angle: -90 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1-i, +1-i]
        [-1+i, +1-i] / 2
        ```
        
    
- <a name="S_DAG"></a>**`S_DAG`**
    
    Alternate name: <a name="SQRT_Z_DAG"></a>`SQRT_Z_DAG`
    
    Principle square root of Z gate.
    Phases the amplitude of |1> by -i.
    
    - Example:
    
        ```
        S_DAG 5
        S_DAG 42
        S_DAG 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> -Y
        Z -> +Z
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +Z
        Angle: -90 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ]
        [    ,   -i]
        ```
        
    
## Two Qubit Clifford Gates

- <a name="CX"></a>**`CX`**
    
    Alternate name: <a name="ZCX"></a>`ZCX`
    
    Alternate name: <a name="CNOT"></a>`CNOT`
    
    The Z-controlled X gate.
    First qubit is the control, second qubit is the target.
    The first qubit can be replaced by a measurement record.
    
    Applies an X gate to the target if the control is in the |1> state.
    
    Negates the amplitude of the |1>|-> state.
    
    - Example:
    
        ```
        CX 5 6
        CX 42 43
        CX 5 6 42 43
        CX rec[-1] 111
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +XX
        Z_ -> +Z_
        _X -> +_X
        _Z -> +ZZ
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ,     ,     ]
        [    ,     ,     , +1  ]
        [    ,     , +1  ,     ]
        [    , +1  ,     ,     ]
        ```
        
    
- <a name="CY"></a>**`CY`**
    
    Alternate name: <a name="ZCY"></a>`ZCY`
    
    The Z-controlled Y gate.
    First qubit is the control, second qubit is the target.
    The first qubit can be replaced by a measurement record.
    
    Applies a Y gate to the target if the control is in the |1> state.
    
    Negates the amplitude of the |1>|-i> state.
    
    - Example:
    
        ```
        CY 5 6
        CY 42 43
        CY 5 6 42 43
        CY rec[-1] 111
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +XY
        Z_ -> +Z_
        _X -> +ZX
        _Z -> +ZZ
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ,     ,     ]
        [    ,     ,     ,   -i]
        [    ,     , +1  ,     ]
        [    ,   +i,     ,     ]
        ```
        
    
- <a name="CZ"></a>**`CZ`**
    
    Alternate name: <a name="ZCZ"></a>`ZCZ`
    
    The Z-controlled Z gate.
    First qubit is the control, second qubit is the target.
    Either qubit can be replaced by a measurement record.
    
    Applies a Z gate to the target if the control is in the |1> state.
    
    Negates the amplitude of the |1>|1> state.
    
    - Example:
    
        ```
        CZ 5 6
        CZ 42 43
        CZ 5 6 42 43
        CZ rec[-1] 111
        CZ 111 rec[-1]
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +XZ
        Z_ -> +Z_
        _X -> +ZX
        _Z -> +_Z
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ,     ,     ]
        [    , +1  ,     ,     ]
        [    ,     , +1  ,     ]
        [    ,     ,     , -1  ]
        ```
        
    
- <a name="ISWAP"></a>**`ISWAP`**
    
    Swaps two qubits and phases the -1 eigenspace of the ZZ observable by i.
    Equivalent to `SWAP` then `CZ` then `S` on both targets.
    
    - Example:
    
        ```
        ISWAP 5 6
        ISWAP 42 43
        ISWAP 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +ZY
        Z_ -> +_Z
        _X -> +YZ
        _Z -> +Z_
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ,     ,     ]
        [    ,     ,   +i,     ]
        [    ,   +i,     ,     ]
        [    ,     ,     , +1  ]
        ```
        
    
- <a name="ISWAP_DAG"></a>**`ISWAP_DAG`**
    
    Swaps two qubits and phases the -1 eigenspace of the ZZ observable by -i.
    Equivalent to `SWAP` then `CZ` then `S_DAG` on both targets.
    
    - Example:
    
        ```
        ISWAP_DAG 5 6
        ISWAP_DAG 42 43
        ISWAP_DAG 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> -ZY
        Z_ -> +_Z
        _X -> -YZ
        _Z -> +Z_
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ,     ,     ]
        [    ,     ,   -i,     ]
        [    ,   -i,     ,     ]
        [    ,     ,     , +1  ]
        ```
        
    
- <a name="SWAP"></a>**`SWAP`**
    
    Swaps two qubits.
    
    - Example:
    
        ```
        SWAP 5 6
        SWAP 42 43
        SWAP 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +_X
        Z_ -> +_Z
        _X -> +X_
        _Z -> +Z_
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ,     ,     ]
        [    ,     , +1  ,     ]
        [    , +1  ,     ,     ]
        [    ,     ,     , +1  ]
        ```
        
    
- <a name="XCX"></a>**`XCX`**
    
    The X-controlled X gate.
    First qubit is the control, second qubit is the target.
    
    Applies an X gate to the target if the control is in the |-> state.
    
    Negates the amplitude of the |->|-> state.
    
    - Example:
    
        ```
        XCX 5 6
        XCX 42 43
        XCX 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +X_
        Z_ -> +ZX
        _X -> +_X
        _Z -> +XZ
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  , +1  , +1  , -1  ]
        [+1  , +1  , -1  , +1  ]
        [+1  , -1  , +1  , +1  ]
        [-1  , +1  , +1  , +1  ] / 2
        ```
        
    
- <a name="XCY"></a>**`XCY`**
    
    The X-controlled Y gate.
    First qubit is the control, second qubit is the target.
    
    Applies a Y gate to the target if the control is in the |-> state.
    
    Negates the amplitude of the |->|-i> state.
    
    - Example:
    
        ```
        XCY 5 6
        XCY 42 43
        XCY 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +X_
        Z_ -> +ZY
        _X -> +XX
        _Z -> +XZ
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  , +1  ,   -i,   +i]
        [+1  , +1  ,   +i,   -i]
        [  +i,   -i, +1  , +1  ]
        [  -i,   +i, +1  , +1  ] / 2
        ```
        
    
- <a name="XCZ"></a>**`XCZ`**
    
    The X-controlled Z gate.
    First qubit is the control, second qubit is the target.
    The second qubit can be replaced by a measurement record.
    
    Applies a Z gate to the target if the control is in the |-> state.
    
    Negates the amplitude of the |->|1> state.
    
    - Example:
    
        ```
        XCZ 5 6
        XCZ 42 43
        XCZ 5 6 42 43
        XCZ 111 rec[-1]
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +X_
        Z_ -> +ZZ
        _X -> +XX
        _Z -> +_Z
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ,     ,     ]
        [    , +1  ,     ,     ]
        [    ,     ,     , +1  ]
        [    ,     , +1  ,     ]
        ```
        
    
- <a name="YCX"></a>**`YCX`**
    
    The Y-controlled X gate.
    First qubit is the control, second qubit is the target.
    
    Applies an X gate to the target if the control is in the |-i> state.
    
    Negates the amplitude of the |-i>|-> state.
    
    - Example:
    
        ```
        YCX 5 6
        YCX 42 43
        YCX 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +XX
        Z_ -> +ZX
        _X -> +_X
        _Z -> +YZ
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,   -i, +1  ,   +i]
        [  +i, +1  ,   -i, +1  ]
        [+1  ,   +i, +1  ,   -i]
        [  -i, +1  ,   +i, +1  ] / 2
        ```
        
    
- <a name="YCY"></a>**`YCY`**
    
    The Y-controlled Y gate.
    First qubit is the control, second qubit is the target.
    
    Applies a Y gate to the target if the control is in the |-i> state.
    
    Negates the amplitude of the |-i>|-i> state.
    
    - Example:
    
        ```
        YCY 5 6
        YCY 42 43
        YCY 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +XY
        Z_ -> +ZY
        _X -> +YX
        _Z -> +YZ
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,   -i,   -i, +1  ]
        [  +i, +1  , -1  ,   -i]
        [  +i, -1  , +1  ,   -i]
        [+1  ,   +i,   +i, +1  ] / 2
        ```
        
    
- <a name="YCZ"></a>**`YCZ`**
    
    The Y-controlled Z gate.
    First qubit is the control, second qubit is the target.
    The second qubit can be replaced by a measurement record.
    
    Applies a Z gate to the target if the control is in the |-i> state.
    
    Negates the amplitude of the |-i>|1> state.
    
    - Example:
    
        ```
        YCZ 5 6
        YCZ 42 43
        YCZ 5 6 42 43
        YCZ 111 rec[-1]
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +XZ
        Z_ -> +ZZ
        _X -> +YX
        _Z -> +_Z
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ,     ,     ]
        [    , +1  ,     ,     ]
        [    ,     ,     ,   -i]
        [    ,     ,   +i,     ]
        ```
        
    
## Noise Channels

- <a name="DEPOLARIZE1"></a>**`DEPOLARIZE1`**
    
    The single qubit depolarizing channel.
    
    Applies a randomly chosen Pauli with a given probability.
    
    - Pauli Mixture:
    
        ```
        1-p: I
        p/3: X
        p/3: Y
        p/3: Z
        ```
    
    - Example:
    
        ```
        DEPOLARIZE1(0.001) 5
        DEPOLARIZE1(0.001) 42
        DEPOLARIZE1(0.001) 5 42
        ```
        
    
- <a name="DEPOLARIZE2"></a>**`DEPOLARIZE2`**
    
    The two qubit depolarizing channel.
    
    Applies a randomly chosen two-qubit Pauli product with a given probability.
    
    - Pauli Mixture:
    
        ```
         1-p: II
        p/15: IX
        p/15: IY
        p/15: IZ
        p/15: XI
        p/15: XX
        p/15: XY
        p/15: XZ
        p/15: YI
        p/15: YX
        p/15: YY
        p/15: YZ
        p/15: ZI
        p/15: ZX
        p/15: ZY
        p/15: ZZ
        ```
    
    - Example:
    
        ```
        DEPOLARIZE2(0.001) 5 6
        DEPOLARIZE2(0.001) 42 43
        DEPOLARIZE2(0.001) 5 6 42 43
        ```
        
    
- <a name="E"></a>**`E`**
    
    Alternate name: <a name="CORRELATED_ERROR"></a>`CORRELATED_ERROR`
    
    Probabilistically applies a Pauli product error with a given probability.
    Sets the "correlated error occurred flag" to true if the error occurred.
    Otherwise sets the flag to false.
    
    See also: `ELSE_CORRELATED_ERROR`.
    
    - Example:
    
        ```
        # With 40% probability, uniformly pick X1*Y2 or Z2*Z3 or X1*Y2*Z3.
        CORRELATED_ERROR(0.2) X1 Y2
        ELSE_CORRELATED_ERROR(0.25) Z2 Z3
        ELSE_CORRELATED_ERROR(0.33333333333) X1 Y2 Z3
        ```
    
- <a name="ELSE_CORRELATED_ERROR"></a>**`ELSE_CORRELATED_ERROR`**
    
    Probabilistically applies a Pauli product error with a given probability, unless the "correlated error occurred flag" is set.
    If the error occurs, sets the "correlated error occurred flag" to true.
    Otherwise leaves the flag alone.
    
    See also: `CORRELATED_ERROR`.
    
    - Example:
    
        ```
        # With 40% probability, uniformly pick X1*Y2 or Z2*Z3 or X1*Y2*Z3.
        CORRELATED_ERROR(0.2) X1 Y2
        ELSE_CORRELATED_ERROR(0.25) Z2 Z3
        ELSE_CORRELATED_ERROR(0.33333333333) X1 Y2 Z3
        ```
    
- <a name="X_ERROR"></a>**`X_ERROR`**
    
    Applies a Pauli X with a given probability.
    
    - Pauli Mixture:
    
        ```
        1-p: I
         p : X
        ```
    
    - Example:
    
        ```
        X_ERROR(0.001) 5
        X_ERROR(0.001) 42
        X_ERROR(0.001) 5 42
        ```
        
    
- <a name="Y_ERROR"></a>**`Y_ERROR`**
    
    Applies a Pauli Y with a given probability.
    
    - Pauli Mixture:
    
        ```
        1-p: I
         p : Y
        ```
    
    - Example:
    
        ```
        Y_ERROR(0.001) 5
        Y_ERROR(0.001) 42
        Y_ERROR(0.001) 5 42
        ```
        
    
- <a name="Z_ERROR"></a>**`Z_ERROR`**
    
    Applies a Pauli Z with a given probability.
    
    - Pauli Mixture:
    
        ```
        1-p: I
         p : Z
        ```
    
    - Example:
    
        ```
        Z_ERROR(0.001) 5
        Z_ERROR(0.001) 42
        Z_ERROR(0.001) 5 42
        ```
        
    
## Collapsing Gates

- <a name="M"></a>**`M`**
    
    Alternate name: <a name="MZ"></a>`MZ`
    
    Z-basis measurement.
    Projects each target qubit into `|0>` or `|1>` and reports its value (false=`|0>`, true=`|1>`).
    Prefixing a target with ! inverts its recorded measurement result.
    
    - Example:
    
        ```
        M 5
        M !42
        M 5 !42
        ```
        
    - Stabilizer Generators:
    
        ```
        Z -> mZ
        ```
        
    
- <a name="MR"></a>**`MR`**
    
    Alternate name: <a name="MRZ"></a>`MRZ`
    
    Z-basis demolition measurement.
    Projects each target qubit into `|0>` or `|1>`, reports its value (false=`|0>`, true=`|1>`), then resets to `|0>`.
    Prefixing a target with ! inverts its recorded measurement result.
    
    - Example:
    
        ```
        MR 5
        MR !42
        MR 5 !42
        ```
        
    - Stabilizer Generators:
    
        ```
        Z -> m
        1 -> +Z
        ```
        
    
- <a name="MRX"></a>**`MRX`**
    
    X-basis demolition measurement.
    Projects each target qubit into `|+>` or `|->`, reports its value (false=`|+>`, true=`|->`), then resets to `|+>`.
    Prefixing a target with ! inverts its recorded measurement result.
    
    - Example:
    
        ```
        MRX 5
        MRX !42
        MRX 5 !42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> m
        1 -> +X
        ```
        
    
- <a name="MRY"></a>**`MRY`**
    
    Y-basis demolition measurement.
    Projects each target qubit into `|i>` or `|-i>`, reports its value (false=`|i>`, true=`|-i>`), then resets to `|i>`.
    Prefixing a target with ! inverts its recorded measurement result.
    
    - Example:
    
        ```
        MRY 5
        MRY !42
        MRY 5 !42
        ```
        
    - Stabilizer Generators:
    
        ```
        Y -> m
        1 -> +Y
        ```
        
    
- <a name="MX"></a>**`MX`**
    
    X-basis measurement.
    Projects each target qubit into `|+>` or `|->` and reports its value (false=`|+>`, true=`|->`).
    Prefixing a target with ! inverts its recorded measurement result.
    
    - Example:
    
        ```
        MX 5
        MX !42
        MX 5 !42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> mX
        ```
        
    
- <a name="MY"></a>**`MY`**
    
    Y-basis measurement.
    Projects each target qubit into `|i>` or `|-i>` and reports its value (false=`|i>`, true=`|-i>`).
    Prefixing a target with ! inverts its recorded measurement result.
    
    - Example:
    
        ```
        MY 5
        MY !42
        MY 5 !42
        ```
        
    - Stabilizer Generators:
    
        ```
        Y -> mY
        ```
        
    
- <a name="R"></a>**`R`**
    
    Alternate name: <a name="RZ"></a>`RZ`
    
    Z-basis reset.
    Forces each target qubit into the `|0>` state by silently measuring it in the Z basis and applying an `X` gate if it ended up in the `|1>` state.
    
    - Example:
    
        ```
        R 5
        R 42
        R 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        1 -> +Z
        ```
        
    
- <a name="RX"></a>**`RX`**
    
    X-basis reset.
    Forces each target qubit into the `|+>` state by silently measuring it in the X basis and applying a `Z` gate if it ended up in the `|->` state.
    
    - Example:
    
        ```
        RX 5
        RX 42
        RX 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        1 -> +X
        ```
        
    
- <a name="RY"></a>**`RY`**
    
    Y-basis reset.
    Forces each target qubit into the `|i>` state by silently measuring it in the Y basis and applying an `X` gate if it ended up in the `|-i>` state.
    
    - Example:
    
        ```
        RY 5
        RY 42
        RY 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        1 -> +Y
        ```
        
    
## Control Flow

- <a name="REPEAT"></a>**`REPEAT`**
    
    Repeats the instructions in its body N times.
    The implementation-defined maximum value of N is 9223372036854775807.
    
    - Example:
    
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
    
## Annotations

- <a name="DETECTOR"></a>**`DETECTOR`**
    
    Annotates that a set of measurements have a deterministic result, which can be used to detect errors.
    
    Detectors are ignored in measurement sampling mode.
    In detector sampling mode, detectors produce results (false=expected parity, true=incorrect parity detected).
    
    - Example:
    
        ```
        H 0
        CNOT 0 1
        M 0 1
        DETECTOR rec[-1] rec[-2]
        ```
    
- <a name="OBSERVABLE_INCLUDE"></a>**`OBSERVABLE_INCLUDE`**
    
    Adds measurement results to a given logical observable index.
    
    A logical observable's measurement result is the parity of all physical measurement results added to it.
    
    A logical observable is similar to a Detector, except the measurements making up an observable can be built up
    incrementally over the entire circuit.
    
    Logical observables are ignored in measurement sampling mode.
    In detector sampling mode, observables produce results (false=expected parity, true=incorrect parity detected).
    These results are optionally appended to the detector results, depending on simulator arguments / command line flags.
    
    - Example:
    
        ```
        H 0
        CNOT 0 1
        M 0 1
        OBSERVABLE_INCLUDE(5) rec[-1] rec[-2]
        ```
    
- <a name="TICK"></a>**`TICK`**
    
    Indicates the end of a layer of gates, or that time is advancing.
    For example, used by `stimcirq` to preserve the moment structure of cirq circuits converted to/from stim circuits.
    
    - Example:
    
        ```
        TICK
        TICK
        # Oh, and of course:
        TICK
        ```
    
