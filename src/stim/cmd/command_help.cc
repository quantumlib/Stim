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

#include "stim/cmd/command_help.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <stim/circuit/circuit.h>

#include "command_convert.h"
#include "command_detect.h"
#include "command_diagram.h"
#include "command_explain_errors.h"
#include "command_gen.h"
#include "command_m2d.h"
#include "command_repl.h"
#include "command_sample.h"
#include "command_sample_dem.h"
#include "stim/cmd/command_analyze_errors.h"
#include "stim/gates/gates.h"
#include "stim/io/stim_data_formats.h"
#include "stim/stabilizers/flow.h"
#include "stim/stabilizers/tableau.h"
#include "stim/util_bot/arg_parse.h"

using namespace stim;

std::string stim::clean_doc_string(const char *c, bool allow_too_long) {
    // Skip leading empty lines.
    while (*c == '\n') {
        c++;
    }

    // Determine indentation using first non-empty line.
    size_t indent = 0;
    while (*c == ' ') {
        indent++;
        c++;
    }

    std::string result;
    while (*c != '\0') {
        // Skip indentation.
        for (size_t j = 0; j < indent && *c == ' '; j++) {
            c++;
        }

        // Copy rest of line.
        size_t line_length = 0;
        while (*c != '\0') {
            result.push_back(*c);
            c++;
            if (result.back() == '\n') {
                break;
            }
            line_length++;
        }
        const char *start_of_line = result.c_str() + result.size() - line_length - 1;
        if (strstr(start_of_line, "\"\"\"") != nullptr) {
            std::stringstream ss;
            ss << "Docstring line contains \"\"\" (please use ''' instead):\n" << start_of_line << "\n";
            throw std::invalid_argument(ss.str());
        }
        if (!allow_too_long && line_length > 80) {
            if (memcmp(start_of_line, "@signature", strlen("@signature")) != 0 &&
                memcmp(start_of_line, "@overload", strlen("@overload")) != 0 &&
                strstr(start_of_line, "https://") == nullptr) {
                std::stringstream ss;
                ss << "Docstring line has length " << line_length << " > 80:\n"
                   << start_of_line << std::string(80, '^') << "\n";
                throw std::invalid_argument(ss.str());
            }
        }
    }

    return result;
}

std::vector<SubCommandHelp> make_sub_command_help() {
    SubCommandHelp help_help;
    help_help.subcommand_name = "help";
    help_help.description = "Prints helpful information about using stim.";
    auto result = std::vector<SubCommandHelp>{
        command_analyze_errors_help(),
        command_convert_help(),
        command_detect_help(),
        command_diagram_help(),
        command_explain_errors_help(),
        command_gen_help(),
        command_m2d_help(),
        command_repl_help(),
        command_sample_help(),
        command_sample_dem_help(),
        help_help,
    };
    std::sort(result.begin(), result.end(), [](const SubCommandHelp &a, const SubCommandHelp &b) {
        return a.subcommand_name < b.subcommand_name;
    });
    return result;
}

std::string to_upper_case(std::string_view val) {
    std::string result;
    result.reserve(val.size());
    for (char c : val) {
        result.push_back(toupper(c));
    }
    return result;
}

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

void print_example(Acc &out, std::string_view name, const Gate &gate) {
    out << "\nExample:\n";
    out.change_indent(+4);
    for (size_t k = 0; k < 3; k++) {
        out << name;
        if (gate.flags & GATE_IS_NOISY) {
            if (k == 2 || !(gate.flags & GATE_PRODUCES_RESULTS)) {
                out << "(" << 0.001 << ")";
            }
        }
        if (k != 1) {
            out << " " << 5;
            if (gate.flags & GATE_TARGETS_PAIRS) {
                out << " " << 6;
            }
        }
        if (k != 0) {
            out << " ";
            if (gate.flags & GATE_PRODUCES_RESULTS) {
                out << "!";
            }
            out << 42;
            if (gate.flags & GATE_TARGETS_PAIRS) {
                out << " " << 43;
            }
        }
        out << "\n";
    }
    if (gate.flags & GATE_CAN_TARGET_BITS) {
        if (gate.name[0] == 'C' || gate.name[0] == 'Z') {
            out << gate.name << " rec[-1] 111\n";
        }
        if (gate.name.back() == 'Z') {
            out << gate.name << " 111 rec[-1]\n";
        }
    }
    out.change_indent(-4);
}

std::vector<GateTarget> stim::gate_decomposition_help_targets_for_gate_type(GateType g) {
    if (g == GateType::MPP) {
        return {
            GateTarget::x(0),
            GateTarget::combiner(),
            GateTarget::y(1),
            GateTarget::combiner(),
            GateTarget::z(2),
            GateTarget::x(3),
            GateTarget::combiner(),
            GateTarget::x(4),
        };
    } else if (g == GateType::SPP || g == GateType::SPP_DAG) {
        return {
            GateTarget::x(0),
            GateTarget::combiner(),
            GateTarget::y(1),
            GateTarget::combiner(),
            GateTarget::z(2),
        };
    } else if (g == GateType::DETECTOR || g == GateType::OBSERVABLE_INCLUDE) {
        return {GateTarget::rec(-1)};
    } else if (g == GateType::TICK || g == GateType::SHIFT_COORDS) {
        return {};
    } else if (g == GateType::E || g == GateType::ELSE_CORRELATED_ERROR) {
        return {GateTarget::x(0)};
    } else if (GATE_DATA[g].flags & GATE_TARGETS_PAIRS) {
        return {GateTarget::qubit(0), GateTarget::qubit(1)};
    } else {
        return {GateTarget::qubit(0)};
    }
}

void print_decomposition(Acc &out, const Gate &gate) {
    const char *decomposition = gate.h_s_cx_m_r_decomposition;
    if (decomposition != nullptr) {
        std::stringstream undecomposed;
        auto decomp_targets = gate_decomposition_help_targets_for_gate_type(gate.id);
        undecomposed << CircuitInstruction{gate.id, {}, decomp_targets, ""};

        out << "Decomposition (into H, S, CX, M, R):\n";
        out.change_indent(+4);
        out << "# The following circuit is equivalent (up to global phase) to `";
        out << undecomposed.str() << "`";
        out << decomposition;
        if (Circuit(decomposition) == Circuit(undecomposed.str())) {
            out << "\n# (The decomposition is trivial because this gate is in the target gate set.)\n";
        }
        out.change_indent(-4);
    }
}

void print_stabilizer_generators(Acc &out, const Gate &gate) {
    auto flows = gate.flows<MAX_BITWORD_WIDTH>();
    if (flows.empty()) {
        return;
    }
    auto decomp_targets = gate_decomposition_help_targets_for_gate_type(gate.id);
    if (decomp_targets.size() > 2) {
        out << "Stabilizer Generators (for `";
        out << CircuitInstruction{gate.id, {}, decomp_targets, ""};
        out << "`):\n";
    } else {
        out << "Stabilizer Generators:\n";
    }
    out.change_indent(+4);
    for (const auto &flow : gate.flows<MAX_BITWORD_WIDTH>()) {
        auto s = flow.str();
        std::string no_plus;
        for (char c : s) {
            if (c != '+') {
                no_plus.push_back(c);
            }
        }
        out << no_plus << "\n";
    }
    out.change_indent(-4);
}

void print_bloch_vector(Acc &out, const Gate &gate) {
    if (!(gate.flags & GATE_IS_UNITARY) || !(gate.flags & GATE_IS_SINGLE_QUBIT_GATE)) {
        return;
    }

    out << "Bloch Rotation (axis angle):\n";
    out.change_indent(+4);
    auto axis_angle = gate.to_axis_angle();
    auto rx = axis_angle[0];
    auto ry = axis_angle[1];
    auto rz = axis_angle[2];
    auto angle = (int)round(axis_angle[3] * 180 / 3.14159265359);
    if (angle > 180) {
        angle -= 360;
    }
    out << "Axis: ";
    if (rx != 0) {
        out << "+-"[rx < 0] << 'X';
    }
    if (ry != 0) {
        out << "+-"[ry < 0] << 'Y';
    }
    if (rz != 0) {
        out << "+-"[rz < 0] << 'Z';
    }
    out << "\n";
    out << "Angle: " << angle << "°\n";

    out.change_indent(-4);
    out << "Bloch Rotation (Euler angles):\n";
    out.change_indent(+4);
    auto euler_angles = gate.to_euler_angles();
    auto theta_deg = (int)round(euler_angles[0] * 180 / 3.14159265359) % 360;
    auto phi_deg = (int)round(euler_angles[1] * 180 / 3.14159265359) % 360;
    auto lambda_deg = (int)round(euler_angles[2] * 180 / 3.14159265359) % 360;
    out << "  theta = " << theta_deg << "°\n";
    out << "    phi = " << phi_deg << "°\n";
    out << " lambda = " << lambda_deg << "°\n";
    out << "unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)\n";
    out << "unitary = RotZ(" << phi_deg << "°) * RotY(" << theta_deg << "°) * RotZ(" << lambda_deg << "°)\n";
    out << "unitary = ";
    std::array<const char *, 4> y_rots{"I", "SQRT_Y", "Y", "SQRT_Y_DAG"};
    std::array<const char *, 4> z_rots{"I", "S", "Z", "S_DAG"};
    out << z_rots[(phi_deg / 90) & 3];
    out << " * ";
    out << y_rots[(theta_deg / 90) & 3];
    out << " * ";
    out << z_rots[(lambda_deg / 90) & 3];

    out.change_indent(-4);
    out << "\n";
}

void print_unitary_matrix(Acc &out, const Gate &gate) {
    if (!gate.has_known_unitary_matrix()) {
        return;
    }
    auto matrix = gate.unitary();
    out << "Unitary Matrix";
    if (gate.flags & GATE_TARGETS_PAIRS) {
        out << " (little endian)";
    }
    out << ":\n";
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
    out << "\n";
    out.change_indent(-4);
}

std::string generate_per_gate_help_markdown(const Gate &alt_gate, int indent, bool anchor) {
    Acc out;
    out.indent = indent;
    const Gate &gate = GATE_DATA.at(alt_gate.name);
    if (anchor) {
        out << "<a name=\"" << alt_gate.name << "\"></a>\n";
    }
    if (gate.flags & GATE_IS_UNITARY) {
        out << "### The '" << alt_gate.name << "' Gate\n";
    } else {
        out << "### The '" << alt_gate.name << "' Instruction\n";
    }
    for (const auto &entry : GATE_DATA.hashed_name_to_gate_type_table) {
        if (entry.expected_name.size() > 0 && entry.id == alt_gate.id && entry.expected_name != alt_gate.name) {
            out << "\nAlternate name: ";
            if (anchor) {
                out << "<a name=\"" << entry.expected_name << "\"></a>";
            }
            out << "`" << entry.expected_name << "`\n";
        }
    }
    out << gate.help;
    if (std::string(gate.help).find("xample:\n") == std::string::npos &&
        std::string(gate.help).find("xamples:\n") == std::string::npos) {
        print_example(out, alt_gate.name, gate);
    }
    print_stabilizer_generators(out, gate);
    print_bloch_vector(out, gate);
    print_unitary_matrix(out, gate);
    print_decomposition(out, gate);
    out.flush();
    return out.settled;
}

std::string generate_subcommand_markdown(const SubCommandHelp &data, int indent, bool anchor) {
    Acc out;
    out.indent = indent;
    if (anchor) {
        out << "<a name=\"" << data.subcommand_name << "\"></a>\n";
    }
    out << "### stim " << data.subcommand_name << "\n\n";
    out << "```\n";
    out << data.str_help();
    out << "```\n";

    out.flush();
    return out.settled;
}

std::string generate_per_format_markdown(const FileFormatData &format_data, int indent, bool anchor) {
    Acc out;
    out.indent = indent;
    if (anchor) {
        out << "<a name=\"" << format_data.name << "\"></a>";
    }
    out << "The `" << format_data.name << "` Format\n";
    out << format_data.help;
    out << "\n";

    out << "*Example " << format_data.name << " parsing code (python)*:\n";
    out << "```python";
    out << format_data.help_python_parse;
    out << "```\n";

    out << "*Example " << format_data.name << " saving code (python):*\n";
    out << "```python";
    out << format_data.help_python_save;
    out << "```\n";

    out.flush();
    return out.settled;
}

std::map<std::string, std::string> generate_format_help_markdown() {
    std::map<std::string, std::string> result;

    std::stringstream all;
    all << "Result data formats supported by Stim\n";

    all << "\n# Index\n";
    for (const auto &kv : format_name_to_enum_map()) {
        all << kv.first << "\n";
    }
    result[std::string("FORMATS")] = all.str();

    for (const auto &kv : format_name_to_enum_map()) {
        result[to_upper_case(kv.first)] = generate_per_format_markdown(kv.second, 0, false);
    }

    all.str("");
    all << R"MARKDOWN(# Introduction

A *result format* is a way of representing bits from shots sampled from a circuit.
It is some way of converting between a list-of-list-of-bits (a list-of-shots) and
a flat string of bytes or characters.

Generally, the result data formats supported by Stim are extremely minimalist.
They do not contain metadata about which circuit was run,
how many shots were taken,
how many bits are in each shot,
or even self-identifying information like a header with magic bytes.
They produce *raw* data.
Even details about which bits are measurements, which are detection events,
and which are observable frame changes must be determined from context.

The major driver for having multiple formats is context-dependent preferences for
binary-vs-human-readable and dense-vs-sparse.
For example, '`01`' is a dense text format and '`r8`' is a sparse binary format.
Sometimes you want to be able to eyeball your data, so you want a text format.
Other times you want maximum efficiency, so you want a binary format.
Sometimes your data is high entropy, with as many 1s as 0s, so you use a dense format.
Other times the data is highly biased, with 1s being much rarer and more interesting
than 0s, so you use a sparse format.

# Index
)MARKDOWN";
    for (const auto &kv : format_name_to_enum_map()) {
        all << "- [The **" << kv.first << "** Format](#" << kv.first << ")\n";
    }
    all << "\n\n";
    for (const auto &kv : format_name_to_enum_map()) {
        all << "# " << generate_per_format_markdown(kv.second, 0, true) << "\n";
    }
    result[std::string("FORMATS_MARKDOWN")] = all.str();

    return result;
}

std::map<std::string, std::string> generate_command_help_topics() {
    std::map<std::string, std::string> result;

    auto sub_command_data = make_sub_command_help();

    for (const auto &subcommand : sub_command_data) {
        result[to_upper_case(subcommand.subcommand_name)] = subcommand.str_help();
    }

    {
        std::stringstream markdown;
        markdown << "# Stim command line reference\n\n";
        markdown << "## Index\n\n";
        for (const auto &subcommand : sub_command_data) {
            markdown << "- [stim " << subcommand.subcommand_name << "](#" << subcommand.subcommand_name << ")\n";
        }
        markdown << "## Commands\n\n";
        for (const auto &subcommand : sub_command_data) {
            markdown << generate_subcommand_markdown(subcommand, 0, true) << "\n";
        }
        result["COMMANDS_MARKDOWN"] = markdown.str();
    }

    {
        std::stringstream commands_help;
        commands_help << "Available stim commands:\n\n";
        for (const auto &subcommand : sub_command_data) {
            commands_help << "    stim " << subcommand.subcommand_name
                          << std::string(20 - subcommand.subcommand_name.size(), ' ');
            auto summary = subcommand.description;
            auto n = summary.find('\n');
            if (n != std::string::npos) {
                summary = summary.substr(0, n);
            }
            commands_help << "# " << summary << "\n";
        }
        result["COMMANDS"] = commands_help.str();
    }

    result[""] = result["COMMANDS"] + R"PARAGRAPH(
Use `stim help [topic]` for help on specific topics. Available topics include:

    stim help commands  # List all tasks performed by stim.
    stim help gates     # List all circuit instructions supported by stim.
    stim help formats   # List all result formats supported by stim.
    stim help [command] # Print information about a command, e.g. "sample".
    stim help [gate]    # Print information about a gate, e.g. "CNOT".
    stim help [format]  # Print information about a result format, e.g. "01".
)PARAGRAPH";

    return result;
}

std::map<std::string, std::string> generate_gate_help_markdown() {
    std::map<std::string, std::string> result;
    for (const auto &e : GATE_DATA.hashed_name_to_gate_type_table) {
        if (!e.expected_name.empty()) {
            result[std::string(e.expected_name)] = generate_per_gate_help_markdown(GATE_DATA[e.id], 0, false);
        }
    }

    std::map<std::string, std::set<std::string>> categories;
    for (const auto &e : GATE_DATA.hashed_name_to_gate_type_table) {
        if (!e.expected_name.empty()) {
            const auto &rep = GATE_DATA.at(e.expected_name);
            categories[std::string(rep.category)].insert(std::string(e.expected_name));
        }
    }

    std::stringstream all;
    all << "Gates supported by Stim\n";
    all << "=======================\n";
    for (auto &category : categories) {
        all << category.first.substr(2) << ":\n";
        for (const auto &name : category.second) {
            all << "    " << name << "\n";
        }
    }

    result["GATES"] = all.str();

    all.str("");
    all << "# Gates supported by Stim\n\n";
    for (auto &category : categories) {
        all << "- " << category.first.substr(2) << "\n";
        for (const auto &name : category.second) {
            all << "    - [" << name << "](#" << name << ")\n";
        }
    }
    all << "\n";
    for (auto &category : categories) {
        all << "## " << category.first.substr(2) << "\n\n";
        for (const auto &name : category.second) {
            if (name == GATE_DATA.at(name).name) {
                all << generate_per_gate_help_markdown(GATE_DATA.at(name), 0, true) << "\n";
            }
        }
    }
    result[std::string("GATES_MARKDOWN")] = all.str();

    return result;
}

std::string stim::help_for(std::string help_key) {
    auto m1 = generate_gate_help_markdown();
    auto m2 = generate_format_help_markdown();
    auto m3 = generate_command_help_topics();

    auto key = to_upper_case(help_key);
    auto p = m1.find(key);
    if (p == m1.end()) {
        p = m2.find(key);
        if (p == m2.end()) {
            p = m3.find(key);
            if (p == m3.end()) {
                return "";
            }
        }
    }
    return p->second;
}

int stim::command_help(int argc, const char **argv) {
    const char *help = find_argument("--help", argc, argv);
    if (help == nullptr) {
        help = "\0";
    }
    if (help[0] == '\0' && argc == 3) {
        help = argv[2];
        // Handle usage like "stim sample --help".
        if (strcmp(help, "help") == 0 || strcmp(help, "--help") == 0) {
            help = argv[1];
        }
    }

    auto msg = help_for(help);
    if (msg == "") {
        std::cerr << "Unrecognized help topic '" << help << "'.\n";
        return EXIT_FAILURE;
    }
    std::cout << msg;
    return EXIT_SUCCESS;
}
