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

#ifndef _STIM_TEST_UTIL_TEST_H
#define _STIM_TEST_UTIL_TEST_H

#include <random>

#include "gtest/gtest.h"

std::mt19937_64 INDEPENDENT_TEST_RNG();

std::string rewind_read_close(FILE *f);

std::string resolve_test_file(std::string_view name);
void expect_string_is_identical_to_saved_file(std::string_view actual, std::string_view key);

struct RaiiTempNamedFile {
    int descriptor;
    std::string path;
    RaiiTempNamedFile();
    ~RaiiTempNamedFile();
    RaiiTempNamedFile(std::string_view contents);
    std::string read_contents();
    void write_contents(std::string_view contents);
};

#endif
