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

/// Splits a two qubit gate into two end pieces, which can be drawn independently.
std::pair<std::string, std::string> two_qubit_gate_pieces(const std::string &name, bool keep_it_short);

/// Identifies a location within a cell in a diagram with variable-sized columns and rows.
struct DiagramTimelineAsciiAlignedPos {
    size_t x;   /// The column that the cell is in.
    size_t y;   /// The row that the cell is in.
    float align_x;   /// Identifies a pixel column within the cell, proportionally from left to right.
    float align_y;   /// Identifies a pixel row within the cell, proportionally from top to bottom.

    DiagramTimelineAsciiAlignedPos(size_t x, size_t y, float align_x, float align_y);
    bool operator==(const DiagramTimelineAsciiAlignedPos &other) const;
    bool operator<(const DiagramTimelineAsciiAlignedPos &other) const;
};

/// Describes what to draw within a cell of a diagram with variable-sized columns and rows.
struct DiagramTimelineAsciiCellContents {
    /// The location of the cell, and the alignment to use for the text.
    DiagramTimelineAsciiAlignedPos center;
    /// The text to write.
    std::string label;

    DiagramTimelineAsciiCellContents(DiagramTimelineAsciiAlignedPos center, std::string label);
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

/// A diagram with a table structure where each column and row can have variable sizes.
///
/// The purpose of this struct is to provide a representation of circuits which is easier
/// for drawing code to consume, and to have better consistency between the various
/// diagrams.
struct DiagramTimelineAsciiDrawer {
    size_t cur_moment = 0;
    size_t cur_moment_num_used = 0;
    size_t measure_offset = 0;
    size_t tick_start_moment = 0;
    std::vector<bool> cur_moment_used_flags;
    size_t num_qubits = 0;
    size_t num_ticks = 0;

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
    void draw_tick();
    void draw_two_qubit_gate(const stim::Operation &op, const stim::GateTarget &target1, const stim::GateTarget &target2);
    void draw_feedback(const std::string &gate, const stim::GateTarget &qubit_target, const stim::GateTarget &feedback_target);
    void draw_single_qubit_gate(const stim::Operation &op, const stim::GateTarget &target);
    void draw_measurement_gate(const stim::Operation &op, stim::ConstPointerRange<stim::GateTarget> targets);
    void draw_repeat_block(const stim::Circuit &circuit, const stim::Operation &op);
    void draw_next_operation(const stim::Circuit &circuit, const stim::Operation &op);
    void draw_circuit(const stim::Circuit &circuit);
};

std::ostream &operator<<(std::ostream &out, const DiagramTimelineAsciiDrawer &drawer);


}

#endif
