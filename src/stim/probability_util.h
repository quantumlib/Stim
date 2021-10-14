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

#ifndef _STIM_PROBABILITY_UTIL_H
#define _STIM_PROBABILITY_UTIL_H

#include <random>
#include <stdexcept>
#include <vector>

#include "stim/circuit/circuit.h"

namespace stim {

/// Yields the indices of hits sampled from a Bernoulli distribution.
/// Gets more efficient as the hit probability drops.
struct RareErrorIterator {
    size_t next_candidate;
    bool is_one = false;
    std::geometric_distribution<size_t> dist;
    RareErrorIterator(float probability);
    size_t next(std::mt19937_64 &rng);

    template <typename BODY>
    inline static void for_samples(double p, size_t n, std::mt19937_64 &rng, BODY body) {
        if (p == 0) {
            return;
        }
        RareErrorIterator skipper((float)p);
        while (true) {
            size_t s = skipper.next(rng);
            if (s >= n) {
                break;
            }
            body(s);
        }
    }

    template <typename BODY, typename T>
    inline static void for_samples(double p, const ConstPointerRange<T> &vals, std::mt19937_64 &rng, BODY body) {
        if (p == 0) {
            return;
        }
        RareErrorIterator skipper((float)p);
        while (true) {
            size_t s = skipper.next(rng);
            if (s >= vals.size()) {
                break;
            }
            body(vals[s]);
        }
    }
};

std::vector<size_t> sample_hit_indices(float probability, size_t attempts, std::mt19937_64 &rng);

std::mt19937_64 externally_seeded_rng();

void biased_randomize_bits(float probability, uint64_t *start, uint64_t *end, std::mt19937_64 &rng);

}  // namespace stim

#endif
