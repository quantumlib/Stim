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
