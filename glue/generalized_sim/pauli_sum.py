import numpy as np
from typing import Union, TypeAlias, Optional
from math import prod

from utils import multiply_paulis, isclose

SumPauliStrings: TypeAlias = dict[str, complex]


class PauliSum:
    def __init__(self, terms: Optional[Union[int, SumPauliStrings]] = None):
        if terms is None:
            self._terms = dict()
        elif isinstance(terms, int):
            self._terms = {'I' * terms: 1}
        elif isinstance(terms, dict):
            self._terms = terms
        else:
            raise TypeError("Invalid type for terms. Expected int or dict.")
        
        self._n_qubits = None

    # @profile
    def __add__(self, op: Union["PauliSum", SumPauliStrings]):
        new_dict = self._terms.copy()

        for pauli, coeff in op.items():
            new_coeff = new_dict.get(pauli, 0) + coeff
            if isclose(new_coeff, 0):
                new_dict.pop(pauli, None)  # Remove key if it exists
            else:
                new_dict[pauli] = new_coeff

        return PauliSum(new_dict)

    def __sub__(self, op: Union["PauliSum", SumPauliStrings]):
        return self + -1 * op

    # @profile
    def __mul__(self, op: Union[int, float, complex, "PauliSum"]):
        if isinstance(op, (int, float, complex)):
            if op == 0:
                new_dict = {}
            else:
                new_dict = {pauli: op * coeff for pauli, coeff in self}

        elif isinstance(op, PauliSum):
            new_dict = {}
            for pauli1, coeff1 in self._terms.items():
                for pauli2, coeff2 in op._terms.items():
                    phase, new_pauli = multiply_paulis(pauli1, pauli2)
                    new_coeff = coeff1 * coeff2 * phase
                    new_value = new_dict.get(new_pauli, 0) + new_coeff
                    if isclose(new_value, 0):
                        new_dict.pop(new_pauli, None)  # Remove key if it exists
                    else:
                        new_dict[new_pauli] = new_value
        else:
            raise NotImplementedError("Multiplication not implemented yet for those types")

        return PauliSum(new_dict)
    
    def remove(self, pauli: str):
        self._terms.pop(pauli, None)

    @property
    def n_qubits(self) -> int:
        if self._n_qubits is None:
            if len(self) == 0:
                self._n_qubits = 0
            else:
                self._n_qubits = len(list(self.keys())[0])

        return self._n_qubits
    
    def remove_zeros(self):
        """Remove terms with zero coefficients from the PauliSum."""
        self._terms = {pauli: coeff for pauli, coeff in self._terms.items() if not isclose(coeff, 0)}

        return self

    def dag(self) -> "PauliSum":
        new_dict = {pauli: complex(coeff).conjugate() for pauli, coeff in self}

        return PauliSum(new_dict)
    
    def normalize(self) -> "PauliSum":
        norm = np.linalg.norm(list(self.values()))
        if norm == 0:
            raise ValueError("Cannot normalize a zero vector.")
        
        return self / norm
    
    def proportional_to(self, op: "PauliSum") -> bool:
        self_coeffs = list(self.values())
        op_coeffs = list(op.values())
        if self.keys() == op.keys():
            prop_coeff = self_coeffs[0] / op_coeffs[0]
            if np.isclose(self_coeffs, prop_coeff * np.array(op_coeffs)).all():
                return True

        return False
    
    def commutes_with(self, op: "PauliSum") -> bool:
        return self * op == op * self 
    
    def anticommutes_with(self, op: "PauliSum") -> bool:
        return self * op == - op * self
    
    def restrict_up_to(self, end: int, include_identities: bool = True) -> "PauliSum":
        new_dict = {}
        for pauli, coeff in self.items():
            new_key = pauli[:end]
            if include_identities:
                new_key += 'I' * (len(pauli) - end)
            new_dict[new_key] = coeff

        return PauliSum(new_dict)
    
    def apply_unitary(self, unitary: Union["PauliSum", list["PauliSum"]]) -> "PauliSum":
        evolved_op = self
        if not isinstance(unitary, list):
            unitary = [unitary]
        for u in unitary:
            evolved_op = u * evolved_op * u.dag()

        return evolved_op
    
    def coefficient(self, pauli: str) -> complex:
        return self._terms.get(pauli, 0)

    def keys(self):
        return self._terms.keys()

    def values(self):
        return self._terms.values()

    def items(self):
        return self._terms.items()
    
    def copy(self) -> "PauliSum":
        return PauliSum(self._terms.copy())
    
    def __len__(self):
        return len(self._terms)

    def __eq__(self, op: "PauliSum"):
        return (
            self.keys() == op.keys() and 
            all([isclose(op[p], self[p]) for p in op.keys()])
        )

    def __rmul__(self, op: Union[int, float, complex, "PauliSum"]):
        return self.__mul__(op)
    
    def __pow__(self, power):
        return prod([self for _ in range(power)])
    
    def __neg__(self):
        return -1 * self
    
    def __truediv__(self, coeff: Union[int, float, complex]):
        return 1 / coeff * self

    def __iter__(self):
        return iter(self._terms.items())
    
    def __setitem__(self, key, value):
        self._terms[key] = value

    def __getitem__(self, key):
        return self._terms[key]

    def __str__(self):
        lines = ["PauliSum"]
        for pauli, coeff in self:
            lines.append(f"  {coeff:+.2f} * {pauli}")
        return "\n".join(lines)

    def __repr__(self):
        return self.__str__()


def single_pauli_sum(i: int, pauli_type: str, n: int) -> PauliSum:
    pauli_string = ['I'] * n
    pauli_string[i] = pauli_type

    return PauliSum({''.join(pauli_string): 1.0})

def X(i, n=7):
    return single_pauli_sum(i, 'X', n)

def Z(i, n=7):
    return single_pauli_sum(i, 'Z', n)

def Y(i, n=7):
    return single_pauli_sum(i, 'Y', n)
