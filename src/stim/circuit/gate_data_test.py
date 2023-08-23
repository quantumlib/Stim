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
