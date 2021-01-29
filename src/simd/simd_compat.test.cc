#include "simd_compat.h"
#include "../test_util.test.h"

#include <gtest/gtest.h>

TEST(simd_compat, popcnt64) {
    ASSERT_EQ(popcnt64(0), 0);
    ASSERT_EQ(popcnt64(1), 1);
    ASSERT_EQ(popcnt64(2), 1);
    ASSERT_EQ(popcnt64(3), 2);
    ASSERT_EQ(popcnt64(0b1010111011011011010011111000011101010111101010101010110101011101ULL), 39);
    ASSERT_EQ(popcnt64(UINT64_MAX), 64);
    for (size_t k = 0; k < 70000; k++) {
        auto expected = 0;
        size_t k2 = k;
        while (k2) {
            k2 &= k2 - 1;
            expected++;
        }
        ASSERT_EQ(popcnt64(k), expected);
    }
}

TEST(simd_compat, popcount) {
    union {
        simd_word w;
        uint64_t p[sizeof(simd_word) / sizeof(uint64_t)]{};
    } v{};
    auto n = sizeof(simd_word) * 8;

    for (size_t expected = 0; expected <= n; expected++) {
        std::vector<uint64_t> bits{};
        for (size_t i = 0; i < n; i++) {
            bits.push_back(i < expected);
        }
        for (size_t reps = 0; reps < 100; reps++) {
            std::shuffle(bits.begin(), bits.end(), SHARED_TEST_RNG());
            for (size_t i = 0; i < n; i++) {
                v.p[i >> 6] = 0;
            }
            for (size_t i = 0; i < n; i++) {
                v.p[i >> 6] |= bits[i] << (i & 63);
            }
            if (v.w.popcount() != expected) {
                std::cerr << v.p[0] << "\n";
                std::cerr << v.p[1] << "\n";
                std::cerr << v.p[2] << "\n";
                std::cerr << v.p[3] << "\n";
            }
            ASSERT_EQ(v.w.popcount(), expected);
        }
    }
}

TEST(simd_compat, do_interleave8_tile128) {
    simd_word t1{};
    simd_word t2{};
    auto c1 = t1.u8;
    auto c2 = t2.u8;
    for (uint8_t k = 0; k < (uint8_t)sizeof(uint64_t) * 2; k++) {
        c1[k] = k + 1;
        c2[k] = k + 128;
    }
    t1.do_interleave8_tile128(t2);
    for (size_t k = 0; k < sizeof(uint64_t) * 2; k++) {
        ASSERT_EQ(c1[k], k % 2 == 0 ? (k / 2) + 1 : (k / 2) + 128);
        ASSERT_EQ(c2[k], k % 2 == 0 ? (k / 2) + 1 + 8 : (k / 2) + 128 + 8);
    }
}

TEST(simd_word, operator_bool) {
    simd_word w{};
    auto p = &w.u64[0];
    ASSERT_EQ((bool)w, false);
    p[0] = 5;
    ASSERT_EQ((bool)w, true);
    p[0] = 0;
    p[1] = 5;
    ASSERT_EQ((bool)w, true);
    p[1] = 0;
    ASSERT_EQ((bool)w, false);
}
