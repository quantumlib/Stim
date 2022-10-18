# Stim command line reference

## Index

- [stim analyze_errors](#analyze_errors)
- [stim detect](#detect)
- [stim explain_errors](#explain_errors)
- [stim gen](#gen)
- [stim help](#help)
- [stim m2d](#m2d)
- [stim repl](#repl)
- [stim sample](#sample)
- [stim sample_dem](#sample_dem)
## Commands

<a name="analyze_errors"></a>
### stim analyze_errors

```
NAME
    stim analyze_errors

SYNOPSIS
    stim analyze_errors \
        [--allow_gauge_detectors] \
        [--approximate_disjoint_errors [probability]] \
        [--block_decompose_from_introducing_remnant_edges] \
        [--decompose_errors] \
        [--fold_loops] \
        [--ignore_decomposition_failures] \
        [--in filepath] \
        [--out filepath]

DESCRIPTION
    Converts a circuit into a detector error model.

OPTIONS
    --allow_gauge_detectors
        Allows non-deterministic detectors to appear in the circuit.
        
        Normally (without `--allow_gauge_detectors`), when a detector's
        detecting region anti-commutes with a reset or measurement, stim
        will raise an exception when analyzing the circuit. When
        `--allow_gauge_detectors` is set, stim will instead append an error
        mechanism into the detector error model that has a probability of
        50% and flips all the detectors that anticommute with the operation.
        
        This is potentially useful in situations where the layout of
        detectors is supposed to stay fixed despite variations in the
        circuit structure. Decoders can interpret the existence of the 50%
        error as a weight 0 edge saying that the detectors should be fused
        together.
        
        For example, in the following stim circuit, the two detectors each
        anticommute with the reset operation:
        
            R 0
            H 0
            CNOT 0 1
            M 0 1
            DETECTOR rec[-1]
            DETECTOR rec[-2]
        
        Without `--allow_gauge_detectors`, stim will raise an exception when
        analyzing this circuit. With `--allow_gauge_detectors`, stim will
        add `error(0.5) D1 D2` to the output detector error model.
        
        BEWARE that gauge detectors are very tricky to work with, and not
        necessarily supported by all tools (even within stim itself). For
        example, when converting from measurements to detection events,
        there isn't a single choice for whether or not each individual gauge
        detector produced a detection event. This means that it is valid
        behavior for one conversion from measurements to detection events
        to give different results from another, as long as the gauge
        detectors that anticommute with the same operations flip together in
        a consistent fashion that respects the structure of the circuit.
        

    --approximate_disjoint_errors
        Allows disjoint errors to be approximated during the conversion.
        
        Detector error models require that all error mechanisms be
        specified as independent mechanisms. But some of the circuit error
        mechanisms that Stim allows can express errors that don't correspond
        to independent mechanisms. For example, the custom error channel
        `PAULI_CHANNEL_1(0.1, 0.2, 0.0)` can't be expressed exactly as a set
        of independent error mechanisms. But it can be approximated as an
        `X_ERROR(0.1)` followed by a `Y_ERROR(0.2)`.
        
        This flag can be set to any probability between 0 (the default when
        not specified) and 1 (the default when specified without a value).
        When set to a value strictly between 0 and 1, this determines the
        maximum disjoint probability that is allowed to be approximated as
        an independent probability.
        
        Without `--approximate_disjoint_errors`, attempting to convert a
        circuit containing `PAULI_CHANNEL_1(0.1, 0.2, 0.0)` will fail with
        an error stating an approximation is needed. With
        `--approximate_disjoint_errors`, the conversion will succeed by
        approximating the error into an `X_ERROR(0.1)` followed by an
        independent `Y_ERROR(0.2)`.
        
        Note that, although `DEPOLARIZE1` and `DEPOLARIZE2` are often
        defined in terms of disjoint errors, they can be exactly converted
        into a set of independent errors (unless the probability of the
        depolarizing error occurring exceeds maximum mixing, which is 75%
        for `DEPOLARIZE1` and 93.75% for `DEPOLARIZE2`). So the
        `--approximate_disjoint_errors` flag isn't needed for depolarizing
        errors that appear in practice.
        
        The error mechanisms that require approximations are:
        - PAULI_CHANNEL_1
        - PAULI_CHANNEL_2
        - ELSE_CORRELATED_ERROR
        
        In principle some custom Pauli channels can be converted exactly,
        but Stim does not currently contain logic that attempts to do this.
        

    --block_decompose_from_introducing_remnant_edges
        Prevents A*B from being decomposed unless A,B BOTH appear elsewhere.
        
        Irrelevant unless `--decompose_errors` is specified.
        
        When `--decompose_errors` is specified, any circuit error that
        causes more than two detection events must be decomposed into a
        set of errors with at most two detection events. The main constraint
        on this process is that it must not use errors that couldn't
        otherwise occur, since introducing such errors could violate
        important properties that are used for decoding. For example, in the
        normal surface code, it is very important that the decoding graphs
        for X errors and Z errors are disjoint in the bulk, and decomposing
        an error into a set of errors that violated this property would be
        disastrous.
        
        However, a corner case in this logic occurs if an error E1 that
        produces detection events A*B needs to be decomposed when an error
        E2 that produces detection events A appears elsewhere but no error
        producing detection events B appears elsewhere. The detection events
        B can be produced by both E1 and E2 occurring, but this a
        combination of two errors and so treating it as one error can cause
        problems. For example, it can result in the code distance appearing
        to be smaller than it actually is. Introducing B is referred to as
        introducing a "remnant edge" because B *only* appears in the
        detector error model as a remnant of removing A from A*B.
        
        By default, Stim does allow remnant edges to be introduced. Stim
        will only do this if it is absolutely necessary, but it *will* do
        it. And there are in fact QEC circuits where the decomposition
        requires these edges to succeed. But sometimes the presence of a
        remnant edge is a hint that the DETECTOR declarations in the circuit
        are subtly wrong. To cause the decomposition process to fail in
        this case, the `--block_decompose_from_introducing_remnant_edges`
        can be specified.
        

    --decompose_errors
        Decomposes errors with many detection events into "graphlike" parts.
        
        When `--decompose_errors` is specified, Stim will suggest how errors
        that cause more than 2 detection events (non-graphlike errors) can
        be decomposed into errors with at most 2 detection events (graphlike
        errors). For example, an error like `error(0.1) D0 D1 D2 D3` may be
        instead output as `error(0.1) D0 D1 ^ D2 D3` or as
        `error(0.1) D0 D3 ^ D1 ^ D2`.
        
        The purpose of this feature is to make matching apply to more cases.
        A common decoding strategy is "matching", where detection events are
        paired up in order to determine which errors occurred. Matching only
        works when the Tanner graph of the problem is a graph, not a
        hypergraph. In other words, it requires all errors to produce at
        most two detection events. This is a problem, because in practice
        there are essentially always circuit error mechanisms that produce
        more than two detection events. For example, in a CSS surface code,
        Y type errors on the data qubits will produce four detection events.
        For matching to work in these cases, non-graphlike errors (errors
        with more than two detection events) need to be approximated as a
        combination of graphlike errors.
        
        When Stim is decomposing errors, the main guarantee that it provides
        is that it will not introduce error mechanisms with symptoms that
        are otherwise impossible or that would require a combination of
        non-local errors to actually achieve. Informally, Stim guarantees it
        will preserve the "structure" of the detector error model when
        suggesting decompositions.
        
        It's also worth noting that the suggested decompositions are
        information preserving: the undecomposed model can always be
        recovered by simply filtering out all `^` characters splitting the
        errors into suggested components.
        
        Stim uses two strategies for decomposing errors: intra-channel and
        inter-channel.
        
        The *intra-channel* strategy is always applied first, and works by
        looking at the various detector/observable sets produced by each
        case of a single noise channel. If some cases are products of other
        cases, that product is *always* decomposed. For example, suppose
        that a single qubit depolarizing channel has a `Y5` case that
        produces four detection events `D0 D1 D2 D3`, an `X5` case that
        produces two detection events `D0 D1`, and a `Z5` case that produces
        two detection events `D2 D3`. Because `D0 D1 D2 D3` is the
        combination of `D0 D1` and `D2 D3`, the `Y5` case will be decomposed
        into `D0 D1 ^ D2 D3`. An important corner case here is the corner of
        the CSS surface code, where a Y error has two symptoms which is
        graphlike but because the intra-channel strategy is aggressive the
        Y error will still be decomposed into X and Z pieces. This can keep
        the X and Z decoding graphs disjoint.
        
        The *inter-channel* strategy is used when an error component is
        still not graphlike after the intra-channel strategy was applied.
        This strategy searches over all other error mechanisms looking for a
        combination that explains the error. If
        `--block_decompose_from_introducing_remnant_edges` is specified then
        this must be an exact match, otherwise the match can omit up to two
        of the symptoms in the error (resulting in the producing of a
        "remnant edge").
        
        Note that the code implementing these strategies does not special
        case any Pauli basis. For example, it does not prefer to decompose
        Y into X*Z as opposed to X into Y*Z. It also does not prefer to
        decompose YY into IY*YI as opposed to IY into YY*YI. The code
        operates purely in terms of the sets of symptoms produced by the
        various cases, with little regard for how those sets were produced.
        
        If these strategies fail to decompose error into graphlike pieces,
        Stim will throw an error saying it failed to find a satisfying
        decomposition.
        

    --fold_loops
        Allows the output to contain `repeat` blocks.
        
        This flag substantially improves performance on circuits with
        `REPEAT` blocks with large repetition counts. The analysis will take
        less time and the output will be more compact. This option is only
        OFF by default to maintain strict backwards compatibility of the
        output.
        
        When a circuit contains a `REPEAT` block, the structure of the
        detectors often settles into a form that is identical from iteration
        to iteration. Specifying the `--fold_loops` option tells Stim to
        watch for periodicity in the structure of detectors by using a
        "tortoise and hare" algorithm (see
        https://en.wikipedia.org/wiki/Cycle_detection ).
        This improves the asymptotic complexity of analyzing the loop from
        O(total_repetitions) to O(cycle_period).
        
        Note that, although logical observables can "cross" from the end of
        the loop to the start of the loop without preventing loop folding,
        detectors CANNOT. If there is any detector introduced after the
        loop, whose sensitivity region extends to before the loop, loop
        folding will fail and the code will fall back to flattening the
        loop. This is disastrous for loops with huge repetition counts (e.g.
        in the billions) because in that case loop folding is the difference
        between the error analysis finishing in seconds instead of in days.
        

    --ignore_decomposition_failures
        Allows non-graphlike errors into the output when decomposing errors.
        
        Irrelevant unless `--decompose_errors` is specified.
        
        Without `--ignore_decomposition_failures`, circuit errors that fail
        to decompose into graphlike detector error model errors will cause
        an error and abort the conversion process.
        
        When `--ignore_decomposition_failures` is specified, circuit errors
        that fail to decompose into graphlike detector error model errors
        produce non-graphlike detector error models. Whatever processes
        the detector error model is then responsible for dealing with the
        undecomposed errors (e.g. a tool may choose to simply ignore them).
        

    --in
        Chooses the stim circuit file to read the circuit to convert from.
        
        By default, the circuit is read from stdin. When `--in $FILEPATH` is
        specified, the circuit is instead read from the file at $FILEPATH.
        
        The input should be a stim circuit. See:
        https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md
        

    --out
        Chooses where to write the output detector error model.
        
        By default, the output is written to stdout. When `--out $FILEPATH`
        is specified, the output is instead written to the file at $FILEPATH.
        
        The output is a stim detector error model. See:
        https://github.com/quantumlib/Stim/blob/main/doc/file_format_dem_detector_error_model.md
        

EXAMPLES
    Example #1
        >>> cat example_circuit.stim
        R 0 1
        X_ERROR(0.125) 0 1
        CNOT 0 1
        M 0 1
        DETECTOR rec[-1]
        DETECTOR rec[-2]
        
        >>> stim analyze_errors --in example_circuit.stim
        error(0.125) D0
        error(0.125) D0 D1
        

    Example #2
        >>> stim gen \
                --code repetition_code \
                --task memory \
                --distance 3 \
                --rounds 1000 \
                --after_reset_flip_probability 0.125 \
                > rep_code.stim
        >>> stim analyze_errors --fold_loops --in rep_code.stim
        error(0.125) D0
        error(0.125) D0 D1
        error(0.125) D0 D2
        error(0.125) D1 D3
        error(0.125) D1 L0
        error(0.125) D2 D4
        error(0.125) D3 D5
        detector(1, 0) D0
        detector(3, 0) D1
        repeat 998 {
            error(0.125) D4 D6
            error(0.125) D5 D7
            shift_detectors(0, 1) 0
            detector(1, 0) D2
            detector(3, 0) D3
            shift_detectors 2
        }
        shift_detectors(0, 1) 0
        detector(1, 0) D2
        detector(3, 0) D3
        detector(1, 1) D4
        detector(3, 1) D5
        
```

<a name="detect"></a>
### stim detect

```
NAME
    stim detect

SYNOPSIS
    stim detect \
        [--append_observables] \
        [--in filepath] \
        [--obs_out filepath] \
        [--obs_out_format 01|b8|r8|ptb64|hits|dets] \
        [--out filepath] \
        [--out_format 01|b8|r8|ptb64|hits|dets] \
        [--seed int] \
        [--shots int]

DESCRIPTION
    Sample detection events and observable flips from a circuit.

OPTIONS
    --append_observables
        Appends observable flips to the end of samples as extra detectors.
        
        PREFER --obs_out OVER THIS FLAG. Mixing the observable flip data
        into detection event data tends to require simply separating them
        again immediately, creating unnecessary work. For example, when
        testing a decoder, you do not want to give the observable flips to
        the decoder because that is the information the decoder is supposed
        to be predicting from the detection events.
        
        This flag causes observable flip data to be appended to each sample,
        as if the observables were extra detectors at the end of the
        circuit. For example, if there are 100 detectors and 10 observables
        in the circuit, then the output will contain 110 detectors and the
        last 10 are the observables.
        
        Note that, when using `--out_format dets`, this option is implicitly
        activated but observables are not appended as if they were
        detectors (because `dets` has type hinting information). For
        example, in the example from the last paragraph, the observables
        would be named `L0` through `L9` instead of `D100` through `D109`.
        

    --in
        Chooses the stim circuit file to read the circuit to sample from.
        
        By default, the circuit is read from stdin. When `--in $FILEPATH` is
        specified, the circuit is instead read from the file at $FILEPATH.
        
        The input should be a stim circuit. See:
        https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md
        

    --obs_out
        Specifies the file to write observable flip data to.
        
        When sampling detection event data, the goal is typically to predict
        whether or not the logical observables were flipped by using the
        detection events. This argument specifies where to write that
        observable flip data.
        
        If this argument isn't specified, the observable flip data isn't
        written to a file.
        
        The output is in a format specified by `--obs_format`. See:
        https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        

    --obs_out_format
        Specifies the data format to use when writing observable flip data.
        
        Irrelevant unless `--obs_out` is specified.
        
        The available formats are:
        
            01 (default): dense human readable
            b8: bit packed binary
            r8: run length binary
            ptb64: partially transposed bit packed binary for SIMD
            hits: sparse human readable
            dets: sparse human readable with type hints
        
        For a detailed description of each result format, see the result
        format reference:
        https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        

    --out
        Chooses where to write the sampled data to.
        
        By default, the output is written to stdout. When `--out $FILEPATH`
        is specified, the output is instead written to the file at $FILEPATH.
        
        The output is in a format specified by `--out_format`. See:
        https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        

    --out_format
        Specifies the data format to use when writing output data.
        
        The available formats are:
        
            01 (default): dense human readable
            b8: bit packed binary
            r8: run length binary
            ptb64: partially transposed bit packed binary for SIMD
            hits: sparse human readable
            dets: sparse human readable with type hints
        
        For a detailed description of each result format, see the result
        format reference:
        https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        

    --seed
        Makes simulation results PARTIALLY deterministic.
        
        The seed integer must be a non-negative 64 bit signed integer.
        
        When `--seed` isn't specified, the random number generator is seeded
        using fresh entropy requested from the operating system.
        
        When `--seed #` is set, the exact same simulation results will be
        produced every time ASSUMING:
        
        - the exact same other flags are specified
        - the exact same version of Stim is being used
        - the exact same machine architecture is being used (for example,
            you're not switching from a machine that has AVX2 instructions
            to one that doesn't).
        
        CAUTION: simulation results *WILL NOT* be consistent between
        versions of Stim. This restriction is present to make it possible to
        have future optimizations to the random sampling, and is enforced by
        introducing intentional differences in the seeding strategy from
        version to version.
        
        CAUTION: simulation results *MAY NOT* be consistent across machines.
        For example, using the same seed on a machine that supports AVX
        instructions and one that only supports SSE instructions may produce
        different simulation results.
        
        CAUTION: simulation results *MAY NOT* be consistent if you vary
        other flags and modes. For example, `--skip_reference_sample` may
        result in fewer calls the to the random number generator before
        reported sampling begins. More generally, using the same seed for
        `stim sample` and `stim detect` will not result in detection events
        corresponding to the measurement results.
        

    --shots
        Specifies the number of samples to take from the circuit.
        
        Defaults to 1.
        Must be an integer between 0 and a quintillion (10^18).
        

EXAMPLES
    Example #1
        >>> cat example.stim
        H 0
        CNOT 0 1
        X_ERROR(0.1) 0 1
        M 0 1
        DETECTOR rec[-1] rec[-2]
        
        >>> stim detect --shots 5
        0
        1
        0
        0
        0
        

    Example #2
        >>> cat example.stim
        # Single-shot X-basis rep code circuit.
        RX 0 1 2 3 4 5 6
        MPP X0*X1 X1*X2 X2*X3 X3*X4 X4*X5 X5*X6
        Z_ERROR(0.1) 0 1 2 3 4 5 6
        MPP X0 X1 X2 X3 X4 X5 X6
        DETECTOR rec[-1] rec[-2] rec[-8]   # X6 X5 now = X5*X6 before
        DETECTOR rec[-2] rec[-3] rec[-9]   # X5 X4 now = X4*X5 before
        DETECTOR rec[-3] rec[-4] rec[-10]  # X4 X3 now = X3*X4 before
        DETECTOR rec[-4] rec[-5] rec[-11]  # X3 X2 now = X2*X3 before
        DETECTOR rec[-5] rec[-6] rec[-12]  # X2 X1 now = X1*X2 before
        DETECTOR rec[-6] rec[-7] rec[-13]  # X1 X0 now = X0*X1 before
        OBSERVABLE_INCLUDE(0) rec[-1]
        
        >>> stim detect --shots 10 --append_observables --out_format=dets
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

<a name="explain_errors"></a>
### stim explain_errors

```
NAME
    stim explain_errors

SYNOPSIS
    stim explain_errors \
        [--dem_filter filepath] \
        [--in filepath] \
        [--out filepath] \
        [--single]

DESCRIPTION
    Find circuit errors that produce certain detection events.
    
    Note that this command does not attempt to explain detection events
    by using multiple errors. This command can only tell you how to
    produce a set of detection events if they correspond to a specific
    single physical error annotated into the circuit.
    
    If you need to explain a detection event set using multiple errors,
    use a decoder such as pymatching to find the set of single detector
    error model errors that are needed and then use this command to
    convert those specific errors into circuit errors.
    

OPTIONS
    --dem_filter
        Specifies a detector error model to use as a filter.
        
        If `--dem_filter` isn't specified, an explanation of every single
        set of symptoms that can be produced by the circuit.
        
        If `--dem_filter` is specified, only explanations of the error
        mechanisms present in the filter will be output. This is useful when
        you are interested in a specific set of detection events.
        
        The filter is specified as a detector error model file. See
        https://github.com/quantumlib/Stim/blob/main/doc/file_format_dem_detector_error_model.md
        

    --in
        Chooses the stim circuit file to read the explanatory circuit from.
        
        By default, the circuit is read from stdin. When `--in $FILEPATH` is
        specified, the circuit is instead read from the file at $FILEPATH.
        
        The input should be a stim circuit. See:
        https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md
        

    --out
        Chooses where to write the explanations to.
        
        By default, the output is written to stdout. When `--out $FILEPATH`
        is specified, the output is instead written to the file at $FILEPATH.
        
        The output is in an arbitrary semi-human-readable format.
        

    --single
        Explain using a single simple error instead of all possible errors.
        
        When `--single` isn't specified, every single circuit error that
        produces a specific detector error model is output as a possible
        explanation of that error.
        
        When `--single` is specified, only the simplest circuit error is
        output. The "simplest" error is chosen by using heuristics such as
        "has fewer Pauli terms" and "happens earlier".
        

EXAMPLES
    Example #1
        >>> stim gen \
            --code surface_code \
            --task rotated_memory_z \
            --distance 5 \
            --rounds 10 \
            --after_clifford_depolarization 0.001 \
            > example.stim
        >>> echo "error(1) D97 D102" > example.dem
        
        >>> stim explain_errors \
            --single \
            --in example.stim \
            --dem_filter example.dem
        ExplainedError {
            dem_error_terms: D97[coords 4,6,4] D102[coords 2,8,4]
            CircuitErrorLocation {
                flipped_pauli_product: Z36[coords 3,7]
                Circuit location stack trace:
                    (after 25 TICKs)
                    at instruction #83 (a REPEAT 9 block) in the circuit
                    after 2 completed iterations
                    at instruction #12 (DEPOLARIZE2) in the REPEAT block
                    at targets #3 to #4 of the instruction
                    resolving to DEPOLARIZE2(0.001) 46[coords 2,8] 36[coords 3,7]
            }
        }
        
```

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
    Allows non-deterministic detectors to appear in the circuit.
    
    Normally (without `--allow_gauge_detectors`), when a detector's
    detecting region anti-commutes with a reset or measurement, stim
    will raise an exception when analyzing the circuit. When
    `--allow_gauge_detectors` is set, stim will instead append an error
    mechanism into the detector error model that has a probability of
    50% and flips all the detectors that anticommute with the operation.
    
    This is potentially useful in situations where the layout of
    detectors is supposed to stay fixed despite variations in the
    circuit structure. Decoders can interpret the existence of the 50%
    error as a weight 0 edge saying that the detectors should be fused
    together.
    
    For example, in the following stim circuit, the two detectors each
    anticommute with the reset operation:
    
        R 0
        H 0
        CNOT 0 1
        M 0 1
        DETECTOR rec[-1]
        DETECTOR rec[-2]
    
    Without `--allow_gauge_detectors`, stim will raise an exception when
    analyzing this circuit. With `--allow_gauge_detectors`, stim will
    add `error(0.5) D1 D2` to the output detector error model.
    
    BEWARE that gauge detectors are very tricky to work with, and not
    necessarily supported by all tools (even within stim itself). For
    example, when converting from measurements to detection events,
    there isn't a single choice for whether or not each individual gauge
    detector produced a detection event. This means that it is valid
    behavior for one conversion from measurements to detection events
    to give different results from another, as long as the gauge
    detectors that anticommute with the same operations flip together in
    a consistent fashion that respects the structure of the circuit.
    
    
- <a name="--append_observables"></a>**`--append_observables`**
    Appends observable flips to the end of samples as extra detectors.
    
    PREFER --obs_out OVER THIS FLAG. Mixing the observable flip data
    into detection event data tends to require simply separating them
    again immediately, creating unnecessary work. For example, when
    testing a decoder, you do not want to give the observable flips to
    the decoder because that is the information the decoder is supposed
    to be predicting from the detection events.
    
    This flag causes observable flip data to be appended to each sample,
    as if the observables were extra detectors at the end of the
    circuit. For example, if there are 100 detectors and 10 observables
    in the circuit, then the output will contain 110 detectors and the
    last 10 are the observables.
    
    Note that, when using `--out_format dets`, this option is implicitly
    activated but observables are not appended as if they were
    detectors (because `dets` has type hinting information). For
    example, in the example from the last paragraph, the observables
    would be named `L0` through `L9` instead of `D100` through `D109`.
    
    
- <a name="--approximate_disjoint_errors"></a>**`--approximate_disjoint_errors`**
    Allows disjoint errors to be approximated during the conversion.
    
    Detector error models require that all error mechanisms be
    specified as independent mechanisms. But some of the circuit error
    mechanisms that Stim allows can express errors that don't correspond
    to independent mechanisms. For example, the custom error channel
    `PAULI_CHANNEL_1(0.1, 0.2, 0.0)` can't be expressed exactly as a set
    of independent error mechanisms. But it can be approximated as an
    `X_ERROR(0.1)` followed by a `Y_ERROR(0.2)`.
    
    This flag can be set to any probability between 0 (the default when
    not specified) and 1 (the default when specified without a value).
    When set to a value strictly between 0 and 1, this determines the
    maximum disjoint probability that is allowed to be approximated as
    an independent probability.
    
    Without `--approximate_disjoint_errors`, attempting to convert a
    circuit containing `PAULI_CHANNEL_1(0.1, 0.2, 0.0)` will fail with
    an error stating an approximation is needed. With
    `--approximate_disjoint_errors`, the conversion will succeed by
    approximating the error into an `X_ERROR(0.1)` followed by an
    independent `Y_ERROR(0.2)`.
    
    Note that, although `DEPOLARIZE1` and `DEPOLARIZE2` are often
    defined in terms of disjoint errors, they can be exactly converted
    into a set of independent errors (unless the probability of the
    depolarizing error occurring exceeds maximum mixing, which is 75%
    for `DEPOLARIZE1` and 93.75% for `DEPOLARIZE2`). So the
    `--approximate_disjoint_errors` flag isn't needed for depolarizing
    errors that appear in practice.
    
    The error mechanisms that require approximations are:
    - PAULI_CHANNEL_1
    - PAULI_CHANNEL_2
    - ELSE_CORRELATED_ERROR
    
    In principle some custom Pauli channels can be converted exactly,
    but Stim does not currently contain logic that attempts to do this.
    
    
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
    
    
- <a name="--block_decompose_from_introducing_remnant_edges"></a>**`--block_decompose_from_introducing_remnant_edges`**
    Prevents A*B from being decomposed unless A,B BOTH appear elsewhere.
    
    Irrelevant unless `--decompose_errors` is specified.
    
    When `--decompose_errors` is specified, any circuit error that
    causes more than two detection events must be decomposed into a
    set of errors with at most two detection events. The main constraint
    on this process is that it must not use errors that couldn't
    otherwise occur, since introducing such errors could violate
    important properties that are used for decoding. For example, in the
    normal surface code, it is very important that the decoding graphs
    for X errors and Z errors are disjoint in the bulk, and decomposing
    an error into a set of errors that violated this property would be
    disastrous.
    
    However, a corner case in this logic occurs if an error E1 that
    produces detection events A*B needs to be decomposed when an error
    E2 that produces detection events A appears elsewhere but no error
    producing detection events B appears elsewhere. The detection events
    B can be produced by both E1 and E2 occurring, but this a
    combination of two errors and so treating it as one error can cause
    problems. For example, it can result in the code distance appearing
    to be smaller than it actually is. Introducing B is referred to as
    introducing a "remnant edge" because B *only* appears in the
    detector error model as a remnant of removing A from A*B.
    
    By default, Stim does allow remnant edges to be introduced. Stim
    will only do this if it is absolutely necessary, but it *will* do
    it. And there are in fact QEC circuits where the decomposition
    requires these edges to succeed. But sometimes the presence of a
    remnant edge is a hint that the DETECTOR declarations in the circuit
    are subtly wrong. To cause the decomposition process to fail in
    this case, the `--block_decompose_from_introducing_remnant_edges`
    can be specified.
    
    
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
    Decomposes errors with many detection events into "graphlike" parts.
    
    When `--decompose_errors` is specified, Stim will suggest how errors
    that cause more than 2 detection events (non-graphlike errors) can
    be decomposed into errors with at most 2 detection events (graphlike
    errors). For example, an error like `error(0.1) D0 D1 D2 D3` may be
    instead output as `error(0.1) D0 D1 ^ D2 D3` or as
    `error(0.1) D0 D3 ^ D1 ^ D2`.
    
    The purpose of this feature is to make matching apply to more cases.
    A common decoding strategy is "matching", where detection events are
    paired up in order to determine which errors occurred. Matching only
    works when the Tanner graph of the problem is a graph, not a
    hypergraph. In other words, it requires all errors to produce at
    most two detection events. This is a problem, because in practice
    there are essentially always circuit error mechanisms that produce
    more than two detection events. For example, in a CSS surface code,
    Y type errors on the data qubits will produce four detection events.
    For matching to work in these cases, non-graphlike errors (errors
    with more than two detection events) need to be approximated as a
    combination of graphlike errors.
    
    When Stim is decomposing errors, the main guarantee that it provides
    is that it will not introduce error mechanisms with symptoms that
    are otherwise impossible or that would require a combination of
    non-local errors to actually achieve. Informally, Stim guarantees it
    will preserve the "structure" of the detector error model when
    suggesting decompositions.
    
    It's also worth noting that the suggested decompositions are
    information preserving: the undecomposed model can always be
    recovered by simply filtering out all `^` characters splitting the
    errors into suggested components.
    
    Stim uses two strategies for decomposing errors: intra-channel and
    inter-channel.
    
    The *intra-channel* strategy is always applied first, and works by
    looking at the various detector/observable sets produced by each
    case of a single noise channel. If some cases are products of other
    cases, that product is *always* decomposed. For example, suppose
    that a single qubit depolarizing channel has a `Y5` case that
    produces four detection events `D0 D1 D2 D3`, an `X5` case that
    produces two detection events `D0 D1`, and a `Z5` case that produces
    two detection events `D2 D3`. Because `D0 D1 D2 D3` is the
    combination of `D0 D1` and `D2 D3`, the `Y5` case will be decomposed
    into `D0 D1 ^ D2 D3`. An important corner case here is the corner of
    the CSS surface code, where a Y error has two symptoms which is
    graphlike but because the intra-channel strategy is aggressive the
    Y error will still be decomposed into X and Z pieces. This can keep
    the X and Z decoding graphs disjoint.
    
    The *inter-channel* strategy is used when an error component is
    still not graphlike after the intra-channel strategy was applied.
    This strategy searches over all other error mechanisms looking for a
    combination that explains the error. If
    `--block_decompose_from_introducing_remnant_edges` is specified then
    this must be an exact match, otherwise the match can omit up to two
    of the symptoms in the error (resulting in the producing of a
    "remnant edge").
    
    Note that the code implementing these strategies does not special
    case any Pauli basis. For example, it does not prefer to decompose
    Y into X*Z as opposed to X into Y*Z. It also does not prefer to
    decompose YY into IY*YI as opposed to IY into YY*YI. The code
    operates purely in terms of the sets of symptoms produced by the
    various cases, with little regard for how those sets were produced.
    
    If these strategies fail to decompose error into graphlike pieces,
    Stim will throw an error saying it failed to find a satisfying
    decomposition.
    
    
- <a name="--dem_filter"></a>**`--dem_filter`**
    Specifies a detector error model to use as a filter.
    
    If `--dem_filter` isn't specified, an explanation of every single
    set of symptoms that can be produced by the circuit.
    
    If `--dem_filter` is specified, only explanations of the error
    mechanisms present in the filter will be output. This is useful when
    you are interested in a specific set of detection events.
    
    The filter is specified as a detector error model file. See
    https://github.com/quantumlib/Stim/blob/main/doc/file_format_dem_detector_error_model.md
    
    
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
    Allows the output to contain `repeat` blocks.
    
    This flag substantially improves performance on circuits with
    `REPEAT` blocks with large repetition counts. The analysis will take
    less time and the output will be more compact. This option is only
    OFF by default to maintain strict backwards compatibility of the
    output.
    
    When a circuit contains a `REPEAT` block, the structure of the
    detectors often settles into a form that is identical from iteration
    to iteration. Specifying the `--fold_loops` option tells Stim to
    watch for periodicity in the structure of detectors by using a
    "tortoise and hare" algorithm (see
    https://en.wikipedia.org/wiki/Cycle_detection ).
    This improves the asymptotic complexity of analyzing the loop from
    O(total_repetitions) to O(cycle_period).
    
    Note that, although logical observables can "cross" from the end of
    the loop to the start of the loop without preventing loop folding,
    detectors CANNOT. If there is any detector introduced after the
    loop, whose sensitivity region extends to before the loop, loop
    folding will fail and the code will fall back to flattening the
    loop. This is disastrous for loops with huge repetition counts (e.g.
    in the billions) because in that case loop folding is the difference
    between the error analysis finishing in seconds instead of in days.
    
    
- <a name="--ignore_decomposition_failures"></a>**`--ignore_decomposition_failures`**
    Allows non-graphlike errors into the output when decomposing errors.
    
    Irrelevant unless `--decompose_errors` is specified.
    
    Without `--ignore_decomposition_failures`, circuit errors that fail
    to decompose into graphlike detector error model errors will cause
    an error and abort the conversion process.
    
    When `--ignore_decomposition_failures` is specified, circuit errors
    that fail to decompose into graphlike detector error model errors
    produce non-graphlike detector error models. Whatever processes
    the detector error model is then responsible for dealing with the
    undecomposed errors (e.g. a tool may choose to simply ignore them).
    
    
- <a name="--in"></a>**`--in`**
    Specifies an input file to read from, instead of stdin.
    
    What the file is used for depends on the mode stim is executing in. For example, in `stim sample` mode the circuit to
    sample from is read from stdin (or the file specified by `--in`) whereas in `--m2d` mode the measurement data to convert
    is read from stdin (or the file specified by `--in`).
    
    
- <a name="--in_format"></a>**`--in_format`**
    Specifies a data format to use when reading shot data, e.g. `01` or `r8`.
    
    See `stim help formats` for a list of supported formats.
    
    
- <a name="--obs_out"></a>**`--obs_out`**
    Specifies the file to write observable flip data to.
    
    When sampling detection event data, the goal is typically to predict
    whether or not the logical observables were flipped by using the
    detection events. This argument specifies where to write that
    observable flip data.
    
    If this argument isn't specified, the observable flip data isn't
    written to a file.
    
    The output is in a format specified by `--obs_format`. See:
    https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
    
    
- <a name="--obs_out_format"></a>**`--obs_out_format`**
    Specifies the data format to use when writing observable flip data.
    
    Irrelevant unless `--obs_out` is specified.
    
    The available formats are:
    
        01 (default): dense human readable
        b8: bit packed binary
        r8: run length binary
        ptb64: partially transposed bit packed binary for SIMD
        hits: sparse human readable
        dets: sparse human readable with type hints
    
    For a detailed description of each result format, see the result
    format reference:
    https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
    
    
- <a name="--out"></a>**`--out`**
    Specifies an output file to read from, instead of stdout.
    
    What the output is used for depends on the mode stim is executing in. For example, in `stim gen` mode the generated circuit
    is written to stdout (or the file specified by `--out`) whereas in `stim sample` mode the sampled measurement data is
    written to stdout (or the file specified by `--out`).
    
    
- <a name="--out_format"></a>**`--out_format`**
    Specifies the data format to use when writing output data.
    
    The available formats are:
    
        01 (default): dense human readable
        b8: bit packed binary
        r8: run length binary
        ptb64: partially transposed bit packed binary for SIMD
        hits: sparse human readable
        dets: sparse human readable with type hints
    
    For a detailed description of each result format, see the result
    format reference:
    https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
    
    
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
    Makes simulation results PARTIALLY deterministic.
    
    The seed integer must be a non-negative 64 bit signed integer.
    
    When `--seed` isn't specified, the random number generator is seeded
    using fresh entropy requested from the operating system.
    
    When `--seed #` is set, the exact same simulation results will be
    produced every time ASSUMING:
    
    - the exact same other flags are specified
    - the exact same version of Stim is being used
    - the exact same machine architecture is being used (for example,
        you're not switching from a machine that has AVX2 instructions
        to one that doesn't).
    
    CAUTION: simulation results *WILL NOT* be consistent between
    versions of Stim. This restriction is present to make it possible to
    have future optimizations to the random sampling, and is enforced by
    introducing intentional differences in the seeding strategy from
    version to version.
    
    CAUTION: simulation results *MAY NOT* be consistent across machines.
    For example, using the same seed on a machine that supports AVX
    instructions and one that only supports SSE instructions may produce
    different simulation results.
    
    CAUTION: simulation results *MAY NOT* be consistent if you vary
    other flags and modes. For example, `--skip_reference_sample` may
    result in fewer calls the to the random number generator before
    reported sampling begins. More generally, using the same seed for
    `stim sample` and `stim detect` will not result in detection events
    corresponding to the measurement results.
    
    
- <a name="--shots"></a>**`--shots`**
    Specifies the number of samples to take from the circuit.
    
    Defaults to 1.
    Must be an integer between 0 and a quintillion (10^18).
    
    
- <a name="--single"></a>**`--single`**
    Explain using a single simple error instead of all possible errors.
    
    When `--single` isn't specified, every single circuit error that
    produces a specific detector error model is output as a possible
    explanation of that error.
    
    When `--single` is specified, only the simplest circuit error is
    output. The "simplest" error is chosen by using heuristics such as
    "has fewer Pauli terms" and "happens earlier".
    
    
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
    
    
