/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _STIM_ARG_PARSE_H
#define _STIM_ARG_PARSE_H

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace stim {

/// Searches through command line flags for a particular flag's argument.
///
/// Args:
///     name: The flag's name, including any hyphens. For example, "-mode".
///     argc: Number of command line arguments.
///     argv: Array of command line argument strings.
///
/// Returns:
///     A pointer to the command line flag's value string, or else 0 if the flag
///     is not specified. Flags that are set without specifying a value will
///     cause the method to return a pointer to an empty string.
const char *find_argument(const char *name, int argc, const char **argv);

/// Searches through command line flags for a particular flag's argument.
///
/// Args:
///     name: The flag's name, including any hyphens. For example, "-mode".
///     argc: Number of command line arguments.
///     argv: Array of command line argument strings.
///
/// Returns:
///     A pointer to the command line flag's value string. Flags that are set
///     without specifying a value will cause the method to return a pointer to
///     an empty string.
///
/// Raises:
///     std::invalid_argument: The argument isn't present.
const char *require_find_argument(const char *name, int argc, const char **argv);

/// Checks that all command line arguments are recognized.
///
/// Args:
///     known_arguments: Names of known arguments.
///     for_mode: Can be set to nullptr. Modifies error message.
///     argc: Number of command line arguments.
///     argv: Array of command line argument strings.
///
/// Raises:
///     std::invalid_argument: Unknown arguments are present.
void check_for_unknown_arguments(
    const std::vector<const char *> &known_arguments,
    const std::vector<const char *> &known_but_deprecated_arguments,
    const char *for_mode,
    int argc,
    const char **argv);

/// Returns a floating point value that can be modified using command line arguments.
///
/// If default_value is smaller than min_value or larger than max_value, the argument is required.
///
/// If the specified value is invalid, the program exits with a non-zero return code and prints
/// a message describing the problem.
///
/// Args:
///     name: The name of the float flag.
///     default_value: The value to use if the flag is not specified. If this value is less than
///         min_value or larger than max_value, the flag is required.
///     min_value: Values less than this are rejected.
///     max_value: Values more than this are rejected.
///     argc: Number of command line arguments.
///     argv: Array of command line argument strings.
///
/// Returns:
///     The floating point value.
///
/// Raises:
///     std::invalid_argument:
///         The command line flag was specified but failed to parse into a float in range.
///     std::invalid_argument:
///         The command line flag was not specified and default_value was not in range.
float find_float_argument(
    const char *name, float default_value, float min_value, float max_value, int argc, const char **argv);

/// Returns an integer value that can be modified using command line arguments.
///
/// If default_value is smaller than min_value or larger than max_value, the argument is required.
///
/// If the specified value is invalid, the program exits with a non-zero return code and prints
/// a message describing the problem.
///
/// Args:
///     name: The name of the int flag.
///     default_value: The value to use if the flag is not specified. If this value is less than
///         min_value or larger than max_value, the flag is required.
///     min_value: Values less than this are rejected.
///     max_value: Values more than this are rejected.
///     argc: Number of command line arguments.
///     argv: Array of command line argument strings.
///
/// Returns:
///     The integer value.
///
/// Raises:
///     std::invalid_argument:
///         The command line flag was specified but failed to parse into an int in range.
///     std::invalid_argument:
///         The command line flag was not specified and default_value was not in range.
int64_t find_int64_argument(
    const char *name, int64_t default_value, int64_t min_value, int64_t max_value, int argc, const char **argv);

///
/// Returns a boolean value that can be enabled using a command line argument.
///
/// If the specified value is invalid, the program exits with a non-zero return code and prints
/// a message describing the problem.
///
/// Args:
///     name: The name of the boolean flag.
///     argc: Number of command line arguments.
///     argv: Array of command line argument strings.
///
/// Returns:
///     The boolean value.
///
bool find_bool_argument(const char *name, int argc, const char **argv);

/// Returns the index of an argument value within an enumerated list of allowed values.
///
/// Args:
///     name: The name of the enumerated flag.
///     default_key: The default value of the flag. Set to a key that's not in the map to make the
///         flag required.
///     known_values: A map from allowed keys to returned values.
///     argc: Number of command line arguments.
///     argv: Array of command line argument strings.
///
/// Returns:
///     The chosen value.
///
/// Raises:
///     std::invalid_argument:
///         The command line flag is specified but its key is not in the map.
///     std::invalid_argument:
///         The command line flag is not specified and the default key is not in the map.
template <typename T>
const T &find_enum_argument(
    const char *name, const char *default_key, const std::map<std::string, T> &values, int argc, const char **argv) {
    const char *text = find_argument(name, argc, argv);
    if (text == nullptr) {
        if (default_key == nullptr) {
            std::stringstream msg;
            msg << "\033[31mMust specify a value for enum flag '" << name << "'.\n";
            throw std::invalid_argument(msg.str());
        }
        return values.at(default_key);
    }
    if (values.find(text) == values.end()) {
        std::stringstream msg;
        msg << "\033[31mUnrecognized value '" << text << "' for enum flag '" << name << "'.\n";
        msg << "Recognized values are:\n";
        for (const auto &kv : values) {
            msg << "    '" << kv.first << "'";
            if (default_key != nullptr && kv.first == default_key) {
                msg << " (default)";
            }
            msg << "\n";
        }
        msg << "\033[0m";
        throw std::invalid_argument(msg.str());
    }
    return values.at(text);
}

/// Returns an opened file from a command line argument.
///
/// Args:
///     name: The name of the file flag that will specify the file path.
///     default_file: The file pointer to return if the flag isn't specified (e.g. stdin). Set to
///         nullptr to make the argument required.
///     mode: The mode to open the file path in.
///     argc: Number of command line arguments.
///     argv: Array of command line argument strings.
///
/// Returns:
///     The default file pointer or the opened file.
///
/// Raises:
///     std::invalid_argument:
///         Failed to open the filepath.
///     std::invalid_argument:
///         No argument specified and default file is nullptr.
FILE *find_open_file_argument(const char *name, FILE *default_file, const char *mode, int argc, const char **argv);

/// Exposes an owned ostream or else falls back to std::cout.
struct ostream_else_cout {
   private:
    std::unique_ptr<std::ostream> held;

   public:
    ostream_else_cout(std::unique_ptr<std::ostream> &&held);
    std::ostream &stream();
};

/// Returns an opened ostream from a command line argument.
///
/// Args:
///     name: The name of the command line flag that will specify the file path.
///     default_std_out: If true, defaults to stdout when the command line argument isn't given. Otherwise exits with
///         failure when the command line argumen tisn't given.
///     argc: Number of command line arguments.
///     argv: Array of command line argument strings.
///
/// Returns:
///     The default file pointer or the opened file.
///
/// Raises:
///     std::invalid_argument:
///         Failed to open the filepath.
///     std::invalid_argument:
///         Command line argument isn't present and default_std_out is false.
ostream_else_cout find_output_stream_argument(const char *name, bool default_std_out, int argc, const char **argv);

}  // namespace stim

#endif
