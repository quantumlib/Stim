#ifndef ALIGNED_BITS256_H
#define ALIGNED_BITS256_H

#include <cstdint>

struct aligned_bits256 {
    uint64_t *data;
    size_t num_bits;

    ~aligned_bits256();
    explicit aligned_bits256(size_t num_bits);
    aligned_bits256(size_t num_bits, const uint64_t *other);
    aligned_bits256(aligned_bits256&& other) noexcept;
    aligned_bits256(const aligned_bits256& other);
    aligned_bits256& operator=(aligned_bits256&& other) noexcept;
    aligned_bits256& operator=(const aligned_bits256& other);

    bool get_bit(size_t k) const;
    void set_bit(size_t k, bool value);

    bool operator==(const aligned_bits256 &other) const;
    bool operator!=(const aligned_bits256 &other) const;
};

#endif
