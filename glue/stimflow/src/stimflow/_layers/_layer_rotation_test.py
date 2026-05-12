from __future__ import annotations

import stim

from stimflow._layers._layer_rotation import LayerRotation


def test_fuses_rotations():
    layer = LayerRotation()
    layer.append_named_rotation("H", 0)
    layer.append_named_rotation("H_NXZ", 0)
    assert layer.named_rotations[0] == "Y"


def test_output():
    layer = LayerRotation()

    layer.append_named_rotation("H", 0)
    layer.append_named_rotation("C_XYZ", 0)
    layer.append_named_rotation("S", 0)

    layer.prepend_named_rotation("H", 1)
    layer.prepend_named_rotation("C_ZYX", 1)
    layer.prepend_named_rotation("S_DAG", 1)

    circuit = stim.Circuit()
    layer.append_into_stim_circuit(circuit)
    assert circuit == stim.Circuit(
        """
        C_NZYX 1
        C_XYNZ 0
    """
    )
