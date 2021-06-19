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

#ifndef STIM_DETECTOR_ERROR_MODEL_INSTRUCTION_PYBIND_H
#define STIM_DETECTOR_ERROR_MODEL_INSTRUCTION_PYBIND_H

#include <pybind11/pybind11.h>

#include "detector_error_model.h"
#include "detector_error_model_target.pybind.h"

struct ExposedDemInstruction {
    std::vector<double> arguments;
    std::vector<stim_internal::DemTarget> targets;
    stim_internal::DemInstructionType type;

    std::vector<pybind11::object> exposed_targets() const;
    stim_internal::DemInstruction as_dem_instruction() const;
    std::string type_name() const;
    std::string str() const;
    std::string repr() const;
    bool operator==(const ExposedDemInstruction &other) const;
    bool operator!=(const ExposedDemInstruction &other) const;
};

void pybind_detector_error_model_instruction(pybind11::module &m);

#endif