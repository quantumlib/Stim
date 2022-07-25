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

#include "stim/io/raii_file.h"

#include <sstream>

using namespace stim;

RaiiFile::RaiiFile(FILE *claim_ownership) : f(claim_ownership), responsible_for_closing(true) {
}

RaiiFile::RaiiFile(RaiiFile &&other) noexcept : f(other.f), responsible_for_closing(other.responsible_for_closing) {
    other.responsible_for_closing = false;
    other.f = nullptr;
}

RaiiFile::RaiiFile(const char *path, const char *mode) : f(nullptr), responsible_for_closing(true) {
    if (path == nullptr) {
        f = nullptr;
        return;
    }

    f = fopen(path, mode);
    if (f == nullptr) {
        std::stringstream ss;
        ss << "Failed to open '";
        ss << path;
        ss << "' for ";
        if (*mode == 'r') {
            ss << "reading.";
        } else {
            ss << "writing.";
        }
        throw std::invalid_argument(ss.str());
    }
}

RaiiFile::~RaiiFile() {
    done();
}

void RaiiFile::done() {
    if (f != nullptr && responsible_for_closing) {
        fclose(f);
        f = nullptr;
        responsible_for_closing = false;
    }
}
