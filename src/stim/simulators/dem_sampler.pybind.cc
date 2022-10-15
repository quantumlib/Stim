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

#include "stim/simulators/dem_sampler.pybind.h"

#include "stim/io/raii_file.h"
#include "stim/py/base.pybind.h"
#include "stim/py/numpy.pybind.h"

using namespace stim;
using namespace stim_pybind;

RaiiFile optional_py_path_to_raii_file(const pybind11::object &obj, const char *mode) {
    try {
        auto path = pybind11::cast<std::string>(obj);
        return RaiiFile(path.data(), mode);
    } catch (pybind11::cast_error &ex) {
    }

    auto py_path = pybind11::module::import("pathlib").attr("Path");
    if (pybind11::isinstance(obj, py_path)) {
        auto path = pybind11::cast<std::string>(pybind11::str(obj));
        return RaiiFile(path.data(), mode);
    }

    return RaiiFile(nullptr);
}

pybind11::object dem_sampler_py_sample(
    DemSampler &self, size_t shots, bool bit_packed, bool return_errors, pybind11::object &recorded_errors_to_replay) {

    self.set_min_stripes(shots);

    bool replay = !recorded_errors_to_replay.is_none();
    if (replay && min_bits_to_num_bits_padded<MAX_BITWORD_WIDTH>(shots) != self.num_stripes) {
        DemSampler perfect_size(self.model, std::move(self.rng), shots);
        auto result = dem_sampler_py_sample(perfect_size, shots, bit_packed, return_errors, recorded_errors_to_replay);
        self.rng = std::move(perfect_size.rng);
        return result;
    }

    if (replay) {
        size_t out_shots;
        simd_bit_table<MAX_BITWORD_WIDTH> converted =
            numpy_array_to_transposed_simd_table(recorded_errors_to_replay, self.num_errors, &out_shots);
        if (out_shots != shots) {
            throw std::invalid_argument("recorded_errors_to_replay.shape[0] != shots");
        }
        assert(converted.num_minor_bits_padded() == self.err_buffer.num_minor_bits_padded());
        assert(converted.num_major_bits_padded() == self.err_buffer.num_major_bits_padded());
        self.err_buffer = std::move(converted);
    }

    self.resample(replay);

    pybind11::object err_out = pybind11::none();
    if (return_errors) {
        err_out = transposed_simd_bit_table_to_numpy(self.err_buffer, self.num_errors, shots, bit_packed);
    }
    pybind11::object det_out =
        transposed_simd_bit_table_to_numpy(self.det_buffer, self.num_detectors, shots, bit_packed);
    pybind11::object obs_out =
        transposed_simd_bit_table_to_numpy(self.obs_buffer, self.num_observables, shots, bit_packed);
    return pybind11::make_tuple(det_out, obs_out, err_out);
}

pybind11::class_<DemSampler> stim_pybind::pybind_dem_sampler(pybind11::module &m) {
    return pybind11::class_<DemSampler>(
        m,
        "CompiledDemSampler",
        clean_doc_string(u8R"DOC(
            A helper class for efficiently sampler from a detector error model.

            Examples:
                >>> import stim
                >>> dem = stim.DetectorErrorModel('''
                ...    error(0) D0
                ...    error(1) D1 D2 L0
                ... ''')
                >>> sampler = dem.compile_sampler()
                >>> det_data, obs_data, err_data = sampler.sample(
                ...     shots=4,
                ...     return_errors=True)
                >>> det_data
                array([[False,  True,  True],
                       [False,  True,  True],
                       [False,  True,  True],
                       [False,  True,  True]])
                >>> obs_data
                array([[ True],
                       [ True],
                       [ True],
                       [ True]])
                >>> err_data
                array([[False,  True],
                       [False,  True],
                       [False,  True],
                       [False,  True]])
        )DOC")
            .data());
}

void stim_pybind::pybind_dem_sampler_methods(
    pybind11::module &m, pybind11::class_<stim::DemSampler> &c) {
    c.def(
        "sample",
        &dem_sampler_py_sample,
        pybind11::arg("shots"),
        pybind11::kw_only(),
        pybind11::arg("bit_packed") = false,
        pybind11::arg("return_errors") = false,
        pybind11::arg("recorded_errors_to_replay") = pybind11::none(),
        clean_doc_string(u8R"DOC(
            @signature def sample(self, shots: int, *, bit_packed: bool = False, return_errors: bool = False, recorded_errors_to_replay: Optional[np.ndarray] = None) -> Tuple[np.ndarray, np.ndarray, Optional[np.ndarray]]:
            Samples the detector error model's error mechanisms to produce sample data.

            Args:
                shots: The number of times to sample from the model.
                bit_packed: Defaults to false.
                    False: the returned numpy arrays have dtype=np.bool8.
                    True: the returned numpy arrays have dtype=np.uint8 and pack 8 bits into
                        each byte.

                    Setting this to True is equivalent to running
                    `np.packbits(data, endian='little', axis=1)` on each output value, but
                    has the performance benefit of the data never being expanded into an
                    unpacked form.
                return_errors: Defaults to False.
                    False: the first entry of the returned tuple is None.
                    True: the first entry of the returned tuple is a numpy array recording
                    which errors were sampled.
                recorded_errors_to_replay: Defaults to None, meaning sample errors randomly.
                    If not None, this is expected to be a 2d numpy array specifying which
                    errors to apply (e.g. one returned from a previous call to the sample
                    method). The array must have dtype=np.bool8 and
                    shape=(num_shots, num_errors) or dtype=np.uint8 and
                    shape=(num_shots, math.ceil(num_errors / 8)).

            Returns:
                A tuple (detector_data, obs_data, error_data).

                Assuming bit_packed is False and return_errors is True:
                    - If error_data[s, k] is True, then the error with index k fired in the
                        shot with index s.
                    - If detector_data[s, k] is True, then the detector with index k ended
                        up flipped in the shot with index s.
                    - If obs_data[s, k] is True, then the observable with index k ended up
                        flipped in the shot with index s.

                The dtype and shape of the data depends on the arguments:
                    if bit_packed:
                        detector_data.shape == (num_shots, num_detectors)
                        detector_data.dtype == np.bool8
                        obs_data.shape == (num_shots, num_observables)
                        obs_data.dtype == np.bool8
                        if return_errors:
                            error_data.shape = (num_shots, num_errors)
                            error_data.dtype = np.bool8
                        else:
                            error_data is None
                    else:
                        detector_data.shape == (num_shots, math.ceil(num_detectors / 8))
                        detector_data.dtype == np.uint8
                        obs_data.shape == (num_shots, math.ceil(num_observables / 8))
                        obs_data.dtype == np.uint8
                        if return_errors:
                            error_data.shape = (num_shots, math.ceil(num_errors / 8))
                            error_data.dtype = np.uint8
                        else:
                            error_data is None

                Note that bit packing is done using little endian order on the last axis
                (i.e. like `np.packbits(data, endian='little', axis=1)`).

            Examples:
                >>> import stim
                >>> import numpy as np
                >>> dem = stim.DetectorErrorModel('''
                ...    error(0) D0
                ...    error(1) D1 D2 L0
                ... ''')
                >>> sampler = dem.compile_sampler()

                >>> # Taking samples.
                >>> det_data, obs_data, err_data_not_requested = sampler.sample(shots=4)
                >>> det_data
                array([[False,  True,  True],
                       [False,  True,  True],
                       [False,  True,  True],
                       [False,  True,  True]])
                >>> obs_data
                array([[ True],
                       [ True],
                       [ True],
                       [ True]])
                >>> err_data_not_requested is None
                True

                >>> # Recording errors.
                >>> det_data, obs_data, err_data = sampler.sample(
                ...     shots=4,
                ...     return_errors=True)
                >>> det_data
                array([[False,  True,  True],
                       [False,  True,  True],
                       [False,  True,  True],
                       [False,  True,  True]])
                >>> obs_data
                array([[ True],
                       [ True],
                       [ True],
                       [ True]])
                >>> err_data
                array([[False,  True],
                       [False,  True],
                       [False,  True],
                       [False,  True]])

                >>> # Bit packing.
                >>> det_data, obs_data, err_data = sampler.sample(
                ...     shots=4,
                ...     return_errors=True,
                ...     bit_packed=True)
                >>> det_data
                array([[6],
                       [6],
                       [6],
                       [6]], dtype=uint8)
                >>> obs_data
                array([[1],
                       [1],
                       [1],
                       [1]], dtype=uint8)
                >>> err_data
                array([[2],
                       [2],
                       [2],
                       [2]], dtype=uint8)

                >>> # Recording and replaying errors.
                >>> noisy_dem = stim.DetectorErrorModel('''
                ...    error(0.125) D0
                ...    error(0.25) D1
                ... ''')
                >>> noisy_sampler = noisy_dem.compile_sampler()
                >>> det_data, obs_data, err_data = noisy_sampler.sample(
                ...     shots=100,
                ...     return_errors=True)
                >>> replay_det_data, replay_obs_data, _ = noisy_sampler.sample(
                ...     shots=100,
                ...     recorded_errors_to_replay=err_data)
                >>> np.array_equal(det_data, replay_det_data)
                True
                >>> np.array_equal(obs_data, replay_obs_data)
                True
        )DOC")
            .data());

    c.def(
        "sample_write",
        [](DemSampler &self,
           size_t shots,
           pybind11::object &det_out_file,
           const std::string &det_out_format,
           pybind11::object &obs_out_file,
           const std::string &obs_out_format,
           pybind11::object &err_out_file,
           const std::string &err_out_format,
           pybind11::object &replay_err_in_file,
           const std::string &replay_err_in_format) {
            RaiiFile fd = optional_py_path_to_raii_file(det_out_file, "w");
            RaiiFile fo = optional_py_path_to_raii_file(obs_out_file, "w");
            RaiiFile feo = optional_py_path_to_raii_file(err_out_file, "w");
            RaiiFile fei = optional_py_path_to_raii_file(replay_err_in_file, "r");
            self.sample_write(
                shots,
                fd.f,
                format_to_enum(det_out_format),
                fo.f,
                format_to_enum(obs_out_format),
                feo.f,
                format_to_enum(err_out_format),
                fei.f,
                format_to_enum(replay_err_in_format));
        },
        pybind11::arg("shots"),
        pybind11::kw_only(),
        pybind11::arg("det_out_file"),
        pybind11::arg("det_out_format") = "01",
        pybind11::arg("obs_out_file"),
        pybind11::arg("obs_out_format") = "01",
        pybind11::arg("err_out_file") = pybind11::none(),
        pybind11::arg("err_out_format") = "01",
        pybind11::arg("replay_err_in_file") = pybind11::none(),
        pybind11::arg("replay_err_in_format") = "01",
        clean_doc_string(u8R"DOC(
            @signature def sample_write(self, shots: int, *, det_out_file: Union[None, str, pathlib.Path], det_out_format: str = "01", obs_out_file: Union[None, str, pathlib.Path], obs_out_format: str = "01", err_out_file: Union[None, str, pathlib.Path] = None, err_out_format: str = "01", replay_err_in_file: Union[None, str, pathlib.Path] = None, replay_err_in_format: str = "01") -> None:
            Samples the detector error model and writes the results to disk.

            Args:
                shots: The number of times to sample from the model.
                det_out_file: Where to write detection event data.
                    If None: detection event data is not written.
                    If str or pathlib.Path: opens and overwrites the file at the given path.
                    NOT IMPLEMENTED: io.IOBase
                det_out_format: The format to write the detection event data in
                    (e.g. "01" or "b8").
                obs_out_file: Where to write observable flip data.
                    If None: observable flip data is not written.
                    If str or pathlib.Path: opens and overwrites the file at the given path.
                    NOT IMPLEMENTED: io.IOBase
                obs_out_format: The format to write the observable flip data in
                    (e.g. "01" or "b8").
                err_out_file: Where to write errors-that-occurred data.
                    If None: errors-that-occurred data is not written.
                    If str or pathlib.Path: opens and overwrites the file at the given path.
                    NOT IMPLEMENTED: io.IOBase
                err_out_format: The format to write the errors-that-occurred data in
                    (e.g. "01" or "b8").
                replay_err_in_file: If this is specified, errors are replayed from data
                    instead of generated randomly. The following types are supported:
                    - None: errors are generated randomly according to the probabilities
                        in the detector error model.
                    - str or pathlib.Path: the file at the given path is opened and
                        errors-to-apply data is read from there.
                    - io.IOBase: NOT IMPLEMENTED
                replay_err_in_format: The format to write the errors-that-occurred data in
                    (e.g. "01" or "b8").

            Returns:
                Nothing. Results are written to disk.

            Examples:
                >>> import stim
                >>> import tempfile
                >>> import pathlib
                >>> dem = stim.DetectorErrorModel('''
                ...    error(0) D0
                ...    error(0) D1
                ...    error(0) D0
                ...    error(1) D1 D2 L0
                ...    error(0) D0
                ... ''')
                >>> sampler = dem.compile_sampler()
                >>> with tempfile.TemporaryDirectory() as d:
                ...     d = pathlib.Path(d)
                ...     sampler.sample_write(
                ...         shots=1,
                ...         det_out_file=d / 'dets.01',
                ...         det_out_format='01',
                ...         obs_out_file=d / 'obs.01',
                ...         obs_out_format='01',
                ...         err_out_file=d / 'err.hits',
                ...         err_out_format='hits',
                ...     )
                ...     with open(d / 'dets.01') as f:
                ...         assert f.read() == "011\n"
                ...     with open(d / 'obs.01') as f:
                ...         assert f.read() == "1\n"
                ...     with open(d / 'err.hits') as f:
                ...         assert f.read() == "3\n"
        )DOC")
            .data());
}
