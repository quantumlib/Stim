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
    std::vector<bool> measure_many(const std::vector<size_t> &targets, float bias = 0.5);

    void H(size_t q);
    void H_YZ(size_t q);
    void H_XY(size_t q);
    void SQRT_X(size_t q);
    void SQRT_Y(size_t q);
    void SQRT_Z(size_t q);
    void SQRT_X_DAG(size_t q);
    void SQRT_Y_DAG(size_t q);
    void SQRT_Z_DAG(size_t q);
    void CX(size_t c, size_t t);
    void CY(size_t c, size_t t);
    void CZ(size_t c, size_t t);
    void X(size_t q);
    void Y(size_t q);
    void Z(size_t q);
    void op(const std::string &name, const std::vector<size_t> &targets);

private:
    bool measure_while_transposed(BlockTransposedTableau &transposed, size_t target, float bias);
};

#endif
