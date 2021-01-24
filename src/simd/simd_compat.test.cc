#include "simd_compat.h"

#include <gtest/gtest.h>
#include <immintrin.h>

TEST(simd_compat, popcnt64) {
    ASSERT_EQ(popcnt(0), 0);
    ASSERT_EQ(popcnt(1), 1);
    ASSERT_EQ(popcnt(2), 1);
    ASSERT_EQ(popcnt(3), 2);
    ASSERT_EQ(popcnt(0b1010111011011011010011111000011101010111101010101010110101011101ULL), 39);
    ASSERT_EQ(popcnt(UINT64_MAX), 64);
    for (size_t k = 0; k < 70000; k++) {
        auto expected = 0;
        size_t k2 = k;
        while (k2) {
            k2 &= k2 - 1;
            expected++;
        }
        ASSERT_EQ(popcnt(k), expected);
    }
}

TEST(simd_compat, popcnt128) {
    simd_word_128 w {};
    auto p = (uint64_t*)&w.val;
    p[0] = 0;
    ASSERT_EQ(w.popcount(), 0);
    p[0] = 1;
    ASSERT_EQ(w.popcount(), 1);
    p[0] = 2;
    ASSERT_EQ(w.popcount(), 1);
    p[0] = 3;
    ASSERT_EQ(w.popcount(), 2);
    p[0] = 0b1010111011011011010011111000011101010111101010101010110101011101ULL;
    ASSERT_EQ(w.popcount(), 39);
    p[1] = 0b1010111011011011010011111000011101010111101010101010110101011101ULL;
    ASSERT_EQ(w.popcount(), 39 * 2);
    p[1] = 1;
    ASSERT_EQ(w.popcount(), 40);
    p[0] = UINT64_MAX;
    p[1] = UINT64_MAX;
    ASSERT_EQ(w.popcount(), 128);
    for (size_t k = 0; k < 70000; k++) {
        p[0] = k;
        p[1] = k;
        auto expected = 0;
        size_t k2 = k;
        while (k2) {
            k2 &= k2 - 1;
            expected++;
        }
        ASSERT_EQ(w.popcount(), expected * 2);
    }
    p[0] = 0;
    p[1] = 0;
    for (size_t k = 0; k < 128; k++) {
        p[k >> 6] |= (1ULL << (k & 63));
        ASSERT_EQ(w.popcount(), k + 1);
    }
}

TEST(simd_compat, popcnt256) {
    simd_word_256 w {};
    auto p = (uint64_t*)&w.val;
    p[0] = 0;
    ASSERT_EQ(w.popcount(), 0);
    p[0] = 1;
    ASSERT_EQ(w.popcount(), 1);
    p[0] = 2;
    ASSERT_EQ(w.popcount(), 1);
    p[0] = 3;
    ASSERT_EQ(w.popcount(), 2);
    p[0] = 0b1010111011011011010011111000011101010111101010101010110101011101ULL;
    ASSERT_EQ(w.popcount(), 39);
    p[1] = 0b1010111011011011010011111000011101010111101010101010110101011101ULL;
    ASSERT_EQ(w.popcount(), 39 * 2);
    p[2] = 0b1010111011011011010011111000011101010111101010101010110101011101ULL;
    p[3] = 0b1010111011011011010011111000011101010111101010101010110101011101ULL;
    ASSERT_EQ(w.popcount(), 39 * 4);
    p[1] = 1;
    ASSERT_EQ(w.popcount(), 39 * 3 + 1);
    p[0] = UINT64_MAX;
    p[1] = UINT64_MAX;
    p[2] = UINT64_MAX;
    p[3] = UINT64_MAX;
    ASSERT_EQ(w.popcount(), 256);
    for (size_t k = 0; k < 70000; k++) {
        p[0] = k;
        p[1] = k;
        p[2] = k;
        p[3] = k;
        auto expected = 0;
        size_t k2 = k;
        while (k2) {
            k2 &= k2 - 1;
            expected++;
        }
        ASSERT_EQ(w.popcount(), expected * 4);
    }
    p[0] = 0;
    p[1] = 0;
    p[2] = 0;
    p[3] = 0;
    for (size_t k = 0; k < 256; k++) {
        p[k >> 6] |= (1ULL << (k & 63));
        ASSERT_EQ(w.popcount(), k + 1);
    }
}
