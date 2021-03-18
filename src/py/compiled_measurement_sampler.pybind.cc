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

CompiledMeasurementSampler::CompiledMeasurementSampler(Circuit circuit)
    : ref(TableauSimulator::reference_sample_circuit(circuit)), circuit(std::move(circuit)) {
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

std::string CompiledMeasurementSampler::repr() const {
    std::stringstream result;
    result << "stim.CompiledMeasurementSampler(";
    result << circuit_repr(circuit);
    result << ")";
    return result.str();
}

void pybind_compiled_measurement_sampler(pybind11::module &m) {
    pybind11::class_<CompiledMeasurementSampler>(
        m, "CompiledMeasurementSampler", "An analyzed stabilizer circuit whose measurements can be sampled quickly.")
        .def(pybind11::init<Circuit>())
        .def(
            "sample", &CompiledMeasurementSampler::sample, R"DOC(
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
        )DOC",
            pybind11::arg("shots"))
        .def(
            "sample_bit_packed", &CompiledMeasurementSampler::sample_bit_packed, R"DOC(
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
        )DOC",
            pybind11::arg("shots"))
        .def("__repr__", &CompiledMeasurementSampler::repr);
}
