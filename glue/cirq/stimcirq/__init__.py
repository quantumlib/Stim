from ._cirq_to_stim import cirq_circuit_to_stim_circuit
from ._det_annotation import DetAnnotation
from ._obs_annotation import CumulativeObservableAnnotation
from ._shift_coords_annotation import ShiftCoordsAnnotation
from ._stim_sampler import StimSampler
from ._stim_to_cirq import (
    MeasureAndOrResetGate,
    stim_circuit_to_cirq_circuit,
    TwoQubitAsymmetricDepolarizingChannel,
)
from ._sweep_pauli import SweepPauli

JSON_RESOLVERS_DICT = {
    "CumulativeObservableAnnotation": CumulativeObservableAnnotation,
    "DetAnnotation": DetAnnotation,
    "MeasureAndOrResetGate": MeasureAndOrResetGate,
    "ShiftCoordsAnnotation": ShiftCoordsAnnotation,
    "SweepPauli": SweepPauli,
    "TwoQubitAsymmetricDepolarizingChannel": TwoQubitAsymmetricDepolarizingChannel,
}
JSON_RESOLVER = JSON_RESOLVERS_DICT.get

# Workaround for doctest not searching imported objects.
__test__ = {
    "cirq_circuit_to_stim_circuit": cirq_circuit_to_stim_circuit,
    "CumulativeObservableAnnotation": CumulativeObservableAnnotation,
    "DetAnnotation": DetAnnotation,
    "MeasureAndOrResetGate": MeasureAndOrResetGate,
    "ShiftCoordsAnnotation": ShiftCoordsAnnotation,
    "stim_circuit_to_cirq_circuit": stim_circuit_to_cirq_circuit,
    "StimSampler": StimSampler,
    "SweepPauli": SweepPauli,
    "TwoQubitAsymmetricDepolarizingChannel": TwoQubitAsymmetricDepolarizingChannel,
}
