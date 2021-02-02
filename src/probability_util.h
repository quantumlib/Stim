#ifndef PROBABILITY_UTIL_H
#define PROBABILITY_UTIL_H

#include <random>
#include <stdexcept>
#include <vector>

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
    inline static void for_samples(double p, const std::vector<T> &vals, std::mt19937_64 &rng, BODY body) {
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

#endif
