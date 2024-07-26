/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _STIM_STABILIZERS_PAULI_STRING_ITER_H
#define _STIM_STABILIZERS_PAULI_STRING_ITER_H

#include "stim/mem/fixed_cap_vector.h"
#include "stim/mem/span_ref.h"
#include "stim/stabilizers/tableau.h"

namespace stim {

/// Tracks the state of a loop.
struct NestedLooperLoop {
    /// The first index that should be iterated.
    uint64_t start;
    /// One past the last index that should be iterated.
    uint64_t end;
    /// If this is set to the index of another loop, the starting offset is shifted by that other loop's value.
    /// Set to UINT64_MAX to not use.
    /// This is used in 'append_combination_loops' to avoid repeating combinations.
    uint64_t offset_from_other = UINT64_MAX;
    /// The current value of the iteration variable for this loop.
    /// UINT64_MAX means 'loop is not started yet'.
    uint64_t cur = UINT64_MAX;
};

/// A helper class for managing dynamically nested loops.
struct NestedLooper {
    std::vector<NestedLooperLoop> loops;
    uint64_t k = 0;

    /// Adds a series of nested loops for iterating combinations of w values from [0, n).
    inline void append_combination_loops(uint64_t n, uint64_t w) {
        if (w > 0) {
            loops.push_back(NestedLooperLoop{0, n - w + 1});
            for (uint64_t j = 1; j < w; j++) {
                auto v = loops.size() - 1;
                loops.push_back(NestedLooperLoop{1, n - w + j + 1, v});
            }
        }
    }

    /// Clears all loop variables and sets the loop index to the outermost loop.
    inline void start() {
        k = 0;
        for (auto &loop : loops) {
            loop.cur = UINT64_MAX;
        }
    }

    inline bool iter_next(const std::function<void(size_t)> &on_iter) {
        if (loops.empty()) {
            return false;
        }

        // k is the index of the loop to advance.
        // In the first step, k will be 0.
        // In later step, k is loops.size().
        if (k == loops.size()) {
            // Drop k by 1 to advance the innermost loop.
            k--;
        }

        while (true) {
            // Start or advance the current loop.
            if (loops[k].cur == UINT64_MAX) {
                loops[k].cur = loops[k].start;
                if (loops[k].offset_from_other != UINT64_MAX) {
                    loops[k].cur += loops[loops[k].offset_from_other].cur;
                }
            } else {
                loops[k].cur++;
            }

            // Notify the caller so they can dynamically add inner loops if needed.
            on_iter(k);

            // Check if the current loop has ended.
            if (loops[k].cur >= loops[k].end) {
                if (k == 0) {
                    // The outermost loop ended.
                    return false;
                }
                loops[k].cur = UINT64_MAX;
                k -= 1;
                continue;
            }

            // Move down to the next loop.
            k++;
            if (k == loops.size()) {
                // We're inside the innermost loop.
                return true;
            }
        }
    }
};

/// Iterates over pauli strings matching specified parameters.
///
/// The template parameter, W, represents the SIMD width.
template <size_t W>
struct PauliStringIterator {
    // Parameter storage.
    size_t num_qubits;  /// Number of qubits in results.
    size_t min_weight;  /// Minimum number of non-identity terms in results.
    size_t max_weight;  /// Maximum number of non-identity terms in results.
    bool allow_x;       /// Whether results are permitted to contain 'X' terms.
    bool allow_y;       /// Whether results are permitted to contain 'Y' terms.
    bool allow_z;       /// Whether results are permitted to contain 'Z' terms.

    // Progress storage.
    NestedLooper looper;    /// Tracks nested loops over target weight, the target qubits, and the target paulis.
    PauliString<W> result;  /// When iter_next() returns true, the result is stored in this field.

    PauliStringIterator(
        size_t num_qubits, size_t min_weight, size_t max_weight, bool allow_x, bool allow_y, bool allow_z);

    /// Updates the `result` field to point at the next yielded pauli string.
    /// Returns true if this succeeded, or false if iteration has ended.
    bool iter_next();

    // Restarts iteration.
    void restart();
};

}  // namespace stim

#include "stim/stabilizers/pauli_string_iter.inl"

#endif
