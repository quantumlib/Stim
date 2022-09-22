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

#include "stim/simulators/measurements_to_detection_events.pybind.h"

#include "stim/circuit/circuit.pybind.h"
#include "stim/io/raii_file.h"
#include "stim/py/base.pybind.h"
#include "stim/py/numpy.pybind.h"
#include "stim/simulators/detection_simulator.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/simulators/measurements_to_detection_events.h"
#include "stim/simulators/tableau_simulator.h"

using namespace stim;
using namespace stim_pybind;

CompiledMeasurementsToDetectionEventsConverter::CompiledMeasurementsToDetectionEventsConverter(
    simd_bits<MAX_BITWORD_WIDTH> ref_sample, Circuit circuit, bool skip_reference_sample)
    : skip_reference_sample(skip_reference_sample),
      ref_sample(ref_sample),
      circuit_num_measurements(circuit.count_measurements()),
      circuit_num_sweep_bits(circuit.count_sweep_bits()),
      circuit_num_detectors(circuit.count_detectors()),
      circuit_num_observables(circuit.count_observables()),
      circuit_num_qubits(circuit.count_qubits()),
      circuit(std::move(circuit)) {
}
std::string CompiledMeasurementsToDetectionEventsConverter::repr() const {
    std::stringstream result;
    result << "stim.CompiledMeasurementsToDetectionEventsConverter(";
    result << circuit_repr(circuit);
    if (skip_reference_sample) {
        result << ", skip_reference_sample=True";
    }
    result << ")";
    return result.str();
}

void CompiledMeasurementsToDetectionEventsConverter::convert_file(
    const std::string &measurements_filepath,
    const std::string &measurements_format,
    const char *sweep_bits_filepath,
    const std::string &sweep_bits_format,
    const std::string &detection_events_filepath,
    const std::string &detection_events_format,
    bool append_observables,
    const char *obs_out_filepath,
    const std::string &obs_out_format) {
    auto format_in = format_to_enum(measurements_format);
    auto format_sweep_bits = format_to_enum(sweep_bits_format);
    auto format_out = format_to_enum(detection_events_format);
    RaiiFile file_in(measurements_filepath.data(), "r");
    RaiiFile obs_out(obs_out_filepath, "w");
    RaiiFile sweep_bits_in(sweep_bits_filepath, "r");
    RaiiFile detections_out(detection_events_filepath.data(), "w");
    auto parsed_obs_out_format = format_to_enum(obs_out_format);

    stim::stream_measurements_to_detection_events_helper(
        file_in.f,
        format_in,
        sweep_bits_in.f,
        format_sweep_bits,
        detections_out.f,
        format_out,
        circuit.aliased_noiseless_circuit(),
        append_observables,
        ref_sample,
        obs_out.f,
        parsed_obs_out_format,
        circuit_num_measurements,
        circuit_num_observables,
        circuit_num_detectors,
        circuit_num_qubits,
        circuit_num_sweep_bits);
}

pybind11::object CompiledMeasurementsToDetectionEventsConverter::convert(
    const pybind11::object &measurements,
    const pybind11::object &sweep_bits,
    const pybind11::object &separate_observables_obj,
    const pybind11::object &append_observables_obj,
    bool bit_pack_result) {
    if (separate_observables_obj.is_none() && append_observables_obj.is_none()) {
        throw std::invalid_argument(
            "To ignore observable flip data, you must explicitly specify either separate_observables=False or "
            "append_observables=False.");
    }
    bool separate_observables = pybind11::cast<bool>(separate_observables_obj);
    bool append_observables = pybind11::cast<bool>(append_observables_obj);
    size_t num_shots;
    simd_bit_table<MAX_BITWORD_WIDTH> measurements_minor_shot_index =
        numpy_array_to_transposed_simd_table(measurements, circuit_num_measurements, &num_shots);

    simd_bit_table<MAX_BITWORD_WIDTH> sweep_bits_minor_shot_index{0, num_shots};
    if (!sweep_bits.is_none()) {
        size_t num_sweep_shots;
        sweep_bits_minor_shot_index =
            numpy_array_to_transposed_simd_table(sweep_bits, circuit_num_sweep_bits, &num_sweep_shots);
        if (num_shots != num_sweep_shots) {
            throw std::invalid_argument("Need sweep_bits.shape[0] == measurements.shape[0]");
        }
    }

    size_t num_intermediate_bits =
        circuit_num_detectors + circuit_num_observables * (append_observables || separate_observables);
    simd_bit_table<MAX_BITWORD_WIDTH> out_detection_results_minor_shot_index(num_intermediate_bits, num_shots);
    stim::measurements_to_detection_events_helper(
        measurements_minor_shot_index,
        sweep_bits_minor_shot_index,
        out_detection_results_minor_shot_index,
        circuit.aliased_noiseless_circuit(),
        ref_sample,
        append_observables || separate_observables,
        circuit_num_measurements,
        circuit_num_detectors,
        circuit_num_observables,
        circuit_num_qubits);

    size_t num_output_bits = circuit_num_detectors + circuit_num_observables * append_observables;
    pybind11::object obs_data = pybind11::none();
    if (separate_observables) {
        simd_bit_table<MAX_BITWORD_WIDTH> obs_table(circuit_num_observables, num_shots);
        for (size_t obs = 0; obs < circuit_num_observables; obs++) {
            auto obs_slice = out_detection_results_minor_shot_index[circuit_num_detectors + obs];
            obs_table[obs] = obs_slice;
            if (!append_observables) {
                obs_slice.clear();
            }
        }
        obs_data = transposed_simd_bit_table_to_numpy(obs_table, circuit_num_observables, num_shots, bit_pack_result);
    }

    // Caution: only do this after extracting the observable data, lest it leak into the packed bytes.
    pybind11::object det_data = transposed_simd_bit_table_to_numpy(
        out_detection_results_minor_shot_index, num_output_bits, num_shots, bit_pack_result);

    if (separate_observables) {
        return pybind11::make_tuple(det_data, obs_data);
    }
    return det_data;
}

pybind11::class_<CompiledMeasurementsToDetectionEventsConverter>
stim_pybind::pybind_compiled_measurements_to_detection_events_converter(pybind11::module &m) {
    return pybind11::class_<CompiledMeasurementsToDetectionEventsConverter>(
        m,
        "CompiledMeasurementsToDetectionEventsConverter",
        "A tool for quickly converting measurements from an analyzed stabilizer circuit into detection events.");
}

CompiledMeasurementsToDetectionEventsConverter stim_pybind::py_init_compiled_measurements_to_detection_events_converter(
    const Circuit &circuit, bool skip_reference_sample) {
    simd_bits<MAX_BITWORD_WIDTH> ref_sample = skip_reference_sample ? simd_bits<MAX_BITWORD_WIDTH>(circuit.count_measurements())
                                                 : TableauSimulator::reference_sample_circuit(circuit);
    return CompiledMeasurementsToDetectionEventsConverter(ref_sample, circuit, skip_reference_sample);
}

void stim_pybind::pybind_compiled_measurements_to_detection_events_converter_methods(
    pybind11::module &m,
    pybind11::class_<CompiledMeasurementsToDetectionEventsConverter> &c) {
    c.def(
        pybind11::init(&py_init_compiled_measurements_to_detection_events_converter),
        pybind11::arg("circuit"),
        pybind11::kw_only(),
        pybind11::arg("skip_reference_sample") = false,
        clean_doc_string(u8R"DOC(
            Creates a measurement-to-detection-events converter for the given circuit.

            The converter uses a noiseless reference sample, collected from the circuit
            using stim's Tableau simulator during initialization of the converter, as a
            baseline for determining what the expected value of a detector is.

            Note that the expected behavior of gauge detectors (detectors that are not
            actually deterministic under noiseless execution) can vary depending on the
            reference sample. Stim mitigates this by always generating the same reference
            sample for a given circuit.

            Args:
                circuit: The stim circuit to use for conversions.
                skip_reference_sample: Defaults to False. When set to True, the reference
                    sample used by the converter is initialized to all-zeroes instead of
                    being collected from the circuit. This should only be used if it's known
                    that the all-zeroes sample is actually a possible result from the
                    circuit (under noiseless execution).

            Returns:
                An initialized stim.CompiledMeasurementsToDetectionEventsConverter.

            Examples:
                >>> import stim
                >>> import numpy as np
                >>> converter = stim.Circuit('''
                ...    X 0
                ...    M 0
                ...    DETECTOR rec[-1]
                ... ''').compile_m2d_converter()
                >>> converter.convert(
                ...     measurements=np.array([[0], [1]], dtype=np.bool8),
                ...     append_observables=False,
                ... )
                array([[ True],
                       [False]])
        )DOC")
            .data());

    c.def(
        "convert_file",
        &CompiledMeasurementsToDetectionEventsConverter::convert_file,
        pybind11::kw_only(),
        pybind11::arg("measurements_filepath"),
        pybind11::arg("measurements_format") = "01",
        pybind11::arg("sweep_bits_filepath") = pybind11::none(),
        pybind11::arg("sweep_bits_format") = "01",
        pybind11::arg("detection_events_filepath"),
        pybind11::arg("detection_events_format") = "01",
        pybind11::arg("append_observables") = false,
        pybind11::arg("obs_out_filepath") = nullptr,
        pybind11::arg("obs_out_format") = "01",
        clean_doc_string(u8R"DOC(
            Reads measurement data from a file and writes detection events to another file.

            Args:
                measurements_filepath: A file containing measurement data to be converted.
                measurements_format: The format the measurement data is stored in.
                    Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
                    Defaults to "01".
                detection_events_filepath: Where to save detection event data to.
                detection_events_format: The format to save the detection event data in.
                    Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
                    Defaults to "01".
                sweep_bits_filepath: Defaults to None. A file containing sweep data, or
                    None. When specified, sweep data (used for `sweep[k]` controls in the
                    circuit, which can vary from shot to shot) will be read from the given
                    file. When not specified, all sweep bits default to False and no
                    sweep-controlled operations occur.
                sweep_bits_format: The format the sweep data is stored in.
                    Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
                    Defaults to "01".
                obs_out_filepath: Sample observables as part of each shot, and write them to
                    this file. This keeps the observable data separate from the detector
                    data.
                obs_out_format: If writing the observables to a file, this is the format to
                    write them in.
                    Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
                    Defaults to "01".
                append_observables: When True, the observables in the circuit are included
                    as part of the detection event data. Specifically, they are treated as
                    if they were additional detectors at the end of the circuit. When False,
                    observable data is not output.

            Examples:
                >>> import stim
                >>> import tempfile
                >>> converter = stim.Circuit('''
                ...    X 0
                ...    M 0
                ...    DETECTOR rec[-1]
                ... ''').compile_m2d_converter()
                >>> with tempfile.TemporaryDirectory() as d:
                ...    with open(f"{d}/measurements.01", "w") as f:
                ...        print("0", file=f)
                ...        print("1", file=f)
                ...    converter.convert_file(
                ...        measurements_filepath=f"{d}/measurements.01",
                ...        detection_events_filepath=f"{d}/detections.01",
                ...        append_observables=False,
                ...    )
                ...    with open(f"{d}/detections.01", "r") as f:
                ...        print(f.read(), end="")
                1
                0
        )DOC")
            .data());

    c.def(
        "convert",
        &CompiledMeasurementsToDetectionEventsConverter::convert,
        pybind11::kw_only(),
        pybind11::arg("measurements"),
        pybind11::arg("sweep_bits") = pybind11::none(),
        pybind11::arg("separate_observables") = pybind11::none(),
        pybind11::arg("append_observables") = pybind11::none(),
        pybind11::arg("bit_pack_result") = false,
        clean_doc_string(u8R"DOC(
            Converts measurement data into detection event data.
            @signature def convert(self, *, measurements: np.ndarray, sweep_bits: Optional[np.ndarray] = None, separate_observables: bool = False, append_observables: bool = False, bit_pack_result: bool = False) -> Union[np.ndarray, Tuple[np.ndarray, np.ndarray]]:

            Args:
                measurements: A numpy array containing measurement data.

                    The dtype of the array is used to determine if it is bit packed or not.
                    dtype=np.bool8 (unpacked data):
                        shape=(num_shots, circuit.num_measurements)
                    dtype=np.uint8 (bit packed data):
                        shape=(num_shots, math.ceil(circuit.num_measurements / 8))
                sweep_bits: Optional. A numpy array containing sweep data for the `sweep[k]`
                    controls in the circuit.

                    The dtype of the array is used to determine if it is bit packed or not.
                    dtype=np.bool8 (unpacked data):
                        shape=(num_shots, circuit.num_sweep_bits)
                    dtype=np.uint8 (bit packed data):
                        shape=(num_shots, math.ceil(circuit.num_sweep_bits / 8))
                separate_observables: Defaults to False. When set to True, two numpy arrays
                    are returned instead of one, with the second array containing the
                    observable flip data.
                append_observables: Defaults to False. When set to True, the observables in
                    the circuit are treated as if they were additional detectors. Their
                    results are appended to the end of the detection event data.
                bit_pack_result: Defaults to False. When set to True, the returned numpy
                    array contains bit packed data (dtype=np.uint8 with 8 bits per item)
                    instead of unpacked data (dtype=np.bool8).

            Returns:
                The detection event data and (optionally) observable data. The result is a
                single numpy array if separate_observables is false, otherwise it's a tuple
                of two numpy arrays.

                When returning two numpy arrays, the first array is the detection event data
                and the second is the observable flip data.

                The dtype of the returned arrays is np.bool8 if bit_pack_result is false,
                otherwise they're np.uint8 arrays.

                shape[0] of the array(s) is the number of shots.
                shape[1] of the array(s) is the number of bits per shot (divided by 8 if bit
                packed) (e.g. for just detection event data it would be
                circuit.num_detectors).

            Examples:
                >>> import stim
                >>> import numpy as np
                >>> converter = stim.Circuit('''
                ...    X 0
                ...    M 0 1
                ...    DETECTOR rec[-1]
                ...    DETECTOR rec[-2]
                ...    OBSERVABLE_INCLUDE(0) rec[-2]
                ... ''').compile_m2d_converter()
                >>> dets, obs = converter.convert(
                ...     measurements=np.array([[1, 0],
                ...                            [1, 0],
                ...                            [1, 0],
                ...                            [0, 0],
                ...                            [1, 0]], dtype=np.bool8),
                ...     separate_observables=True,
                ... )
                >>> dets
                array([[False, False],
                       [False, False],
                       [False, False],
                       [False,  True],
                       [False, False]])
                >>> obs
                array([[False],
                       [False],
                       [False],
                       [ True],
                       [False]])
        )DOC")
            .data());

    c.def(
        "__repr__",
        &CompiledMeasurementsToDetectionEventsConverter::repr,
        "Returns text that is a valid python expression evaluating to an equivalent "
        "`stim.CompiledMeasurementsToDetectionEventsConverter`.");
}
