#ifndef SIMD_BITS_H
#define SIMD_BITS_H

#include <cstdint>
#include <immintrin.h>
#include <random>
#include "bit_ref.h"
#include "simd_bits_range_ref.h"

/// Densely packed bits, allocated with alignment and padding enabling SIMD operations.
struct simd_bits {
    size_t num_simd_words;
    union {
        uint8_t *u8;
        uint16_t *u16;
        uint32_t *u32;
        uint64_t *u64;
        __m128i *u128;
        __m256i *u256;
    };

    /// Constructs a zero-initialized simd_bits with at least the given number of bits.
    explicit simd_bits(size_t min_bits);
    /// Returns a simd_bits with at least the given number of bits, with bits up to the given number of bits randomized.
    /// Padding bits beyond the minimum number of bits are not randomized.
    static simd_bits random(size_t min_bits, std::mt19937& rng);

    /// Randomizes the contents of this simd_bits using the given random number generator, up to the given bit position.
    void randomize(size_t num_bits, std::mt19937& rng);

    /// Frees allocated bits.
    ~simd_bits();

    /// Copy constructor.
    simd_bits(const simd_bits& other);
    /// Copy constructor from range reference.
    simd_bits(const simd_bits_range_ref other);
    /// Move constructor.
    simd_bits(simd_bits&& other) noexcept;

    /// Copy-assignment.
    simd_bits& operator=(const simd_bits& other);
    /// Copy-assignment from range reference.
    simd_bits& operator=(const simd_bits_range_ref other);
    /// Move-assignment.
    simd_bits& operator=(simd_bits&& other) noexcept;

    /// Returns a reference to the bit at offset k.
    bit_ref operator[](size_t k);
    /// Returns a const reference to the bit at offset k.
    const bit_ref operator[](size_t k) const;

    operator simd_bits_range_ref();
    operator const simd_bits_range_ref() const;

    simd_bits_range_ref word_range_ref(size_t word_offset, size_t sub_num_simd_words);
    const simd_bits_range_ref word_range_ref(size_t word_offset, size_t sub_num_simd_words) const;
    void clear();

    bool operator==(const simd_bits_range_ref &other) const;
    bool operator!=(const simd_bits_range_ref &other) const;
    simd_bits &operator^=(const simd_bits_range_ref other);
    bool not_zero() const;

    inline size_t num_u64_padded() const { return num_simd_words << 2; }
    inline size_t num_u32_padded() const { return num_simd_words << 3; }
    inline size_t num_u16_padded() const { return num_simd_words << 4; }
    inline size_t num_u8_padded() const { return num_simd_words << 5; }
    inline size_t num_bits_padded() const { return num_simd_words << 8; }

    std::string str() const;
};

#endif
