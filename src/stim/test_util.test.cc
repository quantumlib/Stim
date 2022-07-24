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

#include "stim/test_util.test.h"

#include "stim/probability_util.h"

using namespace stim;

static bool shared_test_rng_initialized;
static std::mt19937_64 shared_test_rng;

std::mt19937_64 &SHARED_TEST_RNG() {
    if (!shared_test_rng_initialized) {
        shared_test_rng = externally_seeded_rng();
        shared_test_rng_initialized = true;
    }
    return shared_test_rng;
}

std::string rewind_read_close(FILE *f) {
    rewind(f);
    std::string result;
    while (true) {
        int c = getc(f);
        if (c == EOF) {
            fclose(f);
            return result;
        }
        result.push_back((char)c);
    }
}

RaiiTempNamedFile::RaiiTempNamedFile() {
    char tmp_stdin_filename[] = "/tmp/stim_test_named_file_XXXXXX";
    descriptor = mkstemp(tmp_stdin_filename);
    if (descriptor == -1) {
        throw std::runtime_error("Failed to create temporary file.");
    }
    path = tmp_stdin_filename;
}

RaiiTempNamedFile::~RaiiTempNamedFile() {
    if (!path.empty()) {
        remove(path.data());
        path = "";
    }
}

std::string RaiiTempNamedFile::read_contents() {
    FILE *f = fopen(path.c_str(), "r");
    if (f == nullptr) {
        throw std::runtime_error("Failed to open temp named file " + path);
    }
    std::string result;
    while (true) {
        int c = getc(f);
        if (c == EOF) {
            break;
        }
        result.push_back(c);
    }
    fclose(f);
    return result;
}

void RaiiTempNamedFile::write_contents(const std::string &contents) {
    FILE *f = fopen(path.c_str(), "w");
    if (f == nullptr) {
        throw std::runtime_error("Failed to open temp named file " + path);
    }
    for (char c : contents) {
        putc(c, f);
    }
    fclose(f);
}
