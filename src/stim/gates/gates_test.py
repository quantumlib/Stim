import numpy as np
import stim


def test_gate_data_eq():
    assert stim.gate_data('H') == stim.GateData('H')
    assert stim.gate_data('H') == stim.gate_data('H_XZ')
    assert not (stim.gate_data('H') == stim.GateData('X_ERROR'))
    assert stim.gate_data('X') != stim.GateData('H')


def test_gate_data_repr():
    val = stim.GateData('MPP')
    assert eval(repr(val), {"stim": stim}) == val


def test_gate_data_inverse():
    for v in stim.gate_data().values():
        assert v.is_unitary == (v.inverse is not None)
        if v.is_unitary:
            assert np.allclose(v.unitary_matrix.conj().T, v.inverse.unitary_matrix, atol=1e-6)
            assert v.inverse == v.generalized_inverse

    assert stim.gate_data('H').inverse == stim.gate_data('H')
    assert stim.gate_data('S').inverse == stim.gate_data('S_DAG')
    assert stim.gate_data('M').inverse is None
    assert stim.gate_data('CXSWAP').inverse == stim.gate_data('SWAPCX')

    assert stim.gate_data('S').generalized_inverse == stim.gate_data('S_DAG')
    assert stim.gate_data('M').generalized_inverse == stim.gate_data('M')
    assert stim.gate_data('R').generalized_inverse == stim.gate_data('M')
    assert stim.gate_data('MR').generalized_inverse == stim.gate_data('MR')
    assert stim.gate_data('MPP').generalized_inverse == stim.gate_data('MPP')
    assert stim.gate_data('ELSE_CORRELATED_ERROR').generalized_inverse == stim.gate_data('ELSE_CORRELATED_ERROR')
