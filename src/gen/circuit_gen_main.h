#ifndef STIM_CIRCUIT_GEN_MAIN_H
#define STIM_CIRCUIT_GEN_MAIN_H

#include <map>
#include <stdint.h>
#include <stddef.h>
#include "../circuit/circuit.h"

namespace stim_internal {
    int main_generate_circuit(int argc, const char **argv, FILE *out);
}

#endif
