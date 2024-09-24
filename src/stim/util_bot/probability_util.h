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

#ifndef _STIM_UTIL_BOT_PROBABILITY_UTIL_H
#define _STIM_UTIL_BOT_PROBABILITY_UTIL_H

#include <random>
#include <stdexcept>
#include <vector>

#include "stim/mem/span_ref.h"

namespace stim {

// Change this number from time to time to ensure people don't rely on seeds across versions.
constexpr uint64_t INTENTIONAL_VERSION_SEED_INCOMPATIBILITY = 0xDEADBEEF124BULL;

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
    inline static void for_samples(double p, const SpanRef<const T> &vals, std::mt19937_64 &rng, BODY body) {
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

/// Create a fresh random number generator seeded by entropy from the operating system.
std::mt19937_64 externally_seeded_rng();

/// Create a random number generator either seeded by a --seed argument, or else by entropy from the operating system.
std::mt19937_64 optionally_seeded_rng(int argc, const char **argv);

/// Overwrite the given span with random data where bits are set with the given probability.
///
/// Args:
///     probability: The chance that each bit will be on.
///     start: Inclusive start of the memory span to overwrite.
///     end: Exclusive end of the memory span to overwrite.
///     rng: The random number generator to use to generate entropy.
void biased_randomize_bits(float probability, uint64_t *start, uint64_t *end, std::mt19937_64 &rng);

}  // namespace stim

#endif
