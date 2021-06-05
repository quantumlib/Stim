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

#include "arg_parse.h"

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace stim_internal;

const char *stim_internal::require_find_argument(const char *name, int argc, const char **argv) {
    const char *result = find_argument(name, argc, argv);
    if (result == 0) {
        fprintf(stderr, "\033[31mMissing command line argument: '%s'\033[0m\n", name);
        exit(EXIT_FAILURE);
    }
    return result;
}

const char *stim_internal::find_argument(const char *name, int argc, const char **argv) {
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

void stim_internal::check_for_unknown_arguments(
    const std::vector<const char *> &known_arguments, const char *for_mode, int argc, const char **argv) {
    for (int i = 1; i < argc; i++) {
        // Respect that the "--" argument terminates flags.
        if (!strcmp(argv[i], "--")) {
            break;
        }

        // Check if there's a matching command line argument.
        int matched = 0;
        for (size_t j = 0; j < known_arguments.size(); j++) {
            const char *loc = strstr(argv[i], known_arguments[j]);
            size_t n = strlen(known_arguments[j]);
            if (loc == argv[i] && (loc[n] == '\0' || loc[n] == '=')) {
                // Skip words that are values for a previous flag.
                if (loc[n] == '\0' && i < argc - 1 && argv[i + 1][0] != '-') {
                    i++;
                }
                matched = 1;
                break;
            }
        }

        // Print error and exit if flag is not recognized.
        if (!matched) {
            if (for_mode == nullptr) {
                fprintf(stderr, "\033[31mUnrecognized command line argument %s.\n", argv[i]);
                fprintf(stderr, "Recognized command line arguments:\n");
            } else {
                fprintf(stderr, "\033[31mUnrecognized command line argument %s for mode %s.\n", argv[i], for_mode);
                fprintf(stderr, "Recognized command line arguments for mode %s:\n", for_mode);
            }
            for (size_t j = 0; j < known_arguments.size(); j++) {
                fprintf(stderr, "    %s\n", known_arguments[j]);
            }
            fprintf(stderr, "\033[0m");
            exit(EXIT_FAILURE);
        }
    }
}

bool stim_internal::find_bool_argument(const char *name, int argc, const char **argv) {
    const char *text = find_argument(name, argc, argv);
    if (text == nullptr) {
        return false;
    }
    if (text[0] == '\0') {
        return true;
    }
    fprintf(stderr, "\033[31mGot non-empty value '%s' for boolean flag '%s'.\033[0m\n", text, name);
    exit(EXIT_FAILURE);
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
            return false; // Overflow.
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

int64_t stim_internal::find_int64_argument(const char *name, int64_t default_value, int64_t min_value, int64_t max_value, int argc, const char **argv) {
    const char *text = find_argument(name, argc, argv);
    if (text == nullptr || text[0] == '\0') {
        if (default_value < min_value || default_value > max_value) {
            fprintf(
                stderr,
                "\033[31m"
                "Must specify a value for int flag '%s'.\n"
                "\033[0m",
                name);
            exit(EXIT_FAILURE);
        }
        return default_value;
    }

    // Attempt to parse.
    int64_t i;
    if (!parse_int64(text, &i)) {
        fprintf(stderr, "\033[31mGot non-int64 value '%s' for int64 flag '%s'.\033[0m\n", text, name);
        exit(EXIT_FAILURE);
    }

    // In range?
    if (i < min_value || i > max_value) {
        std::cerr
            << "\033[31mInteger value '" << text
            << "' for flag '" << name
            << "' doesn't satisfy " << min_value << " <= " << i << " <= " << max_value
            << ".\033[0m\n";
        exit(EXIT_FAILURE);
    }

    return i;
}

float stim_internal::find_float_argument(
    const char *name, float default_value, float min_value, float max_value, int argc, const char **argv) {
    const char *text = find_argument(name, argc, argv);
    if (text == nullptr) {
        if (default_value < min_value || default_value > max_value) {
            fprintf(
                stderr,
                "\033[31m"
                "Must specify a value for float flag '%s'.\n"
                "\033[0m",
                name);
            exit(EXIT_FAILURE);
        }
        return default_value;
    }

    // Attempt to parse.
    char *processed;
    float f = strtof(text, &processed);
    if (*processed != '\0') {
        fprintf(stderr, "\033[31mGot non-float value '%s' for float flag '%s'.\033[0m\n", text, name);
        exit(EXIT_FAILURE);
    }

    // In range?
    if (f < min_value || f > max_value || f != f) {
        fprintf(
            stderr, "\033[31mFloat value '%s' for flag '%s' doesn't satisfy %f <= %f <= %f.\033[0m\n", text, name,
            min_value, f, max_value);
        exit(EXIT_FAILURE);
    }

    return f;
}

FILE *stim_internal::find_open_file_argument(
    const char *name, FILE *default_file, const char *mode, int argc, const char **argv) {
  const char *path = find_argument(name, argc, argv);
  if (path == nullptr) {
    if (default_file == nullptr) {
      std::cerr << "\033[31mMissing command line argument: '" << name << "'\033[0m\n";
      exit(EXIT_FAILURE);
    }
    return default_file;
  }
  if (*path == '\0') {
    std::cerr << "\033[31mCommand line argument '" << name
              << "' can't be empty. It's supposed to be a file path.\033[0m\n";
    exit(EXIT_FAILURE);
  }
  FILE *file = fopen(path, mode);
  if (file == nullptr) {
    std::cerr << "\033[31mFailed to open '" << path << "'\033[0m\n";
    exit(EXIT_FAILURE);
  }
  return file;
}
