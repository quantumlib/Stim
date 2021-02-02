#include "simd_util.h"

struct emu_u128 {
    uint64_t a;
    uint64_t b;
};

struct simd_word {
    union {
        uint64_t u64[2];
        uint8_t u8[8];
        emu_u128 u128[1];
    };

    inline constexpr simd_word() : u64{} {
    }
    inline constexpr simd_word(uint64_t v1, uint64_t v2) : u64{v1, v2} {
    }

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

    inline operator bool() const {  // NOLINT(hicpp-explicit-conversions)
        return u64[0] | u64[1];
    }

    inline simd_word &operator^=(const simd_word &other) {
        u64[0] ^= other.u64[0];
        u64[1] ^= other.u64[1];
        return *this;
    }

    inline simd_word &operator&=(const simd_word &other) {
        u64[0] &= other.u64[0];
        u64[1] &= other.u64[1];
        return *this;
    }

    inline simd_word &operator|=(const simd_word &other) {
        u64[0] |= other.u64[0];
        u64[1] |= other.u64[1];
        return *this;
    }

    inline simd_word operator^(const simd_word &other) const {
        return {u64[0] ^ other.u64[0], u64[1] ^ other.u64[1]};
    }

    inline simd_word operator&(const simd_word &other) const {
        return {u64[0] & other.u64[0], u64[1] & other.u64[1]};
    }

    inline simd_word operator|(const simd_word &other) const {
        return {u64[0] | other.u64[0], u64[1] | other.u64[1]};
    }

    inline simd_word andnot(const simd_word &other) const {
        return {~u64[0] & other.u64[0], ~u64[1] & other.u64[1]};
    }

    inline uint16_t popcount() const {
        return popcnt64(u64[0]) + popcnt64(u64[1]);
    }

    inline simd_word leftshift_tile64(uint8_t offset) {
        return {u64[0] << offset, u64[1] << offset};
    }

    inline simd_word rightshift_tile64(uint8_t offset) {
        return {u64[0] >> offset, u64[1] >> offset};
    }

    /// For each 128 bit word pair between the two registers, the byte order goes from this:
    /// [a0 a1 a2 a3 ... a14 a15] [b0 b1 b2 b3 ... b14 b15]
    /// to this:
    /// [a0 b0 a1 b1 ...  a7  b7] [a8 b8 a9 b9 ... a15 b15]
    inline void do_interleave8_tile128(simd_word &other) {
        auto a1 = spread_bytes_32_to_64((uint32_t)u64[0]);
        auto a2 = spread_bytes_32_to_64((uint32_t)(u64[0] >> 32));
        auto b1 = spread_bytes_32_to_64((uint32_t)u64[1]);
        auto b2 = spread_bytes_32_to_64((uint32_t)(u64[1] >> 32));
        auto c1 = spread_bytes_32_to_64((uint32_t)other.u64[0]);
        auto c2 = spread_bytes_32_to_64((uint32_t)(other.u64[0] >> 32));
        auto d1 = spread_bytes_32_to_64((uint32_t)other.u64[1]);
        auto d2 = spread_bytes_32_to_64((uint32_t)(other.u64[1] >> 32));
        u64[0] = a1 | (c1 << 8);
        u64[1] = a2 | (c2 << 8);
        other.u64[0] = b1 | (d1 << 8);
        other.u64[1] = b2 | (d2 << 8);
    }
};
