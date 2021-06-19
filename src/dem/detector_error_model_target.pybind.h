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

#ifndef STIM_DETECTOR_ERROR_MODEL_TARGET_PYBIND_H
#define STIM_DETECTOR_ERROR_MODEL_TARGET_PYBIND_H

#include <pybind11/pybind11.h>

#include "detector_error_model.h"

struct ExposedDemTarget : stim_internal::DemTarget {
    std::string repr() const;
    ExposedDemTarget(stim_internal::DemTarget target);
    stim_internal::DemTarget internal() const;
    static ExposedDemTarget observable_id(uint32_t id);
    static ExposedDemTarget relative_detector_id(uint64_t id);
    static ExposedDemTarget separator();
};

void pybind_detector_error_model_target(pybind11::module &m);

#endif