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

#ifndef _STIM_DIAGRAM_TIMELINE_ASCII_DIAGRAM_TIMELINE_ASCII_DRAWER_H
#define _STIM_DIAGRAM_TIMELINE_ASCII_DIAGRAM_TIMELINE_ASCII_DRAWER_H

#include <iostream>

#include "stim/circuit/circuit.h"

namespace stim_draw_internal {

/// Identifies a location within a cell in a diagram with variable-sized columns and rows.
struct DiagramTimelineAsciiAlignedPos {
    size_t x;   /// The column that the cell is in.
    size_t y;   /// The row that the cell is in.
    float align_x;   /// Identifies a pixel column within the cell, proportionally from left to right.
    float align_y;   /// Identifies a pixel row within the cell, proportionally from top to bottom.

    DiagramTimelineAsciiAlignedPos(size_t x, size_t y, float align_x, float align_y);
    bool operator==(const DiagramTimelineAsciiAlignedPos &other) const;
    bool operator<(const DiagramTimelineAsciiAlignedPos &other) const;
    DiagramTimelineAsciiAlignedPos transposed() const;
};

/// Describes what to draw within a cell of a diagram with variable-sized columns and rows.
struct DiagramTimelineAsciiCellContents {
    /// The location of the cell, and the alignment to use for the text.
    DiagramTimelineAsciiAlignedPos center;
    /// The text to write.
    std::string label;

    DiagramTimelineAsciiCellContents(DiagramTimelineAsciiAlignedPos center, std::string label);
    DiagramTimelineAsciiCellContents transposed() const;
};

/// Describes sizes and offsets within a diagram with variable-sized columns and rows.
struct DiagramTimelineAsciiSizing {
    size_t num_x;
    size_t num_y;
    std::vector<size_t> x_spans;
    std::vector<size_t> y_spans;
    std::vector<size_t> x_offsets;
    std::vector<size_t> y_offsets;
};

struct LoopingIndexMap {
    std::vector<uint32_t> brute_force_data;
    void set(uint64_t index, stim::ConstPointerRange<uint64_t> offsets_per_iteration, stim::ConstPointerRange<uint64_t> iteration_counts, uint32_t value);
    uint32_t get(uint64_t index);
};

/// A diagram with a table structure where each column and row can have variable sizes.
///
/// The purpose of this struct is to provide a representation of circuits which is easier
/// for drawing code to consume, and to have better consistency between the various
/// diagrams.
struct DiagramTimelineAsciiDrawer {
    size_t cur_moment = 0;
    size_t cur_moment_is_used = false;
    uint64_t measure_offset = 0;
    uint64_t detector_offset = 0;
    size_t tick_start_moment = 0;
    std::vector<bool> cur_moment_used_flags;
    size_t num_qubits = 0;
    size_t num_ticks = 0;
    std::vector<std::vector<double>> latest_qubit_coords;
    std::vector<uint64_t> cur_loop_measurement_periods;
    std::vector<uint64_t> cur_loop_repeat_counts;
    std::vector<uint64_t> cur_loop_detector_periods;
    std::vector<std::vector<double>> cur_loop_shift_periods;
    std::vector<double> coord_shift;
    LoopingIndexMap m2q;

    /// What to draw in various cells.
    std::map<DiagramTimelineAsciiAlignedPos, DiagramTimelineAsciiCellContents> cells;
    /// Lines to draw in between cells.
    std::vector<std::pair<DiagramTimelineAsciiAlignedPos, DiagramTimelineAsciiAlignedPos>> lines;

    /// Sets the contents of a cell in the diagram.
    void add_cell(DiagramTimelineAsciiCellContents cell);
    /// Iterates over non-empty positions in the diagram.
    /// A position is non-empty if it has cell contents or a line end point.
    void for_each_pos(const std::function<void(DiagramTimelineAsciiAlignedPos pos)> &callback) const;
    /// Based on the current contents, pick sizes and offsets for rows and columns.
    DiagramTimelineAsciiSizing compute_sizing() const;

    /// Converts a circuit into a cell diagram.
    static DiagramTimelineAsciiDrawer from_circuit(const stim::Circuit &circuit);

    void render(std::ostream &out) const;
    std::string str() const;

    void start_next_moment();
    void reserve_drawing_room_for_targets(stim::ConstPointerRange<stim::GateTarget> targets);
    void reserve_drawing_room_for_targets(stim::GateTarget t1, stim::GateTarget t2);
    void reserve_drawing_room_for_targets(stim::GateTarget t);
    void write_rec_index(std::ostream &out, int64_t lookback_shift = 0);
    void write_det_index(std::ostream &out);
    void write_coord(std::ostream &out, size_t coord_index, double relative_coordinate);
    void write_coords(std::ostream &out, stim::ConstPointerRange<double> relative_coordinates);
    stim::GateTarget rec_to_qubit(const stim::GateTarget &target);
    stim::GateTarget pick_pseudo_target_representing_measurements(const stim::Operation &op);

    void do_tick();
    void do_two_qubit_gate_instance(const stim::Operation &op, const stim::GateTarget &target1, const stim::GateTarget &target2);
    void do_feedback(const std::string &gate, const stim::GateTarget &qubit_target, const stim::GateTarget &feedback_target);
    void do_single_qubit_gate_instance(const stim::Operation &op, const stim::GateTarget &target);
    void do_multi_qubit_gate_with_pauli_targets(const stim::Operation &op, stim::ConstPointerRange<stim::GateTarget> targets);
    void do_repeat_block(const stim::Circuit &circuit, const stim::Operation &op);
    void do_next_operation(const stim::Circuit &circuit, const stim::Operation &op);
    void do_circuit(const stim::Circuit &circuit);
    void do_mpp(const stim::Operation &op);
    void do_correlated_error(const stim::Operation &op);
    void do_else_correlated_error(const stim::Operation &op);
    void do_two_qubit_gate(const stim::Operation &op);
    void do_single_qubit_gate(const stim::Operation &op);
    void do_detector(const stim::Operation &op);
    void do_observable_include(const stim::Operation &op);
    void do_shift_coords(const stim::Operation &op);
    void do_qubit_coords(const stim::Operation &op);

    DiagramTimelineAsciiDrawer transposed() const;
};

std::ostream &operator<<(std::ostream &out, const DiagramTimelineAsciiDrawer &drawer);


}

#endif
