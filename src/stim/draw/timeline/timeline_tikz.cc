#include "stim/draw/timeline/timeline_layout.h"
#include "stim/draw/timeline/timeline_tikz.h"

using namespace stim;
using namespace stim_draw_internal;

std::string stim::circuit_diagram_timeline_tikz(const Circuit &circuit) {
    std::stringstream out;
    write_circuit_diagram_timeline_tikz(circuit, out);
    return out.str();
}

void escape_for_latex(const std::string &label, std::ostream &out) {
    out << "$";
    for (size_t k = 0; k < label.size(); k++) {
        uint8_t c = label[k];
        if (c == '\\') {
            out << "\\textbackslash";
        } else if (c == '$') {
            out << "\\$";
        } else if (c == '%') {
            out << "\\%";
        } else if (c > 127) {
            if ((uint8_t)label[k] == 0xE2 && (uint8_t)label[k + 1] == 0x88 && (uint8_t)label[k + 2] == 0x9A) {
                out << "\\sqrt";
            } else if ((uint8_t)label[k] == 0xE2 && (uint8_t)label[k + 1] == 0x80 && (uint8_t)label[k + 2] == 0xA0) {
                out << "^\\dagger";
            } else {
                out << '?';
            }
            k++;
            while (k < label.size() && ((uint8_t)label[k] & 0xB0) == 0xB0) {
                k++;
            }
        } else {
            out << c;
        }
    }
    out << '$';
}

void stim::write_circuit_diagram_timeline_tikz(const Circuit &circuit, std::ostream &out) {
    CellDiagram diagram = CellDiagram::from_circuit(circuit);
    diagram.compactify();
    const float h_scale = 0.6;
    const float v_scale = 0.6;

    out << "% Assumes '\\usepackage{tikz}' is present at the top of the document.\n";
    out << "\\resizebox{\\textwidth}{!}{  % Optional; ensures the diagram fits exactly on the page.\n\n";
    out << "\\begin{tikzpicture}[y=-1cm]\n";

    for (const auto &e : diagram.lines) {
        auto &p1 = e.first;
        auto &p2 = e.second;
        double x1 = p1.x * h_scale;
        double x2 = p2.x * h_scale;
        double y1 = p1.y * v_scale;
        double y2 = p2.y * v_scale;
        x1 += h_scale * p1.align_x;
        y1 += v_scale * p1.align_y;
        x2 += h_scale * p2.align_x;
        y2 += v_scale * p2.align_y;
        out << " \\draw[black, thick] (" << x1 << "," << y1 << ") -- (" << x2 << "," << y2 << ");\n";
    }

    for (const auto &item : diagram.cells) {
        const auto &box = item.second;
        auto x1 = box.center.x * h_scale;
        auto y1 = box.center.y * v_scale;
        auto x2 = x1 + h_scale;
        auto y2 = y1 + v_scale;
        auto cx = x1 + h_scale * box.center.align_x;
        auto cy = y1 + v_scale * box.center.align_y;
        x1 -= h_scale * 0.3;
        y1 -= v_scale * 0.3;
        x2 += h_scale * 0.3;
        y2 += v_scale * 0.3;

        if (box.label == "@") {
            auto r = 0.15;
            out << " \\fill[fill=black](" << cx << "," << cy << ") circle (" << r << ");\n";
            continue;
        }
        if (box.label == "SWAP") {
            auto r = 0.2;
            out << " \\draw[black, thick] (" << cx - r << "," << cy - r << ") -- (" << cx + r << "," << cy + r << ");\n";
            out << " \\draw[black, thick] (" << cx - r << "," << cy + r << ") -- (" << cx + r << "," << cy - r << ");\n";
            continue;
        }
        if (box.label == "X") {
            auto r = 0.2;
            out << " \\filldraw[color=black, fill=white, thick](" << cx << "," << cy << ") circle (" << r << ");\n";
            out << " \\draw[black, thick] (" << cx << "," << cy - r << ") -- (" << cx << "," << cy + r << ");\n";
            out << " \\draw[black, thick] (" << cx - r << "," << cy << ") -- (" << cx + r << "," << cy << ");\n";
            continue;
        }
        if (strcmp(box.stroke, "none") == 0) {
            out << " \\fill[fill=white] ";
        } else {
            out << " \\filldraw[black, fill=white, thick] ";
        }
        out << "(" << x1 << "," << y1 << ") rectangle (" << x2 << "," << y2 << ") "
            << "node[midway,text centered] {\\makecell{";
        escape_for_latex(box.label, out);
        out << "}};\n";
    }

    out << "\\end{tikzpicture}\n\n";
    out << "}  % end optional resize box.\n";
}
