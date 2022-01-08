from ._cirq_to_stim import cirq_circuit_to_stim_circuit
from ._det_annotation import DetAnnotation
from ._obs_annotation import CumulativeObservableAnnotation
from ._stim_sampler import StimSampler
from ._stim_to_cirq import stim_circuit_to_cirq_circuit, MeasureAndOrResetGate, TwoQubitAsymmetricDepolarizingChannel

JSON_RESOLVER = {
    "CumulativeObservableAnnotation": CumulativeObservableAnnotation._from_json_helper,
    "DetAnnotation": DetAnnotation._from_json_helper,
    "MeasureAndOrResetGate": MeasureAndOrResetGate._from_json_helper,
    "TwoQubitAsymmetricDepolarizingChannel": TwoQubitAsymmetricDepolarizingChannel._from_json_helper,
}.get

# Workaround for doctest not searching imported objects.
__test__ = {
    "cirq_circuit_to_stim_circuit": cirq_circuit_to_stim_circuit,
    "CumulativeObservableAnnotation": CumulativeObservableAnnotation,
    "DetAnnotation": DetAnnotation,
    "MeasureAndOrResetGate": MeasureAndOrResetGate,
    "stim_circuit_to_cirq_circuit": stim_circuit_to_cirq_circuit,
    "StimSampler": StimSampler,
    "TwoQubitAsymmetricDepolarizingChannel": TwoQubitAsymmetricDepolarizingChannel,
}
