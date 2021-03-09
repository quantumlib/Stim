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

    return pybind11::array_t<uint8_t>(pybind11::buffer_info(
        bytes.data(), sizeof(uint8_t), pybind11::format_descriptor<uint8_t>::value, 2,
        {num_samples, circuit.num_measurements}, {(long long)sample.num_minor_bits_padded(), (long long)1}, true));
}

pybind11::array_t<uint8_t> CompiledMeasurementSampler::sample_bit_packed(size_t num_samples) {
    auto sample = FrameSimulator::sample(circuit, ref, num_samples, PYBIND_SHARED_RNG());
    return pybind11::array_t<uint8_t>(pybind11::buffer_info(
        sample.data.u8, sizeof(uint8_t), pybind11::format_descriptor<uint8_t>::value, 2,
        {num_samples, (circuit.num_measurements + 7) / 8}, {(long long)sample.num_minor_u8_padded(), (long long)1},
        true));
}

std::string CompiledMeasurementSampler::str() const {
    std::stringstream result;
    result << "# reference sample: ";
    for (size_t k = 0; k < circuit.num_measurements; k++) {
        result << "01"[ref[k]];
    }
    result << "\n";
    result << circuit;
    return result.str();
}

void pybind_compiled_measurement_sampler(pybind11::module &m) {
    pybind11::class_<CompiledMeasurementSampler>(
        m, "CompiledMeasurementSampler", "An analyzed stabilizer circuit whose measurements can be sampled quickly.")
        .def(pybind11::init<Circuit>())
        .def(
            "sample", &CompiledMeasurementSampler::sample, R"DOC(
            Returns a numpy array containing a batch of measurement samples from the circuit.

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

            Args:
                shots: The number of times to sample every measurement in the circuit.

            Returns:
                A numpy array with `dtype=uint8` and `shape=(shots, (num_measurements + 7) // 8)`.
                The bit for measurement `m` in shot `s` is at `result[s, (m // 8)] & 2**(m % 8)`.
        )DOC",
            pybind11::arg("shots"))
        .def("__str__", &CompiledMeasurementSampler::str);
}
