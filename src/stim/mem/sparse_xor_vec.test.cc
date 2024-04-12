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

#include "stim/mem/sparse_xor_vec.h"

#include <algorithm>

#include "gtest/gtest.h"

#include "stim/mem/simd_util.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

TEST(sparse_xor_table, inplace_xor) {
    SparseXorVec<uint32_t> v1;
    SparseXorVec<uint32_t> v2;
    v1.xor_item(1);
    v1.xor_item(3);
    v2.xor_item(3);
    v2.xor_item(2);
    ASSERT_EQ(v1.sorted_items, (std::vector<uint32_t>{1, 3}));
    ASSERT_EQ(v2.sorted_items, (std::vector<uint32_t>{2, 3}));
    v1 ^= v2;
    ASSERT_EQ(v1.sorted_items, (std::vector<uint32_t>{1, 2}));
    ASSERT_EQ(v2.sorted_items, (std::vector<uint32_t>{2, 3}));
}

TEST(sparse_xor_table, grow) {
    SparseXorVec<uint32_t> v1;
    SparseXorVec<uint32_t> v2;
    v1.xor_item(1);
    v1.xor_item(3);
    v1.xor_item(6);
    v2.xor_item(2);
    v2.xor_item(3);
    v2.xor_item(4);
    v2.xor_item(5);
    v1 ^= v2;
    ASSERT_EQ(v1.sorted_items, (std::vector<uint32_t>{1, 2, 4, 5, 6}));
}

TEST(sparse_xor_table, historical_failure_case) {
    SparseXorVec<uint32_t> v1;
    SparseXorVec<uint32_t> v2;
    v1.xor_item(1);
    v1.xor_item(2);
    v1.xor_item(3);
    v1.xor_item(6);
    v1.xor_item(9);
    v2.xor_item(2);
    v1.xor_item(2);
    ASSERT_EQ(v1.sorted_items, (std::vector<uint32_t>{1, 3, 6, 9}));
    ASSERT_EQ(v2.sorted_items, (std::vector<uint32_t>{2}));
}

TEST(sparse_xor_table, comparison) {
    SparseXorVec<uint32_t> v1;
    v1.xor_item(1);
    v1.xor_item(3);
    SparseXorVec<uint32_t> v2;
    v2.xor_item(1);
    ASSERT_TRUE(v1 != v2);
    ASSERT_TRUE(!(v1 == v2));
    ASSERT_TRUE(v2 < v1);
    ASSERT_TRUE(!(v1 < v2));
    v2.xor_item(4);
    ASSERT_TRUE(v1 != v2);
    ASSERT_TRUE(!(v1 == v2));
    ASSERT_TRUE(!(v2 < v1));
    ASSERT_TRUE(v1 < v2);
    v2.xor_item(4);
    v2.xor_item(3);
    ASSERT_TRUE(v1 == v2);
    ASSERT_TRUE(!(v1 != v2));
    ASSERT_TRUE(!(v2 < v1));
    ASSERT_TRUE(!(v1 < v2));
}

TEST(sparse_xor_table, str) {
    SparseXorVec<uint32_t> v;
    ASSERT_EQ(v.str(), "SparseXorVec{}");
    v.xor_item(5);
    ASSERT_EQ(v.str(), "SparseXorVec{5}");
    v.xor_item(2);
    ASSERT_EQ(v.str(), "SparseXorVec{2, 5}");
    v.xor_item(5000);
    ASSERT_EQ(v.str(), "SparseXorVec{2, 5, 5000}");
}

TEST(sparse_xor_table, empty) {
    SparseXorVec<uint32_t> v1;
    ASSERT_TRUE(v1.empty());
    v1.xor_item(1);
    ASSERT_FALSE(v1.empty());
    v1.xor_item(3);
    ASSERT_FALSE(v1.empty());
}

TEST(sparse_xor_vec, is_subset_of_sorted) {
    ASSERT_TRUE(is_subset_of_sorted(
        (SpanRef<const uint32_t>)std::vector<uint32_t>{}, (SpanRef<const uint32_t>)std::vector<uint32_t>{}));
    ASSERT_TRUE(is_subset_of_sorted(
        (SpanRef<const uint32_t>)std::vector<uint32_t>{}, (SpanRef<const uint32_t>)std::vector<uint32_t>{0}));
    ASSERT_FALSE(is_subset_of_sorted(
        (SpanRef<const uint32_t>)std::vector<uint32_t>{0}, (SpanRef<const uint32_t>)std::vector<uint32_t>{}));
    ASSERT_TRUE(is_subset_of_sorted(
        (SpanRef<const uint32_t>)std::vector<uint32_t>{0}, (SpanRef<const uint32_t>)std::vector<uint32_t>{0}));
    ASSERT_FALSE(is_subset_of_sorted(
        (SpanRef<const uint32_t>)std::vector<uint32_t>{1}, (SpanRef<const uint32_t>)std::vector<uint32_t>{0}));
    ASSERT_FALSE(is_subset_of_sorted(
        (SpanRef<const uint32_t>)std::vector<uint32_t>{0}, (SpanRef<const uint32_t>)std::vector<uint32_t>{1}));
    ASSERT_TRUE(is_subset_of_sorted(
        (SpanRef<const uint32_t>)std::vector<uint32_t>{0},
        (SpanRef<const uint32_t>)std::vector<uint32_t>{0, 1, 5, 6, 8}));
    ASSERT_TRUE(is_subset_of_sorted(
        (SpanRef<const uint32_t>)std::vector<uint32_t>{1},
        (SpanRef<const uint32_t>)std::vector<uint32_t>{0, 1, 5, 6, 8}));
    ASSERT_FALSE(is_subset_of_sorted(
        (SpanRef<const uint32_t>)std::vector<uint32_t>{2},
        (SpanRef<const uint32_t>)std::vector<uint32_t>{0, 1, 5, 6, 8}));
    ASSERT_TRUE(is_subset_of_sorted(
        (SpanRef<const uint32_t>)std::vector<uint32_t>{5},
        (SpanRef<const uint32_t>)std::vector<uint32_t>{0, 1, 5, 6, 8}));
    ASSERT_TRUE(is_subset_of_sorted(
        (SpanRef<const uint32_t>)std::vector<uint32_t>{6},
        (SpanRef<const uint32_t>)std::vector<uint32_t>{0, 1, 5, 6, 8}));
    ASSERT_FALSE(is_subset_of_sorted(
        (SpanRef<const uint32_t>)std::vector<uint32_t>{7},
        (SpanRef<const uint32_t>)std::vector<uint32_t>{0, 1, 5, 6, 8}));
    ASSERT_TRUE(is_subset_of_sorted(
        (SpanRef<const uint32_t>)std::vector<uint32_t>{8},
        (SpanRef<const uint32_t>)std::vector<uint32_t>{0, 1, 5, 6, 8}));
    ASSERT_FALSE(is_subset_of_sorted(
        (SpanRef<const uint32_t>)std::vector<uint32_t>{9},
        (SpanRef<const uint32_t>)std::vector<uint32_t>{0, 1, 5, 6, 8}));
    ASSERT_TRUE(is_subset_of_sorted(
        (SpanRef<const uint32_t>)std::vector<uint32_t>{0, 5},
        (SpanRef<const uint32_t>)std::vector<uint32_t>{0, 1, 5, 6, 8}));
    ASSERT_TRUE(is_subset_of_sorted(
        (SpanRef<const uint32_t>)std::vector<uint32_t>{0, 1, 6},
        (SpanRef<const uint32_t>)std::vector<uint32_t>{0, 1, 5, 6, 8}));
    ASSERT_FALSE(is_subset_of_sorted(
        (SpanRef<const uint32_t>)std::vector<uint32_t>{0, 2, 6},
        (SpanRef<const uint32_t>)std::vector<uint32_t>{0, 1, 5, 6, 8}));
}

TEST(sparse_xor_vec, is_superset_of) {
    ASSERT_TRUE((SparseXorVec<uint32_t>{{0}}).is_superset_of(SparseXorVec<uint32_t>{{0}}.range()));
    ASSERT_TRUE((SparseXorVec<uint32_t>{{0, 1}}).is_superset_of(SparseXorVec<uint32_t>{{0}}.range()));
    ASSERT_FALSE((SparseXorVec<uint32_t>{{0}}).is_superset_of(SparseXorVec<uint32_t>{{0, 1}}.range()));
}

TEST(sparse_xor_vec, contains) {
    ASSERT_TRUE((SparseXorVec<uint32_t>{{0}}).contains(0));
    ASSERT_TRUE((SparseXorVec<uint32_t>{{2}}).contains(2));
    ASSERT_TRUE((SparseXorVec<uint32_t>{{0, 2}}).contains(0));
    ASSERT_TRUE((SparseXorVec<uint32_t>{{0, 2}}).contains(2));
    ASSERT_FALSE((SparseXorVec<uint32_t>{{0, 2}}).contains(1));
    ASSERT_FALSE((SparseXorVec<uint32_t>{{}}).contains(0));
    ASSERT_FALSE((SparseXorVec<uint32_t>{{1}}).contains(0));
}

TEST(sparse_xor_vec, inplace_xor_sort) {
    auto f = [](std::vector<int> v) -> std::vector<int> {
        SpanRef<int> s = v;
        auto r = inplace_xor_sort(s);
        v.resize(r.size());
        return v;
    };
    ASSERT_EQ(f({}), (std::vector<int>({})));
    ASSERT_EQ(f({5}), (std::vector<int>({5})));
    ASSERT_EQ(f({5, 5}), (std::vector<int>({})));
    ASSERT_EQ(f({5, 5, 5}), (std::vector<int>({5})));
    ASSERT_EQ(f({5, 5, 5, 5}), (std::vector<int>({})));
    ASSERT_EQ(f({5, 4, 5, 5}), (std::vector<int>({4, 5})));
    ASSERT_EQ(f({4, 5, 5, 5}), (std::vector<int>({4, 5})));
    ASSERT_EQ(f({5, 5, 5, 4}), (std::vector<int>({4, 5})));
    ASSERT_EQ(f({4, 5, 5, 4}), (std::vector<int>({})));
    ASSERT_EQ(f({3, 5, 5, 4}), (std::vector<int>({3, 4})));
}
