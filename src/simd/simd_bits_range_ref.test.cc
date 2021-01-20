#include "gtest/gtest.h"
#include "simd_bits_range_ref.h"
#include "../test_util.test.h"

TEST(simd_bits_range_ref, construct) {
    __m256i data[4] {};
    simd_bits_range_ref ref(data, 4);

    ASSERT_EQ(ref.u256, &data[0]);
    ASSERT_EQ(ref.num_simd_words, 4);
    ASSERT_EQ(ref.num_bits_padded(), 1024);
    ASSERT_EQ(ref.num_u8_padded(), 128);
    ASSERT_EQ(ref.num_u16_padded(), 64);
    ASSERT_EQ(ref.num_u32_padded(), 32);
    ASSERT_EQ(ref.num_u64_padded(), 16);
}

TEST(simd_bits_range_ref, aliased_editing_and_bit_refs) {
    __m256i data[4] {};
    auto c = (char *)&data;
    simd_bits_range_ref ref(data, 4);
    const simd_bits_range_ref cref(data, 4);

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
    __m256i data {};
    simd_bits_range_ref ref(&data, 1);
    ASSERT_EQ(ref.str(),
            "________________________________________________________________"
            "________________________________________________________________"
            "________________________________________________________________"
            "________________________________________________________________");
    ref[5] = 1;
    ASSERT_EQ(ref.str(),
            "_____1__________________________________________________________"
            "________________________________________________________________"
            "________________________________________________________________"
            "________________________________________________________________");
}

TEST(simd_bits_range_ref, randomize) {
    __m256i data[4]{};
    simd_bits_range_ref ref(data, 4);

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
    ASSERT_EQ(ref.u64[1] & ~mask, SIZE_MAX & ~mask);
    ASSERT_EQ(ref.u64[2], SIZE_MAX);
    ASSERT_EQ(ref.u64[3], SIZE_MAX);
}

TEST(simd_bits_range_ref, xor_assignment) {
    __m256i data[6]{};
    simd_bits_range_ref m0(&data[0], 2);
    simd_bits_range_ref m1(&data[2], 2);
    simd_bits_range_ref m2(&data[4], 2);
    m0.randomize(512, SHARED_TEST_RNG());
    m1.randomize(512, SHARED_TEST_RNG());
    ASSERT_NE(m0, m1);
    ASSERT_NE(m0, m2);
    m2 ^= m0;
    ASSERT_EQ(m0, m2);
    for (size_t k = 0; k < m0.num_u64_padded(); k++) {
        ASSERT_EQ(m2.u64[k], m0.u64[k] ^ m1.u64[k]);
    }
    ASSERT_TRUE(false);
}

TEST(simd_bits_range_ref, assignment) {
    __m256i data[4]{};
    simd_bits_range_ref m0(&data[0], 2);
    simd_bits_range_ref m1(&data[2], 2);
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
    __m256i data[8]{};
    simd_bits_range_ref m0(&data[0], 2);
    simd_bits_range_ref m1(&data[2], 2);
    simd_bits_range_ref m4(&data[4], 4);

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
    __m256i data[8]{};
    simd_bits_range_ref m0(&data[0], 2);
    simd_bits_range_ref m1(&data[2], 2);
    simd_bits_range_ref m2(&data[4], 2);
    simd_bits_range_ref m3(&data[6], 2);
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
    __m256i data[2]{};
    simd_bits_range_ref m0(&data[0], 2);
    m0.randomize(512, SHARED_TEST_RNG());
    ASSERT_TRUE(m0.not_zero());
    m0.clear();
    ASSERT_TRUE(!m0.not_zero());
}

TEST(simd_bits_range_ref, not_zero) {
    __m256i data[2]{};
    simd_bits_range_ref m0(&data[0], 2);
    ASSERT_FALSE(m0.not_zero());
    m0[5] = 1;
    ASSERT_TRUE(m0.not_zero());
    m0[511] = 1;
    ASSERT_TRUE(m0.not_zero());
    m0[5] = 0;
    ASSERT_TRUE(m0.not_zero());
}

TEST(simd_bits_range_ref, word_range_ref) {
    __m256i data[4]{};
    simd_bits_range_ref ref(&data[0], 4);
    const simd_bits_range_ref cref(&data[0], 4);
    auto r1 = ref.word_range_ref(1, 2);
    auto r2 = ref.word_range_ref(2, 2);
    r1[1] = 1;
    ASSERT_TRUE(!r2.not_zero());
    ASSERT_EQ(r1[257], false);
    r2[1] = 1;
    ASSERT_EQ(r1[257], true);
    ASSERT_EQ(cref.word_range_ref(1, 2)[257], true);
}
