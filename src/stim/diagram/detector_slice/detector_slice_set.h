/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _STIM_DIAGRAM_DETECTOR_SLICE_DETECTOR_SLICE_SET_H
#define _STIM_DIAGRAM_DETECTOR_SLICE_DETECTOR_SLICE_SET_H

#include <iostream>

#include "stim/circuit/circuit.h"
#include "stim/dem/detector_error_model.h"
#include "stim/diagram/coord.h"

namespace stim_draw_internal {

struct DetectorSliceSet {
    uint64_t num_qubits;
    std::map<uint64_t, std::vector<double>> coordinates;
    std::map<uint64_t, std::vector<double>> detector_coordinates;
    std::map<stim::DemTarget, std::vector<stim::GateTarget>> slices;

    /// Args:
    ///     circuit: The circuit to make a detector slice diagram from.
    ///     tick_index: The tick to target. tick_index=0 is the start of the
    ///         circuit. tick_index=1 is the first TICK instruction, and so
    ///         forth.
    static DetectorSliceSet from_circuit_tick(const stim::Circuit &circuit, uint64_t tick_index);

    std::set<uint64_t> used_qubits() const;
    std::string str() const;

    void write_text_diagram_to(std::ostream &out) const;
    void write_svg_diagram_to(std::ostream &out) const;
};

struct FlattenedCoords {
    std::vector<Coord<2>> qubit_coords;
    std::map<uint64_t, Coord<2>> det_coords;
    Coord<2> size;

    static FlattenedCoords from(const DetectorSliceSet &set, float desired_unit_distance);
};

std::ostream &operator<<(std::ostream &out, const DetectorSliceSet &slice);

}  // namespace stim_draw_internal

#endif
