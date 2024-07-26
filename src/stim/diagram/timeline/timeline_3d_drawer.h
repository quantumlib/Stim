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

#ifndef _STIM_DIAGRAM_TIMELINE_TIMELINE_3D_DRAWER_H
#define _STIM_DIAGRAM_TIMELINE_TIMELINE_3D_DRAWER_H

#include <iostream>

#include "stim/diagram/basic_3d_diagram.h"
#include "stim/diagram/circuit_timeline_helper.h"

namespace stim_draw_internal {

struct DiagramTimeline3DDrawer {
    CircuitTimelineHelper resolver;

    std::vector<size_t> loop_start_moment_stack;
    Basic3dDiagram diagram_out;
    size_t cur_moment = 0;
    size_t cur_moment_is_used = false;
    size_t tick_start_moment = 0;
    std::vector<bool> cur_moment_used_flags;
    size_t num_qubits = 0;
    bool has_ticks = false;
    std::vector<Coord<2>> qubit_coords;
    std::pair<Coord<2>, Coord<2>> coord_bounds;

    DiagramTimeline3DDrawer(size_t num_qubits, bool has_ticks);

    static Basic3dDiagram circuit_to_basic_3d_diagram(const stim::Circuit &circuit);

    void do_start_repeat(const CircuitTimelineLoopData &loop_data);
    void do_end_repeat(const CircuitTimelineLoopData &loop_data);
    void start_next_moment();
    void reserve_drawing_room_for_targets(stim::SpanRef<const stim::GateTarget> targets);
    Coord<3> mq2xyz(size_t m, size_t q) const;
    void draw_two_qubit_gate_end_point(Coord<3> center, std::string_view type);
    void draw_gate_connecting_line(Coord<3> a, Coord<3> b);

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
