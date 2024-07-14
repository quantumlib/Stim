import numpy as np
import stim


def test_gate_data_eq():
    assert stim.gate_data('H') == stim.GateData('H')
    assert stim.gate_data('H') == stim.gate_data('H_XZ')
    assert not (stim.gate_data('H') == stim.GateData('X_ERROR'))
    assert stim.gate_data('X') != stim.GateData('H')


def test_gate_data_str():
    assert str(stim.GateData('MXX')) == '''
stim.GateData {
    .name = 'MXX'
    .aliases = ['MXX']
    .is_noisy_gate = True
    .is_reset = False
    .is_single_qubit_gate = False
    .is_two_qubit_gate = True
    .is_unitary = False
    .num_parens_arguments_range = range(0, 2)
    .produces_measurements = True
    .takes_measurement_record_targets = False
    .takes_pauli_targets = False
}
    '''.strip()
    assert str(stim.GateData('H')) == '''
stim.GateData {
    .name = 'H'
    .aliases = ['H', 'H_XZ']
    .is_noisy_gate = False
    .is_reset = False
    .is_single_qubit_gate = True
    .is_two_qubit_gate = False
    .is_unitary = True
    .num_parens_arguments_range = range(0, 1)
    .produces_measurements = False
    .takes_measurement_record_targets = False
    .takes_pauli_targets = False
    .tableau = stim.Tableau.from_conjugated_generators(
        xs=[
            stim.PauliString("+Z"),
        ],
        zs=[
            stim.PauliString("+X"),
        ],
    )
    .unitary_matrix = np.array([[(0.7071067690849304+0j), (0.7071067690849304+0j)], [(0.7071067690849304+0j), (-0.7071067690849304-0j)]], dtype=np.complex64)
}
    '''.strip()


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


def test_gate_data_flows():
    assert stim.GateData('H').flows == [
        stim.Flow("X -> Z"),
        stim.Flow("Z -> X"),
    ]


def test_gate_is_symmetric():
    assert stim.GateData('SWAP').is_symmetric_gate
    assert stim.GateData('H').is_symmetric_gate
    assert stim.GateData('MYY').is_symmetric_gate
    assert stim.GateData('DEPOLARIZE2').is_symmetric_gate
    assert not stim.GateData('PAULI_CHANNEL_2').is_symmetric_gate
    assert not stim.GateData('DETECTOR').is_symmetric_gate
    assert not stim.GateData('TICK').is_symmetric_gate


def test_gate_hadamard_conjugated():
    assert stim.GateData('CZSWAP').hadamard_conjugated(unsigned=True) is None
    assert stim.GateData('TICK').hadamard_conjugated() == stim.GateData('TICK')
    assert stim.GateData('MYY').hadamard_conjugated() == stim.GateData('MYY')
    assert stim.GateData('XCZ').hadamard_conjugated() == stim.GateData('CX')
