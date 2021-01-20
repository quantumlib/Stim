#include "gtest/gtest.h"
#include "probability_util.h"

TEST(probability_util, qubit_order) {
    std::mt19937 rng((std::random_device {})());
    size_t b = 10000;
    size_t s = 100000;
    double p = 0.001;
    std::vector<size_t> buckets(b, 0);
    for (size_t k = 0; k < s; k++) {
        for (auto bucket : sample_hit_indices(p, b, rng)) {
            buckets[bucket] += 1;
        }
    }
    size_t total = 0;
    for (auto b : buckets) {
        total += b;
    }
    ASSERT_TRUE(abs(total / (double)(b * s) - p) <= 0.0001);
    for (auto b : buckets) {
        ASSERT_TRUE(abs(b / (double)s - p) <= 0.01);
    }
}
