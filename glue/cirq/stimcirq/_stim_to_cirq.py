import collections
import functools
from typing import Callable, Dict, List, Tuple, Union, Iterator, cast, Sequence, Iterable, Set, Any, DefaultDict

import cirq
import stim


@cirq.value_equality
class TwoQubitAsymmetricDepolarizingChannel(cirq.Gate):
    def __init__(self, probabilities: Sequence[float]):
        if len(probabilities) != 15:
            raise ValueError(len(probabilities) != 15)
        self.probabilities = tuple(probabilities)

    def _num_qubits_(self):
        return 2

    def _value_equality_values_(self):
        return self.probabilities

    def _has_mixture_(self):
        return True

    def _dense_mixture_(self):
        result = [(1 - sum(self.probabilities), cirq.DensePauliString([0, 0]))]
        result.extend([
            (p, cirq.DensePauliString([((k + 1) >> 2) & 3, (k + 1) & 3]))
            for k, p in enumerate(self.probabilities)
        ])
        return [(p, g) for p, g in result if p]

    def _mixture_(self):
        return [(p, cirq.unitary(g)) for p, g in self._dense_mixture_()]

    def _circuit_diagram_info_(self, args: cirq.CircuitDiagramInfoArgs):
        result = []
        for p, d in self._dense_mixture_():
            result.append(str(d)[1:] + ":" + args.format_real(p))
        return "PauliMix(" + ",".join(result) + ")", "#2"

    def _stim_conversion_(
            self,
            edit_circuit: stim.Circuit,
            targets: List[int],
            **kwargs):
        edit_circuit.append_operation("PAULI_CHANNEL_2", targets, self.probabilities)

    def __repr__(self):
        return f"stimcirq.TwoQubitAsymmetricDepolarizingChannel({self.probabilities!r})"


@cirq.value_equality
class MeasureAndOrReset(cirq.SingleQubitGate):
    def __init__(self, measure: bool, reset: bool, basis: str, invert_measure: bool, key: str, measure_flip_probability: float = 0):
        self.measure = measure
        self.reset = reset
        self.basis = basis
        self.invert_measure = invert_measure
        self.key = key
        self.measure_flip_probability = measure_flip_probability

    def _value_equality_values_(self):
        return self.measure, self.reset, self.basis, self.invert_measure, self.key, self.measure_flip_probability

    def _decompose_(self, qubits):
        q, = qubits
        if self.measure:
            if self.basis == 'X':
                yield cirq.H(q)
            elif self.basis == 'Y':
                yield cirq.X(q)**0.5
            if self.measure_flip_probability:
                raise NotImplementedError("Noisy measurement as a cirq operation.")
            else:
                yield cirq.measure(q, key=self.key, invert_mask=(True,) if self.invert_measure else ())
        if self.reset:
            yield cirq.ResetChannel().on(q)
        if self.measure or self.reset:
            if self.basis == 'X':
                yield cirq.H(q)
            elif self.basis == 'Y':
                yield cirq.X(q)**-0.5

    def with_key(self, key: str) -> 'MeasureAndOrReset':
        return MeasureAndOrReset(
            measure=self.measure,
            reset=self.reset,
            basis=self.basis,
            invert_measure=self.invert_measure,
            key=key,
            measure_flip_probability=self.measure_flip_probability
        )

    def with_bits_flipped(self, *bit_positions):
        assert bit_positions == (0,)
        return MeasureAndOrReset(
            measure=self.measure,
            reset=self.reset,
            basis=self.basis,
            invert_measure=not self.invert_measure,
            key=self.key,
            measure_flip_probability=self.measure_flip_probability
        )

    def _stim_op_name(self) -> str:
        result = ''
        if self.measure:
            result += "M"
        if self.reset:
            result += "R"
        if self.basis != 'Z':
            result += self.basis
        return result

    def _stim_conversion_(
            self,
            edit_circuit: stim.Circuit,
            targets: List[int],
            **kwargs):
        if self.invert_measure:
            targets[0] = stim.target_inv(targets[0])
        if self.measure_flip_probability:
            edit_circuit.append_operation(self._stim_op_name(), targets, self.measure_flip_probability)
        else:
            edit_circuit.append_operation(self._stim_op_name(), targets)

    def __str__(self) -> str:
        result = self._stim_op_name()
        if self.invert_measure:
            result = "!" + result
        if self.measure:
            result += f"('{self.key}')"
        if self.measure_flip_probability:
            result += f"^{self.measure_flip_probability:%}"
        return result

    def __repr__(self):
        return (f'stimcirq.MeasureAndOrReset('
                f'measure={self.measure!r}, '
                f'reset={self.reset!r}, '
                f'basis={self.basis!r}, '
                f'invert_measure={self.invert_measure!r}, '
                f'key={self.key!r}, '
                f'measure_flip_probability={self.measure_flip_probability!r})')


def args_to_cirq_depolarize_1(args: List[float]) -> cirq.AsymmetricDepolarizingChannel:
    if len(args) != 3:
        raise ValueError(f"len(args) != 3: {args!r}")
    return cirq.AsymmetricDepolarizingChannel(p_x=args[0], p_y=args[1], p_z=args[2])


def thrower(message) -> Callable[[Any], None]:
    def f(*args, **kwargs):
        raise NotImplementedError(message)
    return f


def _opt(args: Union[float, List[float]]) -> float:
    if not args:
        return 0
    return args if isinstance(args, float) else args[0]


@functools.lru_cache(maxsize=1)
def stim_to_cirq_gate_table() -> Dict[str, Union[Tuple, cirq.Gate, Callable[[Union[float, List[float]]], cirq.Gate]]]:
    return {
        "R": cirq.ResetChannel(),
        "RX": MeasureAndOrReset(measure=False, reset=True, basis='X', invert_measure=False, key='', measure_flip_probability=0),
        "RY": MeasureAndOrReset(measure=False, reset=True, basis='Y', invert_measure=False, key='', measure_flip_probability=0),
        "RZ": cirq.ResetChannel(),
        "M": lambda args: cirq.MeasurementGate(num_qubits=1, key='0') if not _opt(args) else MeasureAndOrReset(measure=True, reset=False, basis='Z', invert_measure=False, key='0', measure_flip_probability=_opt(args)),
        "MX": lambda args: MeasureAndOrReset(measure=True, reset=False, basis='X', invert_measure=False, key='0', measure_flip_probability=_opt(args)),
        "MY": lambda args: MeasureAndOrReset(measure=True, reset=False, basis='Y', invert_measure=False, key='0', measure_flip_probability=_opt(args)),
        "MZ": lambda args: cirq.MeasurementGate(num_qubits=1, key='0') if not _opt(args) else MeasureAndOrReset(measure=True, reset=False, basis='Z', invert_measure=False, key='0', measure_flip_probability=_opt(args)),
        "MR": lambda args: MeasureAndOrReset(measure=True, reset=True, basis='Z', invert_measure=False, key='0', measure_flip_probability=_opt(args)),
        "MRX": lambda args: MeasureAndOrReset(measure=True, reset=True, basis='X', invert_measure=False, key='0', measure_flip_probability=_opt(args)),
        "MRY": lambda args: MeasureAndOrReset(measure=True, reset=True, basis='Y', invert_measure=False, key='0', measure_flip_probability=_opt(args)),
        "MRZ": lambda args: MeasureAndOrReset(measure=True, reset=True, basis='Z', invert_measure=False, key='0', measure_flip_probability=_opt(args)),
        "I": cirq.I,
        "X": cirq.X,
        "Y": cirq.Y,
        "Z": cirq.Z,
        "H_XY": cirq.SingleQubitCliffordGate.from_xz_map(x_to=(cirq.Y, False), z_to=(cirq.Z, True)),
        "H": cirq.H,
        "H_XZ": cirq.H,
        "H_YZ": cirq.SingleQubitCliffordGate.from_xz_map(x_to=(cirq.X, True), z_to=(cirq.Y, False)),
        "SQRT_X": cirq.X**0.5,
        "SQRT_X_DAG": cirq.X**-0.5,
        "SQRT_Y": cirq.Y**0.5,
        "SQRT_Y_DAG": cirq.Y**-0.5,
        "C_XYZ": cirq.SingleQubitCliffordGate.from_xz_map(x_to=(cirq.Y, False), z_to=(cirq.X, False)),
        "C_ZYX": cirq.SingleQubitCliffordGate.from_xz_map(x_to=(cirq.Z, False), z_to=(cirq.Y, False)),
        "SQRT_XX": cirq.XX**0.5,
        "SQRT_YY": cirq.YY**0.5,
        "SQRT_ZZ": cirq.ZZ**0.5,
        "SQRT_XX_DAG": cirq.XX**-0.5,
        "SQRT_YY_DAG": cirq.YY**-0.5,
        "SQRT_ZZ_DAG": cirq.ZZ**-0.5,
        "S": cirq.S,
        "S_DAG": cirq.S**-1,
        "SQRT_Z": cirq.S,
        "SQRT_Z_DAG": cirq.S**-1,
        "SWAP": cirq.SWAP,
        "ISWAP": cirq.ISWAP,
        "ISWAP_DAG": cirq.ISWAP**-1,
        "XCX": cirq.PauliInteractionGate(cirq.X, False, cirq.X, False),
        "XCY": cirq.PauliInteractionGate(cirq.X, False, cirq.Y, False),
        "XCZ": cirq.PauliInteractionGate(cirq.X, False, cirq.Z, False),
        "YCX": cirq.PauliInteractionGate(cirq.Y, False, cirq.X, False),
        "YCY": cirq.PauliInteractionGate(cirq.Y, False, cirq.Y, False),
        "YCZ": cirq.PauliInteractionGate(cirq.Y, False, cirq.Z, False),
        "CX": cirq.CNOT,
        "CNOT": cirq.CNOT,
        "ZCX": cirq.CNOT,
        "CY": cirq.Y.controlled(1),
        "ZCY": cirq.Y.controlled(1),
        "CZ": cirq.CZ,
        "ZCZ": cirq.CZ,
        "DEPOLARIZE1": lambda arg: cirq.DepolarizingChannel(arg, 1),
        "DEPOLARIZE2": lambda arg: cirq.DepolarizingChannel(arg, 2),
        "X_ERROR": cirq.X.with_probability,
        "Y_ERROR": cirq.Y.with_probability,
        "Z_ERROR": cirq.Z.with_probability,
        "PAULI_CHANNEL_1": args_to_cirq_depolarize_1,
        "PAULI_CHANNEL_2": lambda args: TwoQubitAsymmetricDepolarizingChannel(args),
        "DETECTOR": (),
        "OBSERVABLE_INCLUDE": (),
        "ELSE_CORRELATED_ERROR": thrower("Converting ELSE_CORRELATED_ERROR to cirq is not supported."),
        "TICK": (),
    }


def not_handled_or_handled_specially_set() -> Set[str]:
    return {"E", "CORRELATED_ERROR", "REPEAT", "ELSE_CORRELATED_ERROR", "QUBIT_COORDS", "SHIFT_COORDS"}


class CoordTracker:
    def __init__(self):
        self.qubit_coords: Dict[int, cirq.Qid] = {}
        self.origin: DefaultDict[float] = collections.defaultdict(float)


def _translate_flattened_operation(
        op: Tuple[str, List, Union[float, Iterable[float]]],
        coord_tracker: CoordTracker,
        get_next_measure_id: Callable[[], int]) -> Iterator[cirq.Operation]:
    name, targets, arg = op

    handler = stim_to_cirq_gate_table().get(name)

    if handler is not None:
        if isinstance(handler, cirq.Gate):
            gate = handler
        elif handler == ():
            return
        else:
            gate = handler(arg)
        for q in targets:
            if isinstance(q, tuple) and q[0] == "rec":
                raise NotImplementedError("Measurement record.")
        if name[0] == 'M':  # Measurement.
            for t in targets:
                if isinstance(t, int):
                    q = t
                    g = gate
                elif t[0] == "inv":
                    q = t[1]
                    g = cast(Union[MeasureAndOrReset, cirq.MeasurementGate], gate).with_bits_flipped(0)
                else:
                    raise NotImplementedError("Unrecognized measurement target.")
                key = str(get_next_measure_id())
                yield g.with_key(key).on(cirq.LineQubit(q))
        else:
            m = cirq.num_qubits(gate)
            for k in range(0, len(targets), m):
                yield gate(*[cirq.LineQubit(q) for q in targets[k:k+m]])
        return

    if name == "E":
        yield cirq.PauliString({cirq.LineQubit(q): k for k, q in targets}).with_probability(arg)
        return

    if name == "QUBIT_COORDS":
        args: List[float] = [arg] if isinstance(arg, float) else arg
        for k in range(len(args)):
            args[k] += coord_tracker.origin[k]
            if args[k] == int(args[k]):
                args[k] = int(args[k])
        for t in targets:
            if len(args) == 1:
                coord_tracker.qubit_coords[t] = cirq.LineQubit(*args)
            elif len(args) == 2:
                coord_tracker.qubit_coords[t] = cirq.GridQubit(*args)
        return

    if name == "SHIFT_COORDS":
        args: List[float] = [arg] if isinstance(arg, float) else arg
        for k, a in enumerate(args):
            coord_tracker.origin[k] += a
        return

    raise NotImplementedError(f"Unsupported gate: {name}")


def stim_circuit_to_cirq_circuit(circuit: stim.Circuit) -> cirq.Circuit:
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
    _next_measure_id = 0

    def get_next_measure_id() -> int:
        nonlocal _next_measure_id
        _next_measure_id += 1
        return _next_measure_id - 1

    coord_tracker = CoordTracker()

    full_circuit = cirq.Circuit()
    current_tick = cirq.Circuit()
    for op in circuit.flattened_operations():
        if op[0] == 'TICK':
            if len(current_tick):
                full_circuit += current_tick
                current_tick = cirq.Circuit()
            else:
                full_circuit += cirq.Moment()
        else:
            current_tick += _translate_flattened_operation(op, coord_tracker, get_next_measure_id)
    full_circuit += current_tick

    if coord_tracker.qubit_coords:
        remap: Dict[cirq.Qid, cirq.Qid] = {
            q: coord_tracker.qubit_coords.get(cast(cirq.LineQubit, q).x, q)
            for q in full_circuit.all_qubits()
        }
        # Only remap if there are no collisions.
        if len(set(remap.values())) == len(remap):
            full_circuit = full_circuit.transform_qubits(remap.__getitem__)

    return full_circuit
