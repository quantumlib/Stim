from __future__ import annotations

import functools
from typing import Any, cast, Literal

import stim


def _single_qubit_tableau_to_key(t: stim.Tableau) -> str:
    return f"{t.x_output(0)}{t.z_output(0)}"


@functools.cache
def gate_to_unsigned_pauli_change_inverse() -> (
    dict[str, dict[Literal["X", "Y", "Z"], Literal["X", "Y", "Z"]]]
):
    return {
        gate: {v: k for k, v in items.items()}
        for gate, items in gate_to_unsigned_pauli_change().items()
    }


@functools.cache
def gate_to_unsigned_pauli_change() -> (
    dict[str, dict[Literal["X", "Y", "Z"], Literal["X", "Y", "Z"]]]
):
    result: dict[str, dict[Literal["X", "Y", "Z"], Literal["X", "Y", "Z"]]] = {}
    for vals, gate in keyed_single_qubit_cliffords().items():
        _x_sign: Literal["-", "+"]
        x_out: Literal["X", "Y", "Z"]
        _z_sign: Literal["-", "+"]
        z_out: Literal["X", "Y", "Z"]
        _x_sign, x_out, _z_sign, z_out = cast(Any, vals)
        (y_out,) = set("XYZ") - {x_out, z_out}
        result[gate] = cast(Any, {"X": x_out, "Z": z_out, "Y": y_out})
    return result


@functools.cache
def keyed_single_qubit_cliffords() -> dict[str, str]:
    tableau_to_gate_name = {}

    # Find basic gates.
    for gate in stim.gate_data().values():
        if gate.is_single_qubit_gate and gate.is_unitary:
            tableau_to_gate_name[_single_qubit_tableau_to_key(gate.tableau)] = gate.name

    # Form remaining composite gates.
    for g in ["H", "S", "SQRT_X", "C_XYZ", "C_ZYX"]:
        gt = stim.gate_data(g).tableau
        for p in "XZY":
            pt = stim.gate_data(p).tableau
            k2 = _single_qubit_tableau_to_key(pt * gt)
            if k2 not in tableau_to_gate_name:
                tableau_to_gate_name[k2] = p + "*" + g

    return tableau_to_gate_name


@functools.cache
def single_qubit_clifford_inverse_table() -> dict[str, str]:
    m = keyed_single_qubit_cliffords()
    inverse_table = {}

    for t1 in stim.Tableau.iter_all(num_qubits=1):
        k1 = _single_qubit_tableau_to_key(t1)
        k2 = _single_qubit_tableau_to_key(t1**-1)
        inverse_table[m[k1]] = m[k2]

    return inverse_table


@functools.cache
def single_qubit_clifford_multiplication_table() -> dict[tuple[str, str], str]:
    m = keyed_single_qubit_cliffords()

    # Compute the multiplication table.
    multiplication_table = {}
    tableaus = list(stim.Tableau.iter_all(num_qubits=1))
    for t1 in tableaus:
        k1 = _single_qubit_tableau_to_key(t1)
        g1 = m[k1]
        for t2 in tableaus:
            k2 = _single_qubit_tableau_to_key(t2)
            g2 = m[k2]
            t3 = t1 * t2
            k3 = _single_qubit_tableau_to_key(t3)
            g3 = m[k3]
            multiplication_table[(g1, g2)] = g3

    return multiplication_table
