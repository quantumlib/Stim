"""Stim: a fast quantum stabilizer circuit library."""

# This is the entrypoint when running `import stim`.
#
# It does runtime detection of CPU features, and based on that imports the fastest pre-built C++ extension that only uses
# compatible instructions. Importing a different one can result in runtime segfaults that crash the python interpreter.

import stim._detect_machine_architecture as _tmp


_tmp = _tmp._UNSTABLE_detect_march()
try:
    # NOTE: avx2 disabled until https://github.com/quantumlib/Stim/issues/432 is fixed
    # if _tmp == 'avx2':
    #     from stim._stim_avx2 import _UNSTABLE_raw_format_data, __version__
    #     from stim._stim_avx2 import *
    if _tmp == 'avx2' or _tmp == 'sse2':
        from stim._stim_sse2 import _UNSTABLE_raw_format_data, __version__
        from stim._stim_sse2 import *
    else:
        from stim._stim_polyfill import _UNSTABLE_raw_format_data, __version__
        from stim._stim_polyfill import *
except ModuleNotFoundError:
    from stim._stim_polyfill import _UNSTABLE_raw_format_data, __version__
    from stim._stim_polyfill import *

del _tmp


def _pytest_pycharm_pybind_repr_bug_workaround(cls):
    f = cls.__repr__
    cls.__repr__ = lambda e: f(e)
    cls.__repr__.__doc__ = f.__doc__
_pytest_pycharm_pybind_repr_bug_workaround(Circuit)
_pytest_pycharm_pybind_repr_bug_workaround(CircuitErrorLocation)
_pytest_pycharm_pybind_repr_bug_workaround(CircuitErrorLocationStackFrame)
_pytest_pycharm_pybind_repr_bug_workaround(CircuitInstruction)
_pytest_pycharm_pybind_repr_bug_workaround(CircuitRepeatBlock)
_pytest_pycharm_pybind_repr_bug_workaround(CircuitTargetsInsideInstruction)
_pytest_pycharm_pybind_repr_bug_workaround(CompiledDemSampler)
_pytest_pycharm_pybind_repr_bug_workaround(CompiledDetectorSampler)
_pytest_pycharm_pybind_repr_bug_workaround(CompiledMeasurementSampler)
_pytest_pycharm_pybind_repr_bug_workaround(CompiledMeasurementsToDetectionEventsConverter)
_pytest_pycharm_pybind_repr_bug_workaround(DemInstruction)
_pytest_pycharm_pybind_repr_bug_workaround(DemRepeatBlock)
_pytest_pycharm_pybind_repr_bug_workaround(DemTarget)
_pytest_pycharm_pybind_repr_bug_workaround(DemTargetWithCoords)
_pytest_pycharm_pybind_repr_bug_workaround(DetectorErrorModel)
_pytest_pycharm_pybind_repr_bug_workaround(ExplainedError)
_pytest_pycharm_pybind_repr_bug_workaround(FlipSimulator)
_pytest_pycharm_pybind_repr_bug_workaround(FlippedMeasurement)
_pytest_pycharm_pybind_repr_bug_workaround(Flow)
_pytest_pycharm_pybind_repr_bug_workaround(GateData)
_pytest_pycharm_pybind_repr_bug_workaround(GateTarget)
_pytest_pycharm_pybind_repr_bug_workaround(GateTargetWithCoords)
_pytest_pycharm_pybind_repr_bug_workaround(PauliString)
_pytest_pycharm_pybind_repr_bug_workaround(PauliStringIterator)
_pytest_pycharm_pybind_repr_bug_workaround(Tableau)
_pytest_pycharm_pybind_repr_bug_workaround(TableauIterator)
_pytest_pycharm_pybind_repr_bug_workaround(TableauSimulator)
del _pytest_pycharm_pybind_repr_bug_workaround
