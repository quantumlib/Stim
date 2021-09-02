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

#include "compiled_measurement_sampler.pybind.h"

#include "../circuit/circuit.pybind.h"
#include "../simulators/detection_simulator.h"
#include "../simulators/frame_simulator.h"
#include "../simulators/tableau_simulator.h"
#include "base.pybind.h"

using namespace stim_internal;

CompiledMeasurementSampler::CompiledMeasurementSampler(Circuit circuit, bool skip_reference_sample)
    : ref(skip_reference_sample ? simd_bits(circuit.count_measurements())
                                : TableauSimulator::reference_sample_circuit(circuit)),
      circuit(circuit),
      skip_reference_sample(skip_reference_sample) {
}

pybind11::array_t<uint8_t> CompiledMeasurementSampler::sample(size_t num_samples) {
    auto sample = FrameSimulator::sample(circuit, ref, num_samples, PYBIND_SHARED_RNG());

    const simd_bits &flat = sample.data;
    std::vector<uint8_t> bytes;
    bytes.reserve(flat.num_bits_padded());
    auto *end = flat.u64 + flat.num_u64_padded();
    for (auto u64 = flat.u64; u64 != end; u64++) {
        auto v = *u64;
        for (size_t k = 0; k < 64; k++) {
            bytes.push_back((v >> k) & 1);
        }
    }

    void *ptr = bytes.data();
    ssize_t itemsize = sizeof(uint8_t);
    std::vector<ssize_t> shape{(ssize_t)num_samples, (ssize_t)circuit.count_measurements()};
    std::vector<ssize_t> stride{(ssize_t)sample.num_minor_bits_padded(), 1};
    const std::string &format = pybind11::format_descriptor<uint8_t>::value;
    bool readonly = true;
    return pybind11::array_t<uint8_t>(pybind11::buffer_info(ptr, itemsize, format, 2, shape, stride, readonly));
}

pybind11::array_t<uint8_t> CompiledMeasurementSampler::sample_bit_packed(size_t num_samples) {
    auto sample = FrameSimulator::sample(circuit, ref, num_samples, PYBIND_SHARED_RNG());

    void *ptr = sample.data.u8;
    ssize_t itemsize = sizeof(uint8_t);
    std::vector<ssize_t> shape{(ssize_t)num_samples, (ssize_t)(circuit.count_measurements() + 7) / 8};
    std::vector<ssize_t> stride{(ssize_t)sample.num_minor_u8_padded(), 1};
    const std::string &format = pybind11::format_descriptor<uint8_t>::value;
    bool readonly = true;
    return pybind11::array_t<uint8_t>(pybind11::buffer_info(ptr, itemsize, format, 2, shape, stride, readonly));
}

void CompiledMeasurementSampler::sample_write(
    size_t num_samples, const std::string &filepath, const std::string &format) {
    auto f = format_to_enum(format);
    FILE *out = fopen(filepath.data(), "w");
    if (out == nullptr) {
        throw std::invalid_argument("Failed to open '" + filepath + "' to write.");
    }
    FrameSimulator::sample_out(circuit, ref, num_samples, out, f, PYBIND_SHARED_RNG());
    fclose(out);
}

std::string CompiledMeasurementSampler::repr() const {
    std::stringstream result;
    result << "stim.CompiledMeasurementSampler(";
    result << circuit_repr(circuit);
    if (skip_reference_sample) {
        result << ", skip_reference_sample=True";
    }
    result << ")";
    return result.str();
}

pybind11::class_<CompiledMeasurementSampler> pybind_compiled_measurement_sampler_class(pybind11::module &m) {
    return pybind11::class_<CompiledMeasurementSampler>(
        m, "CompiledMeasurementSampler", "An analyzed stabilizer circuit whose measurements can be sampled quickly.");
}

void pybind_compiled_measurement_sampler_methods(pybind11::class_<CompiledMeasurementSampler> &c) {
    c.def(
        pybind11::init<Circuit, bool>(),
        pybind11::arg("circuit"),
        pybind11::kw_only(),
        pybind11::arg("skip_reference_sample") = false,
        clean_doc_string(u8R"DOC(
            Creates a measurement sampler for the given circuit.

            The sampler uses a noiseless reference sample, collected from the circuit using stim's Tableau simulator
            during initialization of the sampler, as a baseline for deriving more samples using an error propagation
            simulator.

            Args:
                circuit: The stim circuit to sample from.
                skip_reference_sample: Defaults to False. When set to True, the reference sample used by the sampler is
                    initialized to all-zeroes instead of being collected from the circuit. This means that the results
                    returned by the sampler are actually whether or not each measurement was *flipped*, instead of true
                    measurement results.

                    Forcing an all-zero reference sample is useful when you are only interested in error propagation and
                    don't want to have to deal with the fact that some measurements want to be On when no errors occur.
                    It is also useful when you know for sure that the all-zero result is actually a possible result from
                    the circuit (under noiseless execution), meaning it is a valid reference sample as good as any
                    other. Computing the reference sample is the most time consuming and memory intensive part of
                    simulating the circuit, so promising that the simulator can safely skip that step is an effective
                    optimization.

            Returns:
                A numpy array with `dtype=uint8` and `shape=(shots, num_measurements)`.
                The bit for measurement `m` in shot `s` is at `result[s, m]`.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    X 0   2 3
                ...    M 0 1 2 3
                ... ''')
                >>> s = c.compile_sampler()
                >>> s.sample(shots=1)
                array([[1, 0, 1, 1]], dtype=uint8)
        )DOC")
            .data());

    c.def(
        "sample",
        &CompiledMeasurementSampler::sample,
        pybind11::arg("shots"),
        clean_doc_string(u8R"DOC(
            Returns a numpy array containing a batch of measurement samples from the circuit.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    X 0   2 3
                ...    M 0 1 2 3
                ... ''')
                >>> s = c.compile_sampler()
                >>> s.sample(shots=1)
                array([[1, 0, 1, 1]], dtype=uint8)

            Args:
                shots: The number of times to sample every measurement in the circuit.

            Returns:
                A numpy array with `dtype=uint8` and `shape=(shots, num_measurements)`.
                The bit for measurement `m` in shot `s` is at `result[s, m]`.
        )DOC")
            .data());

    c.def(
        "sample_bit_packed",
        &CompiledMeasurementSampler::sample_bit_packed,
        pybind11::arg("shots"),
        clean_doc_string(u8R"DOC(
            Returns a numpy array containing a bit packed batch of measurement samples from the circuit.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    X 0 1 2 3 4 5 6 7     10
                ...    M 0 1 2 3 4 5 6 7 8 9 10
                ... ''')
                >>> s = c.compile_sampler()
                >>> s.sample_bit_packed(shots=1)
                array([[255,   4]], dtype=uint8)

            Args:
                shots: The number of times to sample every measurement in the circuit.

            Returns:
                A numpy array with `dtype=uint8` and `shape=(shots, (num_measurements + 7) // 8)`.
                The bit for measurement `m` in shot `s` is at `result[s, (m // 8)] & 2**(m % 8)`.
        )DOC")
            .data());

    c.def(
        "sample_write",
        &CompiledMeasurementSampler::sample_write,
        pybind11::arg("shots"),
        pybind11::kw_only(),
        pybind11::arg("filepath"),
        pybind11::arg("format"),
        clean_doc_string(u8R"DOC(
            Samples measurements from the circuit and writes them to a file.

            Examples:
                >>> import stim
                >>> import tempfile
                >>> with tempfile.TemporaryDirectory() as d:
                ...     path = f"{d}/tmp.dat"
                ...     c = stim.Circuit('''
                ...         X 0   2 3
                ...         M 0 1 2 3
                ...     ''')
                ...     c.compile_sampler().sample_write(5, filepath=path, format="01")
                ...     with open(path) as f:
                ...         print(f.read(), end='')
                1011
                1011
                1011
                1011
                1011

            Args:
                shots: The number of times to sample every measurement in the circuit.
                filepath: The file to write the results to.
                format: The output format to write the results with.
                    Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".

            Returns:
                None.
        )DOC")
            .data());

    c.def(
        "__repr__",
        &CompiledMeasurementSampler::repr,
        "Returns text that is a valid python expression evaluating to an equivalent "
        "`stim.CompiledMeasurementSampler`.");
}
