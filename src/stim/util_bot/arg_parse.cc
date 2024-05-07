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

#include "stim/util_bot/arg_parse.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>

using namespace stim;

std::string SubCommandHelp::str_help() const {
    std::stringstream ss;
    write_help(ss);
    return ss.str();
}

void write_indented(std::string_view s, std::ostream &out, size_t indent) {
    bool was_new_line = true;
    for (char c : s) {
        if (was_new_line && c != '\n') {
            for (size_t k = 0; k < indent; k++) {
                out.put(' ');
            }
        }
        out.put(c);
        was_new_line = c == '\n';
    }
}

void SubCommandHelp::write_help(std::ostream &out) const {
    std::vector<SubCommandHelpFlag> flags_copy = flags;
    std::sort(flags_copy.begin(), flags_copy.end(), [](const SubCommandHelpFlag &f1, const SubCommandHelpFlag &f2) {
        return f1.flag_name < f2.flag_name;
    });
    out << "NAME\n";
    out << "    stim " << subcommand_name << "\n\n";
    out << "SYNOPSIS\n";
    out << "    stim " << subcommand_name;
    for (const auto &flag : flags_copy) {
        out << " \\\n        ";
        bool allows_none =
            std::find(flag.allowed_values.begin(), flag.allowed_values.end(), "[none]") != flag.allowed_values.end();
        bool allows_empty =
            std::find(flag.allowed_values.begin(), flag.allowed_values.end(), "[switch]") != flag.allowed_values.end();
        if (allows_none) {
            out << "[";
        }
        out << flag.flag_name;
        if (flag.type != "bool") {
            out << " ";
            if (allows_empty) {
                out << "[";
            }
            out << flag.type;
            if (allows_empty) {
                out << "]";
            }
        }
        if (allows_none) {
            out << "]";
        }
    }
    out << "\n\n";
    out << "DESCRIPTION\n";
    write_indented(description, out, 4);
    out << "\n\n";
    if (!flags_copy.empty()) {
        out << "OPTIONS\n";
        for (const auto &f : flags_copy) {
            out << "    " << f.flag_name << "\n";
            write_indented(f.description, out, 8);
            out << "\n\n";
        }
    }
    if (!examples.empty()) {
        out << "EXAMPLES\n";
        for (size_t k = 0; k < examples.size(); k++) {
            if (k) {
                out << "\n\n";
            }
            out << "    Example #" << (k + 1) << "\n";
            write_indented(examples[k], out, 8);
        }
    }
}

const char *stim::require_find_argument(const char *name_c_str, int argc, const char **argv) {
    const char *result = find_argument(name_c_str, argc, argv);
    if (result == 0) {
        std::stringstream msg;
        msg << "\033[31mMissing command line argument: '" << name_c_str << "'";
        throw std::invalid_argument(msg.str());
    }
    return result;
}

const char *stim::find_argument(const char *name_c_str, int argc, const char **argv) {
    // Respect that the "--" argument terminates flags.
    size_t flag_count = 1;
    while (flag_count < (size_t)argc && strcmp(argv[flag_count], "--") != 0) {
        flag_count++;
    }

    // Search for the desired flag.
    size_t n = strlen(name_c_str);
    for (size_t i = 1; i < flag_count; i++) {
        // Check if argument starts with expected flag.
        const char *loc = strstr(argv[i], name_c_str);
        if (loc != argv[i] || (loc[n] != '\0' && loc[n] != '=')) {
            continue;
        }

        // If the flag is alone and followed by the end or another flag, no
        // argument was provided. Return the empty string to indicate this.
        if (loc[n] == '\0' && ((int)i == argc - 1 || (argv[i + 1][0] == '-' && !isdigit(argv[i + 1][1])))) {
            return argv[i] + n;
        }

        // If the flag value is specified inline with '=', return a pointer to
        // the start of the value within the flag string.
        if (loc[n] == '=') {
            // Argument provided inline.
            return loc + n + 1;
        }

        // The argument value is specified by the next command line argument.
        return argv[i + 1];
    }

    // Not found.
    return 0;
}

void stim::check_for_unknown_arguments(
    const std::vector<const char *> &known_arguments,
    const std::vector<const char *> &known_but_deprecated_arguments,
    const char *for_mode,
    int argc,
    const char **argv) {
    for (int i = 1; i < argc; i++) {
        if (for_mode != nullptr && i == 1 && strcmp(argv[i], for_mode) == 0) {
            continue;
        }
        // Respect that the "--" argument terminates flags.
        if (!strcmp(argv[i], "--")) {
            break;
        }

        // Check if there's a matching command line argument.
        int matched = 0;
        std::array<const std::vector<const char *> *, 2> both{&known_arguments, &known_but_deprecated_arguments};
        for (const auto &knowns : both) {
            for (const auto &known : *knowns) {
                const char *loc = strstr(argv[i], known);
                size_t n = strlen(known);
                if (loc == argv[i] && (loc[n] == '\0' || loc[n] == '=')) {
                    // Skip words that are values for a previous flag.
                    if (loc[n] == '\0' && i < argc - 1 && argv[i + 1][0] != '-') {
                        i++;
                    }
                    matched = 1;
                    break;
                }
            }
        }

        // Print error and exit if flag is not recognized.
        if (!matched) {
            std::stringstream msg;
            if (for_mode == nullptr) {
                msg << "Unrecognized command line argument " << argv[i] << ".\n";
                msg << "Recognized command line arguments:\n";
            } else {
                msg << "Unrecognized command line argument " << argv[i] << " for `stim " << for_mode << "`.\n";
                msg << "Recognized command line arguments for `stim " << for_mode << "`:\n";
            }
            std::set<std::string> known_sorted;
            for (const auto &v : known_arguments) {
                known_sorted.insert(v);
            }
            for (const auto &v : known_sorted) {
                msg << "    " << v << "\n";
            }
            throw std::invalid_argument(msg.str());
        }
    }
}

bool stim::find_bool_argument(const char *name_c_str, int argc, const char **argv) {
    const char *text = find_argument(name_c_str, argc, argv);
    if (text == nullptr) {
        return false;
    }
    if (text[0] == '\0') {
        return true;
    }
    std::stringstream msg;
    msg << "Got non-empty value '" << text << "' for boolean flag '" << name_c_str << "'.";
    throw std::invalid_argument(msg.str());
}

bool stim::parse_int64(std::string_view data, int64_t *out) {
    if (data.empty()) {
        return false;
    }
    bool negate = false;
    if (data.starts_with("-")) {
        negate = true;
        data = data.substr(1);
    } else if (data.starts_with("+")) {
        data = data.substr(1);
    }

    uint64_t accumulator = 0;
    for (char c : data) {
        if (!(c >= '0' && c <= '9')) {
            return false;
        }
        uint64_t digit = c - '0';
        uint64_t next = accumulator * 10 + digit;
        if (accumulator != (next - digit) / 10) {
            return false;  // Overflow.
        }
        accumulator = next;
    }

    if (negate && accumulator == (uint64_t)INT64_MAX + uint64_t{1}) {
        *out = INT64_MIN;
        return true;
    }
    if (accumulator > INT64_MAX) {
        return false;
    }

    *out = (int64_t)accumulator;
    if (negate) {
        *out *= -1;
    }
    return true;
}

int64_t stim::find_int64_argument(
    const char *name_c_str, int64_t default_value, int64_t min_value, int64_t max_value, int argc, const char **argv) {
    const char *text = find_argument(name_c_str, argc, argv);
    if (text == nullptr || text[0] == '\0') {
        if (default_value < min_value || default_value > max_value) {
            std::stringstream msg;
            msg << "Must specify a value for int flag '" << name_c_str << "'.";
            throw std::invalid_argument(msg.str());
        }
        return default_value;
    }

    // Attempt to parse.
    int64_t i;
    if (!parse_int64(text, &i)) {
        std::stringstream msg;
        msg << "Got non-int64 value '" << text << "' for int64 flag '" << name_c_str << "'.";
        throw std::invalid_argument(msg.str());
    }

    // In range?
    if (i < min_value || i > max_value) {
        std::stringstream msg;
        msg << "Integer value '" << text << "' for flag '" << name_c_str << "' doesn't satisfy " << min_value
            << " <= " << i << " <= " << max_value << ".";
        throw std::invalid_argument(msg.str());
    }

    return i;
}

float stim::find_float_argument(
    const char *name_c_str, float default_value, float min_value, float max_value, int argc, const char **argv) {
    const char *text = find_argument(name_c_str, argc, argv);
    if (text == nullptr) {
        if (default_value < min_value || default_value > max_value) {
            std::stringstream msg;
            msg << "Must specify a value for float flag '" << name_c_str << "'.";
            throw std::invalid_argument(msg.str());
        }
        return default_value;
    }

    // Attempt to parse.
    char *processed;
    float f = strtof(text, &processed);
    if (*processed != '\0') {
        std::stringstream msg;
        msg << "Got non-float value '" << text << "' for float flag '" << name_c_str << "'.";
        throw std::invalid_argument(msg.str());
    }

    // In range?
    if (f < min_value || f > max_value || f != f) {
        std::stringstream msg;
        msg << "Float value '" << text << "' for flag '" << name_c_str << "' doesn't satisfy " << min_value
            << " <= " << f << " <= " << max_value << ".";
        throw std::invalid_argument(msg.str());
    }

    return f;
}

FILE *stim::find_open_file_argument(
    const char *name_c_str, FILE *default_file, const char *mode, int argc, const char **argv) {
    const char *path_c_str = find_argument(name_c_str, argc, argv);
    if (path_c_str == nullptr) {
        if (default_file == nullptr) {
            std::stringstream msg;
            msg << "Missing command line argument: '" << name_c_str << "'";
            throw std::invalid_argument(msg.str());
        }
        return default_file;
    }
    if (*path_c_str == '\0') {
        std::stringstream msg;
        msg << "Command line argument '" << name_c_str << "' can't be empty. It's supposed to be a file path.";
        throw std::invalid_argument(msg.str());
    }
    FILE *file = fopen(path_c_str, mode);
    if (file == nullptr) {
        std::stringstream msg;
        msg << "Failed to open '" << path_c_str << "'";
        throw std::invalid_argument(msg.str());
    }
    return file;
}

ostream_else_cout::ostream_else_cout(std::unique_ptr<std::ostream> &&held) : held(std::move(held)) {
}

std::ostream &ostream_else_cout::stream() {
    if (held) {
        return *held;
    } else {
        return std::cout;
    }
}

ostream_else_cout stim::find_output_stream_argument(
    const char *name_c_str, bool default_std_out, int argc, const char **argv) {
    const char *path_c_str = find_argument(name_c_str, argc, argv);
    if (path_c_str == nullptr) {
        if (!default_std_out) {
            std::stringstream msg;
            msg << "Missing command line argument: '" << name_c_str << "'";
            throw std::invalid_argument(msg.str());
        }
        return {nullptr};
    }
    if (*path_c_str == '\0') {
        std::stringstream msg;
        msg << "Command line argument '" << name_c_str << "' can't be empty. It's supposed to be a file path.";
        throw std::invalid_argument(msg.str());
    }
    std::unique_ptr<std::ostream> f(new std::ofstream(path_c_str));
    if (f->fail()) {
        std::stringstream msg;
        msg << "Failed to open '" << path_c_str << "'";
        throw std::invalid_argument(msg.str());
    }
    return {std::move(f)};
}

std::vector<std::string_view> stim::split_view(char splitter, std::string_view text) {
    std::vector<std::string_view> result;
    size_t start = 0;
    for (size_t k = 0; k < text.size(); k++) {
        if (text[k] == splitter) {
            result.push_back(text.substr(start, k - start));
            start = k + 1;
        }
    }
    result.push_back(text.substr(start, text.size() - start));
    return result;
}

static double parse_exact_double_from_null_terminated(const char *c, size_t size) {
    char *end = nullptr;
    double d = strtod(c, &end);
    if (size > 0 && !isspace(*c)) {
        if (end == c + size && !std::isinf(d) && !std::isnan(d)) {
            return d;
        }
    }
    std::stringstream ss;
    ss << "Not an exact finite double: '" << c << "'";
    throw std::invalid_argument(ss.str());
}

double stim::parse_exact_double_from_string(std::string_view text) {
    if (text.size() + 1 < 15) {
        char buf[16];
        memcpy(buf, text.data(), text.size());
        buf[text.size()] = '\0';
        return parse_exact_double_from_null_terminated(&buf[0], text.size());
    } else {
        std::string s(text);
        return parse_exact_double_from_null_terminated(s.c_str(), text.size());
    }
}

static bool try_parse_exact_uint64_t_from_string(std::string_view text, uint64_t *out) {
    if (text.empty()) {
        return false;
    }
    if (text[0] == '-') {
        return false;
    }
    size_t k = 0;
    if (text[k] == '+') {
        k += 1;
    }
    uint64_t acc = 0;
    while (k < text.size()) {
        char c = text[k];
        if (c < '0' || c > '9') {
            return false;
        }
        if (acc > UINT64_MAX / 10) {
            return false;
        }
        acc *= 10;
        uint8_t d = c - '0';
        if (acc > UINT64_MAX - d) {
            return false;
        }
        acc += d;
        k++;
    }
    *out = acc;
    return true;
}

uint64_t stim::parse_exact_uint64_t_from_string(std::string_view text) {
    uint64_t result = 0;
    if (try_parse_exact_uint64_t_from_string(text, &result)) {
        return result;
    }
    std::stringstream ss;
    ss << "Not an exact integer that can be stored in a uint64_t: '" << text << "'";
    throw std::invalid_argument(ss.str());
}
