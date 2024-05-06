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

#include "stim/mem/simd_bits.h"
#include "stim/mem/simd_word.test.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, construct, {
    alignas(64) std::array<uint64_t, 16> data{};
    simd_bits_range_ref<W> ref((bitword<W> *)data.data(), sizeof(data) / (W / 8));

    ASSERT_EQ(ref.ptr_simd, (bitword<W> *)&data[0]);
    ASSERT_EQ(ref.num_simd_words, 16 * sizeof(uint64_t) / sizeof(bitword<W>));
    ASSERT_EQ(ref.num_bits_padded(), 1024);
    ASSERT_EQ(ref.num_u8_padded(), 128);
    ASSERT_EQ(ref.num_u16_padded(), 64);
    ASSERT_EQ(ref.num_u32_padded(), 32);
    ASSERT_EQ(ref.num_u64_padded(), 16);
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, aliased_editing_and_bit_refs, {
    alignas(64) std::array<uint64_t, 16> data{};
    auto c = (char *)&data;
    simd_bits_range_ref<W> ref((bitword<W> *)data.data(), sizeof(data) / sizeof(bitword<W>));
    const simd_bits_range_ref<W> cref((bitword<W> *)data.data(), sizeof(data) / sizeof(bitword<W>));

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
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, str, {
    alignas(64) std::array<uint64_t, 8> data{};
    simd_bits_range_ref<W> ref((bitword<W> *)data.data(), sizeof(data) / sizeof(bitword<W>));
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
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, randomize, {
    alignas(64) std::array<uint64_t, 16> data{};
    simd_bits_range_ref<W> ref((bitword<W> *)data.data(), sizeof(data) / sizeof(bitword<W>));

    auto rng = INDEPENDENT_TEST_RNG();
    ref.randomize(64 + 57, rng);
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
    ref.randomize(64 + 57, rng);
    // Randomized.
    ASSERT_NE(ref.u64[0], 0);
    ASSERT_NE(ref.u64[0], SIZE_MAX);
    ASSERT_NE(ref.u64[1] & mask, 0);
    ASSERT_NE(ref.u64[1] & mask, 0);
    // Not touched.
    ASSERT_EQ(ref.u64[1] & ~mask, UINT64_MAX & ~mask);
    ASSERT_EQ(ref.u64[2], UINT64_MAX);
    ASSERT_EQ(ref.u64[3], UINT64_MAX);
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, xor_assignment, {
    alignas(64) std::array<uint64_t, 24> data{};
    simd_bits_range_ref<W> m0((bitword<W> *)&data[0], sizeof(data) / sizeof(bitword<W>) / 3);
    simd_bits_range_ref<W> m1((bitword<W> *)&data[8], sizeof(data) / sizeof(bitword<W>) / 3);
    simd_bits_range_ref<W> m2((bitword<W> *)&data[16], sizeof(data) / sizeof(bitword<W>) / 3);
    auto rng = INDEPENDENT_TEST_RNG();
    m0.randomize(512, rng);
    m1.randomize(512, rng);
    ASSERT_NE(m0, m1);
    ASSERT_NE(m0, m2);
    m2 ^= m0;
    ASSERT_EQ(m0, m2);
    m2 ^= m1;
    for (size_t k = 0; k < m0.num_u64_padded(); k++) {
        ASSERT_EQ(m2.u64[k], m0.u64[k] ^ m1.u64[k]);
    }
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, assignment, {
    alignas(64) std::array<uint64_t, 16> data{};
    simd_bits_range_ref<W> m0((bitword<W> *)&data[0], sizeof(data) / sizeof(bitword<W>) / 2);
    simd_bits_range_ref<W> m1((bitword<W> *)&data[8], sizeof(data) / sizeof(bitword<W>) / 2);
    auto rng = INDEPENDENT_TEST_RNG();
    m0.randomize(512, rng);
    m1.randomize(512, rng);
    auto old_m1 = m1.u64[0];
    ASSERT_NE(m0, m1);
    m0 = m1;
    ASSERT_EQ(m0, m1);
    ASSERT_EQ(m0.u64[0], old_m1);
    ASSERT_EQ(m1.u64[0], old_m1);
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, equality, {
    alignas(64) std::array<uint64_t, 32> data{};
    simd_bits_range_ref<W> m0((bitword<W> *)&data[0], sizeof(data) / sizeof(bitword<W>) / 4);
    simd_bits_range_ref<W> m1((bitword<W> *)&data[8], sizeof(data) / sizeof(bitword<W>) / 4);
    simd_bits_range_ref<W> m4((bitword<W> *)&data[16], sizeof(data) / sizeof(bitword<W>) / 2);

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
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, add_assignment, {
    alignas(64) std::array<uint64_t, 8> data{
        0xFFFFFFFFFFFFFFFFULL,
        0x0F0F0F0F0F0F0F0FULL,
        0xFFFFFFFFFFFFFFFFULL,
        0x0F0F0F0F0F0F0F0FULL,
        0xFFFFFFFFFFFFFFFFULL,
        0x0F0F0F0F0F0F0F0FULL,
        0xFFFFFFFFFFFFFFFFULL,
        0x0F0F0F0F0F0F0F0FULL};
    simd_bits_range_ref<W> m0((bitword<W> *)&data[0], sizeof(data) / sizeof(bitword<W>) / 2);
    simd_bits_range_ref<W> m1((bitword<W> *)&data[4], sizeof(data) / sizeof(bitword<W>) / 2);
    m0 += m1;
    for (size_t word = 0; word < m0.num_u64_padded(); word++) {
        uint64_t pattern = 0ULL;
        for (size_t k = 0; k < 64; k++) {
            pattern |= (uint64_t{m0[word * 64 + k]} << k);
        }
        if (word % 2 == 0) {
            ASSERT_EQ(pattern, 0xFFFFFFFFFFFFFFFEULL);
        } else {
            ASSERT_EQ(pattern, 0x1E1E1E1E1E1E1E1FULL);
        }
    }
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, right_shift_assignment, {
    alignas(64) std::array<uint64_t, 8> data{
        0xAAAAAAAAAAAAAAAAULL,
        0xAAAAAAAAAAAAAAAAULL,
        0xAAAAAAAAAAAAAAAAULL,
        0xAAAAAAAAAAAAAAAAULL,
        0xAAAAAAAAAAAAAAAAULL,
        0xAAAAAAAAAAAAAAAAULL,
        0xAAAAAAAAAAAAAAAAULL,
        0xAAAAAAAAAAAAAAAAULL,
    };
    simd_bits_range_ref<W> m0((bitword<W> *)&data[0], sizeof(data) / sizeof(bitword<W>));
    m0 >>= 1;
    for (size_t word = 0; word < m0.num_u64_padded(); word++) {
        uint64_t pattern = 0ULL;
        for (size_t k = 0; k < 64; k++) {
            pattern |= (uint64_t{m0[word * 64 + k]} << k);
        }
        ASSERT_EQ(pattern, 0x5555555555555555ULL);
    }
    m0 >>= 511;
    ASSERT_TRUE(!m0.not_zero());
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, left_shift_assignment, {
    alignas(64) std::array<uint64_t, 8> data{
        0xAAAAAAAAAAAAAAAAULL,
        0xAAAAAAAAAAAAAAAAULL,
        0xAAAAAAAAAAAAAAAAULL,
        0xAAAAAAAAAAAAAAAAULL,
        0xAAAAAAAAAAAAAAAAULL,
        0xAAAAAAAAAAAAAAAAULL,
        0xAAAAAAAAAAAAAAAAULL,
        0xAAAAAAAAAAAAAAAAULL,
    };
    simd_bits_range_ref<W> m0((bitword<W> *)&data[0], sizeof(data) / sizeof(bitword<W>));
    m0 <<= 1;
    for (size_t word = 0; word < m0.num_u64_padded(); word++) {
        uint64_t pattern = 0ULL;
        for (size_t k = 0; k < 64; k++) {
            pattern |= (uint64_t{m0[word * 64 + k]} << k);
        }
        if (word == 0) {
            ASSERT_EQ(pattern, 0x5555555555555554ULL);
        } else {
            ASSERT_EQ(pattern, 0x5555555555555555ULL);
        }
    }
    m0 <<= 63;
    for (size_t w = 0; w < m0.num_u64_padded(); w++) {
        if (w == 0) {
            ASSERT_EQ(m0.u64[w], 0ULL);
        } else {
            ASSERT_EQ(m0.u64[w], 0xAAAAAAAAAAAAAAAAULL);
        }
    }
    for (size_t word = 0; word < m0.num_u64_padded(); word++) {
        uint64_t pattern = 0ULL;
        for (size_t k = 0; k < 64; k++) {
            pattern |= (uint64_t{m0[word * 64 + k]} << k);
        }
        if (word == 0) {
            ASSERT_EQ(pattern, 0ULL);
        } else {
            ASSERT_EQ(pattern, 0xAAAAAAAAAAAAAAAAULL);
        }
    }
    m0 <<= 488;
    ASSERT_TRUE(!m0.not_zero());
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, swap_with, {
    alignas(64) std::array<uint64_t, 32> data{};
    simd_bits_range_ref<W> m0((bitword<W> *)&data[0], sizeof(data) / sizeof(bitword<W>) / 4);
    simd_bits_range_ref<W> m1((bitword<W> *)&data[8], sizeof(data) / sizeof(bitword<W>) / 4);
    simd_bits_range_ref<W> m2((bitword<W> *)&data[16], sizeof(data) / sizeof(bitword<W>) / 4);
    simd_bits_range_ref<W> m3((bitword<W> *)&data[24], sizeof(data) / sizeof(bitword<W>) / 4);
    auto rng = INDEPENDENT_TEST_RNG();
    m0.randomize(512, rng);
    m1.randomize(512, rng);
    m2 = m0;
    m3 = m1;
    ASSERT_EQ(m0, m2);
    ASSERT_EQ(m1, m3);
    m0.swap_with(m1);
    ASSERT_EQ(m0, m3);
    ASSERT_EQ(m1, m2);
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, clear, {
    alignas(64) std::array<uint64_t, 8> data{};
    simd_bits_range_ref<W> m0((bitword<W> *)&data[0], sizeof(data) / sizeof(bitword<W>));
    auto rng = INDEPENDENT_TEST_RNG();
    m0.randomize(512, rng);
    ASSERT_TRUE(m0.not_zero());
    m0.clear();
    ASSERT_TRUE(!m0.not_zero());
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, not_zero256, {
    alignas(64) std::array<uint64_t, 8> data{};
    simd_bits_range_ref<W> m0((bitword<W> *)&data[0], sizeof(data) / sizeof(bitword<W>));
    ASSERT_FALSE(m0.not_zero());
    m0[5] = true;
    ASSERT_TRUE(m0.not_zero());
    m0[511] = true;
    ASSERT_TRUE(m0.not_zero());
    m0[5] = false;
    ASSERT_TRUE(m0.not_zero());
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, word_range_ref, {
    bitword<W> d[sizeof(uint64_t) * 16 / sizeof(bitword<W>)]{};
    simd_bits_range_ref<W> ref(d, sizeof(d) / sizeof(bitword<W>));
    const simd_bits_range_ref<W> cref(d, sizeof(d) / sizeof(bitword<W>));
    auto r1 = ref.word_range_ref(1, 2);
    auto r2 = ref.word_range_ref(2, 2);
    r1[1] = true;
    ASSERT_TRUE(!r2.not_zero());
    auto k = sizeof(bitword<W>) * 8 + 1;
    ASSERT_EQ(r1[k], false);
    r2[1] = true;
    ASSERT_EQ(r1[k], true);
    ASSERT_EQ(cref.word_range_ref(1, 2)[k], true);
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, for_each_set_bit, {
    simd_bits<W> data(256);
    simd_bits_range_ref<W> ref(data);
    ref[5] = true;
    ref[101] = true;
    std::vector<size_t> hits;
    ref.for_each_set_bit([&](size_t k) {
        hits.push_back(k);
    });
    ASSERT_EQ(hits, (std::vector<size_t>{5, 101}));
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, truncated_overwrite_from, {
    auto rng = INDEPENDENT_TEST_RNG();
    simd_bits<W> dat = simd_bits<W>::random(1024, rng);
    simd_bits<W> mut = simd_bits<W>::random(1024, rng);
    simd_bits<W> old = mut;

    simd_bits_range_ref<W>(mut).truncated_overwrite_from(dat, 455);

    for (size_t k = 0; k < 1024; k++) {
        ASSERT_EQ(mut[k], k < 455 ? dat[k] : old[k]) << k;
    }
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, popcnt, {
    simd_bits<W> data(1024);
    simd_bits_range_ref<W> ref(data);
    ASSERT_EQ(ref.popcnt(), 0);
    data[101] = 1;
    ASSERT_EQ(ref.popcnt(), 1);
    data[0] = 1;
    ASSERT_EQ(ref.popcnt(), 2);
    data.u64[8] = 0xFFFFFFFFFFFFFFFFULL;
    ASSERT_EQ(ref.popcnt(), 66);
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, countr_zero, {
    simd_bits<W> data(1024);
    simd_bits_range_ref<W> ref(data);
    ASSERT_EQ(ref.countr_zero(), SIZE_MAX);
    data[1000] = 1;
    ASSERT_EQ(ref.countr_zero(), 1000);
    data[101] = 1;
    ASSERT_EQ(ref.countr_zero(), 101);
    data[260] = 1;
    ASSERT_EQ(ref.countr_zero(), 101);
    data[0] = 1;
    ASSERT_EQ(ref.countr_zero(), 0);
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, intersects, {
    simd_bits<W> data(1024);
    simd_bits<W> other(512);
    simd_bits_range_ref<W> ref(data);
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
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, prefix_ref, {
    simd_bits<W> data(1024);
    simd_bits_range_ref<W> ref(data);
    auto prefix = ref.prefix_ref(257);
    ASSERT_TRUE(prefix.num_bits_padded() >= 257);
    ASSERT_TRUE(prefix.num_bits_padded() < 1024);
    ASSERT_FALSE(data[0]);
    prefix[0] = true;
    ASSERT_TRUE(data[0]);
})

TEST_EACH_WORD_SIZE_W(simd_bits_range_ref, as_u64, {
    simd_bits<W> data(1024);
    simd_bits_range_ref<W> ref(data);
    ASSERT_EQ(data.as_u64(), 0);
    ASSERT_EQ(ref.as_u64(), 0);

    ref[63] = 1;
    ASSERT_EQ(data.as_u64(), uint64_t{1} << 63);
    ASSERT_EQ(ref.as_u64(), uint64_t{1} << 63);
    ASSERT_EQ(data.word_range_ref(0, 0).as_u64(), 0);
    ASSERT_EQ(data.word_range_ref(0, 1).as_u64(), uint64_t{1} << 63);
    ASSERT_EQ(data.word_range_ref(0, 2).as_u64(), uint64_t{1} << 63);
    ASSERT_EQ(data.word_range_ref(1, 1).as_u64(), 0);
    ASSERT_EQ(data.word_range_ref(1, 2).as_u64(), 0);

    ref[64] = 1;
    ASSERT_THROW({ data.as_u64(); }, std::invalid_argument);
    ASSERT_THROW({ data.word_range_ref(0, 2).as_u64(); }, std::invalid_argument);
    ASSERT_THROW({ ref.as_u64(); }, std::invalid_argument);
    if (data.num_simd_words > 2) {
        ASSERT_EQ(data.word_range_ref(2, 1).as_u64(), 0);
    }
})
