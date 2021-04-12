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

#include "compiled_detector_sampler.pybind.h"

#include "../circuit/circuit.pybind.h"
#include "../simulators/detection_simulator.h"
#include "../simulators/frame_simulator.h"
#include "../simulators/tableau_simulator.h"
#include "base.pybind.h"

CompiledDetectorSampler::CompiledDetectorSampler(Circuit circuit) : dets_obs(circuit), circuit(std::move(circuit)) {
}

pybind11::array_t<uint8_t> CompiledDetectorSampler::sample(
    size_t num_shots, bool prepend_observables, bool append_observables) {
    auto sample =
        detector_samples(circuit, dets_obs, num_shots, prepend_observables, append_observables, PYBIND_SHARED_RNG())
            .transposed();

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

    size_t n = dets_obs.detectors.size() + dets_obs.observables.size() * (prepend_observables + append_observables);

    void *ptr = bytes.data();
    ssize_t itemsize = sizeof(uint8_t);
    std::vector<ssize_t> shape{(ssize_t)num_shots, (ssize_t)n};
    std::vector<ssize_t> stride{(ssize_t)sample.num_minor_bits_padded(), 1};
    const std::string &format = pybind11::format_descriptor<uint8_t>::value;
    bool readonly = true;
    return pybind11::array_t<uint8_t>(pybind11::buffer_info(ptr, itemsize, format, 2, shape, stride, readonly));
}

pybind11::array_t<uint8_t> CompiledDetectorSampler::sample_bit_packed(
    size_t num_shots, bool prepend_observables, bool append_observables) {
    auto sample =
        detector_samples(circuit, dets_obs, num_shots, prepend_observables, append_observables, PYBIND_SHARED_RNG())
            .transposed();
    size_t n = dets_obs.detectors.size() + dets_obs.observables.size() * (prepend_observables + append_observables);

    void *ptr = sample.data.u8;
    ssize_t itemsize = sizeof(uint8_t);
    std::vector<ssize_t> shape{(ssize_t)num_shots, (ssize_t)(n + 7) / 8};
    std::vector<ssize_t> stride{(ssize_t)sample.num_minor_u8_padded(), 1};
    const std::string &format = pybind11::format_descriptor<uint8_t>::value;
    bool readonly = true;
    return pybind11::array_t<uint8_t>(pybind11::buffer_info(ptr, itemsize, format, 2, shape, stride, readonly));
}

std::string CompiledDetectorSampler::repr() const {
    std::stringstream result;
    result << "stim.CompiledDetectorSampler(";
    result << circuit_repr(circuit);
    result << ")";
    return result.str();
}

void pybind_compiled_detector_sampler(pybind11::module &m) {
    auto &&c = pybind11::class_<CompiledDetectorSampler>(
        m,
        "CompiledDetectorSampler",
        "An analyzed stabilizer circuit whose detection events can be sampled quickly."
    );

    c.def(pybind11::init<Circuit>());

    c.def(
        "sample",
        &CompiledDetectorSampler::sample,
        pybind11::arg("shots"),
        pybind11::kw_only(),
        pybind11::arg("prepend_observables") = false,
        pybind11::arg("append_observables") = false,
        clean_doc_string(u8R"DOC(
            Returns a numpy array containing a batch of detector samples from the circuit.

            The circuit must define the detectors using DETECTOR instructions. Observables defined by OBSERVABLE_INCLUDE
            instructions can also be included in the results as honorary detectors.

            Args:
                shots: The number of times to sample every detector in the circuit.
                prepend_observables: Defaults to false. When set, observables are included with the detectors and are
                    placed at the start of the results.
                append_observables: Defaults to false. When set, observables are included with the detectors and are
                    placed at the end of the results.

            Returns:
                A numpy array with `dtype=uint8` and `shape=(shots, n)` where
                `n = num_detectors + num_observables*(append_observables + prepend_observables)`.
                The bit for detection event `m` in shot `s` is at `result[s, m]`.
        )DOC").data()
    );

    c.def(
        "sample_bit_packed",
        &CompiledDetectorSampler::sample_bit_packed,
        pybind11::arg("shots"),
        pybind11::kw_only(),
        pybind11::arg("prepend_observables") = false,
        pybind11::arg("append_observables") = false,
        clean_doc_string(u8R"DOC(
            Returns a numpy array containing bit packed batch of detector samples from the circuit.

            The circuit must define the detectors using DETECTOR instructions. Observables defined by OBSERVABLE_INCLUDE
            instructions can also be included in the results as honorary detectors.

            Args:
                shots: The number of times to sample every detector in the circuit.
                prepend_observables: Defaults to false. When set, observables are included with the detectors and are
                    placed at the start of the results.
                append_observables: Defaults to false. When set, observables are included with the detectors and are
                    placed at the end of the results.

            Returns:
                A numpy array with `dtype=uint8` and `shape=(shots, n)` where
                `n = num_detectors + num_observables*(append_observables + prepend_observables)`.
                The bit for detection event `m` in shot `s` is at `result[s, (m // 8)] & 2**(m % 8)`.
        )DOC").data()
    );

    c.def(
        "__repr__",
        &CompiledDetectorSampler::repr
    );
}
