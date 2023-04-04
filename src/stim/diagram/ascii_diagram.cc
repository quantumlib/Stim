#include "stim/diagram/ascii_diagram.h"

#include <cmath>

#include "stim/mem/span_ref.h"

using namespace stim;
using namespace stim_draw_internal;

/// Describes sizes and offsets within a diagram with variable-sized columns and rows.
struct AsciiLayout {
    size_t num_x;
    size_t num_y;
    std::vector<size_t> x_spans;
    std::vector<size_t> y_spans;
    std::vector<size_t> x_offsets;
    std::vector<size_t> y_offsets;
};

AsciiDiagramPos::AsciiDiagramPos(size_t x, size_t y, float align_x, float align_y)
    : x(x), y(y), align_x(align_x), align_y(align_y) {
}

bool AsciiDiagramPos::operator==(const AsciiDiagramPos &other) const {
    return x == other.x && y == other.y;
}

bool AsciiDiagramPos::operator<(const AsciiDiagramPos &other) const {
    if (x != other.x) {
        return x < other.x;
    }
    return y < other.y;
}

AsciiDiagramEntry::AsciiDiagramEntry(AsciiDiagramPos center, std::string label) : center(center), label(label) {
}

void AsciiDiagram::add_entry(AsciiDiagramEntry entry) {
    cells.insert({entry.center, entry});
}

void AsciiDiagram::for_each_pos(const std::function<void(AsciiDiagramPos pos)> &callback) const {
    for (const auto &item : cells) {
        callback(item.first);
    }
    for (const auto &item : lines) {
        callback(item.first);
        callback(item.second);
    }
}

AsciiLayout compute_sizing(const AsciiDiagram &diagram) {
    AsciiLayout layout{0, 0, {}, {}, {}, {}};
    diagram.for_each_pos([&](AsciiDiagramPos pos) {
        layout.num_x = std::max(layout.num_x, pos.x + 1);
        layout.num_y = std::max(layout.num_y, pos.y + 1);
    });
    layout.x_spans.resize(layout.num_x, 1);
    layout.y_spans.resize(layout.num_y, 1);

    for (const auto &item : diagram.cells) {
        const auto &box = item.second;
        auto &dx = layout.x_spans[box.center.x];
        auto &dy = layout.y_spans[box.center.y];
        dx = std::max(dx, box.label.size());
        dy = std::max(dy, (size_t)1);
    }

    layout.x_offsets.push_back(0);
    layout.y_offsets.push_back(0);
    for (const auto &e : layout.x_spans) {
        layout.x_offsets.push_back(layout.x_offsets.back() + e);
    }
    for (const auto &e : layout.y_spans) {
        layout.y_offsets.push_back(layout.y_offsets.back() + e);
    }

    return layout;
}

AsciiDiagramPos AsciiDiagramPos::transposed() const {
    return {y, x, align_y, align_x};
}
AsciiDiagramEntry AsciiDiagramEntry::transposed() const {
    return {center.transposed(), label};
}

AsciiDiagram AsciiDiagram::transposed() const {
    AsciiDiagram result;
    for (const auto &e : cells) {
        result.cells.insert({e.first.transposed(), e.second.transposed()});
    }
    result.lines.reserve(lines.size());
    for (const auto &e : lines) {
        result.lines.push_back({e.first.transposed(), e.second.transposed()});
    }
    return result;
}

void strip_padding_from_lines_and_write_to(SpanRef<std::string> out_lines, std::ostream &out) {
    // Strip spacing at end of lines and end of diagram.
    for (auto &line : out_lines) {
        while (!line.empty() && line.back() == ' ') {
            line.pop_back();
        }
    }

    // Strip empty lines at start and end.
    while (!out_lines.empty() && out_lines.back().empty()) {
        out_lines.ptr_end--;
    }
    while (!out_lines.empty() && out_lines.front().empty()) {
        out_lines.ptr_start++;
    }

    // Find indentation.
    size_t indentation = SIZE_MAX;
    for (const auto &line : out_lines) {
        size_t k = 0;
        while (k < line.length() && line[k] == ' ') {
            k++;
        }
        indentation = std::min(indentation, k);
    }

    // Output while stripping empty lines at start of diagram.
    for (size_t k = 0; k < out_lines.size(); k++) {
        if (k) {
            out.put('\n');
        }
        out.write(out_lines[k].data() + indentation, out_lines[k].size() - indentation);
    }
}

void AsciiDiagram::render(std::ostream &out) const {
    AsciiLayout layout = compute_sizing(*this);

    std::vector<std::string> out_lines;
    out_lines.resize(layout.y_offsets.back());
    for (auto &line : out_lines) {
        line.resize(layout.x_offsets.back(), ' ');
    }

    auto p_align = [&](size_t c0, size_t cn, float align) {
        if (align == 0.5f) {
            cn--;
        }
        return c0 + (int)floor(align * cn);
    };
    auto x_align = [&](AsciiDiagramPos pos) {
        return p_align(layout.x_offsets[pos.x], layout.x_spans[pos.x], pos.align_x);
    };
    auto y_align = [&](AsciiDiagramPos pos) {
        return p_align(layout.y_offsets[pos.y], layout.y_spans[pos.y], pos.align_y);
    };

    for (const auto &line : lines) {
        auto &p1 = line.first;
        auto &p2 = line.second;

        auto x1 = x_align(p1);
        auto x2 = x_align(p2);
        auto y1 = y_align(p1);
        auto y2 = y_align(p2);
        if (x1 > x2) {
            std::swap(x1, x2);
        }
        if (y1 > y2) {
            std::swap(y1, y2);
        }
        bool bx = x1 != x2;
        while (x1 < x2) {
            out_lines[y1][x1] = '-';
            x1++;
        }

        char next_char = bx ? '.' : '|';
        while (y1 < y2) {
            out_lines[y1][x1] = next_char;
            next_char = '|';
            y1++;
        }
    }

    for (const auto &item : cells) {
        const auto &box = item.second;
        auto x = layout.x_offsets[box.center.x];
        auto y = layout.y_offsets[box.center.y];
        x += (int)floor(box.center.align_x * (layout.x_spans[box.center.x] - box.label.size()));
        y += (int)floor(box.center.align_y * (layout.y_spans[box.center.y] - 1));
        for (size_t k = 0; k < box.label.size(); k++) {
            out_lines[y][x + k] = box.label[k];
        }
    }

    strip_padding_from_lines_and_write_to(out_lines, out);
}

std::string AsciiDiagram::str() const {
    std::stringstream ss;
    render(ss);
    return ss.str();
}

std::ostream &stim_draw_internal::operator<<(std::ostream &out, const AsciiDiagram &drawer) {
    drawer.render(out);
    return out;
}
