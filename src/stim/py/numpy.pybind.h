#ifndef _STIM_PY_NUMPY_PYBIND_H
#define _STIM_PY_NUMPY_PYBIND_H

#include "stim/py/base.pybind.h"
#include "stim/mem/simd_bit_table.h"

namespace stim_pybind {

stim::simd_bit_table<stim::MAX_BITWORD_WIDTH> numpy_array_to_transposed_simd_table(
    const pybind11::object &data, size_t expected_bits_per_shot, size_t *num_shots_out);

pybind11::object transposed_simd_bit_table_to_numpy(
    const stim::simd_bit_table<stim::MAX_BITWORD_WIDTH> &table, size_t num_major_in, size_t num_minor_in, bool bit_pack_result);

pybind11::object simd_bit_table_to_numpy(
    const stim::simd_bit_table<stim::MAX_BITWORD_WIDTH> &table,
    size_t num_major,
    size_t num_minor,
    bool bit_pack_result);

void memcpy_bits_from_numpy_to_simd_bit_table(
    size_t num_major,
    size_t num_minor,
    const pybind11::object &src,
    stim::simd_bit_table<stim::MAX_BITWORD_WIDTH> &dst);

pybind11::object simd_bits_to_numpy(stim::simd_bits_range_ref<stim::MAX_BITWORD_WIDTH> bits, size_t num_bits, bool bit_packed);
void memcpy_bits_from_numpy_to_simd(size_t num_bits, const pybind11::object &src, stim::simd_bits_range_ref<stim::MAX_BITWORD_WIDTH> dst);

}

#endif
