# Using Stim from the command line

Stim reads a quantum circuit from `stdin` and writes measurement results to `stdout`.
The circuit input format is a series of lines, each starting with an optional gate (e.g. `CNOT 0 1`)
and ending with an optional comment prefixed by `#`.
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
" | ./stim --analyze_errors
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

- **`--analyze_errors`**:
    **Detector error model creation mode**.
    Determines the detectors and logical observables flipped by error channels in the input circuit.
    Outputs lines like `error(p) D2 D3 L5`, which means that there is an independent error
    mechanism that occurs with probability `p` that flips detector 2, detector 3, and logical observable 5.
    Detectors are numbered, starting from 0, based on the order they appear in the circuit.
    Observables are numbered based on the observable indices specified in the circuit.
    
    Doesn't support `ELSE_CORRELATED_ERROR`.
    
    - **`--fold_loops`**:
        Allows the output error model to contain `repeat` blocks.
        This makes it feasible to get error models for circuits with `REPEAT` blocks that repeat
        instructions millions or trillions of times.

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

    - **`--decompose_errors`**:
        Decomposes errors into components that are "graphlike" (that have at most two detection events).

        Two strategies are used for finding decompositions.
        Stim starts by trying the *within-error* strategy, which looks at the various cases of a single noise
        channel and checks if some cases are products of other cases.
        For example, in a two qubit depolarizing channel, the `X1*Y2` case may produce four detection events but often
        they will decompose into two detection events produced by the `X1*I2` case and two detection events produced by
        the `I1*Y2` case.
        If the within-error strategy fails, Stim falls back to the *future-error* strategy, which checks if any single
        detection event or pair of detection events in the error happen to occur on their own later in the circuit. If
        both strategies fail, Stim raise an error saying it failed to find a satisfying decomposition.

    - **`--allow_gauge_detectors`**:
        Normally, when a detector anti-commutes with a stabilizer of the circuit (forcing the detector
        to have random results instead of deterministic results), error analysis throws an exception.

        Specifying `--allow_gauge_detectors` instead allows this behavior and reports it as an `error(0.5)`.

        For example, in the following circuit, the two detectors are gauge detectors:

        ```
        R 0
        H 0
        CNOT 0 1
        M 0 1
        DETECTOR rec[-1]
        DETECTOR rec[-2]
        ```

        Without `--allow_gauge_detectors`, stim will raise an exception when analyzing this circuit.
        With `--allow_gauge_detectors`, stim will replace this exception with an `error(0.5) D1 D2` error mechanism in
        the output.

    - **`--approximate_disjoint_errors`** or **`--approximate_disjoint_errors=threshold`**:
        Defaults to 0 (false) when not specified.
        Defaults to 1 (true) when specified with an empty argument.
        Specifies a threshold for allowing error mechanisms with disjoint components
        (such as `PAULI_CHANNEL_1(0.1, 0.2, 0.0)`)
        to be approximated as having independent components.
        If any of the component error probabilities that will be approximated as independent is larger than this
        threshold, error analysis will fail instead of performing the approximation.

        For example, if `--approximate_disjoint_errors` is specified then a `PAULI_CHANNEL_1(0.1, 0.2, 0.0)` is
        approximated as an `X_ERROR(0.1)` followed by a `Z_ERROR(0.2)`.

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

- **`--out_format=[name]`**: <a name="out_format"></a>Output format to use.
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
