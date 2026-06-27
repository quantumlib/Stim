import collections
from typing import Dict, List, Sequence

import cirq

from ._cirq_to_stim import cirq_circuit_to_stim_data


try:
    ResultImpl = cirq.ResultDict  # For cirq >= 0.14
except AttributeError:
    ResultImpl = cirq.Result  # For cirq < 0.14


class StimSampler(cirq.Sampler):
    """Samples stabilizer circuits using Stim.

    Supports circuits that contain Clifford operations, measurement operations, reset operations, and noise operations
    that can be decomposed into probabilistic Pauli operations. Unknown operations are supported as long as they provide
    a decomposition into supported operations via `cirq.decompose` (i.e. via a `_decompose_` method).

    Note that batch sampling is significantly faster (as in potentially thousands of times faster) than individual
    sampling, because it amortizes the cost of parsing and analyzing the circuit.
    """

    def run_sweep(
        self, program: cirq.Circuit, params: cirq.Sweepable, repetitions: int = 1
    ) -> Sequence[cirq.Result]:
        results: List[cirq.Result] = []
        for param_resolver in cirq.to_resolvers(params):
            # Request samples from stim.
            instance = cirq.resolve_parameters(program, param_resolver)
            converted_circuit, key_ranges = cirq_circuit_to_stim_data(instance, flatten=True)
            samples = converted_circuit.compile_sampler().sample(repetitions)

            # Find number of qubits (length), number of instances, and indices for each measurement key.
            lengths: Dict[str, int] = {}
            instances: Dict[str, int] = collections.Counter()
            indices: Dict[str, int] = collections.defaultdict(list)
            k = 0
            for key, length in key_ranges:
                prev_length = lengths.get(key)
                if prev_length is None:
                    lengths[key] = length
                elif length != prev_length:
                    raise ValueError(f"different numbers of qubits for key {key}: {prev_length} != {length}")
                instances[key] += 1
                indices[key].extend(range(k, k + length))
                k += length

            # Convert unlabelled samples into keyed results.
            records = {
                key: samples[:, indices[key]].reshape((repetitions, instances[key], length))
                for key, length in lengths.items()
            }
            results.append(ResultImpl(params=param_resolver, records=records))

        return results
