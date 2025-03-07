import functools
import itertools
import math
from typing import Callable, cast, Dict, Iterable, List, Optional, Sequence, Tuple, Type

import cirq
import stim

from ._ii_gate import IIGate


def _forward_single_str_tag(op: cirq.CircuitOperation) -> str:
    tags = [tag for tag in op.tags if isinstance(tag, str)]
    if len(tags) == 1:
        return tags[0]
    return ""


def cirq_circuit_to_stim_circuit(
    circuit: cirq.AbstractCircuit,
    *,
    qubit_to_index_dict: Optional[Dict[cirq.Qid, int]] = None,
    tag_func: Callable[[cirq.Operation], str] = _forward_single_str_tag,
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
        tag_func: Controls the tag attached to the stim instructions the cirq operation turns
            into. If not specified, defaults to checking for string tags on the circuit operation
            and if there is exactly one string tag then using that tag (otherwise not specifying a
            tag).

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

                # A custom string associated with the operation, which can be tagged
                # onto any operations appended to the stim circuit.
                tag: str,

                # Forward compatibility with future arguments.
                **kwargs):

            edit_circuit.append_operation("H", targets)
    """
    return cirq_circuit_to_stim_data(circuit, q2i=qubit_to_index_dict, flatten=False, tag_func=tag_func)[0]


def cirq_circuit_to_stim_data(
    circuit: cirq.AbstractCircuit,
    *,
    q2i: Optional[Dict[cirq.Qid, int]] = None,
    flatten: bool = False,
    tag_func: Callable[[cirq.Operation], str] = _forward_single_str_tag,
) -> Tuple[stim.Circuit, List[Tuple[str, int]]]:
    """Converts a Cirq circuit into a Stim circuit and also metadata about where measurements go."""
    if q2i is None:
        q2i = {q: i for i, q in enumerate(sorted(circuit.all_qubits()))}
    helper = CirqToStimHelper(tag_func=tag_func)
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


StimTypeHandler = Callable[[stim.Circuit, cirq.Gate, List[int], str], None]


@functools.lru_cache(maxsize=1)
def gate_to_stim_append_func() -> Dict[cirq.Gate, Callable[[stim.Circuit, List[int], str], None]]:
    """A dictionary mapping specific gate instances to stim circuit appending functions."""
    x = (cirq.X, False)
    y = (cirq.Y, False)
    z = (cirq.Z, False)
    nx = (cirq.X, True)
    ny = (cirq.Y, True)
    nz = (cirq.Z, True)

    def do_nothing(_gates, _targets, tag):
        pass

    def use(
        *gates: str, individuals: Sequence[Tuple[str, int]] = ()
    ) -> Callable[[stim.Circuit, List[int], str], None]:
        if len(gates) == 1 and not individuals:
            (g,) = gates
            return lambda c, t, tag: c.append(g, t, tag=tag)

        if not individuals:

            def do(c, t, tag: str):
                for g in gates:
                    c.append(g, t, tag=tag)

        else:

            def do(c, t, tag: str):
                for g in gates:
                    c.append(g, t, tag=tag)
                for g, k in individuals:
                    c.append(g, [t[k]], tag=tag)

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
        sqcg(nx, ny): use("H_NYZ"),
        sqcg(x, z): do_nothing,
        sqcg(x, nz): use("X"),
        sqcg(nx, z): use("Z"),
        sqcg(nx, nz): use("Y"),
        sqcg(y, x): use("C_XYZ"),
        sqcg(y, nx): use("C_XYNZ"),
        sqcg(ny, x): use("C_XNYZ"),
        sqcg(ny, nx): use("C_NXYZ"),
        sqcg(y, z): use("S"),
        sqcg(y, nz): use("H_XY"),
        sqcg(ny, z): use("S_DAG"),
        sqcg(ny, nz): use("H_NXY"),
        sqcg(z, x): use("H"),
        sqcg(z, nx): use("SQRT_Y_DAG"),
        sqcg(nz, x): use("SQRT_Y"),
        sqcg(nz, nx): use("H_NXZ"),
        sqcg(z, y): use("C_ZYX"),
        sqcg(z, ny): use("C_ZNYX"),
        sqcg(nz, y): use("C_ZYNX"),
        sqcg(nz, ny): use("C_NZYX"),
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
        cirq.BitFlipChannel: lambda c, g, t, tag: c.append(
            "X_ERROR", t, cast(cirq.BitFlipChannel, g).p, tag=tag
        ),
        cirq.PhaseFlipChannel: lambda c, g, t, tag: c.append(
            "Z_ERROR", t, cast(cirq.PhaseFlipChannel, g).p, tag=tag
        ),
        cirq.PhaseDampingChannel: lambda c, g, t, tag: c.append(
            "Z_ERROR", t, 0.5 - math.sqrt(1 - cast(cirq.PhaseDampingChannel, g).gamma) / 2, tag=tag
        ),
        cirq.RandomGateChannel: cast(StimTypeHandler, _stim_append_random_gate_channel),
        cirq.DepolarizingChannel: cast(StimTypeHandler, _stim_append_depolarizing_channel),
    }


def _stim_append_measurement_gate(
    circuit: stim.Circuit, gate: cirq.MeasurementGate, targets: List[int], tag: str
):
    for i, b in enumerate(gate.invert_mask):
        if b:
            targets[i] = stim.target_inv(targets[i])
    circuit.append("M", targets, tag=tag)


def _stim_append_pauli_measurement_gate(
    circuit: stim.Circuit, gate: cirq.PauliMeasurementGate, targets: List[int], tag: str
):
    obs: cirq.DensePauliString = gate.observable()

    # Convert to stim Pauli product targets.
    if len(targets) == 0:
        raise NotImplementedError(f"len(targets)={len(targets)} == 0")
    new_targets = []
    for t, p in zip(targets, obs.pauli_mask):
        if p == 1:
            t = stim.target_x(t, invert=not new_targets and obs.coefficient == -1)
        elif p == 2:
            t = stim.target_y(t, invert=not new_targets and obs.coefficient == -1)
        elif p == 3:
            t = stim.target_z(t, invert=not new_targets and obs.coefficient == -1)
        else:
            raise NotImplementedError(f"obs={obs!r}")
        new_targets.append(t)
        new_targets.append(stim.target_combiner())
    new_targets.pop()

    # Inverted result?
    if obs.coefficient != 1 and obs.coefficient != -1:
        raise NotImplementedError(f"obs.coefficient={obs.coefficient!r} not in [1, -1]")

    circuit.append("MPP", new_targets, tag=tag)


def _stim_append_spp_gate(
    circuit: stim.Circuit, gate: cirq.PauliStringPhasorGate, targets: List[int], tag: str
):
    obs: cirq.DensePauliString = gate.dense_pauli_string
    a = gate.exponent_neg
    b = gate.exponent_pos
    d = (a - b) % 2
    if obs.coefficient == -1:
        d += 1
        d %= 2
    if d != 0.5 and d != 1.5:
        return False

    new_targets = []
    for t, p in zip(targets, obs.pauli_mask):
        if p:
            new_targets.append(stim.target_pauli(t, p))
            new_targets.append(stim.target_combiner())
    if len(new_targets) == 0:
        return False
    new_targets.pop()

    circuit.append("SPP" if d == 0.5 else "SPP_DAG", new_targets, tag=tag)
    return True


def _stim_append_dense_pauli_string_gate(
    c: stim.Circuit, g: cirq.BaseDensePauliString, t: List[int], tag: str
):
    gates = [None, "X", "Y", "Z"]
    for p, k in zip(g.pauli_mask, t):
        if p:
            c.append(gates[p], [k], tag=tag)


def _stim_append_asymmetric_depolarizing_channel(
    c: stim.Circuit, g: cirq.AsymmetricDepolarizingChannel, t: List[int], tag: str
):
    if cirq.num_qubits(g) == 1:
        c.append("PAULI_CHANNEL_1", t, [g.p_x, g.p_y, g.p_z], tag=tag)
    elif cirq.num_qubits(g) == 2:
        c.append(
            "PAULI_CHANNEL_2",
            t,
            [
                g.error_probabilities.get('IX', 0),
                g.error_probabilities.get('IY', 0),
                g.error_probabilities.get('IZ', 0),
                g.error_probabilities.get('XI', 0),
                g.error_probabilities.get('XX', 0),
                g.error_probabilities.get('XY', 0),
                g.error_probabilities.get('XZ', 0),
                g.error_probabilities.get('YI', 0),
                g.error_probabilities.get('YX', 0),
                g.error_probabilities.get('YY', 0),
                g.error_probabilities.get('YZ', 0),
                g.error_probabilities.get('ZI', 0),
                g.error_probabilities.get('ZX', 0),
                g.error_probabilities.get('ZY', 0),
                g.error_probabilities.get('ZZ', 0),
            ],
            tag=tag,
        )
    else:
        raise NotImplementedError(f'cirq-to-stim gate {g!r}')


def _stim_append_depolarizing_channel(c: stim.Circuit, g: cirq.DepolarizingChannel, t: List[int], tag: str):
    if g.num_qubits() == 1:
        c.append("DEPOLARIZE1", t, g.p, tag=tag)
    elif g.num_qubits() == 2:
        c.append("DEPOLARIZE2", t, g.p, tag=tag)
    else:
        raise TypeError(f"Don't know how to turn {g!r} into Stim operations.")


def _stim_append_controlled_gate(c: stim.Circuit, g: cirq.ControlledGate, t: List[int], tag: str):
    if isinstance(g.sub_gate, cirq.BaseDensePauliString) and g.num_controls() == 1:
        gates = [None, "CX", "CY", "CZ"]
        for p, k in zip(g.sub_gate.pauli_mask, t[1:]):
            if p:
                c.append(gates[p], [t[0], k], tag=tag)
        if g.sub_gate.coefficient == 1j:
            c.append("S", t[:1], tag=tag)
        elif g.sub_gate.coefficient == -1:
            c.append("Z", t[:1], tag=tag)
        elif g.sub_gate.coefficient == -1j:
            c.append("S_DAG", t[:1], tag=tag)
        elif g.sub_gate.coefficient == 1:
            pass
        else:
            raise TypeError(f"Phase kickback from {g!r} isn't a stabilizer operation.")
        return

    raise TypeError(f"Don't know how to turn controlled gate {g!r} into Stim operations.")


def _stim_append_random_gate_channel(c: stim.Circuit, g: cirq.RandomGateChannel, t: List[int], tag: str):
    if g.sub_gate in [cirq.X, cirq.Y, cirq.Z, cirq.I]:
        c.append(f"{g.sub_gate}_ERROR", t, g.probability, tag=tag)
    elif isinstance(g.sub_gate, IIGate):
        c.append(f"II_ERROR", t, g.probability, tag=tag)
    elif isinstance(g.sub_gate, cirq.DensePauliString):
        target_p = [None, stim.target_x, stim.target_y, stim.target_z]
        pauli_targets = [target_p[p](t) for t, p in zip(t, g.sub_gate.pauli_mask) if p]
        c.append(f"CORRELATED_ERROR", pauli_targets, g.probability, tag=tag)
    else:
        raise NotImplementedError(
            f"Don't know how to turn probabilistic {g!r} into Stim operations."
        )


class CirqToStimHelper:
    def __init__(self, tag_func: Callable[[cirq.Operation], str]):
        self.key_out: List[Tuple[str, int]] = []
        self.out = stim.Circuit()
        self.q2i = {}
        self.have_seen_loop = False
        self.flatten = False
        self.tag_func = tag_func

    def process_circuit_operation_into_repeat_block(self, op: cirq.CircuitOperation, tag: str) -> None:
        if self.flatten or op.repetitions == 1:
            moments = cirq.unroll_circuit_op(cirq.Circuit(op), deep=False, tags_to_check=None).moments
            self.process_moments(moments)
            self.out = self.out[:-1] # Remove a trailing TICK (to avoid double TICK)
            return

        child = CirqToStimHelper(tag_func=self.tag_func)
        child.key_out = self.key_out
        child.q2i = self.q2i
        child.have_seen_loop = True
        self.have_seen_loop = True
        child.process_moments(op.transform_qubits(lambda q: op.qubit_map.get(q, q)).circuit)
        self.out.append(stim.CircuitRepeatBlock(op.repetitions, child.out, tag=tag))

    def process_operations(self, operations: Iterable[cirq.Operation]) -> None:
        g2f = gate_to_stim_append_func()
        t2f = gate_type_to_stim_append_func()
        for op in operations:
            assert isinstance(op, cirq.Operation)
            tag = self.tag_func(op)
            op = op.untagged
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
                    tag=tag,
                )
                continue

            if isinstance(op, cirq.CircuitOperation):
                self.process_circuit_operation_into_repeat_block(op, tag=tag)
                continue

            # Special case measurement, because of its metadata.
            if isinstance(gate, cirq.PauliStringPhasorGate):
                if _stim_append_spp_gate(self.out, gate, targets, tag=tag):
                    continue
            if isinstance(gate, cirq.PauliMeasurementGate):
                self.key_out.append((gate.key, len(targets)))
                _stim_append_pauli_measurement_gate(self.out, gate, targets, tag=tag)
                continue
            if isinstance(gate, cirq.MeasurementGate):
                self.key_out.append((gate.key, len(targets)))
                _stim_append_measurement_gate(self.out, gate, targets, tag=tag)
                continue

            # Look for recognized gate values like cirq.H.
            val_append_func = g2f.get(gate)
            if val_append_func is not None:
                val_append_func(self.out, targets, tag=tag)
                continue

            # Look for recognized gate types like cirq.DepolarizingChannel.
            type_append_func = t2f.get(type(gate))
            if type_append_func is not None:
                type_append_func(self.out, gate, targets, tag=tag)
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
            self.out.append("TICK", [])

    def process_moments(self, moments: Iterable[cirq.Moment]):
        for moment in moments:
            self.process_moment(moment)
