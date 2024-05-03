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
    #     from stim._stim_avx2 import *
    #     from stim._stim_avx2 import _UNSTABLE_raw_format_data, __version__
    if _tmp == 'avx2' or _tmp == 'sse2':
        from stim._stim_sse2 import *
        from stim._stim_sse2 import __version__
    else:
        from stim._stim_polyfill import *
        from stim._stim_polyfill import __version__
except ImportError:
    from stim._stim_polyfill import *
    from stim._stim_polyfill import __version__

del _tmp
