from __future__ import annotations

import stim

import stimflow


def test_survives_transpile():
    circuit = stim.Circuit(
        """
        RX 0
        TICK
        CX 0 1
        TICK
        CX[test] 0 2
        TICK
        MRX 0
        SQRT_X[test2] 1
        TICK
        CX 0 1
        TICK
        CX 0 2
        TICK
        MRX 0
        DETECTOR rec[-1] rec[-2]
    """
    )
    circuit = stimflow.transpile_to_z_basis_interaction_circuit(circuit)
    assert circuit == stim.Circuit(
        """
        R 0
        TICK
        H 0 1
        TICK
        CZ 0 1
        TICK
        CX[test] 0 2
        TICK
        H 0 1
        TICK
        M 0
        TICK
        R 0
        TICK
        SQRT_X[test2] 1
        TICK
        H 0 1 2
        TICK
        CZ 0 1
        TICK
        CZ 0 2
        TICK
        H 0 1 2
        TICK
        M 0
        DETECTOR rec[-1] rec[-2]
    """
    )
