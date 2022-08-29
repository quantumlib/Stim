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

#ifndef _STIM_DEM_DETECTOR_ERROR_MODEL_INSTRUCTION_PYBIND_H
#define _STIM_DEM_DETECTOR_ERROR_MODEL_INSTRUCTION_PYBIND_H

#include <pybind11/pybind11.h>

#include "stim/dem/detector_error_model.h"


namespace stim_pybind {

struct ExposedDemInstruction {
    std::vector<double> arguments;
    std::vector<stim::DemTarget> targets;
    stim::DemInstructionType type;

    std::vector<double> args_copy() const;
    std::vector<pybind11::object> targets_copy() const;
    stim::DemInstruction as_dem_instruction() const;
    std::string type_name() const;
    std::string str() const;
    std::string repr() const;
    bool operator==(const ExposedDemInstruction &other) const;
    bool operator!=(const ExposedDemInstruction &other) const;
};

pybind11::class_<ExposedDemInstruction> pybind_detector_error_model_instruction(pybind11::module &m);
void pybind_detector_error_model_instruction_methods(pybind11::module &m, pybind11::class_<ExposedDemInstruction> &c);

}
#endif
