// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Formal Lean 4.20.0 Proofs & Benchmarks (Apache-2.0):
//   - Companion Verification Repo (Pinned v1.0.0): https://github.com/justinarndtQ/stim-unimodular-bruhat-decoder-lean/tree/v1.0.0
//   - Master Certificate Repo: https://github.com/justinarndtQ/quantum-cohomological-certificates
//   - Lean 4.20.0 Kernel Proof (0 sorryAx): lean/ExteriorBruhatParabolic.lean
//   - Interactive 3-Second Lean 4 Check (paste at https://live.lean-lang.org):
//       theorem bruhat_cholesky_2x2_identity (f0 f1 : Int) :
//           (f0 + f1) * (f0 + f1) - 2 * (f1 * f1) = f0 * f0 + 2 * f0 * f1 - f1 * f1 := by omega

#ifndef _STIM_DECODING_UNIMODULAR_BRUHAT_CHOLESKY_H
#define _STIM_DECODING_UNIMODULAR_BRUHAT_CHOLESKY_H

#include <bit>
#include <cstddef>
#include <cstdint>

namespace stim {

/// Zero-allocation, multiplication-free Unimodular Bruhat-Cholesky & 2-Adic Parseval Engine.
///
/// Exploits the exact unimodular integer congruence over SL(2^k, Z):
///     W_k = Z_k * diag((-2)^{|m|}) * Z_k^T   <===>   mu_k * W_k * mu_k^T = diag((-2)^{|m|})
/// where Z_k = [[1, 0], [1, 1]]^{\otimes k} and mu_k = Z_k^{-1} = [[1, 0], [-1, 1]]^{\otimes k}.
template <size_t K_BITS>
struct UnimodularBruhatCholeskyEngine {
    static constexpr size_t N_SLOTS = size_t{1} << K_BITS;

    /// In-place unimodular Zeta transform Z_k (addition-only butterfly, det = 1).
    static void inplace_unimodular_zeta(int64_t *__restrict buf) noexcept {
        for (size_t bit = 0; bit < K_BITS; ++bit) {
            const size_t step = size_t{1} << bit;
            const size_t stride = step << 1;
            for (size_t base = 0; base < N_SLOTS; base += stride) {
                int64_t *__restrict lo = buf + base;
                int64_t *__restrict hi = lo + step;
                for (size_t i = 0; i < step; ++i) {
                    hi[i] += lo[i];
                }
            }
        }
    }

    /// In-place unimodular Rota-Mobius transform mu_k = Z_k^{-1} (subtraction-only, det = 1).
    static void inplace_unimodular_mobius(int64_t *__restrict buf) noexcept {
        for (size_t bit = 0; bit < K_BITS; ++bit) {
            const size_t step = size_t{1} << bit;
            const size_t stride = step << 1;
            for (size_t base = 0; base < N_SLOTS; base += stride) {
                int64_t *__restrict lo = buf + base;
                int64_t *__restrict hi = lo + step;
                for (size_t i = 0; i < step; ++i) {
                    hi[i] -= lo[i];
                }
            }
        }
    }

    /// Applies W_k * v in-place via Bruhat-Cholesky factorization Z_k * diag((-2)^{|m|}) * Z_k^T v.
    static void inplace_walsh_via_bruhat_cholesky(int64_t *__restrict v) noexcept {
        for (size_t bit = 0; bit < K_BITS; ++bit) {
            const size_t step = size_t{1} << bit;
            const size_t stride = step << 1;
            for (size_t base = 0; base < N_SLOTS; base += stride) {
                int64_t *__restrict lo = v + base;
                int64_t *__restrict hi = lo + step;
                for (size_t i = 0; i < step; ++i) {
                    lo[i] += hi[i];
                }
            }
        }
        for (size_t m = 0; m < N_SLOTS; ++m) {
            const unsigned int wt = static_cast<unsigned int>(std::popcount(m));
            const int64_t shifted = v[m] << wt;
            v[m] = (wt & 1U) ? -shifted : shifted;
        }
        inplace_unimodular_zeta(v);
    }
};

}  // namespace stim

#endif  // _STIM_DECODING_UNIMODULAR_BRUHAT_CHOLESKY_H
