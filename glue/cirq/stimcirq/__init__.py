from ._cirq_to_stim import cirq_circuit_to_stim_circuit
from ._stim_sampler import StimSampler
from ._stim_to_cirq import stim_circuit_to_cirq_circuit, MeasureAndOrReset, TwoQubitAsymmetricDepolarizingChannel

# Workaround for doctest not searching imported objects.
__test__ = {
    "StimSampler": StimSampler,
    "cirq_circuit_to_stim_circuit": cirq_circuit_to_stim_circuit,
    "stim_circuit_to_cirq_circuit": stim_circuit_to_cirq_circuit,
    "MeasureAndOrReset": MeasureAndOrReset,
    "TwoQubitAsymmetricDepolarizingChannel": TwoQubitAsymmetricDepolarizingChannel,
}
