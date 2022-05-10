"""
This is the entrypoint when running `import stim`.

It does runtime detection of CPU features, and based on that imports the fastest pre-built C++ extension that only uses
compatible instructions. Importing a different one can result in runtime segfaults that crash the python interpreter.
"""
import stim._detect_machine_architecture as _tmp

_tmp = _tmp._UNSTABLE_detect_march()
if _tmp == 'avx2':
    from stim._stim_avx2 import *
    from stim._stim_avx2 import _UNSTABLE_raw_gate_data, _UNSTABLE_raw_format_data, __version__
elif _tmp == 'sse2':
    from stim._stim_sse2 import *
    from stim._stim_sse2 import _UNSTABLE_raw_gate_data, _UNSTABLE_raw_format_data, __version__
else:
    from stim._stim_polyfill import *
    from stim._stim_polyfill import _UNSTABLE_raw_gate_data, _UNSTABLE_raw_format_data, __version__
del _tmp


def _pytest_pycharm_pybind_repr_bug_workaround(cls):
    f = cls.__repr__
    cls.__repr__ = lambda e: f(e)
    cls.__repr__.__doc__ = f.__doc__


_pytest_pycharm_pybind_repr_bug_workaround(Circuit)
_pytest_pycharm_pybind_repr_bug_workaround(DetectorErrorModel)
_pytest_pycharm_pybind_repr_bug_workaround(Tableau)
_pytest_pycharm_pybind_repr_bug_workaround(PauliString)
del _pytest_pycharm_pybind_repr_bug_workaround
