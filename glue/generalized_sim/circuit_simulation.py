import os
import time
from itertools import product
from typing import TypeAlias, Optional, Union
import random

import numpy as np
import numpy.typing as npt
import sinter
import stim
from panqec.codes import StabilizerCode, Color666PlanarCode

from pauli_sum import PauliSum
from utils import stringify, str_to_bsf, bsf_to_str, isclose
from logger import Logger
from circuits import generate_circuit
from gates import propagation

PauliBSF: TypeAlias = npt.NDArray[np.uint8]
BinaryVect: TypeAlias = npt.NDArray[np.uint8]
WeightedPauliBSF: TypeAlias = tuple[float, PauliBSF]

np.random.seed(0)


logical_I = PauliSum({'I': 1})
logical_H = PauliSum({'X': np.sqrt(2), 'Z': np.sqrt(2)})
logical_X = PauliSum({'X': 1})

direction_dict = {
    'DEPOLARIZE1': tuple([1/3 for _ in range(3)]),
    'DEPOLARIZE2': tuple([1/15 for _ in range(15)]),
    'X_ERROR': (1, 0, 0),
    'Y_ERROR': (0, 1, 0),
    'Z_ERROR': (0, 0, 1),
}


def bs_product(matrix: npt.NDArray[np.uint8], pauli: npt.NDArray[np.uint8]) -> bool:
    """
    Calculate the syndrome of a Pauli vector with respect to a matrix, 
    both given in the bsf format.
    """
    n = matrix.shape[1] // 2

    matrix_X = matrix[:, :n]
    matrix_Z = matrix[:, n:]
    pauli_X = pauli[:n]
    pauli_Z = pauli[n:]

    prod = (matrix_X.dot(pauli_Z) + matrix_Z.dot(pauli_X)) % 2

    return prod


class CircuitSimulation:
    def __init__(
        self,
        code: StabilizerCode,
        circuit: stim.Circuit,
        global_measured_op: str = 'H',
        logger=None
    ):
        if logger is None:
            self.logger = Logger('log.txt', stdout_print=False)
        else:
            self.logger = logger

        self.code = code
        self.circuit = circuit
        self.n_qubits_circuit = circuit.num_qubits
        self.global_measured_op = global_measured_op

        self.Hx = self.code.Hx.toarray()
        self.Hz = self.code.Hz.toarray()
        self.Lx = self.code.logicals_x[0][:self.code.n]
        self.Lz = self.code.logicals_z[0][self.code.n:]

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

        error = ['I'] * self.n_qubits_circuit
        for i, pos in enumerate(positions):
            error[pos] = restricted_error[i]

        return PauliSum({''.join(error): 1})
    
    def get_syndrome(self, error_bsf: npt.NDArray) -> str:
        """Measure the syndrome of a given error using the stabilizer matrices"""
        n = self.code.n

        syndrome = (
            stringify(self.Hx@error_bsf[n:] % 2) +
            stringify(self.Hz@error_bsf[:n] % 2)
        )

        return syndrome
    
    # @profile
    def logical_errors(self, error_bsf: npt.NDArray) -> str:
        """Measure the syndrome of a given error using the stabilizer matrices"""
        n = self.code.n

        logical_syndrome = [self.Lz@error_bsf[:n] % 2, self.Lx@error_bsf[n:] % 2]

        return logical_syndrome
    
    # @profile
    def project(
        self, 
        meas_positions: list, 
        syndrome: str, 
        error: PauliSum,
        pauli_type: str = 'Z'
    ) -> PauliSum:
        projected_op = PauliSum()

        for pauli, coeff in error:
            commutation = stringify([
                int(pauli[i] not in [pauli_type, 'I'])
                for i in meas_positions
            ])
            if commutation == syndrome:
                projected_op[pauli] = coeff

        return projected_op
    
    # @profile
    def decompose_error(self, error: PauliSum):
        """Decompose error into Pauli error times logical gate, assuming
        all the terms in the error have the same syndrome
        """

        first_pauli, coeff = list(error.items())[0]
        pauli_error = PauliSum({first_pauli: 1})
        factorized_error = pauli_error * error
        logical_error = self.physical_to_logical(factorized_error)

        return pauli_error, logical_error
    
    # @profile
    def physical_to_logical(self, pauli_sum: PauliSum) -> PauliSum:
        """Convert a physical error to a logical error using the code's stabilizer matrix."""
        logical_terms = dict()

        for pauli, coeff in pauli_sum.items():
            logical_pauli = bsf_to_str(
                self.logical_errors(
                    str_to_bsf(pauli[:self.code.n])
                )
            )
            logical_terms[logical_pauli] = logical_terms.get(logical_pauli, 0) + coeff

        return PauliSum(logical_terms)
    
    # @profile
    def measurement_distribution(
        self, 
        meas_positions: list,
        error: PauliSum,
        is_H_meas: bool = False,
        pauli_type: str = 'Z'
    ) -> tuple[dict[str, float], list[PauliSum]]:
        new_error: dict[PauliSum] = dict()
        syndrome_prob: dict[float] = dict()

        possible_syndromes = set([
            stringify([
                int(pauli[i] != pauli_type and pauli[i] != 'I')
                for i in meas_positions
            ])
            for pauli in error.keys()
        ])

        for s in possible_syndromes:
            self.logger.print(f"Syndrome: {s}")
            syndrome_prob[s] = 0

            new_error[s] = self.project(meas_positions, s, error, pauli_type)
            self.logger.print(f"New error: {new_error[s]}")

            # If H measurement, we cannot rely on the fact that all terms
            # in the Pauli sum have the same syndrome
            if is_H_meas:
                sandwiched_op: PauliSum = error.dag() * new_error[s]
                self.logger.print(f"Initial sandwiched op: {sandwiched_op}")
                simplified_sandwiched_op = PauliSum()
                for pauli, coeff in sandwiched_op:
                    restricted_pauli_bsf = str_to_bsf(pauli[:self.code.n])
                    if not ('1' in self.get_syndrome(restricted_pauli_bsf)):
                        simplified_sandwiched_op[pauli] = coeff
                sandwiched_op = simplified_sandwiched_op
                self.logger.print(f"Final sandwiched op: {sandwiched_op}")
            else:
                sandwiched_op: PauliSum = new_error[s].dag() * new_error[s]
                self.logger.print(f"Sandwiched op: {sandwiched_op}")

            sandwiched_op_log = self.physical_to_logical(sandwiched_op)
            self.logger.print(f"Logical sandwiched op: {sandwiched_op_log}")

            for pauli, coeff in sandwiched_op_log:
                if pauli == 'I':
                    syndrome_prob[s] += coeff
                elif pauli in ['X', 'Z']:
                    syndrome_prob[s] += coeff / np.sqrt(2)

            if not isclose(abs(syndrome_prob[s]), syndrome_prob[s]):
                raise ValueError(
                    f"Syndrome probability {syndrome_prob[s]} is not a positive real number"
                )

            syndrome_prob[s] = abs(syndrome_prob[s])

        sum_prob = sum(list(syndrome_prob.values()))

        if sum_prob == 0:
            raise ValueError(
                f"The syndrome probabilities are all zero."
                f"\nPossible syndromes {syndrome_prob}"
            )
        
        for s in syndrome_prob.keys():
            syndrome_prob[s] /= sum_prob

        return syndrome_prob, new_error
    
    # @profile
    def measure(
        self, 
        meas_positions: list,
        error: PauliSum,
        is_H_meas: bool = False
    ) -> tuple[dict[str, float], list[PauliSum]]:
        possible_syndromes, possible_propagations = self.measurement_distribution(
            meas_positions, error, is_H_meas
        )

        selected_syndrome = random.choices(
            list(possible_syndromes.keys()), 
            weights=list(possible_syndromes.values())
        )[0]

        self.logger.print(f"Possible syndromes: {possible_syndromes}")
        self.logger.print(f"Possible propagations\n {possible_propagations}")
        self.logger.print(f"Selected syndrome: {selected_syndrome}")

        norm = np.sqrt(possible_syndromes[selected_syndrome])
        new_error = possible_propagations[selected_syndrome] / norm

        self.logger.print(f"New error: {new_error}")

        return selected_syndrome, new_error

    # @profile
    def run_once(self):
        self.logger.print("-" * 40 + " Run " + "-" * 40)

        result = {'accept': False, 'fail': False}

        error = PauliSum(self.n_qubits_circuit)
        # logical_error = PauliSum(1)
        for layer in self.circuit:
            self.logger.print('\n' + '=' * 50)
            self.logger.print(layer)
            self.logger.print('=' * 50)

            if layer.name in ['TICK', 'R', 'QUBIT_COORDS', 'DETECTOR']:
                pass
            elif layer.name in ['DEPOLARIZE1', 'DEPOLARIZE2', 'X_ERROR', 'Y_ERROR', 'Z_ERROR']:
                new_error = self.generate_error(
                    list(map(lambda t: t.value, layer.targets_copy())), 
                    layer.gate_args_copy()[0],
                    direction_dict[layer.name]
                )
                # new_pauli = 'IIIIZXX' if len(error.keys()) == 1 else 'IIXXZIX'
                # new_error = PauliSum({new_pauli + 'I'*13: 1})

                self.logger.print(f"New error: {new_error}")

                error = new_error * error
            elif layer.name == 'MR':
                syndrome, error = self.measure(
                    list(map(lambda t: t.value, layer.targets_copy())),
                    error,
                    (layer.tag == 'H')
                )

                if '1' in syndrome:
                    return result
                                
                error = error.restrict_up_to(self.code.n)
                
                # if layer.tag != 'H':
                #     if '1' in self.get_syndrome(str_to_bsf(list(error.keys())[0][:self.code.n])):
                #         error, new_logical_error = self.decompose_error(error)
                #         logical_error *= new_logical_error
                #     else:
                #         logical_error = self.physical_to_logical(error)
                #         error = PauliSum(self.n_qubits_circuit)
                
                # self.logger.print(f"Logical error: {logical_error}")
            else:
                propagated_error = propagation(layer, error)

            self.logger.print(f"Propagated error {error}")

        # if len(error.keys()) > 1:
        #     raise ValueError(
        #         f"The final error must be a Pauli error. Current error: {error}"
        #     )
        
        restricted_error_bsf = str_to_bsf(list(error.keys())[0][:self.code.n])

        if '1' in self.get_syndrome(restricted_error_bsf):
            raise ValueError(
                f"The final error must be in the codespace. Current error: {error}"
            )
        
        # logical_error *= self.physical_to_logical(error)
        logical_error = self.physical_to_logical(error)

        self.logger.print(f"Final physical error {error}")
        self.logger.print(f"Final logical error {logical_error}")

        # print(f"Logical error {logical_error}")

        result['accept'] = True
    
        if not (
            logical_error.proportional_to(logical_I) or
            (self.global_measured_op == 'H' and logical_error.proportional_to(logical_H)) or
            (self.global_measured_op == 'X' and logical_error.proportional_to(logical_X))
        ):
            result['fail'] = True
            self.logger.print("Failure")
            self.logger.print(f"Logical error {logical_error}")

        self.logger.print("Terminates")
        return result
    

class Sampler(sinter.Sampler):
    def compiled_sampler_for_task(self, task: sinter.Task) -> sinter.CompiledSampler:
        n_layers = task.json_metadata['n_layers']
        p_phys = task.json_metadata['p_phys']
        p_meas = task.json_metadata['p_meas']
        replace_T_with_S = task.json_metadata['replace_T_with_S']
        L = task.json_metadata['L']

        return CompiledSampler(n_layers, p_phys, p_meas, replace_T_with_S, L)


class CompiledSampler(sinter.CompiledSampler):
    def __init__(self, n_layers, p_phys, p_meas, replace_T_with_S, L):
        self.n_layers = n_layers
        self.p_phys = p_phys
        self.p_meas = p_meas
        self.L = L

        logger = Logger('log.txt', logging=False, stdout_print=False)
        code = Color666PlanarCode(L)
        circuit = generate_circuit(
            code, 
            n_rounds=n_layers, 
            p_phys=p_phys, 
            p_meas=p_meas,
            noise_model='pheno',
            replace_T_with_S=replace_T_with_S
        )
        global_measured_op = 'X' if replace_T_with_S else 'H'

        self.sim = CircuitSimulation(
            code, circuit, global_measured_op=global_measured_op, logger=logger
        )

    def sample(self, shots: int) -> sinter.AnonTaskStats:
        t0 = time.monotonic()
        n_accept, n_fail = 0, 0
        for k in range(shots):
            run = self.sim.run_once()
            n_fail += int(run['fail'])
            n_accept += int(run['accept'])

        t1 = time.monotonic()

        return sinter.AnonTaskStats(
            shots=shots, 
            errors=n_fail,
            discards=shots - n_accept, 
            seconds=t1-t0
        )
            

if __name__ == "__main__":
    max_shots = int(1e9)
    max_errors = 2000
    p_phys_list = [0.01, 0.02, 0.05, 0.1, 0.15, 0.2]
    n_layers_list = [1]
    L_list = [2]
    replace_T_with_S_list = [True, False]

    parameters = product(
        p_phys_list, n_layers_list, replace_T_with_S_list, L_list
    )

    tasks = []
    for p_phys, n_layers, replace_T_with_S, L in parameters:
        tasks.append(sinter.Task(circuit=stim.Circuit(), json_metadata={
            'n_layers': n_layers,
            'p_phys': p_phys,
            'p_meas': np.sqrt(p_phys),
            'replace_T_with_S': replace_T_with_S,
            'L': L
        }))

    state = sinter.collect(
        print_progress=True,
        save_resume_filepath='result-circuit-2.csv',
        tasks=tasks,
        max_shots=max_shots,
        max_errors=max_errors,
        num_workers=os.cpu_count(),
        decoders=['simulation'],
        custom_decoders={'simulation': Sampler()}
    )
    

# if __name__ == "__main__":
#     code = Color666PlanarCode(1)
#     logger = Logger('log.txt', logging=False, stdout_print=False)

#     sim = Simulation(code, logger=logger)

#     for i in range(100):
#         sim.run_once()