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

#ifndef _STIM_DIAGRAM_TIMELINE_TIMELINE_SVG_DRAWER_H
#define _STIM_DIAGRAM_TIMELINE_TIMELINE_SVG_DRAWER_H

#include <iostream>

#include "stim/circuit/circuit.h"
#include "stim/diagram/ascii_diagram.h"
#include "stim/diagram/circuit_timeline_helper.h"
#include "stim/diagram/detector_slice/detector_slice_set.h"
#include "stim/diagram/gate_data_svg.h"
#include "stim/diagram/lattice_map.h"
#include "stim/util_bot/str_util.h"

namespace stim_draw_internal {

enum class DiagramTimelineSvgDrawerMode {
    SVG_MODE_TIMELINE = 0,
    SVG_MODE_TIME_SLICE = 1,
    SVG_MODE_TIME_DETECTOR_SLICE = 2,
};

struct DiagramTimelineSvgDrawer {
    std::ostream &svg_out;
    CircuitTimelineHelper resolver;

    size_t cur_moment = 0;
    uint16_t moment_width = 1;
    size_t cur_moment_is_used = false;
    size_t tick_start_moment = 0;
    std::vector<bool> cur_moment_used_flags;
    size_t num_qubits = 0;
    bool has_ticks = false;
    uint64_t min_tick = 0;
    uint64_t max_tick = UINT64_MAX;
    uint64_t num_cols = UINT64_MAX;
    uint64_t num_rows = 1;
    DiagramTimelineSvgDrawerMode mode;
    DetectorSliceSet detector_slice_set;
    FlattenedCoords coord_sys;
    std::map<std::string_view, SvgGateData> gate_data_map;

    DiagramTimelineSvgDrawer(std::ostream &out, size_t num_qubits, bool has_ticks);

    /// Converts a circuit into a cell diagram.
    static void make_diagram_write_to(
        const stim::Circuit &circuit,
        std::ostream &svg_out,
        uint64_t tick_slice_start,
        uint64_t tick_slice_num,
        DiagramTimelineSvgDrawerMode mode,
        stim::SpanRef<const CoordFilter> det_coord_filter,
        size_t num_rows = 0);

    void do_start_repeat(const CircuitTimelineLoopData &loop_data);
    void do_end_repeat(const CircuitTimelineLoopData &loop_data);
    void start_next_moment();
    void reserve_drawing_room_for_targets(stim::SpanRef<const stim::GateTarget> targets);
    void write_rec_index(std::ostream &out, int64_t lookback_shift = -1);
    void write_det_index(std::ostream &out);
    void write_coord(std::ostream &out, size_t coord_index, double relative_coordinate);
    void write_coords(std::ostream &out, stim::SpanRef<const double> relative_coordinates);
    size_t m2x(size_t m) const;
    size_t q2y(size_t q) const;
    Coord<2> q2xy(size_t q) const;
    Coord<2> qt2xy(uint64_t tick, uint64_t moment_delta, size_t q) const;
    void draw_annotated_gate(float cx, float cy, const SvgGateData &data, stim::SpanRef<const double> end_args);

    void draw_xswap_control(float cx, float cy);
    void draw_zswap_control(float cx, float cy);
    void draw_x_control(float cx, float cy);
    void draw_y_control(float cx, float cy);
    void draw_z_control(float cx, float cy);
    void draw_swap_control(float cx, float cy);
    void draw_iswap_control(float cx, float cy, bool inverse);
    void draw_generic_box(float cx, float cy, std::string_view text, stim::SpanRef<const double> end_args);
    void draw_two_qubit_gate_end_point(float cx, float cy, std::string_view type, stim::SpanRef<const double> args);
    void draw_rec(float cx, float cy);

    void do_resolved_operation(const ResolvedTimelineOperation &op);
    void do_tick();
    void do_two_qubit_gate_instance(const ResolvedTimelineOperation &op);
    void do_feedback(
        std::string_view gate, const stim::GateTarget &qubit_target, const stim::GateTarget &feedback_target);
    void do_single_qubit_gate_instance(const ResolvedTimelineOperation &op);
    void do_multi_qubit_gate_with_pauli_targets(const ResolvedTimelineOperation &op);
    void do_multi_qubit_gate_with_paired_pauli_targets(const ResolvedTimelineOperation &op);
    void do_mpp(const ResolvedTimelineOperation &op);
    void do_spp(const ResolvedTimelineOperation &op);
    void do_correlated_error(const ResolvedTimelineOperation &op);
    void do_qubit_coords(const ResolvedTimelineOperation &op);
    void do_else_correlated_error(const ResolvedTimelineOperation &op);
    void do_detector(const ResolvedTimelineOperation &op);
    void do_observable_include(const ResolvedTimelineOperation &op);
};

}  // namespace stim_draw_internal

#endif
