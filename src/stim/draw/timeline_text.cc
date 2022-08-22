#include "stim/draw/diagram.h"
#include "stim/draw/timeline_text.h"

using namespace stim;
using namespace stim_draw_internal;

std::string stim::circuit_diagram_timeline_text(const Circuit &circuit) {
    Diagram diagram = to_diagram(circuit);
    DiagramLayout layout = diagram.to_layout();

    std::vector<std::string> lines;
    lines.resize(layout.y_offsets.back());
    for (auto &line : lines) {
        line.resize(layout.x_offsets.back(), ' ');
    }

    for (const auto &line : diagram.lines) {
        auto x = layout.x_offsets[line.p1.x];
        auto y = layout.y_offsets[line.p1.y];
        auto x2 = layout.x_offsets[line.p2.x];
        auto y2 = layout.y_offsets[line.p2.y];
        x += (int)floor(line.p1.align_x * (layout.x_widths[line.p1.x] - 1));
        y += (int)floor(line.p1.align_y * (layout.y_heights[line.p1.y] - 1));
        x2 += (int)floor(line.p2.align_x * (layout.x_widths[line.p2.x] - 1));
        y2 += (int)floor(line.p2.align_y * (layout.y_heights[line.p2.y] - 1));
        while (x != x2) {
            lines[y][x] = '-';
            x += x < x2 ? 1 : -1;
        }
        if (line.p1.x != line.p2.x && line.p1.y != line.p2.y) {
            lines[y][x] = '.';
        } else if (line.p1.x != line.p2.x) {
            lines[y][x] = '-';
        } else if (line.p1.y != line.p2.y) {
            lines[y][x] = '|';
        } else {
            lines[y][x] = '.';
        }
        while (y != y2) {
            y += y < y2 ? 1 : -1;
            lines[y][x] = '|';
        }
    }

    for (const auto &item : diagram.boxes) {
        const auto &box = item.second;
        auto x = layout.x_offsets[box.center.x];
        auto y = layout.y_offsets[box.center.y];
        x += (int)floor(box.center.align_x * (layout.x_widths[box.center.x] - box.label.size()));
        y += (int)floor(box.center.align_y * (layout.y_heights[box.center.y] - 1));
        for (size_t k = 0; k < box.label.size(); k++) {
            lines[y][x + k] = box.label[k];
        }
        for (size_t a = 0; a < box.annotations.size(); a++) {
            for (size_t k = 0; k < box.label.size(); k++) {
                lines[y + a][x + k] = box.label[k];
            }
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
