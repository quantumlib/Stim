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

#include <random>

#include "gtest/gtest.h"

#include "stim/mem/simd_util.h"
#include "stim/mem/simd_word.test.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(simd_bits, move, {
    simd_bits<W> a(512);
    auto ptr = a.u64;

    simd_bits<W> b(std::move(a));
    ASSERT_EQ(b.u64, ptr);

    simd_bits<W> c(1);
    c = std::move(b);
    ASSERT_EQ(c.u64, ptr);
})

TEST_EACH_WORD_SIZE_W(simd_bits, construct, {
    simd_bits<W> d(1024);
    ASSERT_NE(d.ptr_simd, nullptr);
    ASSERT_EQ(d.num_simd_words, 1024 / W);
    ASSERT_EQ(d.num_bits_padded(), 1024);
    ASSERT_EQ(d.num_u8_padded(), 128);
    ASSERT_EQ(d.num_u16_padded(), 64);
    ASSERT_EQ(d.num_u32_padded(), 32);
    ASSERT_EQ(d.num_u64_padded(), 16);
})

TEST_EACH_WORD_SIZE_W(simd_bits, aliased_editing_and_bit_refs, {
    simd_bits<W> d(1024);
    auto c = (char *)d.u8;
    const simd_bits<W> &cref = d;

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
})

TEST_EACH_WORD_SIZE_W(simd_bits, min_bits_to_num_bits_padded, {
    const auto &f = &min_bits_to_num_bits_padded<W>;
    if (W == 256) {
        ASSERT_EQ(f(0), 0);
        ASSERT_EQ(f(1), 256);
        ASSERT_EQ(f(100), 256);
        ASSERT_EQ(f(255), 256);
        ASSERT_EQ(f(256), 256);
        ASSERT_EQ(f(257), 512);
        ASSERT_EQ(f((1 << 30) - 1), 1 << 30);
        ASSERT_EQ(f(1 << 30), 1 << 30);
        ASSERT_EQ(f((1 << 30) + 1), (1 << 30) + 256);
    } else if (W == 128) {
        ASSERT_EQ(f(0), 0);
        ASSERT_EQ(f(1), 128);
        ASSERT_EQ(f(100), 128);
        ASSERT_EQ(f(255), 256);
        ASSERT_EQ(f(256), 256);
        ASSERT_EQ(f(257), 256 + 128);
        ASSERT_EQ(f((1 << 30) - 1), 1 << 30);
        ASSERT_EQ(f(1 << 30), 1 << 30);
        ASSERT_EQ(f((1 << 30) + 1), (1 << 30) + 128);
    } else if (W == 64) {
        ASSERT_EQ(f(0), 0);
        ASSERT_EQ(f(1), 64);
        ASSERT_EQ(f(100), 128);
        ASSERT_EQ(f(255), 256);
        ASSERT_EQ(f(256), 256);
        ASSERT_EQ(f(257), 256 + 64);
        ASSERT_EQ(f((1 << 30) - 1), 1 << 30);
        ASSERT_EQ(f(1 << 30), 1 << 30);
        ASSERT_EQ(f((1 << 30) + 1), (1 << 30) + 64);
    } else {
        ASSERT_TRUE(false) << "Unrecognized size.";
    }
})

TEST_EACH_WORD_SIZE_W(simd_bits, str, {
    simd_bits<W> d(256);
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
})

TEST_EACH_WORD_SIZE_W(simd_bits, randomize, {
    simd_bits<W> d(1024);

    auto rng = INDEPENDENT_TEST_RNG();
    d.randomize(64 + 57, rng);
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
    d.randomize(64 + 57, rng);
    // Randomized.
    ASSERT_NE(d.u64[0], 0);
    ASSERT_NE(d.u64[0], SIZE_MAX);
    ASSERT_NE(d.u64[1] & mask, 0);
    ASSERT_NE(d.u64[1] & mask, 0);
    // Not touched.
    ASSERT_EQ(d.u64[1] & ~mask, UINT64_MAX & ~mask);
    ASSERT_EQ(d.u64[2], UINT64_MAX);
    ASSERT_EQ(d.u64[3], UINT64_MAX);
})

TEST_EACH_WORD_SIZE_W(simd_bits, xor_assignment, {
    auto rng = INDEPENDENT_TEST_RNG();
    simd_bits<W> m0 = simd_bits<W>::random(512, rng);
    simd_bits<W> m1 = simd_bits<W>::random(512, rng);
    simd_bits<W> m2(512);
    m2 ^= m0;
    ASSERT_EQ(m0, m2);
    m2 ^= m1;
    for (size_t k = 0; k < m0.num_u64_padded(); k++) {
        ASSERT_EQ(m2.u64[k], m0.u64[k] ^ m1.u64[k]);
    }
})

template <size_t W>
void set_bits_from_u64_vector(simd_bits<W> &bits, std::vector<uint64_t> &vec) {
    for (size_t w = 0; w < bits.num_u64_padded(); w++) {
        for (size_t b = 0; b < 64; b++) {
            bits[w * 64 + b] |= (vec[w] & (1ULL << b));
        }
    }
}

TEST_EACH_WORD_SIZE_W(simd_bits, add_assignment, {
    simd_bits<W> m0(512);
    simd_bits<W> m1(512);
    uint64_t all_set = 0xFFFFFFFFFFFFFFFFULL;
    uint64_t on_off = 0x0F0F0F0F0F0F0F0FULL;
    for (size_t word = 0; word < m0.num_u64_padded(); word++) {
        for (size_t k = 0; k < 64; k++) {
            if (word % 2 == 0) {
                m0[word * 64 + k] = all_set & (1ULL << k);
                m1[word * 64 + k] = all_set & (1ULL << k);
            } else {
                m0[word * 64 + k] = (bool)(on_off & (1ULL << k));
                m1[word * 64 + k] = (bool)(on_off & (1ULL << k));
            }
        }
    }
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
    for (size_t k = 0; k < m0.num_u64_padded() / 2; k++) {
        m1.u64[2 * k] = 0ULL;
        m1.u64[2 * k + 1] = 0ULL;
    }
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
    m0.clear();
    m1.clear();
    m1[0] = 1;
    for (int i = 0; i < 512; i++) {
        m0 += m1;
    }
    for (size_t k = 0; k < 64; k++) {
        if (k == 9) {
            ASSERT_EQ(m0[k], 1);
        } else {
            ASSERT_EQ(m0[k], 0);
        }
    }
    m0.clear();
    for (size_t k = 0; k < 64; k++) {
        m0[k] = all_set & (1ULL << k);
    }
    m0 += m1;
    ASSERT_EQ(m0[0], 0);
    ASSERT_EQ(m0[64], 1);
    // Test carrying across multiple (>=2) words.
    size_t num_bits = 193;
    simd_bits<W> add(num_bits);
    simd_bits<W> one(num_bits);
    for (size_t word = 0; word < add.num_u64_padded() - 1; word++) {
        for (size_t k = 0; k < 64; k++) {
            add[word * 64 + k] = 1;
        }
    }
    one[0] = 1;
    add += one;
    // These should all overflow and carries should propagate to the last word.
    for (size_t k = 0; k < num_bits - 1; k++) {
        ASSERT_EQ(add[k], 0);
    }
    ASSERT_EQ(add[num_bits - 1], 1);
    // From python
    std::vector<uint64_t> x{
        0ULL,
        14988672980522980357ULL,
        18446744073709551615ULL,
        18446744073709551615ULL,
        6866573900576593249ULL,
        0ULL,
        18446744073709551615ULL,
        0ULL};
    std::vector<uint64_t> y{
        4413476325400229597ULL,
        0ULL,
        9428810821357656676ULL,
        7863636477302268070ULL,
        0ULL,
        18446744073709551615ULL,
        0ULL,
        15077824728923429555ULL};
    std::vector<uint64_t> z{
        4413476325400229597ULL,
        14988672980522980357ULL,
        9428810821357656675ULL,
        7863636477302268070ULL,
        6866573900576593250ULL,
        18446744073709551615ULL,
        18446744073709551615ULL,
        15077824728923429555ULL};
    simd_bits<W> a(512), b(512), ref(512);
    set_bits_from_u64_vector(a, x);
    set_bits_from_u64_vector(b, y);
    set_bits_from_u64_vector(ref, z);
    a += b;
    ASSERT_EQ(a, ref);
})

template <size_t W>
void set_random_words_to_all_set(
    simd_bits<W> &bits, size_t num_bits, std::mt19937_64 &rng, std::uniform_real_distribution<double> &dist_real) {
    bits.randomize(num_bits, rng);
    size_t max_bit = W;
    for (size_t iword = 0; iword < bits.num_simd_words; iword++) {
        double r = dist_real(rng);
        if (iword == bits.num_simd_words - 1) {
            max_bit = num_bits - W * iword;
        }
        if (r < 1.0 / 3.0) {
            double rall = dist_real(rng);
            if (rall > 0.5) {
                for (size_t k = 0; k < max_bit; k++) {
                    bits[iword * W + k] = 1;
                }
            } else {
                for (size_t k = 0; k < max_bit; k++) {
                    bits[iword * W + k] = 0;
                }
            }
        }
    }
}

TEST_EACH_WORD_SIZE_W(simd_bits, fuzz_add_assignment, {
    auto rng = INDEPENDENT_TEST_RNG();
    // a + b == b + a
    std::uniform_real_distribution<double> dist_real(0, 1);
    for (int i = 0; i < 10; i++) {
        std::uniform_int_distribution<int> dist_bits(1, 1200);
        int num_bits = dist_bits(rng);
        simd_bits<W> m1(num_bits), m2(num_bits);
        set_random_words_to_all_set(m1, num_bits, rng, dist_real);
        set_random_words_to_all_set(m2, num_bits, rng, dist_real);
        simd_bits<W> ref1(m1), ref2(m2);
        m1 += ref2;
        m2 += ref1;
        ASSERT_EQ(m1, m2);
    }
    // (a + 1) + ~a = allset
    for (int i = 0; i < 10; i++) {
        std::uniform_int_distribution<int> dist_bits(1, 1200);
        int num_bits = dist_bits(rng);
        simd_bits<W> m1(num_bits);
        simd_bits<W> zero(num_bits);
        simd_bits<W> one(num_bits);
        one[0] = 1;
        set_random_words_to_all_set(m1, num_bits, rng, dist_real);
        simd_bits<W> m2(m1);
        m2.invert_bits();
        m1 += one;
        m1 += m2;
        ASSERT_EQ(m1, zero);
    }
    // m1 += x; m1 = ~m1; m1 += x; m1 is unchanged.
    for (int i = 0; i < 10; i++) {
        std::uniform_int_distribution<int> dist_bits(1, 1200);
        int num_bits = dist_bits(rng);
        simd_bits<W> m1(num_bits);
        m1.randomize(num_bits, rng);
        set_random_words_to_all_set(m1, num_bits, rng, dist_real);
        simd_bits<W> ref(m1);
        simd_bits<W> m2(num_bits);
        m1 += m2;
        m1.invert_bits();
        m1 += m2;
        m1.invert_bits();
        ASSERT_EQ(m1, ref);
    }
    // a + (b + c) == (a + b) + c
    for (int i = 0; i < 10; i++) {
        std::uniform_int_distribution<int> dist_bits(1, 1200);
        int num_bits = dist_bits(rng);
        simd_bits<W> alhs(num_bits);
        simd_bits<W> blhs(num_bits);
        simd_bits<W> clhs(num_bits);
        simd_bits<W> arhs(num_bits);
        simd_bits<W> brhs(num_bits);
        simd_bits<W> crhs(num_bits);
        set_random_words_to_all_set(alhs, num_bits, rng, dist_real);
        arhs = alhs;
        set_random_words_to_all_set(blhs, num_bits, rng, dist_real);
        brhs = blhs;
        set_random_words_to_all_set(clhs, num_bits, rng, dist_real);
        crhs = clhs;
        blhs += clhs;
        alhs += blhs;
        arhs += brhs;
        arhs += crhs;
        ASSERT_EQ(alhs, arhs);
    }
})

TEST_EACH_WORD_SIZE_W(simd_bits, right_shift_assignment, {
    simd_bits<W> m0(512), m1(512);
    m0[511] = 1;
    m0 >>= 64;
    for (size_t word = 0; word < m0.num_u64_padded(); word++) {
        uint64_t pattern = 0ULL;
        for (size_t k = 0; k < 64; k++) {
            pattern |= (uint64_t{m0[word * 64 + k]} << k);
        }
        if (word != m0.num_u64_padded() - 2) {
            ASSERT_EQ(pattern, 0ULL);
        } else {
            ASSERT_EQ(pattern, uint64_t{1} << 63);
        }
    }
    m1 = m0;
    m1 >>= 0;
    for (size_t k = 0; k < 512; k++) {
        ASSERT_EQ(m0[k], m1[k]);
    }
    m0.clear();
    uint64_t on_off = 0xAAAAAAAAAAAAAAAAULL;
    for (size_t word = 0; word < m0.num_u64_padded(); word++) {
        for (size_t k = 0; k < 64; k++) {
            m0[word * 64 + k] = (bool)(on_off & (1ULL << k));
        }
    }
    m0 >>= 1;
    for (size_t word = 0; word < m0.num_u64_padded(); word++) {
        uint64_t pattern = 0ULL;
        for (size_t k = 0; k < 64; k++) {
            pattern |= (uint64_t{m0[word * 64 + k]} << k);
        }
        ASSERT_EQ(pattern, 0x5555555555555555ULL);
    }
    m0.clear();
    for (size_t word = 0; word < m0.num_u64_padded(); word++) {
        for (size_t k = 0; k < 64; k++) {
            m0[word * 64 + k] = (bool)(on_off & (1ULL << k));
        }
    }
    m0 >>= 128;
    for (size_t word = 0; word < m0.num_u64_padded(); word++) {
        uint64_t pattern = 0ULL;
        for (size_t k = 0; k < 64; k++) {
            pattern |= (uint64_t{m0[word * 64 + k]} << k);
        }
        if (word < 6) {
            ASSERT_EQ(pattern, 0xAAAAAAAAAAAAAAAA);
        } else {
            ASSERT_EQ(pattern, 0ULL);
        }
    }
})

TEST_EACH_WORD_SIZE_W(simd_bits, fuzz_right_shift_assignment, {
    auto rng = INDEPENDENT_TEST_RNG();
    for (int i = 0; i < 5; i++) {
        std::uniform_int_distribution<int> dist_bits(1, 1200);
        int num_bits = dist_bits(rng);
        simd_bits<W> m1(num_bits), m2(num_bits);
        m1.randomize(num_bits, rng);
        m2 = m1;
        std::uniform_int_distribution<size_t> dist_shift(0, (int)m1.num_bits_padded());
        size_t shift = dist_shift(rng);
        m1 >>= shift;
        for (size_t k = 0; k < m1.num_bits_padded() - shift; k++) {
            ASSERT_EQ(m1[k], m2[k + shift]);
        }
        for (size_t k = m1.num_bits_padded() - shift; k < m1.num_bits_padded(); k++) {
            ASSERT_EQ(m1[k], 0);
        }
    }
})

TEST_EACH_WORD_SIZE_W(simd_bits, left_shift_assignment, {
    simd_bits<W> m0(512), m1(512);
    for (size_t w = 0; w < m0.num_u64_padded(); w++) {
        m0.u64[w] = 0xAAAAAAAAAAAAAAAAULL;
    }
    m0 <<= 1;
    m1 = m0;
    m1 <<= 0;
    for (size_t k = 0; k < 512; k++) {
        ASSERT_EQ(m0[k], m1[k]);
    }
    for (size_t w = 0; w < m0.num_u64_padded(); w++) {
        if (w == 0) {
            ASSERT_EQ(m0.u64[w], 0x5555555555555554ULL);
        } else {
            ASSERT_EQ(m0.u64[w], 0x5555555555555555ULL);
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
    m0 <<= 488;
    ASSERT_TRUE(!m0.not_zero());
})

TEST_EACH_WORD_SIZE_W(simd_bits, fuzz_left_shift_assignment, {
    auto rng = INDEPENDENT_TEST_RNG();
    for (int i = 0; i < 5; i++) {
        std::uniform_int_distribution<int> dist_bits(1, 1200);
        int num_bits = dist_bits(rng);
        simd_bits<W> m1(num_bits), m2(num_bits);
        m1.randomize(num_bits, rng);
        m2 = m1;
        std::uniform_int_distribution<size_t> dist_shift(0, (int)m1.num_bits_padded());
        size_t shift = dist_shift(rng);
        m1 <<= shift;
        for (size_t k = 0; k < m1.num_bits_padded() - shift; k++) {
            ASSERT_EQ(m1[k + shift], m2[k]);
        }
        for (size_t k = 0; k < shift; k++) {
            ASSERT_EQ(m1[k], 0);
        }
    }
})

TEST_EACH_WORD_SIZE_W(simd_bits, assignment, {
    simd_bits<W> m0(512);
    simd_bits<W> m1(512);
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

TEST_EACH_WORD_SIZE_W(simd_bits, equality, {
    simd_bits<W> m0(512);
    simd_bits<W> m1(512);
    simd_bits<W> m4(1024);

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
})

TEST_EACH_WORD_SIZE_W(simd_bits, swap_with, {
    simd_bits<W> m0(512);
    simd_bits<W> m1(512);
    simd_bits<W> m2(512);
    simd_bits<W> m3(512);
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

TEST_EACH_WORD_SIZE_W(simd_bits, clear, {
    simd_bits<W> m0(512);
    auto rng = INDEPENDENT_TEST_RNG();
    m0.randomize(512, rng);
    ASSERT_TRUE(m0.not_zero());
    m0.clear();
    ASSERT_TRUE(!m0.not_zero());
})

TEST_EACH_WORD_SIZE_W(simd_bits, not_zero, {
    simd_bits<W> m0(512);
    ASSERT_FALSE(m0.not_zero());
    m0[5] = true;
    ASSERT_TRUE(m0.not_zero());
    m0[511] = true;
    ASSERT_TRUE(m0.not_zero());
    m0[5] = false;
    ASSERT_TRUE(m0.not_zero());
})

TEST_EACH_WORD_SIZE_W(simd_bits, word_range_ref, {
    simd_bits<W> d(1024);
    const simd_bits<W> &cref = d;
    auto r1 = d.word_range_ref(1, 2);
    auto r2 = d.word_range_ref(2, 2);
    r1[1] = true;
    ASSERT_TRUE(!r2.not_zero());
    auto k = W + 1;
    ASSERT_EQ(r1[k], false);
    r2[1] = true;
    ASSERT_EQ(r1[k], true);
    ASSERT_EQ(cref.word_range_ref(1, 2)[k], true);
})

TEST_EACH_WORD_SIZE_W(simd_bits, invert_bits, {
    simd_bits<W> r(100);
    r[5] = true;
    r.invert_bits();
    for (size_t k = 0; k < 100; k++) {
        ASSERT_EQ(r[k], k != 5);
    }
})

TEST_EACH_WORD_SIZE_W(simd_bits, mask_assignment_and, {
    simd_bits<W> a(4);
    simd_bits<W> b(4);
    a[2] = true;
    a[3] = true;
    b[1] = true;
    b[3] = true;
    b &= a;
    simd_bits<W> expected(4);
    expected[3] = true;
    ASSERT_EQ(b, expected);
})

TEST_EACH_WORD_SIZE_W(simd_bits, mask_assignment_or, {
    simd_bits<W> a(4);
    simd_bits<W> b(4);
    a[2] = true;
    a[3] = true;
    b[1] = true;
    b[3] = true;
    b |= a;
    simd_bits<W> expected(4);
    expected[1] = true;
    expected[2] = true;
    expected[3] = true;
    ASSERT_EQ(b, expected);
})

TEST_EACH_WORD_SIZE_W(simd_bits, truncated_overwrite_from, {
    auto rng = INDEPENDENT_TEST_RNG();
    simd_bits<W> dat = simd_bits<W>::random(1024, rng);
    simd_bits<W> mut = simd_bits<W>::random(1024, rng);
    simd_bits<W> old = mut;

    mut.truncated_overwrite_from(dat, 455);

    for (size_t k = 0; k < 1024; k++) {
        ASSERT_EQ(mut[k], k < 455 ? dat[k] : old[k]) << k;
    }
})

TEST_EACH_WORD_SIZE_W(simd_bits, popcnt, {
    simd_bits<W> data(1024);
    ASSERT_EQ(data.popcnt(), 0);
    data[101] = 1;
    ASSERT_EQ(data.popcnt(), 1);
    data[0] = 1;
    ASSERT_EQ(data.popcnt(), 2);
    data.u64[8] = 0xFFFFFFFFFFFFFFFFULL;
    ASSERT_EQ(data.popcnt(), 66);
    ASSERT_EQ(simd_bits<W>(0).popcnt(), 0);
})

TEST_EACH_WORD_SIZE_W(simd_bits, countr_zero, {
    simd_bits<W> data(1024);
    ASSERT_EQ(data.countr_zero(), SIZE_MAX);
    data[1000] = 1;
    ASSERT_EQ(data.countr_zero(), 1000);
    data[101] = 1;
    ASSERT_EQ(data.countr_zero(), 101);
    data[260] = 1;
    ASSERT_EQ(data.countr_zero(), 101);
    data[0] = 1;
    ASSERT_EQ(data.countr_zero(), 0);
})

TEST_EACH_WORD_SIZE_W(simd_bits, prefix_ref, {
    simd_bits<W> data(1024);
    auto prefix = data.prefix_ref(257);
    ASSERT_TRUE(prefix.num_bits_padded() >= 257);
    ASSERT_TRUE(prefix.num_bits_padded() < 1024);
    ASSERT_FALSE(data[0]);
    prefix[0] = true;
    ASSERT_TRUE(data[0]);
})

TEST_EACH_WORD_SIZE_W(simd_bits, out_of_place_bit_masking, {
    simd_bits<W> m0(4);
    simd_bits<W> m1(4);
    m0.ptr_simd[0] = simd_word<W>(0b0101);
    m1.ptr_simd[0] = simd_word<W>(0b0011);
    ASSERT_EQ((m0 & m1).ptr_simd[0], 0b0001);
    ASSERT_EQ((m0 | m1).ptr_simd[0], 0b0111);
    ASSERT_EQ((m0 ^ m1).ptr_simd[0], 0b0110);
})
