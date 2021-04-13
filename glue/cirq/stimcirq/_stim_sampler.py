from typing import Callable, cast, Dict, Iterable, List, Optional, Sequence, Tuple, Type

import functools
import itertools
import math

import cirq
import stim


class StimSampler(cirq.Sampler):
    """Samples stabilizer circuits using Stim.

    Supports circuits that contain Clifford operations, measurement operations, reset operations, and noise operations
    that can be decomposed into probabilistic Pauli operations. Unknown operations are supported as long as they provide
    a decomposition into supported operations via `cirq.decompose` (i.e. via a `_decompose_` method).

    Note that batch sampling is significantly faster (as in potentially thousands of times faster) than individual
    sampling, because it amortizes the cost of parsing and analyzing the circuit.
    """

    def run_sweep(
        self,
        program: cirq.Circuit,
        params: cirq.Sweepable,
        repetitions: int = 1,
    ) -> List[cirq.Result]:
        trial_results: List[cirq.Result] = []
        for param_resolver in cirq.to_resolvers(params):
            # Request samples from stim.
            instance = cirq.resolve_parameters(program, param_resolver)
            converted_circuit, key_ranges = cirq_circuit_to_stim_data(instance)
            samples = converted_circuit.compile_sampler().sample(repetitions)

            # Convert unlabelled samples into keyed results.
            k = 0
            measurements = {}
            for key, length in key_ranges:
                p = k
                k += length
                measurements[key] = samples[:, p:k]
            trial_results.append(cirq.Result(params=param_resolver, measurements=measurements))

        return trial_results


def cirq_circuit_to_stim_circuit(circuit: cirq.Circuit) -> stim.Circuit:
    """Converts a cirq circuit into an equivalent stim circuit.

    Qubits are indexed in sorted order.

    Not all circuits can be converted. In order for a circuit to be convertable,
    all of its operations must be convertable.

    An operation is convertable if:
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
        - Or it has a _decompose_ method that yields convertable operations.
        - Or it has a correctly implemented _stim_conversion_ method.

    Returns:
        The converted circuit.

    Examples:
        >>> import cirq, stimcirq
        >>> a = cirq.NamedQubit("one")
        >>> b = cirq.NamedQubit("two")
        >>> stimcirq.cirq_circuit_to_stim_circuit(cirq.Circuit(
        ...     cirq.H(a),
        ...     cirq.CNOT(a, b),
        ...     cirq.X(a).with_probability(0.25),
        ...     cirq.DepolarizingChannel(0.125, n_qubits=2).on(b, a),
        ...     cirq.measure(a, b),
        ... ))
        stim.Circuit('''
        H 0
        CX 0 1
        X_ERROR(0.25) 0
        DEPOLARIZE2(0.125) 1 0
        M 0 1
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

                # The indices of qubits the gate should target.
                targets: List[int],

                # Forward compatibility with future arguments.
                **kwargs):

            edit_circuit.append_operation("H", targets)
    """
    return cirq_circuit_to_stim_data(circuit)[0]


def cirq_circuit_to_stim_data(
    circuit: cirq.Circuit, *, q2i: Optional[Dict[cirq.Qid, int]] = None
) -> Tuple[stim.Circuit, List[Tuple[str, int]]]:
    """Converts a Cirq circuit into a Stim circuit and also metadata about where measurements go."""
    if q2i is None:
        q2i = {q: i for i, q in enumerate(sorted(circuit.all_qubits()))}
    out = stim.Circuit()
    key_out: List[Tuple[str, int]] = []
    for moment in circuit:
        _c2s_helper(moment, q2i, out, key_out)
        out.append_operation("TICK", [])
    return out, key_out


StimTypeHandler = Callable[[stim.Circuit, cirq.Gate, List[int]], None]


@functools.lru_cache()
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
        # All 24 cirq.SingleQubitCliffordGate instances.
        sqcg(x, y): use("SQRT_X_DAG"),
        sqcg(x, ny): use("SQRT_X"),
        sqcg(nx, y): use("H_YZ"),
        sqcg(nx, ny): use("H_YZ", "X"),
        sqcg(x, z): do_nothing,
        sqcg(x, nz): use("X"),
        sqcg(nx, z): use("Z"),
        sqcg(nx, nz): use("Y"),
        sqcg(y, x): use("S", "SQRT_Y"),
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
        sqcg(z, y): use("SQRT_Y_DAG", "S_DAG"),
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


def _stim_append_measurement_gate(circuit: stim.Circuit, gate: cirq.MeasurementGate, targets: List[int]):
    for i, b in enumerate(gate.invert_mask):
        if b:
            targets[i] = stim.target_inv(targets[i])
    circuit.append_operation("M",  targets)


def _stim_append_dense_pauli_string_gate(c: stim.Circuit, g: cirq.BaseDensePauliString, t: List[int]):
    gates = [None, "X", "Y", "Z"]
    for p, k in zip(g.pauli_mask, t):
        if p:
            c.append_operation(gates[p], [k])


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
        pauli_targets = [
            target_p[p](t)
            for t, p in zip(t, g.sub_gate.pauli_mask)
        ]
        c.append_operation(f"CORRELATED_ERROR", pauli_targets, g.probability)
    else:
        raise NotImplementedError(f"Don't know how to turn probabilistic {g!r} into Stim operations.")


def _c2s_helper(
    operations: Iterable[cirq.Operation], q2i: Dict[cirq.Qid, int], out: stim.Circuit,
    key_out: List[Tuple[str, int]]
):
    g2f = gate_to_stim_append_func()
    t2f = gate_type_to_stim_append_func()
    for op in operations:
        gate = op.gate
        targets = [q2i[q] for q in op.qubits]

        custom_method = getattr(op, '_stim_conversion_', getattr(gate, '_stim_conversion_', None))
        if custom_method is not None:
            custom_method(
                dont_forget_your_star_star_kwargs=True,
                edit_circuit=out,
                edit_measurement_key_lengths=key_out,
                targets=targets)
            continue

        # Special case measurement, because of its metadata.
        if isinstance(gate, cirq.MeasurementGate):
            key_out.append((gate.key, len(targets)))
            _stim_append_measurement_gate(out, gate, targets)
            continue

        # Look for recognized gate values like cirq.H.
        val_append_func = g2f.get(gate)
        if val_append_func is not None:
            val_append_func(out, targets)
            continue

        # Look for recognized gate types like cirq.DepolarizingChannel.
        type_append_func = t2f.get(type(gate))
        if type_append_func is not None:
            type_append_func(out, gate, targets)
            continue

        # Ask unrecognized operations to decompose themselves into simpler operations.
        try:
            _c2s_helper(cirq.decompose_once(op), q2i, out, key_out)
        except TypeError as ex:
            raise TypeError(f"Don't know how to translate {op!r} into stim gates.") from ex

    return out
