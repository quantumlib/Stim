#ifndef _STIM_UTIL_TOP_CIRCUIT_INVERSE_QEC_H
#define _STIM_UTIL_TOP_CIRCUIT_INVERSE_QEC_H

#include "stim/circuit/circuit.h"

namespace stim {

template <size_t W>
std::pair<Circuit, std::vector<Flow<W>>> circuit_inverse_qec(
    const Circuit &circuit, std::span<const Flow<W>> flows, bool dont_turn_measurements_into_resets = false);

}  // namespace stim

#include "stim/util_top/circuit_inverse_qec.inl"

#endif
