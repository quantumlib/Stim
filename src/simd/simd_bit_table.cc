#include <algorithm>
#include <cstring>
#include <immintrin.h>
#include <random>

#include "simd_bit_table.h"
#include "simd_util.h"

simd_bit_table::simd_bit_table(size_t num_bits_major, size_t num_bits_minor) :
    num_bits_major(num_bits_major),
    num_words_minor(ceil256(num_bits_minor) >> 8),
    data(ceil256(num_bits_minor) * num_bits_major) {
}

simd_bits_range_ref simd_bit_table::operator[](size_t major_index) {
    return data.word_range_ref(major_index * num_words_minor, num_words_minor);
}

const simd_bits_range_ref simd_bit_table::operator[](size_t major_index) const {
    return data.word_range_ref(major_index * num_words_minor, num_words_minor);
}

void simd_bit_table::clear() {
    data.clear();
}

bool simd_bit_table::operator==(const simd_bit_table &other) const {
    return num_words_minor == other.num_words_minor && num_bits_major == other.num_bits_major && data == other.data;
}
bool simd_bit_table::operator!=(const simd_bit_table &other) const {
    return !(*this == other);
}
