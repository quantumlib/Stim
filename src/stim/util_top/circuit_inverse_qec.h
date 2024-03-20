#ifndef _STIM_UTIL_TOP_CIRCUIT_INVERSE_QEC_H
#define _STIM_UTIL_TOP_CIRCUIT_INVERSE_QEC_H

#include "stim/circuit/circuit.h"

namespace stim {

template <size_t W>
Circuit circuit_inverse_qec(const Circuit &circuit, std::span<const Flow<W>> flows);

}  // namespace stim

#include "stim/util_top/circuit_inverse_qec.inl"

#endif
