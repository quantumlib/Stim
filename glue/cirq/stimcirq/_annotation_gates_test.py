import cirq
import numpy as np
import pytest
import stimcirq


def _annotations():
    a, b = cirq.LineQubit.range(2)
    return [
        stimcirq.DetAnnotation(parity_keys=["a"], coordinate_metadata=(1, 2)),
        stimcirq.ShiftCoordsAnnotation([1, 2, 3]),
        stimcirq.CumulativeObservableAnnotation(parity_keys=["a"], observable_index=0),
        stimcirq.CumulativeObservableAnnotation(
            parity_keys=["a"], pauli_keys=[(a, "X0"), (b, "Z1")], observable_index=1
        ),
    ]


_ANNOTATION_TYPE_TO_GATE_TYPE = {
    stimcirq.DetAnnotation: stimcirq.DetAnnotationGate,
    stimcirq.ShiftCoordsAnnotation: stimcirq.ShiftCoordsAnnotationGate,
    stimcirq.CumulativeObservableAnnotation: stimcirq.CumulativeObservableAnnotationGate,
}


@pytest.mark.parametrize("annotation", _annotations())
def test_gate_property(annotation):
    gate = annotation.gate
    assert isinstance(gate, cirq.Gate)
    assert not isinstance(gate, cirq.Operation)
    assert type(gate) is _ANNOTATION_TYPE_TO_GATE_TYPE[type(annotation)]
    assert gate.annotation == annotation
    assert gate == annotation.gate
    assert cirq.num_qubits(gate) == len(annotation.qubits)
    assert gate.on(*annotation.qubits) == annotation
    assert gate(*annotation.qubits) == annotation
    assert cirq.decompose_once_with_qubits(gate, annotation.qubits) == []
    assert eval(repr(gate), {"stimcirq": stimcirq, "cirq": cirq}) == gate


@pytest.mark.parametrize("annotation", _annotations())
def test_gate_on_wrong_number_of_qubits(annotation):
    qubits = cirq.LineQubit.range(len(annotation.qubits) + 1)
    with pytest.raises(ValueError):
        annotation.gate.on(*qubits)


def test_gate_on_new_qubits():
    a, b, c, d = cirq.LineQubit.range(4)
    obs = stimcirq.CumulativeObservableAnnotation(
        pauli_keys=[(a, "X0"), (b, "Z1")], observable_index=2
    )
    assert obs.gate.on(c, d) == stimcirq.CumulativeObservableAnnotation(
        pauli_keys=[(c, "X0"), (d, "Z1")], observable_index=2
    )


def test_gate_equality():
    eq = cirq.testing.EqualsTester()
    eq.add_equality_group(
        stimcirq.DetAnnotation(parity_keys=["a"]).gate,
        stimcirq.DetAnnotationGate(stimcirq.DetAnnotation(parity_keys=["a"])),
    )
    eq.add_equality_group(stimcirq.DetAnnotation(parity_keys=["b"]).gate)
    eq.add_equality_group(stimcirq.ShiftCoordsAnnotation([1]).gate)
    eq.add_equality_group(
        stimcirq.CumulativeObservableAnnotation(parity_keys=["a"], observable_index=0).gate
    )


@pytest.mark.parametrize("annotation", _annotations())
def test_gateset_membership(annotation):
    gate_type = _ANNOTATION_TYPE_TO_GATE_TYPE[type(annotation)]
    gateset = cirq.Gateset(gate_type)
    assert annotation in gateset
    assert annotation.gate in gateset
    assert annotation.with_tags("tag") in gateset
    assert annotation not in cirq.Gateset(cirq.CZ)
    for other_gate_type in _ANNOTATION_TYPE_TO_GATE_TYPE.values():
        if other_gate_type is not gate_type:
            assert annotation not in cirq.Gateset(other_gate_type)


def test_gateset_validation():
    a = cirq.LineQubit(0)
    circuit = cirq.Circuit(
        cirq.X(a),
        cirq.measure(a, key="a"),
        stimcirq.DetAnnotation(parity_keys=["a"]),
        stimcirq.ShiftCoordsAnnotation([1, 2]),
        stimcirq.CumulativeObservableAnnotation(parity_keys=["a"], observable_index=0),
    )
    gateset = cirq.Gateset(
        cirq.XPowGate,
        cirq.MeasurementGate,
        stimcirq.DetAnnotationGate,
        stimcirq.ShiftCoordsAnnotationGate,
        stimcirq.CumulativeObservableAnnotationGate,
    )
    assert gateset.validate(circuit)
    assert not cirq.Gateset(cirq.XPowGate, cirq.MeasurementGate).validate(circuit)


def test_simulation_still_works():
    a = cirq.LineQubit(0)
    circuit = cirq.Circuit(
        cirq.Moment(cirq.X(a)),
        cirq.Moment(cirq.measure(a, key="a")),
        cirq.Moment(
            stimcirq.DetAnnotation(parity_keys=["a"]),
            stimcirq.ShiftCoordsAnnotation([1, 2]),
            stimcirq.CumulativeObservableAnnotation(parity_keys=["a"], observable_index=0),
        ),
    )
    for sampler in [cirq.Simulator(), stimcirq.StimSampler()]:
        result = sampler.run(circuit, repetitions=3)
        np.testing.assert_array_equal(result.measurements["a"], [[1], [1], [1]])


def test_gate_diagram():
    a, b = cirq.LineQubit.range(2)
    obs = stimcirq.CumulativeObservableAnnotation(
        pauli_keys=[(a, "X0"), (b, "Z1")], observable_index=2
    )
    assert cirq.circuit_diagram_info(obs.gate) == cirq.circuit_diagram_info(obs)


@pytest.mark.parametrize("annotation", _annotations())
def test_gate_json_serialization(annotation):
    gate = annotation.gate
    json = cirq.to_json(gate)
    gate2 = cirq.read_json(
        json_text=json, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER]
    )
    assert gate == gate2
