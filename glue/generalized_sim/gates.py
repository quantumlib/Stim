from pauli_sum import PauliSum
import numpy as np
import stim


def stim_to_pauli(pauli_string):
    pauli_string = str(pauli_string)
    coeff = {'+': 1, '-': -1}[pauli_string[0]]
    if pauli_string[1] == 'i':
        coeff *= 1j
        pauli = pauli_string[2:]
    else:
        pauli = pauli_string[1:]

    return coeff, pauli.replace('_', 'I')


def single_pauli(pauli_type: str, pos: int, n: int) -> str:
    """Generate the n-qubit Pauli string corresponding to $X_i$, $Y_i$ or $Z_i$"""
    error = ['I'] * n
    error[pos] = pauli_type
    return ''.join(error)


def propagator_T(positions: list[int], pauli: str):
    n = len(pauli)
    initial_pauli = list(pauli)
    for i_pos in positions:
        if initial_pauli[i_pos] in ['X', 'Z']:
            initial_pauli[i_pos] = 'I'

    propagated_error = PauliSum({''.join(initial_pauli): 1})
    for i in range(n):
        if i in positions and pauli[i] in ['X', 'Z']:
            propagated_error *= PauliSum({
                single_pauli('X', i, n): 1/np.sqrt(2),
                single_pauli('Z', i, n): {'X': -1, 'Z': 1}[pauli[i]]*1/np.sqrt(2)
            })

    return propagated_error


def propagator_Tdag(positions: list[int], pauli: str):
    n = len(pauli)
    initial_pauli = list(pauli)
    for i_pos in positions:
        if initial_pauli[i_pos] in ['X', 'Z']:
            initial_pauli[i_pos] = 'I'

    propagated_error = PauliSum({''.join(initial_pauli): 1})
    for i in range(n):
        if i in positions and pauli[i] in ['X', 'Z']:
            propagated_error *= PauliSum({
                single_pauli('X', i, n): {'Z': -1, 'X': 1}[pauli[i]]*1/np.sqrt(2),
                single_pauli('Z', i, n): 1/np.sqrt(2),
            })

    return propagated_error

def propagation(layer: stim.CircuitInstruction, error: PauliSum, dag=False) -> PauliSum:
    propagated_error = PauliSum({})
    for pauli, coeff in error.items():
        if layer.name == 'I' and len(layer.tag) > 0:
            positions = list(map(lambda t: t.value, layer.targets_copy()))
            prop_fn = propagator_dag[layer.tag] if dag else propagator[layer.tag]
            new_pauli_sum = prop_fn(positions, pauli) * coeff
            propagated_error += new_pauli_sum
        else:
            if dag:
                new_pauli_stim = stim.PauliString(pauli).before(layer)
            else:
                new_pauli_stim = stim.PauliString(pauli).after(layer)

            new_coeff, new_pauli = stim_to_pauli(new_pauli_stim)
            propagated_error += PauliSum({new_pauli: coeff * new_coeff})
    return propagated_error


propagator = {
    'T': propagator_T, 
    'Tdag': propagator_Tdag
}

propagator_dag = {
    'T': propagator_Tdag, 
    'Tdag': propagator_T
}