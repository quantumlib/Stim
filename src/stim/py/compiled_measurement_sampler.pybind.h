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

#ifndef _STIM_PY_COMPILED_MEASUREMENT_SAMPLER_PYBIND_H
#define _STIM_PY_COMPILED_MEASUREMENT_SAMPLER_PYBIND_H

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "stim/circuit/circuit.h"
#include "stim/mem/simd_bits.h"

namespace stim_pybind {

struct CompiledMeasurementSampler {
    const stim::simd_bits<stim::MAX_BITWORD_WIDTH> ref_sample;
    const stim::Circuit circuit;
    const bool skip_reference_sample;
    std::shared_ptr<std::mt19937_64> prng;
    CompiledMeasurementSampler() = delete;
    CompiledMeasurementSampler(const CompiledMeasurementSampler &) = delete;
    CompiledMeasurementSampler(CompiledMeasurementSampler &&) = default;
    CompiledMeasurementSampler(
        stim::simd_bits<stim::MAX_BITWORD_WIDTH> ref_sample,
        stim::Circuit circuit,
        bool skip_reference_sample,
        std::shared_ptr<std::mt19937_64> prng);
    pybind11::object sample_to_numpy(size_t num_shots, bool bit_packed);
    void sample_write(size_t num_samples, const std::string &filepath, const std::string &format);
    std::string repr() const;
};

pybind11::class_<CompiledMeasurementSampler> pybind_compiled_measurement_sampler(pybind11::module &m);
    void pybind_compiled_measurement_sampler_methods(pybind11::module &m, pybind11::class_<CompiledMeasurementSampler> &c);
CompiledMeasurementSampler py_init_compiled_sampler(
    const stim::Circuit &circuit, bool skip_reference_sample, const pybind11::object &seed);

}  // namespace stim_pybind

#endif
