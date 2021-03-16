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

#ifndef COMPILED_DETECTOR_SAMPLER_PYBIND_H
#define COMPILED_DETECTOR_SAMPLER_PYBIND_H

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "../circuit/circuit.h"
#include "../simd/simd_bits.h"

void pybind_compiled_detector_sampler(pybind11::module &m);

struct CompiledDetectorSampler {
    const DetectorsAndObservables dets_obs;
    const Circuit circuit;
    CompiledDetectorSampler(Circuit circuit);
    pybind11::array_t<uint8_t> sample(size_t num_shots, bool append_observables);
    pybind11::array_t<uint8_t> sample_bit_packed(size_t num_shots, bool append_observables);
    std::string repr() const;
};

#endif
