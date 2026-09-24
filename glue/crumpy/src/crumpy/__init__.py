"""
Copyright (c) 2025 Riverlane. All rights reserved.

crumpy: A python library for visualizing Crumble circuits in Jupyter
"""

# mypy: ignore-errors
# pylint: disable=abstract-method

from __future__ import annotations

import pathlib
from importlib.metadata import version

import anywidget
import cirq
import qiskit
import qiskit.qasm3
import stim
import stimcirq
import traitlets
from cirq.contrib.qasm_import import circuit_from_qasm

__version__ = version(__name__)

__all__ = ["__version__"]

bundler_output_dir = pathlib.Path(__file__).parent.parent / "js"


class CircuitWidget(anywidget.AnyWidget):
    """A Jupyter widget for displaying Crumble circuit diagrams.

    Attributes
    ----------
    stim : str
      Stim circuit to be drawn.
    indentCircuitLines : bool
      If circuit lines for subsequent qubits in the same row get indented when drawn.
      Defaults to True (matching Crumble).
    curveConnectors : bool
      If connectors (e.g., the line between control and target of a CNOT) can be drawn curved.
      Defaults to True (matching Crumble).
    showAnnotationRegions : bool
      If detector region and observable marks are shown.
      Defaults to True (matching Crumble).
    """

    _esm = bundler_output_dir / "bundle.js"
    stim = traitlets.Unicode(default_value="", help="Stim circuit to be drawn").tag(
        sync=True
    )
    indentCircuitLines = traitlets.Bool(True).tag(sync=True)
    curveConnectors = traitlets.Bool(True).tag(sync=True)
    showAnnotationRegions = traitlets.Bool(True).tag(sync=True)

    @staticmethod
    def from_cirq(cirq_circuit: cirq.Circuit):
        """Create a `CircuitWidget` from a `cirq.Circuit`.

        `cirq_circuit` will be transpiled to a `stim.Circuit` before use with `CircuitWidget`.
        `cirq_circuit` must be transpilable to a `stim.Circuit`.

        Parameters
        ----------
        cirq_circuit : cirq.Circuit
            cirq circuit to visualize

        Raises
        ------
        TypeError
            If `cirq_circuit` is unable to be converted to a `stim.Circuit`
        """
        try:
            stim_circuit = stimcirq.cirq_circuit_to_stim_circuit(cirq_circuit)
            return CircuitWidget(stim=str(stim_circuit))
        except TypeError as ex:
            msg = "Unable to translate cirq circuit to stim."
            raise TypeError(msg) from ex

    @staticmethod
    def from_qiskit(qiskit_circuit: qiskit.QuantumCircuit):
        """Create a `CircuitWidget` from a `qiskit.QuantumCircuit`.

        `qiskit_circuit` will be transpiled to a `stim.Circuit` before use with `CircuitWidget`.
        `qiskit_circuit` must be transpilable to a `stim.Circuit`.

        Parameters
        ----------
        qiskit_circuit : qiskit.QuantumCircuit
            qiskit circuit to visualize

        Raises
        ------
        TypeError
            If `qiskit_circuit` is unable to be converted to a `stim.Circuit`
        """
        try:
            qasm_circuit = qiskit.qasm3.dumps(qiskit_circuit)
            cirq_circuit = circuit_from_qasm(qasm_circuit)
            return CircuitWidget.from_cirq(cirq_circuit)
        except TypeError as ex:
            msg = "Unable to translate qiskit circuit to stim."
            raise TypeError(msg) from ex

    @staticmethod
    def from_stim(stim_circuit: stim.Circuit):
        """Create a `CircuitWidget` from a `stim.Circuit`.

        Parameters
        ----------
        stim_circuit : stim.Circuit
            stim circuit to visualize
        """
        return CircuitWidget(stim=str(stim_circuit))
