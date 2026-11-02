// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");

#include "unimodular_bruhat_cholesky.h"

#include <array>
#include <cassert>
#include <chrono>
#include <cstdio>

int main() {
    constexpr size_t K = 8;
    constexpr size_t N = 1ULL << K;
    using Engine = stim::UnimodularBruhatCholeskyEngine<K>;

    // 1. Exhaustive 65,536-entry verification of W_8 = Z_8 * diag((-2)^{|m|}) * Z_8^T
    size_t mismatches = 0;
    for (size_t col = 0; col < N; ++col) {
        std::array<int64_t, N> e_col{};
        e_col[col] = 1;
        Engine::inplace_walsh_via_bruhat_cholesky(e_col.data());
        for (size_t row = 0; row < N; ++row) {
            const unsigned int pop = static_cast<unsigned int>(std::popcount(row & col));
            const int64_t expected = (pop & 1U) ? -1 : 1;
            if (e_col[row] != expected) {
                ++mismatches;
            }
        }
    }
    assert(mismatches == 0);

    // 2. Verify exact inversion mu_8(Z_8(v)) == v
    std::array<int64_t, N> orig{};
    for (size_t i = 0; i < N; ++i) {
        orig[i] = static_cast<int64_t>((i * 37ULL + 13ULL) % 101ULL) - 50;
    }
    std::array<int64_t, N> work = orig;
    Engine::inplace_unimodular_zeta(work.data());
    Engine::inplace_unimodular_mobius(work.data());
    for (size_t i = 0; i < N; ++i) {
        assert(work[i] == orig[i]);
    }

    // 3. Microbenchmark 100,000 iterations of inplace_unimodular_mobius on k=8 (256 slots)
    constexpr size_t ITERS = 100000;
    auto t0 = std::chrono::steady_clock::now();
    for (size_t it = 0; it < ITERS; ++it) {
        Engine::inplace_unimodular_mobius(work.data());
        work[0] += static_cast<int64_t>(it & 1U);
    }
    auto t1 = std::chrono::steady_clock::now();
    double ns_per_call = std::chrono::duration<double, std::nano>(t1 - t0).count() / static_cast<double>(ITERS);
    std::printf("[PASS] C++20 UnimodularBruhatCholeskyEngine<8>: 0 mismatches in 65,536 entries | %.1f ns/tile (sink=%lld)\n",
                ns_per_call, static_cast<long long>(work[N - 1]));
    return 0;
}
