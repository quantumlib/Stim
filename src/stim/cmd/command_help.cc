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
