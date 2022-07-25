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

#include "stim/simulators/dem_sampler.h"

#include "gtest/gtest.h"

#include "stim/test_util.test.h"

using namespace stim;

TEST(DemSampler, basic_sizing) {
    std::mt19937_64 irrelevant_rng(0);
    DemSampler sampler(DetectorErrorModel(R"DEM()DEM"), irrelevant_rng, 700);
    ASSERT_EQ(sampler.det_buffer.num_major_bits_padded(), 0);
    ASSERT_EQ(sampler.obs_buffer.num_major_bits_padded(), 0);
    ASSERT_GE(sampler.det_buffer.num_minor_bits_padded(), 700);
    ASSERT_GE(sampler.obs_buffer.num_minor_bits_padded(), 700);
    sampler.resample(false);
    ASSERT_FALSE(sampler.obs_buffer.data.not_zero());
    ASSERT_FALSE(sampler.det_buffer.data.not_zero());

    sampler = DemSampler(
        DetectorErrorModel(R"DEM(
            logical_observable L2000
            detector D1000
         )DEM"),
        irrelevant_rng,
        200);
    ASSERT_GE(sampler.det_buffer.num_major_bits_padded(), 1000);
    ASSERT_GE(sampler.obs_buffer.num_major_bits_padded(), 2000);
    ASSERT_GE(sampler.det_buffer.num_minor_bits_padded(), 200);
    ASSERT_GE(sampler.obs_buffer.num_minor_bits_padded(), 200);
    sampler.resample(false);
    ASSERT_FALSE(sampler.obs_buffer.data.not_zero());
    ASSERT_FALSE(sampler.det_buffer.data.not_zero());
}

TEST(DemSampler, resample_basic_probabilities) {
    DemSampler sampler(
        DetectorErrorModel(R"DEM(
            error(0) D0
            error(0.25) D1 L0
            error(0.5) D2
            error(0.75) D3
            error(1) D4 ^ D5
         )DEM"),
        SHARED_TEST_RNG(),
        1000);
    for (size_t k = 0; k < 2; k++) {
        sampler.resample(false);
        ASSERT_EQ(sampler.det_buffer[0].popcnt(), 0);
        ASSERT_GT(sampler.det_buffer[1].popcnt(), 0);
        ASSERT_LT(sampler.det_buffer[1].popcnt(), 500);
        ASSERT_GT(sampler.det_buffer[2].popcnt(), 250);
        ASSERT_LT(sampler.det_buffer[2].popcnt(), 750);
        ASSERT_GT(sampler.det_buffer[3].popcnt(), 500);
        ASSERT_LT(sampler.det_buffer[3].popcnt(), 1000);
        ASSERT_EQ(sampler.det_buffer[4].popcnt(), sampler.det_buffer[4].num_bits_padded());

        ASSERT_EQ(sampler.det_buffer[1], sampler.obs_buffer[0]);
        ASSERT_EQ(sampler.det_buffer[4], sampler.det_buffer[5]);
    }
}

TEST(DemSampler, resample_combinations) {
    DemSampler sampler(
        DetectorErrorModel(R"DEM(
            error(0.1) D0 D1
            error(0.2) D1 D2
            error(0.3) D2 D0
         )DEM"),
        SHARED_TEST_RNG(),
        1000);
    for (size_t k = 0; k < 2; k++) {
        sampler.resample(false);
        ASSERT_GT(sampler.det_buffer[0].popcnt(), 340 - 100);
        ASSERT_LT(sampler.det_buffer[0].popcnt(), 340 + 100);
        ASSERT_GT(sampler.det_buffer[1].popcnt(), 260 - 100);
        ASSERT_LT(sampler.det_buffer[1].popcnt(), 260 + 100);
        ASSERT_GT(sampler.det_buffer[2].popcnt(), 380 - 100);
        ASSERT_LT(sampler.det_buffer[2].popcnt(), 380 + 100);

        simd_bits total = sampler.det_buffer[0];
        total ^= sampler.det_buffer[1];
        total ^= sampler.det_buffer[2];
        ASSERT_FALSE(total.not_zero());
    }
}
