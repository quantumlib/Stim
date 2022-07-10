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

#include "stim/io/read_write.pybind.h"

#include "stim/circuit/circuit.h"
#include "stim/io/measure_record_reader.h"
#include "stim/io/measure_record_writer.h"
#include "stim/io/raii_file.h"
#include "stim/mem/simd_bits.h"
#include "stim/py/base.pybind.h"
#include "stim/simulators/measurements_to_detection_events.pybind.h"

using namespace stim;
using namespace stim_pybind;

pybind11::object transposed_simd_bit_table_to_numpy_uint8(
    const simd_bit_table &table, size_t bits_per_shot, size_t num_shots) {
    std::vector<uint8_t> bytes;
    bytes.resize(bits_per_shot * num_shots);
    size_t bytes_per_shot = (bits_per_shot + 7) / 8;
    for (size_t shot_index = 0; shot_index < num_shots; shot_index++) {
        size_t shot_offset = bytes_per_shot * shot_index;
        for (size_t o_index = 0; o_index < bits_per_shot; o_index += 8) {
            for (size_t b = 0; b < 8; b++) {
                bool bit = table[o_index + b][shot_index];
                bytes[shot_offset + o_index / 8] |= bit << b;
            }
        }
    }
    void *ptr = bytes.data();
    pybind11::ssize_t itemsize = sizeof(uint8_t);
    std::vector<pybind11::ssize_t> shape{(pybind11::ssize_t)num_shots, (pybind11::ssize_t)bytes_per_shot};
    std::vector<pybind11::ssize_t> stride{(pybind11::ssize_t)bytes_per_shot, 1};
    const std::string &np_format = pybind11::format_descriptor<uint8_t>::value;
    bool readonly = true;
    return pybind11::array_t<uint8_t>(pybind11::buffer_info(ptr, itemsize, np_format, 2, shape, stride, readonly));
}

pybind11::object transposed_simd_bit_table_to_numpy_bool8(
    const simd_bit_table &table, size_t bits_per_shot, size_t num_shots) {
    std::vector<uint8_t> bytes;
    bytes.resize(bits_per_shot * num_shots);
    size_t k = 0;
    for (size_t shot_index = 0; shot_index < num_shots; shot_index++) {
        for (size_t o_index = 0; o_index < bits_per_shot; o_index++) {
            bytes[k++] = table[o_index][shot_index];
        }
    }
    void *ptr = bytes.data();
    pybind11::ssize_t itemsize = sizeof(uint8_t);
    std::vector<pybind11::ssize_t> shape{(pybind11::ssize_t)num_shots, (pybind11::ssize_t)bits_per_shot};
    std::vector<pybind11::ssize_t> stride{(pybind11::ssize_t)bits_per_shot, 1};
    const std::string &format = pybind11::format_descriptor<bool>::value;
    bool readonly = true;
    return pybind11::array_t<bool>(pybind11::buffer_info(ptr, itemsize, format, 2, shape, stride, readonly));
}

pybind11::object stim_pybind::transposed_simd_bit_table_to_numpy(
    const simd_bit_table &table, size_t bits_per_shot, size_t num_shots, bool bit_pack_result) {
    if (bit_pack_result) {
        return transposed_simd_bit_table_to_numpy_uint8(table, bits_per_shot, num_shots);
    } else {
        return transposed_simd_bit_table_to_numpy_bool8(table, bits_per_shot, num_shots);
    }
}

pybind11::object read_shot_data_file(
    const char *path,
    const char *format,
    const pybind11::handle &num_measurements,
    const pybind11::handle &num_detectors,
    const pybind11::handle &num_observables,
    bool bit_pack) {
    auto parsed_format = format_to_enum(format);

    if (num_measurements.is_none() && num_detectors.is_none() && num_observables.is_none()) {
        throw std::invalid_argument("Must specify num_measurements, num_detectors, num_observables.");
    }
    size_t nm = num_measurements.is_none() ? 0 : pybind11::cast<size_t>(num_measurements);
    size_t nd = num_detectors.is_none() ? 0 : pybind11::cast<size_t>(num_detectors);
    size_t no = num_observables.is_none() ? 0 : pybind11::cast<size_t>(num_observables);

    std::vector<uint8_t> full_buffer;
    size_t num_bits_per_shot = nm + nd + no;
    size_t num_bytes_per_shot = (num_bits_per_shot + 7) / 8;
    size_t num_shots = 0;
    {
        RaiiFile f(path, "r");
        auto reader = MeasureRecordReader::make(f.f, parsed_format, nm, nd, no);

        simd_bits buffer(num_bits_per_shot);
        while (true) {
            if (!reader->start_and_read_entire_record(buffer)) {
                break;
            }
            full_buffer.insert(full_buffer.end(), buffer.u8, buffer.u8 + num_bytes_per_shot);
            num_shots += 1;
        }
    }

    if (bit_pack) {
        void *ptr = full_buffer.data();
        pybind11::ssize_t itemsize = sizeof(uint8_t);
        std::vector<pybind11::ssize_t> shape{(pybind11::ssize_t)num_shots, (pybind11::ssize_t)num_bytes_per_shot};
        std::vector<pybind11::ssize_t> stride{(pybind11::ssize_t)num_bytes_per_shot, 1};
        const std::string &np_format = pybind11::format_descriptor<uint8_t>::value;
        bool readonly = true;
        return pybind11::array_t<uint8_t>(pybind11::buffer_info(ptr, itemsize, np_format, 2, shape, stride, readonly));
    } else {
        std::vector<uint8_t> unpacked_buffer;
        for (size_t s = 0; s < num_shots; s++) {
            for (size_t k = 0; k < num_bits_per_shot; k++) {
                auto bi = (s * num_bytes_per_shot + (k / 8));
                unpacked_buffer.push_back((full_buffer[bi] >> (k % 8)) & 1);
            }
        }

        void *ptr = unpacked_buffer.data();
        pybind11::ssize_t itemsize = sizeof(uint8_t);
        std::vector<pybind11::ssize_t> shape{(pybind11::ssize_t)num_shots, (pybind11::ssize_t)num_bits_per_shot};
        std::vector<pybind11::ssize_t> stride{(pybind11::ssize_t)num_bits_per_shot, 1};
        const std::string &np_format = pybind11::format_descriptor<bool>::value;
        bool readonly = true;
        return pybind11::array_t<bool>(pybind11::buffer_info(ptr, itemsize, np_format, 2, shape, stride, readonly));
    }
}

simd_bit_table bit_packed_numpy_uint8_array_to_transposed_simd_table(
    const pybind11::array_t<uint8_t> &data_u8, size_t expected_bits_per_shot, size_t *num_shots_out) {
    size_t num_shots = data_u8.shape(0);
    *num_shots_out = num_shots;

    if (data_u8.ndim() != 2) {
        throw std::invalid_argument("data must be a 2-dimensional numpy array with dtype=np.uint8 or dtype=np.bool8");
    }

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

    simd_bit_table result(actual_bytes_per_shot * 8, num_shots);

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

simd_bit_table bit_packed_numpy_bool8_array_to_transposed_simd_table(
    const pybind11::array_t<bool> &data_bool8, size_t expected_bits_per_shot, size_t *num_shots_out) {
    size_t num_shots = data_bool8.shape(0);
    *num_shots_out = num_shots;

    if (data_bool8.ndim() != 2) {
        throw std::invalid_argument("data must be a 2-dimensional numpy array with dtype=np.uint8 or dtype=np.bool8");
    }

    size_t actual_bits_per_shot = data_bool8.shape(1);
    if (actual_bits_per_shot != expected_bits_per_shot) {
        std::stringstream ss;
        ss << "Expected " << expected_bits_per_shot << " bits per shot. ";
        ss << "Got unpacked boolean data (dtype=np.bool8) but data.shape[1]=" << actual_bits_per_shot;
        throw std::invalid_argument(ss.str());
    }
    simd_bit_table result(actual_bits_per_shot, num_shots);

    auto u = data_bool8.unchecked();
    for (size_t a = 0; a < num_shots; a++) {
        for (size_t b = 0; b < actual_bits_per_shot; b++) {
            result[b][a] |= u(a, b);
        }
    }

    return result;
}

simd_bit_table stim_pybind::numpy_array_to_transposed_simd_table(
    const pybind11::object &data, size_t bits_per_shot, size_t *num_shots_out) {
    if (pybind11::isinstance<pybind11::array_t<uint8_t>>(data)) {
        return bit_packed_numpy_uint8_array_to_transposed_simd_table(
            pybind11::cast<pybind11::array_t<uint8_t>>(data), bits_per_shot, num_shots_out);
    } else if (pybind11::isinstance<pybind11::array_t<bool>>(data)) {
        return bit_packed_numpy_bool8_array_to_transposed_simd_table(
            pybind11::cast<pybind11::array_t<bool>>(data), bits_per_shot, num_shots_out);
    } else {
        throw std::invalid_argument("data must be a 2-dimensional numpy array with dtype=np.uint8 or dtype=np.bool8");
    }
}

void write_shot_data_file(
    const pybind11::object &data,
    const char *path,
    const char *format,
    const pybind11::handle &num_measurements,
    const pybind11::handle &num_detectors,
    const pybind11::handle &num_observables) {
    auto parsed_format = format_to_enum(format);

    if (num_measurements.is_none() && num_detectors.is_none() && num_observables.is_none()) {
        throw std::invalid_argument("Must specify num_measurements, num_detectors, num_observables.");
    }
    size_t nm = num_measurements.is_none() ? 0 : pybind11::cast<size_t>(num_measurements);
    size_t nd = num_detectors.is_none() ? 0 : pybind11::cast<size_t>(num_detectors);
    size_t no = num_observables.is_none() ? 0 : pybind11::cast<size_t>(num_observables);
    if (nm != 0 && (nd != 0 || no != 0)) {
        throw std::invalid_argument("num_measurements and (num_detectors or num_observables)");
    }
    size_t num_bits_per_shot = nm + nd + no;
    size_t num_shots;
    simd_bit_table buffer = numpy_array_to_transposed_simd_table(data, num_bits_per_shot, &num_shots);

    RaiiFile f(path, "w");
    simd_bits unused(0);
    write_table_data(
        f.f,
        num_shots,
        num_bits_per_shot,
        unused,
        buffer,
        parsed_format,
        nm == 0 ? 'D' : 'M',
        nm == 0 ? 'L' : 'M',
        nm + nd);
}

void stim_pybind::pybind_read_write(pybind11::module &m) {
    m.def(
        "read_shot_data_file",
        &read_shot_data_file,
        pybind11::kw_only(),
        pybind11::arg("path"),
        pybind11::arg("format"),
        pybind11::arg("num_measurements") = pybind11::none(),
        pybind11::arg("num_detectors") = pybind11::none(),
        pybind11::arg("num_observables") = pybind11::none(),
        pybind11::arg("bit_pack") = false,
        clean_doc_string(u8R"DOC(
            Reads shot data, such as measurement samples, from a file.
            @signature def read_shot_data_file(*, path: str, format: str, num_measurements: int = 0, num_detectors: int = 0, num_observables: int = 0, bit_pack: bool = False) -> np.ndarray:

            Args:
                path: The path to the file to read the data from.
                format: The format that the data is stored in, such as 'b8'.
                    See https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
                bit_pack: Defaults to false. Determines whether the result is a bool8 numpy array
                    with one bit per byte, or a uint8 numpy array with 8 bits per byte.
                num_measurements: How many measurements there are per shot.
                num_detectors: How many detectors there are per shot.
                num_observables: How many observables there are per shot.
                    Note that this only refers to observables *stored in the file*, not to
                    observables from the original circuit that was sampled.

            Returns:
                A numpy array containing the loaded data.

                If bit_pack=False:
                    dtype = np.bool8
                    shape = (num_shots, num_measurements + num_detectors + num_observables)
                    bit b from shot s is at result[s, b]
                If bit_pack=True:
                    dtype = np.uint8
                    shape = (num_shots, math.ceil((num_measurements + num_detectors + num_observables) / 8))
                    bit b from shot s is at result[s, b // 8] & (1 << (b % 8))

            Examples:
                >>> import stim
                >>> import pathlib
                >>> import tempfile
                >>> with tempfile.TemporaryDirectory() as d:
                ...     path = pathlib.Path(d) / 'shots'
                ...     with open(path, 'w') as f:
                ...         print("0000", file=f)
                ...         print("0101", file=f)
                ...
                ...     read = stim.read_shot_data_file(
                ...         path=str(path),
                ...         format='01',
                ...         num_measurements=4)
                >>> read
                array([[False, False, False, False],
                       [False,  True, False,  True]])
        )DOC")
            .data());

    m.def(
        "write_shot_data_file",
        &write_shot_data_file,
        pybind11::kw_only(),
        pybind11::arg("data"),
        pybind11::arg("path"),
        pybind11::arg("format"),
        pybind11::arg("num_measurements") = pybind11::none(),
        pybind11::arg("num_detectors") = pybind11::none(),
        pybind11::arg("num_observables") = pybind11::none(),
        clean_doc_string(u8R"DOC(
            Writes shot data, such as measurement samples, to a file.

            Args:
                data: The data to write to the file. This must be a numpy array. The dtype
                    of the array determines whether or not the data is bit packed, and the
                    shape must match the bits per shot.

                    dtype=np.bool8: Not bit packed. Shape must be (num_shots, num_measurements + num_detectors + num_observables).
                    dtype=np.uint8: Yes bit packed. Shape must be (num_shots, math.ceil((num_measurements + num_detectors + num_observables) / 8)).
                path: The path to the file to write the data to.
                format: The format that the data is stored in, such as 'b8'.
                    See https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
                num_measurements: How many measurements there are per shot.
                num_detectors: How many detectors there are per shot.
                num_observables: How many observables there are per shot.
                    Note that this only refers to observables *in the given shot data*, not to
                    observables from the original circuit that was sampled.

            Examples:
                >>> import stim
                >>> import pathlib
                >>> import tempfile
                >>> import numpy as np
                >>> with tempfile.TemporaryDirectory() as d:
                ...     path = pathlib.Path(d) / 'shots'
                ...     shot_data = np.array([
                ...         [0, 1, 0],
                ...         [0, 1, 1],
                ...     ], dtype=np.bool8)
                ...
                ...     stim.write_shot_data_file(
                ...         path=str(path),
                ...         data=shot_data,
                ...         format='01',
                ...         num_measurements=3)
                ...
                ...     with open(path) as f:
                ...         written = f.read()
                >>> written
                '010\n011\n'
        )DOC")
            .data());
}
