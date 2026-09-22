from typing import Union

import numpy as np
import numpy.typing as npt

table_mult = {
    'II': ('I', 1),
    'IX': ('X', 1),
    'IY': ('Y', 1),
    'IZ': ('Z', 1),
    'XI': ('X', 1),
    'XX': ('I', 1),
    'XY': ('Z', 1j),
    'XZ': ('Y', -1j),
    'YI': ('Y', 1),
    'YX': ('Z', -1j),
    'YY': ('I', 1),
    'YZ': ('X', 1j),
    'ZI': ('Z', 1),
    'ZX': ('Y', 1j),
    'ZY': ('X', -1j),
    'ZZ': ('I', 1),
}

table_anticommute = {
    'II': 0,
    'IX': 0,
    'IY': 0,
    'IZ': 0,
    'XI': 0,
    'XX': 0,
    'XY': 1,
    'XZ': 1,
    'YI': 0,
    'YX': 1,
    'YY': 0,
    'YZ': 1,
    'ZI': 0,
    'ZX': 1,
    'ZY': 1,
    'ZZ': 0
}


def stringify(vect: Union[npt.NDArray, list]) -> str:
    if isinstance(vect, np.ndarray):
        vect = vect.tolist()
    vect = [str(a) for a in vect]
    return ''.join(vect)


def unstringify(string: str) -> npt.NDArray:
    vect = np.array(list(string), dtype=np.uint8)
    return vect


def bsf_to_str(bsf_vec: npt.NDArray[np.uint8]) -> str:
    n = len(bsf_vec) // 2
    x_part = bsf_vec[:n]
    z_part = bsf_vec[n:]
    result = []
    for x, z in zip(x_part, z_part):
        if x == 0 and z == 0:
            result.append('I')
        elif x == 1 and z == 0:
            result.append('X')
        elif x == 0 and z == 1:
            result.append('Z')
        elif x == 1 and z == 1:
            result.append('Y')
    return ''.join(result)


def str_to_bsf(str_pauli: Union[str, list[str]]) -> npt.NDArray[np.uint8]:
    x = np.array([int(c in ('X', 'Y')) for c in str_pauli], dtype=np.uint8)
    z = np.array([int(c in ('Z', 'Y')) for c in str_pauli], dtype=np.uint8)

    return np.concatenate([x, z])


def multiply_single_qubit_paulis(p1: str, p2: str):
    # Returns (resulting_pauli: str, phase: complex)
    return table_mult[p1 + p2]


# @profile
def multiply_paulis(p1: str, p2: str) -> tuple[complex, str]:
    phase = 1
    pauli_string = []
    for a, b in zip(p1, p2):
        pauli, p = multiply_single_qubit_paulis(a, b)
        pauli_string.append(pauli)
        phase *= p
    pauli_string = ''.join(pauli_string)

    return phase, pauli_string


def commute(p1: str, p2: str) -> bool:
    n_anticommute = 0
    for a, b in zip(p1, p2):
        if table_anticommute[a + b]:
            n_anticommute += 1

    return 1 - (n_anticommute % 2)


def isclose(a, b, eps=1e-9):
    return abs(a - b) < eps


def support(pauli: str) -> int:
    """Returns the support of a Pauli string, i.e., the number of non-identity terms."""
    return sum(c != 'I' for c in pauli)
