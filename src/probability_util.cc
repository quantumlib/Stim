#include "probability_util.h"

RareErrorIterator::RareErrorIterator(float probability) : next_candidate(0), dist(probability) {
}

size_t RareErrorIterator::next(std::mt19937_64 &rng) {
    size_t result = next_candidate + dist(rng);
    next_candidate = result + 1;
    return result;
}

std::vector<size_t> sample_hit_indices(float probability, size_t attempts, std::mt19937_64 &rng) {
    std::vector<size_t> result;
    RareErrorIterator skipper(probability);
    while (true) {
        size_t k = skipper.next(rng);
        if (k >= attempts) {
            break;
        }
        result.push_back(k);
    }
    return result;
}