# Stim command line reference

## Index

- **(mode)** [stim analyze_errors](#analyze_errors)
    - [--allow_gauge_detectors](#--allow_gauge_detectors)
    - [--approximate_disjoint_errors](#--approximate_disjoint_errors)
    - [--block_decompose_from_introducing_remnant_edges](#--block_decompose_from_introducing_remnant_edges)
    - [--decompose_errors](#--decompose_errors)
    - [--fold_loops](#--fold_loops)
    - [--ignore_decomposition_failures](#--ignore_decomposition_failures)
    - [--in](#--in)
    - [--out](#--out)
- **(mode)** [stim detect](#detect)
    - [--append_observables](#--append_observables)
    - [--in](#--in)
    - [--out](#--out)
    - [--out_format](#--out_format)
    - [--seed](#--seed)
    - [--shots](#--shots)
- **(mode)** [stim explain_errors](#explain_errors)
    - [--dem_filter](#--dem_filter)
    - [--in](#--in)
    - [--out](#--out)
- **(mode)** [stim gen](#gen)
    - [--after_clifford_depolarization](#--after_clifford_depolarization)
    - [--after_reset_flip_probability](#--after_reset_flip_probability)
    - [--before_measure_flip_probability](#--before_measure_flip_probability)
    - [--before_round_data_depolarization](#--before_round_data_depolarization)
    - [--distance](#--distance)
    - [--in](#--in)
    - [--out](#--out)
    - [--rounds](#--rounds)
    - [--task](#--task)
- **(mode)** [stim help](#help)
- **(mode)** [stim m2d](#m2d)
    - [--circuit](#--circuit)
    - [--in](#--in)
    - [--in_format](#--in_format)
    - [--out](#--out)
    - [--out_format](#--out_format)
    - [--skip_reference_sample](#--skip_reference_sample)
- **(mode)** [stim repl](#repl)
- **(mode)** [stim sample](#sample)
    - [--in](#--in)
    - [--out](#--out)
    - [--out_format](#--out_format)
    - [--seed](#--seed)
    - [--shots](#--shots)
    - [--skip_reference_sample](#--skip_reference_sample)
- **(mode)** [stim sample_dem](#sample_dem)
    - [--err_out](#--err_out)
    - [--err_out_format](#--err_out_format)
    - [--in](#--in)
    - [--obs_out](#--obs_out)
    - [--obs_out_format](#--obs_out_format)
    - [--out](#--out)
    - [--out_format](#--out_format)
    - [--replay_err_in](#--replay_err_in)
    - [--replay_err_in_format](#--replay_err_in_format)
    - [--seed](#--seed)
    - [--shots](#--shots)
## Modes

<a name="analyze_errors"></a>
### stim analyze_errors

*Converts a circuit into a detector error model.*

Determines the detectors and logical observables that are flipped by each error channel in the given circuit, and
summarizes this information as an error model framed entirely in terms of independent error mechanisms that flip sets of
detectors and observables.

stdin: The circuit to convert into a detector error model.

stdout: The detector error model in [detector error model file format](https://github.com/quantumlib/Stim/blob/main/doc/file_format_dem_detector_error_model.md).

stderr:
    Circuit failed to parse.
    Failed to produce a graphlike detector error model but `--decompose_errors` was set.
    Circuit contained gauge detectors but `--allow_gauge_detectors` wasn't set.
    Circuit contained disjoint error channels but `--approximate_disjoint_errors` wasn't set.

- Example:

    ```
    >>> stim analyze_errors
    ... # Single-shot X-basis rep code circuit.
    ... RX 0 1 2 3 4 5 6
    ... MPP X0*X1 X1*X2 X2*X3 X3*X4 X4*X5 X5*X6
    ... Z_ERROR(0.125) 0 1 2 3 4 5 6
    ... MPP X0 X1 X2 X3 X4 X5 X6
    ... DETECTOR rec[-1] rec[-2] rec[-8]   # X6 X5 now = X5*X6 before
    ... DETECTOR rec[-2] rec[-3] rec[-9]   # X5 X4 now = X4*X5 before
    ... DETECTOR rec[-3] rec[-4] rec[-10]  # X4 X3 now = X3*X4 before
    ... DETECTOR rec[-4] rec[-5] rec[-11]  # X3 X2 now = X2*X3 before
    ... DETECTOR rec[-5] rec[-6] rec[-12]  # X2 X1 now = X1*X2 before
    ... DETECTOR rec[-6] rec[-7] rec[-13]  # X1 X0 now = X0*X1 before
    ... OBSERVABLE_INCLUDE(0) rec[-1]
    error(0.125) D0 D1
    error(0.125) D0 L0
    error(0.125) D1 D2
    error(0.125) D2 D3
    error(0.125) D3 D4
    error(0.125) D4 D5
    error(0.125) D5
    ```

Flags used with this mode:
- [--allow_gauge_detectors](#--allow_gauge_detectors)
- [--approximate_disjoint_errors](#--approximate_disjoint_errors)
- [--block_decompose_from_introducing_remnant_edges](#--block_decompose_from_introducing_remnant_edges)
- [--decompose_errors](#--decompose_errors)
- [--fold_loops](#--fold_loops)
- [--ignore_decomposition_failures](#--ignore_decomposition_failures)
- [--in](#--in)
- [--out](#--out)

<a name="detect"></a>
### stim detect

*Samples detection events from a circuit.*

stdin: The circuit, specified using the stim circuit file format, to sample detection events from.

stdout: The sampled detection event data. Each output bit corresponds to a `DETECTOR` instruction or (if
    `--append_observables` is specified) accumulated results from `OBSERVABLE_INCLUDE` instructions.

See also: `stim help DETECTOR` and `stim help OBSERVABLE_INCLUDE`.

- Examples:

    ```
    >>> stim detect --shots 5
    ... H 0
    ... CNOT 0 1
    ... X_ERROR(0.1) 0 1
    ... M 0 1
    ... DETECTOR rec[-1] rec[-2]
    0
    1
    0
    0
    0
    ```

    ```
    >>> stim detect --shots 10 --append_observables
    ... # Single-shot X-basis rep code circuit.
    ... RX 0 1 2 3 4 5 6
    ... MPP X0*X1 X1*X2 X2*X3 X3*X4 X4*X5 X5*X6
    ... Z_ERROR(0.1) 0 1 2 3 4 5 6
    ... MPP X0 X1 X2 X3 X4 X5 X6
    ... DETECTOR rec[-1] rec[-2] rec[-8]   # X6 X5 now = X5*X6 before
    ... DETECTOR rec[-2] rec[-3] rec[-9]   # X5 X4 now = X4*X5 before
    ... DETECTOR rec[-3] rec[-4] rec[-10]  # X4 X3 now = X3*X4 before
    ... DETECTOR rec[-4] rec[-5] rec[-11]  # X3 X2 now = X2*X3 before
    ... DETECTOR rec[-5] rec[-6] rec[-12]  # X2 X1 now = X1*X2 before
    ... DETECTOR rec[-6] rec[-7] rec[-13]  # X1 X0 now = X0*X1 before
    ... OBSERVABLE_INCLUDE(0) rec[-1]
    0110000
    0000000
    1000001
    0110010
    1100000
    0000010
    1000001
    0110000
    0000000
    0011000
    ```

    ```
    >>> stim detect --shots 10 --append_observables --out_format=dets
    ... # Single-shot X-basis rep code circuit.
    ... RX 0 1 2 3 4 5 6
    ... MPP X0*X1 X1*X2 X2*X3 X3*X4 X4*X5 X5*X6
    ... Z_ERROR(0.1) 0 1 2 3 4 5 6
    ... MPP X0 X1 X2 X3 X4 X5 X6
    ... DETECTOR rec[-1] rec[-2] rec[-8]   # X6 X5 now = X5*X6 before
    ... DETECTOR rec[-2] rec[-3] rec[-9]   # X5 X4 now = X4*X5 before
    ... DETECTOR rec[-3] rec[-4] rec[-10]  # X4 X3 now = X3*X4 before
    ... DETECTOR rec[-4] rec[-5] rec[-11]  # X3 X2 now = X2*X3 before
    ... DETECTOR rec[-5] rec[-6] rec[-12]  # X2 X1 now = X1*X2 before
    ... DETECTOR rec[-6] rec[-7] rec[-13]  # X1 X0 now = X0*X1 before
    ... OBSERVABLE_INCLUDE(0) rec[-1]
    shot D1 D2
    shot
    shot D0 L0
    shot D1 D2 D5
    shot D0 D1
    shot D5
    shot D0 L0
    shot D1 D2
    shot
    shot D2 D3
    ```

Flags used with this mode:
- [--append_observables](#--append_observables)
- [--in](#--in)
- [--out](#--out)
- [--out_format](#--out_format)
- [--seed](#--seed)
- [--shots](#--shots)

<a name="explain_errors"></a>
### stim explain_errors

*Describes how detector error model errors correspond to circuit errors.*

Takes a circuit on stdin, and optionally a detector error model file containing the errors to match on `--dem_filter`.
Iterates over the circuit, recording which circuit noise processes violate which combinations of detectors and
observables.
In other words, matches circuit errors to detector error model (dem) errors.
If a filter is specified, matches to dem errors not in the filter are discarded.
Then outputs, for each dem error, that dem error and which circuit errors cause it.

stdin: The circuit to look for noise processes in, specified using the [stim circuit file format](https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md).

stdout: A human readable description of dem errors that were found, with associated circuit error data.

- Examples:

    ```
    >>> stim gen --code surface_code --task rotated_memory_z --distance 5 --rounds 10 --after_clifford_depolarization 0.001 > tmp.stim
    >>> echo "error(1) D97 D98 D102 D103" > tmp.dem
    >>> stim explain_errors --in tmp.stim --dem_filter tmp.dem
    ExplainedError {
        dem_error_terms: D97[coords 4,6,0] D98[coords 6,6,0] D102[coords 2,8,0] D103[coords 4,8,0]
        CircuitErrorLocation {
            flipped_pauli_product: Y37[coords 4,6]*Y36[coords 3,7]
            Circuit location stack trace:
                (after 31 TICKs)
                at instruction #89 (a REPEAT 0 block) in the circuit
                after 3 completed iterations
                at instruction #10 (DEPOLARIZE2) in the REPEAT block
                at targets #9 to #10 of the instruction
                resolving to DEPOLARIZE2(0.001) 37[coords 4,6] 36[coords 3,7]
        }
    }
    ```

Flags used with this mode:
- [--dem_filter](#--dem_filter)
- [--in](#--in)
- [--out](#--out)

<a name="gen"></a>
### stim gen

*Generates example circuits.*

The generated circuits include annotations for noise, detectors, logical observables, the spacetial locations of qubits,
the spacetime locations of detectors, and the inexorable passage of time steps.

stdout: A circuit in [stim's circuit file format](https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md).

The type of circuit to generate is specified using the `--code` and `--task` flags. Each code supports different tasks.
Other information that must be specified is the number of `--rounds`, the `--distance`, and any desired noise.

- Example:

    ```
    >>> stim gen --code repetition_code --task memory --distance 3 --rounds 100 --after_clifford_depolarization 0.001
    # Generated repetition_code circuit.
    # task: memory
    # rounds: 100
    # distance: 3
    # before_round_data_depolarization: 0
    # before_measure_flip_probability: 0
    # after_reset_flip_probability: 0
    # after_clifford_depolarization: 0.001
    # layout:
    # L0 Z1 d2 Z3 d4
    # Legend:
    #     d# = data qubit
    #     L# = data qubit with logical observable crossing
    #     Z# = measurement qubit
    R 0 1 2 3 4
    TICK
    CX 0 1 2 3
    DEPOLARIZE2(0.001) 0 1 2 3
    TICK
    CX 2 1 4 3
    DEPOLARIZE2(0.001) 2 1 4 3
    TICK
    MR 1 3
    DETECTOR(1, 0) rec[-2]
    DETECTOR(3, 0) rec[-1]
    REPEAT 99 {
        TICK
        CX 0 1 2 3
        DEPOLARIZE2(0.001) 0 1 2 3
        TICK
        CX 2 1 4 3
        DEPOLARIZE2(0.001) 2 1 4 3
        TICK
        MR 1 3
        SHIFT_COORDS(0, 1)
        DETECTOR(1, 0) rec[-2] rec[-4]
        DETECTOR(3, 0) rec[-1] rec[-3]
    }
    M 0 2 4
    DETECTOR(1, 1) rec[-2] rec[-3] rec[-5]
    DETECTOR(3, 1) rec[-1] rec[-2] rec[-4]
    OBSERVABLE_INCLUDE(0) rec[-1]
    ```

Flags used with this mode:
- [--after_clifford_depolarization](#--after_clifford_depolarization)
- [--after_reset_flip_probability](#--after_reset_flip_probability)
- [--before_measure_flip_probability](#--before_measure_flip_probability)
- [--before_round_data_depolarization](#--before_round_data_depolarization)
- [--distance](#--distance)
- [--in](#--in)
- [--out](#--out)
- [--rounds](#--rounds)
- [--task](#--task)

<a name="help"></a>
### stim help

*Prints helpful information about stim.*

Use `stim help [topic]` for information about specific topics. Available topics include:

    stim help gates    # List all circuit instructions supported by stim.
    stim help formats  # List all result formats supported by stim.
    stim help modes    # List all tasks performed by stim.
    stim help flags    # List all command line flags supported by stim.
    stim help [mode]   # Print information about a mode, such as `sample` or `analyze_errors`.
    stim help [flag]   # Print information about a command line flag, such as `--out` or `--in_format`.
    stim help [gate]   # Print information about a circuit instruction, such as the `CNOT` gate.
    stim help [format] # Print information about a supported result format, such as the `01` format.

<a name="m2d"></a>
### stim m2d

*Convert measurement data into detection event data.*

Takes measurement data from stdin, and a circuit from the file given to `--circuit`. Converts the measurement data
into detection event data based on annotations in the circuit. Outputs detection event data to stdout.

Note that this conversion requires taking a reference sample from the circuit, in order to determine whether the
measurement sets defining the detectors and observables have an expected parity of 0 or an expected parity of 1.
To get the reference sample, a noiseless stabilizer simulation of the circuit is performed.

stdin: The measurement data, in the format specified by --in_format.

stdout: The detection event data, in the format specified by --out_format (defaults to '01').

- Examples:

    ```
    >>> echo -e "X 0\nM 0 1\nDETECTOR rec[-2]\nDETECTOR rec[-1]\nOBSERVABLE_INCLUDE(2) rec[-1]" > tmp.stim
    >>> stim m2d --in_format 01 --out_format dets --circuit tmp.stim --append_observables
    ... 00
    ... 01
    ... 10
    ... 11
    shot D0
    shot D0 D1 L2
    shot
    shot D1 L2
    ```

Flags used with this mode:
- [--circuit](#--circuit)
- [--in](#--in)
- [--in_format](#--in_format)
- [--out](#--out)
- [--out_format](#--out_format)
- [--skip_reference_sample](#--skip_reference_sample)

<a name="repl"></a>
### stim repl

*Read-eval-print-loop mode.*

Reads operations from stdin while immediately writing measurement results to stdout.

stdin: A circuit to execute.

stdout: Measurement results.

stderr: Ignored errors encountered while parsing/simulating the circuit arriving via stdin.

- Example:

    ```
    >>> stim repl
    ... M 0
    0
    ... X 0
    ... M 0
    1
    ... X 2 3 9
    ... M 0 1 2 3 4 5 6 7 8 9
    1 0 1 1 0 0 0 0 0 1
    ... REPEAT 5 {
    ...     R 0 1
    ...     H 0
    ...     CNOT 0 1
    ...     M 0 1
    ... }
    00
    11
    11
    00
    11
    ```

<a name="sample"></a>
### stim sample

*Samples measurements from a circuit.*

stdin: The circuit to sample from, specified using the [stim circuit file format](https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md).

stdout: The sample data.

- Examples:

    ```
    >>> stim sample --shots 5
    ... H 0
    ... CNOT 0 1
    ... M 0 1
    00
    11
    11
    00
    11
    ```

    ```
    >>> stim sample --out_format dets
    ... X 2 3 5
    ... M 0 1 2 3 4 5 6 7 8 9
    shot M2 M3 M5
    ```

Flags used with this mode:
- [--in](#--in)
- [--out](#--out)
- [--out_format](#--out_format)
- [--seed](#--seed)
- [--shots](#--shots)
- [--skip_reference_sample](#--skip_reference_sample)

<a name="sample_dem"></a>
### stim sample_dem

*Samples detection events and observable flips from a detector error model.*

stdin (or --in): The detector error model to sample from, specified using the [detector error model file format](https://github.com/quantumlib/Stim/blob/main/doc/file_format_dem_detector_error_model.md).

stdout (or --out): The detection event data is written here.

- Example:

    ```bash
    echo "error(0) D0" > example.dem
    echo "error(0.5) D1 L0" >> example.dem
    echo "error(1) D2 D3" >> example.dem
    stim sample_dem \
        --shots 5 \
        --in example.dem \
        --out dets.01 \
        --out_format 01 \
        --obs_out obs_flips.01 \
        --obs_out_format 01 \
        --seed 0
    cat dets.01
    # 0111
    # 0011
    # 0011
    # 0111
    # 0111
    cat obs_flips.01
    # 1
    # 0
    # 0
    # 1
    # 1
    ```

Flags used with this mode:
- [--err_out](#--err_out)
- [--err_out_format](#--err_out_format)
- [--in](#--in)
- [--obs_out](#--obs_out)
- [--obs_out_format](#--obs_out_format)
- [--out](#--out)
- [--out_format](#--out_format)
- [--replay_err_in](#--replay_err_in)
- [--replay_err_in_format](#--replay_err_in_format)
- [--seed](#--seed)
- [--shots](#--shots)

## Flags

- <a name="--after_clifford_depolarization"></a>**`--after_clifford_depolarization`**
    Adds depolarizing noise after Clifford operations.
    
    Must be a probability between 0 and 1.
    Defaults to 0.
    
    Adds a `DEPOLARIZE1(p)` operation after every single-qubit Clifford operation and a `DEPOLARIZE2(p)` noise operation
    after every two-qubit Clifford operation.
    When the probability is set to 0, the noise operations are not inserted.
    
    
- <a name="--after_reset_flip_probability"></a>**`--after_reset_flip_probability`**
    Specifies a reset noise level.
    
    Defaults to 0 when not specified.
    Must be a number between 0 and 1.
    
    Adds an `X_ERROR(p)` after `R` (`RZ`) and `RY` operations, and a `Z_ERROR(p)` after `RX` operations.
    When set to 0, the noise operations are not inserted.
    
    
- <a name="--allow_gauge_detectors"></a>**`--allow_gauge_detectors`**
    
    Normally, when a detector anti-commutes with a stabilizer of the circuit (forcing the detector
    to have random results instead of deterministic results), error analysis throws an exception.
    
    Specifying `--allow_gauge_detectors` instead allows this behavior and reports it as an `error(0.5)` in the model.
    
    For example, in the following circuit, the two detectors are gauge detectors:
    
    ```
    R 0
    H 0
    CNOT 0 1
    M 0 1
    DETECTOR rec[-1]
    DETECTOR rec[-2]
    ```
    
    Without `--allow_gauge_detectors`, stim will raise an exception when analyzing this circuit. With
    `--allow_gauge_detectors`, stim will replace this exception with an `error(0.5) D1 D2` error mechanism in the output.
    
    
- <a name="--append_observables"></a>**`--append_observables`**
    Treat observables as extra detectors at the end of the circuit.
    
    By default, when reporting detection events, observables are not reported. This flag causes the observables to instead
    be reported as if they were detectors. For example, if there are 100 detectors and 10 observables in the circuit, then
    the output will contain 110 detectors and the last 10 are the observables. A notable exception to the "observables are
    just extra detectors" behavior of this flag is that, when using `out_format=dets`, the observables are distinguished
    from detectors by being named e.g. `L0` through `L9` instead of `D100` through `D109`.
    
    
- <a name="--approximate_disjoint_errors"></a>**`--approximate_disjoint_errors`**
    
    Specifies a threshold for allowing error mechanisms with disjoint components
    (such as `PAULI_CHANNEL_1(0.1, 0.2, 0.0)`) to be approximated as having independent components.
    
    Defaults to 0 (false) when not specified.
    Defaults to 1 (true) when specified with an empty argument.
    Must be set to a probability between 0 and 1.
    
    If any of the component error probabilities (that will be approximated as independent) is larger than the given
    threshold, error analysis will fail instead of performing the approximation.
    
    For example, if `--approximate_disjoint_errors` is specified then a `PAULI_CHANNEL_1(0.1, 0.2, 0.0)` is
    approximated as an `X_ERROR(0.1)` followed by a `Z_ERROR(0.2)`.
    
    
- <a name="--before_measure_flip_probability"></a>**`--before_measure_flip_probability`**
    Specifies a measurement noise level.
    
    Defaults to 0 when not specified.
    Must be a number between 0 and 1.
    
    Adds an `X_ERROR(p)` before `M` (`MZ`) and `MY` operations, and a `Z_ERROR(p)` before `MX` operations.
    When set to 0, the noise operations are not inserted.
    
    
- <a name="--before_round_data_depolarization"></a>**`--before_round_data_depolarization`**
    Specifies a phenomenological noise level.
    
    Defaults to 0 when not specified.
    Must be a number between 0 and 1.
    
    Adds a `DEPOLARIZE1(p)` operation to each data qubit at the start of each round of stabilizer measurements.
    When set to 0, the noise operations are not inserted.
    
    
- <a name="--block_decomposition_from_introducing_remnant_edges"></a>**`--block_decomposition_from_introducing_remnant_edges`**
    
    Requires that both A B and C D be present elsewhere in the detector error model
    in order to decompose A B C D into A B ^ C D. Normally, only one of A B or C D
    needs to appear to allow this decomposition.
    
    Remnant edges can be a useful feature for ensuring decomposition succeeds, but
    they can also reduce the effective code distance by giving the decoder single
    edges that actually represent multiple errors in the circuit (resulting in the
    decoder making misinformed choices when decoding).
    
    Irrelevant unless --decompose_errors is specified.
    
    
- <a name="--circuit"></a>**`--circuit`**
    Specifies the circuit to use when converting measurement data to detector data.
    
    The argument must be a filepath leading to a [stim circuit format file](https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md).
    
    
- <a name="--code"></a>**`--code`**
    The error correcting code to use.
    
    Supported codes are:
    
        `--code surface_code`
        `--code repetition_code`
        `--code color_code`
    
    
- <a name="--decompose_errors"></a>**`--decompose_errors`**
    
    Decomposes errors into components that are guaranteed to be "graphlike" (have at most two detection events).
    
    Stim uses two strategies for decomposing errors: within-channel and other-error.
    
    The *within-channel* strategy is always applied first, and works by looking at the various detector/observable sets
    producing by each case of a single noise channel. If some cases are products of other cases, that product is used as
    the decomposition. For example, suppose that a single qubit depolarizing channel has a `Y5` case that produces four
    detection events `D0 D1 D2 D3`, an `X5` case that produces two detection events `D0 D1`, and a `Z5` case that produces
    two detection events `D2 D3`. Because `D0 D1 D2 D3` is the combination of `D0 D1` and `D2 D3`, the `Y5` case will be
    decomposed into `D0 D1 ^ D2 D3`.
    
    The *other-error* strategy is used (as late as possible) while an error still has a component with more than two
    detection events. It is checked of one or two of those detection events appear as an individual error elsewhere in the
    model. If they do, they are split out of the component (decomposing it). This applies iteratively. For example, if an
    error `D0 ^ D1 D2 D3` appears in the model, then stim will check if there is an error anywhere in the model that has
    exactly `D1 D2`, `D1 D3`, `D2 D3`, `D1`, `D2`, or `D3` as its detection events. Suppose there is an error with `D1 D2`.
    Then the original error will be decomposed into `D0 ^ D1 D2 ^ D3`.
    
    If these strategies fail to decompose error into graphlike pieces, Stim will throw an error saying it failed to find a
    satisfying decomposition.
    
    
- <a name="--dem_filter"></a>**`--dem_filter`**
    Specifies a detector error model to use as a filter.
    
    Only details relevant to error mechanisms that appear in this model will be included in the output.
    
    The argument must be a filepath leading to a [stim detector error model format file](https://github.com/quantumlib/Stim/blob/main/doc/file_format_dem_detector_error_model.md).
    
    
- <a name="--distance"></a>**`--distance`**
    The minimum number of physical errors needed to cause a logical error.
    
    The code distance determines how large the generated circuit has to be. Conventionally, the code distance specifically
    refers to single-qubit errors between rounds instead of circuit errors during rounds.
    
    The distance must always be a positive integer. Different codes/tasks may place additional constraints on the distance
    (e.g. must be larger than 2 or must be odd or etc).
    
    
- <a name="--err_out"></a>**`--err_out`**
    Specifies a file to write a record of which errors occurred.
    
    This data can then be analyzed, modified, and later given to for example a --replay_err_in argument.
    
    
- <a name="--err_out_format"></a>**`--err_out_format`**
    The format to use when writing error data (e.g. b8 or 01).
    
    
- <a name="--fold_loops"></a>**`--fold_loops`**
    
    Allows the output error model to contain `repeat` blocks.
    
    Analyzes `REPEAT` blocks in the input circuit using a procedure that solves the loop in O(period) iterations, instead of
    O(total_repetition_count) iterations, by using ["tortoise and hare"](https://en.wikipedia.org/wiki/Cycle_detection#Floyd's_tortoise_and_hare)
    period finding algorithm. The "period" of a loop is the number of iterations required for the logical observables to end
    up back in the same place and for any errors introduced in the current iteration to not affected any detectors defined
    at least that many iterations later (including detectors after the end of the loop).
    
    This flag substantially improves performance on circuits with `REPEAT` blocks that have large repetition counts. The
    analysis will take less time and the output will be more compact.
    
    Note that, although logical observables can "cross" the loop without preventing loop folding, detectors CANNOT. If there
    is any detector introduced after the loop, whose sensitivity region extends to before the loop, loop folding will fail.
    This is disastrous for loops with repetition counts in the billions, because in that case loop folding is the difference
    between the error analysis finishing in seconds instead of days.
    
    
- <a name="--ignore_decomposition_failures"></a>**`--ignore_decomposition_failures`**
    
    When this flag is set, circuit errors that fail to decompose into graphlike
    detector error model errors no longer cause the conversion process to abort.
    Instead, the undecomposed error is inserted into the output. Whatever processes
    the detector error model is then responsible for dealing with the undecomposed
    errors (e.g. a tool may choose to simply ignore them).
    
    Irrelevant unless --decompose_errors is specified.
    
    
- <a name="--in"></a>**`--in`**
    Specifies an input file to read from, instead of stdin.
    
    What the file is used for depends on the mode stim is executing in. For example, in `stim sample` mode the circuit to
    sample from is read from stdin (or the file specified by `--in`) whereas in `--m2d` mode the measurement data to convert
    is read from stdin (or the file specified by `--in`).
    
    
- <a name="--in_format"></a>**`--in_format`**
    Specifies a data format to use when reading shot data, e.g. `01` or `r8`.
    
    See `stim help formats` for a list of supported formats.
    
    
- <a name="--obs_out"></a>**`--obs_out`**
    Specifies a file to write observable flip data to.
    
    When sampling detection event data, this is an alternative to --append_observables which has the benefit
    of never mixing the two types of data together.
    
    
- <a name="--obs_out_format"></a>**`--obs_out_format`**
    The format to use when writing observable flip data (e.g. b8 or 01).
    
    
- <a name="--out"></a>**`--out`**
    Specifies an output file to read from, instead of stdout.
    
    What the output is used for depends on the mode stim is executing in. For example, in `stim gen` mode the generated circuit
    is written to stdout (or the file specified by `--out`) whereas in `stim sample` mode the sampled measurement data is
    written to stdout (or the file specified by `--out`).
    
    
- <a name="--out_format"></a>**`--out_format`**
    Specifies a data format to use when writing shot data, e.g. `01` or `r8`.
    
    Defaults to `01` when not specified.
    
    See `stim help formats` for a list of supported formats.
    
    
- <a name="--replay_err_in"></a>**`--replay_err_in`**
    Specifies a file to read error data to replay from.
    
    When replaying error information, errors are no longer sampled randomly but instead driven by the file data.
    For example, this file data could come from a previous run that wrote error data using --err_out.
    
    
- <a name="--replay_err_in_format"></a>**`--replay_err_in_format`**
    The format to use when reading error data to replay. (e.g. b8 or 01).
    
    
- <a name="--rounds"></a>**`--rounds`**
    The number of times the circuit's measurement qubits are measured.
    
    The number of rounds must be an integer between 1 and a quintillion (10^18). Different codes/tasks may place additional
    constraints on the number of rounds (e.g. enough rounds to have measured all the stabilizers at least once).
    
    
- <a name="--seed"></a>**`--seed`**
    Make simulation results PARTIALLY deterministic.
    
    When not set, the random number generator is seeded with external system entropy.
    
    When set to an integer, using the exact same other flags on the exact same machine with the exact
    same version of Stim will produce the exact same simulation results.
    The integer must be a non-negative 64 bit signed integer.
    
    CAUTION: simulation results *WILL NOT* be consistent between versions of Stim. This restriction is
    present to make it possible to have future optimizations to the random sampling, and is enforced by
    introducing intentional differences in the seeding strategy from version to version.
    
    CAUTION: simulation results *MAY NOT* be consistent across machines. For example, using the same
    seed on a machine that supports AVX instructions and one that only supports SSE instructions may
    produce different simulation results.
    
    CAUTION: simulation results *MAY NOT* be consistent if you vary other flags and modes. For example,
    `--skip_reference_sample` may result in fewer calls the to the random number generator before reported
    sampling begins. More generally, using the same seed for `stim sample` and `stim detect` will not
    result in detection events corresponding to the measurement results.
    
    
- <a name="--shots"></a>**`--shots`**
    Specifies the number of times to run a circuit, producing data each time.
    
    Defaults to 1.
    Must be an integer between 0 and a quintillion (10^18).
    
    
- <a name="--single"></a>**`--single`**
    Instead of returning every circuit error that corresponds to a dem error, only return one representative circuit error.
    
    
- <a name="--skip_reference_sample"></a>**`--skip_reference_sample`**
    Instead of computing a reference sample for the given circuit, use
    a vacuous reference sample where where all measurement results are 0.
    
    Skipping the reference sample can significantly improve performance, because acquiring the reference sample requires
    using the tableau simulator. If the vacuous reference sample is actually a result that can be produced by the circuit,
    under noiseless execution, then specifying this flag has no observable outcome other than improving performance.
    
    When the all-zero sample isn't a result that can be produced by the circuit under noiseless execution, the effects of
    skipping the reference sample vary depending on the mode. For example, in measurement sampling mode, the reported
    measurements are not true measurement results but rather reports of which measurement results would have been flipped
    due to errors or Heisenberg uncertainty. They need to be XOR'd against a noiseless reference sample to become true
    measurement results.
    
    
- <a name="--sweep"></a>**`--sweep`**
    Specifies a per-shot sweep data file.
    
    Sweep bits are used to vary whether certain Pauli gates are included in a circuit, or not, from shot to shot.
    For example, if a circuit contains the instruction "CX sweep[5] 0" then there is an X pauli that is included
    only in shots where the corresponding sweep data has the bit at index 5 set to True.
    
    
- <a name="--sweep_format"></a>**`--sweep_format`**
    Specifies the format sweep data is stored in (e.g. b8 or 01).
    
    
- <a name="--task"></a>**`--task`**
    What the generated circuit should do; the experiment it should run.
    
        Different error correcting codes support different tasks.
    
        `--task=memory` (repetition_code):
            Initialize a logical `|0>`,
            preserve it against noise for the given number of rounds,
            then measure.
        `--task=rotated_memory_x` (surface_code):
            Initialize a logical `|+>` in a rotated surface code,
            preserve it against noise for the given number of rounds,
            then measure in the X basis.
        `--task=rotated_memory_z` (surface_code):
            Initialize a logical `|0>` in a rotated surface code,
            preserve it against noise for the given number of rounds,
            then measure in the X basis.
        `--task=unrotated_memory_x` (surface_code):
            Initialize a logical `|+>` in an unrotated surface code,
            preserve it against noise for the given number of rounds,
            then measure in the Z basis.
        `--task=unrotated_memory_z` (surface_code):
            Initialize a logical `|0>` in an unrotated surface code,
            preserve it against noise for the given number of rounds,
            then measure in the Z basis.
        `--task=memory_xyz` (color_code):
            Initialize a logical `|0>`,
            preserve it against noise for the given number of rounds,
            then measure.
            Use a color code that alternates between measuring X, then Y, then Z stabilizers.
    
    
