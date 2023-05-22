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
#include "stim/py/numpy.pybind.h"

using namespace stim;
using namespace stim_pybind;

std::string path_to_string(const pybind11::object &path_obj) {
    try {
        return pybind11::cast<std::string>(path_obj);
    } catch (pybind11::cast_error &ex) {
    }

    auto py_path = pybind11::module::import("pathlib").attr("Path");
    if (pybind11::isinstance(path_obj, py_path)) {
        return pybind11::cast<std::string>(pybind11::str(path_obj));
    }

    throw std::invalid_argument("Not a str or pathlib.Path: " + pybind11::cast<std::string>(pybind11::str(path_obj)));
}

pybind11::object buffer_slice_to_numpy(
    size_t num_shots,
    size_t shot_num_stride_bytes,
    size_t shot_bit_copy_offset,
    size_t shot_bit_copy_length,
    bool bit_packed,
    SpanRef<const uint8_t> immovable_buffer) {
    size_t num_bytes_copied_per_shot = (shot_bit_copy_length + 7) / 8;
    if (bit_packed) {
        uint8_t *buffer = new uint8_t[num_bytes_copied_per_shot * num_shots];
        memset(buffer, 0, num_bytes_copied_per_shot * num_shots);
        for (size_t s = 0; s < num_shots; s++) {
            size_t t = s * num_bytes_copied_per_shot * 8;
            for (size_t k = 0; k < shot_bit_copy_length; k++) {
                auto k2 = k + shot_bit_copy_offset;
                auto bi = s * shot_num_stride_bytes + k2 / 8;
                bool bit = (immovable_buffer[bi] >> (k2 % 8)) & 1;
                buffer[t / 8] |= bit << (t & 7);
                t++;
            }
        }

        pybind11::capsule free_when_done(buffer, [](void *f) {
            delete[] reinterpret_cast<uint8_t *>(f);
        });

        return pybind11::array_t<uint8_t>(
            {(pybind11::ssize_t)num_shots, (pybind11::ssize_t)num_bytes_copied_per_shot},
            {(pybind11::ssize_t)num_bytes_copied_per_shot, (pybind11::ssize_t)1},
            buffer,
            free_when_done);
    } else {
        bool *buffer = new bool[shot_bit_copy_length * num_shots];
        size_t t = 0;
        for (size_t s = 0; s < num_shots; s++) {
            for (size_t k = 0; k < shot_bit_copy_length; k++) {
                auto k2 = k + shot_bit_copy_offset;
                auto bi = s * shot_num_stride_bytes + k2 / 8;
                bool bit = (immovable_buffer[bi] >> (k2 % 8)) & 1;
                buffer[t++] = bit;
            }
        }

        pybind11::capsule free_when_done(buffer, [](void *f) {
            delete[] reinterpret_cast<bool *>(f);
        });

        return pybind11::array_t<bool>(
            {(pybind11::ssize_t)num_shots, (pybind11::ssize_t)shot_bit_copy_length},
            {(pybind11::ssize_t)shot_bit_copy_length, (pybind11::ssize_t)1},
            buffer,
            free_when_done);
    }
}

pybind11::object read_shot_data_file(
    const pybind11::object &path_obj,
    const char *format,
    const pybind11::handle &num_measurements,
    const pybind11::handle &num_detectors,
    const pybind11::handle &num_observables,
    bool separate_observables,
    bool bit_packed,
    bool _legacy_bit_pack) {
    auto path = path_to_string(path_obj);
    auto parsed_format = format_to_enum(format);
    bit_packed |= _legacy_bit_pack;

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
        RaiiFile f(path.c_str(), "rb");
        auto reader = MeasureRecordReader<MAX_BITWORD_WIDTH>::make(f.f, parsed_format, nm, nd, no);

        simd_bits<MAX_BITWORD_WIDTH> buffer(num_bits_per_shot);
        while (true) {
            if (!reader->start_and_read_entire_record(buffer)) {
                break;
            }
            full_buffer.insert(full_buffer.end(), buffer.u8, buffer.u8 + num_bytes_per_shot);
            num_shots += 1;
        }
    }

    if (separate_observables) {
        pybind11::object dets =
            buffer_slice_to_numpy(num_shots, num_bytes_per_shot, 0, num_bits_per_shot - no, bit_packed, full_buffer);
        pybind11::object obs =
            buffer_slice_to_numpy(num_shots, num_bytes_per_shot, num_bits_per_shot - no, no, bit_packed, full_buffer);
        return pybind11::make_tuple(dets, obs);
    }

    return buffer_slice_to_numpy(num_shots, num_bytes_per_shot, 0, num_bits_per_shot, bit_packed, full_buffer);
}

void write_shot_data_file(
    const pybind11::object &data,
    const pybind11::object &path_obj,
    const char *format,
    const pybind11::handle &num_measurements,
    const pybind11::handle &num_detectors,
    const pybind11::handle &num_observables) {
    auto parsed_format = format_to_enum(format);
    auto path = path_to_string(path_obj);

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
    simd_bit_table<MAX_BITWORD_WIDTH> buffer =
        numpy_array_to_transposed_simd_table(data, num_bits_per_shot, &num_shots);

    RaiiFile f(path.c_str(), "wb");
    simd_bits<MAX_BITWORD_WIDTH> unused(0);
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
        pybind11::arg("separate_observables") = false,
        pybind11::arg("bit_packed") = false,
        pybind11::arg("bit_pack") = false,  // Legacy argument for backwards compat.
        clean_doc_string(R"DOC(
            Reads shot data, such as measurement samples, from a file.
            @overload def read_shot_data_file(*, path: Union[str, pathlib.Path], format: Union[str, 'Literal["01", "b8", "r8", "ptb64", "hits", "dets"]'], bit_packed: bool = False, num_measurements: int = 0, num_detectors: int = 0, num_observables: int = 0) -> np.ndarray:
            @overload def read_shot_data_file(*, path: Union[str, pathlib.Path], format: Union[str, 'Literal["01", "b8", "r8", "ptb64", "hits", "dets"]'], bit_packed: bool = False, num_measurements: int = 0, num_detectors: int = 0, num_observables: int = 0, separate_observables: 'Literal[True]') -> Tuple[np.ndarray, np.ndarray]:
            @signature def read_shot_data_file(*, path: Union[str, pathlib.Path], format: Union[str, 'Literal["01", "b8", "r8", "ptb64", "hits", "dets"]'], bit_packed: bool = False, num_measurements: int = 0, num_detectors: int = 0, num_observables: int = 0, separate_observables: bool = False) -> Union[Tuple[np.ndarray, np.ndarray], np.ndarray]:

            Args:
                path: The path to the file to read the data from.
                format: The format that the data is stored in, such as 'b8'.
                    See https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
                bit_packed: Defaults to false. Determines whether the result is a bool_
                    numpy array with one bit per byte, or a uint8 numpy array with 8 bits
                    per byte.
                num_measurements: How many measurements there are per shot.
                num_detectors: How many detectors there are per shot.
                num_observables: How many observables there are per shot.
                    Note that this only refers to observables *stored in the file*, not to
                    observables from the original circuit that was sampled.
                separate_observables: When set to True, the result is a tuple of two arrays,
                    one containing the detection event data and the other containing the
                    observable data, instead of a single array.

            Returns:
                If separate_observables=True:
                    A tuple (dets, obs) of numpy arrays containing the loaded data.

                    If bit_packed=False:
                        dets.dtype = np.bool_
                        dets.shape = (num_shots, num_measurements + num_detectors)
                        det bit b from shot s is at dets[s, b]
                        obs.dtype = np.bool_
                        obs.shape = (num_shots, num_observables)
                        obs bit b from shot s is at dets[s, b]
                    If bit_packed=True:
                        dets.dtype = np.uint8
                        dets.shape = (num_shots, math.ceil(
                            (num_measurements + num_detectors) / 8))
                        obs.dtype = np.uint8
                        obs.shape = (num_shots, math.ceil(num_observables / 8))
                        det bit b from shot s is at dets[s, b // 8] & (1 << (b % 8))
                        obs bit b from shot s is at obs[s, b // 8] & (1 << (b % 8))

                If separate_observables=False:
                    A numpy array containing the loaded data.

                    If bit_packed=False:
                        dtype = np.bool_
                        shape = (num_shots,
                                 num_measurements + num_detectors + num_observables)
                        bit b from shot s is at result[s, b]
                    If bit_packed=True:
                        dtype = np.uint8
                        shape = (num_shots, math.ceil(
                            (num_measurements + num_detectors + num_observables) / 8))
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
        clean_doc_string(R"DOC(
            Writes shot data, such as measurement samples, to a file.
            @signature def write_shot_data_file(*, data: np.ndarray, path: Union[str, pathlib.Path], format: str, num_measurements: int = 0, num_detectors: int = 0, num_observables: int = 0) -> None:

            Args:
                data: The data to write to the file. This must be a numpy array. The dtype
                    of the array determines whether or not the data is bit packed, and the
                    shape must match the bits per shot.

                    dtype=np.bool_: Not bit packed. Shape must be
                        (num_shots, num_measurements + num_detectors + num_observables).
                    dtype=np.uint8: Yes bit packed. Shape must be
                        (num_shots, math.ceil(
                            (num_measurements + num_detectors + num_observables) / 8)).
                path: The path to the file to write the data to.
                format: The format that the data is stored in, such as 'b8'.
                    See https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
                num_measurements: How many measurements there are per shot.
                num_detectors: How many detectors there are per shot.
                num_observables: How many observables there are per shot.
                    Note that this only refers to observables *in the given shot data*, not
                    to observables from the original circuit that was sampled.

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
                ...     ], dtype=np.bool_)
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
