import stim

from stimflow._layers._layer_interact import LayerInteract
from stimflow._layers._layer_sqrt_pp import LayerSqrtPP
from stimflow._layers._layer_rotation import LayerRotation


def test_to_z_basis():
    layer = LayerSqrtPP(
        targets1=[0, 1, 2],
        targets2=[4, 5, 6],
        bases=['X', 'Y', 'Z'],
    )
    v = layer.to_z_basis()
    assert v == [
        LayerRotation(named_rotations={0: 'SQRT_X', 4: 'SQRT_X', 1: 'SQRT_Y', 5: 'SQRT_Y', 2: 'S', 6: 'S'}),
        LayerRotation(named_rotations={0: 'H', 1: 'H_YZ', 4: 'H', 5: 'H_YZ'}),
        LayerInteract(targets1=[0, 1, 2], targets2=[4, 5, 6], bases1=['Z', 'Z', 'Z'], bases2=['Z', 'Z', 'Z']),
        LayerRotation(named_rotations={0: 'H', 1: 'H_YZ', 4: 'H', 5: 'H_YZ'}),
    ]


def test_append_into_circuit():
    layer = LayerSqrtPP(
        targets1=[0, 1, 2],
        targets2=[4, 5, 6],
        bases=['X', 'Y', 'Z'],
    )
    circuit = stim.Circuit()
    layer.append_into_stim_circuit(circuit)
    assert circuit == stim.Circuit(
        """
        SQRT_XX 0 4
        SQRT_YY 1 5
        SQRT_ZZ 2 6
        """
    )
