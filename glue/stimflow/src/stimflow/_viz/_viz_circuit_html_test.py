from __future__ import annotations

import stim

import stimflow


def test_viewer_works_with_all_gates():
    circuit = stim.Circuit(
        """
        M 0 1
    """
    )
    for name, data in stim.gate_data().items():
        args = [0.01] * data.num_parens_arguments_range[0]
        if name == "REPEAT":
            continue
        if name in ["DETECTOR", "OBSERVABLE_INCLUDE"]:
            targets = [stim.target_rec(-1)]
            args = [0]
        elif name in ["SHIFT_COORDS", "TICK"]:
            targets = []
        elif data.takes_pauli_targets:
            targets = [stim.target_x(0)]
        else:
            targets = [0, 1]
        circuit.append(name, targets, args)
    viewer = stimflow.stim_circuit_html_viewer(circuit)
    assert viewer is not None
