import collections
import functools
from typing import (
    Any,
    Callable,
    cast,
    DefaultDict,
    Dict,
    Iterable,
    Iterator,
    List,
    Sequence,
    Set,
    Tuple,
    Union,
)

import cirq
import stim

from ._det_annotation import DetAnnotation
from ._measure_and_or_reset_gate import MeasureAndOrResetGate
from ._obs_annotation import CumulativeObservableAnnotation
from ._shift_coords_annotation import ShiftCoordsAnnotation
from ._sweep_pauli import SweepPauli
from ._two_qubit_asymmetric_depolarize import TwoQubitAsymmetricDepolarizingChannel


def _stim_targets_to_dense_pauli_string(
    targets: List[stim.GateTarget],
) -> cirq.BaseDensePauliString:
    obs = cirq.MutableDensePauliString("I" * len(targets))
    for k, target in enumerate(targets):
        if target.is_inverted_result_target:
            obs.coefficient *= -1
        if target.is_x_target:
            obs.pauli_mask[k] = 1
        elif target.is_y_target:
            obs.pauli_mask[k] = 2
        elif target.is_z_target:
            obs.pauli_mask[k] = 3
        else:
            raise NotImplementedError(f"target={target!r}")
    return obs.frozen()


def _proper_transform_circuit_qubits(circuit: cirq.AbstractCircuit, remap: Dict[cirq.Qid, cirq.Qid]) -> cirq.Circuit:
    # Note: doing this the hard way because cirq.CircuitOperation otherwise remembers the old indices in
    # its `remap` entry, instead of completely expunging those indices.
    return cirq.Circuit(
        cirq.Moment(
            cirq.CircuitOperation(
                circuit=_proper_transform_circuit_qubits(op.circuit, remap).freeze(),
                repetitions=op.repetitions,
            )
            if isinstance(op, cirq.CircuitOperation)
            else op.with_qubits(*[remap[q] for q in op.qubits])
            for op in moment
        )
        for moment in circuit
    )


class CircuitTranslationTracker:
    def __init__(self, flatten: bool):
        self.qubit_coords: Dict[int, cirq.Qid] = {}
        self.origin: DefaultDict[float] = collections.defaultdict(float)
        self.num_measurements_seen = 0
        self.full_circuit = cirq.Circuit()
        self.tick_circuit = cirq.Circuit()
        self.flatten = flatten
        self.have_seen_loop = False

    def get_next_measure_id(self) -> int:
        self.num_measurements_seen += 1
        return self.num_measurements_seen - 1

    def append_operation(self, op: cirq.Operation) -> None:
        self.tick_circuit.append(op, strategy=cirq.InsertStrategy.INLINE)

    def process_gate_instruction(
        self, gate: cirq.Gate, instruction: stim.CircuitInstruction
    ) -> None:
        targets: List[stim.GateTarget] = instruction.targets_copy()
        m = cirq.num_qubits(gate)
        if not all(t.is_qubit_target for t in targets) or len(targets) % m != 0:
            raise NotImplementedError(f"instruction={instruction!r}")
        for k in range(0, len(targets), m):
            self.append_operation(gate(*[cirq.LineQubit(t.value) for t in targets[k : k + m]]))

    def process_tick(self, instruction: stim.CircuitInstruction) -> None:
        self.full_circuit += self.tick_circuit or cirq.Moment()
        self.tick_circuit = cirq.Circuit()

    def process_pauli_channel_1(self, instruction: stim.CircuitInstruction) -> None:
        args = instruction.gate_args_copy()
        if len(args) != 3:
            raise ValueError(f"len(args={args!r}) != 3")
        self.process_gate_instruction(
            cirq.AsymmetricDepolarizingChannel(p_x=args[0], p_y=args[1], p_z=args[2]), instruction
        )

    def process_pauli_channel_2(self, instruction: stim.CircuitInstruction) -> None:
        args = instruction.gate_args_copy()
        if len(args) != 15:
            raise ValueError(f"len(args={args!r}) != 15")
        self.process_gate_instruction(TwoQubitAsymmetricDepolarizingChannel(args), instruction)

    def process_repeat_block(self, block: stim.CircuitRepeatBlock):
        if self.flatten or block.repeat_count == 1:
            self.process_circuit(block.repeat_count, block.body_copy())
            return

        self.have_seen_loop = True
        child = CircuitTranslationTracker(flatten=self.flatten)
        child.origin = self.origin.copy()
        child.num_measurements_seen = self.num_measurements_seen
        child.qubit_coords = self.qubit_coords.copy()
        child.have_seen_loop = True
        child.process_circuit(1, block.body_copy())
        self.append_operation(
            cirq.CircuitOperation(
                cirq.FrozenCircuit(child.full_circuit + child.tick_circuit),
                repetitions=block.repeat_count,
            )
        )
        self.qubit_coords = child.qubit_coords
        self.num_measurements_seen += (
            child.num_measurements_seen - self.num_measurements_seen
        ) * block.repeat_count
        for k, v in child.origin.items():
            self.origin[k] += (v - self.origin[k]) * block.repeat_count

    def process_measurement_instruction(
        self, instruction: stim.CircuitInstruction, measure: bool, reset: bool, basis: str
    ) -> None:
        args = instruction.gate_args_copy()
        flip_probability = 0
        if args:
            flip_probability = args[0]

        targets: List[stim.GateTarget] = instruction.targets_copy()
        for t in targets:
            if not t.is_qubit_target:
                raise NotImplementedError(f"instruction={instruction!r}")
            key = str(self.get_next_measure_id())
            self.append_operation(
                MeasureAndOrResetGate(
                    measure=measure,
                    reset=reset,
                    basis=basis,
                    invert_measure=t.is_inverted_result_target,
                    key=key,
                    measure_flip_probability=flip_probability,
                ).resolve(cirq.LineQubit(t.value))
            )

    def process_circuit(self, repetitions: int, circuit: stim.Circuit) -> None:
        handler_table = CircuitTranslationTracker.get_handler_table()
        for _ in range(repetitions):
            for instruction in circuit:
                if isinstance(instruction, stim.CircuitInstruction):
                    handler = handler_table.get(instruction.name)
                    if handler is None:
                        raise NotImplementedError(f"{instruction!r}")
                    handler(self, instruction)
                elif isinstance(instruction, stim.CircuitRepeatBlock):
                    self.process_repeat_block(instruction)
                else:
                    raise NotImplementedError(f"instruction={instruction!r}")

    def output(self) -> cirq.Circuit:
        out = self.full_circuit + self.tick_circuit

        if self.qubit_coords:
            remap: Dict[cirq.Qid, cirq.Qid] = {
                q: self.qubit_coords.get(cast(cirq.LineQubit, q).x, q) for q in out.all_qubits()
            }

            # Only remap if there are no collisions.
            if len(set(remap.values())) == len(remap):
                # Note: doing this the hard way because cirq.CircuitOperation otherwise remembers the old indices in
                # its `remap` entry, instead of completely expunging those indices.
                out = _proper_transform_circuit_qubits(out, remap)

        return out

    def process_mpp(self, instruction: stim.CircuitInstruction) -> None:
        args = instruction.gate_args_copy()
        if args and args[0]:
            raise NotImplementedError("Noisy MPP")

        targets: List[stim.GateTarget] = instruction.targets_copy()
        start = 0
        while start < len(targets):
            next_start = start + 1
            while next_start < len(targets) and targets[next_start].is_combiner:
                next_start += 2
            group = targets[start:next_start:2]
            start = next_start

            obs = _stim_targets_to_dense_pauli_string(group)
            qubits = [cirq.LineQubit(t.value) for t in group]
            key = str(self.get_next_measure_id())
            if obs.coefficient == -1:
                raise NotImplementedError(
                    "Converting inverted MPP blocked by https://github.com/quantumlib/Cirq/issues/4814"
                )
            self.append_operation(cirq.PauliMeasurementGate(obs, key=key).on(*qubits))

    def process_correlated_error(self, instruction: stim.CircuitInstruction) -> None:
        args = instruction.gate_args_copy()
        probability = args[0] if args else 0
        targets = instruction.targets_copy()
        qubits = [cirq.LineQubit(t.value) for t in targets]
        self.append_operation(
            _stim_targets_to_dense_pauli_string(targets).on(*qubits).with_probability(probability)
        )

    def coords_after_offset(
        self, relative_coords: List[float], *, even_if_flattening: bool = False
    ) -> List[Union[float, int]]:
        if not self.flatten and not even_if_flattening:
            return list(relative_coords)
        result = []
        for k in range(len(relative_coords)):
            t = relative_coords[k] + self.origin[k]
            if t == int(t):
                t = int(t)
            result.append(t)
        return result

    def resolve_measurement_record_keys(
        self, targets: Iterable[stim.GateTarget]
    ) -> Tuple[List[str], List[int]]:
        if self.have_seen_loop:
            return [], [t.value for t in targets]
        else:
            return [str(self.num_measurements_seen + t.value) for t in targets], []

    def process_detector(self, instruction: stim.CircuitInstruction) -> None:
        coords = self.coords_after_offset(instruction.gate_args_copy())
        keys, rels = self.resolve_measurement_record_keys(instruction.targets_copy())
        self.append_operation(
            DetAnnotation(parity_keys=keys, relative_keys=rels, coordinate_metadata=coords)
        )

    def process_observable_include(self, instruction: stim.CircuitInstruction) -> None:
        args = instruction.gate_args_copy()
        index = 0 if not args else int(args[0])
        keys, rels = self.resolve_measurement_record_keys(instruction.targets_copy())
        self.append_operation(
            CumulativeObservableAnnotation(
                parity_keys=keys, relative_keys=rels, observable_index=index
            )
        )

    def process_qubit_coords(self, instruction: stim.CircuitInstruction) -> None:
        coords = self.coords_after_offset(instruction.gate_args_copy(), even_if_flattening=True)
        for t in instruction.targets_copy():
            if len(coords) == 1:
                self.qubit_coords[t.value] = cirq.LineQubit(*coords)
            elif len(coords) == 2:
                self.qubit_coords[t.value] = cirq.GridQubit(*coords)

    def process_shift_coords(self, instruction: stim.CircuitInstruction) -> None:
        args = instruction.gate_args_copy()
        if not self.flatten:
            self.append_operation(ShiftCoordsAnnotation(args))
        for k, a in enumerate(args):
            self.origin[k] += a

    class OneToOneGateHandler:
        def __init__(self, gate: cirq.Gate):
            self.gate = gate

        def __call__(
            self, tracker: 'CircuitTranslationTracker', instruction: stim.CircuitInstruction
        ) -> None:
            tracker.process_gate_instruction(gate=self.gate, instruction=instruction)

    class SweepableGateHandler:
        def __init__(self, pauli_gate: cirq.Pauli, gate: cirq.Gate):
            self.pauli_gate = pauli_gate
            self.gate = gate

        def __call__(
            self, tracker: 'CircuitTranslationTracker', instruction: stim.CircuitInstruction
        ) -> None:
            targets: List[stim.GateTarget] = instruction.targets_copy()
            for k in range(0, len(targets), 2):
                a = targets[k]
                b = targets[k + 1]
                if not a.is_qubit_target and not b.is_qubit_target:
                    raise NotImplementedError(f"instruction={instruction!r}")
                if a.is_sweep_bit_target or b.is_sweep_bit_target:
                    if b.is_sweep_bit_target:
                        a, b = b, a
                    assert not a.is_inverted_result_target
                    tracker.append_operation(
                        SweepPauli(
                            stim_sweep_bit_index=a.value,
                            cirq_sweep_symbol=f'sweep[{a.value}]',
                            pauli=self.pauli_gate,
                        ).on(cirq.LineQubit(b.value))
                    )
                else:
                    if not a.is_qubit_target or not b.is_qubit_target:
                        raise NotImplementedError(f"instruction={instruction!r}")
                    tracker.append_operation(
                        self.gate(cirq.LineQubit(a.value), cirq.LineQubit(b.value))
                    )

    class OneToOneMeasurementHandler:
        def __init__(self, *, reset: bool, measure: bool, basis: str):
            self.reset = reset
            self.measure = measure
            self.basis = basis

        def __call__(
            self, tracker: 'CircuitTranslationTracker', instruction: stim.CircuitInstruction
        ) -> None:
            tracker.process_measurement_instruction(
                measure=self.measure, reset=self.reset, basis=self.basis, instruction=instruction
            )

    class OneToOneNoisyGateHandler:
        def __init__(self, prob_to_gate: Callable[[float], cirq.Gate]):
            self.prob_to_gate = prob_to_gate

        def __call__(
            self, tracker: 'CircuitTranslationTracker', instruction: stim.CircuitInstruction
        ) -> None:
            tracker.process_gate_instruction(
                self.prob_to_gate(instruction.gate_args_copy()[0]), instruction
            )

    @staticmethod
    @functools.lru_cache(maxsize=1)
    def get_handler_table() -> Dict[
        str, Callable[['CircuitTranslationTracker', stim.Circuit], None]
    ]:
        gate = CircuitTranslationTracker.OneToOneGateHandler
        measure_gate = CircuitTranslationTracker.OneToOneMeasurementHandler
        noise = CircuitTranslationTracker.OneToOneNoisyGateHandler
        sweep_gate = CircuitTranslationTracker.SweepableGateHandler

        def not_impl(message) -> Callable[[Any], None]:
            def handler(
                tracker: CircuitTranslationTracker, instruction: stim.CircuitInstruction
            ) -> None:
                raise NotImplementedError(message)

            return handler

        return {
            "M": measure_gate(measure=True, reset=False, basis='Z'),
            "MX": measure_gate(measure=True, reset=False, basis='X'),
            "MY": measure_gate(measure=True, reset=False, basis='Y'),
            "MZ": measure_gate(measure=True, reset=False, basis='Z'),
            "MR": measure_gate(measure=True, reset=True, basis='Z'),
            "MRX": measure_gate(measure=True, reset=True, basis='X'),
            "MRY": measure_gate(measure=True, reset=True, basis='Y'),
            "MRZ": measure_gate(measure=True, reset=True, basis='Z'),
            "R": gate(cirq.ResetChannel()),
            "RX": gate(
                MeasureAndOrResetGate(
                    measure=False, reset=True, basis='X', invert_measure=False, key=''
                )
            ),
            "RY": gate(
                MeasureAndOrResetGate(
                    measure=False, reset=True, basis='Y', invert_measure=False, key=''
                )
            ),
            "RZ": gate(cirq.ResetChannel()),
            "I": gate(cirq.I),
            "X": gate(cirq.X),
            "Y": gate(cirq.Y),
            "Z": gate(cirq.Z),
            "H_XY": gate(
                cirq.SingleQubitCliffordGate.from_xz_map(x_to=(cirq.Y, False), z_to=(cirq.Z, True))
            ),
            "H": gate(cirq.H),
            "H_XZ": gate(cirq.H),
            "H_YZ": gate(
                cirq.SingleQubitCliffordGate.from_xz_map(x_to=(cirq.X, True), z_to=(cirq.Y, False))
            ),
            "SQRT_X": gate(cirq.X ** 0.5),
            "SQRT_X_DAG": gate(cirq.X ** -0.5),
            "SQRT_Y": gate(cirq.Y ** 0.5),
            "SQRT_Y_DAG": gate(cirq.Y ** -0.5),
            "C_XYZ": gate(
                cirq.SingleQubitCliffordGate.from_xz_map(x_to=(cirq.Y, False), z_to=(cirq.X, False))
            ),
            "C_ZYX": gate(
                cirq.SingleQubitCliffordGate.from_xz_map(x_to=(cirq.Z, False), z_to=(cirq.Y, False))
            ),
            "SQRT_XX": gate(cirq.XX ** 0.5),
            "SQRT_YY": gate(cirq.YY ** 0.5),
            "SQRT_ZZ": gate(cirq.ZZ ** 0.5),
            "SQRT_XX_DAG": gate(cirq.XX ** -0.5),
            "SQRT_YY_DAG": gate(cirq.YY ** -0.5),
            "SQRT_ZZ_DAG": gate(cirq.ZZ ** -0.5),
            "S": gate(cirq.S),
            "S_DAG": gate(cirq.S ** -1),
            "SQRT_Z": gate(cirq.S),
            "SQRT_Z_DAG": gate(cirq.S ** -1),
            "SWAP": gate(cirq.SWAP),
            "ISWAP": gate(cirq.ISWAP),
            "ISWAP_DAG": gate(cirq.ISWAP ** -1),
            "XCX": gate(cirq.PauliInteractionGate(cirq.X, False, cirq.X, False)),
            "XCY": gate(cirq.PauliInteractionGate(cirq.X, False, cirq.Y, False)),
            "XCZ": sweep_gate(cirq.X, cirq.PauliInteractionGate(cirq.X, False, cirq.Z, False)),
            "YCX": gate(cirq.PauliInteractionGate(cirq.Y, False, cirq.X, False)),
            "YCY": gate(cirq.PauliInteractionGate(cirq.Y, False, cirq.Y, False)),
            "YCZ": sweep_gate(cirq.Y, cirq.PauliInteractionGate(cirq.Y, False, cirq.Z, False)),
            "CX": sweep_gate(cirq.X, cirq.CNOT),
            "CNOT": sweep_gate(cirq.X, cirq.CNOT),
            "ZCX": sweep_gate(cirq.X, cirq.CNOT),
            "CY": sweep_gate(cirq.Y, cirq.Y.controlled(1)),
            "ZCY": sweep_gate(cirq.Y, cirq.Y.controlled(1)),
            "CZ": sweep_gate(cirq.Z, cirq.CZ),
            "ZCZ": sweep_gate(cirq.Z, cirq.CZ),
            "DEPOLARIZE1": noise(lambda p: cirq.DepolarizingChannel(p, 1)),
            "DEPOLARIZE2": noise(lambda p: cirq.DepolarizingChannel(p, 2)),
            "X_ERROR": noise(cirq.X.with_probability),
            "Y_ERROR": noise(cirq.Y.with_probability),
            "Z_ERROR": noise(cirq.Z.with_probability),
            "PAULI_CHANNEL_1": CircuitTranslationTracker.process_pauli_channel_1,
            "PAULI_CHANNEL_2": CircuitTranslationTracker.process_pauli_channel_2,
            "ELSE_CORRELATED_ERROR": not_impl(
                "Converting ELSE_CORRELATED_ERROR to cirq is not supported."
            ),
            "REPEAT": not_impl("[handled special]"),
            "TICK": CircuitTranslationTracker.process_tick,
            "SHIFT_COORDS": CircuitTranslationTracker.process_shift_coords,
            "QUBIT_COORDS": CircuitTranslationTracker.process_qubit_coords,
            "E": CircuitTranslationTracker.process_correlated_error,
            "CORRELATED_ERROR": CircuitTranslationTracker.process_correlated_error,
            "MPP": CircuitTranslationTracker.process_mpp,
            "DETECTOR": CircuitTranslationTracker.process_detector,
            "OBSERVABLE_INCLUDE": CircuitTranslationTracker.process_observable_include,
        }


def stim_circuit_to_cirq_circuit(circuit: stim.Circuit, *, flatten: bool = False) -> cirq.Circuit:
    """Converts a stim circuit into an equivalent cirq circuit.

    Qubit indices are turned into cirq.LineQubit instances. Measurements are
    keyed by their ordering (e.g. the first measurement is keyed "0", the second
    is keyed "1", etc).

    Not all circuits can be converted:
        - ELSE_CORRELATED_ERROR instructions are not supported.

    Not all circuits can be converted with perfect 1:1 fidelity:
        - DETECTOR annotations are discarded.
        - OBSERVABLE_INCLUDE annotations are discarded.

    Args:
        circuit: The stim circuit to convert into a cirq circuit.
        flatten: Defaults to False. When set to True, REPEAT blocks are removed by
            explicitly repeating their instructions multiple times. Also,
            SHIFT_COORDS instructions are removed by appropriately adjusting the
            coordinate metadata of later instructions.

    Returns:
        The converted circuit.

    Examples:

        >>> import stimcirq
        >>> import stim
        >>> print(stimcirq.stim_circuit_to_cirq_circuit(stim.Circuit('''
        ...     H 0
        ...     CNOT 0 1
        ...     X_ERROR(0.25) 0
        ...     TICK
        ...     M !1 0
        ... ''')))
        0: ───H───@───X[prob=0.25]───M('1')────
                  │
        1: ───────X──────────────────!M('0')───
    """
    tracker = CircuitTranslationTracker(flatten=flatten)
    tracker.process_circuit(repetitions=1, circuit=circuit)
    return tracker.output()
