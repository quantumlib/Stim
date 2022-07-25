"""Stim (Development Version): a fast quantum stabilizer circuit library."""
# (This a stubs file describing the classes and methods in stim.)
from typing import overload, TYPE_CHECKING, List, Dict, Tuple, Any, Union, Iterable
if TYPE_CHECKING:
    import io
    import pathlib
    import numpy as np
    import stim
class Circuit:
    """A mutable stabilizer circuit.

    Examples:
        >>> import stim
        >>> c = stim.Circuit()
        >>> c.append("X", 0)
        >>> c.append("M", 0)
        >>> c.compile_sampler().sample(shots=1)
        array([[ True]])

        >>> stim.Circuit('''
        ...    H 0
        ...    CNOT 0 1
        ...    M 0 1
        ...    DETECTOR rec[-1] rec[-2]
        ... ''').compile_detector_sampler().sample(shots=1)
        array([[False]])

    """
    def __add__(
        self,
        second: stim.Circuit,
    ) -> stim.Circuit:
        """Creates a circuit by appending two circuits.

        Examples:
            >>> import stim
            >>> c1 = stim.Circuit('''
            ...    X 0
            ...    Y 1 2
            ... ''')
            >>> c2 = stim.Circuit('''
            ...    M 0 1 2
            ... ''')
            >>> c1 + c2
            stim.Circuit('''
                X 0
                Y 1 2
                M 0 1 2
            ''')
        """
    def __eq__(
        self,
        arg0: stim.Circuit,
    ) -> bool:
        """Determines if two circuits have identical contents.
        """
    @overload
    def __getitem__(
        self,
        index_or_slice: int,
    ) -> Union[stim.CircuitInstruction, stim.CircuitRepeatBlock]:
        pass
    @overload
    def __getitem__(
        self,
        index_or_slice: slice,
    ) -> stim.Circuit:
        pass
    def __getitem__(
        self,
        index_or_slice: object,
    ) -> object:
        """Returns copies of instructions from the circuit.

        Args:
            index_or_slice: An integer index picking out an instruction to return, or a slice picking out a range
                of instructions to return as a circuit.

        Returns:
            If the index was an integer, then an instruction from the circuit.
            If the index was a slice, then a circuit made up of the instructions in that slice.

        Examples:
            >>> import stim
            >>> circuit = stim.Circuit('''
            ...    X 0
            ...    X_ERROR(0.5) 1 2
            ...    REPEAT 100 {
            ...        X 0
            ...        Y 1 2
            ...    }
            ...    TICK
            ...    M 0
            ...    DETECTOR rec[-1]
            ... ''')
            >>> circuit[1]
            stim.CircuitInstruction('X_ERROR', [stim.GateTarget(1), stim.GateTarget(2)], [0.5])
            >>> circuit[2]
            stim.CircuitRepeatBlock(100, stim.Circuit('''
                X 0
                Y 1 2
            '''))
            >>> circuit[1::2]
            stim.Circuit('''
                X_ERROR(0.5) 1 2
                TICK
                DETECTOR rec[-1]
            ''')
        """
    def __iadd__(
        self,
        second: stim.Circuit,
    ) -> stim.Circuit:
        """Appends a circuit into the receiving circuit (mutating it).

        Examples:
            >>> import stim
            >>> c1 = stim.Circuit('''
            ...    X 0
            ...    Y 1 2
            ... ''')
            >>> c2 = stim.Circuit('''
            ...    M 0 1 2
            ... ''')
            >>> c1 += c2
            >>> print(repr(c1))
            stim.Circuit('''
                X 0
                Y 1 2
                M 0 1 2
            ''')
        """
    def __imul__(
        self,
        repetitions: int,
    ) -> stim.Circuit:
        """Mutates the circuit by putting its contents into a REPEAT block.

        Special case: if the repetition count is 0, the circuit is cleared.
        Special case: if the repetition count is 1, nothing happens.

        Args:
            repetitions: The number of times the REPEAT block should repeat.

        Examples:
            >>> import stim
            >>> c = stim.Circuit('''
            ...    X 0
            ...    Y 1 2
            ... ''')
            >>> c *= 3
            >>> print(repr(c))
            stim.Circuit('''
                REPEAT 3 {
                    X 0
                    Y 1 2
                }
            ''')
        """
    def __init__(
        self,
        stim_program_text: str = '',
    ) -> None:
        """Creates a stim.Circuit.

        Args:
            stim_program_text: Defaults to empty. Describes operations to append into the circuit.

        Examples:
            >>> import stim
            >>> empty = stim.Circuit()
            >>> not_empty = stim.Circuit('''
            ...    X 0
            ...    CNOT 0 1
            ...    M 1
            ... ''')
        """
    def __len__(
        self,
    ) -> int:
        """Returns the number of top-level instructions and blocks in the circuit.

        Instructions inside of blocks are not included in this count.

        Examples:
            >>> import stim
            >>> len(stim.Circuit())
            0
            >>> len(stim.Circuit('''
            ...    X 0
            ...    X_ERROR(0.5) 1 2
            ...    TICK
            ...    M 0
            ...    DETECTOR rec[-1]
            ... '''))
            5
            >>> len(stim.Circuit('''
            ...    REPEAT 100 {
            ...        X 0
            ...        Y 1 2
            ...    }
            ... '''))
            1
        """
    def __mul__(
        self,
        repetitions: int,
    ) -> stim.Circuit:
        """Returns a circuit with a REPEAT block containing the current circuit's instructions.

        Special case: if the repetition count is 0, an empty circuit is returned.
        Special case: if the repetition count is 1, an equal circuit with no REPEAT block is returned.

        Args:
            repetitions: The number of times the REPEAT block should repeat.

        Examples:
            >>> import stim
            >>> c = stim.Circuit('''
            ...    X 0
            ...    Y 1 2
            ... ''')
            >>> c * 3
            stim.Circuit('''
                REPEAT 3 {
                    X 0
                    Y 1 2
                }
            ''')
        """
    def __ne__(
        self,
        arg0: stim.Circuit,
    ) -> bool:
        """Determines if two circuits have non-identical contents.
        """
    def __repr__(
        self,
    ) -> str:
        """Returns text that is a valid python expression evaluating to an equivalent `stim.Circuit`.
        """
    def __rmul__(
        self,
        repetitions: int,
    ) -> stim.Circuit:
        """Returns a circuit with a REPEAT block containing the current circuit's instructions.

        Special case: if the repetition count is 0, an empty circuit is returned.
        Special case: if the repetition count is 1, an equal circuit with no REPEAT block is returned.

        Args:
            repetitions: The number of times the REPEAT block should repeat.

        Examples:
            >>> import stim
            >>> c = stim.Circuit('''
            ...    X 0
            ...    Y 1 2
            ... ''')
            >>> 3 * c
            stim.Circuit('''
                REPEAT 3 {
                    X 0
                    Y 1 2
                }
            ''')
        """
    def __str__(
        self,
    ) -> str:
        """Returns stim instructions (that can be saved to a file and parsed by stim) for the current circuit.
        """
    @overload
    def append(
        self,
        name: str,
        targets: Union[int, stim.GateTarget, Iterable[Union[int, stim.GateTarget]]],
        arg: Union[float, Iterable[float]],
    ) -> None:
        pass
    @overload
    def append(
        self,
        name: Union[stim.CircuitOperation, stim.CircuitRepeatBlock],
    ) -> None:
        pass
    def append(
        self,
        name: object,
        targets: object = (),
        arg: object = None,
    ) -> None:
        """Appends an operation into the circuit.

        Note: `stim.Circuit.append_operation` is an alias of `stim.Circuit.append`.

        Examples:
            >>> import stim
            >>> c = stim.Circuit()
            >>> c.append("X", 0)
            >>> c.append("H", [0, 1])
            >>> c.append("M", [0, stim.target_inv(1)])
            >>> c.append("CNOT", [stim.target_rec(-1), 0])
            >>> c.append("X_ERROR", [0], 0.125)
            >>> c.append("CORRELATED_ERROR", [stim.target_x(0), stim.target_y(2)], 0.25)
            >>> print(repr(c))
            stim.Circuit('''
                X 0
                H 0 1
                M 0 !1
                CX rec[-1] 0
                X_ERROR(0.125) 0
                E(0.25) X0 Y2
            ''')

        Args:
            name: The name of the operation's gate (e.g. "H" or "M" or "CNOT").

                This argument can also be set to a `stim.CircuitInstruction` or `stim.CircuitInstructionBlock`, which
                results in the instruction or block being appended to the circuit. The other arguments (targets and
                arg) can't be specified when doing so.

                (The argument name `name` is no longer quite right, but being kept for backwards compatibility.)
            targets: The objects operated on by the gate. This can be either a single target or an iterable of
                multiple targets to broadcast the gate over. Each target can be an integer (a qubit), a
                stim.GateTarget, or a special target from one of the `stim.target_*` methods (such as a
                measurement record target like `rec[-1]` from `stim.target_rec(-1)`).
            arg: The "parens arguments" for the gate, such as the probability for a noise operation. A double or
                list of doubles parameterizing the gate. Different gates take different parens arguments. For
                example, X_ERROR takes a probability, OBSERVABLE_INCLUDE takes an observable index, and
                PAULI_CHANNEL_1 takes three disjoint probabilities.

                Note: Defaults to no parens arguments. Except, for backwards compatibility reasons,
                `cirq.append_operation` (but not `cirq.append`) will default to a single 0.0 argument for gates
                that take exactly one argument.
        """
    def append_from_stim_program_text(
        self,
        stim_program_text: str,
    ) -> None:
        """Appends operations described by a STIM format program into the circuit.

        Examples:
            >>> import stim
            >>> c = stim.Circuit()
            >>> c.append_from_stim_program_text('''
            ...    H 0  # comment
            ...    CNOT 0 2
            ...
            ...    M 2
            ...    CNOT rec[-1] 1
            ... ''')
            >>> print(c)
            H 0
            CX 0 2
            M 2
            CX rec[-1] 1

        Args:
            stim_program_text: The STIM program text containing the circuit operations to append.
        """
    def append_operation(
        self,
        name: object,
        targets: object = (),
        arg: object = None,
    ) -> None:
        """[DEPRECATED] use stim.Circuit.append instead
        """
    def approx_equals(
        self,
        other: object,
        *,
        atol: float,
    ) -> bool:
        """Checks if a circuit is approximately equal to another circuit.

        Two circuits are approximately equal if they are equal up to slight perturbations of instruction arguments
        such as probabilities. For example `X_ERROR(0.100) 0` is approximately equal to `X_ERROR(0.099)` within an
        absolute tolerance of 0.002. All other details of the circuits (such as the ordering of instructions and
        targets) must be exactly the same.

        Args:
            other: The circuit, or other object, to compare to this one.
            atol: The absolute error tolerance. The maximum amount each probability may have been perturbed by.

        Returns:
            True if the given object is a circuit approximately equal up to the receiving circuit up to the given
            tolerance, otherwise False.

        Examples:
            >>> import stim
            >>> base = stim.Circuit('''
            ...    X_ERROR(0.099) 0 1 2
            ...    M 0 1 2
            ... ''')

            >>> base.approx_equals(base, atol=0)
            True

            >>> base.approx_equals(stim.Circuit('''
            ...    X_ERROR(0.101) 0 1 2
            ...    M 0 1 2
            ... '''), atol=0)
            False

            >>> base.approx_equals(stim.Circuit('''
            ...    X_ERROR(0.101) 0 1 2
            ...    M 0 1 2
            ... '''), atol=0.0001)
            False

            >>> base.approx_equals(stim.Circuit('''
            ...    X_ERROR(0.101) 0 1 2
            ...    M 0 1 2
            ... '''), atol=0.01)
            True

            >>> base.approx_equals(stim.Circuit('''
            ...    DEPOLARIZE1(0.099) 0 1 2
            ...    MRX 0 1 2
            ... '''), atol=9999)
            False
        """
    def clear(
        self,
    ) -> None:
        """Clears the contents of the circuit.

        Examples:
            >>> import stim
            >>> c = stim.Circuit('''
            ...    X 0
            ...    Y 1 2
            ... ''')
            >>> c.clear()
            >>> c
            stim.Circuit()
        """
    def compile_detector_sampler(
        self,
        *,
        seed: object = None,
    ) -> stim.CompiledDetectorSampler:
        """Returns a CompiledDetectorSampler, which can quickly batch sample detection events, for the circuit.

        Args:
            seed: PARTIALLY determines simulation results by deterministically seeding the random number generator.
                Must be None or an integer in range(2**64).

                Defaults to None. When set to None, a prng seeded by system entropy is used.

                When set to an integer, making the exact same series calls on the exact same machine with the exact
                same version of Stim will produce the exact same simulation results.

                CAUTION: simulation results *WILL NOT* be consistent between versions of Stim. This restriction is
                present to make it possible to have future optimizations to the random sampling, and is enforced by
                introducing intentional differences in the seeding strategy from version to version.

                CAUTION: simulation results *MAY NOT* be consistent across machines that differ in the width of
                supported SIMD instructions. For example, using the same seed on a machine that supports AVX
                instructions and one that only supports SSE instructions may produce different simulation results.

                CAUTION: simulation results *MAY NOT* be consistent if you vary how many shots are taken. For
                example, taking 10 shots and then 90 shots will give different results from taking 100 shots in one
                call.

        Examples:
            >>> import stim
            >>> c = stim.Circuit('''
            ...    H 0
            ...    CNOT 0 1
            ...    M 0 1
            ...    DETECTOR rec[-1] rec[-2]
            ... ''')
            >>> s = c.compile_detector_sampler()
            >>> s.sample(shots=1)
            array([[False]])
        """
    def compile_m2d_converter(
        self,
        *,
        skip_reference_sample: bool = False,
    ) -> stim.CompiledMeasurementsToDetectionEventsConverter:
        """Returns an object that can efficiently convert measurements into detection events for the given circuit.

        The converter uses a noiseless reference sample, collected from the circuit using stim's Tableau simulator
        during initialization of the converter, as a baseline for determining what the expected value of a detector
        is.

        Note that the expected behavior of gauge detectors (detectors that are not actually deterministic under
        noiseless execution) can vary depending on the reference sample. Stim mitigates this by always generating
        the same reference sample for a given circuit.

        Args:
            skip_reference_sample: Defaults to False. When set to True, the reference sample used by the converter
                is initialized to all-zeroes instead of being collected from the circuit. This should only be used
                if it's known that the all-zeroes sample is actually a possible result from the circuit (under
                noiseless execution).

        Returns:
            An initialized stim.CompiledMeasurementsToDetectionEventsConverter.

        Examples:
            >>> import stim
            >>> import numpy as np
            >>> converter = stim.Circuit('''
            ...    X 0
            ...    M 0
            ...    DETECTOR rec[-1]
            ... ''').compile_m2d_converter()
            >>> converter.convert(
            ...     measurements=np.array([[0], [1]], dtype=np.bool8),
            ...     append_observables=False,
            ... )
            array([[ True],
                   [False]])
        """
    def compile_sampler(
        self,
        *,
        skip_reference_sample: bool = False,
        seed: object = None,
    ) -> stim.CompiledMeasurementSampler:
        """Returns a CompiledMeasurementSampler, which can quickly batch sample measurements, for the circuit.

        Args:
            skip_reference_sample: Defaults to False. When set to True, the reference sample used by the sampler is
                initialized to all-zeroes instead of being collected from the circuit. This means that the results
                returned by the sampler are actually whether or not each measurement was *flipped*, instead of true
                measurement results.

                Forcing an all-zero reference sample is useful when you are only interested in error propagation and
                don't want to have to deal with the fact that some measurements want to be On when no errors occur.
                It is also useful when you know for sure that the all-zero result is actually a possible result from
                the circuit (under noiseless execution), meaning it is a valid reference sample as good as any
                other. Computing the reference sample is the most time consuming and memory intensive part of
                simulating the circuit, so promising that the simulator can safely skip that step is an effective
                optimization.
            seed: PARTIALLY determines simulation results by deterministically seeding the random number generator.
                Must be None or an integer in range(2**64).

                Defaults to None. When set to None, a prng seeded by system entropy is used.

                When set to an integer, making the exact same series calls on the exact same machine with the exact
                same version of Stim will produce the exact same simulation results.

                CAUTION: simulation results *WILL NOT* be consistent between versions of Stim. This restriction is
                present to make it possible to have future optimizations to the random sampling, and is enforced by
                introducing intentional differences in the seeding strategy from version to version.

                CAUTION: simulation results *MAY NOT* be consistent across machines that differ in the width of
                supported SIMD instructions. For example, using the same seed on a machine that supports AVX
                instructions and one that only supports SSE instructions may produce different simulation results.

                CAUTION: simulation results *MAY NOT* be consistent if you vary how many shots are taken. For
                example, taking 10 shots and then 90 shots will give different results from taking 100 shots in one
                call.

        Examples:
            >>> import stim
            >>> c = stim.Circuit('''
            ...    X 2
            ...    M 0 1 2
            ... ''')
            >>> s = c.compile_sampler()
            >>> s.sample(shots=1)
            array([[False, False,  True]])
        """
    def copy(
        self,
    ) -> stim.Circuit:
        """Returns a copy of the circuit. An independent circuit with the same contents.

        Examples:
            >>> import stim

            >>> c1 = stim.Circuit("H 0")
            >>> c2 = c1.copy()
            >>> c2 is c1
            False
            >>> c2 == c1
            True
        """
    def detector_error_model(
        self,
        *,
        decompose_errors: bool = False,
        flatten_loops: bool = False,
        allow_gauge_detectors: bool = False,
        approximate_disjoint_errors: float = False,
        ignore_decomposition_failures: bool = False,
        block_decomposition_from_introducing_remnant_edges: bool = False,
    ) -> stim.DetectorErrorModel:
        """Returns a stim.DetectorErrorModel describing the error processes in the circuit.

        Args:
            decompose_errors: Defaults to false. When set to true, the error analysis attempts to decompose the
                components of composite error mechanisms (such as depolarization errors) into simpler errors, and
                suggest this decomposition via `stim.target_separator()` between the components. For example, in an
                XZ surface code, single qubit depolarization has a Y error term which can be decomposed into simpler
                X and Z error terms. Decomposition fails (causing this method to throw) if it's not possible to
                decompose large errors into simple errors that affect at most two detectors.
            flatten_loops: Defaults to false. When set to true, the output will not contain any `repeat` blocks.
                When set to false, the error analysis watches for loops in the circuit reaching a periodic steady
                state with respect to the detectors being introduced, the error mechanisms that affect them, and the
                locations of the logical observables. When it identifies such a steady state, it outputs a repeat
                block. This is massively more efficient than flattening for circuits that contain loops, but creates
                a more complex output.
            allow_gauge_detectors: Defaults to false. When set to false, the error analysis verifies that detectors
                in the circuit are actually deterministic under noiseless execution of the circuit. When set to
                true, these detectors are instead considered to be part of degrees freedom that can be removed from
                the error model. For example, if detectors D1 and D3 both anti-commute with a reset, then the error
                model has a gauge `error(0.5) D1 D3`. When gauges are identified, one of the involved detectors is
                removed from the system using Gaussian elimination.

                Note that logical observables are still verified to be deterministic, even if this option is set.
            approximate_disjoint_errors: Defaults to false. When set to false, composite error mechanisms with
                disjoint components (such as `PAULI_CHANNEL_1(0.1, 0.2, 0.0)`) can cause the error analysis to throw
                exceptions (because detector error models can only contain independent error mechanisms). When set
                to true, the probabilities of the disjoint cases are instead assumed to be independent
                probabilities. For example, a ``PAULI_CHANNEL_1(0.1, 0.2, 0.0)` becomes equivalent to an
                `X_ERROR(0.1)` followed by a `Z_ERROR(0.2)`. This assumption is an approximation, but it is a good
                approximation for small probabilities.

                This argument can also be set to a probability between 0 and 1, setting a threshold below which the
                approximation is acceptable. Any error mechanisms that have a component probability above the
                threshold will cause an exception to be thrown.
            ignore_decomposition_failures: Defaults to False.
                When this is set to True, circuit errors that fail to decompose into graphlike
                detector error model errors no longer cause the conversion process to abort.
                Instead, the undecomposed error is inserted into the output. Whatever tool
                the detector error model is then given to is responsible for dealing with the
                undecomposed errors (e.g. a tool may choose to simply ignore them).

                Irrelevant unless decompose_errors=True.
            block_decomposition_from_introducing_remnant_edges: Defaults to False.
                Requires that both A B and C D be present elsewhere in the detector error model
                in order to decompose A B C D into A B ^ C D. Normally, only one of A B or C D
                needs to appear to allow this decomposition.

                Remnant edges can be a useful feature for ensuring decomposition succeeds, but
                they can also reduce the effective code distance by giving the decoder single
                edges that actually represent multiple errors in the circuit (resulting in the
                decoder making misinformed choices when decoding).

                Irrelevant unless decompose_errors=True.

        Examples:
            >>> import stim

            >>> stim.Circuit('''
            ...     X_ERROR(0.125) 0
            ...     X_ERROR(0.25) 1
            ...     CORRELATED_ERROR(0.375) X0 X1
            ...     M 0 1
            ...     DETECTOR rec[-2]
            ...     DETECTOR rec[-1]
            ... ''').detector_error_model()
            stim.DetectorErrorModel('''
                error(0.125) D0
                error(0.375) D0 D1
                error(0.25) D1
            ''')
        """
    def explain_detector_error_model_errors(
        self,
        *,
        dem_filter: object = None,
        reduce_to_one_representative_error: bool = False,
    ) -> List[stim.ExplainedError]:
        """Explains how detector error model errors are produced by circuit errors.

        Args:
            dem_filter: Defaults to None (unused). When used, the output will only contain detector error
                model errors that appear in the given `stim.DetectorErrorModel`. Any error mechanisms from the
                detector error model that can't be reproduced using one error from the circuit will also be included
                in the result, but with an empty list of associated circuit error mechanisms.
            reduce_to_one_representative_error: Defaults to False. When True, the items in the result will contain
                at most one circuit error mechanism.

        Returns:
            A `List[stim.ExplainedError]` (see `stim.ExplainedError` for more information). Each item in the list
            describes how a detector error model error can be produced by individual circuit errors.

        Examples:
            >>> import stim
            >>> circuit = stim.Circuit('''
            ...     # Create Bell pair.
            ...     H 0
            ...     CNOT 0 1
            ...
            ...     # Noise.
            ...     DEPOLARIZE1(0.01) 0
            ...
            ...     # Bell basis measurement.
            ...     CNOT 0 1
            ...     H 0
            ...     M 0 1
            ...
            ...     # Both measurements should be False under noiseless execution.
            ...     DETECTOR rec[-1]
            ...     DETECTOR rec[-2]
            ... ''')
            >>> explained_errors = circuit.explain_detector_error_model_errors(
            ...     dem_filter=stim.DetectorErrorModel('error(1) D0 D1'),
            ...     reduce_to_one_representative_error=True,
            ... )
            >>> print(explained_errors[0].circuit_error_locations[0])
            CircuitErrorLocation {
                flipped_pauli_product: Y0
                Circuit location stack trace:
                    (after 0 TICKs)
                    at instruction #3 (DEPOLARIZE1) in the circuit
                    at target #1 of the instruction
                    resolving to DEPOLARIZE1(0.01) 0
            }
        """
    def flattened(
        self,
    ) -> stim.Circuit:
        """Creates an equivalent circuit without REPEAT or SHIFT_COORDS.

        Returns:
            A `stim.Circuit` with the same instructions in the same order,
            but with loops flattened into repeated instructions and with
            all coordinate shifts inlined.

        Examples:
            >>> import stim
            >>> stim.Circuit('''
            ...     REPEAT 5 {
            ...         MR 0 1
            ...         DETECTOR(0, 0) rec[-2]
            ...         DETECTOR(1, 0) rec[-1]
            ...         SHIFT_COORDS(0, 1)
            ...     }
            ... ''').flattened()
            stim.Circuit('''
                MR 0 1
                DETECTOR(0, 0) rec[-2]
                DETECTOR(1, 0) rec[-1]
                MR 0 1
                DETECTOR(0, 1) rec[-2]
                DETECTOR(1, 1) rec[-1]
                MR 0 1
                DETECTOR(0, 2) rec[-2]
                DETECTOR(1, 2) rec[-1]
                MR 0 1
                DETECTOR(0, 3) rec[-2]
                DETECTOR(1, 3) rec[-1]
                MR 0 1
                DETECTOR(0, 4) rec[-2]
                DETECTOR(1, 4) rec[-1]
            ''')
        """
    def flattened_operations(
        self,
    ) -> list:
        """[DEPRECATED]

        Returns a list of tuples encoding the contents of the circuit.
        Instead of this method, use `for instruction in circuit` or, to
        avoid REPEAT blocks, `for instruction in circuit.flattened()`.

        Examples:
            >>> import stim
            >>> stim.Circuit('''
            ...    H 0
            ...    X_ERROR(0.125) 1
            ...    M 0 !1
            ... ''').flattened_operations()
            [('H', [0], 0), ('X_ERROR', [1], 0.125), ('M', [0, ('inv', 1)], 0)]

            >>> stim.Circuit('''
            ...    REPEAT 2 {
            ...        H 6
            ...    }
            ... ''').flattened_operations()
            [('H', [6], 0), ('H', [6], 0)]
        """
    @staticmethod
    def from_file(
        file: Union[io.TextIOBase, str, pathlib.Path],
    ) -> stim.Circuit:

        """Reads a stim circuit from a file.

        The file format is defined at https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md

        Args:
            file: A file path or open file object to read from.

        Returns:
            The circuit parsed from the file.

        Examples:
            >>> import stim
            >>> import tempfile

            >>> with tempfile.TemporaryDirectory() as tmpdir:
            ...     path = tmpdir + '/tmp.stim'
            ...     with open(path, 'w') as f:
            ...         print('H 5', file=f)
            ...     circuit = stim.Circuit.from_file(path)
            >>> circuit
            stim.Circuit('''
                H 5
            ''')

            >>> with tempfile.TemporaryDirectory() as tmpdir:
            ...     path = tmpdir + '/tmp.stim'
            ...     with open(path, 'w') as f:
            ...         print('CNOT 4 5', file=f)
            ...     with open(path) as f:
            ...         circuit = stim.Circuit.from_file(path)
            >>> circuit
            stim.Circuit('''
                CX 4 5
            ''')
        """
    @staticmethod
    def generated(
        code_task: str,
        *,
        distance: int,
        rounds: int,
        after_clifford_depolarization: float = 0.0,
        before_round_data_depolarization: float = 0.0,
        before_measure_flip_probability: float = 0.0,
        after_reset_flip_probability: float = 0.0,
    ) -> stim.Circuit:
        """Generates common circuits.

        The generated circuits can include configurable noise.

        The generated circuits include DETECTOR and OBSERVABLE_INCLUDE annotations so that their detection events
        and logical observables can be sampled.

        The generated circuits include TICK annotations to mark the progression of time. (E.g. so that converting
        them using `stimcirq.stim_circuit_to_cirq_circuit` will produce a `cirq.Circuit` with the intended moment
        structure.)

        Args:
            code_task: A string identifying the type of circuit to generate. Available types are:
                - `repetition_code:memory`
                - `surface_code:rotated_memory_x`
                - `surface_code:rotated_memory_z`
                - `surface_code:unrotated_memory_x`
                - `surface_code:unrotated_memory_z`
                - `color_code:memory_xyz`
            distance: The desired code distance of the generated circuit. The code distance is the minimum number
                of physical errors needed to cause a logical error. This parameter indirectly determines how many
                qubits the generated circuit uses.
            rounds: How many times the measurement qubits in the generated circuit will be measured. Indirectly
                determines the duration of the generated circuit.
            after_clifford_depolarization: Defaults to 0. The probability (p) of `DEPOLARIZE1(p)` operations to add
                after every single-qubit Clifford operation and `DEPOLARIZE2(p)` operations to add after every
                two-qubit Clifford operation. The after-Clifford depolarizing operations are only included if this
                probability is not 0.
            before_round_data_depolarization: Defaults to 0. The probability (p) of `DEPOLARIZE1(p)` operations to
                apply to every data qubit at the start of a round of stabilizer measurements. The start-of-round
                depolarizing operations are only included if this probability is not 0.
            before_measure_flip_probability: Defaults to 0. The probability (p) of `X_ERROR(p)` operations applied
                to qubits before each measurement (X basis measurements use `Z_ERROR(p)` instead). The
                before-measurement flips are only included if this probability is not 0.
            after_reset_flip_probability: Defaults to 0. The probability (p) of `X_ERROR(p)` operations applied
                to qubits after each reset (X basis resets use `Z_ERROR(p)` instead). The after-reset flips are only
                included if this probability is not 0.

        Returns:
            The generated circuit.

        Examples:
            >>> import stim
            >>> circuit = stim.Circuit.generated(
            ...     "repetition_code:memory",
            ...     distance=4,
            ...     rounds=10000,
            ...     after_clifford_depolarization=0.0125)
            >>> print(circuit)
            R 0 1 2 3 4 5 6
            TICK
            CX 0 1 2 3 4 5
            DEPOLARIZE2(0.0125) 0 1 2 3 4 5
            TICK
            CX 2 1 4 3 6 5
            DEPOLARIZE2(0.0125) 2 1 4 3 6 5
            TICK
            MR 1 3 5
            DETECTOR(1, 0) rec[-3]
            DETECTOR(3, 0) rec[-2]
            DETECTOR(5, 0) rec[-1]
            REPEAT 9999 {
                TICK
                CX 0 1 2 3 4 5
                DEPOLARIZE2(0.0125) 0 1 2 3 4 5
                TICK
                CX 2 1 4 3 6 5
                DEPOLARIZE2(0.0125) 2 1 4 3 6 5
                TICK
                MR 1 3 5
                SHIFT_COORDS(0, 1)
                DETECTOR(1, 0) rec[-3] rec[-6]
                DETECTOR(3, 0) rec[-2] rec[-5]
                DETECTOR(5, 0) rec[-1] rec[-4]
            }
            M 0 2 4 6
            DETECTOR(1, 1) rec[-3] rec[-4] rec[-7]
            DETECTOR(3, 1) rec[-2] rec[-3] rec[-6]
            DETECTOR(5, 1) rec[-1] rec[-2] rec[-5]
            OBSERVABLE_INCLUDE(0) rec[-1]
        """
    def get_detector_coordinates(
        self,
        only: object = None,
    ) -> Dict[int, List[float]]:
        """Returns the coordinate metadata of detectors in the circuit.

        Args:
            only: Defaults to None (meaning include all detectors). A list of detector indices to include in the
                result. Detector indices beyond the end of the detector error model of the circuit cause an error.

        Returns:
            A dictionary mapping integers (detector indices) to lists of floats (coordinates).
            A dictionary mapping detector indices to lists of floats.
            Detectors with no specified coordinate data are mapped to an empty tuple.
            If `only` is specified, then `set(result.keys()) == set(only)`.

        Examples:
            >>> import stim
            >>> circuit = stim.Circuit('''
            ...    M 0
            ...    DETECTOR rec[-1]
            ...    DETECTOR(1, 2, 3) rec[-1]
            ...    REPEAT 3 {
            ...        DETECTOR(42) rec[-1]
            ...        SHIFT_COORDS(100)
            ...    }
            ... ''')
            >>> circuit.get_detector_coordinates()
            {0: [], 1: [1.0, 2.0, 3.0], 2: [42.0], 3: [142.0], 4: [242.0]}
            >>> circuit.get_detector_coordinates(only=[1])
            {1: [1.0, 2.0, 3.0]}
        """
    def get_final_qubit_coordinates(
        self,
    ) -> Dict[int, List[float]]:
        """Returns the coordinate metadata of qubits in the circuit.

        If a qubit's coordinates are specified multiple times, only the last specified coordinates are returned.

        Returns:
            A dictionary mapping qubit indices (integers) to coordinates (lists of floats).
            Qubits that never had their coordinates specified are not included in the result.

        Examples:
            >>> import stim
            >>> circuit = stim.Circuit('''
            ...    QUBIT_COORDS(1, 2, 3) 1
            ... ''')
            >>> circuit.get_final_qubit_coordinates()
            {1: [1.0, 2.0, 3.0]}
        """
    @property
    def num_detectors(
        self,
    ) -> int:
        """Counts the number of bits produced when sampling the circuit's detectors.

        Examples:
            >>> import stim
            >>> c = stim.Circuit('''
            ...    M 0
            ...    DETECTOR rec[-1]
            ...    REPEAT 100 {
            ...        M 0 1 2
            ...        DETECTOR rec[-1]
            ...        DETECTOR rec[-2]
            ...    }
            ... ''')
            >>> c.num_detectors
            201
        """
    @property
    def num_measurements(
        self,
    ) -> int:
        """Counts the number of bits produced when sampling the circuit's measurements.

        Examples:
            >>> import stim
            >>> c = stim.Circuit('''
            ...    M 0
            ...    REPEAT 100 {
            ...        M 0 1
            ...    }
            ... ''')
            >>> c.num_measurements
            201
        """
    @property
    def num_observables(
        self,
    ) -> int:
        """Counts the number of bits produced when sampling the circuit's logical observables.

        This is one more than the largest observable index given to OBSERVABLE_INCLUDE.

        Examples:
            >>> import stim
            >>> c = stim.Circuit('''
            ...    M 0
            ...    OBSERVABLE_INCLUDE(2) rec[-1]
            ...    OBSERVABLE_INCLUDE(5) rec[-1]
            ... ''')
            >>> c.num_observables
            6
        """
    @property
    def num_qubits(
        self,
    ) -> int:
        """Counts the number of qubits used when simulating the circuit.

        This is always one more than the largest qubit index used by the circuit.

        Examples:
            >>> import stim
            >>> stim.Circuit('''
            ...    X 0
            ...    M 0 1
            ... ''').num_qubits
            2
            >>> stim.Circuit('''
            ...    X 0
            ...    M 0 1
            ...    H 100
            ... ''').num_qubits
            101
        """
    @property
    def num_sweep_bits(
        self,
    ) -> int:
        """Returns the number of sweep bits needed to completely configure the circuit.

        This is always one more than the largest sweep bit index used by the circuit.

        Examples:
            >>> import stim
            >>> stim.Circuit('''
            ...    CX sweep[2] 0
            ... ''').num_sweep_bits
            3
            >>> stim.Circuit('''
            ...    CZ sweep[5] 0
            ...    CX sweep[2] 0
            ... ''').num_sweep_bits
            6
        """
    def search_for_undetectable_logical_errors(
        self,
        *,
        dont_explore_detection_event_sets_with_size_above: int,
        dont_explore_edges_with_degree_above: int,
        dont_explore_edges_increasing_symptom_degree: bool,
        canonicalize_circuit_errors: bool = False,
    ) -> List[stim.ExplainedError]:
        """Searches for lists of errors from the model that form an undetectable logical error.

        THIS IS A HEURISTIC METHOD. It does not guarantee that it will find errors of particular
        sizes, or with particular properties. The errors it finds are a tangled combination of the
        truncation parameters you specify, internal optimizations which are correct when not
        truncating, and minutia of the circuit being considered.

        If you want a well behaved method that does provide guarantees of finding errors of a
        particular type, use `stim.Circuit.shortest_graphlike_error`. This method is more
        thorough than that (assuming you don't truncate so hard you omit graphlike edges),
        but exactly how thorough is difficult to describe. It's also not guaranteed that the
        behavior of this method will not be changed in the future in a way that permutes which
        logical errors are found and which are missed.

        This search method considers hyper errors, so it has worst case exponential runtime. It is
        important to carefully consider the arguments you are providing, which truncate the search
        space and trade cost for quality.

        The search progresses by starting from each error that crosses a logical observable, noting
        which detection events each error produces, and then iteratively adding in errors touching
        those detection events attempting to cancel out the detection event with the lowest index.

        Beware that the choice of logical observable can interact with the truncation options. Using
        different observables can change whether or not the search succeeds, even if those observables
        are equal modulo the stabilizers of the code. This is because the edges crossing logical
        observables are used as starting points for the search, and starting from different places along
        a path will result in different numbers of symptoms in intermediate states as the search
        progresses. For example, if the logical observable is next to a boundary, then the starting
        edges are likely boundary edges (degree 1) with 'room to grow', whereas if the observable was
        running through the bulk then the starting edges will have degree at least 2.

        Args:
            dont_explore_detection_event_sets_with_size_above: Truncates the search space by refusing to
                cross an edge (i.e. add an error) when doing so would produce an intermediate state that
                has more detection events than this limit.
            dont_explore_edges_with_degree_above: Truncates the search space by refusing to consider
                errors that cause a lot of detection events. For example, you may only want to consider
                graphlike errors which have two or fewer detection events.
            dont_explore_edges_increasing_symptom_degree: Truncates the search space by refusing to
                cross an edge (i.e. add an error) when doing so would produce an intermediate state that
                has more detection events that the previous intermediate state. This massively improves
                the efficiency of the search because instead of, for example, exploring all n^4 possible
                detection event sets with 4 symptoms, the search will attempt to cancel out symptoms one
                by one.
            canonicalize_circuit_errors: Whether or not to use one representative for equal-symptom circuit errors.
                False (default): Each DEM error lists every possible circuit error that single handedly produces
                    those symptoms as a potential match. This is verbose but gives complete information.
                True: Each DEM error is matched with one possible circuit error that single handedly produces those
                    symptoms, with a preference towards errors that are simpler (e.g. apply Paulis to fewer qubits).
                    This discards mostly-redundant information about different ways to produce the same symptoms in
                    order to give a succinct result.

        Returns:
            A detector error model containing only the error mechanisms that cause the undetectable
            logical error. The error mechanisms will have their probabilities set to 1 (indicating that
            they are necessary) and will not suggest a decomposition.

        Examples:
            >>> import stim
            >>> circuit = stim.Circuit.generated(
            ...     "surface_code:rotated_memory_x",
            ...     rounds=5,
            ...     distance=5,
            ...     after_clifford_depolarization=0.001)
            >>> print(len(circuit.search_for_undetectable_logical_errors(
            ...     dont_explore_detection_event_sets_with_size_above=4,
            ...     dont_explore_edges_with_degree_above=4,
            ...     dont_explore_edges_increasing_symptom_degree=True,
            ... )))
            5
        """
    def shortest_graphlike_error(
        self,
        *,
        ignore_ungraphlike_errors: bool = True,
        canonicalize_circuit_errors: bool = False,
    ) -> List[stim.ExplainedError]:
        """Finds a minimum sized set of graphlike errors that produce an undetected logical error.

        A "graphlike error" is an error that creates at most two detection events (causes a change in the parity of
        the measurement sets of at most two DETECTOR annotations).

        Note that this method does not pay attention to error probabilities (other than ignoring errors with
        probability 0). It searches for a logical error with the minimum *number* of physical errors, not the
        maximum probability of those physical errors all occurring.

        This method works by converting the circuit into a `stim.DetectorErrorModel` using
        `circuit.detector_error_model(...)`, computing the shortest graphlike error of the error model, and then
        converting the physical errors making up that logical error back into representative circuit errors.

        Args:
            ignore_ungraphlike_errors:
                False: Attempt to decompose any ungraphlike errors in the circuit into graphlike parts.
                    If this fails, raise an exception instead of continuing.
                    Note: in some cases, graphlike errors only appear as parts of decomposed ungraphlike errors.
                    This can produce a result that lists DEM errors with zero matching circuit errors, because the
                    only way to achieve those errors is by combining a decomposed error with a graphlike error.
                    As a result, when using this option it is NOT guaranteed that the length of the result is an
                    upper bound on the true code distance. That is only the case if every item in the result lists
                    at least one matching circuit error.
                True (default): Ungraphlike errors are simply skipped as if they weren't present, even if they could
                    become graphlike if decomposed. This guarantees the length of the result is an upper bound on
                    the true code distance.
            canonicalize_circuit_errors: Whether or not to use one representative for equal-symptom circuit errors.
                False (default): Each DEM error lists every possible circuit error that single handedly produces
                    those symptoms as a potential match. This is verbose but gives complete information.
                True: Each DEM error is matched with one possible circuit error that single handedly produces those
                    symptoms, with a preference towards errors that are simpler (e.g. apply Paulis to fewer qubits).
                    This discards mostly-redundant information about different ways to produce the same symptoms in
                    order to give a succinct result.

        Returns:
            A detector error model containing only the error mechanisms that cause the undetectable
            logical error. The error mechanisms will have their probabilities set to 1 (indicating that
            they are necessary) and will not suggest a decomposition.

        Examples:
            >>> import stim

            >>> circuit = stim.Circuit.generated(
            ...     "repetition_code:memory",
            ...     rounds=10,
            ...     distance=7,
            ...     before_round_data_depolarization=0.01)
            >>> len(circuit.shortest_graphlike_error())
            7
        """
    def to_file(
        self,
        file: Union[io.TextIOBase, str, pathlib.Path],
    ) -> None:

        """Writes the stim circuit to a file.

        The file format is defined at https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md

        Args:
            file: A file path or an open file to write to.

        Examples:
            >>> import stim
            >>> import tempfile
            >>> c = stim.Circuit('H 5\nX 0')

            >>> with tempfile.TemporaryDirectory() as tmpdir:
            ...     path = tmpdir + '/tmp.stim'
            ...     with open(path, 'w') as f:
            ...         c.to_file(f)
            ...     with open(path) as f:
            ...         contents = f.read()
            >>> contents
            'H 5\nX 0\n'

            >>> with tempfile.TemporaryDirectory() as tmpdir:
            ...     path = tmpdir + '/tmp.stim'
            ...     c.to_file(path)
            ...     with open(path) as f:
            ...         contents = f.read()
            >>> contents
            'H 5\nX 0\n'
        """
    def without_noise(
        self,
    ) -> stim.Circuit:
        """Returns a copy of the circuit with all noise processes removed.

        Pure noise instructions, such as X_ERROR and DEPOLARIZE2, are not
        included in the result.

        Noisy measurement instructions, like `M(0.001)`, have their noise
        parameter removed.

        Returns:
            A `stim.Circuit` with the same instructions except all noise
            processes have been removed.

        Examples:
            >>> import stim
            >>> stim.Circuit('''
            ...     X_ERROR(0.25) 0
            ...     CNOT 0 1
            ...     M(0.125) 0
            ... ''').without_noise()
            stim.Circuit('''
                CX 0 1
                M 0
            ''')
        """
class CircuitErrorLocation:
    """Describes the location of an error mechanism from a stim circuit.
    """
    def __init__(
        self,
        *,
        tick_offset: int,
        flipped_pauli_product: List[stim.GateTargetWithCoords],
        flipped_measurement: object,
        instruction_targets: stim.CircuitTargetsInsideInstruction,
        stack_frames: List[stim.CircuitErrorLocationStackFrame],
    ) -> None:
        """Creates a stim.CircuitErrorLocation.
        """
    @property
    def flipped_measurement(
        self,
    ) -> Optional[stim.FlippedMeasurement]:

        """The measurement that was flipped by the error mechanism.
        If the error isn't a measurement error, this will be None.
        """
    @property
    def flipped_pauli_product(
        self,
    ) -> List[stim.GateTargetWithCoords]:
        """The Pauli errors that the error mechanism applied to qubits.
        When the error is a measurement error, this will be an empty list.
        """
    @property
    def instruction_targets(
        self,
    ) -> stim.CircuitTargetsInsideInstruction:
        """Within the error instruction, which may have hundreds of
        targets, which specific targets were being executed to
        produce the error.
        """
    @property
    def stack_frames(
        self,
    ) -> List[stim.CircuitErrorLocationStackFrame]:
        """Where in the circuit's execution does the error mechanism occur,
        accounting for things like nested loops that iterate multiple times.
        """
    @property
    def tick_offset(
        self,
    ) -> int:
        """The number of TICKs that executed before the error mechanism being discussed,
        including TICKs that occurred multiple times during loops.
        """
class CircuitErrorLocationStackFrame:
    """Describes the location of an instruction being executed within a
    circuit or loop, distinguishing between separate loop iterations.

    The full location of an instruction is a list of these frames,
    drilling down from the top level circuit to the inner-most loop
    that the instruction is within.
    """
    def __init__(
        self,
        *,
        instruction_offset: int,
        iteration_index: int,
        instruction_repetitions_arg: int,
    ) -> None:
        """Creates a stim.CircuitErrorLocationStackFrame.
        """
    @property
    def instruction_offset(
        self,
    ) -> int:
        """The index of the instruction within the circuit, or within the
        instruction's parent REPEAT block. This is slightly different
        from the line number, because blank lines and commented lines
        don't count and also because the offset of the first instruction
        is 0 instead of 1.
        """
    @property
    def instruction_repetitions_arg(
        self,
    ) -> int:
        """If the instruction being referred to is a REPEAT block,
        this is the repetition count of that REPEAT block. Otherwise
        this field defaults to 0.
        """
    @property
    def iteration_index(
        self,
    ) -> int:
        """Disambiguates which iteration of the loop containing this instruction
        is being referred to. If the instruction isn't in a REPEAT block, this
        field defaults to 0.
        """
class CircuitInstruction:
    """An instruction, like `H 0 1` or `CNOT rec[-1] 5`, from a circuit.

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit('''
        ...     H 0
        ...     M 0 !1
        ...     X_ERROR(0.125) 5 3
        ... ''')
        >>> circuit[0]
        stim.CircuitInstruction('H', [stim.GateTarget(0)], [])
        >>> circuit[1]
        stim.CircuitInstruction('M', [stim.GateTarget(0), stim.GateTarget(stim.target_inv(1))], [])
        >>> circuit[2]
        stim.CircuitInstruction('X_ERROR', [stim.GateTarget(5), stim.GateTarget(3)], [0.125])
    """
    def __eq__(
        self,
        arg0: stim.CircuitInstruction,
    ) -> bool:
        """Determines if two `stim.CircuitInstruction`s are identical.
        """
    def __init__(
        self,
        name: str,
        targets: List[object],
        gate_args: List[float] = (),
    ) -> None:
        """Initializes a `stim.CircuitInstruction`.

        Args:
            name: The name of the instruction being applied.
            targets: The targets the instruction is being applied to. These can be raw values like `0` and
                `stim.target_rec(-1)`, or instances of `stim.GateTarget`.
            gate_args: The sequence of numeric arguments parameterizing a gate. For noise gates this is their
                probabilities. For OBSERVABLE_INCLUDE it's the logical observable's index.
        """
    def __ne__(
        self,
        arg0: stim.CircuitInstruction,
    ) -> bool:
        """Determines if two `stim.CircuitInstruction`s are different.
        """
    def __repr__(
        self,
    ) -> str:
        """Returns text that is a valid python expression evaluating to an equivalent `stim.CircuitInstruction`.
        """
    def __str__(
        self,
    ) -> str:
        """Returns a text description of the instruction as a stim circuit file line.
        """
    def gate_args_copy(
        self,
    ) -> List[float]:
        """Returns the gate's arguments (numbers parameterizing the instruction).

        For noisy gates this typically a list of probabilities.
        For OBSERVABLE_INCLUDE it's a singleton list containing the logical observable index.
        """
    @property
    def name(
        self,
    ) -> str:
        """The name of the instruction (e.g. `H` or `X_ERROR` or `DETECTOR`).
        """
    def targets_copy(
        self,
    ) -> List[stim.GateTarget]:
        """Returns a copy of the targets of the instruction.
        """
class CircuitRepeatBlock:
    """A REPEAT block from a circuit.

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit('''
        ...     H 0
        ...     REPEAT 5 {
        ...         CX 0 1
        ...         CZ 1 2
        ...     }
        ... ''')
        >>> repeat_block = circuit[1]
        >>> repeat_block.repeat_count
        5
        >>> repeat_block.body_copy()
        stim.Circuit('''
            CX 0 1
            CZ 1 2
        ''')
    """
    def __eq__(
        self,
        arg0: stim.CircuitRepeatBlock,
    ) -> bool:
        """Determines if two `stim.CircuitRepeatBlock`s are identical.
        """
    def __init__(
        self,
        repeat_count: int,
        body: stim.Circuit,
    ) -> None:
        """Initializes a `stim.CircuitRepeatBlock`.

        Args:
            repeat_count: The number of times to repeat the block.
            body: The body of the block, as a circuit.
        """
    def __ne__(
        self,
        arg0: stim.CircuitRepeatBlock,
    ) -> bool:
        """Determines if two `stim.CircuitRepeatBlock`s are different.
        """
    def __repr__(
        self,
    ) -> str:
        """Returns text that is a valid python expression evaluating to an equivalent `stim.CircuitRepeatBlock`.
        """
    def body_copy(
        self,
    ) -> stim.Circuit:
        """Returns a copy of the body of the repeat block.

        The copy is forced to ensure it's clear that editing the result will not change the circuit that the repeat
        block came from.

        Examples:
            >>> import stim
            >>> circuit = stim.Circuit('''
            ...     H 0
            ...     REPEAT 5 {
            ...         CX 0 1
            ...         CZ 1 2
            ...     }
            ... ''')
            >>> repeat_block = circuit[1]
            >>> repeat_block.body_copy()
            stim.Circuit('''
                CX 0 1
                CZ 1 2
            ''')
        """
    @property
    def repeat_count(
        self,
    ) -> int:
        """The repetition count of the repeat block.

        Examples:
            >>> import stim
            >>> circuit = stim.Circuit('''
            ...     H 0
            ...     REPEAT 5 {
            ...         CX 0 1
            ...         CZ 1 2
            ...     }
            ... ''')
            >>> repeat_block = circuit[1]
            >>> repeat_block.repeat_count
            5
        """
class CircuitTargetsInsideInstruction:
    """Describes a range of targets within a circuit instruction.
    """
    def __init__(
        self,
        *,
        gate: str,
        args: List[float],
        target_range_start: int,
        target_range_end: int,
        targets_in_range: List[stim.GateTargetWithCoords],
    ) -> None:
        """Creates a stim.CircuitTargetsInsideInstruction.
        """
    @property
    def args(
        self,
    ) -> List[float]:
        """Returns parens arguments of the gate / instruction that was being executed.
        """
    @property
    def gate(
        self,
    ) -> Optional[str]:

        """Returns the name of the gate / instruction that was being executed.
        """
    @property
    def target_range_end(
        self,
    ) -> int:
        """Returns the exclusive end of the range of targets that were executing
        within the gate / instruction.
        """
    @property
    def target_range_start(
        self,
    ) -> int:
        """Returns the inclusive start of the range of targets that were executing
        within the gate / instruction.
        """
    @property
    def targets_in_range(
        self,
    ) -> List[stim.GateTargetWithCoords]:
        """Returns the subset of targets of the gate / instruction that were being executed.

        Includes coordinate data with the targets.
        """
class CompiledDemSampler:
    """A helper class for efficiently sampler from a detector error model.

    Examples:
        >>> import stim
        >>> dem = stim.DetectorErrorModel('''
        ...    error(0) D0
        ...    error(1) D1 D2 L0
        ... ''')
        >>> sampler = dem.compile_sampler()
        >>> det_data, obs_data, err_data = sampler.sample(shots=4, return_errors=True)
        >>> det_data
        array([[False,  True,  True],
               [False,  True,  True],
               [False,  True,  True],
               [False,  True,  True]])
        >>> obs_data
        array([[ True],
               [ True],
               [ True],
               [ True]])
        >>> err_data
        array([[False,  True],
               [False,  True],
               [False,  True],
               [False,  True]])
    """
    def sample(
        self,
        shots: int,
        *,
        bit_packed: bool = False,
        return_errors: bool = False,
        recorded_errors_to_replay: Optional[np.ndarray] = None,
    ) -> Tuple[np.ndarray, np.ndarray, Optional[np.ndarray]]:

        """Samples the detector error model's error mechanisms to produce sample data.

        Args:
            shots: The number of times to sample from the model.
            bit_packed: Defaults to false.
                False: the returned numpy arrays have dtype=np.bool8.
                True: the returned numpy arrays have dtype=np.uint8 and pack 8 bits into each byte.

                Setting this to True is equivalent to running np.packbits(data, endian='little', axis=1)
                on each output value, but has the performance benefit of the data never being expanded
                into an unpacked form.
            return_errors: Defaults to False.
                False: the first entry of the returned tuple is None.
                True: the first entry of the returned tuple is a numpy array recording which errors were sampled.
            recorded_errors_to_replay: Defaults to None, meaning sample errors randomly.
                If not None, this is expected to be a 2d numpy array specifying which errors to apply (e.g. one
                returned from a previous call to the sample method). The array must have
                dtype=np.bool8 and shape=(num_shots, num_errors) or
                dtype=np.uint8 and shape=(num_shots, math.ceil(num_errors / 8)).

        Returns:
            A tuple (detector_data, obs_data, error_data).

            Assuming bit_packed is False and return_errors is True:
                If error_data[s, k] is True, then the error with index k fired in the shot with index s.
                If detector_data[s, k] is True, then the detector with index k ended up flipped in the shot with index s.
                If obs_data[s, k] is True, then the observable with index k ended up flipped in the shot with index s.

            The dtype and shape of the data depends on the arguments:
                if bit_packed:
                    detector_data.shape == (num_shots, num_detectors)
                    detector_data.dtype == np.bool8
                    obs_data.shape == (num_shots, num_observables)
                    obs_data.dtype == np.bool8
                    if return_errors:
                        error_data.shape = (num_shots, num_errors)
                        error_data.dtype = np.bool8
                    else:
                        error_data is None
                else:
                    detector_data.shape == (num_shots, math.ceil(num_detectors / 8))
                    detector_data.dtype == np.uint8
                    obs_data.shape == (num_shots, math.ceil(num_observables / 8))
                    obs_data.dtype == np.uint8
                    if return_errors:
                        error_data.shape = (num_shots, math.ceil(num_errors / 8))
                        error_data.dtype = np.uint8
                    else:
                        error_data is None

            Note that bit packing is done using little endian order on the last axis
            (i.e. like `np.packbits(data, endian='little', axis=1)`).

        Examples:
            >>> import stim
            >>> import numpy as np
            >>> dem = stim.DetectorErrorModel('''
            ...    error(0) D0
            ...    error(1) D1 D2 L0
            ... ''')
            >>> sampler = dem.compile_sampler()

            >>> # Taking samples.
            >>> det_data, obs_data, err_data_not_requested = sampler.sample(shots=4)
            >>> det_data
            array([[False,  True,  True],
                   [False,  True,  True],
                   [False,  True,  True],
                   [False,  True,  True]])
            >>> obs_data
            array([[ True],
                   [ True],
                   [ True],
                   [ True]])
            >>> err_data_not_requested is None
            True

            >>> # Recording errors.
            >>> det_data, obs_data, err_data = sampler.sample(shots=4, return_errors=True)
            >>> det_data
            array([[False,  True,  True],
                   [False,  True,  True],
                   [False,  True,  True],
                   [False,  True,  True]])
            >>> obs_data
            array([[ True],
                   [ True],
                   [ True],
                   [ True]])
            >>> err_data
            array([[False,  True],
                   [False,  True],
                   [False,  True],
                   [False,  True]])

            >>> # Bit packing.
            >>> det_data, obs_data, err_data = sampler.sample(shots=4, return_errors=True, bit_packed=True)
            >>> det_data
            array([[6],
                   [6],
                   [6],
                   [6]], dtype=uint8)
            >>> obs_data
            array([[1],
                   [1],
                   [1],
                   [1]], dtype=uint8)
            >>> err_data
            array([[2],
                   [2],
                   [2],
                   [2]], dtype=uint8)

            >>> # Recording and replaying errors.
            >>> noisy_dem = stim.DetectorErrorModel('''
            ...    error(0.125) D0
            ...    error(0.25) D1
            ... ''')
            >>> noisy_sampler = noisy_dem.compile_sampler()
            >>> det_data, obs_data, err_data = noisy_sampler.sample(shots=100, return_errors=True)
            >>> replay_det_data, replay_obs_data, _ = noisy_sampler.sample(shots=100, recorded_errors_to_replay=err_data)
            >>> np.array_equal(det_data, replay_det_data)
            True
            >>> np.array_equal(obs_data, replay_obs_data)
            True
        """
    def sample_write(
        self,
        shots: int,
        *,
        det_out_file: Union[None, str, pathlib.Path],
        det_out_format: str = "01",
        obs_out_file: Union[None, str, pathlib.Path],
        obs_out_format: str = "01",
        err_out_file: Union[None, str, pathlib.Path] = None,
        err_out_format: str = "01",
        replay_err_in_file: Union[None, str, pathlib.Path] = None,
        replay_err_in_format: str = "01",
    ) -> None:

        """Samples the detector error model and writes the results to disk.

        Args:
            shots: The number of times to sample from the model.
            det_out_file: Where to write detection event data.
                If None: detection event data is not written.
                If str or pathlib.Path: opens and overwrites the file at the given path.
                NOT IMPLEMENTED: io.IOBase
            det_out_format: The format to write the detection event data in (e.g. "01" or "b8").
            obs_out_file: Where to write observable flip data.
                If None: observable flip data is not written.
                If str or pathlib.Path: opens and overwrites the file at the given path.
                NOT IMPLEMENTED: io.IOBase
            obs_out_format: The format to write the observable flip data in (e.g. "01" or "b8").
            err_out_file: Where to write errors-that-occurred data.
                If None: errors-that-occurred data is not written.
                If str or pathlib.Path: opens and overwrites the file at the given path.
                NOT IMPLEMENTED: io.IOBase
            err_out_format: The format to write the errors-that-occurred data in (e.g. "01" or "b8").
            replay_err_in_file: If this is specified, errors are replayed from data instead of generated randomly.
                If None: errors are generated randomly according to the probabilities in the detector error model.
                If str or pathlib.Path: the file at the given path is opened and errors-to-apply data is read from there.
                NOT IMPLEMENTED: io.IOBase
            replay_err_in_format: The format to write the errors-that-occurred data in (e.g. "01" or "b8").

        Returns:
            Nothing. Results are written to disk.

        Examples:
            >>> import stim
            >>> import tempfile
            >>> import pathlib
            >>> dem = stim.DetectorErrorModel('''
            ...    error(0) D0
            ...    error(0) D1
            ...    error(0) D0
            ...    error(1) D1 D2 L0
            ...    error(0) D0
            ... ''')
            >>> sampler = dem.compile_sampler()
            >>> with tempfile.TemporaryDirectory() as d:
            ...     d = pathlib.Path(d)
            ...     sampler.sample_write(
            ...         shots=1,
            ...         det_out_file=d / 'dets.01',
            ...         det_out_format='01',
            ...         obs_out_file=d / 'obs.01',
            ...         obs_out_format='01',
            ...         err_out_file=d / 'err.hits',
            ...         err_out_format='hits',
            ...     )
            ...     with open(d / 'dets.01') as f:
            ...         assert f.read() == "011\n"
            ...     with open(d / 'obs.01') as f:
            ...         assert f.read() == "1\n"
            ...     with open(d / 'err.hits') as f:
            ...         assert f.read() == "3\n"
        """
class CompiledDetectorSampler:
    """An analyzed stabilizer circuit whose detection events can be sampled quickly.
    """
    def __init__(
        self,
        circuit: stim.Circuit,
        *,
        seed: object = None,
    ) -> None:
        """Creates a detector sampler, which can sample the detectors (and optionally observables) in a circuit.

        Args:
            circuit: The circuit to sample from.
            seed: PARTIALLY determines simulation results by deterministically seeding the random number generator.
                Must be None or an integer in range(2**64).

                Defaults to None. When set to None, a prng seeded by system entropy is used.

                When set to an integer, making the exact same series calls on the exact same machine with the exact
                same version of Stim will produce the exact same simulation results.

                CAUTION: simulation results *WILL NOT* be consistent between versions of Stim. This restriction is
                present to make it possible to have future optimizations to the random sampling, and is enforced by
                introducing intentional differences in the seeding strategy from version to version.

                CAUTION: simulation results *MAY NOT* be consistent across machines that differ in the width of
                supported SIMD instructions. For example, using the same seed on a machine that supports AVX
                instructions and one that only supports SSE instructions may produce different simulation results.

                CAUTION: simulation results *MAY NOT* be consistent if you vary how many shots are taken. For
                example, taking 10 shots and then 90 shots will give different results from taking 100 shots in one
                call.

        Returns:
            An initialized stim.CompiledDetectorSampler.

        Examples:
            >>> import stim
            >>> c = stim.Circuit('''
            ...    H 0
            ...    CNOT 0 1
            ...    X_ERROR(1.0) 0
            ...    M 0 1
            ...    DETECTOR rec[-1] rec[-2]
            ... ''')
            >>> s = c.compile_detector_sampler()
            >>> s.sample(shots=1)
            array([[ True]])
        """
    def __repr__(
        self,
    ) -> str:
        """Returns text that is a valid python expression evaluating to an equivalent `stim.CompiledDetectorSampler`.
        """
    def sample(
        self,
        shots: int,
        *,
        prepend_observables: bool = False,
        append_observables: bool = False,
    ) -> np.ndarray[bool]:
        """Returns a numpy array containing a batch of detector samples from the circuit.

        The circuit must define the detectors using DETECTOR instructions. Observables defined by OBSERVABLE_INCLUDE
        instructions can also be included in the results as honorary detectors.

        Args:
            shots: The number of times to sample every detector in the circuit.
            prepend_observables: Defaults to false. When set, observables are included with the detectors and are
                placed at the start of the results.
            append_observables: Defaults to false. When set, observables are included with the detectors and are
                placed at the end of the results.

        Returns:
            A numpy array with `dtype=uint8` and `shape=(shots, n)` where
            `n = num_detectors + num_observables*(append_observables + prepend_observables)`.
            The bit for detection event `m` in shot `s` is at `result[s, m]`.
        """
    def sample_bit_packed(
        self,
        shots: int,
        *,
        prepend_observables: bool = False,
        append_observables: bool = False,
    ) -> np.ndarray[np.uint8]:
        """Returns a numpy array containing bit packed batch of detector samples from the circuit.

        The circuit must define the detectors using DETECTOR instructions. Observables defined by OBSERVABLE_INCLUDE
        instructions can also be included in the results as honorary detectors.

        Args:
            shots: The number of times to sample every detector in the circuit.
            prepend_observables: Defaults to false. When set, observables are included with the detectors and are
                placed at the start of the results.
            append_observables: Defaults to false. When set, observables are included with the detectors and are
                placed at the end of the results.

        Returns:
            A numpy array with `dtype=uint8` and `shape=(shots, n)` where
            `n = num_detectors + num_observables*(append_observables + prepend_observables)`.
            The bit for detection event `m` in shot `s` is at `result[s, (m // 8)] & 2**(m % 8)`.
        """
    def sample_write(
        self,
        shots: int,
        *,
        filepath: str,
        format: str = '01',
        prepend_observables: bool = False,
        append_observables: bool = False,
        obs_out_filepath: str = None,
        obs_out_format: str = '01',
    ) -> None:
        """Samples detection events from the circuit and writes them to a file.

        Examples:
            >>> import stim
            >>> import tempfile
            >>> with tempfile.TemporaryDirectory() as d:
            ...     path = f"{d}/tmp.dat"
            ...     c = stim.Circuit('''
            ...         X_ERROR(1) 0
            ...         M 0 1
            ...         DETECTOR rec[-2]
            ...         DETECTOR rec[-1]
            ...     ''')
            ...     c.compile_detector_sampler().sample_write(3, filepath=path, format="dets")
            ...     with open(path) as f:
            ...         print(f.read(), end='')
            shot D0
            shot D0
            shot D0

        Args:
            shots: The number of times to sample every measurement in the circuit.
            filepath: The file to write the results to.
            format: The output format to write the results with.
                Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
                Defaults to "01".
            obs_out_filepath: Sample observables as part of each shot, and write them to this file.
                This keeps the observable data separate from the detector data.
            obs_out_format: If writing the observables to a file, this is the format to write them in.
                Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
                Defaults to "01".
            prepend_observables: Sample observables as part of each shot, and put them at the start of the detector
                data.
            append_observables: Sample observables as part of each shot, and put them at the end of the detector
                data.

        Returns:
            None.
        """
class CompiledMeasurementSampler:
    """An analyzed stabilizer circuit whose measurements can be sampled quickly.
    """
    def __init__(
        self,
        circuit: stim.Circuit,
        *,
        skip_reference_sample: bool = False,
        seed: object = None,
    ) -> None:
        """Creates a measurement sampler for the given circuit.

        The sampler uses a noiseless reference sample, collected from the circuit using stim's Tableau simulator
        during initialization of the sampler, as a baseline for deriving more samples using an error propagation
        simulator.

        Args:
            circuit: The stim circuit to sample from.
            skip_reference_sample: Defaults to False. When set to True, the reference sample used by the sampler is
                initialized to all-zeroes instead of being collected from the circuit. This means that the results
                returned by the sampler are actually whether or not each measurement was *flipped*, instead of true
                measurement results.

                Forcing an all-zero reference sample is useful when you are only interested in error propagation and
                don't want to have to deal with the fact that some measurements want to be On when no errors occur.
                It is also useful when you know for sure that the all-zero result is actually a possible result from
                the circuit (under noiseless execution), meaning it is a valid reference sample as good as any
                other. Computing the reference sample is the most time consuming and memory intensive part of
                simulating the circuit, so promising that the simulator can safely skip that step is an effective
                optimization.
            seed: PARTIALLY determines simulation results by deterministically seeding the random number generator.
                Must be None or an integer in range(2**64).

                Defaults to None. When set to None, a prng seeded by system entropy is used.

                When set to an integer, making the exact same series calls on the exact same machine with the exact
                same version of Stim will produce the exact same simulation results.

                CAUTION: simulation results *WILL NOT* be consistent between versions of Stim. This restriction is
                present to make it possible to have future optimizations to the random sampling, and is enforced by
                introducing intentional differences in the seeding strategy from version to version.

                CAUTION: simulation results *MAY NOT* be consistent across machines that differ in the width of
                supported SIMD instructions. For example, using the same seed on a machine that supports AVX
                instructions and one that only supports SSE instructions may produce different simulation results.

                CAUTION: simulation results *MAY NOT* be consistent if you vary how many shots are taken. For
                example, taking 10 shots and then 90 shots will give different results from taking 100 shots in one
                call.

        Returns:
            An initialized stim.CompiledMeasurementSampler.

        Examples:
            >>> import stim
            >>> c = stim.Circuit('''
            ...    X 0   2 3
            ...    M 0 1 2 3
            ... ''')
            >>> s = c.compile_sampler()
            >>> s.sample(shots=1)
            array([[ True, False,  True,  True]])
        """
    def __repr__(
        self,
    ) -> str:
        """Returns text that is a valid python expression evaluating to an equivalent `stim.CompiledMeasurementSampler`.
        """
    def sample(
        self,
        shots: int,
    ) -> np.ndarray[bool]:
        """Returns a numpy array containing a batch of measurement samples from the circuit.

        Examples:
            >>> import stim
            >>> c = stim.Circuit('''
            ...    X 0   2 3
            ...    M 0 1 2 3
            ... ''')
            >>> s = c.compile_sampler()
            >>> s.sample(shots=1)
            array([[ True, False,  True,  True]])

        Args:
            shots: The number of times to sample every measurement in the circuit.

        Returns:
            A numpy array with `dtype=uint8` and `shape=(shots, num_measurements)`.
            The bit for measurement `m` in shot `s` is at `result[s, m]`.
        """
    def sample_bit_packed(
        self,
        shots: int,
    ) -> np.ndarray[np.uint8]:
        """Returns a numpy array containing a bit packed batch of measurement samples from the circuit.

        Examples:
            >>> import stim
            >>> c = stim.Circuit('''
            ...    X 0 1 2 3 4 5 6 7     10
            ...    M 0 1 2 3 4 5 6 7 8 9 10
            ... ''')
            >>> s = c.compile_sampler()
            >>> s.sample_bit_packed(shots=1)
            array([[255,   4]], dtype=uint8)

        Args:
            shots: The number of times to sample every measurement in the circuit.

        Returns:
            A numpy array with `dtype=uint8` and `shape=(shots, (num_measurements + 7) // 8)`.
            The bit for measurement `m` in shot `s` is at `result[s, (m // 8)] & 2**(m % 8)`.
        """
    def sample_write(
        self,
        shots: int,
        *,
        filepath: str,
        format: str = '01',
    ) -> None:
        """Samples measurements from the circuit and writes them to a file.

        Examples:
            >>> import stim
            >>> import tempfile
            >>> with tempfile.TemporaryDirectory() as d:
            ...     path = f"{d}/tmp.dat"
            ...     c = stim.Circuit('''
            ...         X 0   2 3
            ...         M 0 1 2 3
            ...     ''')
            ...     c.compile_sampler().sample_write(5, filepath=path, format="01")
            ...     with open(path) as f:
            ...         print(f.read(), end='')
            1011
            1011
            1011
            1011
            1011

        Args:
            shots: The number of times to sample every measurement in the circuit.
            filepath: The file to write the results to.
            format: The output format to write the results with.
                Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
                Defaults to "01".

        Returns:
            None.
        """
class CompiledMeasurementsToDetectionEventsConverter:
    """A tool for quickly converting measurements from an analyzed stabilizer circuit into detection events.
    """
    def __init__(
        self,
        circuit: stim.Circuit,
        *,
        skip_reference_sample: bool = False,
    ) -> None:
        """Creates a measurement-to-detection-events converter for the given circuit.

        The converter uses a noiseless reference sample, collected from the circuit using stim's Tableau simulator
        during initialization of the converter, as a baseline for determining what the expected value of a detector
        is.

        Note that the expected behavior of gauge detectors (detectors that are not actually deterministic under
        noiseless execution) can vary depending on the reference sample. Stim mitigates this by always generating
        the same reference sample for a given circuit.

        Args:
            circuit: The stim circuit to use for conversions.
            skip_reference_sample: Defaults to False. When set to True, the reference sample used by the converter
                is initialized to all-zeroes instead of being collected from the circuit. This should only be used
                if it's known that the all-zeroes sample is actually a possible result from the circuit (under
                noiseless execution).

        Returns:
            An initialized stim.CompiledMeasurementsToDetectionEventsConverter.

        Examples:
            >>> import stim
            >>> import numpy as np
            >>> converter = stim.Circuit('''
            ...    X 0
            ...    M 0
            ...    DETECTOR rec[-1]
            ... ''').compile_m2d_converter()
            >>> converter.convert(
            ...     measurements=np.array([[0], [1]], dtype=np.bool8),
            ...     append_observables=False,
            ... )
            array([[ True],
                   [False]])
        """
    def __repr__(
        self,
    ) -> str:
        """Returns text that is a valid python expression evaluating to an equivalent `stim.CompiledMeasurementsToDetectionEventsConverter`.
        """
    def convert(
        self,
        *,
        measurements: np.ndarray,
        sweep_bits: Optional[np.ndarray] = None,
        separate_observables: bool = False,
        append_observables: bool = False,
        bit_pack_result: bool = False,
    ) -> Union[np.ndarray, Tuple[np.ndarray, np.ndarray]]:

        """Converts measurement data into detection event data.

        Args:
            measurements: A numpy array containing measurement data. The dtype of the array is used
                to determine if it is bit packed or not.

                dtype=np.bool8 (unpacked data):
                    shape=(num_shots, circuit.num_measurements)
                dtype=np.uint8 (bit packed data):
                    shape=(num_shots, math.ceil(circuit.num_measurements / 8))
            sweep_bits: Optional. A numpy array containing sweep data for the `sweep[k]` controls in the circuit.
                The dtype of the array is used to determine if it is bit packed or not.

                dtype=np.bool8 (unpacked data):
                    shape=(num_shots, circuit.num_sweep_bits)
                dtype=np.uint8 (bit packed data):
                    shape=(num_shots, math.ceil(circuit.num_sweep_bits / 8))
            separate_observables: Defaults to False. When set to True, two numpy arrays are returned instead of one,
                with the second array containing the observable flip data.
            append_observables: Defaults to False. When set to True, the observables in the circuit are treated as
                if they were additional detectors. Their results are appended to the end of the detection event
                data.
            bit_pack_result: Defaults to False. When set to True, the returned numpy array contains bit packed
                data (dtype=np.uint8 with 8 bits per item) instead of unpacked data (dtype=np.bool8).

        Returns:
            The detection event data and (optionally) observable data.
            The result is a single numpy array if separate_observables is false, otherwise it's a tuple of two numpy arrays.
            When returning two numpy arrays, the first array is the detection event data and the second is the observable flip data.
            The dtype of the returned arrays is np.bool8 if bit_pack_result is false, otherwise they're np.uint8 arrays.
            shape[0] of the array(s) is the number of shots.
            shape[1] of the array(s) is the number of bits per shot (divided by 8 if bit packed) (e.g. for just detection event data it would be circuit.num_detectors).

        Examples:
            >>> import stim
            >>> import numpy as np
            >>> converter = stim.Circuit('''
            ...    X 0
            ...    M 0 1
            ...    DETECTOR rec[-1]
            ...    DETECTOR rec[-2]
            ...    OBSERVABLE_INCLUDE(0) rec[-2]
            ... ''').compile_m2d_converter()
            >>> dets, obs = converter.convert(
            ...     measurements=np.array([[1, 0],
            ...                            [1, 0],
            ...                            [1, 0],
            ...                            [0, 0],
            ...                            [1, 0]], dtype=np.bool8),
            ...     separate_observables=True,
            ... )
            >>> dets
            array([[False, False],
                   [False, False],
                   [False, False],
                   [False,  True],
                   [False, False]])
            >>> obs
            array([[False],
                   [False],
                   [False],
                   [ True],
                   [False]])
        """
    def convert_file(
        self,
        *,
        measurements_filepath: str,
        measurements_format: str = '01',
        sweep_bits_filepath: str = None,
        sweep_bits_format: str = '01',
        detection_events_filepath: str,
        detection_events_format: str = '01',
        append_observables: bool = False,
        obs_out_filepath: str = None,
        obs_out_format: str = '01',
    ) -> None:
        """Reads measurement data from a file, converts it, and writes the detection events to another file.

        Args:
            measurements_filepath: A file containing measurement data to be converted.
            measurements_format: The format the measurement data is stored in.
                Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
                Defaults to "01".
            detection_events_filepath: Where to save detection event data to.
            detection_events_format: The format to save the detection event data in.
                Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
                Defaults to "01".
            sweep_bits_filepath: Defaults to None. A file containing sweep data, or None.
                When specified, sweep data (used for `sweep[k]` controls in the circuit, which can vary from shot to
                shot) will be read from the given file.
                When not specified, all sweep bits default to False and no sweep-controlled operations occur.
            sweep_bits_format: The format the sweep data is stored in.
                Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
                Defaults to "01".
            obs_out_filepath: Sample observables as part of each shot, and write them to this file.
                This keeps the observable data separate from the detector data.
            obs_out_format: If writing the observables to a file, this is the format to write them in.
                Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
                Defaults to "01".
            append_observables: When True, the observables in the circuit are included as part of the detection
                event data. Specifically, they are treated as if they were additional detectors at the end of the
                circuit. When False, observable data is not output.

        Examples:
            >>> import stim
            >>> import tempfile
            >>> converter = stim.Circuit('''
            ...    X 0
            ...    M 0
            ...    DETECTOR rec[-1]
            ... ''').compile_m2d_converter()
            >>> with tempfile.TemporaryDirectory() as d:
            ...    with open(f"{d}/measurements.01", "w") as f:
            ...        print("0", file=f)
            ...        print("1", file=f)
            ...    converter.convert_file(
            ...        measurements_filepath=f"{d}/measurements.01",
            ...        detection_events_filepath=f"{d}/detections.01",
            ...        append_observables=False,
            ...    )
            ...    with open(f"{d}/detections.01", "r") as f:
            ...        print(f.read(), end="")
            1
            0
        """
class DemInstruction:
    """An instruction from a detector error model.

    Examples:
        >>> import stim
        >>> model = stim.DetectorErrorModel('''
        ...     error(0.125) D0
        ...     error(0.125) D0 D1 L0
        ...     error(0.125) D1 D2
        ...     error(0.125) D2 D3
        ...     error(0.125) D3
        ... ''')
        >>> instruction = model[0]
        >>> instruction
        stim.DemInstruction('error', [0.125], [stim.target_relative_detector_id(0)])
    """
    def __eq__(
        self,
        arg0: stim.DemInstruction,
    ) -> bool:
        """Determines if two instructions have identical contents.
        """
    def __init__(
        self,
        type: str,
        args: List[float],
        targets: List[object],
    ) -> None:
        """Creates a stim.DemInstruction.

        Args:
            type: The name of the instruction type (e.g. "error" or "shift_detectors").
            args: Numeric values parameterizing the instruction (e.g. the 0.1 in "error(0.1)").
            targets: The objects the instruction involves (e.g. the "D0" and "L1" in "error(0.1) D0 L1").

        Examples:
            >>> import stim
            >>> instruction = stim.DemInstruction('error', [0.125], [stim.target_relative_detector_id(5)])
            >>> print(instruction)
            error(0.125) D5
        """
    def __ne__(
        self,
        arg0: stim.DemInstruction,
    ) -> bool:
        """Determines if two instructions have non-identical contents.
        """
    def __repr__(
        self,
    ) -> str:
        """Returns text that is a valid python expression evaluating to an equivalent `stim.DetectorErrorModel`.
        """
    def __str__(
        self,
    ) -> str:
        """Returns detector error model (.dem) instructions (that can be parsed by stim) for the model.
        """
    def args_copy(
        self,
    ) -> List[float]:
        """Returns a copy of the list of numbers parameterizing the instruction (e.g. the probability of an error).
        """
    def targets_copy(
        self,
    ) -> List[Union[int, stim.DemTarget]]:

        """Returns a copy of the list of objects the instruction applies to (e.g. affected detectors.
        """
    @property
    def type(
        self,
    ) -> str:
        """The name of the instruction type (e.g. "error" or "shift_detectors").
        """
class DemRepeatBlock:
    """A repeat block from a detector error model.

    Examples:
        >>> import stim
        >>> model = stim.DetectorErrorModel('''
        ...     repeat 100 {
        ...         error(0.125) D0 D1
        ...         shift_detectors 1
        ...     }
        ... ''')
        >>> model[0]
        stim.DemRepeatBlock(100, stim.DetectorErrorModel('''
            error(0.125) D0 D1
            shift_detectors 1
        '''))
    """
    def __eq__(
        self,
        arg0: stim.DemRepeatBlock,
    ) -> bool:
        """Determines if two repeat blocks are identical.
        """
    def __init__(
        self,
        repeat_count: int,
        block: stim.DetectorErrorModel,
    ) -> None:
        """Creates a stim.DemRepeatBlock.

        Args:
            repeat_count: The number of times the repeat block's body is supposed to execute.
            block: The body of the repeat block as a DetectorErrorModel containing the instructions to repeat.

        Examples:
            >>> import stim
            >>> repeat_block = stim.DemRepeatBlock(100, stim.DetectorErrorModel('''
            ...     error(0.125) D0 D1
            ...     shift_detectors 1
            ... '''))
        """
    def __ne__(
        self,
        arg0: stim.DemRepeatBlock,
    ) -> bool:
        """Determines if two repeat blocks are different.
        """
    def __repr__(
        self,
    ) -> str:
        """Returns text that is a valid python expression evaluating to an equivalent `stim.DemRepeatBlock`.
        """
    def body_copy(
        self,
    ) -> stim.DetectorErrorModel:
        """Returns a copy of the block's body, as a stim.DetectorErrorModel.
        """
    @property
    def repeat_count(
        self,
    ) -> int:
        """The number of times the repeat block's body is supposed to execute.
        """
class DemTarget:
    """An instruction target from a detector error model (.dem) file.
    """
    def __eq__(
        self,
        arg0: stim.DemTarget,
    ) -> bool:
        """Determines if two `stim.DemTarget`s are identical.
        """
    def __ne__(
        self,
        arg0: stim.DemTarget,
    ) -> bool:
        """Determines if two `stim.DemTarget`s are different.
        """
    def __repr__(
        self,
    ) -> str:
        """Returns text that is a valid python expression evaluating to an equivalent `stim.DemTarget`.
        """
    def __str__(
        self,
    ) -> str:
        """Returns a text description of the detector error model target.
        """
    def is_logical_observable_id(
        self,
    ) -> bool:
        """Determines if the detector error model target is a logical observable id target (like "L5" in a .dem file).
        """
    def is_relative_detector_id(
        self,
    ) -> bool:
        """Determines if the detector error model target is a relative detector id target (like "D4" in a .dem file).
        """
    def is_separator(
        self,
    ) -> bool:
        """Determines if the detector error model target is a separator (like "^" in a .dem file).
        """
    @staticmethod
    def logical_observable_id(
        index: int,
    ) -> stim.DemTarget:
        """Returns a logical observable id identifying a frame change (e.g. "L5" in a .dem file).

        Args:
            index: The index of the observable.

        Returns:
            The logical observable target.

        Examples:
            >>> import stim
            >>> m = stim.DetectorErrorModel()
            >>> m.append("error", 0.25, [
            ...     stim.DemTarget.logical_observable_id(13)
            ... ])
            >>> print(repr(m))
            stim.DetectorErrorModel('''
                error(0.25) L13
            ''')
        """
    @staticmethod
    def relative_detector_id(
        index: int,
    ) -> stim.DemTarget:
        """Returns a relative detector id (e.g. "D5" in a .dem file).

        Args:
            index: The index of the detector, relative to the current detector offset.

        Returns:
            The relative detector target.

        Examples:
            >>> import stim
            >>> m = stim.DetectorErrorModel()
            >>> m.append("error", 0.25, [
            ...     stim.DemTarget.relative_detector_id(13)
            ... ])
            >>> print(repr(m))
            stim.DetectorErrorModel('''
                error(0.25) D13
            ''')
        """
    @staticmethod
    def separator(
    ) -> stim.DemTarget:
        """Returns a target separator (e.g. "^" in a .dem file).

        Examples:
            >>> import stim
            >>> m = stim.DetectorErrorModel()
            >>> m.append("error", 0.25, [
            ...     stim.DemTarget.relative_detector_id(1),
            ...     stim.DemTarget.separator(),
            ...     stim.DemTarget.relative_detector_id(2),
            ... ])
            >>> print(repr(m))
            stim.DetectorErrorModel('''
                error(0.25) D1 ^ D2
            ''')
        """
    @property
    def val(
        self,
    ) -> int:
        """Returns the target's integer value.

        Example:

            >>> import stim
            >>> stim.target_relative_detector_id(5).val
            5
            >>> stim.target_logical_observable_id(6).val
            6
        """
class DemTargetWithCoords:
    """A detector error model instruction target with associated coords.

    It is also guaranteed that, if the type of the DEM target is a
    relative detector id, it is actually absolute (i.e. relative to
    0).

    For example, if the DEM target is a detector from a circuit with
    coordinate arguments given to detectors, the coords field will
    contain the coordinate data for the detector.

    This is helpful information to have available when debugging a
    problem in a circuit, instead of having to constantly manually
    look up the coordinates of a detector index in order to understand
    what is happening.
    """
    def __init__(
        self,
        *,
        dem_target: stim.DemTarget,
        coords: List[float],
    ) -> None:
        """Creates a stim.DemTargetWithCoords.
        """
    @property
    def coords(
        self,
    ) -> List[float]:
        """Returns the associated coordinate information as a list of flaots.

        If there is no coordinate information, returns an empty list.
        """
    @property
    def dem_target(
        self,
    ) -> stim.DemTarget:
        """Returns the actual DEM target as a `stim.DemTarget`.
        """
class DetectorErrorModel:
    """A list of instructions describing error mechanisms in terms of the detection events they produce.

    Examples:
        >>> import stim
        >>> model = stim.DetectorErrorModel('''
        ...     error(0.125) D0
        ...     error(0.125) D0 D1 L0
        ...     error(0.125) D1 D2
        ...     error(0.125) D2 D3
        ...     error(0.125) D3
        ... ''')
        >>> len(model)
        5

        >>> stim.Circuit('''
        ...     X_ERROR(0.125) 0
        ...     X_ERROR(0.25) 1
        ...     CORRELATED_ERROR(0.375) X0 X1
        ...     M 0 1
        ...     DETECTOR rec[-2]
        ...     DETECTOR rec[-1]
        ... ''').detector_error_model()
        stim.DetectorErrorModel('''
            error(0.125) D0
            error(0.375) D0 D1
            error(0.25) D1
        ''')
    """
    def __add__(
        self,
        second: stim.DetectorErrorModel,
    ) -> stim.DetectorErrorModel:
        """Creates a detector error model by appending two models.

        Examples:
            >>> import stim
            >>> m1 = stim.DetectorErrorModel('''
            ...    error(0.125) D0
            ... ''')
            >>> m2 = stim.DetectorErrorModel('''
            ...    error(0.25) D1
            ... ''')
            >>> m1 + m2
            stim.DetectorErrorModel('''
                error(0.125) D0
                error(0.25) D1
            ''')
        """
    def __eq__(
        self,
        arg0: stim.DetectorErrorModel,
    ) -> bool:
        """Determines if two detector error models have identical contents.
        """
    @overload
    def __getitem__(
        self,
        index_or_slice: int,
    ) -> Union[stim.DemInstruction, stim.DemRepeatBlock]:
        pass
    @overload
    def __getitem__(
        self,
        index_or_slice: slice,
    ) -> stim.DetectorErrorModel:
        pass
    def __getitem__(
        self,
        index_or_slice: object,
    ) -> object:
        """Returns copies of instructions from the detector error model.

        Args:
            index_or_slice: An integer index picking out an instruction to return, or a slice picking out a range
                of instructions to return as a detector error model.

        Examples:
        Examples:
            >>> import stim
            >>> model = stim.DetectorErrorModel('''
            ...    error(0.125) D0
            ...    error(0.125) D1 L1
            ...    repeat 100 {
            ...        error(0.125) D1 D2
            ...        shift_detectors 1
            ...    }
            ...    error(0.125) D2
            ...    logical_observable L0
            ...    detector D5
            ... ''')
            >>> model[1]
            stim.DemInstruction('error', [0.125], [stim.target_relative_detector_id(1), stim.target_logical_observable_id(1)])
            >>> model[2]
            stim.DemRepeatBlock(100, stim.DetectorErrorModel('''
                error(0.125) D1 D2
                shift_detectors 1
            '''))
            >>> model[1::2]
            stim.DetectorErrorModel('''
                error(0.125) D1 L1
                error(0.125) D2
                detector D5
            ''')
        """
    def __iadd__(
        self,
        second: stim.DetectorErrorModel,
    ) -> stim.DetectorErrorModel:
        """Appends a detector error model into the receiving model (mutating it).

        Examples:
            >>> import stim
            >>> m1 = stim.DetectorErrorModel('''
            ...    error(0.125) D0
            ... ''')
            >>> m2 = stim.DetectorErrorModel('''
            ...    error(0.25) D1
            ... ''')
            >>> m1 += m2
            >>> print(repr(m1))
            stim.DetectorErrorModel('''
                error(0.125) D0
                error(0.25) D1
            ''')
        """
    def __imul__(
        self,
        repetitions: int,
    ) -> stim.DetectorErrorModel:
        """Mutates the detector error model by putting its contents into a repeat block.

        Special case: if the repetition count is 0, the model is cleared.
        Special case: if the repetition count is 1, nothing happens.

        Args:
            repetitions: The number of times the repeat block should repeat.

        Examples:
            >>> import stim
            >>> m = stim.DetectorErrorModel('''
            ...    error(0.25) D0
            ...    shift_detectors 1
            ... ''')
            >>> m *= 3
            >>> print(m)
            repeat 3 {
                error(0.25) D0
                shift_detectors 1
            }
        """
    def __init__(
        self,
        detector_error_model_text: str = '',
    ) -> None:
        """Creates a stim.DetectorErrorModel.

        Args:
            detector_error_model_text: Defaults to empty. Describes instructions to append into the circuit in the
                detector error model (.dem) format.

        Examples:
            >>> import stim
            >>> empty = stim.DetectorErrorModel()
            >>> not_empty = stim.DetectorErrorModel('''
            ...    error(0.125) D0 L0
            ... ''')
        """
    def __len__(
        self,
    ) -> int:
        """Returns the number of top-level instructions and blocks in the detector error model.

        Instructions inside of blocks are not included in this count.

        Examples:
            >>> import stim
            >>> len(stim.DetectorErrorModel())
            0
            >>> len(stim.DetectorErrorModel('''
            ...    error(0.1) D0 D1
            ...    shift_detectors 100
            ...    logical_observable L5
            ... '''))
            3
            >>> len(stim.DetectorErrorModel('''
            ...    repeat 100 {
            ...        error(0.1) D0 D1
            ...        error(0.1) D1 D2
            ...    }
            ... '''))
            1
        """
    def __mul__(
        self,
        repetitions: int,
    ) -> stim.DetectorErrorModel:
        """Returns a detector error model with a repeat block containing the current model's instructions.

        Special case: if the repetition count is 0, an empty model is returned.
        Special case: if the repetition count is 1, an equal model with no repeat block is returned.

        Args:
            repetitions: The number of times the repeat block should repeat.

        Examples:
            >>> import stim
            >>> m = stim.DetectorErrorModel('''
            ...    error(0.25) D0
            ...    shift_detectors 1
            ... ''')
            >>> m * 3
            stim.DetectorErrorModel('''
                repeat 3 {
                    error(0.25) D0
                    shift_detectors 1
                }
            ''')
        """
    def __ne__(
        self,
        arg0: stim.DetectorErrorModel,
    ) -> bool:
        """Determines if two detector error models have non-identical contents.
        """
    def __repr__(
        self,
    ) -> str:
        """"Returns text that is a valid python expression evaluating to an equivalent `stim.DetectorErrorModel`."
        """
    def __rmul__(
        self,
        repetitions: int,
    ) -> stim.DetectorErrorModel:
        """Returns a detector error model with a repeat block containing the current model's instructions.

        Special case: if the repetition count is 0, an empty model is returned.
        Special case: if the repetition count is 1, an equal model with no repeat block is returned.

        Args:
            repetitions: The number of times the repeat block should repeat.

        Examples:
            >>> import stim
            >>> m = stim.DetectorErrorModel('''
            ...    error(0.25) D0
            ...    shift_detectors 1
            ... ''')
            >>> 3 * m
            stim.DetectorErrorModel('''
                repeat 3 {
                    error(0.25) D0
                    shift_detectors 1
                }
            ''')
        """
    def __str__(
        self,
    ) -> str:
        """"Returns detector error model (.dem) instructions (that can be parsed by stim) for the model.");
        """
    def append(
        self,
        instruction: object,
        parens_arguments: object = None,
        targets: List[object] = (),
    ) -> None:
        """Appends an instruction to the detector error model.

        Args:
            instruction: Either the name of an instruction, a stim.DemInstruction, or a stim.DemRepeatBlock.
                The `parens_arguments` and `targets` arguments are given if and only if the instruction is a name.
            parens_arguments: Numeric values parameterizing the instruction. The numbers inside parentheses in a
                detector error model file (eg. the `0.25` in `error(0.25) D0`). This argument can be given either
                a list of doubles, or a single double (which will be implicitly wrapped into a list).
            targets: The instruction targets, such as the `D0` in `error(0.25) D0`.

        Examples:
            >>> import stim
            >>> m = stim.DetectorErrorModel()
            >>> m.append("error", 0.125, [
            ...     stim.DemTarget.relative_detector_id(1),
            ... ])
            >>> m.append("error", 0.25, [
            ...     stim.DemTarget.relative_detector_id(1),
            ...     stim.DemTarget.separator(),
            ...     stim.DemTarget.relative_detector_id(2),
            ...     stim.DemTarget.logical_observable_id(3),
            ... ])
            >>> print(repr(m))
            stim.DetectorErrorModel('''
                error(0.125) D1
                error(0.25) D1 ^ D2 L3
            ''')

            >>> m.append("shift_detectors", (1, 2, 3), [5])
            >>> print(repr(m))
            stim.DetectorErrorModel('''
                error(0.125) D1
                error(0.25) D1 ^ D2 L3
                shift_detectors(1, 2, 3) 5
            ''')

            >>> m += m * 3
            >>> m.append(m[0])
            >>> m.append(m[-2])
            >>> print(repr(m))
            stim.DetectorErrorModel('''
                error(0.125) D1
                error(0.25) D1 ^ D2 L3
                shift_detectors(1, 2, 3) 5
                repeat 3 {
                    error(0.125) D1
                    error(0.25) D1 ^ D2 L3
                    shift_detectors(1, 2, 3) 5
                }
                error(0.125) D1
                repeat 3 {
                    error(0.125) D1
                    error(0.25) D1 ^ D2 L3
                    shift_detectors(1, 2, 3) 5
                }
            ''')
        """
    def approx_equals(
        self,
        other: object,
        *,
        atol: float,
    ) -> bool:
        """Checks if a detector error model is approximately equal to another detector error model.

        Two detector error model are approximately equal if they are equal up to slight perturbations of instruction
        arguments such as probabilities. For example `error(0.100) D0` is approximately equal to `error(0.099) D0`
        within an absolute tolerance of 0.002. All other details of the models (such as the ordering of errors and
        their targets) must be exactly the same.

        Args:
            other: The detector error model, or other object, to compare to this one.
            atol: The absolute error tolerance. The maximum amount each probability may have been perturbed by.

        Returns:
            True if the given object is a detector error model approximately equal up to the receiving circuit up to
            the given tolerance, otherwise False.

        Examples:
            >>> import stim
            >>> base = stim.DetectorErrorModel('''
            ...    error(0.099) D0 D1
            ... ''')

            >>> base.approx_equals(base, atol=0)
            True

            >>> base.approx_equals(stim.DetectorErrorModel('''
            ...    error(0.101) D0 D1
            ... '''), atol=0)
            False

            >>> base.approx_equals(stim.DetectorErrorModel('''
            ...    error(0.101) D0 D1
            ... '''), atol=0.0001)
            False

            >>> base.approx_equals(stim.DetectorErrorModel('''
            ...    error(0.101) D0 D1
            ... '''), atol=0.01)
            True

            >>> base.approx_equals(stim.DetectorErrorModel('''
            ...    error(0.099) D0 D1 L0 L1 L2 L3 L4
            ... '''), atol=9999)
            False
        """
    def clear(
        self,
    ) -> None:
        """Clears the contents of the detector error model.

        Examples:
            >>> import stim
            >>> model = stim.DetectorErrorModel('''
            ...    error(0.1) D0 D1
            ... ''')
            >>> model.clear()
            >>> model
            stim.DetectorErrorModel()
        """
    def compile_sampler(
        self,
        *,
        seed: object = None,
    ) -> stim::DemSampler:
        """Returns a CompiledDemSampler, which can quickly batch sample from detector error models.

        Args:
            seed: PARTIALLY determines simulation results by deterministically seeding the random number generator.
                Must be None or an integer in range(2**64).

                Defaults to None. When set to None, a prng seeded by system entropy is used.

                When set to an integer, making the exact same series calls on the exact same machine with the exact
                same version of Stim will produce the exact same simulation results.

                CAUTION: simulation results *WILL NOT* be consistent between versions of Stim. This restriction is
                present to make it possible to have future optimizations to the random sampling, and is enforced by
                introducing intentional differences in the seeding strategy from version to version.

                CAUTION: simulation results *MAY NOT* be consistent across machines that differ in the width of
                supported SIMD instructions. For example, using the same seed on a machine that supports AVX
                instructions and one that only supports SSE instructions may produce different simulation results.

                CAUTION: simulation results *MAY NOT* be consistent if you vary how many shots are taken. For
                example, taking 10 shots and then 90 shots will give different results from taking 100 shots in one
                call.

        Returns:
            A seeded stim.CompiledDemSampler for the given detector error model.

        Examples:
            >>> import stim
            >>> dem = stim.DetectorErrorModel('''
            ...    error(0) D0
            ...    error(1) D1 D2 L0
            ... ''')
            >>> sampler = dem.compile_sampler()
            >>> det_data, obs_data, err_data = sampler.sample(shots=4, return_errors=True)
            >>> det_data
            array([[False,  True,  True],
                   [False,  True,  True],
                   [False,  True,  True],
                   [False,  True,  True]])
            >>> obs_data
            array([[ True],
                   [ True],
                   [ True],
                   [ True]])
            >>> err_data
            array([[False,  True],
                   [False,  True],
                   [False,  True],
                   [False,  True]])
        """
    def copy(
        self,
    ) -> stim.DetectorErrorModel:
        """Returns a copy of the detector error model. An independent model with the same contents.

        Examples:
            >>> import stim

            >>> c1 = stim.DetectorErrorModel("error(0.1) D0 D1")
            >>> c2 = c1.copy()
            >>> c2 is c1
            False
            >>> c2 == c1
            True
        """
    def flattened(
        self,
    ) -> stim.DetectorErrorModel:
        """Creates an equivalent detector error model without repeat blocks or detector_shift instructions.

        Returns:
            A `stim.DetectorErrorModel` with the same errors in the same order,
            but with loops flattened into repeated instructions and with
            all coordinate/index shifts inlined.

        Examples:
            >>> import stim
            >>> stim.DetectorErrorModel('''
            ...     error(0.125) D0
            ...     REPEAT 5 {
            ...         error(0.25) D0 D1
            ...         shift_detectors 1
            ...     }
            ...     error(0.125) D0 L0
            ... ''').flattened()
            stim.DetectorErrorModel('''
                error(0.125) D0
                error(0.25) D0 D1
                error(0.25) D1 D2
                error(0.25) D2 D3
                error(0.25) D3 D4
                error(0.25) D4 D5
                error(0.125) D5 L0
            ''')
        """
    @staticmethod
    def from_file(
        file: Union[io.TextIOBase, str, pathlib.Path],
    ) -> stim.DetectorErrorModel:

        """Reads a detector error model from a file.

        The file format is defined at https://github.com/quantumlib/Stim/blob/main/doc/file_format_dem_detector_error_model.md

        Args:
            file: A file path or open file object to read from.

        Returns:
            The circuit parsed from the file.

        Examples:
            >>> import stim
            >>> import tempfile

            >>> with tempfile.TemporaryDirectory() as tmpdir:
            ...     path = tmpdir + '/tmp.stim'
            ...     with open(path, 'w') as f:
            ...         print('error(0.25) D2 D3', file=f)
            ...     circuit = stim.DetectorErrorModel.from_file(path)
            >>> circuit
            stim.DetectorErrorModel('''
                error(0.25) D2 D3
            ''')

            >>> with tempfile.TemporaryDirectory() as tmpdir:
            ...     path = tmpdir + '/tmp.stim'
            ...     with open(path, 'w') as f:
            ...         print('error(0.25) D2 D3', file=f)
            ...     with open(path) as f:
            ...         circuit = stim.DetectorErrorModel.from_file(path)
            >>> circuit
            stim.DetectorErrorModel('''
                error(0.25) D2 D3
            ''')
        """
    def get_detector_coordinates(
        self,
        only: object = None,
    ) -> Dict[int, List[float]]:
        """Returns the coordinate metadata of detectors in the detector error model.

        Args:
            only: Defaults to None (meaning include all detectors). A list of detector indices to include in the
                result. Detector indices beyond the end of the detector error model cause an error.

        Returns:
            A dictionary mapping integers (detector indices) to lists of floats (coordinates).
            Detectors with no specified coordinate data are mapped to an empty tuple.
            If `only` is specified, then `set(result.keys()) == set(only)`.

        Examples:
            >>> import stim
            >>> dem = stim.DetectorErrorModel('''
            ...    error(0.25) D0 D1
            ...    detector(1, 2, 3) D1
            ...    shift_detectors(5) 1
            ...    detector(1, 2) D2
            ... ''')
            >>> dem.get_detector_coordinates()
            {0: [], 1: [1.0, 2.0, 3.0], 2: [], 3: [6.0, 2.0]}
            >>> dem.get_detector_coordinates(only=[1])
            {1: [1.0, 2.0, 3.0]}
        """
    @property
    def num_detectors(
        self,
    ) -> int:
        """Counts the number of detectors (e.g. `D2`) in the error model.

        Detector indices are assumed to be contiguous from 0 up to whatever the maximum detector id is.
        If the largest detector's absolute id is n-1, then the number of detectors is n.

        Examples:
            >>> import stim

            >>> stim.Circuit('''
            ...     X_ERROR(0.125) 0
            ...     X_ERROR(0.25) 1
            ...     CORRELATED_ERROR(0.375) X0 X1
            ...     M 0 1
            ...     DETECTOR rec[-2]
            ...     DETECTOR rec[-1]
            ... ''').detector_error_model().num_detectors
            2

            >>> stim.DetectorErrorModel('''
            ...    error(0.1) D0 D199
            ... ''').num_detectors
            200

            >>> stim.DetectorErrorModel('''
            ...    shift_detectors 1000
            ...    error(0.1) D0 D199
            ... ''').num_detectors
            1200
        """
    @property
    def num_errors(
        self,
    ) -> int:
        """Counts the number of errors (e.g. `error(0.1) D0`) in the error model.

        Error instructions inside repeat blocks count once per repetition.
        Redundant errors with the same targets count as separate errors.

        Examples:
            >>> import stim

            >>> stim.DetectorErrorModel('''
            ...     error(0.125) D0
            ...     repeat 100 {
            ...         repeat 5 {
            ...             error(0.25) D1
            ...         }
            ...     }
            ... ''').num_errors
            501
        """
    @property
    def num_observables(
        self,
    ) -> int:
        """Counts the number of frame changes (e.g. `L2`) in the error model.

        Observable indices are assumed to be contiguous from 0 up to whatever the maximum observable id is.
        If the largest observable's id is n-1, then the number of observables is n.

        Examples:
            >>> import stim

            >>> stim.Circuit('''
            ...     X_ERROR(0.125) 0
            ...     M 0
            ...     OBSERVABLE_INCLUDE(99) rec[-1]
            ... ''').detector_error_model().num_observables
            100

            >>> stim.DetectorErrorModel('''
            ...    error(0.1) L399
            ... ''').num_observables
            400
        """
    def rounded(
        self,
        arg0: int,
    ) -> stim.DetectorErrorModel:
        """Creates an equivalent detector error model but with rounded error probabilities.

        Args:
            digits: The number of digits to round to.

        Returns:
            A `stim.DetectorErrorModel` with the same instructions in the same order,
            but with the parens arguments of error instructions rounded to the given
            precision.

            Instructions whose error probability was rounded to zero are still
            included in the output.

        Examples:
            >>> import stim
            >>> dem = stim.DetectorErrorModel('''
            ...     error(0.019499) D0
            ...     error(0.000001) D0 D1
            ... ''')

            >>> dem.rounded(2)
            stim.DetectorErrorModel('''
                error(0.02) D0
                error(0) D0 D1
            ''')

            >>> dem.rounded(3)
            stim.DetectorErrorModel('''
                error(0.019) D0
                error(0) D0 D1
            ''')
        """
    def shortest_graphlike_error(
        self,
        ignore_ungraphlike_errors: bool = False,
    ) -> stim.DetectorErrorModel:
        """Finds a minimum sized set of graphlike errors that produce an undetected logical error.

        Note that this method does not pay attention to error probabilities (other than ignoring errors with
        probability 0). It searches for a logical error with the minimum *number* of physical errors, not the
        maximum probability of those physical errors all occurring.

        This method works by looking for errors that have frame changes (eg. "error(0.1) D0 D1 L5" flips the frame
        of observable 5). These errors are converted into one or two symptoms and a net frame change. The symptoms
        can then be moved around by following errors touching that symptom. Each symptom is moved until it
        disappears into a boundary or cancels against another remaining symptom, while leaving the other symptoms
        alone (ensuring only one symptom is allowed to move significantly reduces waste in the search space).
        Eventually a path or cycle of errors is found that cancels out the symptoms, and if there is still a frame
        change at that point then that path or cycle is a logical error (otherwise all that was found was a
        stabilizer of the system; a dead end). The search process advances like a breadth first search, seeded from
        all the frame-change errors and branching them outward in tandem, until one of them wins the race to find a
        solution.

        Args:
            ignore_ungraphlike_errors: Defaults to False. When False, an exception is raised if there are any
                errors in the model that are not graphlike. When True, those errors are skipped as if they weren't
                present.

                A graphlike error is an error with at most two symptoms per decomposed component.
                    graphlike:
                        error(0.1) D0
                        error(0.1) D0 D1
                        error(0.1) D0 D1 L0
                        error(0.1) D0 D1 ^ D2
                    not graphlike:
                        error(0.1) D0 D1 D2
                        error(0.1) D0 D1 D2 ^ D3

        Returns:
            A detector error model containing just the error instructions corresponding to an undetectable logical
            error. There will be no other kinds of instructions (no `repeat`s, no `shift_detectors`, etc).
            The error probabilities will all be set to 1.

            The `len` of the returned model is the graphlike code distance of the circuit. But beware that in
            general the true code distance may be smaller. For example, in the XZ surface code with twists, the true
            minimum sized logical error is likely to use Y errors. But each Y error decomposes into two graphlike
            components (the X part and the Z part). As a result, the graphlike code distance in that context is
            likely to be nearly twice as large as the true code distance.

        Examples:
            >>> import stim

            >>> stim.DetectorErrorModel('''
            ...     error(0.125) D0
            ...     error(0.125) D0 D1
            ...     error(0.125) D1 L55
            ...     error(0.125) D1
            ... ''').shortest_graphlike_error()
            stim.DetectorErrorModel('''
                error(1) D1
                error(1) D1 L55
            ''')

            >>> stim.DetectorErrorModel('''
            ...     error(0.125) D0 D1 D2
            ...     error(0.125) L0
            ... ''').shortest_graphlike_error(ignore_ungraphlike_errors=True)
            stim.DetectorErrorModel('''
                error(1) L0
            ''')

            >>> circuit = stim.Circuit.generated(
            ...     "repetition_code:memory",
            ...     rounds=10,
            ...     distance=7,
            ...     before_round_data_depolarization=0.01)
            >>> model = circuit.detector_error_model(decompose_errors=True)
            >>> len(model.shortest_graphlike_error())
            7
        """
    def to_file(
        self,
        file: Union[io.TextIOBase, str, pathlib.Path],
    ) -> None:

        """Writes the detector error model to a file.

        The file format is defined at https://github.com/quantumlib/Stim/blob/main/doc/file_format_dem_detector_error_model.md

        Args:
            file: A file path or an open file to write to.

        Examples:
            >>> import stim
            >>> import tempfile
            >>> c = stim.DetectorErrorModel('error(0.25) D2 D3')

            >>> with tempfile.TemporaryDirectory() as tmpdir:
            ...     path = tmpdir + '/tmp.stim'
            ...     with open(path, 'w') as f:
            ...         c.to_file(f)
            ...     with open(path) as f:
            ...         contents = f.read()
            >>> contents
            'error(0.25) D2 D3\n'

            >>> with tempfile.TemporaryDirectory() as tmpdir:
            ...     path = tmpdir + '/tmp.stim'
            ...     c.to_file(path)
            ...     with open(path) as f:
            ...         contents = f.read()
            >>> contents
            'error(0.25) D2 D3\n'
        """
class ExplainedError:
    """Describes the location of an error mechanism from a stim circuit.
    """
    def __init__(
        self,
        *,
        dem_error_terms: List[stim.DemTargetWithCoords],
        circuit_error_locations: List[stim.CircuitErrorLocation],
    ) -> None:
        """Creates a stim.ExplainedError.
        """
    @property
    def circuit_error_locations(
        self,
    ) -> List[stim.CircuitErrorLocation]:
        """The locations of circuit errors that produce the symptoms in dem_error_terms.

        Note: if this list contains a single entry, it may be because a result
        with a single representative error was requested (as opposed to all possible
        errors).

        Note: if this list is empty, it may be because there was a DEM error decomposed
        into parts where one of the parts is impossible to make on its own from a single
        circuit error.
        """
    @property
    def dem_error_terms(
        self,
    ) -> List[stim.DemTargetWithCoords]:
        """The detectors and observables flipped by this error mechanism.
        """
class FlippedMeasurement:
    """Describes a measurement that was flipped.

    Gives the measurement's index in the measurement record, and also
    the observable of the measurement.
    """
    def __init__(
        self,
        *,
        record_index: int,
        observable: object,
    ) -> None:
        """Creates a stim.FlippedMeasurement.
        """
    @property
    def observable(
        self,
    ) -> List[stim.GateTargetWithCoords]:
        """Returns the observable of the flipped measurement.

        For example, an `MX 5` measurement will have the observable X5.
        """
    @property
    def record_index(
        self,
    ) -> int:
        """The measurement record index of the flipped measurement.
        For example, the fifth measurement in a circuit has a measurement
        record index of 4.
        """
class GateTarget:
    """Represents a gate target, like `0` or `rec[-1]`, from a circuit.

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit('''
        ...     M 0 !1
        ... ''')
        >>> circuit[0].targets_copy()[0]
        stim.GateTarget(0)
        >>> circuit[0].targets_copy()[1]
        stim.GateTarget(stim.target_inv(1))
    """
    def __eq__(
        self,
        arg0: stim.GateTarget,
    ) -> bool:
        """Determines if two `stim.GateTarget`s are identical.
        """
    def __init__(
        self,
        value: object,
    ) -> None:
        """Initializes a `stim.GateTarget`.

        Args:
            value: A target like `5` or `stim.target_rec(-1)`.
        """
    def __ne__(
        self,
        arg0: stim.GateTarget,
    ) -> bool:
        """Determines if two `stim.GateTarget`s are different.
        """
    def __repr__(
        self,
    ) -> str:
        """Returns text that is a valid python expression evaluating to an equivalent `stim.GateTarget`.
        """
    @property
    def is_combiner(
        self,
    ) -> bool:
        """Returns whether or not this is a `stim.target_combiner()` (a `*` in a circuit file).
        """
    @property
    def is_inverted_result_target(
        self,
    ) -> bool:
        """Returns whether or not this is an inverted target.

        Inverted targets include inverted qubit targets `stim.target_inv(5)` (`!5` in a circuit file) and
        inverted Pauli targets like `stim.target_x(4, invert=True)` (`!X4` in a circuit file).
        """
    @property
    def is_measurement_record_target(
        self,
    ) -> bool:
        """Returns whether or not this is a `stim.target_rec` target (e.g. `rec[-5]` in a circuit file).
        """
    @property
    def is_qubit_target(
        self,
    ) -> bool:
        """Returns true if this is a qubit target (e.g. `5`) or an inverted qubit target (e.g. `stim.target_inv(4)`).
        """
    @property
    def is_sweep_bit_target(
        self,
    ) -> bool:
        """Returns whether or not this is a `stim.target_sweep_bit` target (e.g. `sweep[5]` in a circuit file).
        """
    @property
    def is_x_target(
        self,
    ) -> bool:
        """Returns whether or not this is a `stim.target_x` target (e.g. `X5` in a circuit file).
        """
    @property
    def is_y_target(
        self,
    ) -> bool:
        """Returns whether or not this is a `stim.target_y` target (e.g. `Y5` in a circuit file).
        """
    @property
    def is_z_target(
        self,
    ) -> bool:
        """Returns whether or not this is a `stim.target_z` target (e.g. `Z5` in a circuit file).
        """
    @property
    def value(
        self,
    ) -> int:
        """The numeric part of the target. Positive for qubit targets, negative for measurement record targets.
        """
class GateTargetWithCoords:
    """A gate target with associated coordinate information.

    For example, if the gate target is a qubit from a circuit with
    QUBIT_COORDS instructions, the coords field will contain the
    coordinate data from the QUBIT_COORDS instruction for the qubit.

    This is helpful information to have available when debugging a
    problem in a circuit, instead of having to constantly manually
    look up the coordinates of a qubit index in order to understand
    what is happening.
    """
    def __init__(
        self,
        *,
        gate_target: object,
        coords: List[float],
    ) -> None:
        """Creates a stim.GateTargetWithCoords.
        """
    @property
    def coords(
        self,
    ) -> List[float]:
        """Returns the associated coordinate information as a list of flaots.

        If there is no coordinate information, returns an empty list.
        """
    @property
    def gate_target(
        self,
    ) -> stim.GateTarget:
        """Returns the actual gate target as a `stim.GateTarget`.
        """
class PauliString:
    """A signed Pauli tensor product (e.g. "+X \u2297 X \u2297 X" or "-Y \u2297 Z".

    Represents a collection of Pauli operations (I, X, Y, Z) applied pairwise to a collection of qubits.

    Examples:
        >>> import stim
        >>> stim.PauliString("XX") * stim.PauliString("YY")
        stim.PauliString("-ZZ")
        >>> print(stim.PauliString(5))
        +_____
    """
    def __add__(
        self,
        rhs: stim.PauliString,
    ) -> stim.PauliString:
        """Returns the tensor product of two Pauli strings.

        Concatenates the Pauli strings and multiplies their signs.

        Args:
            rhs: A second stim.PauliString.

        Examples:
            >>> import stim

            >>> stim.PauliString("X") + stim.PauliString("YZ")
            stim.PauliString("+XYZ")

            >>> stim.PauliString("iX") + stim.PauliString("-X")
            stim.PauliString("-iXX")

        Returns:
            The tensor product.
        """
    def __eq__(
        self,
        arg0: stim.PauliString,
    ) -> bool:
        """Determines if two Pauli strings have identical contents.
        """
    @overload
    def __getitem__(
        self,
        index_or_slice: int,
    ) -> int:
        pass
    @overload
    def __getitem__(
        self,
        index_or_slice: slice,
    ) -> stim.PauliString:
        pass
    def __getitem__(
        self,
        index_or_slice: object,
    ) -> object:
        """Returns an individual Pauli or Pauli string slice from the pauli string.

        Individual Paulis are returned as an int using the encoding 0=I, 1=X, 2=Y, 3=Z.
        Slices are returned as a stim.PauliString (always with positive sign).

        Examples:
            >>> import stim
            >>> p = stim.PauliString("_XYZ")
            >>> p[2]
            2
            >>> p[-1]
            3
            >>> p[:2]
            stim.PauliString("+_X")
            >>> p[::-1]
            stim.PauliString("+ZYX_")

        Args:
            index_or_slice: The index of the pauli to return, or the slice of paulis to return.

        Returns:
            0: Identity.
            1: Pauli X.
            2: Pauli Y.
            3: Pauli Z.
        """
    def __iadd__(
        self,
        rhs: stim.PauliString,
    ) -> stim.PauliString:
        """Performs an inplace tensor product.

        Concatenates the given Pauli string onto the receiving string and multiplies their signs.

        Args:
            rhs: A second stim.PauliString.

        Examples:
            >>> import stim

            >>> p = stim.PauliString("iX")
            >>> alias = p
            >>> p += stim.PauliString("-YY")
            >>> p
            stim.PauliString("-iXYY")
            >>> alias is p
            True

        Returns:
            The mutated pauli string.
        """
    def __imul__(
        self,
        rhs: object,
    ) -> stim.PauliString:
        """Inplace right-multiplies the Pauli string by another Pauli string, a complex unit, or a tensor power.

        Args:
            rhs: The right hand side of the multiplication. This can be:
                - A stim.PauliString to right-multiply term-by-term into the paulis of the pauli string.
                - A complex unit (1, -1, 1j, -1j) to multiply into the sign of the pauli string.
                - A non-negative integer indicating the tensor power to raise the pauli string to (how many times to repeat it).

        Examples:
            >>> import stim

            >>> p = stim.PauliString("X")
            >>> p *= 1j
            >>> p
            stim.PauliString("+iX")

            >>> p = stim.PauliString("iXY_")
            >>> p *= 3
            >>> p
            stim.PauliString("-iXY_XY_XY_")

            >>> p = stim.PauliString("X")
            >>> alias = p
            >>> p *= stim.PauliString("Y")
            >>> alias
            stim.PauliString("+iZ")

            >>> p = stim.PauliString("X")
            >>> p *= stim.PauliString("_YY")
            >>> p
            stim.PauliString("+XYY")

        Returns:
            The mutated Pauli string.
        """
    @staticmethod
    def __init__(
        *args,
        **kwargs,
    ):
        """Overloaded function.

        1. __init__(self: stim.PauliString, num_qubits: int) -> None

        Creates an identity Pauli string over the given number of qubits.

        Examples:
            >>> import stim
            >>> p = stim.PauliString(5)
            >>> print(p)
            +_____

        Args:
            num_qubits: The number of qubits the Pauli string acts on.


        2. __init__(self: stim.PauliString, text: str) -> None

        Creates a stim.PauliString from a text string.

        The string can optionally start with a sign ('+', '-', 'i', '+i', or '-i').
        The rest of the string should be characters from '_IXYZ' where
        '_' and 'I' mean identity, 'X' means Pauli X, 'Y' means Pauli Y, and 'Z' means Pauli Z.

        Examples:
            >>> import stim
            >>> print(stim.PauliString("YZ"))
            +YZ
            >>> print(stim.PauliString("+IXYZ"))
            +_XYZ
            >>> print(stim.PauliString("-___X_"))
            -___X_
            >>> print(stim.PauliString("iX"))
            +iX

        Args:
            text: A text description of the Pauli string's contents, such as "+XXX" or "-_YX".


        3. __init__(self: stim.PauliString, copy: stim.PauliString) -> None

        Creates a copy of a stim.PauliString.

        Examples:
            >>> import stim
            >>> a = stim.PauliString("YZ")
            >>> b = stim.PauliString(a)
            >>> b is a
            False
            >>> b == a
            True

        Args:
            copy: The pauli string to make a copy of.


        4. __init__(self: stim.PauliString, pauli_indices: List[int]) -> None

        Creates a stim.PauliString from a list of integer pauli indices.

        The indexing scheme that is used is:
            0 -> I
            1 -> X
            2 -> Y
            3 -> Z

        Examples:
            >>> import stim
            >>> stim.PauliString([0, 1, 2, 3, 0, 3])
            stim.PauliString("+_XYZ_Z")

        Args:
            pauli_indices: A sequence of integers from 0 to 3 (inclusive) indicating paulis.
        """
    def __itruediv__(
        self,
        rhs: complex,
    ) -> stim.PauliString:
        """Inplace divides the Pauli string by a complex unit.

        Args:
            rhs: The divisor. Can be 1, -1, 1j, or -1j.

        Examples:
            >>> import stim

            >>> p = stim.PauliString("X")
            >>> p /= 1j
            >>> p
            stim.PauliString("-iX")

        Returns:
            The mutated Pauli string.

        Raises:
            ValueError: The divisor isn't 1, -1, 1j, or -1j.
        """
    def __len__(
        self,
    ) -> int:
        """Returns the length the pauli string; the number of qubits it operates on.
        """
    def __mul__(
        self,
        rhs: object,
    ) -> stim.PauliString:
        """Right-multiplies the Pauli string by another Pauli string, a complex unit, or a tensor power.

        Args:
            rhs: The right hand side of the multiplication. This can be:
                - A stim.PauliString to right-multiply term-by-term with the paulis of the pauli string.
                - A complex unit (1, -1, 1j, -1j) to multiply with the sign of the pauli string.
                - A non-negative integer indicating the tensor power to raise the pauli string to (how many times to repeat it).

        Examples:
            >>> import stim

            >>> stim.PauliString("X") * 1
            stim.PauliString("+X")
            >>> stim.PauliString("X") * -1
            stim.PauliString("-X")
            >>> stim.PauliString("X") * 1j
            stim.PauliString("+iX")

            >>> stim.PauliString("X") * 2
            stim.PauliString("+XX")
            >>> stim.PauliString("-X") * 2
            stim.PauliString("+XX")
            >>> stim.PauliString("iX") * 2
            stim.PauliString("-XX")
            >>> stim.PauliString("X") * 3
            stim.PauliString("+XXX")
            >>> stim.PauliString("iX") * 3
            stim.PauliString("-iXXX")

            >>> stim.PauliString("X") * stim.PauliString("Y")
            stim.PauliString("+iZ")
            >>> stim.PauliString("X") * stim.PauliString("XX_")
            stim.PauliString("+_X_")
            >>> stim.PauliString("XXXX") * stim.PauliString("_XYZ")
            stim.PauliString("+X_ZY")

        Returns:
            The product or tensor power.

        Raises:
            TypeError: The right hand side isn't a stim.PauliString, a non-negative integer, or a complex unit (1, -1, 1j, or -1j).
        """
    def __ne__(
        self,
        arg0: stim.PauliString,
    ) -> bool:
        """Determines if two Pauli strings have non-identical contents.
        """
    def __neg__(
        self,
    ) -> stim.PauliString:
        """Returns the negation of the pauli string.

        Examples:
            >>> import stim
            >>> -stim.PauliString("X")
            stim.PauliString("-X")
            >>> -stim.PauliString("-Y")
            stim.PauliString("+Y")
            >>> -stim.PauliString("iZZZ")
            stim.PauliString("-iZZZ")
        """
    def __pos__(
        self,
    ) -> stim.PauliString:
        """Returns a pauli string with the same contents.

        Examples:
            >>> import stim
            >>> +stim.PauliString("+X")
            stim.PauliString("+X")
            >>> +stim.PauliString("-YY")
            stim.PauliString("-YY")
            >>> +stim.PauliString("iZZZ")
            stim.PauliString("+iZZZ")
        """
    def __repr__(
        self,
    ) -> str:
        """Returns text that is a valid python expression evaluating to an equivalent `stim.PauliString`.
        """
    def __rmul__(
        self,
        lhs: object,
    ) -> stim.PauliString:
        """Left-multiplies the Pauli string by another Pauli string, a complex unit, or a tensor power.

        Args:
            lhs: The left hand side of the multiplication. This can be:
                - A stim.PauliString to left-multiply term-by-term into the paulis of the pauli string.
                - A complex unit (1, -1, 1j, -1j) to multiply into the sign of the pauli string.
                - A non-negative integer indicating the tensor power to raise the pauli string to (how many times to repeat it).

        Examples:
            >>> import stim

            >>> 1 * stim.PauliString("X")
            stim.PauliString("+X")
            >>> -1 * stim.PauliString("X")
            stim.PauliString("-X")
            >>> 1j * stim.PauliString("X")
            stim.PauliString("+iX")

            >>> 2 * stim.PauliString("X")
            stim.PauliString("+XX")
            >>> 2 * stim.PauliString("-X")
            stim.PauliString("+XX")
            >>> 2 * stim.PauliString("iX")
            stim.PauliString("-XX")
            >>> 3 * stim.PauliString("X")
            stim.PauliString("+XXX")
            >>> 3 * stim.PauliString("iX")
            stim.PauliString("-iXXX")

            >>> stim.PauliString("X") * stim.PauliString("Y")
            stim.PauliString("+iZ")
            >>> stim.PauliString("X") * stim.PauliString("XX_")
            stim.PauliString("+_X_")
            >>> stim.PauliString("XXXX") * stim.PauliString("_XYZ")
            stim.PauliString("+X_ZY")

        Returns:
            The product.

        Raises:
            ValueError: The scalar phase factor isn't 1, -1, 1j, or -1j.
        """
    def __setitem__(
        self,
        index: int,
        new_pauli: object,
    ) -> None:
        """Mutates an entry in the pauli string using the encoding 0=I, 1=X, 2=Y, 3=Z.

        Args:
            index: The index of the pauli to overwrite.
            new_pauli: Either a character from '_IXYZ' or an integer from range(4).

        Examples:
            >>> import stim
            >>> p = stim.PauliString(4)
            >>> p[2] = 1
            >>> print(p)
            +__X_
            >>> p[0] = 3
            >>> p[1] = 2
            >>> p[3] = 0
            >>> print(p)
            +ZYX_
            >>> p[0] = 'I'
            >>> p[1] = 'X'
            >>> p[2] = 'Y'
            >>> p[3] = 'Z'
            >>> print(p)
            +_XYZ
            >>> p[-1] = 'Y'
            >>> print(p)
            +_XYY
        """
    def __str__(
        self,
    ) -> str:
        """Returns a text description.
        """
    def __truediv__(
        self,
        rhs: complex,
    ) -> stim.PauliString:
        """Divides the Pauli string by a complex unit.

        Args:
            rhs: The divisor. Can be 1, -1, 1j, or -1j.

        Examples:
            >>> import stim

            >>> stim.PauliString("X") / 1j
            stim.PauliString("-iX")

        Returns:
            The quotient.

        Raises:
            ValueError: The divisor isn't 1, -1, 1j, or -1j.
        """
    def commutes(
        self,
        other: stim.PauliString,
    ) -> bool:
        """Determines if two Pauli strings commute or not.

        Two Pauli strings commute if they have an even number of matched
        non-equal non-identity Pauli terms. Otherwise they anticommute.

        Args:
            other: The other Pauli string.

        Examples:
            >>> import stim
            >>> xx = stim.PauliString("XX")
            >>> xx.commutes(stim.PauliString("X_"))
            True
            >>> xx.commutes(stim.PauliString("XX"))
            True
            >>> xx.commutes(stim.PauliString("XY"))
            False
            >>> xx.commutes(stim.PauliString("XZ"))
            False
            >>> xx.commutes(stim.PauliString("ZZ"))
            True
            >>> xx.commutes(stim.PauliString("X_Y__"))
            True
            >>> xx.commutes(stim.PauliString(""))
            True

        Returns:
            True if the Pauli strings commute, False if they anti-commute.
        """
    def copy(
        self,
    ) -> stim.PauliString:
        """Returns a copy of the pauli string. An independent pauli string with the same contents.

        Examples:
            >>> import stim
            >>> p1 = stim.PauliString.random(2)
            >>> p2 = p1.copy()
            >>> p2 is p1
            False
            >>> p2 == p1
            True
        """
    def extended_product(
        self,
        other: stim.PauliString,
    ) -> Tuple[complex, stim.PauliString]:
        """[DEPRECATED] Use multiplication (__mul__ or *) instead.
        """
    @staticmethod
    def random(
        num_qubits: int,
        *,
        allow_imaginary: bool = False,
    ) -> stim.PauliString:
        """Samples a uniformly random Hermitian Pauli string over the given number of qubits.

        Args:
            num_qubits: The number of qubits the Pauli string should act on.
            allow_imaginary: Defaults to False. If True, the sign of the result may be 1j or -1j
                in addition to +1 or -1. In other words, setting this to True allows the result
                to be non-Hermitian.

        Examples:
            >>> import stim
            >>> p = stim.PauliString.random(5)
            >>> len(p)
            5
            >>> p.sign in [-1, +1]
            True

            >>> p2 = stim.PauliString.random(3, allow_imaginary=True)
            >>> len(p2)
            3
            >>> p2.sign in [-1, +1, 1j, -1j]
            True

        Returns:
            The sampled Pauli string.
        """
    @property
    def sign(
        self,
    ) -> complex:
        """The sign of the Pauli string. Can be +1, -1, 1j, or -1j.

        Examples:
            >>> import stim
            >>> stim.PauliString("X").sign
            (1+0j)
            >>> stim.PauliString("-X").sign
            (-1+0j)
            >>> stim.PauliString("iX").sign
            1j
            >>> stim.PauliString("-iX").sign
            (-0-1j)
        """
    @sign.setter
    def sign(self, value: complex):
        pass
class Tableau:
    """A stabilizer tableau.

    Represents a Clifford operation by explicitly storing how that operation conjugates a list of Pauli
    group generators into composite Pauli products.

    Examples:
        >>> import stim
        >>> stim.Tableau.from_named_gate("H")
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+Z"),
            ],
            zs=[
                stim.PauliString("+X"),
            ],
        )

        >>> t = stim.Tableau.random(5)
        >>> t_inv = t**-1
        >>> print(t * t_inv)
        +-xz-xz-xz-xz-xz-
        | ++ ++ ++ ++ ++
        | XZ __ __ __ __
        | __ XZ __ __ __
        | __ __ XZ __ __
        | __ __ __ XZ __
        | __ __ __ __ XZ

        >>> x2z3 = t.x_output(2) * t.z_output(3)
        >>> t_inv(x2z3)
        stim.PauliString("+__XZ_")
    """
    def __add__(
        self,
        rhs: stim.Tableau,
    ) -> stim.Tableau:
        """Returns the direct sum (diagonal concatenation) of two Tableaus.

        Args:
            rhs: A second stim.Tableau.

        Examples:
            >>> import stim

            >>> s = stim.Tableau.from_named_gate("S")
            >>> cz = stim.Tableau.from_named_gate("CZ")
            >>> print(s + cz)
            +-xz-xz-xz-
            | ++ ++ ++
            | YZ __ __
            | __ XZ Z_
            | __ Z_ XZ

        Returns:
            The direct sum.
        """
    def __call__(
        self,
        pauli_string: stim.PauliString,
    ) -> stim.PauliString:
        """Returns the result of conjugating the given PauliString by the Tableau's Clifford operation.

        Args:
            pauli_string: The pauli string to conjugate.

        Returns:
            The new conjugated pauli string.

        Examples:
            >>> import stim
            >>> t = stim.Tableau.from_named_gate("CNOT")
            >>> p = stim.PauliString("XX")
            >>> result = t(p)
            >>> print(result)
            +X_
        """
    def __eq__(
        self,
        arg0: stim.Tableau,
    ) -> bool:
        """Determines if two tableaus have identical contents.
        """
    def __iadd__(
        self,
        rhs: stim.Tableau,
    ) -> stim.Tableau:
        """Performs an inplace direct sum (diagonal concatenation).

        Args:
            rhs: A second stim.Tableau.

        Examples:
            >>> import stim

            >>> s = stim.Tableau.from_named_gate("S")
            >>> cz = stim.Tableau.from_named_gate("CZ")
            >>> alias = s
            >>> s += cz
            >>> alias is s
            True
            >>> print(s)
            +-xz-xz-xz-
            | ++ ++ ++
            | YZ __ __
            | __ XZ Z_
            | __ Z_ XZ

        Returns:
            The mutated tableau.
        """
    def __init__(
        self,
        num_qubits: int,
    ) -> None:
        """Creates an identity tableau over the given number of qubits.

        Examples:
            >>> import stim
            >>> t = stim.Tableau(3)
            >>> print(t)
            +-xz-xz-xz-
            | ++ ++ ++
            | XZ __ __
            | __ XZ __
            | __ __ XZ

        Args:
            num_qubits: The number of qubits the tableau's operation acts on.
        """
    def __len__(
        self,
    ) -> int:
        """Returns the number of qubits operated on by the tableau.
        """
    def __mul__(
        self,
        rhs: stim.Tableau,
    ) -> stim.Tableau:
        """Returns the product of two tableaus.

        If the tableau T1 represents the Clifford operation with unitary C1,
        and the tableau T2 represents the Clifford operation with unitary C2,
        then the tableau T1*T2 represents the Clifford operation with unitary C1*C2.

        Args:
            rhs: The tableau  on the right hand side of the multiplication.

        Examples:
            >>> import stim
            >>> t1 = stim.Tableau.random(4)
            >>> t2 = stim.Tableau.random(4)
            >>> t3 = t2 * t1
            >>> p = stim.PauliString.random(4)
            >>> t3(p) == t2(t1(p))
            True
        """
    def __ne__(
        self,
        arg0: stim.Tableau,
    ) -> bool:
        """Determines if two tableaus have non-identical contents.
        """
    def __pow__(
        self,
        exponent: int,
    ) -> stim.Tableau:
        """Raises the tableau to an integer power.

        Large powers are reached efficiently using repeated squaring.
        Negative powers are reached by inverting the tableau.

        Args:
            exponent: The power to raise to. Can be negative, zero, or positive.

        Examples:
            >>> import stim
            >>> s = stim.Tableau.from_named_gate("S")
            >>> s**0 == stim.Tableau(1)
            True
            >>> s**1 == s
            True
            >>> s**2 == stim.Tableau.from_named_gate("Z")
            True
            >>> s**-1 == s**3 == stim.Tableau.from_named_gate("S_DAG")
            True
            >>> s**5 == s
            True
            >>> s**(400000000 + 1) == s
            True
            >>> s**(-400000000 + 1) == s
            True
        """
    def __repr__(
        self,
    ) -> str:
        """Returns text that is a valid python expression evaluating to an equivalent `stim.Tableau`.
        """
    def __str__(
        self,
    ) -> str:
        """Returns a text description.
        """
    def append(
        self,
        gate: stim.Tableau,
        targets: List[int],
    ) -> None:
        """Appends an operation's effect into this tableau, mutating this tableau.

        Time cost is O(n*m*m) where n=len(self) and m=len(gate).

        Args:
            gate: The tableau of the operation being appended into this tableau.
            targets: The qubits being targeted by the gate.

        Examples:
            >>> import stim
            >>> cnot = stim.Tableau.from_named_gate("CNOT")
            >>> t = stim.Tableau(2)
            >>> t.append(cnot, [0, 1])
            >>> t.append(cnot, [1, 0])
            >>> t.append(cnot, [0, 1])
            >>> t == stim.Tableau.from_named_gate("SWAP")
            True
        """
    def copy(
        self,
    ) -> stim.Tableau:
        """Returns a copy of the tableau. An independent tableau with the same contents.

        Examples:
            >>> import stim
            >>> t1 = stim.Tableau.random(2)
            >>> t2 = t1.copy()
            >>> t2 is t1
            False
            >>> t2 == t1
            True
        """
    @staticmethod
    def from_circuit(
        circuit: stim.Circuit,
        *,
        ignore_noise: bool = False,
        ignore_measurement: bool = False,
        ignore_reset: bool = False,
    ) -> stim.Tableau:

        """Converts a circuit into an equivalent stabilizer tableau.

        Args:
            circuit: The circuit to compile into a tableau.
            ignore_noise: Defaults to False. When False, any noise operations in the circuit will cause
                the conversion to fail with an exception. When True, noise operations are skipped over
                as if they weren't even present in the circuit.
            ignore_measurement: Defaults to False. When False, any measurement operations in the circuit
                will cause the conversion to fail with an exception. When True, measurement operations are
                skipped over as if they weren't even present in the circuit.
            ignore_reset: Defaults to False. When False, any reset operations in the circuit will cause
                the conversion to fail with an exception. When True, reset operations are skipped over
                as if they weren't even present in the circuit.

        Returns:
            The tableau equivalent to the given circuit (up to global phase).

        Raises:
            ValueError:
                The circuit contains noise operations but ignore_noise=False.
                OR
                The circuit contains measurement operations but ignore_measurement=False.
                OR
                The circuit contains reset operations but ignore_reset=False.

        Examples:
            >>> import stim
            >>> stim.Tableau.from_circuit(stim.Circuit("""
            ...     H 0
            ...     CNOT 0 1
            ... """))
            stim.Tableau.from_conjugated_generators(
                xs=[
                    stim.PauliString("+Z_"),
                    stim.PauliString("+_X"),
                ],
                zs=[
                    stim.PauliString("+XX"),
                    stim.PauliString("+ZZ"),
                ],
            )
        """
    @staticmethod
    def from_conjugated_generators(
        *,
        xs: List[stim.PauliString],
        zs: List[stim.PauliString],
    ) -> stim.Tableau:
        """Creates a tableau from the given outputs for each generator.

        Verifies that the tableau is well formed.

        Args:
            xs: A List[stim.PauliString] with the results of conjugating X0, X1, etc.
            zs: A List[stim.PauliString] with the results of conjugating Z0, Z1, etc.

        Returns:
            The created tableau.

        Raises:
            ValueError: The given outputs are malformed. Their lengths are inconsistent,
                or they don't satisfy the required commutation relationships.

        Examples:
            >>> import stim
            >>> identity3 = stim.Tableau.from_conjugated_generators(
            ...     xs=[
            ...         stim.PauliString("X__"),
            ...         stim.PauliString("_X_"),
            ...         stim.PauliString("__X"),
            ...     ],
            ...     zs=[
            ...         stim.PauliString("Z__"),
            ...         stim.PauliString("_Z_"),
            ...         stim.PauliString("__Z"),
            ...     ],
            ... )
            >>> identity3 == stim.Tableau(3)
            True
        """
    @staticmethod
    def from_named_gate(
        name: str,
    ) -> stim.Tableau:
        """Returns the tableau of a named Clifford gate.

        Args:
            name: The name of the Clifford gate.

        Returns:
            The gate's tableau.

        Examples:
            >>> import stim
            >>> print(stim.Tableau.from_named_gate("H"))
            +-xz-
            | ++
            | ZX
            >>> print(stim.Tableau.from_named_gate("CNOT"))
            +-xz-xz-
            | ++ ++
            | XZ _Z
            | X_ XZ
            >>> print(stim.Tableau.from_named_gate("S"))
            +-xz-
            | ++
            | YZ
        """
    @staticmethod
    def from_unitary_matrix(
        matrix: Iterable[Iterable[float]],
        *,
        endian: str = 'little',
    ) -> stim.Tableau:

        """Creates a tableau from the unitary matrix of a Clifford operation.

        Args:
            matrix: A unitary matrix specified as an iterable of rows, with each row is an iterable of amplitudes.
                The unitary matrix must correspond to a Clifford operation.
            endian:
                "little": matrix entries are in little endian order, where higher index qubits
                    correspond to larger changes in row/col indices.
                "big": matrix entries are in little endian order, where higher index qubits correspond to
                    smaller changes in row/col indices.
        Returns:
            The tableau equivalent to the given unitary matrix (up to global phase).

        Raises:
            ValueError: The given matrix isn't the unitary matrix of a Clifford operation.

        Examples:
            >>> import stim
            >>> stim.Tableau.from_unitary_matrix([
            ...     [1, 0],
            ...     [0, 1j],
            ... ], endian='little')
            stim.Tableau.from_conjugated_generators(
                xs=[
                    stim.PauliString("+Y"),
                ],
                zs=[
                    stim.PauliString("+Z"),
                ],
            )

            >>> stim.Tableau.from_unitary_matrix([
            ...     [1, 0, 0, 0],
            ...     [0, 1, 0, 0],
            ...     [0, 0, 0, -1j],
            ...     [0, 0, 1j, 0],
            ... ], endian='little')
            stim.Tableau.from_conjugated_generators(
                xs=[
                    stim.PauliString("+XZ"),
                    stim.PauliString("+YX"),
                ],
                zs=[
                    stim.PauliString("+ZZ"),
                    stim.PauliString("+_Z"),
                ],
            )
        """
    def inverse(
        self,
        *,
        unsigned: bool = False,
    ) -> stim.Tableau:
        """Computes the inverse of the tableau.

        The inverse T^-1 of a tableau T is the unique tableau with the property that T * T^-1 = T^-1 * T = I where
        I is the identity tableau.

        Args:
            unsigned: Defaults to false. When set to true, skips computing the signs of the output observables and
                instead just set them all to be positive. This is beneficial because computing the signs takes
                O(n^3) time and the rest of the inverse computation is O(n^2) where n is the number of qubits in the
                tableau. So, if you only need the Pauli terms (not the signs), it is significantly cheaper.

        Returns:
            The inverse tableau.

        Examples:
            >>> import stim

            >>> # Check that the inverse agrees with hard-coded tableaus in the gate data.
            >>> s = stim.Tableau.from_named_gate("S")
            >>> s_dag = stim.Tableau.from_named_gate("S_DAG")
            >>> s.inverse() == s_dag
            True
            >>> z = stim.Tableau.from_named_gate("Z")
            >>> z.inverse() == z
            True

            >>> # Check that multiplying by the inverse produces the identity.
            >>> t = stim.Tableau.random(10)
            >>> t_inv = t.inverse()
            >>> identity = stim.Tableau(10)
            >>> t * t_inv == t_inv * t == identity
            True

            >>> # Check a manual case.
            >>> t = stim.Tableau.from_conjugated_generators(
            ...     xs=[stim.PauliString("-__Z"), stim.PauliString("+XZ_"), stim.PauliString("+_ZZ")],
            ...     zs=[stim.PauliString("-YYY"), stim.PauliString("+Z_Z"), stim.PauliString("-ZYZ")],
            ... )
            >>> print(t.inverse())
            +-xz-xz-xz-
            | -- +- --
            | XX XX YX
            | XZ Z_ X_
            | X_ YX Y_
            >>> print(t.inverse(unsigned=True))
            +-xz-xz-xz-
            | ++ ++ ++
            | XX XX YX
            | XZ Z_ X_
            | X_ YX Y_
        """
    def inverse_x_output(
        self,
        input_index: int,
        *,
        unsigned: bool = False,
    ) -> stim.PauliString:
        """Returns the result of conjugating an X Pauli generator by the inverse of the tableau.

        A faster version of `tableau.inverse(unsigned).x_output(input_index)`.

        Args:
            input_index: Identifies the column (the qubit of the X generator) to return from the inverse tableau.
            unsigned: Defaults to false. When set to true, skips computing the result's sign and instead just sets
                it to positive. This is beneficial because computing the sign takes O(n^2) time whereas all other
                parts of the computation take O(n) time where n is the number of qubits in the tableau.

        Returns:
            The result of conjugating an X generator by the inverse of the tableau.

        Examples:
            >>> import stim

            # Check equivalence with the inverse's x_output.
            >>> t = stim.Tableau.random(4)
            >>> expected = t.inverse().x_output(0)
            >>> t.inverse_x_output(0) == expected
            True
            >>> expected.sign = +1
            >>> t.inverse_x_output(0, unsigned=True) == expected
            True
        """
    def inverse_x_output_pauli(
        self,
        input_index: int,
        output_index: int,
    ) -> int:
        """Returns a Pauli term from the tableau's inverse's output pauli string for an input X generator.

        A constant-time equivalent for `tableau.inverse().x_output(input_index)[output_index]`.

        Args:
            input_index: Identifies the column (the qubit of the input X generator) in the inverse tableau.
            output_index: Identifies the row (the output qubit) in the inverse tableau.

        Returns:
            An integer identifying Pauli at the given location in the inverse tableau:

                0: I
                1: X
                2: Y
                3: Z

        Examples:
            >>> import stim

            >>> t_inv = stim.Tableau.from_conjugated_generators(
            ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
            ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
            ... ).inverse()
            >>> t_inv.inverse_x_output_pauli(0, 0)
            2
            >>> t_inv.inverse_x_output_pauli(0, 1)
            0
            >>> t_inv.inverse_x_output_pauli(1, 0)
            2
            >>> t_inv.inverse_x_output_pauli(1, 1)
            3
        """
    def inverse_y_output(
        self,
        input_index: int,
        *,
        unsigned: bool = False,
    ) -> stim.PauliString:
        """Returns the result of conjugating a Y Pauli generator by the inverse of the tableau.

        A faster version of `tableau.inverse(unsigned).y_output(input_index)`.

        Args:
            input_index: Identifies the column (the qubit of the Y generator) to return from the inverse tableau.
            unsigned: Defaults to false. When set to true, skips computing the result's sign and instead just sets
                it to positive. This is beneficial because computing the sign takes O(n^2) time whereas all other
                parts of the computation take O(n) time where n is the number of qubits in the tableau.

        Returns:
            The result of conjugating a Y generator by the inverse of the tableau.

        Examples:
            >>> import stim

            # Check equivalence with the inverse's y_output.
            >>> t = stim.Tableau.random(4)
            >>> expected = t.inverse().y_output(0)
            >>> t.inverse_y_output(0) == expected
            True
            >>> expected.sign = +1
            >>> t.inverse_y_output(0, unsigned=True) == expected
            True
        """
    def inverse_y_output_pauli(
        self,
        input_index: int,
        output_index: int,
    ) -> int:
        """Returns a Pauli term from the tableau's inverse's output pauli string for an input Y generator.

        A constant-time equivalent for `tableau.inverse().y_output(input_index)[output_index]`.

        Args:
            input_index: Identifies the column (the qubit of the input Y generator) in the inverse tableau.
            output_index: Identifies the row (the output qubit) in the inverse tableau.

        Returns:
            An integer identifying Pauli at the given location in the inverse tableau:

                0: I
                1: X
                2: Y
                3: Z

        Examples:
            >>> import stim

            >>> t_inv = stim.Tableau.from_conjugated_generators(
            ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
            ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
            ... ).inverse()
            >>> t_inv.inverse_y_output_pauli(0, 0)
            1
            >>> t_inv.inverse_y_output_pauli(0, 1)
            2
            >>> t_inv.inverse_y_output_pauli(1, 0)
            0
            >>> t_inv.inverse_y_output_pauli(1, 1)
            2
        """
    def inverse_z_output(
        self,
        input_index: int,
        *,
        unsigned: bool = False,
    ) -> stim.PauliString:
        """Returns the result of conjugating a Z Pauli generator by the inverse of the tableau.

        A faster version of `tableau.inverse(unsigned).z_output(input_index)`.

        Args:
            input_index: Identifies the column (the qubit of the Z generator) to return from the inverse tableau.
            unsigned: Defaults to false. When set to true, skips computing the result's sign and instead just sets
                it to positive. This is beneficial because computing the sign takes O(n^2) time whereas all other
                parts of the computation take O(n) time where n is the number of qubits in the tableau.

        Returns:
            The result of conjugating a Z generator by the inverse of the tableau.

        Examples:
            >>> import stim

            >>> import stim

            # Check equivalence with the inverse's z_output.
            >>> t = stim.Tableau.random(4)
            >>> expected = t.inverse().z_output(0)
            >>> t.inverse_z_output(0) == expected
            True
            >>> expected.sign = +1
            >>> t.inverse_z_output(0, unsigned=True) == expected
            True
        """
    def inverse_z_output_pauli(
        self,
        input_index: int,
        output_index: int,
    ) -> int:
        """Returns a Pauli term from the tableau's inverse's output pauli string for an input Z generator.

        A constant-time equivalent for `tableau.inverse().z_output(input_index)[output_index]`.

        Args:
            input_index: Identifies the column (the qubit of the input Z generator) in the inverse tableau.
            output_index: Identifies the row (the output qubit) in the inverse tableau.

        Returns:
            An integer identifying Pauli at the given location in the inverse tableau:

                0: I
                1: X
                2: Y
                3: Z

        Examples:
            >>> import stim

            >>> t_inv = stim.Tableau.from_conjugated_generators(
            ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
            ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
            ... ).inverse()
            >>> t_inv.inverse_z_output_pauli(0, 0)
            3
            >>> t_inv.inverse_z_output_pauli(0, 1)
            2
            >>> t_inv.inverse_z_output_pauli(1, 0)
            2
            >>> t_inv.inverse_z_output_pauli(1, 1)
            1
        """
    @staticmethod
    def iter_all(
        num_qubits: int,
        *,
        unsigned: bool = False,
    ) -> stim.TableauIterator:
        """Returns an iterator that iterates over all Tableaus of a given size.

        Args:
            num_qubits: The size of tableau to iterate over.
            unsigned: Defaults to False. If set to True, only tableaus where
                all columns have positive sign are yielded by the iterator.
                This substantially reduces the total number of tableaus to
                iterate over.

        Returns:
            An Iterable[stim.Tableau] that yields the requested tableaus.

        Examples:
            >>> import stim
            >>> single_qubit_gate_reprs = set()
            >>> for t in stim.Tableau.iter_all(1):
            ...     single_qubit_gate_reprs.add(repr(t))
            >>> len(single_qubit_gate_reprs)
            24

            >>> num_2q_gates_mod_paulis = 0
            >>> for _ in stim.Tableau.iter_all(2, unsigned=True):
            ...     num_2q_gates_mod_paulis += 1
            >>> num_2q_gates_mod_paulis
            720
        """
    def prepend(
        self,
        gate: stim.Tableau,
        targets: List[int],
    ) -> None:
        """Prepends an operation's effect into this tableau, mutating this tableau.

        Time cost is O(n*m*m) where n=len(self) and m=len(gate).

        Args:
            gate: The tableau of the operation being prepended into this tableau.
            targets: The qubits being targeted by the gate.

        Examples:
            >>> import stim
            >>> h = stim.Tableau.from_named_gate("H")
            >>> cnot = stim.Tableau.from_named_gate("CNOT")
            >>> t = stim.Tableau.from_named_gate("H")
            >>> t.prepend(stim.Tableau.from_named_gate("X"), [0])
            >>> t == stim.Tableau.from_named_gate("SQRT_Y_DAG")
            True
        """
    @staticmethod
    def random(
        num_qubits: int,
    ) -> stim.Tableau:
        """Samples a uniformly random Clifford operation over the given number of qubits and returns its tableau.

        Args:
            num_qubits: The number of qubits the tableau should act on.

        Returns:
            The sampled tableau.

        Examples:
            >>> import stim
            >>> t = stim.Tableau.random(42)

        References:
            "Hadamard-free circuits expose the structure of the Clifford group"
            Sergey Bravyi, Dmitri Maslov
            https://arxiv.org/abs/2003.09412
        """
    def then(
        self,
        second: stim.Tableau,
    ) -> stim.Tableau:
        """Returns the result of composing two tableaus.

        If the tableau T1 represents the Clifford operation with unitary C1,
        and the tableau T2 represents the Clifford operation with unitary C2,
        then the tableau T1.then(T2) represents the Clifford operation with unitary C2*C1.

        Args:
            second: The result is equivalent to applying the second tableau after
                the receiving tableau.

        Examples:
            >>> import stim
            >>> t1 = stim.Tableau.random(4)
            >>> t2 = stim.Tableau.random(4)
            >>> t3 = t1.then(t2)
            >>> p = stim.PauliString.random(4)
            >>> t3(p) == t2(t1(p))
            True
        """
    def to_circuit(
        self,
        *,
        method: str,
    ) -> stim.Circuit:

        """Synthesizes a circuit that implements the tableau's Clifford operation.

        The circuits returned by this method are not guaranteed to be stable
        from version to version, and may be produced using randomization.

        Args:
            method: The method to use when synthesizing the circuit. Available values are:
                "elimination": Uses Gaussian elimination to cancel off-diagonal terms one by one.
                    Gate set: H, S, CX
                    Circuit qubit count: n
                    Circuit operation count: O(n^2)
                    Circuit depth: O(n^2)

        Returns:
            The synthesized circuit.

        Example:
            >>> import stim
            >>> tableau = stim.Tableau.from_conjugated_generators(
            ...     xs=[
            ...         stim.PauliString("-_YZ"),
            ...         stim.PauliString("-YY_"),
            ...         stim.PauliString("-XZX"),
            ...     ],
            ...     zs=[
            ...         stim.PauliString("+Y_Y"),
            ...         stim.PauliString("-_XY"),
            ...         stim.PauliString("-Y__"),
            ...     ],
            ... )
            >>> tableau.to_circuit(method="elimination")
            stim.Circuit('''
                CX 2 0 0 2 2 0
                S 0
                H 0
                S 0
                H 1
                CX 0 1 0 2
                H 1 2
                CX 1 0 2 0 2 1 1 2 2 1
                H 1
                S 1 2
                H 2
                CX 2 1
                S 2
                H 0 1 2
                S 0 0 1 1 2 2
                H 0 1 2
                S 1 1 2 2
            ''')
        """
    def to_unitary_matrix(
        self,
        *,
        endian: str,
    ) -> np.ndarray[np.complex64]:

        """Converts the tableau into a unitary matrix.

        Args:
            endian:
                "little": The first qubit is the least significant (corresponds
                    to an offset of 1 in the state vector).
                "big": The first qubit is the most significant (corresponds
                    to an offset of 2**(n - 1) in the state vector).

        Returns:
            A numpy array with dtype=np.complex64 and shape=(1 << len(tableau), 1 << len(tableau)).

        Example:
            >>> import stim
            >>> cnot = stim.Tableau.from_conjugated_generators(
            ...     xs=[
            ...         stim.PauliString("XX"),
            ...         stim.PauliString("_X"),
            ...     ],
            ...     zs=[
            ...         stim.PauliString("Z_"),
            ...         stim.PauliString("ZZ"),
            ...     ],
            ... )
            >>> cnot.to_unitary_matrix(endian='big')
            array([[1.+0.j, 0.+0.j, 0.+0.j, 0.+0.j],
                   [0.+0.j, 1.+0.j, 0.+0.j, 0.+0.j],
                   [0.+0.j, 0.+0.j, 0.+0.j, 1.+0.j],
                   [0.+0.j, 0.+0.j, 1.+0.j, 0.+0.j]], dtype=complex64)
        """
    def x_output(
        self,
        target: int,
    ) -> stim.PauliString:
        """Returns the result of conjugating a Pauli X by the tableau's Clifford operation.

        Args:
            target: The qubit targeted by the Pauli X operation.

        Examples:
            >>> import stim
            >>> h = stim.Tableau.from_named_gate("H")
            >>> h.x_output(0)
            stim.PauliString("+Z")

            >>> cnot = stim.Tableau.from_named_gate("CNOT")
            >>> cnot.x_output(0)
            stim.PauliString("+XX")
            >>> cnot.x_output(1)
            stim.PauliString("+_X")
        """
    def x_output_pauli(
        self,
        input_index: int,
        output_index: int,
    ) -> int:
        """Returns a Pauli term from the tableau's output pauli string for an input X generator.

        A constant-time equivalent for `tableau.x_output(input_index)[output_index]`.

        Args:
            input_index: Identifies the tableau column (the qubit of the input X generator).
            output_index: Identifies the tableau row (the output qubit).

        Returns:
            An integer identifying Pauli at the given location in the tableau:

                0: I
                1: X
                2: Y
                3: Z

        Examples:
            >>> import stim

            >>> t = stim.Tableau.from_conjugated_generators(
            ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
            ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
            ... )
            >>> t.x_output_pauli(0, 0)
            2
            >>> t.x_output_pauli(0, 1)
            0
            >>> t.x_output_pauli(1, 0)
            2
            >>> t.x_output_pauli(1, 1)
            3
        """
    def y_output(
        self,
        target: int,
    ) -> stim.PauliString:
        """Returns the result of conjugating a Pauli Y by the tableau's Clifford operation.

        Args:
            target: The qubit targeted by the Pauli Y operation.

        Examples:
            >>> import stim
            >>> h = stim.Tableau.from_named_gate("H")
            >>> h.y_output(0)
            stim.PauliString("-Y")

            >>> cnot = stim.Tableau.from_named_gate("CNOT")
            >>> cnot.y_output(0)
            stim.PauliString("+YX")
            >>> cnot.y_output(1)
            stim.PauliString("+ZY")
        """
    def y_output_pauli(
        self,
        input_index: int,
        output_index: int,
    ) -> int:
        """Returns a Pauli term from the tableau's output pauli string for an input Y generator.

        A constant-time equivalent for `tableau.y_output(input_index)[output_index]`.

        Args:
            input_index: Identifies the tableau column (the qubit of the input Y generator).
            output_index: Identifies the tableau row (the output qubit).

        Returns:
            An integer identifying Pauli at the given location in the tableau:

                0: I
                1: X
                2: Y
                3: Z

        Examples:
            >>> import stim

            >>> t = stim.Tableau.from_conjugated_generators(
            ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
            ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
            ... )
            >>> t.y_output_pauli(0, 0)
            1
            >>> t.y_output_pauli(0, 1)
            2
            >>> t.y_output_pauli(1, 0)
            0
            >>> t.y_output_pauli(1, 1)
            2
        """
    def z_output(
        self,
        target: int,
    ) -> stim.PauliString:
        """Returns the result of conjugating a Pauli Z by the tableau's Clifford operation.

        Args:
            target: The qubit targeted by the Pauli Z operation.

        Examples:
            >>> import stim
            >>> h = stim.Tableau.from_named_gate("H")
            >>> h.z_output(0)
            stim.PauliString("+X")

            >>> cnot = stim.Tableau.from_named_gate("CNOT")
            >>> cnot.z_output(0)
            stim.PauliString("+Z_")
            >>> cnot.z_output(1)
            stim.PauliString("+ZZ")
        """
    def z_output_pauli(
        self,
        input_index: int,
        output_index: int,
    ) -> int:
        """Returns a Pauli term from the tableau's output pauli string for an input Z generator.

        A constant-time equivalent for `tableau.z_output(input_index)[output_index]`.

        Args:
            input_index: Identifies the tableau column (the qubit of the input Z generator).
            output_index: Identifies the tableau row (the output qubit).

        Returns:
            An integer identifying Pauli at the given location in the tableau:

                0: I
                1: X
                2: Y
                3: Z

        Examples:
            >>> import stim

            >>> t = stim.Tableau.from_conjugated_generators(
            ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
            ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
            ... )
            >>> t.z_output_pauli(0, 0)
            3
            >>> t.z_output_pauli(0, 1)
            2
            >>> t.z_output_pauli(1, 0)
            2
            >>> t.z_output_pauli(1, 1)
            1
        """
class TableauIterator:
    """Iterates over all stabilizer tableaus of a specified size.

    Examples:
        >>> import stim
        >>> tableau_iterator = stim.Tableau.iter_all(1)
        >>> n = 0
        >>> for single_qubit_clifford in tableau_iterator:
        ...     n += 1
        >>> n
        24
    """
    def __iter__(
        self,
    ) -> stim.TableauIterator:
        """Returns an independent copy of the tableau iterator.

        Since for-loops and loop-comprehensions call `iter` on things they
        iterate, this effectively allows the iterator to be iterated
        multiple times.
        """
    def __next__(
        self,
    ) -> stim.Tableau:
        """Returns the next iterated tableau.
        """
class TableauSimulator:
    """A quantum stabilizer circuit simulator whose internal state is an inverse stabilizer tableau.

    Supports interactive usage, where gates and measurements are applied on demand.

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.h(0)
        >>> if s.measure(0):
        ...     s.h(1)
        ...     s.cnot(1, 2)
        >>> s.measure(1) == s.measure(2)
        True

        >>> s = stim.TableauSimulator()
        >>> s.h(0)
        >>> s.cnot(0, 1)
        >>> s.current_inverse_tableau()
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+ZX"),
                stim.PauliString("+_X"),
            ],
            zs=[
                stim.PauliString("+X_"),
                stim.PauliString("+XZ"),
            ],
        )
    """
    def c_xyz(
        self,
        *targets,
    ) -> None:
        """Applies a C_XYZ gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
        """
    def c_zyx(
        self,
        *targets,
    ) -> None:
        """Applies a C_ZYX gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
        """
    def canonical_stabilizers(
        self,
    ) -> List[stim.PauliString]:
        """Returns a list of the stabilizers of the simulator's current state in a standard form.

        Two simulators have the same canonical stabilizers if and only if their current quantum state is equal
        (and tracking the same number of qubits).

        The canonical form is computed as follows:

            1. Get a list of stabilizers using the `z_output`s of `simulator.current_inverse_tableau()**-1`.
            2. Perform Gaussian elimination on each generator g (ordered X0, Z0, X1, Z1, X2, Z2, etc).
                2a) Pick any stabilizer that uses the generator g. If there are none, go to the next g.
                2b) Multiply that stabilizer into all other stabilizers that use the generator g.
                2c) Swap that stabilizer with the stabilizer at position `next_output` then increment `next_output`.

        Returns:
            A List[stim.PauliString] of the simulator's state's stabilizers.

        Examples:
            >>> import stim
            >>> s = stim.TableauSimulator()
            >>> s.h(0)
            >>> s.cnot(0, 1)
            >>> s.x(2)
            >>> s.canonical_stabilizers()
            [stim.PauliString("+XX_"), stim.PauliString("+ZZ_"), stim.PauliString("-__Z")]

            >>> # Scramble the stabilizers then check that the canonical form is unchanged.
            >>> s.set_inverse_tableau(s.current_inverse_tableau()**-1)
            >>> s.cnot(0, 1)
            >>> s.cz(0, 2)
            >>> s.s(0, 2)
            >>> s.cy(2, 1)
            >>> s.set_inverse_tableau(s.current_inverse_tableau()**-1)
            >>> s.canonical_stabilizers()
            [stim.PauliString("+XX_"), stim.PauliString("+ZZ_"), stim.PauliString("-__Z")]
        """
    def cnot(
        self,
        *targets,
    ) -> None:
        """Applies a controlled X gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
                Applies the gate to the first two targets, then the next two targets, and so forth.
                There must be an even number of targets.
        """
    def copy(
        self,
    ) -> stim.TableauSimulator:
        """Returns a copy of the simulator. A simulator with the same internal state.

        Examples:
            >>> import stim

            >>> s1 = stim.TableauSimulator()
            >>> s1.set_inverse_tableau(stim.Tableau.random(1))
            >>> s2 = s1.copy()
            >>> s2 is s1
            False
            >>> s2.current_inverse_tableau() == s1.current_inverse_tableau()
            True

            >>> s = stim.TableauSimulator()
            >>> def brute_force_post_select(qubit, desired_result):
            ...     global s
            ...     while True:
            ...         copy = s.copy()
            ...         if copy.measure(qubit) == desired_result:
            ...             s = copy
            ...             break
            >>> s.h(0)
            >>> brute_force_post_select(qubit=0, desired_result=True)
            >>> s.measure(0)
            True
        """
    def current_inverse_tableau(
        self,
    ) -> stim.Tableau:
        """Returns a copy of the internal state of the simulator as a stim.Tableau.

        Returns:
            A stim.Tableau copy of the simulator's state.

        Examples:
            >>> import stim
            >>> s = stim.TableauSimulator()
            >>> s.h(0)
            >>> s.current_inverse_tableau()
            stim.Tableau.from_conjugated_generators(
                xs=[
                    stim.PauliString("+Z"),
                ],
                zs=[
                    stim.PauliString("+X"),
                ],
            )
            >>> s.cnot(0, 1)
            >>> s.current_inverse_tableau()
            stim.Tableau.from_conjugated_generators(
                xs=[
                    stim.PauliString("+ZX"),
                    stim.PauliString("+_X"),
                ],
                zs=[
                    stim.PauliString("+X_"),
                    stim.PauliString("+XZ"),
                ],
            )
        """
    def current_measurement_record(
        self,
    ) -> List[bool]:
        """Returns a copy of the record of all measurements performed by the simulator.

        Examples:
            >>> import stim
            >>> s = stim.TableauSimulator()
            >>> s.current_measurement_record()
            []
            >>> s.measure(0)
            False
            >>> s.x(0)
            >>> s.measure(0)
            True
            >>> s.current_measurement_record()
            [False, True]
            >>> s.do(stim.Circuit("M 0"))
            >>> s.current_measurement_record()
            [False, True, True]

        Returns:
            A list of booleans containing the result of every measurement performed by the simulator so far.
        """
    def cx(
        self,
        *targets,
    ) -> None:
        """Applies a controlled X gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
                Applies the gate to the first two targets, then the next two targets, and so forth.
                There must be an even number of targets.
        """
    def cy(
        self,
        *targets,
    ) -> None:
        """Applies a controlled Y gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
                Applies the gate to the first two targets, then the next two targets, and so forth.
                There must be an even number of targets.
        """
    def cz(
        self,
        *targets,
    ) -> None:
        """Applies a controlled Z gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
                Applies the gate to the first two targets, then the next two targets, and so forth.
                There must be an even number of targets.
        """
    @overload
    def do(
        self,
        circuit_or_pauli_string: stim.Circuit,
    ) -> None:
        pass
    @overload
    def do(
        self,
        circuit_or_pauli_string: stim.PauliString,
    ) -> None:
        pass
    def do(
        self,
        circuit_or_pauli_string: object,
    ) -> None:
        """Applies a circuit or pauli string to the simulator's state.

        Args:
            circuit_or_pauli_string: A stim.Circuit or a stim.PauliString containing operations to apply.

        Examples:
            >>> import stim
            >>> s = stim.TableauSimulator()
            >>> s.do(stim.Circuit('''
            ...     X 0
            ...     M 0
            ... '''))
            >>> s.current_measurement_record()
            [True]

            >>> s = stim.TableauSimulator()
            >>> s.do(stim.PauliString("IXYZ"))
            >>> s.measure_many(0, 1, 2, 3)
            [False, True, True, False]
        """
    def do_circuit(
        self,
        circuit: stim.Circuit,
    ) -> None:
        """Applies a circuit to the simulator's state.

        Args:
            circuit: A stim.Circuit containing operations to apply.

        Examples:
            >>> import stim
            >>> s = stim.TableauSimulator()
            >>> s.do_circuit(stim.Circuit('''
            ...     X 0
            ...     M 0
            ... '''))
            >>> s.current_measurement_record()
            [True]
        """
    def do_pauli_string(
        self,
        pauli_string: stim.PauliString,
    ) -> None:
        """Applies the paulis from a pauli string to the simulator's state.

        Args:
            pauli_string: A stim.PauliString containing Paulis to apply.

        Examples:
            >>> import stim
            >>> s = stim.TableauSimulator()
            >>> s.do_pauli_string(stim.PauliString("IXYZ"))
            >>> s.measure_many(0, 1, 2, 3)
            [False, True, True, False]
        """
    def do_tableau(
        self,
        tableau: stim.Tableau,
        targets: List[int],
    ) -> None:
        """Applies a custom tableau operation to qubits in the simulator.

        Note that this method has to compute the inverse of the tableau, because the
        simulator's internal state is an inverse tableau.

        Args:
            tableau: A stim.Tableau representing the Clifford operation to apply.
            targets: The indices of the qubits to operate on.

        Examples:
            >>> import stim
            >>> sim = stim.TableauSimulator()
            >>> sim.h(1)
            >>> sim.h_yz(2)
            >>> [str(sim.peek_bloch(k)) for k in range(4)]
            ['+Z', '+X', '+Y', '+Z']
            >>> rot3 = stim.Tableau.from_conjugated_generators(
            ...     xs=[
            ...         stim.PauliString("_X_"),
            ...         stim.PauliString("__X"),
            ...         stim.PauliString("X__"),
            ...     ],
            ...     zs=[
            ...         stim.PauliString("_Z_"),
            ...         stim.PauliString("__Z"),
            ...         stim.PauliString("Z__"),
            ...     ],
            ... )

            >>> sim.do_tableau(rot3, [1, 2, 3])
            >>> [str(sim.peek_bloch(k)) for k in range(4)]
            ['+Z', '+Z', '+X', '+Y']

            >>> sim.do_tableau(rot3, [1, 2, 3])
            >>> [str(sim.peek_bloch(k)) for k in range(4)]
            ['+Z', '+Y', '+Z', '+X']
        """
    def h(
        self,
        *targets,
    ) -> None:
        """Applies a Hadamard gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
        """
    def h_xy(
        self,
        *targets,
    ) -> None:
        """Applies a variant of the Hadamard gate that swaps the X and Y axes to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
        """
    def h_xz(
        self,
        *targets,
    ) -> None:
        """Applies a Hadamard gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
        """
    def h_yz(
        self,
        *targets,
    ) -> None:
        """Applies a variant of the Hadamard gate that swaps the Y and Z axes to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
        """
    def iswap(
        self,
        *targets,
    ) -> None:
        """Applies an ISWAP gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
                Applies the gate to the first two targets, then the next two targets, and so forth.
                There must be an even number of targets.
        """
    def iswap_dag(
        self,
        *targets,
    ) -> None:
        """Applies an ISWAP_DAG gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
                Applies the gate to the first two targets, then the next two targets, and so forth.
                There must be an even number of targets.
        """
    def measure(
        self,
        target: int,
    ) -> bool:
        """Measures a single qubit.

        Unlike the other methods on TableauSimulator, this one does not broadcast
        over multiple targets. This is to avoid returning a list, which would
        create a pitfall where typing `if sim.measure(qubit)` would be a bug.

        To measure multiple qubits, use `TableauSimulator.measure_many`.

        Args:
            target: The index of the qubit to measure.

        Returns:
            The measurement result as a bool.
        """
    def measure_kickback(
        self,
        target: int,
    ) -> tuple:
        """Measures a qubit and returns the result as well as its Pauli kickback (if any).

        The "Pauli kickback" of a stabilizer circuit measurement is a set of Pauli operations that
        flip the post-measurement system state between the two possible post-measurement states.
        For example, consider measuring one of the qubits in the state |00>+|11> in the Z basis.
        If the measurement result is False, then the system projects into the state |00>.
        If the measurement result is True, then the system projects into the state |11>.
        Applying a Pauli X operation to both qubits flips between |00> and |11>.
        Therefore the Pauli kickback of the measurement is `stim.PauliString("XX")`.
        Note that there are often many possible equivalent Pauli kickbacks. For example,
        if in the previous example there was a third qubit in the |0> state, then both
        `stim.PauliString("XX_")` and `stim.PauliString("XXZ")` are valid kickbacks.

        Measurements with deterministic results don't have a Pauli kickback.

        Args:
            target: The index of the qubit to measure.

        Returns:
            A (result, kickback) tuple.
            The result is a bool containing the measurement's output.
            The kickback is either None (meaning the measurement was deterministic) or a stim.PauliString
            (meaning the measurement was random, and the operations in the Pauli string flip between the
            two possible post-measurement states).

        Examples:
            >>> import stim
            >>> s = stim.TableauSimulator()

            >>> s.measure_kickback(0)
            (False, None)

            >>> s.h(0)
            >>> s.measure_kickback(0)[1]
            stim.PauliString("+X")

            >>> def pseudo_post_select(qubit, desired_result):
            ...     m, kick = s.measure_kickback(qubit)
            ...     if m != desired_result:
            ...         if kick is None:
            ...             raise ValueError("Deterministic measurement differed from desired result.")
            ...         s.do(kick)
            >>> s = stim.TableauSimulator()
            >>> s.h(0)
            >>> s.cnot(0, 1)
            >>> s.cnot(0, 2)
            >>> pseudo_post_select(qubit=2, desired_result=True)
            >>> s.measure_many(0, 1, 2)
            [True, True, True]
        """
    def measure_many(
        self,
        *targets,
    ) -> List[bool]:
        """Measures multiple qubits.

        Args:
            *targets: The indices of the qubits to measure.

        Returns:
            The measurement results as a list of bools.
        """
    @property
    def num_qubits(
        self,
    ) -> int:
        """Returns the number of qubits currently being tracked by the simulator's internal state.

        Note that the number of qubits being tracked will implicitly increase if qubits beyond
        the current limit are touched. Untracked qubits are always assumed to be in the |0> state.

        Examples:
            >>> import stim
            >>> s = stim.TableauSimulator()
            >>> s.num_qubits
            0
            >>> s.h(2)
            >>> s.num_qubits
            3
        """
    def peek_bloch(
        self,
        target: int,
    ) -> stim.PauliString:
        """Returns the current bloch vector of the qubit, represented as a stim.PauliString.

        This is a non-physical operation. It reports information about the qubit without disturbing it.

        Args:
            target: The qubit to peek at.

        Returns:
            stim.PauliString("I"): The qubit is entangled. Its bloch vector is x=y=z=0.
            stim.PauliString("+Z"): The qubit is in the |0> state. Its bloch vector is z=+1, x=y=0.
            stim.PauliString("-Z"): The qubit is in the |1> state. Its bloch vector is z=-1, x=y=0.
            stim.PauliString("+Y"): The qubit is in the |i> state. Its bloch vector is y=+1, x=z=0.
            stim.PauliString("-Y"): The qubit is in the |-i> state. Its bloch vector is y=-1, x=z=0.
            stim.PauliString("+X"): The qubit is in the |+> state. Its bloch vector is x=+1, y=z=0.
            stim.PauliString("-X"): The qubit is in the |-> state. Its bloch vector is x=-1, y=z=0.

        Examples:
            >>> import stim
            >>> s = stim.TableauSimulator()
            >>> s.peek_bloch(0)
            stim.PauliString("+Z")
            >>> s.x(0)
            >>> s.peek_bloch(0)
            stim.PauliString("-Z")
            >>> s.h(0)
            >>> s.peek_bloch(0)
            stim.PauliString("-X")
            >>> s.sqrt_x(1)
            >>> s.peek_bloch(1)
            stim.PauliString("-Y")
            >>> s.cz(0, 1)
            >>> s.peek_bloch(0)
            stim.PauliString("+_")
        """
    def peek_observable_expectation(
        self,
        observable: stim.PauliString,
    ) -> int:
        """Determines the expected value of an observable (which will always be -1, 0, or +1).

        This is a non-physical operation.
        It reports information about the quantum state without disturbing it.

        Args:
            observable: The observable to determine the expected value of.
                This observable must have a real sign, not an imaginary sign.

        Returns:
            +1: Observable will be deterministically false when measured.
            -1: Observable will be deterministically true when measured.
            0: Observable will be random when measured.

        Examples:
            >>> import stim
            >>> s = stim.TableauSimulator()
            >>> s.peek_observable_expectation(stim.PauliString("+Z"))
            1
            >>> s.peek_observable_expectation(stim.PauliString("+X"))
            0
            >>> s.peek_observable_expectation(stim.PauliString("-Z"))
            -1

            >>> s.do(stim.Circuit('''
            ...     H 0
            ...     CNOT 0 1
            ... '''))
            >>> queries = ['XX', 'YY', 'ZZ', '-ZZ', 'ZI', 'II', 'IIZ']
            >>> for q in queries:
            ...     print(q, s.peek_observable_expectation(stim.PauliString(q)))
            XX 1
            YY -1
            ZZ 1
            -ZZ -1
            ZI 0
            II 1
            IIZ 1
        """
    def peek_x(
        self,
        target: int,
    ) -> int:
        """Returns the expected value of a qubit's X observable (which will always be -1, 0, or +1).

        This is a non-physical operation.
        It reports information about the quantum state without disturbing it.

        Args:
            target: The qubit to analyze.

        Returns:
            +1: Qubit is in the |+> state.
            -1: Qubit is in the |-> state.
            0: Qubit is in some other state.

        Example:
            >>> import stim
            >>> s = stim.TableauSimulator()
            >>> s.reset_z(0)
            >>> s.peek_x(0)
            0
            >>> s.reset_x(0)
            >>> s.peek_x(0)
            1
            >>> s.z(0)
            >>> s.peek_x(0)
            -1
        """
    def peek_y(
        self,
        target: int,
    ) -> int:
        """Returns the expected value of a qubit's Y observable (which will always be -1, 0, or +1).

        This is a non-physical operation.
        It reports information about the quantum state without disturbing it.

        Args:
            target: The qubit to analyze.

        Returns:
            +1: Qubit is in the |i> state.
            -1: Qubit is in the |-i> state.
            0: Qubit is in some other state.

        Example:
            >>> import stim
            >>> s = stim.TableauSimulator()
            >>> s.reset_z(0)
            >>> s.peek_y(0)
            0
            >>> s.reset_y(0)
            >>> s.peek_y(0)
            1
            >>> s.z(0)
            >>> s.peek_y(0)
            -1
        """
    def peek_z(
        self,
        target: int,
    ) -> int:
        """Returns the expected value of a qubit's Z observable (which will always be -1, 0, or +1).

        This is a non-physical operation.
        It reports information about the quantum state without disturbing it.

        Args:
            target: The qubit to analyze.

        Returns:
            +1: Qubit is in the |0> state.
            -1: Qubit is in the |1> state.
            0: Qubit is in some other state.

        Example:
            >>> import stim
            >>> s = stim.TableauSimulator()
            >>> s.reset_x(0)
            >>> s.peek_z(0)
            0
            >>> s.reset_z(0)
            >>> s.peek_z(0)
            1
            >>> s.x(0)
            >>> s.peek_z(0)
            -1
        """
    def postselect_x(
        self,
        targets: Union[int, Iterable[int]],
        *,
        desired_value: bool,
    ) -> None:

        """Postselects qubits in the X basis, or raises an exception.

        Postselecting a qubit forces it to collapse to a specific state, as
        if it was measured and that state was the result of the measurement.

        Args:
            targets: The qubit index or indices to postselect.
            desired_value:
                False: postselect targets into the |+> state.
                True: postselect targets into the |-> state.

        Raises:
            ValueError:
                The postselection failed. One of the qubits was in a state
                orthogonal to the desired state, so it was literally
                impossible for a measurement of the qubit to return the
                desired result.
        """
    def postselect_y(
        self,
        targets: Union[int, Iterable[int]],
        *,
        desired_value: bool,
    ) -> None:

        """Postselects qubits in the Y basis, or raises an exception.

        Postselecting a qubit forces it to collapse to a specific state, as
        if it was measured and that state was the result of the measurement.

        Args:
            targets: The qubit index or indices to postselect.
            desired_value:
                False: postselect targets into the |i> state.
                True: postselect targets into the |-i> state.

        Raises:
            ValueError:
                The postselection failed. One of the qubits was in a state
                orthogonal to the desired state, so it was literally
                impossible for a measurement of the qubit to return the
                desired result.
        """
    def postselect_z(
        self,
        targets: Union[int, Iterable[int]],
        *,
        desired_value: bool,
    ) -> None:

        """Postselects qubits in the Z basis, or raises an exception.

        Postselecting a qubit forces it to collapse to a specific state, as
        if it was measured and that state was the result of the measurement.

        Args:
            targets: The qubit index or indices to postselect.
            desired_value:
                False: postselect targets into the |0> state.
                True: postselect targets into the |1> state.

        Raises:
            ValueError:
                The postselection failed. One of the qubits was in a state
                orthogonal to the desired state, so it was literally
                impossible for a measurement of the qubit to return the
                desired result.
        """
    def reset(
        self,
        *targets,
    ) -> None:
        """Resets qubits to the |0> state.

        Args:
            *targets: The indices of the qubits to reset.

        Example:
            >>> import stim
            >>> s = stim.TableauSimulator()
            >>> s.x(0)
            >>> s.reset(0)
            >>> s.peek_bloch(0)
            stim.PauliString("+Z")
        """
    def reset_x(
        self,
        *targets,
    ) -> None:
        """Resets qubits to the |+> state.

        Args:
            *targets: The indices of the qubits to reset.

        Example:
            >>> import stim
            >>> s = stim.TableauSimulator()
            >>> s.reset_x(0)
            >>> s.peek_bloch(0)
            stim.PauliString("+X")
        """
    def reset_y(
        self,
        *targets,
    ) -> None:
        """Resets qubits to the |i> state.

        Args:
            *targets: The indices of the qubits to reset.

        Example:
            >>> import stim
            >>> s = stim.TableauSimulator()
            >>> s.reset_y(0)
            >>> s.peek_bloch(0)
            stim.PauliString("+Y")
        """
    def reset_z(
        self,
        *targets,
    ) -> None:
        """Resets qubits to the |0> state.

        Args:
            *targets: The indices of the qubits to reset.

        Example:
            >>> import stim
            >>> s = stim.TableauSimulator()
            >>> s.h(0)
            >>> s.reset_z(0)
            >>> s.peek_bloch(0)
            stim.PauliString("+Z")
        """
    def s(
        self,
        *targets,
    ) -> None:
        """Applies a SQRT_Z gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
        """
    def s_dag(
        self,
        *targets,
    ) -> None:
        """Applies a SQRT_Z_DAG gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
        """
    def set_inverse_tableau(
        self,
        new_inverse_tableau: stim.Tableau,
    ) -> None:
        """Overwrites the simulator's internal state with a copy of the given inverse tableau.

        The inverse tableau specifies how Pauli product observables of qubits at the current time transform
        into equivalent Pauli product observables at the beginning of time, when all qubits were in the
        |0> state. For example, if the Z observable on qubit 5 maps to a product of Z observables at the
        start of time then a Z basis measurement on qubit 5 will be deterministic and equal to the sign
        of the product. Whereas if it mapped to a product of observables including an X or a Y then the Z
        basis measurement would be random.

        Any qubits not within the length of the tableau are implicitly in the |0> state.

        Args:
            new_inverse_tableau: The tableau to overwrite the internal state with.

        Examples:
            >>> import stim
            >>> s = stim.TableauSimulator()
            >>> t = stim.Tableau.random(4)
            >>> s.set_inverse_tableau(t)
            >>> s.current_inverse_tableau() == t
            True
        """
    def set_num_qubits(
        self,
        new_num_qubits: int,
    ) -> None:
        """Forces the simulator's internal state to track exactly the qubits whose indices are in range(new_num_qubits).

        Note that untracked qubits are always assumed to be in the |0> state. Therefore, calling this method
        will effectively force any qubit whose index is outside range(new_num_qubits) to be reset to |0>.

        Note that this method does not prevent future operations from implicitly expanding the size of the
        tracked state (e.g. setting the number of qubits to 5 will not prevent a Hadamard from then being
        applied to qubit 100, increasing the number of qubits to 101).

        Args:
            new_num_qubits: The length of the range of qubits the internal simulator should be tracking.

        Examples:
            >>> import stim
            >>> s = stim.TableauSimulator()
            >>> len(s.current_inverse_tableau())
            0

            >>> s.set_num_qubits(5)
            >>> len(s.current_inverse_tableau())
            5

            >>> s.x(0, 1, 2, 3)
            >>> s.set_num_qubits(2)
            >>> s.measure_many(0, 1, 2, 3)
            [True, True, False, False]
        """
    def set_state_from_stabilizers(
        self,
        stabilizers: Iterable[stim.PauliString],
        *,
        bool allow_redundant = False,
        bool allow_underconstrained = False,
    ) -> None:

        """Sets the tableau simulator's state to a state satisfying the given stabilizers.

        The old quantum state is completely overwritten, even if the new state is underconstrained
        underconstrained by the given stabilizers. The number of qubits is changed to exactly match
        the number of qubits in the longest given stabilizer.

        Args:
            stabilizers: A list of `stim.PauliString`s specifying the stabilizers that the new
                state must have. It is permitted for stabilizers to have different lengths. All
                stabilizers are padded up to the length of the longest stabilizer by appending
                identity terms.
            allow_redundant: Defaults to False. If set to False, then the given stabilizers must
                all be independent. If any one of them is a product of the others (including the
                empty product), an exception will be raised. If set to True, then redundant
                stabilizers are simply ignored.
            allow_underconstrained: Defaults to False. If set to False, then the given stabilizers
                must form a complete set of generators. They must exactly specify the desired
                stabilizer state, with no degrees of freedom left over. For an n-qubit state there
                must be n independent stabilizers. If set to True, then there can be leftover
                degrees of freedom which can be set arbitrarily.

        Returns:
            Nothing. Mutates the states of the simulator to match the desired stabilizers.
            Guarantees that self.current_inverse_tableau().inverse_z_output(k) will be equal
            to the k'th independent stabilizer from the `stabilizers` argument.

        Raises:
            ValueError:
                A stabilizer is redundant but allow_redundant=True wasn't set.
                OR
                The given stabilizers are contradictory (e.g. "+Z" and "-Z" both specified).
                OR
                The given stabilizers anticommute (e.g. "+Z" and "+X" both specified).
                OR
                The stabilizers left behind a degree of freedom but allow_underconstrained=True wasn't set.
                OR
                A stabilizer has an imaginary sign (i or -i).

        Examples:

            >>> import stim
            >>> tab_sim = stim.TableauSimulator()
            >>> tab_sim.set_state_from_stabilizers([
            ...     stim.PauliString("XX"),
            ...     stim.PauliString("ZZ"),
            ... ])
            >>> tab_sim.current_inverse_tableau().inverse()
            stim.Tableau.from_conjugated_generators(
                xs=[
                    stim.PauliString("+Z_"),
                    stim.PauliString("+_X"),
                ],
                zs=[
                    stim.PauliString("+XX"),
                    stim.PauliString("+ZZ"),
                ],
            )

            >>> tab_sim.set_state_from_stabilizers([
            ...     stim.PauliString("XX_"),
            ...     stim.PauliString("ZZ_"),
            ...     stim.PauliString("-YY_"),
            ...     stim.PauliString(""),
            ... ], allow_underconstrained=True, allow_redundant=True)
            >>> tab_sim.current_inverse_tableau().inverse()
            stim.Tableau.from_conjugated_generators(
                xs=[
                    stim.PauliString("+Z__"),
                    stim.PauliString("+_X_"),
                    stim.PauliString("+__X"),
                ],
                zs=[
                    stim.PauliString("+XX_"),
                    stim.PauliString("+ZZ_"),
                    stim.PauliString("+__Z"),
                ],
            )
        """
    def set_state_from_state_vector(
        self,
        state_vector: Iterable[float],
        *,
        endian: str,
    ) -> None:

        """Sets the tableau simulator's state to a superposition specified by a vector of amplitudes.

        Args:
            state_vector: A list of complex amplitudes specifying a superposition. The vector
                must correspond to a state that is reachable using Clifford operations, and must
                be normalized (i.e. it must be a unit vector).
            endian:
                "little": state vector is in little endian order, where higher index qubits
                    correspond to larger changes in the state index.
                "big": state vector is in little endian order, where higher index qubits correspond to
                    smaller changes in the state index.

        Returns:
            Nothing. Mutates the states of the simulator to match the desired state.

        Raises:
            ValueError:
                The given state vector isn't a list of complex values specifying a stabilizer state.
                OR
                The given endian value isn't 'little' or 'big'.

        Examples:

            >>> import stim
            >>> tab_sim = stim.TableauSimulator()
            >>> tab_sim.set_state_from_state_vector([
            ...     0.5**0.5,
            ...     0.5**0.5 * 1j,
            ... ], endian='little')
            >>> tab_sim.current_inverse_tableau().inverse()
            stim.Tableau.from_conjugated_generators(
                xs=[
                    stim.PauliString("+Z"),
                ],
                zs=[
                    stim.PauliString("+Y"),
                ],
            )
            >>> tab_sim.set_state_from_state_vector([
            ...     0.5**0.5,
            ...     0,
            ...     0,
            ...     0.5**0.5,
            ... ], endian='little')
            >>> tab_sim.current_inverse_tableau().inverse()
            stim.Tableau.from_conjugated_generators(
                xs=[
                    stim.PauliString("+Z_"),
                    stim.PauliString("+_X"),
                ],
                zs=[
                    stim.PauliString("+XX"),
                    stim.PauliString("+ZZ"),
                ],
            )
        """
    def sqrt_x(
        self,
        *targets,
    ) -> None:
        """Applies a SQRT_X gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
        """
    def sqrt_x_dag(
        self,
        *targets,
    ) -> None:
        """Applies a SQRT_X_DAG gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
        """
    def sqrt_y(
        self,
        *targets,
    ) -> None:
        """Applies a SQRT_Y gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
        """
    def sqrt_y_dag(
        self,
        *targets,
    ) -> None:
        """Applies a SQRT_Y_DAG gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
        """
    def state_vector(
        self,
        *,
        endian: str = 'little',
    ) -> np.ndarray[np.complex64]:

        """Returns a wavefunction that satisfies the stabilizers of the simulator's current state.

        This function takes O(n * 2**n) time and O(2**n) space, where n is the number of qubits. The computation is
        done by initialization a random state vector and iteratively projecting it into the +1 eigenspace of each
        stabilizer of the state. The state is then canonicalized so that zero values are actually exactly 0, and so
        that the first non-zero entry is positive.

        Args:
            endian:
                "little" (default): state vector is in little endian order, where higher index qubits
                    correspond to larger changes in the state index.
                "big": state vector is in little endian order, where higher index qubits correspond to
                    smaller changes in the state index.

        Returns:
            A `numpy.ndarray[numpy.complex64]` of computational basis amplitudes.

            If the result is in little endian order then the amplitude at offset b_0 + b_1*2 + b_2*4 + ... + b_{n-1}*2^{n-1} is
            the amplitude for the computational basis state where the qubit with index 0 is storing the bit b_0, the
            qubit with index 1 is storing the bit b_1, etc.

            If the result is in big endian order then the amplitude at offset b_0 + b_1*2 + b_2*4 + ... + b_{n-1}*2^{n-1} is
            the amplitude for the computational basis state where the qubit with index 0 is storing the bit b_{n-1}, the
            qubit with index 1 is storing the bit b_{n-2}, etc.

        Examples:
            >>> import stim
            >>> import numpy as np
            >>> s = stim.TableauSimulator()
            >>> s.x(2)
            >>> list(s.state_vector(endian='little'))
            [0j, 0j, 0j, 0j, (1+0j), 0j, 0j, 0j]

            >>> list(s.state_vector(endian='big'))
            [0j, (1+0j), 0j, 0j, 0j, 0j, 0j, 0j]

            >>> s.sqrt_x(1, 2)
            >>> list(s.state_vector())
            [(0.5+0j), 0j, -0.5j, 0j, 0.5j, 0j, (0.5+0j), 0j]
        """
    def swap(
        self,
        *targets,
    ) -> None:
        """Applies a swap gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
                Applies the gate to the first two targets, then the next two targets, and so forth.
                There must be an even number of targets.
        """
    def x(
        self,
        *targets,
    ) -> None:
        """Applies a Pauli X gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
        """
    def xcx(
        self,
        *targets,
    ) -> None:
        """Applies an X-controlled X gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
                Applies the gate to the first two targets, then the next two targets, and so forth.
                There must be an even number of targets.
        """
    def xcy(
        self,
        *targets,
    ) -> None:
        """Applies an X-controlled Y gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
                Applies the gate to the first two targets, then the next two targets, and so forth.
                There must be an even number of targets.
        """
    def xcz(
        self,
        *targets,
    ) -> None:
        """Applies an X-controlled Z gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
                Applies the gate to the first two targets, then the next two targets, and so forth.
                There must be an even number of targets.
        """
    def y(
        self,
        *targets,
    ) -> None:
        """Applies a Pauli Y gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
        """
    def ycx(
        self,
        *targets,
    ) -> None:
        """Applies a Y-controlled X gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
                Applies the gate to the first two targets, then the next two targets, and so forth.
                There must be an even number of targets.
        """
    def ycy(
        self,
        *targets,
    ) -> None:
        """Applies a Y-controlled Y gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
                Applies the gate to the first two targets, then the next two targets, and so forth.
                There must be an even number of targets.
        """
    def ycz(
        self,
        *targets,
    ) -> None:
        """Applies a Y-controlled Z gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
                Applies the gate to the first two targets, then the next two targets, and so forth.
                There must be an even number of targets.
        """
    def z(
        self,
        *targets,
    ) -> None:
        """Applies a Pauli Z gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
        """
    def zcx(
        self,
        *targets,
    ) -> None:
        """Applies a controlled X gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
                Applies the gate to the first two targets, then the next two targets, and so forth.
                There must be an even number of targets.
        """
    def zcy(
        self,
        *targets,
    ) -> None:
        """Applies a controlled Y gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
                Applies the gate to the first two targets, then the next two targets, and so forth.
                There must be an even number of targets.
        """
    def zcz(
        self,
        *targets,
    ) -> None:
        """Applies a controlled Z gate to the simulator's state.

        Args:
            *targets: The indices of the qubits to target with the gate.
                Applies the gate to the first two targets, then the next two targets, and so forth.
                There must be an even number of targets.
        """
def main(
    *,
    command_line_args: List[str],
) -> int:
    """Runs the command line tool version of stim on the given arguments.

    Note that by default any input will be read from stdin, any output
    will print to stdout (as opposed to being intercepted). For most
    commands, you can use arguments like `--out` to write to a file
    instead of stdout and `--in` to read from a file instead of stdin.

    Returns:
        An exit code (0 means success, not zero means failure).

    Raises:
        A large variety of errors, depending on what you are doing and
        how it failed! Beware that many errors are caught by the main
        method itself and printed to stderr, with the only indication
        that something went wrong being the return code.

    Example:
        >>> import stim
        >>> import tempfile
        >>> with tempfile.TemporaryDirectory() as d:
        ...     path = f'{d}/tmp.out'
        ...     return_code = stim.main(command_line_args=[
        ...         "gen",
        ...         "--code=repetition_code",
        ...         "--task=memory",
        ...         "--rounds=1000",
        ...         "--distance=2",
        ...         "--out",
        ...         path,
        ...     ])
        ...     assert return_code == 0
        ...     with open(path) as f:
        ...         print(f.read(), end='')
        # Generated repetition_code circuit.
        # task: memory
        # rounds: 1000
        # distance: 2
        # before_round_data_depolarization: 0
        # before_measure_flip_probability: 0
        # after_reset_flip_probability: 0
        # after_clifford_depolarization: 0
        # layout:
        # L0 Z1 d2
        # Legend:
        #     d# = data qubit
        #     L# = data qubit with logical observable crossing
        #     Z# = measurement qubit
        R 0 1 2
        TICK
        CX 0 1
        TICK
        CX 2 1
        TICK
        MR 1
        DETECTOR(1, 0) rec[-1]
        REPEAT 999 {
            TICK
            CX 0 1
            TICK
            CX 2 1
            TICK
            MR 1
            SHIFT_COORDS(0, 1)
            DETECTOR(1, 0) rec[-1] rec[-2]
        }
        M 0 2
        DETECTOR(1, 1) rec[-1] rec[-2] rec[-3]
        OBSERVABLE_INCLUDE(0) rec[-1]
    """
def read_shot_data_file(
    *,
    path: str,
    format: str,
    num_measurements: int = 0,
    num_detectors: int = 0,
    num_observables: int = 0,
    bit_pack: bool = False,
) -> np.ndarray:

    """Reads shot data, such as measurement samples, from a file.

    Args:
        path: The path to the file to read the data from.
        format: The format that the data is stored in, such as 'b8'.
            See https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        bit_pack: Defaults to false. Determines whether the result is a bool8 numpy array
            with one bit per byte, or a uint8 numpy array with 8 bits per byte.
        num_measurements: How many measurements there are per shot.
        num_detectors: How many detectors there are per shot.
        num_observables: How many observables there are per shot.
            Note that this only refers to observables *stored in the file*, not to
            observables from the original circuit that was sampled.

    Returns:
        A numpy array containing the loaded data.

        If bit_pack=False:
            dtype = np.bool8
            shape = (num_shots, num_measurements + num_detectors + num_observables)
            bit b from shot s is at result[s, b]
        If bit_pack=True:
            dtype = np.uint8
            shape = (num_shots, math.ceil((num_measurements + num_detectors + num_observables) / 8))
            bit b from shot s is at result[s, b // 8] & (1 << (b % 8))

    Examples:
        >>> import stim
        >>> import pathlib
        >>> import tempfile
        >>> with tempfile.TemporaryDirectory() as d:
        ...     path = pathlib.Path(d) / 'shots'
        ...     with open(path, 'w') as f:
        ...         print("0000", file=f)
        ...         print("0101", file=f)
        ...
        ...     read = stim.read_shot_data_file(
        ...         path=str(path),
        ...         format='01',
        ...         num_measurements=4)
        >>> read
        array([[False, False, False, False],
               [False,  True, False,  True]])
    """
def target_combiner(
) -> stim.GateTarget:
    """Returns a target combiner (`*` in circuit files) that can be used as an operation target.
    """
def target_inv(
    qubit_index: int,
) -> stim.GateTarget:
    """Returns a target flagged as inverted that can be passed into Circuit.append_operation
    For example, the '!1' in 'M 0 !1 2' is qubit 1 flagged as inverted,
    meaning the measurement result from qubit 1 should be inverted when reported.
    """
def target_logical_observable_id(
    index: int,
) -> stim.DemTarget:
    """Returns a logical observable id identifying a frame change (e.g. "L5" in a .dem file).

    Args:
        index: The index of the observable.

    Returns:
        The logical observable target.

    Examples:
        >>> import stim
        >>> m = stim.DetectorErrorModel()
        >>> m.append("error", 0.25, [
        ...     stim.target_logical_observable_id(13)
        ... ])
        >>> print(repr(m))
        stim.DetectorErrorModel('''
            error(0.25) L13
        ''')
    """
def target_rec(
    lookback_index: int,
) -> stim.GateTarget:
    """Returns a record target that can be passed into Circuit.append_operation.
    For example, the 'rec[-2]' in 'DETECTOR rec[-2]' is a record target.
    """
def target_relative_detector_id(
    index: int,
) -> stim.DemTarget:
    """Returns a relative detector id (e.g. "D5" in a .dem file).

    Args:
        index: The index of the detector, relative to the current detector offset.

    Returns:
        The relative detector target.

    Examples:
        >>> import stim
        >>> m = stim.DetectorErrorModel()
        >>> m.append("error", 0.25, [
        ...     stim.target_relative_detector_id(13)
        ... ])
        >>> print(repr(m))
        stim.DetectorErrorModel('''
            error(0.25) D13
        ''')
    """
def target_separator(
) -> stim.DemTarget:
    """Returns a target separator (e.g. "^" in a .dem file).

    Examples:
        >>> import stim
        >>> m = stim.DetectorErrorModel()
        >>> m.append("error", 0.25, [
        ...     stim.target_relative_detector_id(1),
        ...     stim.target_separator(),
        ...     stim.target_relative_detector_id(2),
        ... ])
        >>> print(repr(m))
        stim.DetectorErrorModel('''
            error(0.25) D1 ^ D2
        ''')
    """
def target_sweep_bit(
    sweep_bit_index: int,
) -> stim.GateTarget:
    """Returns a sweep bit target that can be passed into Circuit.append_operation
    For example, the 'sweep[5]' in 'CNOT sweep[5] 7' is from `stim.target_sweep_bit(5)`.
    """
def target_x(
    qubit_index: int,
    invert: bool = False,
) -> stim.GateTarget:
    """Returns a target flagged as Pauli X that can be passed into Circuit.append_operation
    For example, the 'X1' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 1 flagged as Pauli X.
    """
def target_y(
    qubit_index: int,
    invert: bool = False,
) -> stim.GateTarget:
    """Returns a target flagged as Pauli Y that can be passed into Circuit.append_operation
    For example, the 'Y2' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 2 flagged as Pauli Y.
    """
def target_z(
    qubit_index: int,
    invert: bool = False,
) -> stim.GateTarget:
    """Returns a target flagged as Pauli Z that can be passed into Circuit.append_operation
    For example, the 'Z3' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 3 flagged as Pauli Z.
    """
def write_shot_data_file(
    *,
    data: object,
    path: str,
    format: str,
    num_measurements: Any = None,
    num_detectors: Any = None,
    num_observables: Any = None,
) -> None:
    """Writes shot data, such as measurement samples, to a file.

    Args:
        data: The data to write to the file. This must be a numpy array. The dtype
            of the array determines whether or not the data is bit packed, and the
            shape must match the bits per shot.

            dtype=np.bool8: Not bit packed. Shape must be (num_shots, num_measurements + num_detectors + num_observables).
            dtype=np.uint8: Yes bit packed. Shape must be (num_shots, math.ceil((num_measurements + num_detectors + num_observables) / 8)).
        path: The path to the file to write the data to.
        format: The format that the data is stored in, such as 'b8'.
            See https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        num_measurements: How many measurements there are per shot.
        num_detectors: How many detectors there are per shot.
        num_observables: How many observables there are per shot.
            Note that this only refers to observables *in the given shot data*, not to
            observables from the original circuit that was sampled.

    Examples:
        >>> import stim
        >>> import pathlib
        >>> import tempfile
        >>> import numpy as np
        >>> with tempfile.TemporaryDirectory() as d:
        ...     path = pathlib.Path(d) / 'shots'
        ...     shot_data = np.array([
        ...         [0, 1, 0],
        ...         [0, 1, 1],
        ...     ], dtype=np.bool8)
        ...
        ...     stim.write_shot_data_file(
        ...         path=str(path),
        ...         data=shot_data,
        ...         format='01',
        ...         num_measurements=3)
        ...
        ...     with open(path) as f:
        ...         written = f.read()
        >>> written
        '010\n011\n'
    """
