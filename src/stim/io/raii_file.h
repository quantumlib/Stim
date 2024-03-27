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

#ifndef _STIM_IO_RAII_FILE
#define _STIM_IO_RAII_FILE

#include <cstdio>
#include <string_view>

namespace stim {

struct RaiiFile {
    FILE* f;
    bool responsible_for_closing;
    RaiiFile(const char* optional_path, const char* mode);
    RaiiFile(std::string_view optional_path, const char* mode);
    RaiiFile(FILE* claim_ownership);
    RaiiFile(const RaiiFile& other) = delete;
    RaiiFile(RaiiFile&& other) noexcept;
    ~RaiiFile();
    void open(std::string_view optional_path, const char* mode);
    void open(const char* optional_path, const char* mode);
    void done();
};

}  // namespace stim

#endif
