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

#include "stim/py/compiled_detector_sampler.pybind.h"

#include "stim/circuit/circuit.pybind.h"
#include "stim/io/raii_file.h"
#include "stim/py/base.pybind.h"
#include "stim/py/numpy.pybind.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/simulators/frame_simulator_util.h"
#include "stim/simulators/tableau_simulator.h"

using namespace stim;
using namespace stim_pybind;

CompiledDetectorSampler::CompiledDetectorSampler(Circuit init_circuit, std::mt19937_64 &&rng)
    : circuit_stats(init_circuit.compute_stats()),
      circuit(std::move(init_circuit)),
      frame_sim(circuit_stats, FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY, 0, std::move(rng)) {
}

pybind11::object CompiledDetectorSampler::sample_to_numpy(
    size_t num_shots,
    bool prepend_observables,
    bool append_observables,
    bool separate_observables,
    bool bit_packed,
    pybind11::object dets_out,
    pybind11::object obs_out) {
    if (separate_observables && (append_observables || prepend_observables)) {
        throw std::invalid_argument(
            "Can't specify separate_observables=True with append_observables=True or prepend_observables=True");
    }

    {
        pybind11::gil_scoped_release release;
        frame_sim.configure_for(circuit_stats, FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY, num_shots);
        frame_sim.reset_all();
        frame_sim.do_circuit(circuit);
    }

    const auto &det_data = frame_sim.det_record.storage;
    const auto &obs_data = frame_sim.obs_record;

    pybind11::object py_obs_data = pybind11::none();
    if (separate_observables || !obs_out.is_none()) {
        py_obs_data =
            simd_bit_table_to_numpy(obs_data, circuit_stats.num_observables, num_shots, bit_packed, true, obs_out);
    }

    pybind11::object py_det_data = pybind11::none();
    if (append_observables || prepend_observables) {
        size_t num_concat = circuit_stats.num_detectors;
        simd_bit_table<MAX_BITWORD_WIDTH> concat_data = det_data;
        if (append_observables) {
            concat_data = concat_data.concat_major(obs_data, num_concat, circuit_stats.num_observables);
            num_concat += circuit_stats.num_observables;
        }
        if (prepend_observables) {
            concat_data = obs_data.concat_major(concat_data, circuit_stats.num_observables, num_concat);
            num_concat += circuit_stats.num_observables;
        }
        py_det_data = simd_bit_table_to_numpy(concat_data, num_concat, num_shots, bit_packed, true, dets_out);
    } else {
        py_det_data =
            simd_bit_table_to_numpy(det_data, circuit_stats.num_detectors, num_shots, bit_packed, true, dets_out);
    }

    if (separate_observables) {
        return pybind11::make_tuple(py_det_data, py_obs_data);
    } else {
        return py_det_data;
    }
}

void CompiledDetectorSampler::sample_write(
    size_t num_samples,
    pybind11::object filepath_obj,
    std::string_view format,
    bool prepend_observables,
    bool append_observables,
    pybind11::object obs_out_filepath_obj,
    std::string_view obs_out_format) {
    auto f = format_to_enum(format);

    auto py_path = pybind11::module::import("pathlib").attr("Path");
    if (pybind11::isinstance(filepath_obj, py_path)) {
        filepath_obj = pybind11::str(filepath_obj);
    }
    if (pybind11::isinstance(obs_out_filepath_obj, py_path)) {
        obs_out_filepath_obj = pybind11::str(obs_out_filepath_obj);
    }

    std::string_view filepath;
    if (pybind11::isinstance<pybind11::str>(filepath_obj)) {
        filepath = pybind11::cast<std::string_view>(filepath_obj);
    } else {
        std::stringstream ss;
        ss << "Don't know how to write to ";
        ss << pybind11::repr(filepath_obj);
        throw std::invalid_argument(ss.str());
    }

    std::string_view obs_out_filepath_view;
    if (pybind11::isinstance<pybind11::str>(obs_out_filepath_obj)) {
        obs_out_filepath_view = pybind11::cast<std::string_view>(obs_out_filepath_obj);
    } else if (obs_out_filepath_obj.is_none()) {
        // Empty string view does the right thing.
    } else {
        std::stringstream ss;
        ss << "Don't know how to write observables to ";
        ss << pybind11::repr(obs_out_filepath_obj);
        throw std::invalid_argument(ss.str());
    }

    RaiiFile out(filepath, "wb");
    RaiiFile obs_out(obs_out_filepath_view, "wb");
    auto parsed_obs_out_format = format_to_enum(obs_out_format);
    sample_batch_detection_events_writing_results_to_disk<MAX_BITWORD_WIDTH>(
        circuit,
        num_samples,
        prepend_observables,
        append_observables,
        out.f,
        f,
        frame_sim.rng,
        obs_out.f,
        parsed_obs_out_format);
}

std::string CompiledDetectorSampler::repr() const {
    std::stringstream result;
    result << "stim.CompiledDetectorSampler(";
    result << circuit_repr(circuit);
    result << ")";
    return result.str();
}

CompiledDetectorSampler stim_pybind::py_init_compiled_detector_sampler(
    const Circuit &circuit, const pybind11::object &seed) {
    return CompiledDetectorSampler(circuit, make_py_seeded_rng(seed));
}

pybind11::class_<CompiledDetectorSampler> stim_pybind::pybind_compiled_detector_sampler(pybind11::module &m) {
    return pybind11::class_<CompiledDetectorSampler>(
        m, "CompiledDetectorSampler", "An analyzed stabilizer circuit whose detection events can be sampled quickly.");
}

void stim_pybind::pybind_compiled_detector_sampler_methods(
    pybind11::module &m, pybind11::class_<CompiledDetectorSampler> &c) {
    c.def(
        pybind11::init(&py_init_compiled_detector_sampler),
        pybind11::arg("circuit"),
        pybind11::kw_only(),
        pybind11::arg("seed") = pybind11::none(),
        clean_doc_string(R"DOC(
            Creates an object that can sample the detection events from a circuit.

            Args:
                circuit: The circuit to sample from.
                seed: PARTIALLY determines simulation results by deterministically seeding
                    the random number generator.

                    Must be None or an integer in range(2**64).

                    Defaults to None. When None, the prng is seeded from system entropy.

                    When set to an integer, making the exact same series calls on the exact
                    same machine with the exact same version of Stim will produce the exact
                    same simulation results.

                    CAUTION: simulation results *WILL NOT* be consistent between versions of
                    Stim. This restriction is present to make it possible to have future
                    optimizations to the random sampling, and is enforced by introducing
                    intentional differences in the seeding strategy from version to version.

                    CAUTION: simulation results *MAY NOT* be consistent across machines that
                    differ in the width of supported SIMD instructions. For example, using
                    the same seed on a machine that supports AVX instructions and one that
                    only supports SSE instructions may produce different simulation results.

                    CAUTION: simulation results *MAY NOT* be consistent if you vary how many
                    shots are taken. For example, taking 10 shots and then 90 shots will
                    give different results from taking 100 shots in one call.

            Returns:
                An initialized stim.CompiledDetectorSampler.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    H 0
                ...    CNOT 0 1
                ...    X_ERROR(1.0) 0
                ...    M 0 1
                ...    DETECTOR rec[-1] rec[-2]
                ... ''')
                >>> s = c.compile_detector_sampler()
                >>> s.sample(shots=1)
                array([[ True]])
        )DOC")
            .data());

    c.def(
        "sample",
        [](CompiledDetectorSampler &self,
           size_t shots,
           bool prepend,
           bool append,
           bool separate_observables,
           bool bit_packed,
           pybind11::object dets_out,
           pybind11::object obs_out) {
            return self.sample_to_numpy(shots, prepend, append, separate_observables, bit_packed, dets_out, obs_out);
        },
        pybind11::arg("shots"),
        pybind11::kw_only(),
        pybind11::arg("prepend_observables") = false,
        pybind11::arg("append_observables") = false,
        pybind11::arg("separate_observables") = false,
        pybind11::arg("bit_packed") = false,
        pybind11::arg("dets_out") = pybind11::none(),
        pybind11::arg("obs_out") = pybind11::none(),
        clean_doc_string(R"DOC(
            @signature def sample(self, shots: int, *, prepend_observables: bool = False, append_observables: bool = False, separate_observables: bool = False, bit_packed: bool = False, dets_out: Optional[np.ndarray] = None, obs_out: Optional[np.ndarray] = None) -> Union[np.ndarray, Tuple[np.ndarray, np.ndarray]]:
            Returns a numpy array containing a batch of detector samples from the circuit.

            The circuit must define the detectors using DETECTOR instructions. Observables
            defined by OBSERVABLE_INCLUDE instructions can also be included in the results
            as honorary detectors.

            Args:
                shots: The number of times to sample every detector in the circuit.
                separate_observables: Defaults to False. When set to True, the return value
                    is a (detection_events, observable_flips) tuple instead of a flat
                    detection_events array.
                prepend_observables: Defaults to false. When set, observables are included
                    with the detectors and are placed at the start of the results.
                append_observables: Defaults to false. When set, observables are included
                    with the detectors and are placed at the end of the results.
                bit_packed: Returns a uint8 numpy array with 8 bits per byte, instead of
                    a bool_ numpy array with 1 bit per byte. Uses little endian packing.
                dets_out: Defaults to None. Specifies a pre-allocated numpy array to write
                    the detection event data into. This array must have the correct shape
                    and dtype.
                obs_out: Defaults to None. Specifies a pre-allocated numpy array to write
                    the observable flip data into. This array must have the correct shape
                    and dtype.

            Returns:
                A numpy array or tuple of numpy arrays containing the samples.

                if separate_observables=False and bit_packed=False:
                    A single numpy array.
                    dtype=bool_
                    shape=(
                        shots,
                        num_detectors + num_observables * (
                            append_observables + prepend_observables),
                    )
                    The bit for detection event `m` in shot `s` is at
                        result[s, m]

                if separate_observables=False and bit_packed=True:
                    A single numpy array.
                    dtype=uint8
                    shape=(
                        shots,
                        math.ceil((num_detectors + num_observables * (
                            append_observables + prepend_observables)) / 8),
                    )
                    The bit for detection event `m` in shot `s` is at
                        (result[s, m // 8] >> (m % 8)) & 1

                if separate_observables=True and bit_packed=False:
                    A (dets, obs) tuple.
                    dets.dtype=bool_
                    dets.shape=(shots, num_detectors)
                    obs.dtype=bool_
                    obs.shape=(shots, num_observables)
                    The bit for detection event `m` in shot `s` is at
                        dets[s, m]
                    The bit for observable `m` in shot `s` is at
                        obs[s, m]

                if separate_observables=True and bit_packed=True:
                    A (dets, obs) tuple.
                    dets.dtype=uint8
                    dets.shape=(shots, math.ceil(num_detectors / 8))
                    obs.dtype=uint8
                    obs.shape=(shots, math.ceil(num_observables / 8))
                    The bit for detection event `m` in shot `s` is at
                        (dets[s, m // 8] >> (m % 8)) & 1
                    The bit for observable `m` in shot `s` is at
                        (obs[s, m // 8] >> (m % 8)) & 1

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    H 0
                ...    CNOT 0 1
                ...    X_ERROR(1.0) 0
                ...    M 0 1
                ...    DETECTOR rec[-1] rec[-2]
                ... ''')
                >>> s = c.compile_detector_sampler()
                >>> s.sample(shots=1)
                array([[ True]])
        )DOC")
            .data());

    c.def(
        "sample_bit_packed",
        [](CompiledDetectorSampler &self, size_t shots, bool prepend, bool append) {
            return self.sample_to_numpy(shots, prepend, append, false, true, pybind11::none(), pybind11::none());
        },
        pybind11::arg("shots"),
        pybind11::kw_only(),
        pybind11::arg("prepend_observables") = false,
        pybind11::arg("append_observables") = false,
        clean_doc_string(R"DOC(
            [DEPRECATED] Use sampler.sample(..., bit_packed=True) instead.

            Returns a numpy array containing bit packed detector samples from the circuit.

            The circuit must define the detectors using DETECTOR instructions. Observables
            defined by OBSERVABLE_INCLUDE instructions can also be included in the results
            as honorary detectors.

            Args:
                shots: The number of times to sample every detector in the circuit.
                prepend_observables: Defaults to false. When set, observables are included
                    with the detectors and are placed at the start of the results.
                append_observables: Defaults to false. When set, observables are included
                    with the detectors and are placed at the end of the results.

            Returns:
                A numpy array with `dtype=uint8` and `shape=(shots, n)` where `n` is
                `num_detectors + num_observables*(append_observables+prepend_observables)`.
                The bit for detection event `m` in shot `s` is at
                `result[s, (m // 8)] & 2**(m % 8)`.
        )DOC")
            .data());

    c.def(
        "sample_write",
        &CompiledDetectorSampler::sample_write,
        pybind11::arg("shots"),
        pybind11::kw_only(),
        pybind11::arg("filepath"),
        pybind11::arg("format") = "01",
        pybind11::arg("prepend_observables") = false,
        pybind11::arg("append_observables") = false,
        pybind11::arg("obs_out_filepath") = pybind11::none(),
        pybind11::arg("obs_out_format") = "01",
        clean_doc_string(R"DOC(
            @signature def sample_write(self, shots: int, *, filepath: Union[str, pathlib.Path], format: 'Literal["01", "b8", "r8", "ptb64", "hits", "dets"]' = '01', obs_out_filepath: Optional[Union[str, pathlib.Path]] = None, obs_out_format: 'Literal["01", "b8", "r8", "ptb64", "hits", "dets"]' = '01', prepend_observables: bool = False, append_observables: bool = False) -> None:
            Samples detection events from the circuit and writes them to a file.

            Args:
                shots: The number of times to sample every measurement in the circuit.
                filepath: The file to write the results to.
                format: The output format to write the results with.
                    Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
                    Defaults to "01".
                obs_out_filepath: Sample observables as part of each shot, and write them to
                    this file. This keeps the observable data separate from the detector
                    data.
                obs_out_format: If writing the observables to a file, this is the format to
                    write them in.

                    Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
                    Defaults to "01".
                prepend_observables: Sample observables as part of each shot, and put them
                    at the start of the detector data.
                append_observables: Sample observables as part of each shot, and put them at
                    the end of the detector data.

            Returns:
                None.

            Examples:
                >>> import stim
                >>> import tempfile
                >>> with tempfile.TemporaryDirectory() as d:
                ...     path = f"{d}/tmp.dat"
                ...     c = stim.Circuit('''
                ...         X_ERROR(1) 0
                ...         M 0 1
                ...         DETECTOR rec[-2]
                ...         DETECTOR rec[-1]
                ...     ''')
                ...     c.compile_detector_sampler().sample_write(
                ...         shots=3,
                ...         filepath=path,
                ...         format="dets")
                ...     with open(path) as f:
                ...         print(f.read(), end='')
                shot D0
                shot D0
                shot D0
        )DOC")
            .data());

    c.def(
        "__repr__",
        &CompiledDetectorSampler::repr,
        "Returns valid python code evaluating to an equivalent `stim.CompiledDetectorSampler`.");
}
