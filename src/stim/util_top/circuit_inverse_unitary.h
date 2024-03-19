#ifndef _STIM_UTIL_TOP_CIRCUIT_INVERSE_UNITARY_H
#define _STIM_UTIL_TOP_CIRCUIT_INVERSE_UNITARY_H

#include "stim/circuit/circuit.h"

namespace stim {

/// Inverts the given circuit, as long as it only contains unitary operations.
Circuit unitary_circuit_inverse(const Circuit &unitary_circuit);

}

#endif
