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

#include "gtest/gtest.h"

#include "stim/test_util.test.h"

using namespace stim;

TEST(probability_util, sample_hit_indices_corner_cases) {
    ASSERT_EQ(sample_hit_indices(0, 100000, SHARED_TEST_RNG()), (std::vector<size_t>{}));
    ASSERT_EQ(sample_hit_indices(1, 10, SHARED_TEST_RNG()), (std::vector<size_t>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
}

TEST(probability_util, sample_hit_indices) {
    size_t num_buckets = 10000;
    size_t num_samples = 100000;
    double p = 0.001;
    std::vector<size_t> buckets(num_buckets, 0);
    for (size_t k = 0; k < num_samples; k++) {
        for (auto bucket : sample_hit_indices(p, num_buckets, SHARED_TEST_RNG())) {
            buckets[bucket] += 1;
        }
    }
    size_t total = 0;
    for (auto b : buckets) {
        total += b;
    }
    ASSERT_TRUE(abs(total / (double)(num_buckets * num_samples) - p) <= 0.0001);
    for (auto b : buckets) {
        ASSERT_TRUE(abs(b / (double)num_samples - p) <= 0.01);
    }
}

TEST(probability_util, biased_random) {
    std::vector<float> probs{0, 0.01, 0.03, 0.1, 0.4, 0.49, 0.5, 0.6, 0.9, 0.99, 0.999, 1};
    simd_bits<MAX_BITWORD_WIDTH> data(1000000);
    size_t n = data.num_bits_padded();
    for (auto p : probs) {
        biased_randomize_bits(p, data.u64, data.u64 + data.num_u64_padded(), SHARED_TEST_RNG());
        size_t t = 0;
        for (size_t k = 0; k < data.num_u64_padded(); k++) {
            t += popcnt64(data.u64[k]);
        }
        float dev = sqrtf(p * (1 - p) * n);
        float min_expected = n * p - dev * 5;
        float max_expected = n * p + dev * 5;
        ASSERT_TRUE(min_expected >= 0 && max_expected <= n) << min_expected << ", " << max_expected;
        EXPECT_TRUE(min_expected <= t && t <= max_expected)
            << min_expected / n << " < " << t / (float)n << " < " << max_expected / n << " for p=" << p;
    }
}