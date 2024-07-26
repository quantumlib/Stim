#include "stim/diagram/graph/match_graph_svg_drawer.h"

#include "stim/diagram/graph/match_graph_3d_drawer.h"

using namespace stim;
using namespace stim_draw_internal;

Coord<2> project(Coord<3> coord) {
    return {coord.xyz[0] * 5.0f + coord.xyz[2] * 0.2f, coord.xyz[1] * 5.0f + coord.xyz[2] * 0.1f};
}

template <typename T>
inline void write_key_val(std::ostream &out, const char *key, const T &val) {
    out << ' ' << key << "=\"" << val << "\"";
}

void stim_draw_internal::dem_match_graph_to_svg_diagram_write_to(
    const stim::DetectorErrorModel &dem, std::ostream &svg_out) {
    auto diagram = dem_match_graph_to_basic_3d_diagram(dem);
    std::vector<Coord<2>> used_coords;
    for (const auto &e : diagram.line_data) {
        used_coords.push_back(project(e));
    }
    for (const auto &e : diagram.red_line_data) {
        used_coords.push_back(project(e));
    }
    for (const auto &e : diagram.blue_line_data) {
        used_coords.push_back(project(e));
    }
    for (const auto &e : diagram.purple_line_data) {
        used_coords.push_back(project(e));
    }
    for (const auto &e : diagram.elements) {
        used_coords.push_back(project(e.center));
    }
    auto minmax = Coord<2>::min_max(used_coords);
    minmax.first.xyz[0] -= 5;
    minmax.first.xyz[1] -= 5;
    minmax.second.xyz[0] += 5;
    minmax.second.xyz[1] += 5;
    auto off = minmax.first * -1.0f;

    svg_out << R"SVG(<svg viewBox="0 0 )SVG";
    svg_out << (minmax.second.xyz[0] - minmax.first.xyz[0]);
    svg_out << " ";
    svg_out << (minmax.second.xyz[1] - minmax.first.xyz[1]);
    svg_out << '"' << ' ';
    write_key_val(svg_out, "version", "1.1");
    write_key_val(svg_out, "xmlns", "http://www.w3.org/2000/svg");
    svg_out << ">\n";

    auto write_lines = [&](const std::vector<Coord<3>> &line_data, const char *color) {
        if (line_data.empty()) {
            return;
        }
        svg_out << "<path d=\"";
        for (size_t k = 0; k < line_data.size(); k++) {
            if (k) {
                svg_out << ' ';
            }
            svg_out << (k % 2 == 0 ? 'M' : 'L');
            auto c = project(line_data[k]);
            c += off;
            svg_out << c.xyz[0];
            svg_out << ',';
            svg_out << c.xyz[1];
        }
        svg_out << '"';
        write_key_val(svg_out, "stroke", color);
        write_key_val(svg_out, "fill", "none");
        write_key_val(svg_out, "stroke-width", "0.2");
        svg_out << "/>\n";
    };
    write_lines(diagram.line_data, "black");
    write_lines(diagram.red_line_data, "red");
    write_lines(diagram.blue_line_data, "blue");
    write_lines(diagram.purple_line_data, "purple");
    for (const auto &e : diagram.elements) {
        auto c = project(e.center);
        c += off;
        svg_out << "<circle";
        write_key_val(svg_out, "cx", c.xyz[0]);
        write_key_val(svg_out, "cy", c.xyz[1]);
        write_key_val(svg_out, "r", 0.5);
        write_key_val(svg_out, "stroke", "none");
        write_key_val(svg_out, "fill", "black");
        svg_out << "/>\n";
    }

    svg_out << "</svg>";
}
