#ifndef STIM_REP_CODE_H
#define STIM_REP_CODE_H

#include "../circuit/circuit.h"
#include "circuit_gen_params.h"

namespace stim_internal {
GeneratedCircuit generate_rep_code_circuit(const CircuitGenParameters &params);
}

#endif
