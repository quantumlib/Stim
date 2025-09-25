# mypy: ignore-errors

from __future__ import annotations

import cirq
import ipywidgets
import pytest
import qiskit
import stim
import traitlets

from crumpy import CircuitWidget


def test_basic_no_exception():
    """CircuitWidget should initialize without errors."""
    CircuitWidget(stim="H 0;", indentCircuitLines=False, curveConnectors=False)


def test_bad_stim_instruction_no_exception():
    """CircuitWidget should not raise an error if given any invalid stim instructions."""
    CircuitWidget(stim="H 0;thisIsNotAnInstruction 123;CNOT 0 1;")


def test_bad_trait_assignment_exception():
    """Assigning a non-string value to a CircuitWidget's stim should raise a TraitError."""
    widg = CircuitWidget()
    with pytest.raises(traitlets.TraitError):
        widg.stim = 123


def test_has_traits():
    """CircuitWidget should have the expected traits and sync metadata."""
    widg = CircuitWidget()
    assert widg.has_trait("stim")
    assert widg.has_trait("indentCircuitLines")
    assert widg.has_trait("curveConnectors")
    assert widg.has_trait("showAnnotationRegions")
    assert widg.trait_metadata("stim", "sync")
    assert widg.trait_metadata("indentCircuitLines", "sync")
    assert widg.trait_metadata("curveConnectors", "sync")
    assert widg.trait_metadata("showAnnotationRegions", "sync")


def test_use_case_dlink():
    """CircuitWidget should update its stim when linked to an IntSlider via ipywidgets.dlink."""

    def sliderToCircuit(slider_val):
        return "H 0;" * int(slider_val)

    circuit = CircuitWidget()
    slider_default_value = 1
    slider = ipywidgets.IntSlider(
        description="# of H gates", value=slider_default_value, min=0, max=30
    )
    ipywidgets.dlink((slider, "value"), (circuit, "stim"), sliderToCircuit)
    assert circuit.stim == sliderToCircuit(slider_default_value)

    new_slider_value = 5
    slider.value = new_slider_value
    assert circuit.stim == sliderToCircuit(new_slider_value)

    new_stim = "Y 0;"
    circuit.stim = new_stim
    assert circuit.stim == new_stim


class Test_CircuitImporting:
    def test_from_cirq_valid(self):
        """CircuitWidget.from_cirq should create a CircuitWidget with the correct circuit when given a valid (stim-transpilable) cirq circuit."""

        q0 = cirq.LineQubit(0)
        q1 = cirq.LineQubit(1)
        cirq_circuit = cirq.Circuit(cirq.H(q0), cirq.CNOT(q0, q1))

        widg = CircuitWidget.from_cirq(cirq_circuit)

        assert "H 0" in widg.stim
        assert "CNOT 0 1" in widg.stim or "CX 0 1" in widg.stim

    def test_from_cirq_invalid(self):
        """CircuitWidget.from_cirq should raise an error when given an invalid (non-stim-transpilable) cirq circuit."""

        q0 = cirq.LineQubit(0)
        q1 = cirq.LineQubit(1)
        cirq_circuit = cirq.Circuit(cirq.H(q0), cirq.CNOT(q0, q1), cirq.rx(2).on(q0))
        with pytest.raises(TypeError):
            CircuitWidget.from_cirq(cirq_circuit)

    def test_from_qiskit_valid(self):
        """CircuitWidget.from_qiskit should create a CircuitWidget with the correct circuit when given a valid (stim-transpilable) qiskit circuit."""

        qc = qiskit.QuantumCircuit(2)
        qc.z(0)
        qc.cx(1, 0)

        widg = CircuitWidget.from_qiskit(qc)

        assert "Z 0" in widg.stim
        assert "CNOT 1 0" in widg.stim or "CX 1 0" in widg.stim

    def test_from_qiskit_invalid(self):
        """CircuitWidget.from_qiskit should raise an error when given an invalid (non-stim-transpilable) qiskit circuit."""

        qc = qiskit.QuantumCircuit(2)
        qc.z(0)
        qc.cx(1, 0)
        qc.ry(1.234, 0)

        with pytest.raises(TypeError):
            CircuitWidget.from_qiskit(qc)

    def test_from_stim(self):
        """CircuitWidget.from_stim should create a CircuitWidget with the given stim circuit."""

        stim_circuit = stim.Circuit("""
        X 1
        CY 0 1
        """)

        widg = CircuitWidget.from_stim(stim_circuit)

        assert "X 1" in widg.stim
        assert "CY 0 1" in widg.stim  # also covers alternate name ZCY
