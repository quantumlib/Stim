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
    void H(size_t q);
    void S(size_t q);
    void CNOT(size_t c, size_t t);
    void CZ(size_t c, size_t t);
    void X(size_t q);
    void Y(size_t q);
    void Z(size_t q);
};

#endif
