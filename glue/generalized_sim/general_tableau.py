from math import prod, sqrt, acos
from cmath import exp
import random
from itertools import product
from typing import Union

import numpy as np
import stim
from ldpc.mod2 import nullspace, rank

from pauli_sum import PauliSum, X, Z, single_pauli_sum
from utils import commute, multiply_paulis, str_to_bsf, bsf_to_str, support
from gates import propagation
from logger import Logger


direction_dict = {
    'DEPOLARIZE1': tuple([1/3 for _ in range(3)]),
    'DEPOLARIZE2': tuple([1/15 for _ in range(15)]),
    'X_ERROR': (1, 0, 0),
    'Y_ERROR': (0, 1, 0),
    'Z_ERROR': (0, 0, 1),
}

logical_H = PauliSum({'IIXXIXIIIIIIIIII': 1/np.sqrt(2), 'ZZIIIZIIIIIIIIII': 1/np.sqrt(2)})
nonclifford_gates = ['T', 'Tdag']

class GeneralTableau:
    def __init__(self, n: int, logger=None):
        self.n = n
        self.stabilizers: list[PauliSum] = [Z(i, n) for i in range(n)]
        self.destabilizers: list[PauliSum] = [X(i, n) for i in range(n)]
        self.nonpauli_index = -1
        self.buffer_layers: list[stim.CircuitInstruction] = []
        self.is_circuit_buffer_nonclifford = False
        self.decomposition_threshold = self.n // 2 - 1

        self.buffer_errors: list[PauliSum] = []
        self.nontrivial_logicals = None

        if logger is None:
            self.logger = Logger('log.txt', stdout_print=False)
        else:
            self.logger = logger

    def check_commutation_relations(self, stabilizers=None, destabilizers=None):
        """Check that all stabilizers/destabilizers commute with each other
        and each stabilizer anticommute only with its corresponding destabilizer."""

        if stabilizers is None:
            stabilizers = self.stabilizers
        if destabilizers is None:
            destabilizers = self.destabilizers

        for i, stab in enumerate(stabilizers):
            for j, destab in enumerate(destabilizers):
                if i != j and not stab.commutes_with(destab):
                    raise ValueError(
                        f"Stabilizer {i} {stab}\nand destabilizer {j} {destab}\ndo not commute."
                    )
                if i == j and stab.commutes_with(destab):
                    raise ValueError(
                        f"Stabilizer {i} {stab}\nand destabilizer {j} {destab}\ncommute."
                    )
                
            for j, other_stab in enumerate(stabilizers):
                if i != j and not stab.commutes_with(other_stab):
                    raise ValueError(
                        f"Stabilizers {i} {stab}\nand {j} {other_stab}\ndo not commute."
                    )
                
        for i, destab in enumerate(destabilizers):
            for j, other_destab in enumerate(destabilizers):
                if i != j and not destab.commutes_with(other_destab):
                    raise ValueError(
                        f"Destabilizers {i} {destab}\nand {j} {other_destab}\ndo not commute."
                    )
                
    def weak_commute(self, op1: list[PauliSum], op2: PauliSum) -> bool:
        commutant = op2
        for op in op1:
            commutant = op * commutant * op.dag()
        commutant = commutant * op2.dag()
        # self.logger.print(f"Commutant: {commutant}")

        if commutant == PauliSum({'I'*self.n: 1}):
            return True
        
        if commutant == PauliSum({'I'*self.n: -1}):
            return False

        for i_stab, stab in enumerate(self.stabilizers):
            if not self.weak_commute([commutant], stab):
                self.logger.print(f"Weak commute failed with stabilizer {i_stab}: {stab}")
                self.logger.print(f"Commutant: {commutant}")
                return False
            
        return True
    
    def weak_anticommute(self, op1: list[PauliSum], op2: PauliSum) -> bool:
        commutant = op2
        for op in op1:
            commutant = op * commutant * op.dag()
        commutant = commutant * op2.dag()
        # self.logger.print(f"Commutant: {commutant}")

        if commutant == PauliSum({'I'*self.n: 1}):
            return False
        
        if commutant == PauliSum({'I'*self.n: -1}):
            return True

        for i_stab, stab in enumerate(self.stabilizers):
            if not self.weak_anticommute([commutant], stab):
                self.logger.print(f"Weak commute failed with stabilizer {i_stab}: {stab}")
                return False
            
        return True
    
    def is_stabilizer(self, op: list[PauliSum], index: int = None) -> bool:
        """Check if an operator is a stabilizer of the tableau.
        If index is provided, check that the stabilizer anticommutes with the destabilizer
        at that index and commutes with all other destabilizers.
        """
        self.logger.print(f"Is {index} stabilizer? {op}")
        # if index is not None:
        #     for j, destab in enumerate(self.destabilizers):
        #         # self.logger.print(f"Checking commutation between destabilizer {j} {destab}\nand stabilizer {index} {op}.")
        #         if j != index and not self.weak_commute(op, destab):
        #             self.logger.print(f"Destabilizer {j} {destab}\ndoes not commute with stabilizer {index} {op}.")
        #             return False

        for i_stab, stab in enumerate(self.stabilizers):
            # self.logger.print(f"Checking commutation between stabilizer {i_stab} {stab}\nand stabilizer {op}.")
            if not self.weak_commute(op, stab):
                self.logger.print(f"Stabilizer {i_stab} {stab}\n does not commute with operator {op}.")
                return False

        self.logger.print("Yes")
        return True
    
    def is_destabilizer(self, op: list[PauliSum], index: int = None) -> bool:
        """Check if an operator is a destabilizer of the tableau.
        If index is provided, check that the destabilizer anticommutes with the stabilizer
        at that index and commutes with all other destabilizers/stabilizers.
        """
        self.logger.print(f"Is {index} destabilizer? {op}")
        # if index is not None:
        #     if not self.weak_anticommute(op, self.stabilizers[index]):
        #         self.logger.print(f"Stabilizer {index} {self.destabilizers[index]}\ndoes not anticommute with destabilizer {index} {op}.")
        #         return False
        #     for j, stab in enumerate(self.stabilizers):
        #         if j != index and not self.weak_commute(op, stab):
        #             self.logger.print(f"Stabilizer {j} {stab}\ndoes not commute with destabilizer {index} {op}.")
        #             return False

        for i_destab, destab in enumerate(self.destabilizers):
            if not self.weak_commute(op, destab):
                self.logger.print(f"Destabilizer {i_destab} {destab}\n does not commute with destabilizer {op}.")
                return False

        self.logger.print("Yes")
        return True
    
    def generate_error(
        self, 
        positions: list[int], 
        p: float, 
        direction: tuple[float]
    ) -> PauliSum:
        if len(direction) == 3:
            restricted_error = random.choices(
                ['I', 'X', 'Y', 'Z'], 
                k=len(positions), 
                weights=[1-p, p * direction[0], p * direction[1], p * direction[2]]
            )
        else:
            restricted_error = ''.join(random.choices(
                list(map(''.join, list(product(['I', 'X', 'Y', 'Z'], repeat=2)))),
                k=len(positions)//2, 
                weights=[1-p] + [p/15] * 15
            ))

        error = ['I'] * self.n
        for i, pos in enumerate(positions):
            error[pos] = restricted_error[i]

        return PauliSum({''.join(error): 1})

    def get_pauli_tableau_matrix(self):
        bsf_stabilizers = []
        bsf_destabilizers = []
        for i_stab in range(self.n):
            if i_stab != self.nonpauli_index:
                stab_pauli = list(self.stabilizers[i_stab].keys())[0]
                destab_pauli = list(self.destabilizers[i_stab].keys())[0]
                bsf_stabilizers.append(str_to_bsf(stab_pauli))
                bsf_destabilizers.append(str_to_bsf(destab_pauli))
        
        tableau_matrix = np.array(bsf_stabilizers + bsf_destabilizers, dtype=np.uint8)

        return tableau_matrix
    
    def get_nontrivial_logicals(self):
        tableau_matrix = self.get_pauli_tableau_matrix()
        normalizer = nullspace(tableau_matrix).toarray()
        logicals = []

        for i in range(normalizer.shape[0]):
            normalizer_bsf = np.hstack(
                [normalizer[i, self.n:], normalizer[i, :self.n]]
            )
            if rank(np.vstack([tableau_matrix, normalizer_bsf])) > len(tableau_matrix):
                logicals.append(
                    bsf_to_str(normalizer_bsf)
                )
                tableau_matrix = np.vstack([tableau_matrix, normalizer_bsf])

        self.logger.print(f"\nLogicals: {logicals}\n")

        self.nontrivial_logicals = logicals

        return logicals
        
    def simplify(self, op: Union[PauliSum, list[PauliSum]], round_eps=13):
        # print("Op", op)
        # print("Nontrivial logicals:", self.nontrivial_logicals)

        if self.nontrivial_logicals is None:
            raise ValueError("Non-trivial logicals are not set, cannot simplify operator.")
        elif len(self.nontrivial_logicals) == 0:
            raise ValueError("Set of non-trivial logicals is empty, cannot simplify operator.")


        X_L_pauli, Z_L_pauli = self.nontrivial_logicals
        Y_L_pauli = multiply_paulis(X_L_pauli, Z_L_pauli)[1]

        X_L = PauliSum({X_L_pauli: 1})
        Y_L = PauliSum({Y_L_pauli: 1})
        Z_L = PauliSum({Z_L_pauli: 1})

        UXU = X_L.apply_unitary(op)
        UYU = Y_L.apply_unitary(op)
        UZU = Z_L.apply_unitary(op)

        tr_xx = self.lds_decompose(UXU * X_L).coefficient('I'*self.n).real
        tr_zz = self.lds_decompose(UZU * Z_L).coefficient('I'*self.n).real
        tr_xy = self.lds_decompose(UXU * Y_L).coefficient('I'*self.n).real
        tr_xz = self.lds_decompose(UXU * Z_L).coefficient('I'*self.n).real
        tr_yy = self.lds_decompose(UYU * Y_L).coefficient('I'*self.n).real

        self.logger.print(f"op {op}")
        self.logger.print(f"UXU {(UXU)}")
        self.logger.print(f"UXUZ {(UXU * Z_L)}")
        self.logger.print(f"UXUZ decompose {self.lds_decompose(UXU * Z_L)}")
        self.logger.print(f"UXUY {(UXU * Y_L)}")
        self.logger.print(f"UXUY decompose {self.lds_decompose(UXU * Y_L)}")
        self.logger.print(f"tr_xx {tr_xx}")
        self.logger.print(f"tr_yy {tr_yy}")
        self.logger.print(f"tr_zz {tr_zz}")
        self.logger.print(f"tr_xy {tr_xy}")
        self.logger.print(f"tr_xz {tr_xz}")
        
        try:
            mod_d = sqrt(round(1/4 * (tr_xx + tr_yy + tr_zz + 1).real, round_eps))
            mod_a = sqrt(round(-0.5 * (tr_yy + tr_zz).real + mod_d**2, round_eps))
            mod_b = sqrt(round(-0.5 * (tr_xx + tr_zz).real + mod_d**2, round_eps))
            mod_c = sqrt(round(-0.5 * (tr_xx + tr_yy).real + mod_d**2, round_eps))
        except ValueError as e:
            print("Op", op)
            print("Nontrivial logicals:", self.nontrivial_logicals)
            print("UXUX", (UXU * X_L))
            print("UXUX decompose", self.lds_decompose(UXU * X_L))
            print("UYUY", (UYU * Y_L))
            print("UYUY decompose", self.lds_decompose(UYU * Y_L))
            print("UZUZ", (UZU * Z_L))
            print("UZUZ decompose", self.lds_decompose(UZU * Z_L))
            # print("LDS decomposed op", self.lds_decompose(prod(op)))

            print("tr_xx", tr_xx)
            print("tr_yy", tr_yy)
            print("tr_zz", tr_zz)
            print("mod_a_square", -0.5 * (tr_yy + tr_zz).real)
            print("mod_b_square", -0.5 * (tr_xx + tr_zz).real)
            print("mod_c_square", -0.5 * (tr_xx + tr_yy).real)
            print("mod_d_square", 1/4 * (tr_xx + tr_yy + tr_zz + 1).real)
            print(self)
            # raise ValueError(e)

        theta_a = 0

        if mod_a * mod_d != 0:
            tr_xi = self.lds_decompose(UXU).coefficient('I'*self.n).real
            theta_d = acos(round(0.5 * tr_xi / (mod_d * mod_a), round_eps))
        elif mod_b * mod_d != 0:
            tr_yi = self.lds_decompose(UYU).coefficient('I'*self.n).real
            theta_d = acos(round(0.5 * tr_yi / (mod_d * mod_b), round_eps))
        elif mod_c * mod_d != 0:
            tr_zi = self.lds_decompose(UZU).coefficient('I'*self.n).real
            theta_d = acos(round(0.5 * tr_zi / (mod_d * mod_c), round_eps))
        else:
            theta_d = 0

        if mod_a * mod_b != 0:
            cos_theta_b = round(0.5 * (tr_xy / (mod_a * mod_b)), round_eps)
            theta_b = acos(cos_theta_b)
        else:
            theta_b = 0

        if mod_a * mod_c != 0:
            cos_theta_c = round(0.5 * (tr_xz / (mod_a * mod_c)), round_eps)
            theta_c = acos(cos_theta_c)
        elif mod_b * mod_c != 0:
            tr_yz = self.lds_decompose(UYU * Z_L).coefficient('I'*self.n).real
            cos_theta_c = round(0.5 * (tr_yz / (mod_b * mod_c)), round_eps)
            theta_c = acos(cos_theta_c) + theta_b
        else:
            theta_c = 0

        self.logger.print(f"theta_a {theta_a}")
        self.logger.print(f"theta_b {theta_b}")
        self.logger.print(f"theta_c {theta_c}")

        simplified_op = PauliSum({
            X_L_pauli: mod_a * exp(1j * theta_a), 
            Y_L_pauli: mod_b * exp(1j * theta_b), 
            Z_L_pauli: mod_c * exp(1j * theta_c),
            'I'*self.n: mod_d * exp(1j * theta_d)
        }).remove_zeros()

        return simplified_op
    
    def lds_decompose(self, op: PauliSum):
        """Simplify a Pauli operator by removing identity terms."""
        
        new_op = PauliSum({})
        for pauli, coeff in op.items():
            # self.logger.print("Old pauli:", pauli, coeff)
            anticommuting_stabilizers = []
            anticommuting_destabilizers = []
            anticommuting_logicals = []
            for i in range(self.n):
                if i != self.nonpauli_index:
                    stab_pauli = list(self.stabilizers[i].keys())[0]
                    destab_pauli = list(self.destabilizers[i].keys())[0]
                    if not commute(stab_pauli, pauli):
                        anticommuting_stabilizers.append(i)
                    if not commute(destab_pauli, pauli):
                        anticommuting_destabilizers.append(i)

            if self.nontrivial_logicals is not None:
                for i in range(len(self.nontrivial_logicals)):
                    if not commute(self.nontrivial_logicals[i], pauli):
                        anticommuting_logicals.append(i)

            if (
                len(anticommuting_stabilizers) == 0 and 
                len(anticommuting_destabilizers) == 0 and 
                len(anticommuting_logicals) == 0
            ):
                new_pauli, new_coeff = pauli, coeff
            else:
                decomposition_d = 'I'*self.n
                decomposition_s = 'I'*self.n
                decomposition_l = 'I'*self.n
                phase_s = 1
                for i in anticommuting_stabilizers:
                    destab_pauli = list(self.destabilizers[i].keys())[0]
                    _, decomposition_d = multiply_paulis(decomposition_d, destab_pauli)
                for i in anticommuting_destabilizers:
                    # We keep the stabilizer phase because it is absorbed in the state
                    # when simplifying the operator
                    stab_pauli = list(self.stabilizers[i].keys())[0]
                    coeff_s = self.stabilizers[i][stab_pauli]
                    new_phase_s, decomposition_s = multiply_paulis(decomposition_s, stab_pauli)
                    phase_s *= new_phase_s * coeff_s
                for i in anticommuting_logicals:
                    logical = self.nontrivial_logicals[1-i]
                    _, decomposition_l = multiply_paulis(decomposition_l, logical)

                # self.logger.print("Decomposition S:", decomposition_s, phase_s)
                
                LD_coeff, LD_pauli = multiply_paulis(decomposition_l, decomposition_d)
                LDS_coeff, LDS_pauli = multiply_paulis(LD_pauli, decomposition_s)
                LDS_coeff *= LD_coeff

                final_phase = LDS_coeff / phase_s
                new_coeff = coeff * final_phase
                new_pauli = LD_pauli

                if LDS_pauli != pauli:
                    raise ValueError(
                        "Decomposition does not match original pauli\n" +
                        f"Original Pauli: {pauli}\n" +
                        f"Decomposition S: {decomposition_s}\n" +
                        f"Decomposition D: {decomposition_d}\n" +
                        f"Decomposition L: {decomposition_l}\n" +
                        f"Recomposition: {LDS_pauli}\n" +
                        f"Anticommuting stabilizers: {anticommuting_stabilizers}" +
                        f"Anticommuting destabilizers: {anticommuting_destabilizers}" +
                        f"Anticommuting logicals: {anticommuting_logicals}" +
                        f"Tableau:\n {self}"
                    )

            new_op += {new_pauli: new_coeff}
            # self.logger.print("New pauli:", new_pauli, new_coeff)
        return new_op
    
    def get_measurement_probability(self, qubit: int) -> float:
        """Get the probability of measuring a qubit in the |0> state."""
        Zq = Z(qubit, self.n)
        M = self.backpropagate_buffer(Zq)
        M_simplified = self.simplify(M)
        a = (M_simplified * self.stabilizers[self.nonpauli_index]).coefficient('I'*self.n)
        d = M_simplified.coefficient('I'*self.n)
        p = 0.5 * (1 + abs(a) + abs(d))

        return p
    
    def get_orthogonal(self, op: PauliSum):
        """Find element that anticommutes with op, assuming it is a sum of logicals.
        We use the Gram-Schmidt process to the find an orthogonal element to the vector (a, b, c)
        where op=aX + bY + cZ
        """

        if self.nontrivial_logicals is None:
            raise ValueError("Non-trivial logicals are not set. Call get_nontrivial_logicals() first.")

        X_L_pauli, Z_L_pauli = self.nontrivial_logicals
        Y_L_pauli = multiply_paulis(X_L_pauli, Z_L_pauli)[1]

        if not set(op.keys()).issubset(set([X_L_pauli, Y_L_pauli, Z_L_pauli])):
            raise ValueError(
                f"Operator {op} is not a sum of logicals. "
                "It should be a sum of X, Y, Z logicals"
                f"\nNon-trivial logicals:\n{self.nontrivial_logicals}."
            )
        
        a, b, c = op.coefficient(X_L_pauli), op.coefficient(Y_L_pauli), op.coefficient(Z_L_pauli)

        if b == c == 0:
            ortho_vector = [0, 0, 1]
        else:
            ortho_vector = [1-a**2, -a*b, -a*c]

        ortho_op = PauliSum({
            X_L_pauli: ortho_vector[0], 
            Y_L_pauli: ortho_vector[1], 
            Z_L_pauli: ortho_vector[2]
        }).remove_zeros()

        return ortho_op
        

    def measure_and_reset(self, qubit: int, meas_prob=None) -> int:
        self.logger.print(f"------- Measure and reset qubit {qubit} -------")
        noncommuting_stabilizers = []
        noncommuting_destabilizers = []
        anticommuting_destabilizers = []
        Zq = Z(qubit, self.n)
        Xq = X(qubit, self.n)

        # self.logger.print("Check commutation relations before measurement")
        # self.check_commutation_relations(self.stabilizers, self.destabilizers)

        i_s0 = -1  # Index of a Pauli stabilizer that anticommutes with Zq
        commuting_non_pauli_index = -1
        for i, s in enumerate(self.stabilizers):
            # self.logger.print("Check stabilizer ", i, ":", s)
            if s * Zq != Zq * s:
                noncommuting_stabilizers.append(i)
                if i_s0 == -1 and len(s) == 1:
                    i_s0 = i
            elif len(s) > 1:
                if commuting_non_pauli_index == -1:
                    commuting_non_pauli_index = i
                else:
                    raise ValueError(
                        "More than one non-Pauli stabilizer that commutes with Zq. "
                        "This is not supported."
                    )

        for i, d in enumerate(self.destabilizers):
            if d * Zq != Zq * d:
                noncommuting_destabilizers.append(i)
            if d * Zq == - Zq * d:
                anticommuting_destabilizers.append(i)

        self.logger.print(f"Noncommuting stabilizers:\n {noncommuting_stabilizers}")
        self.logger.print(f"Pivot:\n {i_s0}")
        self.logger.print(f"Anticommuting destabilizers:\n {anticommuting_destabilizers}")
        self.logger.print(f"Noncommuting destabilizers:\n {noncommuting_destabilizers}")

        if len(noncommuting_stabilizers) == 0:
            S: PauliSum = prod([self.stabilizers[s] for s in anticommuting_destabilizers])
            S *= Zq
            self.logger.print(f"Deterministic. This should be r x I: {S}")
            assert list(S.keys()) == ['I'*self.n], (
                f"Not identity. Tableau:\n{self.stabilizers}\n{self.destabilizers}"
            )

            r = S['I'*self.n]
        else:
            if i_s0 == -1:
                if len(noncommuting_stabilizers) > 1:
                    raise ValueError(
                        "More than one non-Pauli stabilizer that does not commute with Zq. "
                        "This is not supported."
                    )
                else:
                    i_s0 = noncommuting_stabilizers[0]
            else:
                pivot_pauli = list(self.stabilizers[i_s0].keys())[0]
                pivot_coeff = self.stabilizers[i_s0][pivot_pauli]

                for i_s in noncommuting_stabilizers:
                    if i_s != i_s0:
                        new_stab = PauliSum({})
                        for pauli, coeff in self.stabilizers[i_s].items():
                            if pauli[qubit] in ['X', 'Y']:
                                new_coeff, new_pauli = multiply_paulis(pauli, pivot_pauli)
                                new_coeff *= pivot_coeff * coeff
                                new_stab += {new_pauli: new_coeff}
                            else:
                                new_stab += {pauli: coeff}
                        self.stabilizers[i_s] = new_stab

                for i_d in noncommuting_destabilizers:
                    if i_d != i_s0:
                        new_destab = PauliSum({})
                        for pauli, coeff in self.destabilizers[i_d].items():
                            if pauli[qubit] in ['X', 'Y']:
                                new_coeff, new_pauli = multiply_paulis(pauli, pivot_pauli)
                                new_coeff *= pivot_coeff * coeff
                                new_destab += {new_pauli: new_coeff}
                            else:
                                new_destab += {pauli: coeff}
                        self.destabilizers[i_d] = new_destab
                    
            self.destabilizers[i_s0] = Xq
            p = 0.5 if meas_prob is None else meas_prob
            # r = np.random.choice([1, -1], p=[p, 1-p])
            r = 1 # Cheating for testing purposes
            # It is not r * Zq because of resetting
            self.stabilizers[i_s0] = Zq
            
            if commuting_non_pauli_index != -1:
                self.nonpauli_index = commuting_non_pauli_index
                self.get_nontrivial_logicals()
                self.logger.print(f"Non-trivial logicals: {self.nontrivial_logicals}")
                self.logger.print(f"Commuting non-Pauli stabilizer index: {commuting_non_pauli_index}")
                self.logger.print(f"Old stabilizer: {self.stabilizers[commuting_non_pauli_index]}")
                self.logger.print(f"Old destabilizer: {self.destabilizers[commuting_non_pauli_index]}")

                new_stab_without_Zq = self.simplify(self.stabilizers[commuting_non_pauli_index])
                new_stab = self.simplify(self.stabilizers[commuting_non_pauli_index]) * Zq

                self.logger.print(f"New stabilizer: {new_stab}")
                self.stabilizers[commuting_non_pauli_index] = new_stab

                # Calculate the new destablizer by finding an orthogonal element
                new_destab = self.get_orthogonal(new_stab_without_Zq)
                self.destabilizers[commuting_non_pauli_index] = new_destab



            # Reseting by multiplying with r all stabilizers/destabilizers containing Zq
            # for i in range(self.n):
            #     if i != i_s0:
            #         for tableau in [self.stabilizers, self.destabilizers]:
            #             for pauli, _ in tableau[i].items():
            #                 if pauli[qubit] == 'Z':
            #                     tableau[i][pauli] *= r
            # Reseting by multiplying by Zq/D[Zq] all the stabilizers/destabilizers containing Zq
            
            # self.logger.print("Tableau before resetting:")
            # self.logger.print(self)

            for i in range(self.n):
                if i != i_s0:
                    new_stab = PauliSum({})
                    for pauli, coeff in self.stabilizers[i]:
                        if pauli[qubit] == 'Z':
                            new_pauli = pauli[:qubit] + 'I' + pauli[qubit+1:]
                            new_coeff = coeff * r
                        else:
                            new_pauli = pauli
                            new_coeff = coeff
                        new_stab += {new_pauli: new_coeff}
                    self.stabilizers[i] = new_stab

                    new_destab = PauliSum({})
                    for pauli, coeff in self.destabilizers[i]:
                        if pauli[qubit] == 'Z':
                            new_pauli = pauli[:qubit] + 'I' + pauli[qubit+1:]
                            new_coeff = coeff * r
                        else:
                            new_pauli = pauli
                            new_coeff = coeff
                        new_destab += {new_pauli: new_coeff}
                    self.destabilizers[i] = new_destab

            # self.logger.print()
            # self.logger.print("Check commutation relations after measurement")
            self.check_commutation_relations(self.stabilizers, self.destabilizers)

        self.logger.print(f"Result: {r}")
        return r
    
    def backpropagate_buffer(self, op: PauliSum):
        """Backpropagate a Pauli operator through the buffer layers."""
        backpropagated_op = [op.copy()]
        for i_layer in range(len(self.buffer_layers)-1, -1, -1):
            if (self.buffer_layers[i_layer].tag in nonclifford_gates and 
                len(backpropagated_op) == 1 and 
                len(backpropagated_op[0]) == 1 and
                support((list(backpropagated_op[0].keys())[0])) >= self.decomposition_threshold
            ):
                # Decompose the operator before a non-Clifford gate if not already decomposed
                current_op = backpropagated_op[0]
                pauli = list(current_op.keys())[0]
                backpropagated_op = []
                for i in range(self.n):
                    if pauli[i] != 'I':
                        backpropagated_op.append(single_pauli_sum(i, pauli[i], self.n))
                backpropagated_op[0] *= current_op[pauli]

            for i_op, op in enumerate(backpropagated_op):
                new_op = propagation(
                    self.buffer_layers[i_layer], op, dag=True
                )
                backpropagated_op[i_op] = new_op

        # self.logger.print(f"Backpropagated operator:\n{backpropagated_op}")
        return backpropagated_op
    
    def propagate_buffer(self, op: PauliSum):
        """Propagate a Pauli operator through the buffer layers."""
        propagated_op = op.copy()
        for layer in self.buffer_layers:
            propagated_op = propagation(layer, propagated_op)

        return propagated_op
    
    def apply_buffer(self, check_preservation=True):
        new_stabilizers = []
        new_destabilizers = []
        new_nonpauli_index = -1

        for i_stab, stab in enumerate(self.stabilizers):
            self.logger.print(f"Original stabilizer {i_stab}: {stab}")
            destab = self.destabilizers[i_stab]

            backpropagated_stab = self.backpropagate_buffer(stab)
            backpropagated_destab = self.backpropagate_buffer(destab)
            self.logger.print(f"Backpropagated stabilizer {i_stab}: {backpropagated_stab}")
            self.logger.print(f"Backpropagated destabilizer {i_stab}: {backpropagated_destab}")

            # if check_preservation and (
            #     backpropagated_stab == PauliSum({'I'*self.n: 1}) or
            #     backpropagated_stab == self.stabilizers[self.nonpauli_index]
            # ):
            if (check_preservation and 
                self.is_stabilizer(backpropagated_stab, i_stab) and True
                # self.is_destabilizer(backpropagated_destab, i_stab)
            ):
                self.logger.print("Preserve stabilizer\n")
                propagated_stab = self.stabilizers[i_stab]
                propagated_destab = self.destabilizers[i_stab]
            else:
                propagated_stab = self.propagate_buffer(stab)
                propagated_destab = self.propagate_buffer(self.destabilizers[i_stab])
                self.logger.print(f"Change stabilizer to: {propagated_stab}\n")
                
            new_stabilizers.append(propagated_stab)
            new_destabilizers.append(propagated_destab)

            if len(propagated_stab) != 1:
                # if new_nonpauli_index != -1:
                #     raise ValueError(
                #         "More than one non-Pauli stabilizer found. "
                #         "This is not currently supported."
                #     )
                new_nonpauli_index = i_stab

        self.stabilizers = new_stabilizers
        self.destabilizers = new_destabilizers
        self.nonpauli_index = new_nonpauli_index
        self.buffer_layers = []
        self.is_circuit_buffer_nonclifford = False

    def apply_error(self):
        self.logger.print(f"Applying error to the tableau:\n{self.buffer_errors}")

        for tableau in [self.stabilizers, self.destabilizers]:
            for i in range(self.n):
                for error in self.buffer_errors:
                    tableau[i] = error * tableau[i] * error.dag()

        self.buffer_errors = []

    def evolve(self, layer: stim.CircuitInstruction):
        """Evolve the tableau with a given layer of the circuit."""
        measurement_results = []
        ignored_gates = [
            'QUBIT_COORDS', 'DETECTOR'
        ]
        if layer.name == 'RX':
            layer = stim.CircuitInstruction('H', layer.targets_copy())

        if layer.name in ignored_gates:
            pass
        elif layer.name in ['DEPOLARIZE1', 'DEPOLARIZE2', 'X_ERROR', 'Y_ERROR', 'Z_ERROR']:
            new_error = self.generate_error(
                list(map(lambda t: t.value, layer.targets_copy())), 
                layer.gate_args_copy()[0],
                direction_dict[layer.name]
            )
            self.buffer_errors.append(new_error)

        elif layer.name == 'TICK':
            self.logger.print("Tableau before TICK:")
            self.logger.print(self)

            self.apply_buffer(check_preservation=False)
            self.apply_error()

            self.logger.print("Tableau after TICK:")
            self.logger.print(self)

        elif layer.name == 'MR' or layer.name == 'R':
            self.get_nontrivial_logicals()
            meas_indices = list(map(lambda t: t.value, layer.targets_copy()))

            # if self.is_circuit_buffer_nonclifford:
            meas_prob = self.get_measurement_probability(meas_indices[0])
            self.logger.print(f"Measurement probability: {meas_prob}")

            self.apply_buffer(check_preservation=self.is_circuit_buffer_nonclifford)

            self.logger.print("Tableau before applying error:")
            self.logger.print(self)
            self.apply_error()

            self.logger.print("Tableau before measurement:")
            self.logger.print(self)

            for qubit in meas_indices:
                measurement_results.append(self.measure_and_reset(qubit, meas_prob=meas_prob))
            self.check_commutation_relations()

            self.logger.print("Tableau after measurement:")
            self.logger.print(self)
        else:
            self.buffer_layers.append(layer)
            if layer.tag in nonclifford_gates:
                self.is_circuit_buffer_nonclifford = True
            for i in range(len(self.buffer_errors)):
                self.buffer_errors[i] = propagation(layer, self.buffer_errors[i])

        return measurement_results

    def run_circuit(self, circuit: stim.Circuit):
        """Run a circuit on the tableau."""
        result = {'accept': False, 'fail': False}
        for layer in circuit:
            self.logger.print("=======================================================================")
            self.logger.print(layer)
            self.logger.print("=======================================================================")
            measurement_results = self.evolve(layer)

            if -1 in measurement_results:
                return result
            
        result['accept'] = True
        if self.stabilizers[self.nonpauli_index] != logical_H:
            result['fail'] = True

        return result

    def __str__(self):
        lines = ["== General Tableau =="]
        lines.append("Stabilizers")
        for stabilizer in self.stabilizers:
            lines.append(f"{stabilizer}")
        lines.append("\nDestabilizers")
        for destabilizer in self.destabilizers:
            lines.append(f"{destabilizer}")
        
        return "\n".join(lines)

    def __repr__(self):
        return self.__str__()