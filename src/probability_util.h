#ifndef PROBABILITY_UTIL_H
#define PROBABILITY_UTIL_H

#include <random>
#include <vector>

/// Yields the indices of hits sampled from a Bernoulli distribution.
/// Gets more efficient as the hit probability drops.
struct RareErrorIterator {
    size_t next_candidate;
    std::geometric_distribution<size_t> dist;
    RareErrorIterator(float probability);
    size_t next(std::mt19937_64 &rng);
};

std::vector<size_t> sample_hit_indices(float probability, size_t attempts, std::mt19937_64 &rng);

std::mt19937_64 externally_seeded_rng();

#endif
