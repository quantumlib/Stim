#include "simd_bits.h"

struct simd_bit_table {
    size_t num_bits_major;
    size_t num_words_minor;
    simd_bits data;

    simd_bit_table(size_t num_bits_major, size_t num_bits_minor);
    simd_range_ref operator[](size_t major_index);
    const simd_range_ref operator[](size_t major_index) const;
    void clear();
};
