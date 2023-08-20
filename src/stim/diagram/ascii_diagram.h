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

#ifndef _STIM_DIAGRAM_TIMELINE_ASCII_DIAGRAM_ASCII_DIAGRAM_H
#define _STIM_DIAGRAM_TIMELINE_ASCII_DIAGRAM_ASCII_DIAGRAM_H

#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <vector>

namespace stim_draw_internal {

/// Identifies a location within a cell in a diagram with variable-sized columns and rows.
struct AsciiDiagramPos {
    size_t x;       /// The column that the cell is in.
    size_t y;       /// The row that the cell is in.
    float align_x;  /// Identifies a pixel column within the cell, proportionally from left to right.
    float align_y;  /// Identifies a pixel row within the cell, proportionally from top to bottom.

    AsciiDiagramPos(size_t x, size_t y, float align_x, float align_y);
    bool operator==(const AsciiDiagramPos &other) const;
    bool operator<(const AsciiDiagramPos &other) const;
    AsciiDiagramPos transposed() const;
};

/// Describes what to draw within a cell of a diagram with variable-sized columns and rows.
struct AsciiDiagramEntry {
    /// The location of the cell, and the alignment to use for the text.
    AsciiDiagramPos center;
    /// The text to write.
    std::string label;

    AsciiDiagramEntry(AsciiDiagramPos center, std::string label);
    AsciiDiagramEntry transposed() const;
};

struct AsciiDiagram {
    /// What to draw in various cells.
    std::map<AsciiDiagramPos, AsciiDiagramEntry> cells;
    /// Lines to draw in between cells.
    std::vector<std::pair<AsciiDiagramPos, AsciiDiagramPos>> lines;

    void add_entry(AsciiDiagramEntry cell);
    void for_each_pos(const std::function<void(AsciiDiagramPos pos)> &callback) const;
    AsciiDiagram transposed() const;
    void render(std::ostream &out) const;
    std::string str() const;
};
std::ostream &operator<<(std::ostream &out, const AsciiDiagram &drawer);

}  // namespace stim_draw_internal

#endif
