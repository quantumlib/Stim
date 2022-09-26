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

#ifndef _STIM_PY_COMPILED_DETECTOR_SAMPLER_PYBIND_H
#define _STIM_PY_COMPILED_DETECTOR_SAMPLER_PYBIND_H

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "stim/circuit/circuit.h"
#include "stim/mem/simd_bits.h"

namespace stim_pybind {

struct CompiledDetectorSampler {
    const stim::DetectorsAndObservables dets_obs;
    const stim::Circuit circuit;
    std::shared_ptr<std::mt19937_64> prng;
    CompiledDetectorSampler() = delete;
    CompiledDetectorSampler(const CompiledDetectorSampler &) = delete;
    CompiledDetectorSampler(CompiledDetectorSampler &&) = default;
    CompiledDetectorSampler(stim::Circuit circuit, std::shared_ptr<std::mt19937_64> prng);
    pybind11::object sample_to_numpy(size_t num_shots, bool prepend_observables, bool append_observables, bool bit_packed);
    void sample_write(
        size_t num_samples,
        const std::string &filepath,
        const std::string &format,
        bool prepend_observables,
        bool append_observables,
        const char *obs_out_filepath,
        const std::string &obs_out_format);
    std::string repr() const;
};

pybind11::class_<CompiledDetectorSampler> pybind_compiled_detector_sampler(pybind11::module &m);
void pybind_compiled_detector_sampler_methods(pybind11::module &m, pybind11::class_<CompiledDetectorSampler> &c);
CompiledDetectorSampler py_init_compiled_detector_sampler(const stim::Circuit &circuit, const pybind11::object &seed);

}  // namespace stim_pybind

#endif
