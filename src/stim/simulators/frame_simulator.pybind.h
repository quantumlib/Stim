#ifndef _STIM_SIMULATORS_FRAME_SIMULATOR_PYBIND_H
#define _STIM_SIMULATORS_FRAME_SIMULATOR_PYBIND_H

#include <pybind11/pybind11.h>

#include "stim/simulators/frame_simulator.h"

namespace stim_pybind {

pybind11::class_<stim::FrameSimulator<stim::MAX_BITWORD_WIDTH>> pybind_frame_simulator(pybind11::module &m);

void pybind_frame_simulator_methods(
    pybind11::module &m, pybind11::class_<stim::FrameSimulator<stim::MAX_BITWORD_WIDTH>> &c);

}  // namespace stim_pybind

#endif
