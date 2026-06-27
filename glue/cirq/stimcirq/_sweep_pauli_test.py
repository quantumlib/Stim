import cirq
import numpy as np
import stim
import stimcirq


def test_repr():
    cirq.testing.assert_equivalent_repr(
        stimcirq.SweepPauli(stim_sweep_bit_index=1, pauli=cirq.X, cirq_sweep_symbol="a"),
        global_vals={"stimcirq": stimcirq},
    )
    cirq.testing.assert_equivalent_repr(
        stimcirq.SweepPauli(stim_sweep_bit_index=2, pauli=cirq.Y, cirq_sweep_symbol="b"),
        global_vals={"stimcirq": stimcirq},
    )


def test_stim_conversion():
    actual = stimcirq.cirq_circuit_to_stim_circuit(
        cirq.Circuit(
            stimcirq.SweepPauli(
                stim_sweep_bit_index=5, pauli=cirq.X, cirq_sweep_symbol="init_66"
            ).on(cirq.GridQubit(1, 3)),
            stimcirq.SweepPauli(
                stim_sweep_bit_index=7, pauli=cirq.Y, cirq_sweep_symbol="init_63"
            ).on(cirq.GridQubit(2, 4)),
            stimcirq.SweepPauli(
                stim_sweep_bit_index=5, pauli=cirq.Z, cirq_sweep_symbol="init_66"
            ).on(cirq.GridQubit(2, 4)),
        )
    )
    assert actual == stim.Circuit(
        """
        QUBIT_COORDS(1, 3) 0
        QUBIT_COORDS(2, 4) 1
        CX sweep[5] 0
        CY sweep[7] 1
        TICK
        CZ sweep[5] 1
        TICK
    """
    )

def test_resolving_without_sweep_resolver():
    cirq_circuit = cirq.Circuit(
        stimcirq.SweepPauli(
            stim_sweep_bit_index=0, pauli=cirq.X, cirq_sweep_symbol="init_0"
        ).on(cirq.LineQubit(0)),
        stimcirq.SweepPauli(
            stim_sweep_bit_index=1, pauli=cirq.X, cirq_sweep_symbol="init_1"
        ).on(cirq.LineQubit(1)),
    )
    # Resolve the cirq circuit without specifying the sweep_symbol (this should leave the op 
    # unchanged).
    resolved_circuit = cirq.resolve_parameters(cirq_circuit, {"init_1":1})
    op0, op1, = resolved_circuit.all_operations()
    assert isinstance(op0.gate, stimcirq.SweepPauli) # Operation should still be a SweepPauli
    assert isinstance(op1.gate, type(cirq.X)) # Operation should be converted to a cirq.X
    # Conversion to stim still possible
    assert stimcirq.cirq_circuit_to_stim_circuit(resolved_circuit) 

def test_parameter_names():
    cirq_circuit = cirq.Circuit(
        stimcirq.SweepPauli(
            stim_sweep_bit_index=0, pauli=cirq.X, cirq_sweep_symbol="init_0"
        ).on(cirq.LineQubit(0)),
        stimcirq.SweepPauli(
            stim_sweep_bit_index=1, pauli=cirq.X, cirq_sweep_symbol="init_1"
        ).on(cirq.LineQubit(1)),
    )
    assert cirq.parameter_names(cirq_circuit) == {'init_0', 'init_1'} 


def test_cirq_compatibility():
    circuit = cirq.Circuit(
        stimcirq.SweepPauli(stim_sweep_bit_index=5, pauli=cirq.X, cirq_sweep_symbol="xx").on(
            cirq.LineQubit(0)
        ),
        cirq.measure(cirq.LineQubit(0), key="k"),
    )
    np.testing.assert_array_equal(
        cirq.Simulator().sample(circuit, params={"xx": 0}, repetitions=3)["k"], [0, 0, 0]
    )
    np.testing.assert_array_equal(
        cirq.Simulator().sample(circuit, params={"xx": 1}, repetitions=3)["k"], [1, 1, 1]
    )

    circuit = cirq.Circuit(
        stimcirq.SweepPauli(stim_sweep_bit_index=5, pauli=cirq.Z, cirq_sweep_symbol="xx").on(
            cirq.LineQubit(0)
        ),
        cirq.measure(cirq.LineQubit(0), key="k"),
    )
    np.testing.assert_array_equal(
        cirq.Simulator().sample(circuit, params={"xx": 0}, repetitions=3)["k"], [0, 0, 0]
    )
    np.testing.assert_array_equal(
        cirq.Simulator().sample(circuit, params={"xx": 1}, repetitions=3)["k"], [0, 0, 0]
    )

    circuit = cirq.Circuit(
        cirq.H(cirq.LineQubit(0)),
        stimcirq.SweepPauli(stim_sweep_bit_index=5, pauli=cirq.Z, cirq_sweep_symbol="xx").on(
            cirq.LineQubit(0)
        ),
        cirq.H(cirq.LineQubit(0)),
        cirq.measure(cirq.LineQubit(0), key="k"),
    )
    np.testing.assert_array_equal(
        cirq.Simulator().sample(circuit, params={"xx": 0}, repetitions=3)["k"], [0, 0, 0]
    )
    np.testing.assert_array_equal(
        cirq.Simulator().sample(circuit, params={"xx": 1}, repetitions=3)["k"], [1, 1, 1]
    )


def test_json_serialization():
    c = cirq.Circuit(
        stimcirq.SweepPauli(stim_sweep_bit_index=5, pauli=cirq.X, cirq_sweep_symbol="init_66").on(
            cirq.GridQubit(1, 3)
        ),
        stimcirq.SweepPauli(stim_sweep_bit_index=7, pauli=cirq.Y, cirq_sweep_symbol="init_63").on(
            cirq.GridQubit(2, 4)
        ),
    )
    json = cirq.to_json(c)
    c2 = cirq.read_json(json_text=json, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER])
    assert c == c2


def test_json_backwards_compat_exact():
    raw = stimcirq.SweepPauli(stim_sweep_bit_index=2, pauli=cirq.Z, cirq_sweep_symbol="abc")
    packed = '{\n  "cirq_type": "SweepPauli",\n  "pauli": {\n    "cirq_type": "_PauliZ",\n    "exponent": 1.0,\n    "global_shift": 0.0\n  },\n  "stim_sweep_bit_index": 2,\n  "cirq_sweep_symbol": "abc"\n}'
    assert cirq.read_json(json_text=packed, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER]) == raw
    assert cirq.to_json(raw) == packed
