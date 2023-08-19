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

#include <fstream>

#include "stim/probability_util.h"

using namespace stim;

static bool shared_test_rng_initialized;
static std::mt19937_64 shared_test_rng;

std::string resolve_test_file(const std::string &name) {
    std::vector<std::string> prefixes{
        "testdata/",
        "../testdata/",
    };
    for (const auto &prefix : prefixes) {
        std::string full_path = prefix + name;
        FILE *f = fopen(full_path.c_str(), "rb");
        if (f != nullptr) {
            fclose(f);
            return full_path;
        }
    }
    for (const auto &prefix : prefixes) {
        std::string full_path = prefix + name;
        FILE *f = fopen(full_path.c_str(), "wb");
        if (f != nullptr) {
            fclose(f);
            return full_path;
        }
    }
    throw std::invalid_argument("Run tests from the repo root so they can find the testdata/ directory.");
}

void expect_string_is_identical_to_saved_file(const std::string &actual, const std::string &key) {
    auto path = resolve_test_file(key);
    FILE *f = fopen(path.c_str(), "rb");
    auto expected = rewind_read_close(f);

    if (expected != actual) {
        auto dot = key.rfind('.');
        std::string new_path;
        if (dot == std::string::npos) {
            new_path = path + ".new";
        } else {
            dot += path.size() - key.size();
            new_path = path.substr(0, dot) + ".new" + path.substr(dot);
        }
        std::ofstream out;
        out.open(new_path);
        out << actual;
        out.close();
        EXPECT_TRUE(false) << "Diagram didn't agree.\n"
                           << "    key=" << key << "\n"
                           << "    expected: file://" << path << "\n"
                           << "    actual: file://" << new_path << "\n";
    }
}

std::mt19937_64 INDEPENDENT_TEST_RNG() {
    if (!shared_test_rng_initialized) {
        shared_test_rng = externally_seeded_rng();
        shared_test_rng_initialized = true;
    }
    std::seed_seq seq{shared_test_rng(), shared_test_rng(), shared_test_rng(), shared_test_rng()};
    return std::mt19937_64(seq);
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

static void init_path(RaiiTempNamedFile &self) {
    char tmp_stdin_filename[] = "/tmp/stim_test_named_file_XXXXXX";
    self.descriptor = mkstemp(tmp_stdin_filename);
    if (self.descriptor == -1) {
        throw std::runtime_error("Failed to create temporary file.");
    }
    self.path = std::string(tmp_stdin_filename);
}

RaiiTempNamedFile::RaiiTempNamedFile() {
    init_path(*this);
}

RaiiTempNamedFile::RaiiTempNamedFile(const std::string &contents) {
    init_path(*this);
    write_contents(contents);
}

RaiiTempNamedFile::~RaiiTempNamedFile() {
    if (!path.empty()) {
        remove(path.data());
        path = "";
    }
}

std::string RaiiTempNamedFile::read_contents() {
    FILE *f = fopen(path.c_str(), "rb");
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
    FILE *f = fopen(path.c_str(), "wb");
    if (f == nullptr) {
        throw std::runtime_error("Failed to open temp named file " + path);
    }
    for (char c : contents) {
        putc(c, f);
    }
    fclose(f);
}
