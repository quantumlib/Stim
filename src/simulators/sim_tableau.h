#ifndef SIM_TABLEAU_H
#define SIM_TABLEAU_H

#include <iostream>
#include <immintrin.h>
#include <new>
#include <cassert>
#include <sstream>
#include <functional>
#include "../stabilizers/tableau.h"
#include <random>
#include "../circuit.h"
#include "sim_vector.h"
#include "../stabilizers/tableau_transposed_raii.h"

struct SimTableau {
    Tableau inv_state;
    std::mt19937_64 &rng;

    explicit SimTableau(size_t num_qubits, std::mt19937_64 &rng);
    void ensure_large_enough_for_qubit(size_t q);

    SimVector to_vector_sim() const;
    bool is_deterministic(size_t target) const;
    std::vector<bool> measure(const std::vector<size_t> &targets, float bias = 0.5);
    void reset(const std::vector<size_t> &targets);
    static std::vector<bool> simulate(const Circuit &circuit, std::mt19937_64 &rng);
    static void simulate(FILE *in, FILE *out, bool newline_after_ticks, std::mt19937_64 &rng);

    void H(const std::vector<size_t> &targets);
    void H_YZ(const std::vector<size_t> &targets);
    void H_XY(const std::vector<size_t> &targets);
    void SQRT_X(const std::vector<size_t> &targets);
    void SQRT_Y(const std::vector<size_t> &targets);
    void SQRT_Z(const std::vector<size_t> &targets);
    void SQRT_X_DAG(const std::vector<size_t> &targets);
    void SQRT_Y_DAG(const std::vector<size_t> &targets);
    void SQRT_Z_DAG(const std::vector<size_t> &targets);
    void CX(const std::vector<size_t> &targets);
    void CY(const std::vector<size_t> &targets);
    void CZ(const std::vector<size_t> &targets);
    void SWAP(const std::vector<size_t> &targets);
    void X(const std::vector<size_t> &targets);
    void Y(const std::vector<size_t> &targets);
    void Z(const std::vector<size_t> &targets);
    void ISWAP(const std::vector<size_t> &targets);
    void ISWAP_DAG(const std::vector<size_t> &targets);
    void XCX(const std::vector<size_t> &targets);
    void XCY(const std::vector<size_t> &targets);
    void XCZ(const std::vector<size_t> &targets);
    void YCX(const std::vector<size_t> &targets);
    void YCY(const std::vector<size_t> &targets);
    void YCZ(const std::vector<size_t> &targets);

    void tableau_op(const std::string &name, const std::vector<size_t> &targets);
    void broadcast_op(const std::string &name, const std::vector<size_t> &targets);

    std::vector<SparsePauliString> inspected_collapse(
            const std::vector<size_t> &targets);

private:
    void collapse_many(const std::vector<size_t> &targets, float bias);
    void collapse_while_transposed(
            size_t target,
            TableauTransposedRaii &temp_transposed,
            SparsePauliString *destabilizer_out,
            float else_bias);
};

extern const std::unordered_map<std::string, std::function<void(SimTableau &, const std::vector<size_t> &)>> SIM_TABLEAU_GATE_FUNC_DATA;

#endif
