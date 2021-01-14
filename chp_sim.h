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
#include "circuit.h"
#include "vector_sim.h"

struct ChpSim {
    Tableau inv_state;
    std::mt19937 rng;

    explicit ChpSim(size_t num_qubits);
    void ensure_large_enough_for_qubit(size_t q);

    VectorSim to_vector_sim() const;
    bool is_deterministic(size_t target) const;
    bool measure(size_t q, float bias = 0.5);
    void reset(size_t target);
    std::vector<bool> measure_many(const std::vector<size_t> &targets, float bias = 0.5);
    void reset_many(const std::vector<size_t> &targets);
    static std::vector<bool> simulate(const Circuit &circuit);
    static void simulate(FILE *in, FILE *out);

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
    void SWAP(size_t q1, size_t q2);
    void X(size_t q);
    void Y(size_t q);
    void Z(size_t q);

    void op(const std::string &name, const std::vector<size_t> &targets);

private:
    template <typename cache_word_t>
    void collapse_many(const std::vector<size_t> &targets, float bias) {
        std::vector<size_t> collapse_targets;
        collapse_targets.reserve(targets.size());
        for (auto target : targets) {
            if (!is_deterministic(target)) {
                collapse_targets.push_back(target);
            }
        }
        if (collapse_targets.empty()) {
            return;
        }
        std::sort(collapse_targets.begin(), collapse_targets.end());

        auto n = inv_state.num_qubits;
        TempBlockTransposedTableauRaii temp_transposed(inv_state);

        // Perform partial Gaussian elimination.
        constexpr size_t cache_word_bits = sizeof(cache_word_t) * 8;
        constexpr size_t cache_word_shift = (size_t)log2(cache_word_bits + 0.5);
        constexpr size_t cache_mask_low = cache_word_bits - 1;
        constexpr size_t cache_mask_high = ~cache_mask_low;
        aligned_bits256 cache_data_x(n << cache_word_shift);
        aligned_bits256 cache_data_z(n << cache_word_shift);
        auto cache_x = (cache_word_t *)cache_data_x.u64;
        auto cache_z = (cache_word_t *)cache_data_z.u64;
        auto uncached = (cache_word_t *)temp_transposed.tableau.data_x2x_z2x_x2z_z2z.u64;
        size_t loaded = SIZE_MAX;
        for (auto q1 : collapse_targets) {
            if (loaded != (q1 >> cache_word_shift)) {
                for (size_t q2 = 0; q2 < n; q2++) {
                    auto bx = bit_address(q1 & cache_mask_high, q2, n, 1, true);
                    auto bz = bit_address(q1 & cache_mask_high, q2, n, 3, true);
                    cache_x[q2] = uncached[bx >> cache_word_shift];
                    cache_z[q2] = uncached[bz >> cache_word_shift];
                }
                loaded = q1 >> cache_word_shift;
            }
            auto q1_mask = (cache_word_t)1 << (q1 & cache_mask_low);
            size_t pivot = SIZE_MAX;
            for (size_t q2 = 0; q2 < n; q2++) {
                if (cache_x[q2] & q1_mask) {
                    pivot = q2;
                    break;
                }
            }
            if (pivot != SIZE_MAX) {
                for (size_t victim = pivot + 1; victim < n; victim++) {
                    if (cache_x[victim] & q1_mask) {
                        temp_transposed.append_CX(pivot, victim);
                        cache_x[victim] ^= cache_x[pivot];
                        cache_z[pivot] ^= cache_z[victim];
                    }
                }
                if (cache_z[pivot] & q1_mask) {
                    temp_transposed.append_H_YZ(pivot);
                    cache_z[pivot] ^= cache_x[pivot];
                } else {
                    temp_transposed.append_H(pivot);
                    std::swap(cache_z[pivot], cache_x[pivot]);
                }

                auto coin_flip = std::bernoulli_distribution(bias)(rng);
                if (temp_transposed.z_sign(q1) != coin_flip) {
                    temp_transposed.append_X(pivot);
                }
            }
        }
    }
};

extern const std::unordered_map<std::string, std::function<void(ChpSim &, size_t)>> SINGLE_QUBIT_GATE_FUNCS;
extern const std::unordered_map<std::string, std::function<void(ChpSim &, size_t, size_t)>> TWO_QUBIT_GATE_FUNCS;

#endif
