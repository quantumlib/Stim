// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stim/py/numpy.pybind.h"

using namespace stim;
using namespace stim_pybind;

static pybind11::object transposed_simd_bit_table_to_numpy_uint8(
    const simd_bit_table<MAX_BITWORD_WIDTH> &table,
    size_t num_major_in,
    size_t num_minor_in,
    pybind11::object out_buffer) {
    size_t num_major_bytes_in = (num_major_in + 7) / 8;

    if (out_buffer.is_none()) {
        auto numpy = pybind11::module::import("numpy");
        out_buffer = numpy.attr("empty")(pybind11::make_tuple(num_minor_in, num_major_bytes_in), numpy.attr("uint8"));
    }

    if (!pybind11::isinstance<pybind11::array_t<uint8_t>>(out_buffer)) {
        throw std::invalid_argument("Output buffer wasn't a numpy.ndarray[np.uint8].");
    }
    auto buf = pybind11::cast<pybind11::array_t<uint8_t>>(out_buffer);
    if (buf.ndim() != 2) {
        throw std::invalid_argument("Output buffer wasn't two dimensional.");
    }
    if ((size_t)buf.shape(0) != num_minor_in || (size_t)buf.shape(1) != num_major_bytes_in) {
        std::stringstream ss;
        ss << "Expected output buffer to have shape=(" << num_minor_in << ", " << num_major_bytes_in << ")";
        ss << " but its shape is (" << buf.shape(0) << ", " << buf.shape(1) << ").";
        throw std::invalid_argument(ss.str());
    }

    if (num_major_in && num_minor_in) {
        auto stride = buf.strides(1);
        for (size_t minor_in = 0; minor_in < num_minor_in; minor_in++) {
            auto ptr = buf.mutable_data(minor_in, 0);
            for (size_t major_in = 0; major_in < num_major_in; major_in += 8) {
                uint8_t v = 0;
                for (size_t b = 0; b < 8 && major_in + b < num_major_in; b++) {
                    bool bit = table[major_in + b][minor_in];
                    v |= bit << b;
                }
                *ptr = v;
                ptr += stride;
            }
        }
    }

    return out_buffer;
}

static pybind11::object transposed_simd_bit_table_to_numpy_bool8(
    const simd_bit_table<MAX_BITWORD_WIDTH> &table,
    size_t num_major_in,
    size_t num_minor_in,
    pybind11::object out_buffer) {
    if (out_buffer.is_none()) {
        auto numpy = pybind11::module::import("numpy");
        out_buffer = numpy.attr("empty")(pybind11::make_tuple(num_minor_in, num_major_in), numpy.attr("bool_"));
    }

    if (!pybind11::isinstance<pybind11::array_t<bool>>(out_buffer)) {
        throw std::invalid_argument("Output buffer wasn't a numpy.ndarray[np.bool_].");
    }
    auto buf = pybind11::cast<pybind11::array_t<bool>>(out_buffer);
    if (buf.ndim() != 2) {
        throw std::invalid_argument("Output buffer wasn't two dimensional.");
    }
    if ((size_t)buf.shape(0) != num_minor_in || (size_t)buf.shape(1) != num_major_in) {
        std::stringstream ss;
        ss << "Expected output buffer to have shape=(" << num_minor_in << ", " << num_major_in << ")";
        ss << " but its shape is (" << buf.shape(0) << ", " << buf.shape(1) << ").";
        throw std::invalid_argument(ss.str());
    }

    if (num_major_in && num_minor_in) {
        auto stride = buf.strides(0);
        for (size_t major = 0; major < num_major_in; major++) {
            auto row = table[major];
            auto ptr = buf.mutable_data(0, major);
            for (size_t minor = 0; minor < num_minor_in; minor++) {
                *ptr = row[minor];
                ptr += stride;
            }
        }
    }

    return out_buffer;
}

static pybind11::object simd_bit_table_to_numpy_uint8(
    const simd_bit_table<MAX_BITWORD_WIDTH> &table, size_t num_major, size_t num_minor, pybind11::object out_buffer) {
    size_t num_minor_bytes = (num_minor + 7) / 8;
    if (out_buffer.is_none()) {
        auto numpy = pybind11::module::import("numpy");
        out_buffer = numpy.attr("empty")(pybind11::make_tuple(num_major, num_minor_bytes), numpy.attr("uint8"));
    }

    if (!pybind11::isinstance<pybind11::array_t<uint8_t>>(out_buffer)) {
        throw std::invalid_argument("Output buffer wasn't a numpy.ndarray[np.uint8].");
    }
    auto buf = pybind11::cast<pybind11::array_t<uint8_t>>(out_buffer);
    if (buf.ndim() != 2) {
        throw std::invalid_argument("Output buffer wasn't two dimensional.");
    }
    if ((size_t)buf.shape(0) != num_major || (size_t)buf.shape(1) != num_minor_bytes) {
        std::stringstream ss;
        ss << "Expected output buffer to have shape=(" << num_major << ", " << num_minor_bytes << ")";
        ss << " but its shape is (" << buf.shape(0) << ", " << buf.shape(1) << ").";
        throw std::invalid_argument(ss.str());
    }

    uint8_t mask = 0b11111111;
    if (num_minor & 7) {
        mask = (1 << (num_minor & 7)) - 1;
    }

    if (num_major && num_minor) {
        auto stride = buf.strides(1);
        if (stride == 1) {
            for (size_t major = 0; major < num_major; major++) {
                auto row = table[major];
                memcpy(buf.mutable_data(major, 0), row.u8, num_minor_bytes);
                *buf.mutable_data(major, num_minor_bytes - 1) &= mask;
            }
        } else {
            for (size_t major = 0; major < num_major; major++) {
                auto row = table[major];
                auto ptr = buf.mutable_data(major, 0);
                for (size_t minor = 0; minor < num_minor_bytes; minor += 1) {
                    *ptr = row.u8[minor];
                    ptr += stride;
                }
                *(ptr - stride) &= mask;
            }
        }
    }

    return out_buffer;
}

static pybind11::object simd_bit_table_to_numpy_bool8(
    const simd_bit_table<MAX_BITWORD_WIDTH> &table, size_t num_major, size_t num_minor, pybind11::object out_buffer) {
    if (out_buffer.is_none()) {
        auto numpy = pybind11::module::import("numpy");
        out_buffer = numpy.attr("empty")(pybind11::make_tuple(num_major, num_minor), numpy.attr("bool_"));
    }
    if (!pybind11::isinstance<pybind11::array_t<bool>>(out_buffer)) {
        throw std::invalid_argument("Output buffer wasn't a numpy.ndarray[np.bool_].");
    }
    auto buf = pybind11::cast<pybind11::array_t<bool>>(out_buffer);
    if (buf.ndim() != 2) {
        throw std::invalid_argument("Output buffer wasn't two dimensional.");
    }
    if ((size_t)buf.shape(0) != num_major || (size_t)buf.shape(1) != num_minor) {
        std::stringstream ss;
        ss << "Expected output buffer to have shape=(" << num_major << ", " << num_minor << ")";
        ss << " but its shape is (" << buf.shape(0) << ", " << buf.shape(1) << ").";
        throw std::invalid_argument(ss.str());
    }

    if (num_major && num_minor) {
        auto stride = buf.strides(1);
        for (size_t major = 0; major < num_major; major++) {
            auto row = table[major];
            auto out_ptr = buf.mutable_data(major, 0);
            for (size_t minor = 0; minor < num_minor; minor++) {
                *out_ptr = row[minor];
                out_ptr += stride;
            }
        }
    }

    return out_buffer;
}

pybind11::object stim_pybind::simd_bit_table_to_numpy(
    const simd_bit_table<MAX_BITWORD_WIDTH> &table,
    size_t num_major,
    size_t num_minor,
    bool bit_pack_result,
    bool transposed,
    pybind11::object out_buffer) {
    if (transposed) {
        if (bit_pack_result) {
            return transposed_simd_bit_table_to_numpy_uint8(table, num_major, num_minor, out_buffer);
        } else {
            return transposed_simd_bit_table_to_numpy_bool8(table, num_major, num_minor, out_buffer);
        }
    } else {
        if (bit_pack_result) {
            return simd_bit_table_to_numpy_uint8(table, num_major, num_minor, out_buffer);
        } else {
            return simd_bit_table_to_numpy_bool8(table, num_major, num_minor, out_buffer);
        }
    }
}

void stim_pybind::memcpy_bits_from_numpy_to_simd_bit_table(
    size_t num_major,
    size_t num_minor,
    const pybind11::object &src,
    stim::simd_bit_table<stim::MAX_BITWORD_WIDTH> &dst) {
    if (pybind11::isinstance<pybind11::array_t<uint8_t>>(src)) {
        auto arr = pybind11::cast<pybind11::array_t<uint8_t>>(src);
        size_t num_minor_bytes = (num_minor + 7) / 8;
        auto u = arr.unchecked();
        for (size_t major = 0; major < num_major; major++) {
            auto row = dst[major];
            for (size_t minor_byte = 0; minor_byte < num_minor_bytes; minor_byte++) {
                uint8_t v = u(major, minor_byte);
                row.u8[minor_byte] = v;
            }
            // Clear overwrite.
            for (size_t k = num_minor; k < num_minor_bytes * 8; k++) {
                row[k] = false;
            }
        }
    } else if (pybind11::isinstance<pybind11::array_t<bool>>(src)) {
        auto arr = pybind11::cast<pybind11::array_t<bool>>(src);
        auto u = arr.unchecked();
        for (size_t major = 0; major < num_major; major++) {
            auto row = dst[major];
            for (size_t minor = 0; minor < num_minor; minor++) {
                row[minor] = u(major, minor);
            }
        }
    } else {
        throw std::invalid_argument("Expected a 2-dimensional numpy array with dtype=np.uint8 or dtype=np.bool_");
    }
}

simd_bit_table<MAX_BITWORD_WIDTH> bit_packed_numpy_uint8_array_to_transposed_simd_table(
    const pybind11::array_t<uint8_t> &data_u8, size_t expected_bits_per_shot, size_t *num_shots_out) {
    if (data_u8.ndim() != 2) {
        throw std::invalid_argument("data must be a 2-dimensional numpy array with dtype=np.uint8 or dtype=np.bool_");
    }

    size_t num_shots = data_u8.shape(0);
    *num_shots_out = num_shots;

    size_t expected_bytes_per_shot = (expected_bits_per_shot + 7) / 8;
    size_t actual_bytes_per_shot = data_u8.shape(1);
    if (actual_bytes_per_shot != expected_bytes_per_shot) {
        std::stringstream ss;
        ss << "Expected " << expected_bits_per_shot << " bits per shot. ";
        ss << "Got bit packed data (dtype=np.uint8) but data.shape[1]=";
        ss << actual_bytes_per_shot << " != math.ceil(" << expected_bits_per_shot
           << " / 8)=" << expected_bytes_per_shot;
        throw std::invalid_argument(ss.str());
    }

    simd_bit_table<MAX_BITWORD_WIDTH> result(actual_bytes_per_shot * 8, num_shots);

    auto u = data_u8.unchecked();
    for (size_t a = 0; a < num_shots; a++) {
        for (size_t b = 0; b < actual_bytes_per_shot; b++) {
            uint8_t v = u(a, b);
            for (size_t k = 0; k < 8; k++) {
                result[b * 8 + k][a] |= ((v >> k) & 1) != 0;
            }
        }
    }

    return result;
}

simd_bit_table<MAX_BITWORD_WIDTH> bit_packed_numpy_bool8_array_to_transposed_simd_table(
    const pybind11::array_t<bool> &data_bool8, size_t expected_bits_per_shot, size_t *num_shots_out) {
    size_t num_shots = data_bool8.shape(0);
    *num_shots_out = num_shots;

    if (data_bool8.ndim() != 2) {
        throw std::invalid_argument("data must be a 2-dimensional numpy array with dtype=np.uint8 or dtype=np.bool_");
    }

    size_t actual_bits_per_shot = data_bool8.shape(1);
    if (actual_bits_per_shot != expected_bits_per_shot) {
        std::stringstream ss;
        ss << "Expected " << expected_bits_per_shot << " bits per shot. ";
        ss << "Got unpacked boolean data (dtype=np.bool_) but data.shape[1]=" << actual_bits_per_shot;
        throw std::invalid_argument(ss.str());
    }
    simd_bit_table<MAX_BITWORD_WIDTH> result(actual_bits_per_shot, num_shots);

    auto u = data_bool8.unchecked();
    for (size_t a = 0; a < num_shots; a++) {
        for (size_t b = 0; b < actual_bits_per_shot; b++) {
            result[b][a] |= u(a, b);
        }
    }

    return result;
}

simd_bit_table<MAX_BITWORD_WIDTH> stim_pybind::numpy_array_to_transposed_simd_table(
    const pybind11::object &data, size_t bits_per_shot, size_t *num_shots_out) {
    if (pybind11::isinstance<pybind11::array_t<uint8_t>>(data)) {
        return bit_packed_numpy_uint8_array_to_transposed_simd_table(
            pybind11::cast<pybind11::array_t<uint8_t>>(data), bits_per_shot, num_shots_out);
    } else if (pybind11::isinstance<pybind11::array_t<bool>>(data)) {
        return bit_packed_numpy_bool8_array_to_transposed_simd_table(
            pybind11::cast<pybind11::array_t<bool>>(data), bits_per_shot, num_shots_out);
    } else {
        throw std::invalid_argument("data must be a 2-dimensional numpy array with dtype=np.uint8 or dtype=np.bool_");
    }
}

pybind11::object bits_to_numpy_bool8(simd_bits_range_ref<MAX_BITWORD_WIDTH> bits, size_t num_bits) {
    bool *buffer = new bool[num_bits];
    for (size_t minor = 0; minor < num_bits; minor++) {
        buffer[minor] = bits[minor];
    }

    pybind11::capsule free_when_done(buffer, [](void *f) {
        delete[] reinterpret_cast<bool *>(f);
    });

    return pybind11::array_t<bool>({(pybind11::ssize_t)num_bits}, {(pybind11::ssize_t)1}, buffer, free_when_done);
}

pybind11::object bits_to_numpy_uint8_packed(simd_bits_range_ref<MAX_BITWORD_WIDTH> bits, size_t num_bits) {
    size_t num_bytes = (num_bits + 7) / 8;
    uint8_t *buffer = new uint8_t[num_bytes];
    memcpy(buffer, bits.u8, num_bytes);

    pybind11::capsule free_when_done(buffer, [](void *f) {
        delete[] reinterpret_cast<uint8_t *>(f);
    });

    return pybind11::array_t<uint8_t>({(pybind11::ssize_t)num_bytes}, {(pybind11::ssize_t)1}, buffer, free_when_done);
}

pybind11::object stim_pybind::simd_bits_to_numpy(
    simd_bits_range_ref<MAX_BITWORD_WIDTH> bits, size_t num_bits, bool bit_packed) {
    if (bit_packed) {
        return bits_to_numpy_uint8_packed(bits, num_bits);
    }
    return bits_to_numpy_bool8(bits, num_bits);
}

void stim_pybind::memcpy_bits_from_numpy_to_simd(
    size_t num_bits, const pybind11::object &src, simd_bits_range_ref<MAX_BITWORD_WIDTH> dst) {
    if (pybind11::isinstance<pybind11::array_t<uint8_t>>(src)) {
        auto arr = pybind11::cast<pybind11::array_t<uint8_t>>(src);
        if (arr.ndim() == 1) {
            size_t num_bytes = (num_bits + 7) / 8;
            auto u = arr.unchecked();
            for (size_t k = 0; k < num_bytes; k++) {
                uint8_t v = u(k);
                dst.u8[k] = v;
            }

            // Clear overwrite.
            for (size_t k = num_bits; k < num_bytes * 8; k++) {
                dst[k] = false;
            }
            return;
        }
    } else if (pybind11::isinstance<pybind11::array_t<bool>>(src)) {
        auto arr = pybind11::cast<pybind11::array_t<bool>>(src);
        if (arr.ndim() == 1) {
            auto u = arr.unchecked();
            for (size_t k = 0; k < num_bits; k++) {
                dst[k] = u(k);
            }
            return;
        }
    }

    throw std::invalid_argument("Expected a 1-dimensional numpy array with dtype=np.uint8 or dtype=np.bool_");
}
