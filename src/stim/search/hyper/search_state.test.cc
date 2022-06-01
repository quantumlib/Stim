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

#include "stim/search/hyper/search_state.h"

#include "gtest/gtest.h"

#include "stim/search/hyper/node.h"

using namespace stim;
using namespace stim::impl_search_hyper;

TEST(search_hyper_search_state, append_transition_as_error_instruction_to) {
    DetectorErrorModel out;

    SearchState{{{1, 2}}, 9}.append_transition_as_error_instruction_to(SearchState{{{1, 2}}, 16}, out);
    ASSERT_EQ(out, DetectorErrorModel(R"MODEL(
        error(1) L0 L3 L4
    )MODEL"));

    SearchState{{{}}, 9}.append_transition_as_error_instruction_to(SearchState{{{1, 2, 4}}, 16}, out);
    ASSERT_EQ(out, DetectorErrorModel(R"MODEL(
        error(1) L0 L3 L4
        error(1) D1 D2 D4 L0 L3 L4
    )MODEL"));

    SearchState{{{1, 2}}, 9}.append_transition_as_error_instruction_to(SearchState{{{2, 3}}, 9}, out);
    ASSERT_EQ(out, DetectorErrorModel(R"MODEL(
        error(1) L0 L3 L4
        error(1) D1 D2 D4 L0 L3 L4
        error(1) D1 D3
    )MODEL"));
}

TEST(search_hyper_search_state, equality) {
    SearchState v1{{{1, 2}}, 3};
    SearchState v2{{{1, 4}}, 3};
    ASSERT_TRUE(v1 == v1);
    ASSERT_FALSE(v1 == v2);
    ASSERT_FALSE(v1 != v1);
    ASSERT_TRUE(v1 != v2);

    ASSERT_NE(v1, (SearchState{{{1}}, 3}));
    ASSERT_NE(v1, (SearchState{{{1, 2}}, 4}));
}

TEST(search_hyper_search_state, ordering) {
    ASSERT_TRUE((SearchState{{{1}}, 999}) < (SearchState{{{1, 2}}, 999}));
    ASSERT_TRUE((SearchState{{{1, 999}}, 999}) < (SearchState{{{101, 102}}, 103}));
    ASSERT_TRUE((SearchState{{{1, 999}}, 999}) < (SearchState{{{101, 102}}, 103}));
    ASSERT_TRUE((SearchState{{{1, 101}}, 999}) < (SearchState{{{101, 102}}, 103}));
    ASSERT_TRUE((SearchState{{{1, 102}}, 999}) < (SearchState{{{101, 102}}, 103}));
    ASSERT_TRUE((SearchState{{{101, 102}}, 3}) < (SearchState{{{101, 102}}, 103}));

    ASSERT_FALSE((SearchState{{{101, 102}}, 103}) < (SearchState{{{101, 102}}, 103}));
    ASSERT_FALSE((SearchState{{{101, 104}}, 103}) < (SearchState{{{101, 102}}, 103}));
    ASSERT_FALSE((SearchState{{{101, 102}}, 104}) < (SearchState{{{101, 102}}, 103}));
}

TEST(search_hyper_search_state, str) {
    ASSERT_EQ((SearchState{{{1, 2}}, 3}.str()), "D1 D2 L0 L1 ");
}
