from __future__ import annotations

import collections
from collections.abc import Callable, Iterable, Mapping, Set
from typing import Any, cast

import numpy as np
import stim

from stimflow._core import Flow, PauliMap, xor_sorted


def _solve_auto_flow_starts(
    *,
    flows: Iterable[Flow],
    circuit: stim.Circuit,
    q2i: dict[complex, int],
    failure_out: list[Flow],
) -> list[Flow]:

    num_qubits = max(circuit.num_qubits, max([i + 1 for i in q2i.values()], default=0))
    i2q = {i: q for q, i in q2i.items()}

    new_flows = []
    for flow in flows:
        stim_end = cast(PauliMap, flow.end).to_stim_pauli_string(q2i, num_qubits=num_qubits)
        try:
            stim_start = stim_end.before(circuit)
        except ValueError:
            failure_out.append(flow)
            continue
        start = PauliMap({i2q[q]: "_XYZ"[stim_start[q]] for q in stim_start.pauli_indices()})
        new_flows.append(flow.with_edits(start=start))

    return new_flows


def _solve_auto_flow_ends(
    *,
    flows: Iterable[Flow],
    circuit: stim.Circuit,
    q2i: dict[complex, int],
    failure_out: list[Flow],
) -> list[Flow]:

    num_qubits = circuit.num_qubits
    i2q = {i: q for q, i in q2i.items()}

    new_flows = []
    for flow in flows:
        stim_start = cast(PauliMap, flow.start).to_stim_pauli_string(q2i, num_qubits=num_qubits)
        try:
            stim_end = stim_start.after(circuit)
        except ValueError:
            failure_out.append(flow)
            continue
        end = PauliMap({i2q[q]: "_XYZ"[stim_end[q]] for q in stim_end.pauli_indices()})
        new_flows.append(flow.with_edits(end=end))

    return new_flows


def _has_obs_include_instructions(circuit: stim.Circuit) -> bool:
    for inst in circuit:
        if inst.name == "OBSERVABLE_INCLUDE":
            return True
        elif inst.name == "REPEAT":
            return _has_obs_include_instructions(inst.body_copy())
    return False


def _solve_auto_flow_ms(
    *,
    flows: Iterable[Flow],
    circuit: stim.Circuit,
    q2i: dict[complex, int],
    o2i: dict[Any, int],
    failure_out: list[Flow],
) -> list[Flow]:
    result: list[Flow] = list(flows)

    stim_flows: list[stim.Flow] = []
    has_obs_with_auto_measurements = False
    sub_o2i: Mapping[Any, int | None] = dict(o2i)
    if not _has_obs_include_instructions(circuit):
        sub_o2i = collections.defaultdict(lambda: None)
    for k, flow in enumerate(result):
        flow = result[k]
        has_obs_with_auto_measurements |= flow.obs_key is not None
        stim_flows.append(flow.to_stim_flow(q2i=q2i, o2i=sub_o2i))

    if has_obs_with_auto_measurements and circuit.num_observables:
        raise NotImplementedError(
            "The circuit contains OBSERVABLE_INCLUDE instructions, "
            "and a flow with auto-solved measurements mentions an observable."
        )

    if stim_flows:
        measurements = circuit.solve_flow_measurements(stim_flows)
        for k in range(len(measurements)):
            if measurements[k] is None:
                failure_out.append(result[k])
                result[k] = result[k].with_edits(measurement_indices=[])
            else:
                result[k] = result[k].with_edits(measurement_indices=measurements[k])

    return result


def mbqc_to_unitary_by_solving_feedback(
    mbqc_circuit: stim.Circuit,
    *,
    desired_flow_generators: list[stim.Flow] | None = None,
    num_relevant_qubits: int,
) -> stim.Circuit:
    """Converts an MBQC circuit to a unitary circuit by adding Pauli feedback.

    Args:
        mbqc_circuit: The circuit to add feedback to.
        desired_flow_generators: Defaults to None (clear all measurement
            dependence and negative signs). When set to a list, it specifies
            reference signs and measurement dependencies to keep.
        num_relevant_qubits: The number of non-ancillary qubits.

    Returns:
        The circuit with added Pauli feedback.
    """
    num_qubits = mbqc_circuit.num_qubits
    num_measurements = mbqc_circuit.num_measurements
    num_added_dof = num_relevant_qubits * 2

    # Add feedback from extra qubits, so `flow_generators` includes X/Z feedback terms.
    augmented_circuit = mbqc_circuit.copy()
    for q in range(num_relevant_qubits):
        augmented_circuit.append("M", [num_qubits + q * 2])
        augmented_circuit.append("CX", [stim.target_rec(-1), q])
        augmented_circuit.append("M", [num_qubits + q * 2 + 1])
        augmented_circuit.append("CZ", [stim.target_rec(-1), q])

    # Diagonalize the flow generators.
    # - Remove terms mentioning ancillary qubits.
    flow_table: list[tuple[stim.PauliString, stim.PauliString, list[int]]] = [
        (f.input_copy(), f.output_copy(), f.measurements_copy())
        for f in augmented_circuit.flow_generators()
    ]
    num_solved_flows = 0
    pivot_funcs = [
        lambda f, i: len(f[0]) > i and 1 <= f[0][i] <= 2,
        lambda f, i: len(f[0]) > i and 2 <= f[0][i] <= 3,
        lambda f, i: len(f[1]) > i and 1 <= f[1][i] <= 2,
        lambda f, i: len(f[1]) > i and 2 <= f[1][i] <= 3,
    ]

    def elim_step(q: int, func: Callable):
        nonlocal num_solved_flows
        for pivot in range(num_solved_flows, len(flow_table)):
            if func(flow_table[pivot], q):
                break
        else:
            return
        for row in range(len(flow_table)):
            if pivot != row and func(flow_table[row], q):
                i1, o1, m1 = flow_table[row]
                i2, o2, m2 = flow_table[pivot]
                flow_table[row] = (i1 * i2, o1 * o2, xor_sorted(m1 + m2))
        if pivot != num_solved_flows:
            flow_table[num_solved_flows], flow_table[pivot] = (
                flow_table[pivot],
                flow_table[num_solved_flows],
            )
        num_solved_flows += 1

    for q in range(num_relevant_qubits, augmented_circuit.num_qubits):
        for func in pivot_funcs:
            elim_step(q, func)
    flow_table = flow_table[num_solved_flows:]
    num_solved_flows = 0
    for q in range(num_relevant_qubits):
        for func in pivot_funcs[:2]:
            elim_step(q, func)
    for q in range(num_relevant_qubits):
        for func in pivot_funcs[2:]:
            elim_step(q, func)

    if desired_flow_generators is not None:
        # TODO: make this work even if the desired generators are in a different basis.
        for k in range(len(desired_flow_generators)):
            i1 = desired_flow_generators[k].input_copy()
            i2 = flow_table[k][0]
            assert (i1 * i2).weight == 0
            o1 = desired_flow_generators[k].output_copy()
            o2 = flow_table[k][1]
            assert (o1 * o2).weight == 0
            flow_table[k] = (
                i1 * i2.sign,
                o1 * o2.sign,
                xor_sorted(flow_table[k][2] + desired_flow_generators[k].measurements_copy()),
            )

    # Construct a feedback table describing how each measurement affects each flow.
    feedback_table: list[np.ndarray] = []
    for g in flow_table:
        i2 = g[0].pauli_indices()
        o2 = g[1].pauli_indices()
        if i2 and i2[0] >= num_relevant_qubits:
            continue
        if o2 and o2[0] >= num_relevant_qubits:
            continue
        row2 = np.zeros(num_measurements + num_added_dof + 1, dtype=np.bool_)
        for k in g[2]:
            row2[k] ^= 1
        row2[-1] ^= g[0].sign * g[1].sign == -1
        feedback_table.append(row2)

    # Diagonalize the feedback table.
    num_solved = 0
    for k in range(num_measurements, num_measurements + num_added_dof):
        for pivot in range(num_solved, len(feedback_table)):
            if feedback_table[pivot][k]:
                break
        else:
            continue
        for row in range(len(feedback_table)):
            if pivot != row and feedback_table[row][k]:
                feedback_table[row] ^= feedback_table[pivot]
        if pivot != num_solved:
            feedback_table[num_solved] ^= feedback_table[pivot]
            feedback_table[pivot] ^= feedback_table[num_solved]
            feedback_table[num_solved] ^= feedback_table[pivot]
        num_solved += 1

    result = mbqc_circuit.copy()

    # Convert from table to dicts.
    cx: collections.defaultdict[int | None, set[int]] = collections.defaultdict(set)
    cz: collections.defaultdict[int | None, set[int]] = collections.defaultdict(set)
    for q in range(num_added_dof):
        assert np.array_equal(np.flatnonzero(feedback_table[q][-num_added_dof - 1 : -1]), [q])
    for q in range(num_relevant_qubits):
        if feedback_table[2 * q + 0][-1]:
            cx[None].add(q)
        for m in np.flatnonzero(feedback_table[2 * q + 0][: -num_added_dof - 1]):
            cx[m - num_measurements].add(q)
    for q in range(num_relevant_qubits):
        if feedback_table[2 * q + 1][-1]:
            cz[None].add(q)
        for m in np.flatnonzero(feedback_table[2 * q + 1][:-num_added_dof]):
            cz[m - num_measurements].add(q)

    # Output deterministic Paulis.
    for q in sorted(cx[None] - cz[None]):
        result.append("X", [q])
    for q in sorted(cx[None] & cz[None]):
        result.append("Y", [q])
    for q in sorted(cz[None] - cx[None]):
        result.append("Z", [q])
    cx.pop(None, None)
    cz.pop(None, None)
    cx_keys = cast(Set[int], cx.keys())
    cz_keys = cast(Set[int], cz.keys())

    # Output feedback Paulis.
    for k in cx_keys:
        for q in sorted(cx[k] - cz[k]):
            result.append("CX", [stim.target_rec(k), q])
    for k in cx_keys:
        for q in sorted(cx[k] & cz[k]):
            result.append("CY", [stim.target_rec(k), q])
    for k in cz_keys:
        for q in sorted(cz[k] - cx[k]):
            result.append("CZ", [stim.target_rec(k), q])

    return result
