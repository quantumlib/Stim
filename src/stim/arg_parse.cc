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

#include "stim/arg_parse.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

using namespace stim;

const char *stim::require_find_argument(const char *name, int argc, const char **argv) {
    const char *result = find_argument(name, argc, argv);
    if (result == 0) {
        std::stringstream msg;
        msg << "\033[31mMissing command line argument: '" << name << "'";
        throw std::invalid_argument(msg.str());
    }
    return result;
}

const char *stim::find_argument(const char *name, int argc, const char **argv) {
    // Respect that the "--" argument terminates flags.
    size_t flag_count = 1;
    while (flag_count < (size_t)argc && strcmp(argv[flag_count], "--") != 0) {
        flag_count++;
    }

    // Search for the desired flag.
    size_t n = strlen(name);
    for (size_t i = 1; i < flag_count; i++) {
        // Check if argument starts with expected flag.
        const char *loc = strstr(argv[i], name);
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

bool stim::find_bool_argument(const char *name, int argc, const char **argv) {
    const char *text = find_argument(name, argc, argv);
    if (text == nullptr) {
        return false;
    }
    if (text[0] == '\0') {
        return true;
    }
    std::stringstream msg;
    msg << "Got non-empty value '" << text << "' for boolean flag '" << name << "'.";
    throw std::invalid_argument(msg.str());
}

bool parse_int64(const char *data, int64_t *out) {
    char c = *data;
    if (c == 0) {
        return false;
    }
    bool negate = false;
    if (c == '-') {
        negate = true;
        data++;
        c = *data;
    }

    uint64_t accumulator = 0;
    while (c) {
        if (!(c >= '0' && c <= '9')) {
            return false;
        }
        uint64_t digit = c - '0';
        uint64_t next = accumulator * 10 + digit;
        if (accumulator != (next - digit) / 10) {
            return false;  // Overflow.
        }
        accumulator = next;
        data++;
        c = *data;
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
    const char *name, int64_t default_value, int64_t min_value, int64_t max_value, int argc, const char **argv) {
    const char *text = find_argument(name, argc, argv);
    if (text == nullptr || text[0] == '\0') {
        if (default_value < min_value || default_value > max_value) {
            std::stringstream msg;
            msg << "Must specify a value for int flag '" << name << "'.";
            throw std::invalid_argument(msg.str());
        }
        return default_value;
    }

    // Attempt to parse.
    int64_t i;
    if (!parse_int64(text, &i)) {
        std::stringstream msg;
        msg << "Got non-int64 value '" << text << "' for int64 flag '" << name << "'.";
        throw std::invalid_argument(msg.str());
    }

    // In range?
    if (i < min_value || i > max_value) {
        std::stringstream msg;
        msg << "Integer value '" << text << "' for flag '" << name << "' doesn't satisfy " << min_value << " <= " << i
            << " <= " << max_value << ".";
        throw std::invalid_argument(msg.str());
    }

    return i;
}

float stim::find_float_argument(
    const char *name, float default_value, float min_value, float max_value, int argc, const char **argv) {
    const char *text = find_argument(name, argc, argv);
    if (text == nullptr) {
        if (default_value < min_value || default_value > max_value) {
            std::stringstream msg;
            msg << "Must specify a value for float flag '" << name << "'.";
            throw std::invalid_argument(msg.str());
        }
        return default_value;
    }

    // Attempt to parse.
    char *processed;
    float f = strtof(text, &processed);
    if (*processed != '\0') {
        std::stringstream msg;
        msg << "Got non-float value '" << text << "' for float flag '" << name << "'.";
        throw std::invalid_argument(msg.str());
    }

    // In range?
    if (f < min_value || f > max_value || f != f) {
        std::stringstream msg;
        msg << "Float value '" << text << "' for flag '" << name << "' doesn't satisfy " << min_value << " <= " << f
            << " <= " << max_value << ".";
        throw std::invalid_argument(msg.str());
    }

    return f;
}

FILE *stim::find_open_file_argument(
    const char *name, FILE *default_file, const char *mode, int argc, const char **argv) {
    const char *path = find_argument(name, argc, argv);
    if (path == nullptr) {
        if (default_file == nullptr) {
            std::stringstream msg;
            msg << "Missing command line argument: '" << name << "'";
            throw std::invalid_argument(msg.str());
        }
        return default_file;
    }
    if (*path == '\0') {
        std::stringstream msg;
        msg << "Command line argument '" << name << "' can't be empty. It's supposed to be a file path.";
        throw std::invalid_argument(msg.str());
    }
    FILE *file = fopen(path, mode);
    if (file == nullptr) {
        std::stringstream msg;
        msg << "Failed to open '" << path << "'";
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
    const char *name, bool default_std_out, int argc, const char **argv) {
    const char *path = find_argument(name, argc, argv);
    if (path == nullptr) {
        if (!default_std_out) {
            std::stringstream msg;
            msg << "Missing command line argument: '" << name << "'";
            throw std::invalid_argument(msg.str());
        }
        return {nullptr};
    }
    if (*path == '\0') {
        std::stringstream msg;
        msg << "Command line argument '" << name << "' can't be empty. It's supposed to be a file path.";
        throw std::invalid_argument(msg.str());
    }
    std::unique_ptr<std::ostream> f(new std::ofstream(path));
    if (f->fail()) {
        std::stringstream msg;
        msg << "Failed to open '" << path << "'";
        throw std::invalid_argument(msg.str());
    }
    return {std::move(f)};
}
