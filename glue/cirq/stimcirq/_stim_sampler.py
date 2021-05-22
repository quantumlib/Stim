from typing import List

import cirq

from ._cirq_to_stim import cirq_circuit_to_stim_data


class StimSampler(cirq.Sampler):
    """Samples stabilizer circuits using Stim.

    Supports circuits that contain Clifford operations, measurement operations, reset operations, and noise operations
    that can be decomposed into probabilistic Pauli operations. Unknown operations are supported as long as they provide
    a decomposition into supported operations via `cirq.decompose` (i.e. via a `_decompose_` method).

    Note that batch sampling is significantly faster (as in potentially thousands of times faster) than individual
    sampling, because it amortizes the cost of parsing and analyzing the circuit.
    """

    def run_sweep(
        self,
        program: cirq.Circuit,
        params: cirq.Sweepable,
        repetitions: int = 1,
    ) -> List[cirq.Result]:
        trial_results: List[cirq.Result] = []
        for param_resolver in cirq.to_resolvers(params):
            # Request samples from stim.
            instance = cirq.resolve_parameters(program, param_resolver)
            converted_circuit, key_ranges = cirq_circuit_to_stim_data(instance)
            samples = converted_circuit.compile_sampler().sample(repetitions)

            # Convert unlabelled samples into keyed results.
            k = 0
            measurements = {}
            for key, length in key_ranges:
                p = k
                k += length
                measurements[key] = samples[:, p:k]
            trial_results.append(cirq.Result(params=param_resolver, measurements=measurements))

        return trial_results
