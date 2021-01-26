#include "simd_util.h"

struct simd_word {
    uint64_t val[2];

    constexpr inline static simd_word tile8(uint8_t pattern) {
        uint64_t result = pattern;
        result |= result << 8;
        result |= result << 16;
        result |= result << 32;
        return {result, result};
    }

    constexpr inline static simd_word tile16(uint16_t pattern) {
        uint64_t result = pattern;
        result |= result << 16;
        result |= result << 32;
        return {result, result};
    }

    constexpr inline static simd_word tile32(uint32_t pattern) {
        uint64_t result = pattern;
        result |= result << 32;
        return {result, result};
    }

    constexpr inline static simd_word tile64(uint64_t pattern) {
        return {pattern, pattern};
    }

    inline operator bool() const { // NOLINT(hicpp-explicit-conversions)
        return val[0] | val[1];
    }

    inline simd_word &operator^=(const simd_word &other) {
        val[0] ^= other.val[0];
        val[1] ^= other.val[1];
        return *this;
    }

    inline simd_word &operator&=(const simd_word &other) {
        val[0] &= other.val[0];
        val[1] &= other.val[1];
        return *this;
    }

    inline simd_word &operator|=(const simd_word &other) {
        val[0] |= other.val[0];
        val[1] |= other.val[1];
        return *this;
    }

    inline simd_word operator^(const simd_word &other) const {
        return {val[0] ^ other.val[0], val[1] ^ other.val[1]};
    }

    inline simd_word operator&(const simd_word &other) const {
        return {val[0] & other.val[0], val[1] & other.val[1]};
    }

    inline simd_word operator|(const simd_word &other) const {
        return {val[0] | other.val[0], val[1] | other.val[1]};
    }

    inline simd_word andnot(const simd_word &other) const {
        return {~val[0] & other.val[0], ~val[1] & other.val[1]};
    }

    inline uint16_t popcount() const {
        return popcnt64(val[0]) + popcnt64(val[1]);
    }

    inline simd_word leftshift_tile64(uint8_t offset) {
        return {val[0] << offset, val[1] << offset};
    }

    inline simd_word rightshift_tile64(uint8_t offset) {
        return {val[0] >> offset, val[1] >> offset};
    }

    /// For each 128 bit word pair between the two registers, the byte order goes from this:
    /// [a0 a1 a2 a3 ... a14 a15] [b0 b1 b2 b3 ... b14 b15]
    /// to this:
    /// [a0 b0 a1 b1 ...  a7  b7] [a8 b8 a9 b9 ... a15 b15]
    inline void do_interleave8_tile128(simd_word &other) {
        auto a1 = spread_bytes_32_to_64((uint32_t)val[0]);
        auto a2 = spread_bytes_32_to_64((uint32_t)(val[0] >> 32));
        auto b1 = spread_bytes_32_to_64((uint32_t)val[1]);
        auto b2 = spread_bytes_32_to_64((uint32_t)(val[1] >> 32));
        auto c1 = spread_bytes_32_to_64((uint32_t)other.val[0]);
        auto c2 = spread_bytes_32_to_64((uint32_t)(other.val[0] >> 32));
        auto d1 = spread_bytes_32_to_64((uint32_t)other.val[1]);
        auto d2 = spread_bytes_32_to_64((uint32_t)(other.val[1] >> 32));
        val[0] = a1 | (c1 << 8);
        val[1] = a2 | (c2 << 8);
        other.val[0] = b1 | (d1 << 8);
        other.val[1] = b2 | (d2 << 8);
    }
};
