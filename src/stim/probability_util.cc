// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stim/probability_util.h"

using namespace stim;

RareErrorIterator::RareErrorIterator(float probability)
    : next_candidate(0), is_one(probability == 1), dist(probability) {
    if (!(probability >= 0 && probability <= 1)) {
        throw std::out_of_range("Invalid probability: " + std::to_string(probability));
    }
}

size_t RareErrorIterator::next(std::mt19937_64 &rng) {
    size_t result = next_candidate + (is_one ? 0 : dist(rng));
    next_candidate = result + 1;
    return result;
}

std::vector<size_t> stim::sample_hit_indices(float probability, size_t attempts, std::mt19937_64 &rng) {
    std::vector<size_t> result;
    RareErrorIterator::for_samples(probability, attempts, rng, [&](size_t s) {
        result.push_back(s);
    });
    return result;
}

std::mt19937_64 stim::externally_seeded_rng() {
#if defined(__linux) && defined(__GLIBCXX__) && __GLIBCXX__ >= 20200128
    // Workaround for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94087
    // See https://github.com/quantumlib/Stim/issues/26
    std::random_device d("/dev/urandom");
#else
    std::random_device d;
#endif
    std::seed_seq seq{d(), d(), d(), d(), d(), d(), d(), d()};
    std::mt19937_64 result(seq);
    return result;
}

void stim::biased_randomize_bits(float probability, uint64_t *start, uint64_t *end, std::mt19937_64 &rng) {
    if (probability > 0.5) {
        // Recurse and invert for probabilities larger than 0.5.
        biased_randomize_bits(1 - probability, start, end, rng);
        while (start != end) {
            *start ^= UINT64_MAX;
            start++;
        }
    } else if (probability == 0.5) {
        // For the 50/50 case, just copy the bits directly into the buffer.
        while (start != end) {
            *start = rng();
            start++;
        }
    } else if (probability < 0.02) {
        // For small probabilities, sample gaps using a geometric distribution.
        size_t n = (end - start) << 6;
        memset(start, 0, n >> 3);
        RareErrorIterator::for_samples(probability, n, rng, [&](size_t s) {
            start[s >> 6] |= uint64_t{1} << (s & 63);
        });
    } else {
        // To generate a bit with probability p < 1, you can flip a coin until (after k flips) you
        // get heads, and then return the k'th fractional bit in the binary representation of p.
        //
        // Alternatively, you can flip a coin at most N times until you either fail to get any heads
        // or you get your first heads after k flips. If you got a heads, return the k'th fractional
        // bit in the binary representation of p. Account for the leftover probability (from the
        // bits that couldn't be reached due to clamping the coin flip count) by OR'ing with a low
        // entropy bit that's been generated with just the right probability to fix the difference.
        constexpr size_t COIN_FLIPS = 8;
        constexpr float BUCKETS = (float)(1 << COIN_FLIPS);
        float raised = probability * BUCKETS;
        float raised_floor = floorf(raised);
        float raised_leftover = raised - raised_floor;
        float p_truncated = raised_floor / BUCKETS;
        float p_leftover = raised_leftover / BUCKETS;
        uint64_t p_top_bits = (uint64_t)raised_floor;

        // Flip coins, using the position of the first HEADS result to
        // select a bit from the probability's binary representation.
        for (uint64_t *cur = start; cur != end; cur++) {
            uint64_t alive = rng();
            uint64_t result = 0;
            for (size_t k_bit = COIN_FLIPS - 1; k_bit--;) {
                uint64_t shoot = rng();
                result ^= shoot & alive & -((p_top_bits >> k_bit) & 1);
                alive &= ~shoot;
            }
            *cur = result;
        }

        // Correct distortion from truncation.
        size_t n = (end - start) << 6;
        RareErrorIterator::for_samples(p_leftover / (1 - p_truncated), n, rng, [&](size_t s) {
            start[s >> 6] |= uint64_t{1} << (s & 63);
        });
    }
}
