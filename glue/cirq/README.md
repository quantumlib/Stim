# stimcirq

Tools for interop between the quantum python package `cirq` and the stabilizer simulator `stim`.

Includes:

- `stimcirq.StimSampler`
- `stimcirq.cirq_circuit_to_stim_circuit`
- `stimcirq.stim_circuit_to_cirq_circuit`

# Examples

Sampling with `stimcirq.Sampler`:

```python
import cirq
a, b = cirq.LineQubit.range(2)
c = cirq.Circuit(
    cirq.H(a),
    cirq.CNOT(a, b),
    cirq.measure(a, key="a"),
    cirq.measure(b, key="b"),
)

import stimcirq
sampler = stimcirq.StimSampler()
result = sampler.run(c, repetitions=30)

print(result)
# prints something like:
# a=000010100101000011001100110011
# b=000010100101000011001100110011
```

# API Reference

## `stimcirq.StimSampler`

```
A `cirq.Sampler` backed by `stim`.

Supports circuits that contain Clifford operations, measurement operations, reset operations, and noise operations
that can be decomposed into probabilistic Pauli operations. Unknown operations are supported as long as they provide
a decomposition into supported operations via `cirq.decompose` (i.e. via a `_decompose_` method).

Note that batch sampling is significantly faster (as in potentially thousands of times faster) than individual
sampling, because it amortizes the cost of parsing and analyzing the circuit.
```

## `stimcirq.stim_circuit_to_cirq_circuit(circuit: stim.Circuit) -> cirq.Circuit`

```
Converts a stim circuit into an equivalent cirq circuit.

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
```

## `def cirq_circuit_to_stim_circuit(circuit: cirq.Circuit, *, qubit_to_index_dict: Optional[Dict[cirq.Qid, int]] = None) -> stim.Circuit`

```
Converts a cirq circuit into an equivalent stim circuit.

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
```
