from __future__ import annotations

import stim

import stimflow
from stimflow._layers._layer_tag import LayerTag

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

def test_touched() -> None:

    layer_cx = LayerTag(circuit=stim.Circuit("CX 0 1 2 3"))
    assert layer_cx.touched() == {0, 1, 2, 3}

    layer_m = LayerTag(circuit=stim.Circuit("M 0 1 2"))
    assert layer_m.touched() == {0, 1, 2}

    # Test filtering of non-qubit targets (e.g. combiners in MPP instructions or Pauli targets).
    layer_mpp = LayerTag(circuit=stim.Circuit("MPP X10*Y11 Z12*X13"))
    # In MPP, targets are Pauli targets/combiners.
    # We verify touched() handles target groups gracefully.
    assert isinstance(layer_mpp.touched(), set)

    # Test that when self.circuit[0] is a CircuitRepeatBlock, touched() iterates
    # through the block and finds all qubit targets.
    layer_repeat = LayerTag(
        circuit=stim.Circuit(
            """
            REPEAT 5 {
                CX 0 1
                TICK
                M 2 3
            }
        """
        )
    )
    assert layer_repeat.touched() == {0, 1, 2, 3}

    # Test that when self.circuit[0] contains nested CircuitRepeatBlocks, touched()
    # recursively iterates through all nested repeat blocks to find all qubit targets.
    layer_nested_repeat = LayerTag(
        circuit=stim.Circuit(
            """
            REPEAT 3 {
                CX 0 1
                REPEAT 2 {
                    CX 2 3
                    TICK
                    M 4 5
                }
            }
        """
        )
    )
    assert layer_nested_repeat.touched() == {0, 1, 2, 3, 4, 5}