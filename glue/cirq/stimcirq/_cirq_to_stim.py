import functools
import itertools
import math
from typing import Callable, cast, Dict, Iterable, List, Optional, Sequence, Tuple, Type

import cirq
import stim


def cirq_circuit_to_stim_circuit(
    circuit: cirq.Circuit, *, qubit_to_index_dict: Optional[Dict[cirq.Qid, int]] = None
) -> stim.Circuit:
    """Converts a cirq circuit into an equivalent stim circuit.

    Not all circuits can be converted. In order for a circuit to be convertible, all of its operations must be
    convertible.

    An operation is convertible if:
        - It is a stabilizer gate or probabilistic Pauli gate from cirq
            - cirq.H
            - cirq.S
            - cirq.X
            - cirq.X**0.5
            - cirq.CNOT
            - cirq.ResetChannel()
            - cirq.X.with_probability(p)
            - cirq.DepolarizingChannel(p, n_qubits=1 or 2)
            - etc
        - Or it has a _decompose_ method that yields convertible operations.
        - Or it has a correctly implemented _stim_conversion_ method.

    Args:
        circuit: The circuit to convert.
        qubit_to_index_dict: Optional. Which integer each qubit should get mapped to. If not specified, defaults to
            indexing qubits in the circuit in sorted order.

    Returns:
        The converted circuit.

    Examples:
        >>> import cirq, stimcirq
        >>> a = cirq.NamedQubit("zero")
        >>> b = cirq.NamedQubit("two")
        >>> stimcirq.cirq_circuit_to_stim_circuit(cirq.Circuit(
        ...     cirq.Moment(cirq.H(a)),
        ...     cirq.Moment(cirq.CNOT(a, b)),
        ...     cirq.Moment(
        ...         cirq.X(a).with_probability(0.25),
        ...         cirq.Z(b).with_probability(0.25),
        ...     ),
        ...     cirq.Moment(),
        ...     cirq.Moment(),
        ...     cirq.Moment(cirq.DepolarizingChannel(0.125, n_qubits=2).on(b, a)),
        ...     cirq.Moment(cirq.measure(a, b)),
        ... ), qubit_to_index_dict={a: 0, b: 2})
        stim.Circuit('''
            H 0
            TICK
            CX 0 2
            TICK
            X_ERROR(0.25) 0
            Z_ERROR(0.25) 2
            TICK
            TICK
            TICK
            DEPOLARIZE2(0.125) 2 0
            TICK
            M 0 2
            TICK
        ''')

    Here is an example of a _stim_conversion_ method:

        def _stim_conversion_(
                self,

                # The stim circuit being built. Add onto it.
                edit_circuit: stim.Circuit,

                # Metadata about measurement groupings needed by stimcirq.StimSampler.
                # If your gate contains a measurement, it has to append how many qubits
                # that measurement measures (and its key) into this list.
                edit_measurement_key_lengths: List[Tuple[str, int]],

                # The indices of qubits the gate is operating on.
                targets: List[int],

                # Forward compatibility with future arguments.
                **kwargs):

            edit_circuit.append_operation("H", targets)
    """
    return cirq_circuit_to_stim_data(circuit, q2i=qubit_to_index_dict, flatten=False)[0]


def cirq_circuit_to_stim_data(
    circuit: cirq.Circuit, *, q2i: Optional[Dict[cirq.Qid, int]] = None, flatten: bool = False,
) -> Tuple[stim.Circuit, List[Tuple[str, int]]]:
    """Converts a Cirq circuit into a Stim circuit and also metadata about where measurements go."""
    if q2i is None:
        q2i = {q: i for i, q in enumerate(sorted(circuit.all_qubits()))}
    helper = CirqToStimHelper()
    helper.q2i = q2i
    helper.flatten = flatten

    for q in sorted(circuit.all_qubits()):
        if isinstance(q, cirq.LineQubit):
            i = q2i[q]
            if i != q.x:
                helper.out.append_operation("QUBIT_COORDS", [i], [q.x])
        elif isinstance(q, cirq.GridQubit):
            helper.out.append_operation("QUBIT_COORDS", [q2i[q]], [q.row, q.col])

    helper.process_moments(circuit)
    return helper.out, helper.key_out


StimTypeHandler = Callable[[stim.Circuit, cirq.Gate, List[int]], None]


@functools.lru_cache(maxsize=1)
def gate_to_stim_append_func() -> Dict[cirq.Gate, Callable[[stim.Circuit, List[int]], None]]:
    """A dictionary mapping specific gate instances to stim circuit appending functions."""
    x = (cirq.X, False)
    y = (cirq.Y, False)
    z = (cirq.Z, False)
    nx = (cirq.X, True)
    ny = (cirq.Y, True)
    nz = (cirq.Z, True)

    def do_nothing(c, t):
        pass

    def use(
        *gates: str, individuals: Sequence[Tuple[str, int]] = ()
    ) -> Callable[[stim.Circuit, List[int]], None]:
        if len(gates) == 1 and not individuals:
            (g,) = gates
            return lambda c, t: c.append_operation(g, t)

        if not individuals:

            def do(c, t):
                for g in gates:
                    c.append_operation(g, t)

        else:

            def do(c, t):
                for g in gates:
                    c.append_operation(g, t)
                for g, k in individuals:
                    c.append_operation(g, [t[k]])

        return do

    sqcg = cirq.SingleQubitCliffordGate.from_xz_map
    paulis = cast(List[cirq.Pauli], [cirq.X, cirq.Y, cirq.Z])

    return {
        cirq.ResetChannel(): use("R"),
        # Identities.
        cirq.I: use("I"),
        cirq.H ** 0: do_nothing,
        cirq.X ** 0: do_nothing,
        cirq.Y ** 0: do_nothing,
        cirq.Z ** 0: do_nothing,
        cirq.ISWAP ** 0: do_nothing,
        cirq.SWAP ** 0: do_nothing,
        # Common named gates.
        cirq.H: use("H"),
        cirq.X: use("X"),
        cirq.Y: use("Y"),
        cirq.Z: use("Z"),
        cirq.X ** 0.5: use("SQRT_X"),
        cirq.X ** -0.5: use("SQRT_X_DAG"),
        cirq.Y ** 0.5: use("SQRT_Y"),
        cirq.Y ** -0.5: use("SQRT_Y_DAG"),
        cirq.Z ** 0.5: use("SQRT_Z"),
        cirq.Z ** -0.5: use("SQRT_Z_DAG"),
        cirq.CNOT: use("CNOT"),
        cirq.CZ: use("CZ"),
        cirq.ISWAP: use("ISWAP"),
        cirq.ISWAP ** -1: use("ISWAP_DAG"),
        cirq.ISWAP ** 2: use("Z"),
        cirq.SWAP: use("SWAP"),
        cirq.X.controlled(1): use("CX"),
        cirq.Y.controlled(1): use("CY"),
        cirq.Z.controlled(1): use("CZ"),
        cirq.XX ** 0.5: use("SQRT_XX"),
        cirq.YY ** 0.5: use("SQRT_YY"),
        cirq.ZZ ** 0.5: use("SQRT_ZZ"),
        cirq.XX ** -0.5: use("SQRT_XX_DAG"),
        cirq.YY ** -0.5: use("SQRT_YY_DAG"),
        cirq.ZZ ** -0.5: use("SQRT_ZZ_DAG"),
        # All 24 cirq.SingleQubitCliffordGate instances.
        sqcg(x, y): use("SQRT_X_DAG"),
        sqcg(x, ny): use("SQRT_X"),
        sqcg(nx, y): use("H_YZ"),
        sqcg(nx, ny): use("H_YZ", "X"),
        sqcg(x, z): do_nothing,
        sqcg(x, nz): use("X"),
        sqcg(nx, z): use("Z"),
        sqcg(nx, nz): use("Y"),
        sqcg(y, x): use("C_XYZ"),
        sqcg(y, nx): use("S", "SQRT_Y_DAG"),
        sqcg(ny, x): use("S_DAG", "SQRT_Y"),
        sqcg(ny, nx): use("S_DAG", "SQRT_Y_DAG"),
        sqcg(y, z): use("S"),
        sqcg(y, nz): use("H_XY"),
        sqcg(ny, z): use("S_DAG"),
        sqcg(ny, nz): use("H_XY", "Z"),
        sqcg(z, x): use("H"),
        sqcg(z, nx): use("SQRT_Y_DAG"),
        sqcg(nz, x): use("SQRT_Y"),
        sqcg(nz, nx): use("H", "Y"),
        sqcg(z, y): use("C_ZYX"),
        sqcg(z, ny): use("SQRT_Y_DAG", "S"),
        sqcg(nz, y): use("SQRT_Y", "S"),
        sqcg(nz, ny): use("SQRT_Y", "S_DAG"),
        # All 36 cirq.PauliInteractionGate instances.
        **{
            cirq.PauliInteractionGate(p0, s0, p1, s1): use(
                f"{p0}C{p1}", individuals=[(str(p1), 1)] * s0 + [(str(p0), 0)] * s1
            )
            for p0, s0, p1, s1 in itertools.product(paulis, [False, True], repeat=2)
        },
    }


@functools.lru_cache()
def gate_type_to_stim_append_func() -> Dict[Type[cirq.Gate], StimTypeHandler]:
    """A dictionary mapping specific gate types to stim circuit appending functions."""
    return {
        cirq.ControlledGate: cast(StimTypeHandler, _stim_append_controlled_gate),
        cirq.DensePauliString: cast(StimTypeHandler, _stim_append_dense_pauli_string_gate),
        cirq.MutableDensePauliString: cast(StimTypeHandler, _stim_append_dense_pauli_string_gate),
        cirq.AsymmetricDepolarizingChannel: cast(
            StimTypeHandler, _stim_append_asymmetric_depolarizing_channel
        ),
        cirq.BitFlipChannel: lambda c, g, t: c.append_operation(
            "X_ERROR", t, cast(cirq.BitFlipChannel, g).p
        ),
        cirq.PhaseFlipChannel: lambda c, g, t: c.append_operation(
            "Z_ERROR", t, cast(cirq.PhaseFlipChannel, g).p
        ),
        cirq.PhaseDampingChannel: lambda c, g, t: c.append_operation(
            "Z_ERROR", t, 0.5 - math.sqrt(1 - cast(cirq.PhaseDampingChannel, g).gamma) / 2
        ),
        cirq.RandomGateChannel: cast(StimTypeHandler, _stim_append_random_gate_channel),
        cirq.DepolarizingChannel: cast(StimTypeHandler, _stim_append_depolarizing_channel),
    }


def _stim_append_measurement_gate(
    circuit: stim.Circuit, gate: cirq.MeasurementGate, targets: List[int]
):
    for i, b in enumerate(gate.invert_mask):
        if b:
            targets[i] = stim.target_inv(targets[i])
    circuit.append_operation("M", targets)


def _stim_append_pauli_measurement_gate(
    circuit: stim.Circuit, gate: cirq.PauliMeasurementGate, targets: List[int]
):
    obs: cirq.DensePauliString = gate.observable()

    # Convert to stim Pauli product targets.
    if len(targets) == 0:
        raise NotImplementedError(f"len(targets)={len(targets)} == 0")
    new_targets = []
    for t, p in zip(targets, obs.pauli_mask):
        if p == 1:
            t = stim.target_x(t)
        elif p == 2:
            t = stim.target_y(t)
        elif p == 3:
            t = stim.target_z(t)
        else:
            raise NotImplementedError(f"obs={obs!r}")
        new_targets.append(t)
        new_targets.append(stim.target_combiner())
    new_targets.pop()

    # Inverted result?
    if obs.coefficient == -1:
        new_targets[0] |= stim.target_inv(new_targets[0])
        pass
    elif obs.coefficient != 1:
        raise NotImplementedError(f"obs.coefficient={obs.coefficient!r} not in [1, -1]")

    circuit.append_operation("MPP", new_targets)


def _stim_append_dense_pauli_string_gate(
    c: stim.Circuit, g: cirq.BaseDensePauliString, t: List[int]
):
    gates = [None, "X", "Y", "Z"]
    for p, k in zip(g.pauli_mask, t):
        if p:
            c.append_operation(gates[p], [k])


def _stim_append_asymmetric_depolarizing_channel(
    c: stim.Circuit, g: cirq.AsymmetricDepolarizingChannel, t: List[int]
):
    c.append_operation("PAULI_CHANNEL_1", t, [g.p_x, g.p_y, g.p_z])


def _stim_append_depolarizing_channel(c: stim.Circuit, g: cirq.DepolarizingChannel, t: List[int]):
    if g.num_qubits() == 1:
        c.append_operation("DEPOLARIZE1", t, g.p)
    elif g.num_qubits() == 2:
        c.append_operation("DEPOLARIZE2", t, g.p)
    else:
        raise TypeError(f"Don't know how to turn {g!r} into Stim operations.")


def _stim_append_controlled_gate(c: stim.Circuit, g: cirq.ControlledGate, t: List[int]):
    if isinstance(g.sub_gate, cirq.BaseDensePauliString) and g.num_controls() == 1:
        gates = [None, "CX", "CY", "CZ"]
        for p, k in zip(g.sub_gate.pauli_mask, t[1:]):
            if p:
                c.append_operation(gates[p], [t[0], k])
        if g.sub_gate.coefficient == 1j:
            c.append_operation("S", t[:1])
        elif g.sub_gate.coefficient == -1:
            c.append_operation("Z", t[:1])
        elif g.sub_gate.coefficient == -1j:
            c.append_operation("S_DAG", t[:1])
        elif g.sub_gate.coefficient == 1:
            pass
        else:
            raise TypeError(f"Phase kickback from {g!r} isn't a stabilizer operation.")
        return

    raise TypeError(f"Don't know how to turn controlled gate {g!r} into Stim operations.")


def _stim_append_random_gate_channel(c: stim.Circuit, g: cirq.RandomGateChannel, t: List[int]):
    if g.sub_gate in [cirq.X, cirq.Y, cirq.Z]:
        c.append_operation(f"{g.sub_gate}_ERROR", t, g.probability)
    elif isinstance(g.sub_gate, cirq.DensePauliString):
        target_p = [None, stim.target_x, stim.target_y, stim.target_z]
        pauli_targets = [target_p[p](t) for t, p in zip(t, g.sub_gate.pauli_mask) if p]
        c.append_operation(f"CORRELATED_ERROR", pauli_targets, g.probability)
    else:
        raise NotImplementedError(
            f"Don't know how to turn probabilistic {g!r} into Stim operations."
        )


class CirqToStimHelper:
    def __init__(self):
        self.key_out: List[Tuple[str, int]] = []
        self.out = stim.Circuit()
        self.q2i = {}
        self.have_seen_loop = False
        self.flatten = False

    def process_circuit_operation_into_repeat_block(self, op: cirq.CircuitOperation) -> None:
        if self.flatten or op.repetitions == 1:
            self.process_operations(cirq.decompose_once(op))
            return

        child = CirqToStimHelper()
        child.key_out = self.key_out
        child.q2i = self.q2i
        child.have_seen_loop = True
        self.have_seen_loop = True
        child.process_moments(op.transform_qubits(lambda q: op.qubit_map.get(q, q)).circuit)
        self.out += child.out * op.repetitions

    def process_operations(self, operations: Iterable[cirq.Operation]) -> None:
        g2f = gate_to_stim_append_func()
        t2f = gate_type_to_stim_append_func()
        for op in operations:
            assert isinstance(op, cirq.Operation)
            gate = op.gate
            targets = [self.q2i[q] for q in op.qubits]

            custom_method = getattr(
                op, '_stim_conversion_', getattr(gate, '_stim_conversion_', None)
            )
            if custom_method is not None:
                custom_method(
                    dont_forget_your_star_star_kwargs=True,
                    edit_circuit=self.out,
                    edit_measurement_key_lengths=self.key_out,
                    targets=targets,
                    have_seen_loop=self.have_seen_loop,
                )
                continue

            if isinstance(op, cirq.CircuitOperation):
                self.process_circuit_operation_into_repeat_block(op)
                continue

            # Special case measurement, because of its metadata.
            if isinstance(gate, cirq.PauliMeasurementGate):
                self.key_out.append((gate.key, len(targets)))
                _stim_append_pauli_measurement_gate(self.out, gate, targets)
                continue
            if isinstance(gate, cirq.MeasurementGate):
                self.key_out.append((gate.key, len(targets)))
                _stim_append_measurement_gate(self.out, gate, targets)
                continue

            # Look for recognized gate values like cirq.H.
            val_append_func = g2f.get(gate)
            if val_append_func is not None:
                val_append_func(self.out, targets)
                continue

            # Look for recognized gate types like cirq.DepolarizingChannel.
            type_append_func = t2f.get(type(gate))
            if type_append_func is not None:
                type_append_func(self.out, gate, targets)
                continue

            # Ask unrecognized operations to decompose themselves into simpler operations.
            try:
                self.process_operations(cirq.decompose_once(op))
            except TypeError as ex:
                raise TypeError(
                    f"Don't know how to translate {op!r} into stim gates.\n"
                    f"- It doesn't have a _decompose_ method that returns stim-compatible operations.\n"
                    f"- It doesn't have a _stim_conversion_ method.\n"
                ) from ex

    def process_moment(self, moment: cirq.Moment):
        length_before = len(self.out)
        self.process_operations(moment)

        # Append a TICK, unless it was already handled by an internal REPEAT block.
        if length_before == len(self.out) or not isinstance(self.out[-1], stim.CircuitRepeatBlock):
            self.out.append_operation("TICK", [])

    def process_moments(self, moments: Iterable[cirq.Moment]):
        for moment in moments:
            self.process_moment(moment)
