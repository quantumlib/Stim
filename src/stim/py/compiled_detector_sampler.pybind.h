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
#include "stim/simulators/frame_simulator.h"

namespace stim_pybind {

struct CompiledDetectorSampler {
    stim::CircuitStats circuit_stats;
    stim::Circuit circuit;
    stim::FrameSimulator<stim::MAX_BITWORD_WIDTH> frame_sim;

    CompiledDetectorSampler() = delete;
    CompiledDetectorSampler(const CompiledDetectorSampler &) = delete;
    CompiledDetectorSampler(CompiledDetectorSampler &&) = default;
    CompiledDetectorSampler(stim::Circuit circuit, std::mt19937_64 &&rng);
    pybind11::object sample_to_numpy(
        size_t num_shots,
        bool prepend_observables,
        bool append_observables,
        bool separate_observables,
        bool bit_packed,
        pybind11::object dets_out,
        pybind11::object obs_out);
    void sample_write(
        size_t num_samples,
        pybind11::object filepath_obj,
        std::string_view format,
        bool prepend_observables,
        bool append_observables,
        pybind11::object obs_out_filepath_obj,
        std::string_view obs_out_format);
    std::string repr() const;
};

pybind11::class_<CompiledDetectorSampler> pybind_compiled_detector_sampler(pybind11::module &m);
void pybind_compiled_detector_sampler_methods(pybind11::module &m, pybind11::class_<CompiledDetectorSampler> &c);
CompiledDetectorSampler py_init_compiled_detector_sampler(const stim::Circuit &circuit, const pybind11::object &seed);

}  // namespace stim_pybind

#endif
