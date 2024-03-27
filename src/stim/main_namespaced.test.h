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

#ifndef _STIM_MAIN_NAMESPACED_TEST_H
#define _STIM_MAIN_NAMESPACED_TEST_H

#include <string>
#include <vector>

namespace stim {

std::string run_captured_stim_main(std::vector<const char *> flags);
std::string run_captured_stim_main(std::vector<const char *> flags, std::string_view std_in_content);
std::string_view trim(std::string_view text);
bool matches(std::string actual, std::string pattern);

}  // namespace stim

#endif
