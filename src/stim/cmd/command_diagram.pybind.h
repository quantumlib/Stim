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

#ifndef _STIM_CMD_COMMAND_DIAGRAM_PYBIND_H
#define _STIM_CMD_COMMAND_DIAGRAM_PYBIND_H

#include <pybind11/pybind11.h>

#include "stim/circuit/circuit.h"

namespace stim_pybind {

enum DiagramType {
    DIAGRAM_TYPE_SVG,
    DIAGRAM_TYPE_TEXT,
};

struct DiagramHelper {
    DiagramType type;
    std::string content;
};

pybind11::class_<DiagramHelper> pybind_diagram(pybind11::module &m);
void pybind_diagram_methods(pybind11::module &m, pybind11::class_<DiagramHelper> &c);
DiagramHelper circuit_diagram(const stim::Circuit &self, const std::string &type, const pybind11::object &tick);

}  // namespace stim_pybind

#endif
