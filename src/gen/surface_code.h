#ifndef STIM_SURFACE_CODE_H
#define STIM_SURFACE_CODE_H

#include "../circuit/circuit.h"
#include "circuit_gen_main.h"

namespace stim_internal {
    GeneratedCircuit generate_rotated_surface_code_circuit(const CircuitGenParameters &params);
    GeneratedCircuit generate_unrotated_surface_code_circuit(const CircuitGenParameters &params);
}

#endif
