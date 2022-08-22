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

#ifndef _STIM_DRAW_DIAGRAM_H
#define _STIM_DRAW_DIAGRAM_H

#include <iostream>

#include "stim/circuit/circuit.h"

namespace stim_draw_internal {

size_t utf8_char_count(const std::string &s);

struct DiagramPos {
    size_t x;
    size_t y;
    size_t z;
    float align_x;
    float align_y;
    float align_z;

    DiagramPos(size_t x, size_t y, size_t z, float align_x, float align_y, float align_z);
    bool operator==(const DiagramPos &other) const;
    bool operator<(const DiagramPos &other) const;
};

struct DiagramBox {
    DiagramPos center;
    std::string label;
    std::vector<std::string> annotations;
    const char *stroke;
    DiagramBox(DiagramPos center, std::string label, std::vector<std::string> annotations, const char *stroke);
};

struct DiagramLine {
    DiagramPos p1;
    DiagramPos p2;
};

struct DiagramLayout {
    size_t num_x;
    size_t num_y;
    size_t num_z;
    std::vector<size_t> x_widths;
    std::vector<size_t> y_heights;
    std::vector<size_t> z_depths;
    std::vector<size_t> x_offsets;
    std::vector<size_t> y_offsets;
    std::vector<size_t> z_offsets;
};

struct Diagram {
    std::map<DiagramPos, DiagramBox> boxes;
    std::vector<DiagramLine> lines;

    void add_box(DiagramBox box);
    void for_each_pos(const std::function<void(DiagramPos pos)> &callback);
    void compactify();
    DiagramLayout to_layout();
};

Diagram to_diagram(const stim::Circuit &circuit);

}

#endif
