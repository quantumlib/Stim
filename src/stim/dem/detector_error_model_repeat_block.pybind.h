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

#ifndef _STIM_DEM_DETECTOR_ERROR_MODEL_REPEAT_BLOCK_PYBIND_H
#define _STIM_DEM_DETECTOR_ERROR_MODEL_REPEAT_BLOCK_PYBIND_H

#include <pybind11/pybind11.h>

#include "stim/dem/detector_error_model.h"

struct ExposedDemRepeatBlock {
    uint64_t repeat_count;
    stim::DetectorErrorModel body;

    stim::DetectorErrorModel body_copy();
    std::string repr() const;
    bool operator==(const ExposedDemRepeatBlock &other) const;
    bool operator!=(const ExposedDemRepeatBlock &other) const;
};

void pybind_detector_error_model_repeat_block(pybind11::module &m);

#endif