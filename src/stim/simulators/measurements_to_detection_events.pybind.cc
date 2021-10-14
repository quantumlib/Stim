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
#include "stim/py/base.pybind.h"
#include "stim/simulators/detection_simulator.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/simulators/measurements_to_detection_events.h"
#include "stim/simulators/tableau_simulator.h"

using namespace stim;

CompiledMeasurementsToDetectionEventsConverter::CompiledMeasurementsToDetectionEventsConverter(
    simd_bits ref_sample, Circuit circuit, bool skip_reference_sample)
    : skip_reference_sample(skip_reference_sample),
      ref_sample(ref_sample),
      circuit(circuit),
      circuit_num_measurements(circuit.count_measurements()),
      circuit_num_sweep_bits(circuit.count_sweep_bits()),
      circuit_num_detectors(circuit.count_detectors()),
      circuit_num_observables(circuit.count_observables()),
      circuit_num_qubits(circuit.count_qubits()) {
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

struct RaiiFile {
    FILE *f;
    RaiiFile(const char *path, const char *mode) {
        if (path == nullptr) {
            f = nullptr;
            return;
        }

        f = fopen(path, mode);
        if (f == nullptr) {
            std::stringstream ss;
            ss << "Failed to open '";
            ss << path;
            ss << "' for ";
            if (*mode == 'r') {
                ss << "reading.";
            } else {
                ss << "writing.";
            }
            throw std::invalid_argument(ss.str());
        }
    }
    RaiiFile(const RaiiFile &other) = delete;
    RaiiFile(RaiiFile &&other) = delete;
    ~RaiiFile() {
        if (f != nullptr) {
            fclose(f);
            f = nullptr;
        }
    }
};

void CompiledMeasurementsToDetectionEventsConverter::convert_file(
    const std::string &measurements_filepath,
    const std::string &measurements_format,
    const char *sweep_bits_filepath,
    const std::string &sweep_bits_format,
    const std::string &detection_events_filepath,
    const std::string &detection_events_format,
    bool append_observables) {
    auto format_in = format_to_enum(measurements_format);
    auto format_sweep_bits = format_to_enum(sweep_bits_format);
    auto format_out = format_to_enum(detection_events_format);
    RaiiFile file_in(measurements_filepath.data(), "r");
    RaiiFile sweep_bits_in(sweep_bits_filepath, "r");
    RaiiFile detections_out(detection_events_filepath.data(), "w");

    stim::stream_measurements_to_detection_events_helper(
        file_in.f,
        format_in,
        sweep_bits_in.f,
        format_sweep_bits,
        detections_out.f,
        format_out,
        circuit,
        append_observables,
        ref_sample,
        circuit_num_measurements,
        circuit_num_observables,
        circuit_num_detectors,
        circuit_num_qubits,
        circuit_num_sweep_bits);
}

pybind11::array_t<bool> CompiledMeasurementsToDetectionEventsConverter::convert(
    const pybind11::array_t<bool> &measurements, const pybind11::array_t<bool> &sweep_bits, bool append_observables) {
    size_t num_shots = measurements.shape(0);
    if (measurements.ndim() != 2) {
        throw std::invalid_argument("Need len(measurements.shape) == 2");
    }
    if ((size_t)measurements.shape(1) != circuit_num_measurements) {
        throw std::invalid_argument("Need measurements.shape[1] == circuit.num_measurements");
    }
    if (sweep_bits.ndim() != 0) {
        if (sweep_bits.ndim() != 2) {
            throw std::invalid_argument("Need len(sweep_bits.shape) == 2");
        }
        if ((size_t)sweep_bits.shape(1) != circuit_num_sweep_bits) {
            throw std::invalid_argument("Need sweep_bits.shape[1] == circuit.num_sweep_bits");
        }
        if ((size_t)sweep_bits.shape(0) != num_shots) {
            throw std::invalid_argument("Need sweep_bits.shape[0] == measurements.shape[0]");
        }
    }

    simd_bit_table measurements__minor_shot_index(circuit_num_measurements, num_shots);
    auto m = measurements.unchecked();
    for (size_t shot_index = 0; shot_index < num_shots; shot_index++) {
        for (size_t m_index = 0; m_index < circuit_num_measurements; m_index++) {
            measurements__minor_shot_index[m_index][shot_index] ^= (bool)m(shot_index, m_index);
        }
    }

    simd_bit_table sweep_bits__minor_shot_index(circuit_num_sweep_bits, num_shots);
    if (sweep_bits.ndim() != 0) {
        auto s = sweep_bits.unchecked();
        for (size_t shot_index = 0; shot_index < num_shots; shot_index++) {
            for (size_t b_index = 0; b_index < circuit_num_sweep_bits; b_index++) {
                sweep_bits__minor_shot_index[b_index][shot_index] ^= (bool)s(shot_index, b_index);
            }
        }
    }

    size_t num_output_bits = circuit_num_detectors + circuit_num_observables * append_observables;
    simd_bit_table out_detection_results__minor_shot_index(num_output_bits, num_shots);
    stim::measurements_to_detection_events_helper(
        measurements__minor_shot_index,
        sweep_bits__minor_shot_index,
        out_detection_results__minor_shot_index,
        circuit.aliased_noiseless_circuit(),
        ref_sample,
        append_observables,
        circuit_num_measurements,
        circuit_num_detectors,
        circuit_num_observables,
        circuit_num_qubits);

    std::vector<uint8_t> bytes;
    bytes.resize(num_output_bits * num_shots);
    size_t k = 0;
    for (size_t shot_index = 0; shot_index < num_shots; shot_index++) {
        for (size_t o_index = 0; o_index < num_output_bits; o_index++) {
            bytes[k++] = out_detection_results__minor_shot_index[o_index][shot_index];
        }
    }

    void *ptr = bytes.data();
    ssize_t itemsize = sizeof(uint8_t);
    std::vector<ssize_t> shape{(ssize_t)num_shots, (ssize_t)num_output_bits};
    std::vector<ssize_t> stride{(ssize_t)num_output_bits, 1};
    const std::string &format = pybind11::format_descriptor<bool>::value;
    bool readonly = true;
    return pybind11::array_t<bool>(pybind11::buffer_info(ptr, itemsize, format, 2, shape, stride, readonly));
}

pybind11::class_<CompiledMeasurementsToDetectionEventsConverter>
pybind_compiled_measurements_to_detection_events_converter_class(pybind11::module &m) {
    return pybind11::class_<CompiledMeasurementsToDetectionEventsConverter>(
        m,
        "CompiledMeasurementsToDetectionEventsConverter",
        "A tool for quickly converting measurements from an analyzed stabilizer circuit into detection events.");
}

CompiledMeasurementsToDetectionEventsConverter py_init_compiled_measurements_to_detection_events_converter(
    const Circuit &circuit, bool skip_reference_sample) {
    simd_bits ref_sample = skip_reference_sample ? simd_bits(circuit.count_measurements())
                                                 : TableauSimulator::reference_sample_circuit(circuit);
    return CompiledMeasurementsToDetectionEventsConverter(ref_sample, circuit, skip_reference_sample);
}

void pybind_compiled_measurements_to_detection_events_converter_methods(
    pybind11::class_<CompiledMeasurementsToDetectionEventsConverter> &c) {
    c.def(
        pybind11::init(&py_init_compiled_measurements_to_detection_events_converter),
        pybind11::arg("circuit"),
        pybind11::kw_only(),
        pybind11::arg("skip_reference_sample") = false,
        clean_doc_string(u8R"DOC(
            Creates a measurement-to-detection-events converter for the given circuit.

            The converter uses a noiseless reference sample, collected from the circuit using stim's Tableau simulator
            during initialization of the converter, as a baseline for determining what the expected value of a detector
            is.

            Note that the expected behavior of gauge detectors (detectors that are not actually deterministic under
            noiseless execution) can vary depending on the reference sample. Stim mitigates this by always generating
            the same reference sample for a given circuit.

            Args:
                circuit: The stim circuit to use for conversions.
                skip_reference_sample: Defaults to False. When set to True, the reference sample used by the converter
                    is initialized to all-zeroes instead of being collected from the circuit. This should only be used
                    if it's known that the all-zeroes sample is actually a possible result from the circuit (under
                    noiseless execution).

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
        pybind11::arg("append_observables"),
        clean_doc_string(u8R"DOC(
            Reads measurement data from a file, converts it, and writes the detection events to another file.

            Args:
                measurements_filepath: A file containing measurement data to be converted.
                measurements_format: The format the measurement data is stored in.
                    Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
                    Defaults to "01".
                detection_events_filepath: Where to save detection event data to.
                detection_events_format: The format to save the detection event data in.
                    Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
                    Defaults to "01".
                sweep_bits_filepath: Defaults to None. A file containing sweep data, or None.
                    When specified, sweep data (used for `sweep[k]` controls in the circuit, which can vary from shot to
                    shot) will be read from the given file.
                    When not specified, all sweep bits default to False and no sweep-controlled operations occur.
                sweep_bits_format: The format the sweep data is stored in.
                    Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
                    Defaults to "01".
                append_observables: When True, the observables in the circuit are included as part of the detection
                    event data. Specifically, they are treated as if they were additional detectors at the end of the
                    circuit. When False, observable data is not output.

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
        pybind11::arg("append_observables"),
        clean_doc_string(u8R"DOC(
            Reads measurement data from a file, converts it, and writes the detection events to another file.

            Args:
                measurements: A numpy array containing measurement data:
                    dtype=bool8
                    shape=(num_shots, circuit.num_measurements)
                sweep_bits_filepath: A numpy array containing sweep data for `sweep[k]` controls in the circuit:
                    dtype=bool8
                    shape=(num_shots, circuit.num_sweep_bits)
                    Defaults to None (all sweep bits False).
                append_observables: When True, the observables in the circuit are included as part of the detection
                    event data. Specifically, they are treated as if they were additional detectors at the end of the
                    circuit. When False, observable data is not output.

            Returns:
                The detection event data in a numpy array:
                    dtype=bool8
                    shape=(num_shots, circuit.num_detectors + circuit.num_observables * append_observables)

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
        "__repr__",
        &CompiledMeasurementsToDetectionEventsConverter::repr,
        "Returns text that is a valid python expression evaluating to an equivalent "
        "`stim.CompiledMeasurementsToDetectionEventsConverter`.");
}
