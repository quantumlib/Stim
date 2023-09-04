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

#include "stim/search/graphlike/search_state.h"

#include "gtest/gtest.h"

#include "stim/search/graphlike/node.h"

using namespace stim;
using namespace stim::impl_search_graphlike;

static simd_bits<64> obs_mask(uint64_t v) {
    simd_bits<64> result(64);
    result.ptr_simd[0] = v;
    return result;
}

TEST(search_graphlike, DemAdjGraphSearchState_construct) {
    SearchState r(64);
    ASSERT_EQ(r.det_active, NO_NODE_INDEX);
    ASSERT_EQ(r.det_held, NO_NODE_INDEX);
    ASSERT_EQ((uint64_t)r.obs_mask.ptr_simd[0], 0);

    SearchState r2(2, 1, obs_mask(3));
    ASSERT_EQ(r2.det_active, 2);
    ASSERT_EQ(r2.det_held, 1);
    ASSERT_EQ((uint64_t)r2.obs_mask.ptr_simd[0], 3);
}

TEST(search_graphlike, DemAdjGraphSearchState_is_undetected) {
    ASSERT_FALSE(SearchState(1, 2, obs_mask(3)).is_undetected());
    ASSERT_FALSE(SearchState(1, 2, obs_mask(2)).is_undetected());
    ASSERT_FALSE(SearchState(1, 2, obs_mask(0)).is_undetected());
    ASSERT_TRUE(SearchState(1, 1, obs_mask(3)).is_undetected());
    ASSERT_TRUE(SearchState(NO_NODE_INDEX, NO_NODE_INDEX, obs_mask(32)).is_undetected());
    ASSERT_TRUE(SearchState(NO_NODE_INDEX, NO_NODE_INDEX, obs_mask(0)).is_undetected());
}

TEST(search_graphlike, DemAdjGraphSearchState_canonical) {
    SearchState a = SearchState(1, 2, obs_mask(3)).canonical();
    ASSERT_EQ(a.det_active, 1);
    ASSERT_EQ(a.det_held, 2);
    ASSERT_EQ(a.obs_mask, obs_mask(3));

    a = SearchState(2, 1, obs_mask(3)).canonical();
    ASSERT_EQ(a.det_active, 1);
    ASSERT_EQ(a.det_held, 2);
    ASSERT_EQ(a.obs_mask, obs_mask(3));

    a = SearchState(1, 1, obs_mask(3)).canonical();
    ASSERT_EQ(a.det_active, NO_NODE_INDEX);
    ASSERT_EQ(a.det_held, NO_NODE_INDEX);
    ASSERT_EQ(a.obs_mask, obs_mask(3));

    a = SearchState(1, 1, obs_mask(1)).canonical();
    ASSERT_EQ(a.det_active, NO_NODE_INDEX);
    ASSERT_EQ(a.det_held, NO_NODE_INDEX);
    ASSERT_EQ(a.obs_mask, obs_mask(1));

    a = SearchState(1, NO_NODE_INDEX, obs_mask(1)).canonical();
    ASSERT_EQ(a.det_active, 1);
    ASSERT_EQ(a.det_held, NO_NODE_INDEX);
    ASSERT_EQ(a.obs_mask, obs_mask(1));
}

TEST(search_graphlike, DemAdjGraphSearchState_append_transition_as_error_instruction_to) {
    DetectorErrorModel out;

    SearchState(1, 2, obs_mask(9)).append_transition_as_error_instruction_to(SearchState(1, 2, obs_mask(16)), out);
    ASSERT_EQ(out, DetectorErrorModel(R"MODEL(
        error(1) L0 L3 L4
    )MODEL"));

    SearchState(1, 2, obs_mask(9)).append_transition_as_error_instruction_to(SearchState(3, 2, obs_mask(9)), out);
    ASSERT_EQ(out, DetectorErrorModel(R"MODEL(
        error(1) L0 L3 L4
        error(1) D1 D3
    )MODEL"));

    SearchState(1, 2, obs_mask(9))
        .append_transition_as_error_instruction_to(SearchState(1, NO_NODE_INDEX, obs_mask(9)), out);
    ASSERT_EQ(out, DetectorErrorModel(R"MODEL(
        error(1) L0 L3 L4
        error(1) D1 D3
        error(1) D2
    )MODEL"));

    SearchState(NO_NODE_INDEX, NO_NODE_INDEX, obs_mask(0))
        .append_transition_as_error_instruction_to(SearchState(1, NO_NODE_INDEX, obs_mask(9)), out);
    ASSERT_EQ(out, DetectorErrorModel(R"MODEL(
        error(1) L0 L3 L4
        error(1) D1 D3
        error(1) D2
        error(1) D1 L0 L3
    )MODEL"));

    SearchState(1, 1, obs_mask(0)).append_transition_as_error_instruction_to(SearchState(2, 2, obs_mask(4)), out);
    ASSERT_EQ(out, DetectorErrorModel(R"MODEL(
        error(1) L0 L3 L4
        error(1) D1 D3
        error(1) D2
        error(1) D1 L0 L3
        error(1) L2
    )MODEL"));
}

TEST(search_graphlike, DemAdjGraphSearchState_canonical_equality) {
    SearchState v1{1, 2, obs_mask(3)};
    SearchState v2{1, 4, obs_mask(3)};
    ASSERT_TRUE(v1 == v1);
    ASSERT_FALSE(v1 == v2);
    ASSERT_FALSE(v1 != v1);
    ASSERT_TRUE(v1 != v2);

    ASSERT_EQ(v1, SearchState(2, 1, obs_mask(3)));
    ASSERT_NE(v1, SearchState(1, NO_NODE_INDEX, obs_mask(3)));
    ASSERT_EQ(SearchState(NO_NODE_INDEX, NO_NODE_INDEX, obs_mask(0)), SearchState(1, 1, obs_mask(0)));
    ASSERT_EQ(SearchState(3, 3, obs_mask(0)), SearchState(1, 1, obs_mask(0)));
    ASSERT_NE(SearchState(3, 3, obs_mask(1)), SearchState(1, 1, obs_mask(0)));
    ASSERT_EQ(SearchState(3, 3, obs_mask(1)), SearchState(1, 1, obs_mask(1)));
    ASSERT_EQ(SearchState(2, NO_NODE_INDEX, obs_mask(3)), SearchState(NO_NODE_INDEX, 2, obs_mask(3)));
}

TEST(search_graphlike, DemAdjGraphSearchState_canonical_ordering) {
    ASSERT_TRUE(SearchState(1, 999, obs_mask(999)) < SearchState(101, 102, obs_mask(103)));
    ASSERT_TRUE(SearchState(999, 1, obs_mask(999)) < SearchState(101, 102, obs_mask(103)));
    ASSERT_TRUE(SearchState(101, 1, obs_mask(999)) < SearchState(101, 102, obs_mask(103)));
    ASSERT_TRUE(SearchState(102, 1, obs_mask(999)) < SearchState(101, 102, obs_mask(103)));
    ASSERT_TRUE(SearchState(101, 102, obs_mask(3)) < SearchState(101, 102, obs_mask(103)));

    ASSERT_FALSE(SearchState(101, 102, obs_mask(103)) < SearchState(101, 102, obs_mask(103)));
    ASSERT_FALSE(SearchState(101, 104, obs_mask(103)) < SearchState(101, 102, obs_mask(103)));
    ASSERT_FALSE(SearchState(101, 102, obs_mask(104)) < SearchState(101, 102, obs_mask(103)));
}

TEST(search_graphlike, DemAdjGraphSearchState_str) {
    ASSERT_EQ(SearchState(1, 2, obs_mask(3)).str(), "D1 D2 L0 L1 ");
}
