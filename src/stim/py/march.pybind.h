#ifndef _STIM_PY_MARCH_PYBIND_H
#define _STIM_PY_MARCH_PYBIND_H

#include <string>

/// Returns a string indicating the capabilities of the CPU (the M-achine ARCH-itecture).
std::string detect_march();

#endif
