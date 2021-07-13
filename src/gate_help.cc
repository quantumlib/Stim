// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "gate_help.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <set>

#include "arg_parse.h"
#include "stabilizers/tableau.h"

using namespace stim_internal;

struct Acc {
    std::string settled;
    std::stringstream working;
    int indent{};

    void flush() {
        auto s = working.str();
        for (char c : s) {
            settled.push_back(c);
            if (c == '\n') {
                for (int k = 0; k < indent; k++) {
                    settled.push_back(' ');
                }
            }
        }
        working.str("");
    }

    void change_indent(int t) {
        flush();
        if (indent + t < 0) {
            throw std::out_of_range("negative indent");
        }
        indent += t;
        working << '\n';
    }

    template <typename T>
    Acc &operator<<(const T &other) {
        working << other;
        return *this;
    }
};

void print_fixed_width_float(Acc &out, float f, char u) {
    if (f == 0) {
        out << "  ";
    } else if (fabs(f - 1) < 0.0001) {
        out << "+" << u;
    } else if (fabs(f + 1) < 0.0001) {
        out << "-" << u;
    } else {
        if (f > 0) {
            out << "+";
        }
        out << f;
    }
}

void print_example(Acc &out, const char *name, const Gate &gate) {
    out << "\n- Example:\n";
    out.change_indent(+4);
    out << "```\n";
    for (size_t k = 0; k < 3; k++) {
        out << name;
        if ((gate.flags & GATE_IS_NOISE) || (k == 2 && (gate.flags & GATE_PRODUCES_NOISY_RESULTS))) {
            out << "(" << 0.001 << ")";
        }
        if (k != 1) {
            out << " " << 5;
            if (gate.flags & GATE_TARGETS_PAIRS) {
                out << " " << 6;
            }
        }
        if (k != 0) {
            out << " ";
            if (gate.flags & GATE_PRODUCES_NOISY_RESULTS) {
                out << "!";
            }
            out << 42;
            if (gate.flags & GATE_TARGETS_PAIRS) {
                out << " " << 43;
            }
        }
        out << "\n";
    }
    if (gate.flags & GATE_CAN_TARGET_MEASUREMENT_RECORD) {
        if (gate.name[0] == 'C' || gate.name[0] == 'Z') {
            out << gate.name << " rec[-1] 111\n";
        }
        if (gate.name[gate.name_len - 1] == 'Z') {
            out << gate.name << " 111 rec[-1]\n";
        }
    }
    out << "```\n";
    out.change_indent(-4);
}

void print_stabilizer_generators(Acc &out, const Gate &gate) {
    if (gate.flags & GATE_IS_UNITARY) {
        out << "- Stabilizer Generators:\n";
        out.change_indent(+4);
        out << "```\n";
        auto tableau = gate.tableau();
        if (gate.flags & GATE_TARGETS_PAIRS) {
            out << "X_ -> " << tableau.xs[0] << "\n";
            out << "Z_ -> " << tableau.zs[0] << "\n";
            out << "_X -> " << tableau.xs[1] << "\n";
            out << "_Z -> " << tableau.zs[1] << "\n";
        } else {
            out << "X -> " << tableau.xs[0] << "\n";
            out << "Z -> " << tableau.zs[0] << "\n";
        }
        out << "```\n";
        out.change_indent(-4);
    } else {
        auto data = gate.extra_data_func();
        if (data.tableau_data.size()) {
            out << "- Stabilizer Generators:\n";
            out.change_indent(+4);
            out << "```\n";
            for (const auto &e : data.tableau_data) {
                out << e << "\n";
            }
            out << "```\n";
            out.change_indent(-4);
        }
    }
}

void print_bloch_vector(Acc &out, const Gate &gate) {
    if (!(gate.flags & GATE_IS_UNITARY) || (gate.flags & GATE_TARGETS_PAIRS)) {
        return;
    }

    out << "- Bloch Rotation:\n";
    out.change_indent(+4);
    out << "```\n";
    auto matrix = gate.unitary();
    auto a = matrix[0][0];
    auto b = matrix[0][1];
    auto c = matrix[1][0];
    auto d = matrix[1][1];
    auto i = std::complex<float>{0, 1};
    auto x = b + c;
    auto y = b * i + c * -i;
    auto z = a - d;
    auto s = a + d;
    s *= -i;
    std::complex<double> p = 1;
    if (s.imag() != 0) {
        p = s;
    }
    if (x.imag() != 0) {
        p = x;
    }
    if (y.imag() != 0) {
        p = y;
    }
    if (z.imag() != 0) {
        p = z;
    }
    p /= sqrt(p.imag() * p.imag() + p.real() * p.real());
    p *= 2;
    x /= p;
    y /= p;
    z /= p;
    s /= p;
    assert(x.imag() == 0);
    assert(y.imag() == 0);
    assert(z.imag() == 0);
    assert(s.imag() == 0);
    auto rx = x.real();
    auto ry = y.real();
    auto rz = z.real();
    auto rs = s.real();
    auto angle = (int)round(acosf(rs) * 360 / 3.14159265359);
    if (angle > 180) {
        angle -= 360;
    }
    out << "Axis: ";
    if (rx != 0) {
        out << "+-"[rx < 0] << 'X';
    }
    if (ry != 0) {
        out << "+-"[rx < 0] << 'Y';
    }
    if (rz != 0) {
        out << "+-"[rx < 0] << 'Z';
    }
    out << "\n";
    out << "Angle: " << angle << " degrees\n";
    out << "```\n";
    out.change_indent(-4);
}

void print_unitary_matrix(Acc &out, const Gate &gate) {
    if (!(gate.flags & GATE_IS_UNITARY)) {
        return;
    }
    auto matrix = gate.unitary();
    out << "- Unitary Matrix:\n";
    out.change_indent(+4);
    bool all_halves = true;
    bool all_sqrt_halves = true;
    double s = sqrt(0.5);
    for (const auto &row : matrix) {
        for (const auto &cell : row) {
            all_halves &= cell.real() == 0.5 || cell.real() == 0 || cell.real() == -0.5;
            all_halves &= cell.imag() == 0.5 || cell.imag() == 0 || cell.imag() == -0.5;
            all_sqrt_halves &= fabs(fabs(cell.real()) - s) < 0.001 || cell.real() == 0;
            all_sqrt_halves &= fabs(fabs(cell.imag()) - s) < 0.001 || cell.imag() == 0;
        }
    }
    out << "```\n";
    double factor = all_halves ? 2 : all_sqrt_halves ? 1 / s : 1;
    bool first_row = true;
    for (const auto &row : matrix) {
        if (first_row) {
            first_row = false;
        } else {
            out << "\n";
        }
        out << "[";
        bool first = true;
        for (const auto &cell : row) {
            if (first) {
                first = false;
            } else {
                out << ", ";
            }
            print_fixed_width_float(out, cell.real() * factor, '1');
            print_fixed_width_float(out, cell.imag() * factor, 'i');
        }
        out << "]";
    }
    if (all_halves) {
        out << " / 2";
    }
    if (all_sqrt_halves) {
        out << " / sqrt(2)";
    }
    out << "\n```\n";
    out.change_indent(-4);
}

std::string generate_per_gate_help_markdown(const Gate &alt_gate, int indent, bool anchor) {
    Acc out;
    out.indent = indent;
    const Gate &gate = GATE_DATA.at(alt_gate.name);
    if (anchor) {
        out << "<a name=\"" << alt_gate.name << "\"></a>";
    }
    out << "**`" << alt_gate.name << "`**\n";
    for (const auto &other : GATE_DATA.gates()) {
        if (other.id == alt_gate.id && other.name != alt_gate.name) {
            out << "\nAlternate name: ";
            if (anchor) {
                out << "<a name=\"" << other.name << "\"></a>";
            }
            out << "`" << other.name << "`\n";
        }
    }
    auto data = gate.extra_data_func();
    out << data.description;
    if (gate.flags & GATE_PRODUCES_NOISY_RESULTS) {
        out << "If this gate is parameterized by a probability argument, the "
               "recorded result will be flipped with that probability. "
               "If not, the recorded result is noiseless. "
               "Note that the noise only affects the recorded result, not the "
               "target qubit's state.\n\n";
        out << "Prefixing a target with ! inverts its recorded measurement result.\n";
    }

    if (std::string(data.description).find("xample:\n") == std::string::npos) {
        print_example(out, alt_gate.name, gate);
    }
    print_stabilizer_generators(out, gate);
    print_bloch_vector(out, gate);
    print_unitary_matrix(out, gate);
    out.flush();
    return out.settled;
}

std::map<std::string, std::string> stim_internal::generate_gate_help_markdown() {
    std::map<std::string, std::string> result;
    for (const auto &g : GATE_DATA.gates()) {
        result[g.name] = generate_per_gate_help_markdown(g, 0, false);
    }

    std::map<std::string, std::vector<std::string>> categories;
    std::set<std::string> gate_names;
    for (const auto &g : GATE_DATA.gates()) {
        if (g.name == GATE_DATA.at(g.name).name) {
            categories[std::string(g.extra_data_func().category)].push_back(g.name);
        }
        gate_names.insert(g.name);
    }

    std::stringstream all;
    all << "Gates supported by Stim\n";
    all << "=======================\n";
    for (const auto &name : gate_names) {
        all << name << "\n";
    }
    result[std::string("GATES")] = all.str();

    all.str("");
    all << R"MARKDOWN(# Gates supported by Stim

)MARKDOWN";
    for (const auto &name : gate_names) {
        all << "- [" << name << "](#" << name << ")\n";
    }
    all << "\n";
    for (auto &category : categories) {
        all << "## " << category.first.substr(2) << "\n\n";
        std::sort(category.second.begin(), category.second.end());
        for (const auto &name : category.second) {
            all << "- " << generate_per_gate_help_markdown(GATE_DATA.at(name), 4, true) << "\n";
        }
    }
    result[std::string("GATES_MARKDOWN")] = all.str();

    result[""] = R"HELP(BASIC USAGE
===========
Gate reference:
    stim --help gates
    stim --help [gate_name]

Interactive measurement sampling mode:
    stim --repl

Bulk measurement sampling mode:
    stim --sample[=#shots] \
         [--frame0] \
         [--out_format=01|b8|ptb64|r8|hits|dets] \
         [--in=file] \
         [--out=file]

Detection event sampling mode:
    stim --detect[=#shots] \
         [--out_format=01|b8|ptb64|r8|hits|dets] \
         [--in=file] \
         [--out=file]

Error analysis mode:
    stim --analyze_errors \
         [--decompose_errors] \
         [--in=file] \
         [--out=file]

Circuit generation mode:
    stim --gen=repetition_code|surface_code|color_code \
         --rounds=# \
         --distance=# \
         --task=... \
         [--out=file] \
         [--after_clifford_depolarization=0] \
         [--after_reset_flip_probability=0] \
         [--before_measure_flip_probability=0] \
         [--before_round_data_depolarization=0]

EXAMPLE CIRCUIT (GHZ)
=====================
H 0
CNOT 0 1
CNOT 0 2
M 0 1 2
)HELP";

    return result;
}

int stim_internal::main_help(int argc, const char **argv) {
    const char *help = require_find_argument("--help", argc, argv);
    auto m = generate_gate_help_markdown();
    auto key = std::string(help);
    for (auto &c : key) {
        c = toupper(c);
    }
    auto p = m.find(key);
    if (p == m.end()) {
        std::cerr << "Unrecognized help topic '" << help << "'.\n";
        return EXIT_FAILURE;
    }

    std::cout << p->second;
    return EXIT_SUCCESS;
}
