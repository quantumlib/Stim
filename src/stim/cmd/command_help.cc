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

#include "stim/arg_parse.h"
#include "stim/circuit/gate_data.h"
#include "stim/io/stim_data_formats.h"
#include "stim/stabilizers/tableau.h"
#include "stim/cmd/command_analyze_errors.h"
#include "command_detect.h"
#include "command_explain_errors.h"
#include "command_gen.h"
#include "command_m2d.h"

using namespace stim;

struct CommandLineSingleModeData {
    std::string mode_summary;
    std::string mode_description;
    std::set<std::string> flags;
};

struct CommandLineFlagData {
    std::map<std::string, std::string> non_mode_help;
    std::map<std::string, CommandLineSingleModeData> mode_help;
};

std::string stim::clean_doc_string(const char *c) {
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
        if (line_length > 80) {
            const char *start_of_line = result.c_str() + result.size() - line_length - 1;
            if (memcmp(start_of_line, "@signature", strlen("@signature")) != 0 &&
                memcmp(start_of_line, "@overload", strlen("@overload")) != 0 &&
                memcmp(start_of_line, "https://", strlen("https://")) != 0) {
                std::stringstream ss;
                ss << "Docstring line has length " << line_length << " > 80:\n" << start_of_line << std::string(80, '^') << "\n";
                throw std::invalid_argument(ss.str());
            }
        }
    }

    return result;
}

CommandLineFlagData make_mode_help() {
    std::map<std::string, CommandLineSingleModeData> modes;
    std::map<std::string, std::string> flags;

    modes["help"] = CommandLineSingleModeData{
        "Prints helpful information about stim.",
        R"PARAGRAPH(
Use `stim help [topic]` for information about specific topics. Available topics include:

    stim help gates    # List all circuit instructions supported by stim.
    stim help formats  # List all result formats supported by stim.
    stim help modes    # List all tasks performed by stim.
    stim help flags    # List all command line flags supported by stim.
    stim help [mode]   # Print information about a mode, such as `sample` or `analyze_errors`.
    stim help [flag]   # Print information about a command line flag, such as `--out` or `--in_format`.
    stim help [gate]   # Print information about a circuit instruction, such as the `CNOT` gate.
    stim help [format] # Print information about a supported result format, such as the `01` format.
)PARAGRAPH",
        {}};

    modes["repl"] = CommandLineSingleModeData{
        "Read-eval-print-loop mode.",
        R"PARAGRAPH(
Reads operations from stdin while immediately writing measurement results to stdout.

stdin: A circuit to execute.

stdout: Measurement results.

stderr: Ignored errors encountered while parsing/simulating the circuit arriving via stdin.

- Example:

    ```
    >>> stim repl
    ... M 0
    0
    ... X 0
    ... M 0
    1
    ... X 2 3 9
    ... M 0 1 2 3 4 5 6 7 8 9
    1 0 1 1 0 0 0 0 0 1
    ... REPEAT 5 {
    ...     R 0 1
    ...     H 0
    ...     CNOT 0 1
    ...     M 0 1
    ... }
    00
    11
    11
    00
    11
    ```
)PARAGRAPH",
        {},
    };

    modes["sample"] = CommandLineSingleModeData{
        "Samples measurements from a circuit.",
        R"PARAGRAPH(
stdin: The circuit to sample from, specified using the [stim circuit file format](https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md).

stdout: The sample data.

- Examples:

    ```
    >>> stim sample --shots 5
    ... H 0
    ... CNOT 0 1
    ... M 0 1
    00
    11
    11
    00
    11
    ```

    ```
    >>> stim sample --out_format dets
    ... X 2 3 5
    ... M 0 1 2 3 4 5 6 7 8 9
    shot M2 M3 M5
    ```
)PARAGRAPH",
        {"--out_format", "--seed", "--in", "--out", "--skip_reference_sample", "--shots"},
    };

    modes["sample_dem"] = CommandLineSingleModeData{
        "Samples detection events and observable flips from a detector error model.",
        R"PARAGRAPH(
stdin (or --in): The detector error model to sample from, specified using the [detector error model file format](https://github.com/quantumlib/Stim/blob/main/doc/file_format_dem_detector_error_model.md).

stdout (or --out): The detection event data is written here.

- Example:

    ```bash
    echo "error(0) D0" > example.dem
    echo "error(0.5) D1 L0" >> example.dem
    echo "error(1) D2 D3" >> example.dem
    stim sample_dem \
        --shots 5 \
        --in example.dem \
        --out dets.01 \
        --out_format 01 \
        --obs_out obs_flips.01 \
        --obs_out_format 01 \
        --seed 0
    cat dets.01
    # 0111
    # 0011
    # 0011
    # 0111
    # 0111
    cat obs_flips.01
    # 1
    # 0
    # 0
    # 1
    # 1
    ```
)PARAGRAPH",
        {
            "--in",
            "--out",
            "--out_format",
            "--obs_out",
            "--obs_out_format",
            "--seed",
            "--shots",
            "--err_out",
            "--err_out_format",
            "--replay_err_in",
            "--replay_err_in_format",
        },
    };

    flags["--err_out"] = R"PARAGRAPH(Specifies a file to write a record of which errors occurred.

This data can then be analyzed, modified, and later given to for example a --replay_err_in argument.
)PARAGRAPH";

    flags["--err_out_format"] = R"PARAGRAPH(The format to use when writing error data (e.g. b8 or 01).
)PARAGRAPH";

    flags["--replay_err_in"] = R"PARAGRAPH(Specifies a file to read error data to replay from.

When replaying error information, errors are no longer sampled randomly but instead driven by the file data.
For example, this file data could come from a previous run that wrote error data using --err_out.
)PARAGRAPH";

    flags["--replay_err_in_format"] = R"PARAGRAPH(The format to use when reading error data to replay. (e.g. b8 or 01).
)PARAGRAPH";

    flags["--in"] = R"PARAGRAPH(Specifies an input file to read from, instead of stdin.

What the file is used for depends on the mode stim is executing in. For example, in `stim sample` mode the circuit to
sample from is read from stdin (or the file specified by `--in`) whereas in `--m2d` mode the measurement data to convert
is read from stdin (or the file specified by `--in`).
)PARAGRAPH";

    flags["--out"] = R"PARAGRAPH(Specifies an output file to read from, instead of stdout.

What the output is used for depends on the mode stim is executing in. For example, in `stim gen` mode the generated circuit
is written to stdout (or the file specified by `--out`) whereas in `stim sample` mode the sampled measurement data is
written to stdout (or the file specified by `--out`).
)PARAGRAPH";

    std::vector<SubCommandHelp> sub_commands{
        command_analyze_errors_help(),
        command_detect_help(),
        command_explain_errors_help(),
        command_gen_help(),
        command_m2d_help(),
    };
    for (const auto &sub_command : sub_commands) {
        auto summary = sub_command.description;
        if (summary.find("\n") != std::string::npos) {
            summary = summary.substr(0, summary.find("\n"));
        }
        modes[sub_command.subcommand_name] = CommandLineSingleModeData{
            summary,
            sub_command.str_help(),
            sub_command.flag_set(),
        };
        for (const auto& f: sub_command.flags) {
            if (flags.find(f.flag_name) == flags.end()) {
                flags[f.flag_name] = f.description;
            }
        }
    }

    return {flags, modes};
}

std::string upper(const std::string &val) {
    std::string copy = val;
    for (char &c : copy) {
        c = toupper(c);
    }
    return copy;
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

void print_example(Acc &out, const char *name, const Gate &gate) {
    out << "\nExample:\n";
    out.change_indent(+4);
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
    if (gate.flags & GATE_CAN_TARGET_BITS) {
        if (gate.name[0] == 'C' || gate.name[0] == 'Z') {
            out << gate.name << " rec[-1] 111\n";
        }
        if (gate.name[gate.name_len - 1] == 'Z') {
            out << gate.name << " 111 rec[-1]\n";
        }
    }
    out.change_indent(-4);
}

void print_decomposition(Acc &out, const Gate &gate) {
    const char *decomposition = gate.extra_data_func().h_s_cx_m_r_decomposition;
    if (decomposition != nullptr) {
        std::stringstream undecomposed;
        undecomposed << gate.name << " 0";
        if (gate.flags & GATE_TARGETS_PAIRS) {
            undecomposed << " 1";
        }

        out << "Decomposition (into H, S, CX, M, R):\n";
        out.change_indent(+4);
        out << "# The following circuit is equivalent (up to global phase) to `";
        out << undecomposed.str() << "`";
        out << decomposition;
        if (Circuit(decomposition) == Circuit(undecomposed.str().data())) {
            out << "\n# (The decomposition is trivial because this gate is in the target gate set.)\n";
        }
        out.change_indent(-4);
    }
}

void print_stabilizer_generators(Acc &out, const Gate &gate) {
    if (gate.flags & GATE_IS_UNITARY) {
        out << "Stabilizer Generators:\n";
        out.change_indent(+4);
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
        out.change_indent(-4);
    } else {
        auto data = gate.extra_data_func();
        if (data.tableau_data.size()) {
            out << "Stabilizer Generators:\n";
            out.change_indent(+4);
            for (const auto &e : data.tableau_data) {
                out << e << "\n";
            }
            out.change_indent(-4);
        }
    }
}

void print_bloch_vector(Acc &out, const Gate &gate) {
    if (!(gate.flags & GATE_IS_UNITARY) || (gate.flags & GATE_TARGETS_PAIRS)) {
        return;
    }

    out << "Bloch Rotation:\n";
    out.change_indent(+4);
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
    out.change_indent(-4);
}

void print_unitary_matrix(Acc &out, const Gate &gate) {
    if (!(gate.flags & GATE_IS_UNITARY)) {
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
    out << data.help;
    if (gate.flags & GATE_PRODUCES_NOISY_RESULTS) {
        out << "If this gate is parameterized by a probability argument, the "
               "recorded result will be flipped with that probability. "
               "If not, the recorded result is noiseless. "
               "Note that the noise only affects the recorded result, not the "
               "target qubit's state.\n\n";
        out << "Prefixing a target with ! inverts its recorded measurement result.\n";
    }

    if (std::string(data.help).find("xample:\n") == std::string::npos) {
        print_example(out, alt_gate.name, gate);
    }
    print_stabilizer_generators(out, gate);
    print_bloch_vector(out, gate);
    print_unitary_matrix(out, gate);
    print_decomposition(out, gate);
    out.flush();
    return out.settled;
}

std::string generate_per_mode_help(
    const std::string &mode_name, const CommandLineSingleModeData &data) {
    std::stringstream out;

    if (data.mode_description.find("SYNOPSIS\n") != std::string::npos) {
        return data.mode_description;
    }

    out << "### stim " << mode_name << "\n\n";
    out << "*" << data.mode_summary << "*\n";
    out << data.mode_description;
    if (!data.flags.empty()) {
        out << "\nFlags used with this mode:\n";
        for (const auto &e : data.flags) {
            out << "    " << e << "\n";
        }
    }

    return out.str();
}

std::string generate_per_mode_markdown(
    const std::string &mode_name, const CommandLineSingleModeData &data, int indent, bool anchor) {
    Acc out;
    out.indent = indent;
    if (anchor) {
        out << "<a name=\"" << mode_name << "\"></a>\n";
    }
    if (data.mode_description.find("SYNOPSIS\n") != std::string::npos) {
        out << "### stim " << mode_name << "\n\n";
        out << "```\n";
        out << data.mode_description << "\n";
        out << "```\n";

        out.flush();
        return out.settled;
    }

    out << "### stim " << mode_name << "\n\n";
    out << "*" << data.mode_summary << "*\n";
    out << data.mode_description;
    if (!data.flags.empty()) {
        out << "\nFlags used with this mode:\n";
        for (const auto &e : data.flags) {
            if (anchor) {
                out << "- [" << e << "](#" << e << ")\n";
            } else {
                out << "    " << e << "\n";
            }
        }
    }

    out.flush();
    return out.settled;
}

std::string generate_per_flag_markdown(const std::string &flag_name, const std::string &desc, int indent, bool anchor) {
    Acc out;
    out.indent = indent;
    if (anchor) {
        out << "<a name=\"" << flag_name << "\"></a>";
    }
    out << "**`" << flag_name << "`**\n";
    out << desc;
    out << "\n";

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
    for (const auto &kv : format_name_to_enum_map) {
        all << kv.first << "\n";
    }
    result[std::string("FORMATS")] = all.str();

    for (const auto &kv : format_name_to_enum_map) {
        result[upper(kv.first)] = generate_per_format_markdown(kv.second, 0, false);
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
    for (const auto &kv : format_name_to_enum_map) {
        all << "- [The **" << kv.first << "** Format](#" << kv.first << ")\n";
    }
    all << "\n\n";
    for (const auto &kv : format_name_to_enum_map) {
        all << "# " << generate_per_format_markdown(kv.second, 0, true) << "\n";
    }
    result[std::string("FORMATS_MARKDOWN")] = all.str();

    return result;
}

std::map<std::string, std::string> generate_flag_help_markdown() {
    std::map<std::string, std::string> result;

    CommandLineFlagData data = make_mode_help();

    std::stringstream markdown;

    markdown << "# Stim command line reference\n\n";
    markdown << "## Index\n\n";
    for (const auto &kv : data.mode_help) {
        markdown << "- [stim " << kv.first << "](#" << kv.first << ")\n";
    }

    markdown << "## Commands\n\n";
    for (const auto &kv : data.mode_help) {
        std::string key = upper(kv.first);
        while (true) {
            result[key] = generate_per_mode_help(kv.first, kv.second);
            if (key[0] == '-') {
                key.erase(key.begin());
            } else {
                break;
            }
        }
        markdown << generate_per_mode_markdown(kv.first, kv.second, 0, true) << "\n";
    }
    markdown << "## Flags\n\n";
    for (const auto &kv : data.non_mode_help) {
        std::string key = upper(kv.first);
        while (true) {
            result[key] = generate_per_flag_markdown(kv.first, kv.second, 0, false);
            if (key[0] == '-') {
                key.erase(key.begin());
            } else {
                break;
            }
        }
        markdown << "- " << generate_per_flag_markdown(kv.first, kv.second, 4, true) << "\n";
    }
    result["FLAGS_MARKDOWN"] = markdown.str();

    std::stringstream flags;
    flags << "Available stim commands:\n\n";
    for (const auto &kv : data.mode_help) {
        flags << "    stim " << kv.first << std::string(20 - kv.first.size(), ' ') << "# " << kv.second.mode_summary
              << "\n";
    }
    result["MODES"] = flags.str();
    flags << "\nOther flags:\n";
    for (const auto &kv : data.non_mode_help) {
        flags << "    " << kv.first << "\n";
    }
    result["FLAGS"] = flags.str();

    result[""] = result["MODES"] + "\n" + data.mode_help["help"].mode_description;

    return result;
}

std::map<std::string, std::string> generate_gate_help_markdown() {
    std::map<std::string, std::string> result;
    for (const auto &g : GATE_DATA.gates()) {
        result[g.name] = generate_per_gate_help_markdown(g, 0, false);
    }

    std::map<std::string, std::set<std::string>> categories;
    for (const auto &g : GATE_DATA.gates()) {
        const auto &rep = GATE_DATA.at(g.name);
        categories[std::string(rep.extra_data_func().category)].insert(g.name);
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
    auto m3 = generate_flag_help_markdown();

    auto key = upper(help_key);
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
