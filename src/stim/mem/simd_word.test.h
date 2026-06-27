/// Test utilities for working with various sizes of simd words.

#include "gtest/gtest.h"

#define TEST_EACH_WORD_SIZE_UP_TO_64(test_suite, test_name, ...) \
    TEST(test_suite, test_name##_64) {                           \
        constexpr size_t W = 64;                                 \
        __VA_ARGS__                                              \
    }

#define TEST_EACH_WORD_SIZE_UP_TO_128(test_suite, test_name, ...) \
    TEST(test_suite, test_name##_128) {                           \
        constexpr size_t W = 128;                                 \
        __VA_ARGS__                                               \
    }                                                             \
    TEST(test_suite, test_name##_64) {                            \
        constexpr size_t W = 64;                                  \
        __VA_ARGS__                                               \
    }

#define TEST_EACH_WORD_SIZE_UP_TO_256(test_suite, test_name, ...) \
    TEST(test_suite, test_name##_256) {                           \
        constexpr size_t W = 256;                                 \
        __VA_ARGS__                                               \
    }                                                             \
    TEST(test_suite, test_name##_128) {                           \
        constexpr size_t W = 128;                                 \
        __VA_ARGS__                                               \
    }                                                             \
    TEST(test_suite, test_name##_64) {                            \
        constexpr size_t W = 64;                                  \
        __VA_ARGS__                                               \
    }

#if __AVX2__
#define TEST_EACH_WORD_SIZE_W(test_suite, test_name, ...) \
    TEST_EACH_WORD_SIZE_UP_TO_256(test_suite, test_name, __VA_ARGS__)
#elif __SSE2__
#define TEST_EACH_WORD_SIZE_W(test_suite, test_name, ...) \
    TEST_EACH_WORD_SIZE_UP_TO_128(test_suite, test_name, __VA_ARGS__)
#else
#define TEST_EACH_WORD_SIZE_W(test_suite, test_name, ...) \
    TEST_EACH_WORD_SIZE_UP_TO_64(test_suite, test_name, __VA_ARGS__)
#endif
