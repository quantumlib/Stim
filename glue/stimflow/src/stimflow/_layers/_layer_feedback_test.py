from stimflow._layers._layer_feedback import _basis_before_rotation


def test_basis_before_rotation():
    assert _basis_before_rotation("X", "C_ZYX") == "Y"
    assert _basis_before_rotation("Y", "C_ZYX") == "Z"
    assert _basis_before_rotation("Z", "C_ZYX") == "X"
