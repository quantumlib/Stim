__version__ = "1.15.dev0"
from ._cirq_to_stim import cirq_circuit_to_stim_circuit
from ._cx_swap_gate import CXSwapGate
from ._cz_swap_gate import CZSwapGate
from ._det_annotation import DetAnnotation
from ._obs_annotation import CumulativeObservableAnnotation
from ._shift_coords_annotation import ShiftCoordsAnnotation
from ._stim_sampler import StimSampler
from ._stim_to_cirq import (
    MeasureAndOrResetGate,
    stim_circuit_to_cirq_circuit,
)
from ._sweep_pauli import SweepPauli
from ._two_qubit_asymmetric_depolarize import TwoQubitAsymmetricDepolarizingChannel
from ._i_error_gate import IErrorGate
from ._ii_error_gate import IIErrorGate
from ._ii_gate import IIGate

JSON_RESOLVERS_DICT = {
    "CumulativeObservableAnnotation": CumulativeObservableAnnotation,
    "DetAnnotation": DetAnnotation,
    "MeasureAndOrResetGate": MeasureAndOrResetGate,
    "ShiftCoordsAnnotation": ShiftCoordsAnnotation,
    "SweepPauli": SweepPauli,
    "TwoQubitAsymmetricDepolarizingChannel": TwoQubitAsymmetricDepolarizingChannel,
    "CXSwapGate": CXSwapGate,
    "CZSwapGate": CZSwapGate,
    "IIGate": IIGate,
    "IIErrorGate": IIErrorGate,
    "IErrorGate": IErrorGate,
}
JSON_RESOLVER = JSON_RESOLVERS_DICT.get
