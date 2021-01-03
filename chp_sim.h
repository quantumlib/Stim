#ifndef CHP_SIM_H
#define CHP_SIM_H

#include <iostream>
#include <immintrin.h>
#include <new>
#include <cassert>
#include <sstream>
#include <functional>
#include "tableau.h"
#include <random>

struct ChpSim {
    Tableau inv_state;
    std::mt19937 rng;

    explicit ChpSim(size_t num_qubits);

    bool is_deterministic(size_t target) const;
    bool measure(size_t q, float bias = 0.5);
    void hadamard(size_t q);
    void phase(size_t q);
    void cnot(size_t c, size_t t);
};

#endif
