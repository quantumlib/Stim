import os
import time
from itertools import product
from typing import TypeAlias, Optional

import numpy as np
import numpy.typing as npt
import sinter
import stim
from panqec.codes import StabilizerCode, Color666PlanarCode

from pauli_sum import PauliSum
from utils import stringify, unstringify, str_to_bsf, bsf_to_str, isclose
from logger import Logger

PauliBSF: TypeAlias = npt.NDArray[np.uint8]
BinaryVect: TypeAlias = npt.NDArray[np.uint8]
WeightedPauliBSF: TypeAlias = tuple[float, PauliBSF]


def hadamard(op: PauliSum) -> np.ndarray:
    new_terms = dict()

    h_map = {'I': 'I', 'X': 'Z', 'Y': 'Y', 'Z': 'X'}
    for pauli, coeff in op._terms.items():
        new_pauli = ''.join([h_map[p] for p in pauli])
        new_coeff = -coeff if pauli.count('Y') % 2 == 1 else coeff
        new_terms[new_pauli] = new_coeff

    h_op = PauliSum(new_terms)

    return h_op


logical_I = PauliSum({'I': 1})
logical_H = PauliSum({'X': np.sqrt(2), 'Z': np.sqrt(2)})
logical_X = PauliSum({'X': 1})
logical_Y = PauliSum({'Y': 1})
logical_Z = PauliSum({'Z': 1})


class PhenomenologicalSimulation:
    def __init__(
        self,
        code: StabilizerCode,
        error_model: tuple[float] = (1/3, 1/3, 1/3),
        logger=None
    ):
        self.code = code
        self.error_model = error_model

        self.H = code.stabilizer_matrix.toarray()
        self.Hx = code.Hx.toarray()
        self.Hz = code.Hz.toarray()

        if logger is None:
            self.logger = Logger('log.txt', stdout_print=False)
        else:
            self.logger = logger

        self.project = {
            'H': self.project_H,
            'C': self.project_codespace,
            'Y': self.project_Y,
            'X': self.project_X,
            'Z': self.project_Z}

    def is_stabilizer(self, pauli: str):
        pauli_bsf = str_to_bsf(pauli)
        return (
            np.all(self.get_syndrome(pauli_bsf) == 0) and
            np.all(self.code.logical_errors(pauli_bsf) == 0)
        )

    def generate_error(self, p_phys):
        sx, sy, sz = self.error_model

        return ''.join(list(np.random.choice(
            ['I', 'X', 'Y', 'Z'],
            size=self.code.n,
            p=[1-p_phys, sx*p_phys, sy*p_phys, sz*p_phys]
        )))

    def physical_to_logical(self, pauli_sum: PauliSum) -> PauliSum:
        """Convert a physical error to a logical error using the code's stabilizer matrix."""
        logical_terms = dict()

        for pauli, coeff in pauli_sum.items():
            logical_pauli = bsf_to_str(self.code.logical_errors(str_to_bsf(pauli)))
            logical_terms[logical_pauli] = logical_terms.get(logical_pauli, 0) + coeff

        return PauliSum(logical_terms)

    def project_H(self, syndrome: BinaryVect, error: PauliSum) -> PauliSum:
        """Calculate Pi^s E when acting on the state |H>,
        for a syndrome s and error E that takes the form of a Pauli sum.
        The syndrome can only be [0] (corresponding to +1) or [1] (corresponding to -1).
        """
        projected_op = 0.5 * (error + (-1)**syndrome[0] * hadamard(error))

        return projected_op

    def project_X(self, syndrome: BinaryVect, error: PauliSum) -> PauliSum:
        """Calculate Pi^s E when acting on the state |Y>,
        for a syndrome s and error E that takes the form of a Pauli sum.
        The syndrome can only be [0] (corresponding to +1) or [1] (corresponding to -1).
        """
        projected_op = PauliSum()

        for pauli, coeff in error:
            if (pauli.count('Z') + pauli.count('Y')) % 2 == syndrome[0]:
                projected_op += {pauli: coeff}

        return projected_op

    def project_Y(self, syndrome: BinaryVect, error: PauliSum) -> PauliSum:
        """Calculate Pi^s E when acting on the state |Y>,
        for a syndrome s and error E that takes the form of a Pauli sum.
        The syndrome can only be [0] (corresponding to +1) or [1] (corresponding to -1).
        """
        projected_op = PauliSum()

        for pauli, coeff in error:
            if (pauli.count('X') + pauli.count('Z')) % 2 == syndrome[0]:
                projected_op += {pauli: coeff}

        return projected_op

    def project_Z(self, syndrome: BinaryVect, error: PauliSum) -> PauliSum:
        """Calculate Pi^s E when acting on the state |Z>,
        for a syndrome s and error E that takes the form of a Pauli sum.
        The syndrome can only be [0] (corresponding to +1) or [1] (corresponding to -1).
        """
        projected_op = PauliSum()

        for pauli, coeff in error:
            if (pauli.count('X') + pauli.count('Y')) % 2 == syndrome[0]:
                projected_op += {pauli: coeff}

        return projected_op

    def project_codespace(self, syndrome: BinaryVect, error: PauliSum) -> PauliSum:
        projected_op = PauliSum()

        for pauli, coeff in error:
            pauli_bsf = str_to_bsf(pauli)
            if np.all(self.get_syndrome(pauli_bsf) == syndrome):
                projected_op += {pauli: coeff}

        return projected_op

    def measure(self, proj_key: str, error: PauliSum) -> tuple[dict[str, float], list[PauliSum]]:
        new_error = dict()
        syndrome_prob = dict()

        if proj_key in ['H', 'Y', 'X', 'Z']:
            possible_syndromes = {'0', '1'}
        elif proj_key == "C":
            possible_syndromes = set([
                stringify(self.get_syndrome(str_to_bsf(pauli))) for pauli in error.keys()
            ])
        else:
            raise ValueError("Invalid projection operator. Use 'H', 'X', 'Y', 'Z' or 'C'.")

        for s in possible_syndromes:
            syndrome_prob[s] = 0

            new_error[s] = self.project[proj_key](unstringify(s), error)

            sandwiched_op: PauliSum = error.dag() * new_error[s]
            self.logger.print(f"Initial sandwiched op: {sandwiched_op}")

            simplified_sandwiched_op = PauliSum()
            for pauli, coeff in sandwiched_op:
                pauli_bsf = str_to_bsf(pauli)
                if np.all(self.get_syndrome(pauli_bsf) == 0):
                    simplified_sandwiched_op[pauli] = coeff
            sandwiched_op = simplified_sandwiched_op

            self.logger.print(f"Final sandwiched op: {sandwiched_op}")

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

        sum_prob = np.sum(list(syndrome_prob.values()))
        
        if sum_prob == 0:
            raise ValueError("The syndrome probabilities are all zero")

        for s in syndrome_prob.keys():
            syndrome_prob[s] /= sum_prob

        return syndrome_prob, new_error

    def get_syndrome(self, error_bsf: npt.NDArray) -> npt.NDArray:
        """Measure the syndrome of a given error using the stabilizer matrices"""
        n = self.Hx.shape[1]

        syndrome = np.concatenate([self.Hx@error_bsf[n:], self.Hz@error_bsf[:n]]) % 2

        return syndrome

    def noisy_measurement_layer(
        self,
        measured_op: str,
        p_phys: float,
        p_meas: float = 0,
        init_error: Optional[PauliSum] = None,
    ) -> tuple[BinaryVect, PauliSum]:
        """Perform a noisy measurement layer on the code.

        Args:
            measured_op (str):
                The operator to measure ('H', 'C', 'Y', or 'X').
            p_phys (float):
                The physical error probability.
            p_meas (float, optional):
                The measurement error probability. Defaults to 0.
            init_error (PauliSum, optional):
                Initial error to apply before measurement. Defaults to None.
        Returns:
            tuple: A tuple containing the syndrome and new error
        """
        self.logger.print('\n' + '=' * 50)
        self.logger.print(f"Measuring {measured_op}")
        self.logger.print('=' * 50)
        self.logger.print(f"  Initial error:\n\t{init_error}")

        random_pauli = self.generate_error(p_phys)
        # random_pauli = 'IIIIZXX' if len(init_error.keys()) == 1 else 'IIXXZIX'
        new_error = PauliSum({random_pauli: 1})

        total_error_before = new_error * init_error if init_error else new_error

        self.logger.print(f"New error:\n\t{new_error}")
        self.logger.print(f"Total error before measurement:\n\t{total_error_before}")
        
        probs, possible_propagations = self.measure(measured_op, total_error_before)

        selected_syndrome = np.random.choice(list(probs.keys()), p=list(probs.values()))

        norm = np.sqrt(probs[selected_syndrome])
        propagated_error = possible_propagations[selected_syndrome] / norm

        if p_meas > 0:
            syndrome_perturbation = np.random.choice(
                [0, 1],
                size=len(selected_syndrome),
                p=[1-p_meas, p_meas]
            )
            selected_syndrome_perturbed = stringify(
                (syndrome_perturbation + unstringify(selected_syndrome)) % 2
            )
        else:
            selected_syndrome_perturbed = selected_syndrome

        self.logger.print(f"Syndrome probabilities:\n\t{probs}")
        self.logger.print(f"Possible propagations:\n\t{possible_propagations}")
        self.logger.print(f"Selected syndrome: {selected_syndrome}")
        self.logger.print(f"Selected syndrome perturbed: {selected_syndrome_perturbed}")
        self.logger.print(f"Final error:\n\t{propagated_error}")

        return selected_syndrome_perturbed, propagated_error
    
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
    def run_once(self, n_layers, p_phys, p_meas=0, global_measured_op='H'):
        result = {'accept': False, 'fail': False}

        physical_error = PauliSum({'I' * self.code.n: 1})
        logical_error = PauliSum({'I': 1})
        for layer in range(n_layers):
            if layer == n_layers - 1:
                p_meas = 0

            for measured_op in [global_measured_op, 'C']:
                syndrome, physical_error = self.noisy_measurement_layer(
                    measured_op,
                    p_phys,
                    p_meas=p_meas,
                    init_error=physical_error,
                )

                if '1' in syndrome:
                    return result

            self.logger.print(f"Physical error:\n{physical_error}")

            if np.all(self.get_syndrome(str_to_bsf(list(physical_error.keys())[0])) == 0):
                log_err = self.physical_to_logical(physical_error)
                physical_error = PauliSum(self.code.n)
            else:
                physical_error, log_err = self.decompose_error(physical_error)
            self.logger.print(f"Decomposition:\n{physical_error}\n{log_err}\n")
            logical_error *= log_err


        result['accept'] = True

        self.logger.print("Accept!")
        self.logger.print(f"Logical error:\n{logical_error}")

        # print(f"Logical error {logical_error}")

        if not (
            logical_error.proportional_to(logical_I) or
            (global_measured_op == 'Y' and logical_error.proportional_to(logical_Y)) or
            (global_measured_op == 'X' and logical_error.proportional_to(logical_X)) or
            (global_measured_op == 'Z' and logical_error.proportional_to(logical_Z)) or
            (global_measured_op == 'H' and logical_error.proportional_to(logical_H))
        ):
            self.logger.print("Failure!")
            self.logger.print(f"Logical error {logical_error}")
            result['fail'] = True

        return result


class Sampler(sinter.Sampler):
    def compiled_sampler_for_task(self, task: sinter.Task) -> sinter.CompiledSampler:
        n_layers = task.json_metadata['n_layers']
        measured_op = task.json_metadata['measured_op']
        p_phys = task.json_metadata['p_phys']
        p_meas = task.json_metadata['p_meas']

        return CompiledSampler(n_layers, measured_op, p_phys, p_meas)


class CompiledSampler(sinter.CompiledSampler):
    def __init__(self, n_layers, measured_op, p_phys, p_meas):
        self.n_layers = n_layers
        self.measured_op = measured_op
        self.p_phys = p_phys
        self.p_meas = p_meas

        logger = Logger('log.txt', logging=False, stdout_print=False)
        code = Color666PlanarCode(1)

        self.sim = PhenomenologicalSimulation(code, logger=logger)

    def sample(self, shots: int) -> sinter.AnonTaskStats:
        t0 = time.monotonic()
        n_accept, n_fail = 0, 0
        for k in range(shots):
            run = self.sim.run_once(
                self.n_layers, 
                self.p_phys, 
                global_measured_op=self.measured_op,
                p_meas=self.p_meas
            )
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
    max_shots = int(1e8)
    max_errors = 1000
    p_phys_list = [0.01, 0.02, 0.05]
    measured_op_list = ['H', 'X']
    n_layers_list = [1]

    parameters = product(
        p_phys_list, measured_op_list, n_layers_list
    )

    tasks = []
    for p_phys, measured_op, n_layers in parameters:
        tasks.append(sinter.Task(circuit=stim.Circuit(), json_metadata={
            'n_layers': n_layers,
            'measured_op': measured_op,
            'p_phys': p_phys,
            'p_meas': np.sqrt(p_phys)
        }))

    state = sinter.collect(
        print_progress=True,
        save_resume_filepath='result-pheno.csv',
        tasks=tasks,
        max_shots=max_shots,
        max_errors=max_errors,
        num_workers=os.cpu_count(),
        decoders=['simulation'],
        custom_decoders={'simulation': Sampler()}
    )