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

#ifndef _STIM_DIAGRAM_TIMELINE_ASCII_DIAGRAM_CIRCUIT_WITH_RESOLVED_TIMELINE_INFO_H
#define _STIM_DIAGRAM_TIMELINE_ASCII_DIAGRAM_CIRCUIT_WITH_RESOLVED_TIMELINE_INFO_H

#include <functional>
#include <iostream>

#include "lattice_map.h"
#include "stim/circuit/circuit.h"

namespace stim_draw_internal {

struct CircuitTimelineLoopData {
    uint64_t num_repetitions;
    uint64_t measurements_per_iteration;
    uint64_t detectors_per_iteration;
    uint64_t ticks_per_iteration;
    std::vector<double> shift_per_iteration;
};

struct CircuitTimelineHelper;

/// Changes w.r.t. normal circuit operations:
///     There is no broadcasting. Single-qubit gates have a single target, etc.
///     DETECTOR and OBSERVABLE_INCLUDE instructions start with a qubit pseudo-target as a location hint.
///     All coordinates have the effects SHIFT_COORDS folded in.
///     QUBIT_COORDS is treated as a single-qubit gate. Will only have one target.
struct ResolvedTimelineOperation {
    stim::GateType gate_type;
    stim::SpanRef<const double> args;
    stim::SpanRef<const stim::GateTarget> targets;
};

struct CircuitTimelineHelper {
    std::function<void(ResolvedTimelineOperation)> resolved_op_callback;
    std::function<void(CircuitTimelineLoopData)> start_repeat_callback;
    std::function<void(CircuitTimelineLoopData)> end_repeat_callback;
    std::vector<double> cur_coord_shift;
    uint64_t measure_offset = 0;
    uint64_t detector_offset = 0;
    uint64_t num_ticks_seen = 0;
    bool unroll_loops = false;
    std::vector<double> coord_workspace;
    std::vector<uint64_t> u64_workspace;
    std::vector<stim::GateTarget> targets_workspace;
    std::vector<std::vector<double>> latest_qubit_coords;
    std::vector<CircuitTimelineLoopData> cur_loop_nesting;
    LatticeMap measure_index_to_qubit;

    void do_atomic_operation(
        const stim::GateType gate_type,
        stim::SpanRef<const double> args,
        stim::SpanRef<const stim::GateTarget> targets);

    stim::GateTarget rec_to_qubit(const stim::GateTarget &target);
    stim::GateTarget pick_pseudo_target_representing_measurements(const stim::CircuitInstruction &op);
    void skip_loop_iterations(const CircuitTimelineLoopData &loop_data, uint64_t skipped_reps);
    void do_record_measure_result(uint32_t target_qubit);
    void do_repeat_block(const stim::Circuit &circuit, const stim::CircuitInstruction &op);
    void do_next_operation(const stim::Circuit &circuit, const stim::CircuitInstruction &op);
    void do_circuit(const stim::Circuit &circuit);
    void do_operation_with_target_combiners(const stim::CircuitInstruction &op);
    void do_multi_qubit_atomic_operation(const stim::CircuitInstruction &op);
    void do_two_qubit_gate(const stim::CircuitInstruction &op);
    void do_single_qubit_gate(const stim::CircuitInstruction &op);
    void do_detector(const stim::CircuitInstruction &op);
    void do_observable_include(const stim::CircuitInstruction &op);
    void do_shift_coords(const stim::CircuitInstruction &op);
    void do_qubit_coords(const stim::CircuitInstruction &op);
    stim::SpanRef<const double> shifted_coordinates_in_workspace(stim::SpanRef<const double> coords);
};

}  // namespace stim_draw_internal

#endif
