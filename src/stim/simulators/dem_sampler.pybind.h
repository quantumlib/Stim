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

#ifndef _STIM_SIMULATORS_DEM_SAMPLER_PYBIND_H
#define _STIM_SIMULATORS_DEM_SAMPLER_PYBIND_H

#include <pybind11/pybind11.h>

#include "stim/simulators/dem_sampler.h"

namespace stim_pybind {

struct PyDemSampler {
    std::mt19937_64 rng;
    stim::DemSampler dem_sampler;
    PyDemSampler(stim::DetectorErrorModel model, std::mt19937_64 rng, size_t min_stripes) :
        rng(std::move(rng)),
        dem_sampler(std::move(model), this->rng, min_stripes) {};
};

pybind11::class_<PyDemSampler> pybind_dem_sampler(pybind11::module &m);
void pybind_dem_sampler_methods(pybind11::module &m, pybind11::class_<PyDemSampler> &c);

}  // namespace stim_pybind

#endif
