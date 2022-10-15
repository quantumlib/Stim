#include "stim/diagram/timeline_img/timeline_layout.h"
#include "stim/diagram/timeline_img/timeline_svg.h"

using namespace stim;
using namespace stim_draw_internal;

std::string stim::circuit_diagram_timeline_svg(const Circuit &circuit) {
    std::stringstream out;
    write_circuit_diagram_timeline_svg(circuit, out);
    return out.str();
}

void stim::write_circuit_diagram_timeline_svg(const Circuit &circuit, std::ostream &out) {
    CellDiagram diagram = CellDiagram::from_circuit(circuit);
    diagram.compactify();
    CellDiagramSizing layout = diagram.compute_sizing();
    const size_t h_scale = 16;
    const size_t v_scale = 24;
    const size_t font_height = 24;
    const size_t font_width = 16;

    out << "<svg width=\"" << layout.x_offsets.back() * h_scale
        << "\" height=\"" << layout.y_offsets.back() * v_scale + 10
        << "\" version=\"1.1"
        << "\" xmlns=\"http://www.w3.org/2000/svg"
        << "\">\n";

    for (const auto &e : diagram.lines) {
        auto &p1 = e.first;
        auto &p2 = e.second;
        double x1 = layout.x_offsets[p1.x] * h_scale;
        double x2 = layout.x_offsets[p2.x] * h_scale;
        auto w1 = layout.x_spans[p1.x] * h_scale;
        auto h1 = layout.y_spans[p1.y] * v_scale;
        double y1 = layout.y_offsets[p1.y] * v_scale;
        double y2 = layout.y_offsets[p2.y] * v_scale;
        auto w2 = layout.x_spans[p2.x] * h_scale;
        auto h2 = layout.y_spans[p2.y] * v_scale;
        x1 += w1 * p1.align_x;
        y1 += h1 * p1.align_y;
        x2 += w2 * p2.align_x;
        y2 += h2 * p2.align_y;
        out << " <line x1=\"" << x1 << "\" x2=\"" << x2 << "\" y1=\"" << y1 << "\" y2=\"" << y2 << "\" stroke=\"black\" stroke-width=\"1\"/>\n";
    }

    for (const auto &item : diagram.cells) {
        const auto &box = item.second;
        auto cx = layout.x_offsets[box.center.x] * h_scale;
        auto cy = layout.y_offsets[box.center.y] * v_scale;
        auto w = layout.x_spans[box.center.x] * h_scale;
        auto h = layout.y_spans[box.center.y] * v_scale;
        cx += w * box.center.align_x;
        cy += h * box.center.align_y;
        w = utf8_char_count(box.label) * font_width;
        auto w2 = w + (font_height - font_width);
        auto h2 = h + 4;
        if (box.label == "@") {
            out << " <circle cx=\"" << cx
                << "\" cy=\"" << cy
                << "\" r=\"" << 8
                << "\" stroke=\"none"
                << "\" fill=\"black\"/>\n";
            continue;
        }
        if (box.label == "X") {
            out << " <circle cx=\"" << cx
                << "\" cy=\"" << cy
                << "\" r=\"" << 12
                << "\" stroke=\"black"
                << "\" fill=\"white\"/>\n";
            out << " <line x1=\"" << (cx - 12)
                << "\" x2=\"" << (cx + 12)
                << "\" y1=\"" << cy
                << "\" y2=\"" << cy
                << "\" stroke=\"black\"/>\n";
            out << " <line x1=\"" << cx
                << "\" x2=\"" << cx
                << "\" y1=\"" << (cy + 12)
                << "\" y2=\"" << (cy - 12)
                << "\" stroke=\"black\"/>\n";
            continue;
        }
        out << " <rect x=\"" << (cx - w2 * box.center.align_x)
            << "\" y=\"" << (cy - h2 * box.center.align_y)
            << "\" width=\"" << w2
            << "\" height=\"" << h2
            << "\" stroke=\"" << box.stroke
            << "\" fill=\"white\" stroke-width=\"1\"/>\n";
        out << " <text dominant-baseline=\"central\" textLength=\"" << w
            << "\" text-anchor=\"" << (box.center.align_x == 0 ? "start" : box.center.align_x == 1 ? "end" : "middle")
            << "\" font-family=\"monospace\" font-size=\""
            << font_height << "px\" x=\"" << cx
            << "\" y=\"" << cy
            << "\">" << box.label << "</text>\n";
    }

    out << "</svg>\n";
}
