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

#include <functional>
#include <iostream>

#include "stim/circuit/circuit.h"
#include "stim/dem/detector_error_model.h"
#include "stim/diagram/coord.h"

namespace stim_draw_internal {

struct CoordFilter {
    std::vector<double> coordinates{};
    bool use_target = false;
    stim::DemTarget exact_target{};

    bool matches(stim::SpanRef<const double> coords, stim::DemTarget target) const;
    static CoordFilter parse_from(std::string_view data);
};

struct DetectorSliceSet {
    uint64_t num_qubits;
    uint64_t min_tick;
    uint64_t num_ticks;
    /// Qubit ID -> qubit coordinates
    std::map<uint64_t, std::vector<double>> coordinates;
    /// DemTarget value -> detector coordinates
    std::map<uint64_t, std::vector<double>> detector_coordinates;
    /// (tick, DemTarget) -> terms in the slice
    std::map<std::pair<uint64_t, stim::DemTarget>, std::vector<stim::GateTarget>> slices;
    /// (tick, DemTarget) -> anticommutations in the slice
    std::map<std::pair<uint64_t, stim::DemTarget>, std::vector<stim::GateTarget>> anticommutations;

    /// Args:
    ///     circuit: The circuit to make a detector slice diagram from.
    ///     tick_index: The tick to target. tick_index=0 is the start of the
    ///         circuit. tick_index=1 is the first TICK instruction, and so
    ///         forth.
    ///     coord_filter: Detectors that fail to match these coordinates
    ///         are excluded.
    static DetectorSliceSet from_circuit_ticks(
        const stim::Circuit &circuit,
        uint64_t start_tick,
        uint64_t num_ticks,
        stim::SpanRef<const CoordFilter> coord_filter);

    std::set<uint64_t> used_qubits() const;
    std::string str() const;

    void write_text_diagram_to(std::ostream &out) const;
    void write_svg_diagram_to(std::ostream &out, size_t num_rows = 0) const;
    void write_svg_contents_to(
        std::ostream &out,
        const std::function<Coord<2>(uint32_t qubit)> &unscaled_coords,
        const std::function<Coord<2>(uint64_t tick, uint32_t qubit)> &coords,
        uint64_t end_tick,
        size_t scale) const;
};

double inv_space_fill_transform(Coord<2> a);

struct FlattenedCoords {
    std::vector<Coord<2>> unscaled_qubit_coords;
    std::vector<Coord<2>> qubit_coords;
    std::map<uint64_t, Coord<2>> det_coords;
    Coord<2> size;
    float unit_distance;

    static FlattenedCoords from(const DetectorSliceSet &set, float desired_unit_distance);
};
Coord<2> pick_polygon_center(stim::SpanRef<const Coord<2>> coords);
bool is_colinear(Coord<2> a, Coord<2> b, Coord<2> c, float atol);

std::ostream &operator<<(std::ostream &out, const DetectorSliceSet &slice);
std::ostream &operator<<(std::ostream &out, const CoordFilter &filter);

}  // namespace stim_draw_internal

#endif
