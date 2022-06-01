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

#include "stim/mem/simd_bits_range_ref.h"

#include "gtest/gtest.h"

#include "stim/test_util.test.h"

using namespace stim;

TEST(simd_bits_range_ref, construct) {
    alignas(64) std::array<uint64_t, 16> data{};
    simd_bits_range_ref ref((simd_word *)data.data(), sizeof(data) / sizeof(simd_word));

    ASSERT_EQ(ref.ptr_simd, (simd_word *)&data[0]);
    ASSERT_EQ(ref.num_simd_words, 16 * sizeof(uint64_t) / sizeof(simd_word));
    ASSERT_EQ(ref.num_bits_padded(), 1024);
    ASSERT_EQ(ref.num_u8_padded(), 128);
    ASSERT_EQ(ref.num_u16_padded(), 64);
    ASSERT_EQ(ref.num_u32_padded(), 32);
    ASSERT_EQ(ref.num_u64_padded(), 16);
}

TEST(simd_bits_range_ref, aliased_editing_and_bit_refs) {
    alignas(64) std::array<uint64_t, 16> data{};
    auto c = (char *)&data;
    simd_bits_range_ref ref((simd_word *)data.data(), sizeof(data) / sizeof(simd_word));
    const simd_bits_range_ref cref((simd_word *)data.data(), sizeof(data) / sizeof(simd_word));

    ASSERT_EQ(c[0], 0);
    ASSERT_EQ(c[13], 0);
    ref[5] = true;
    ASSERT_EQ(c[0], 32);
    ref[0] = true;
    ASSERT_EQ(c[0], 33);
    ref[100] = true;
    ASSERT_EQ(ref[100], true);
    ASSERT_EQ(c[12], 16);
    c[12] = 0;
    ASSERT_EQ(ref[100], false);
    ASSERT_EQ(cref[100], ref[100]);
}

TEST(simd_bits_range_ref, str) {
    alignas(64) std::array<uint64_t, 8> data{};
    simd_bits_range_ref ref((simd_word *)data.data(), sizeof(data) / sizeof(simd_word));
    ASSERT_EQ(
        ref.str(),
        "________________________________________________________________"
        "________________________________________________________________"
        "________________________________________________________________"
        "________________________________________________________________"
        "________________________________________________________________"
        "________________________________________________________________"
        "________________________________________________________________"
        "________________________________________________________________");
    ref[5] = 1;
    ASSERT_EQ(
        ref.str(),
        "_____1__________________________________________________________"
        "________________________________________________________________"
        "________________________________________________________________"
        "________________________________________________________________"
        "________________________________________________________________"
        "________________________________________________________________"
        "________________________________________________________________"
        "________________________________________________________________");
}

TEST(simd_bits_range_ref, randomize) {
    alignas(64) std::array<uint64_t, 16> data{};
    simd_bits_range_ref ref((simd_word *)data.data(), sizeof(data) / sizeof(simd_word));

    ref.randomize(64 + 57, SHARED_TEST_RNG());
    uint64_t mask = (1ULL << 57) - 1;
    // Randomized.
    ASSERT_NE(ref.u64[0], 0);
    ASSERT_NE(ref.u64[0], SIZE_MAX);
    ASSERT_NE(ref.u64[1] & mask, 0);
    ASSERT_NE(ref.u64[1] & mask, 0);
    // Not touched.
    ASSERT_EQ(ref.u64[1] & ~mask, 0);
    ASSERT_EQ(ref.u64[2], 0);
    ASSERT_EQ(ref.u64[3], 0);

    for (size_t k = 0; k < ref.num_u64_padded(); k++) {
        ref.u64[k] = UINT64_MAX;
    }
    ref.randomize(64 + 57, SHARED_TEST_RNG());
    // Randomized.
    ASSERT_NE(ref.u64[0], 0);
    ASSERT_NE(ref.u64[0], SIZE_MAX);
    ASSERT_NE(ref.u64[1] & mask, 0);
    ASSERT_NE(ref.u64[1] & mask, 0);
    // Not touched.
    ASSERT_EQ(ref.u64[1] & ~mask, UINT64_MAX & ~mask);
    ASSERT_EQ(ref.u64[2], UINT64_MAX);
    ASSERT_EQ(ref.u64[3], UINT64_MAX);
}

TEST(simd_bits_range_ref, xor_assignment) {
    alignas(64) std::array<uint64_t, 24> data{};
    simd_bits_range_ref m0((simd_word *)&data[0], sizeof(data) / sizeof(simd_word) / 3);
    simd_bits_range_ref m1((simd_word *)&data[8], sizeof(data) / sizeof(simd_word) / 3);
    simd_bits_range_ref m2((simd_word *)&data[16], sizeof(data) / sizeof(simd_word) / 3);
    m0.randomize(512, SHARED_TEST_RNG());
    m1.randomize(512, SHARED_TEST_RNG());
    ASSERT_NE(m0, m1);
    ASSERT_NE(m0, m2);
    m2 ^= m0;
    ASSERT_EQ(m0, m2);
    m2 ^= m1;
    for (size_t k = 0; k < m0.num_u64_padded(); k++) {
        ASSERT_EQ(m2.u64[k], m0.u64[k] ^ m1.u64[k]);
    }
}

TEST(simd_bits_range_ref, assignment) {
    alignas(64) std::array<uint64_t, 16> data{};
    simd_bits_range_ref m0((simd_word *)&data[0], sizeof(data) / sizeof(simd_word) / 2);
    simd_bits_range_ref m1((simd_word *)&data[8], sizeof(data) / sizeof(simd_word) / 2);
    m0.randomize(512, SHARED_TEST_RNG());
    m1.randomize(512, SHARED_TEST_RNG());
    auto old_m1 = m1.u64[0];
    ASSERT_NE(m0, m1);
    m0 = m1;
    ASSERT_EQ(m0, m1);
    ASSERT_EQ(m0.u64[0], old_m1);
    ASSERT_EQ(m1.u64[0], old_m1);
}

TEST(simd_bits_range_ref, equality) {
    alignas(64) std::array<uint64_t, 32> data{};
    simd_bits_range_ref m0((simd_word *)&data[0], sizeof(data) / sizeof(simd_word) / 4);
    simd_bits_range_ref m1((simd_word *)&data[8], sizeof(data) / sizeof(simd_word) / 4);
    simd_bits_range_ref m4((simd_word *)&data[16], sizeof(data) / sizeof(simd_word) / 2);

    ASSERT_TRUE(m0 == m1);
    ASSERT_FALSE(m0 != m1);
    ASSERT_FALSE(m0 == m4);
    ASSERT_TRUE(m0 != m4);

    m1[505] = 1;
    ASSERT_FALSE(m0 == m1);
    ASSERT_TRUE(m0 != m1);
    m0[505] = 1;
    ASSERT_TRUE(m0 == m1);
    ASSERT_FALSE(m0 != m1);
}

TEST(simd_bits_range_ref, swap_with) {
    alignas(64) std::array<uint64_t, 32> data{};
    simd_bits_range_ref m0((simd_word *)&data[0], sizeof(data) / sizeof(simd_word) / 4);
    simd_bits_range_ref m1((simd_word *)&data[8], sizeof(data) / sizeof(simd_word) / 4);
    simd_bits_range_ref m2((simd_word *)&data[16], sizeof(data) / sizeof(simd_word) / 4);
    simd_bits_range_ref m3((simd_word *)&data[24], sizeof(data) / sizeof(simd_word) / 4);
    m0.randomize(512, SHARED_TEST_RNG());
    m1.randomize(512, SHARED_TEST_RNG());
    m2 = m0;
    m3 = m1;
    ASSERT_EQ(m0, m2);
    ASSERT_EQ(m1, m3);
    m0.swap_with(m1);
    ASSERT_EQ(m0, m3);
    ASSERT_EQ(m1, m2);
}

TEST(simd_bits_range_ref, clear) {
    alignas(64) std::array<uint64_t, 8> data{};
    simd_bits_range_ref m0((simd_word *)&data[0], sizeof(data) / sizeof(simd_word));
    m0.randomize(512, SHARED_TEST_RNG());
    ASSERT_TRUE(m0.not_zero());
    m0.clear();
    ASSERT_TRUE(!m0.not_zero());
}

TEST(simd_bits_range_ref, not_zero256) {
    alignas(64) std::array<uint64_t, 8> data{};
    simd_bits_range_ref m0((simd_word *)&data[0], sizeof(data) / sizeof(simd_word));
    ASSERT_FALSE(m0.not_zero());
    m0[5] = true;
    ASSERT_TRUE(m0.not_zero());
    m0[511] = true;
    ASSERT_TRUE(m0.not_zero());
    m0[5] = false;
    ASSERT_TRUE(m0.not_zero());
}

TEST(simd_bits_range_ref, word_range_ref) {
    simd_word d[sizeof(uint64_t) * 16 / sizeof(simd_word)]{};
    simd_bits_range_ref ref(d, sizeof(d) / sizeof(simd_word));
    const simd_bits_range_ref cref(d, sizeof(d) / sizeof(simd_word));
    auto r1 = ref.word_range_ref(1, 2);
    auto r2 = ref.word_range_ref(2, 2);
    r1[1] = true;
    ASSERT_TRUE(!r2.not_zero());
    auto k = sizeof(simd_word) * 8 + 1;
    ASSERT_EQ(r1[k], false);
    r2[1] = true;
    ASSERT_EQ(r1[k], true);
    ASSERT_EQ(cref.word_range_ref(1, 2)[k], true);
}

TEST(simd_bits_range_ref, for_each_set_bit) {
    simd_bits data(256);
    simd_bits_range_ref ref(data);
    ref[5] = true;
    ref[101] = true;
    std::vector<size_t> hits;
    ref.for_each_set_bit([&](size_t k) {
        hits.push_back(k);
    });
    ASSERT_EQ(hits, (std::vector<size_t>{5, 101}));
}

TEST(simd_bits_range_ref, truncated_overwrite_from) {
    simd_bits dat = simd_bits::random(1024, SHARED_TEST_RNG());
    simd_bits mut = simd_bits::random(1024, SHARED_TEST_RNG());
    simd_bits old = mut;

    simd_bits_range_ref(mut).truncated_overwrite_from(dat, 455);

    for (size_t k = 0; k < 1024; k++) {
        ASSERT_EQ(mut[k], k < 455 ? dat[k] : old[k]) << k;
    }
}

TEST(simd_bits_range_ref, popcnt) {
    simd_bits data(1024);
    simd_bits_range_ref ref(data);
    ASSERT_EQ(ref.popcnt(), 0);
    data[101] = 1;
    ASSERT_EQ(ref.popcnt(), 1);
    data[0] = 1;
    ASSERT_EQ(ref.popcnt(), 2);
    data.u64[8] = 0xFFFFFFFFFFFFFFFFULL;
    ASSERT_EQ(ref.popcnt(), 66);
}

TEST(simd_bits_range_ref, intersects) {
    simd_bits data(1024);
    simd_bits other(512);
    simd_bits_range_ref ref(data);
    ASSERT_EQ(data.intersects(other), false);
    ASSERT_EQ(ref.intersects(other), false);
    other[511] = true;
    ASSERT_EQ(data.intersects(other), false);
    ASSERT_EQ(ref.intersects(other), false);
    data[513] = true;
    ASSERT_EQ(data.intersects(other), false);
    ASSERT_EQ(ref.intersects(other), false);
    data[511] = true;
    ASSERT_EQ(data.intersects(other), true);
    ASSERT_EQ(ref.intersects(other), true);
    data[101] = true;
    ASSERT_EQ(data.intersects(other), true);
    ASSERT_EQ(ref.intersects(other), true);
    other[101] = true;
    ASSERT_EQ(data.intersects(other), true);
    ASSERT_EQ(ref.intersects(other), true);
}

TEST(simd_bits_range_ref, prefix_ref) {
    simd_bits data(1024);
    simd_bits_range_ref ref(data);
    auto prefix = ref.prefix_ref(257);
    ASSERT_TRUE(prefix.num_bits_padded() >= 257);
    ASSERT_TRUE(prefix.num_bits_padded() < 1024);
    ASSERT_FALSE(data[0]);
    prefix[0] = true;
    ASSERT_TRUE(data[0]);
}
