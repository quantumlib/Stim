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

#include "stim/mem/simd_bits.h"

#include "gtest/gtest.h"

#include "stim/mem/simd_util.h"
#include "stim/test_util.test.h"

using namespace stim;

TEST(simd_bits, move) {
    simd_bits a(512);
    auto ptr = a.u64;

    simd_bits b(std::move(a));
    ASSERT_EQ(b.u64, ptr);

    simd_bits c(1);
    c = std::move(b);
    ASSERT_EQ(c.u64, ptr);
}

TEST(simd_bits, construct) {
    simd_bits d(1024);
    ASSERT_NE(d.ptr_simd, nullptr);
    ASSERT_EQ(d.num_simd_words, 1024 / 8 / sizeof(simd_word));
    ASSERT_EQ(d.num_bits_padded(), 1024);
    ASSERT_EQ(d.num_u8_padded(), 128);
    ASSERT_EQ(d.num_u16_padded(), 64);
    ASSERT_EQ(d.num_u32_padded(), 32);
    ASSERT_EQ(d.num_u64_padded(), 16);
}

TEST(simd_bits, aliased_editing_and_bit_refs) {
    simd_bits d(1024);
    auto c = (char *)d.u8;
    const simd_bits &cref = d;

    ASSERT_EQ(c[0], 0);
    ASSERT_EQ(c[13], 0);
    d[5] = true;
    ASSERT_EQ(c[0], 32);
    d[0] = true;
    ASSERT_EQ(c[0], 33);
    d[100] = true;
    ASSERT_EQ(d[100], true);
    ASSERT_EQ(c[12], 16);
    c[12] = 0;
    ASSERT_EQ(d[100], false);
    ASSERT_EQ(cref[100], d[100]);
}

TEST(simd_util, min_bits_to_num_bits_padded) {
    const auto &f = &min_bits_to_num_bits_padded;
    if (sizeof(simd_word) == 256 / 8) {
        ASSERT_EQ(f(0), 0);
        ASSERT_EQ(f(1), 256);
        ASSERT_EQ(f(100), 256);
        ASSERT_EQ(f(255), 256);
        ASSERT_EQ(f(256), 256);
        ASSERT_EQ(f(257), 512);
        ASSERT_EQ(f((1 << 30) - 1), 1 << 30);
        ASSERT_EQ(f(1 << 30), 1 << 30);
        ASSERT_EQ(f((1 << 30) + 1), (1 << 30) + 256);
    } else if (sizeof(simd_word) == 128 / 8) {
        ASSERT_EQ(f(0), 0);
        ASSERT_EQ(f(1), 128);
        ASSERT_EQ(f(100), 128);
        ASSERT_EQ(f(255), 256);
        ASSERT_EQ(f(256), 256);
        ASSERT_EQ(f(257), 256 + 128);
        ASSERT_EQ(f((1 << 30) - 1), 1 << 30);
        ASSERT_EQ(f(1 << 30), 1 << 30);
        ASSERT_EQ(f((1 << 30) + 1), (1 << 30) + 128);
    } else {
        ASSERT_TRUE(false) << "Unrecognized size.";
    }
}

TEST(simd_bits, str) {
    simd_bits d(256);
    ASSERT_EQ(
        d.str(),
        "________________________________________________________________"
        "________________________________________________________________"
        "________________________________________________________________"
        "________________________________________________________________");
    d[5] = true;
    ASSERT_EQ(
        d.str(),
        "_____1__________________________________________________________"
        "________________________________________________________________"
        "________________________________________________________________"
        "________________________________________________________________");
}

TEST(simd_bits, randomize) {
    simd_bits d(1024);

    d.randomize(64 + 57, SHARED_TEST_RNG());
    uint64_t mask = (1ULL << 57) - 1;
    // Randomized.
    ASSERT_NE(d.u64[0], 0);
    ASSERT_NE(d.u64[0], SIZE_MAX);
    ASSERT_NE(d.u64[1] & mask, 0);
    ASSERT_NE(d.u64[1] & mask, 0);
    // Not touched.
    ASSERT_EQ(d.u64[1] & ~mask, 0);
    ASSERT_EQ(d.u64[2], 0);
    ASSERT_EQ(d.u64[3], 0);

    for (size_t k = 0; k < d.num_u64_padded(); k++) {
        d.u64[k] = UINT64_MAX;
    }
    d.randomize(64 + 57, SHARED_TEST_RNG());
    // Randomized.
    ASSERT_NE(d.u64[0], 0);
    ASSERT_NE(d.u64[0], SIZE_MAX);
    ASSERT_NE(d.u64[1] & mask, 0);
    ASSERT_NE(d.u64[1] & mask, 0);
    // Not touched.
    ASSERT_EQ(d.u64[1] & ~mask, UINT64_MAX & ~mask);
    ASSERT_EQ(d.u64[2], UINT64_MAX);
    ASSERT_EQ(d.u64[3], UINT64_MAX);
}

TEST(simd_bits, xor_assignment) {
    simd_bits m0 = simd_bits::random(512, SHARED_TEST_RNG());
    simd_bits m1 = simd_bits::random(512, SHARED_TEST_RNG());
    simd_bits m2(512);
    m2 ^= m0;
    ASSERT_EQ(m0, m2);
    m2 ^= m1;
    for (size_t k = 0; k < m0.num_u64_padded(); k++) {
        ASSERT_EQ(m2.u64[k], m0.u64[k] ^ m1.u64[k]);
    }
}

TEST(simd_bits, assignment) {
    simd_bits m0(512);
    simd_bits m1(512);
    m0.randomize(512, SHARED_TEST_RNG());
    m1.randomize(512, SHARED_TEST_RNG());
    auto old_m1 = m1.u64[0];
    ASSERT_NE(m0, m1);
    m0 = m1;
    ASSERT_EQ(m0, m1);
    ASSERT_EQ(m0.u64[0], old_m1);
    ASSERT_EQ(m1.u64[0], old_m1);
}

TEST(simd_bits, equality) {
    simd_bits m0(512);
    simd_bits m1(512);
    simd_bits m4(1024);

    ASSERT_TRUE(m0 == m1);
    ASSERT_FALSE(m0 != m1);
    ASSERT_FALSE(m0 == m4);
    ASSERT_TRUE(m0 != m4);

    m1[505] = true;
    ASSERT_FALSE(m0 == m1);
    ASSERT_TRUE(m0 != m1);
    m0[505] = true;
    ASSERT_TRUE(m0 == m1);
    ASSERT_FALSE(m0 != m1);
}

TEST(simd_bits, swap_with) {
    simd_bits m0(512);
    simd_bits m1(512);
    simd_bits m2(512);
    simd_bits m3(512);
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

TEST(simd_bits, clear) {
    simd_bits m0(512);
    m0.randomize(512, SHARED_TEST_RNG());
    ASSERT_TRUE(m0.not_zero());
    m0.clear();
    ASSERT_TRUE(!m0.not_zero());
}

TEST(simd_bits, not_zero) {
    simd_bits m0(512);
    ASSERT_FALSE(m0.not_zero());
    m0[5] = true;
    ASSERT_TRUE(m0.not_zero());
    m0[511] = true;
    ASSERT_TRUE(m0.not_zero());
    m0[5] = false;
    ASSERT_TRUE(m0.not_zero());
}

TEST(simd_bits, word_range_ref) {
    simd_bits d(1024);
    const simd_bits &cref = d;
    auto r1 = d.word_range_ref(1, 2);
    auto r2 = d.word_range_ref(2, 2);
    r1[1] = true;
    ASSERT_TRUE(!r2.not_zero());
    auto k = sizeof(simd_word) * 8 + 1;
    ASSERT_EQ(r1[k], false);
    r2[1] = true;
    ASSERT_EQ(r1[k], true);
    ASSERT_EQ(cref.word_range_ref(1, 2)[k], true);
}

TEST(simd_bits, invert_bits) {
    simd_bits r(100);
    r[5] = true;
    r.invert_bits();
    for (size_t k = 0; k < 100; k++) {
        ASSERT_EQ(r[k], k != 5);
    }
}

TEST(simd_bits, mask_assignment_and) {
    simd_bits a(4);
    simd_bits b(4);
    a[2] = true;
    a[3] = true;
    b[1] = true;
    b[3] = true;
    b &= a;
    simd_bits expected(4);
    expected[3] = true;
    ASSERT_EQ(b, expected);
}

TEST(simd_bits, mask_assignment_or) {
    simd_bits a(4);
    simd_bits b(4);
    a[2] = true;
    a[3] = true;
    b[1] = true;
    b[3] = true;
    b |= a;
    simd_bits expected(4);
    expected[1] = true;
    expected[2] = true;
    expected[3] = true;
    ASSERT_EQ(b, expected);
}

TEST(simd_bits, truncated_overwrite_from) {
    simd_bits dat = simd_bits::random(1024, SHARED_TEST_RNG());
    simd_bits mut = simd_bits::random(1024, SHARED_TEST_RNG());
    simd_bits old = mut;

    mut.truncated_overwrite_from(dat, 455);

    for (size_t k = 0; k < 1024; k++) {
        ASSERT_EQ(mut[k], k < 455 ? dat[k] : old[k]) << k;
    }
}

TEST(simd_bits, popcnt) {
    simd_bits data(1024);
    ASSERT_EQ(data.popcnt(), 0);
    data[101] = 1;
    ASSERT_EQ(data.popcnt(), 1);
    data[0] = 1;
    ASSERT_EQ(data.popcnt(), 2);
    data.u64[8] = 0xFFFFFFFFFFFFFFFFULL;
    ASSERT_EQ(data.popcnt(), 66);
    ASSERT_EQ(simd_bits(0).popcnt(), 0);
}

TEST(simd_bits, prefix_ref) {
    simd_bits data(1024);
    auto prefix = data.prefix_ref(257);
    ASSERT_TRUE(prefix.num_bits_padded() >= 257);
    ASSERT_TRUE(prefix.num_bits_padded() < 1024);
    ASSERT_FALSE(data[0]);
    prefix[0] = true;
    ASSERT_TRUE(data[0]);
}
