#include "probability_util.h"

#include <gtest/gtest.h>

#include "test_util.test.h"

TEST(probability_util, sample_hit_indices_corner_cases) {
    ASSERT_EQ(sample_hit_indices(0, 100000, SHARED_TEST_RNG()), (std::vector<size_t>{}));
    ASSERT_EQ(sample_hit_indices(1, 10, SHARED_TEST_RNG()), (std::vector<size_t>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
}

TEST(probability_util, sample_hit_indices) {
    size_t b = 10000;
    size_t s = 100000;
    double p = 0.001;
    std::vector<size_t> buckets(b, 0);
    for (size_t k = 0; k < s; k++) {
        for (auto bucket : sample_hit_indices(p, b, SHARED_TEST_RNG())) {
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
