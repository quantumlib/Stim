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

#ifndef STIM_CIRCUIT_GATE_TARGET_PYBIND_H
#define STIM_CIRCUIT_GATE_TARGET_PYBIND_H

#include <cstdint>
#include <pybind11/pybind11.h>
#include <string>

void pybind_circuit_gate_target(pybind11::module &m);

struct GateTarget {
    uint32_t target;
    GateTarget(uint32_t target);
    GateTarget(pybind11::object init_target);
    int32_t value() const;
    bool is_x_target() const;
    bool is_y_target() const;
    bool is_z_target() const;
    bool is_inverted_result_target() const;
    bool is_measurement_record_target() const;
    bool operator==(const GateTarget &other) const;
    bool operator!=(const GateTarget &other) const;
    std::string repr_inner() const;
    std::string repr() const;
};

#endif