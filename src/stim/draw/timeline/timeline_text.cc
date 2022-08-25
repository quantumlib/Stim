#include "stim/draw/timeline/timeline_layout.h"
#include "stim/draw/timeline/timeline_text.h"

using namespace stim;
using namespace stim_draw_internal;

std::string stim::circuit_diagram_timeline_text(const Circuit &circuit) {
    CellDiagram diagram = CellDiagram::from_circuit(circuit);
    CellDiagramSizing layout = diagram.compute_sizing();

    std::vector<std::string> lines;
    lines.resize(layout.y_offsets.back());
    for (auto &line : lines) {
        line.resize(layout.x_offsets.back(), ' ');
    }

    for (const auto &line : diagram.lines) {
        auto &p1 = line.first;
        auto &p2 = line.second;
        auto x = layout.x_offsets[p1.x];
        auto y = layout.y_offsets[p1.y];
        auto x2 = layout.x_offsets[p2.x];
        auto y2 = layout.y_offsets[p2.y];
        x += (int)floor(p1.align_x * (layout.x_spans[p1.x] - 1));
        y += (int)floor(p1.align_y * (layout.y_spans[p1.y] - 1));
        x2 += (int)floor(p2.align_x * (layout.x_spans[p2.x] - 1));
        y2 += (int)floor(p2.align_y * (layout.y_spans[p2.y] - 1));
        while (x != x2) {
            lines[y][x] = '-';
            x += x < x2 ? 1 : -1;
        }
        if (p1.x != p2.x && p1.y != p2.y) {
            lines[y][x] = '.';
        } else if (p1.x != p2.x) {
            lines[y][x] = '-';
        } else if (p1.y != p2.y) {
            lines[y][x] = '|';
        } else {
            lines[y][x] = '.';
        }
        while (y != y2) {
            y += y < y2 ? 1 : -1;
            lines[y][x] = '|';
        }
    }

    for (const auto &item : diagram.cells) {
        const auto &box = item.second;
        auto x = layout.x_offsets[box.center.x];
        auto y = layout.y_offsets[box.center.y];
        x += (int)floor(box.center.align_x * (layout.x_spans[box.center.x] - box.label.size()));
        y += (int)floor(box.center.align_y * (layout.y_spans[box.center.y] - 1));
        for (size_t k = 0; k < box.label.size(); k++) {
            lines[y][x + k] = box.label[k];
        }
    }

    std::string result;
    for (const auto &line : lines) {
        result.append(line);
        while (result.size() > 0 && result[0] == '\n') {
            result.erase(result.begin());
        }
        while (result.size() > 0 && result.back() == ' ') {
            result.pop_back();
        }
        result.push_back('\n');
    }

    return result;
}
