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

#include "sparse_xor_vec.h"

#include <algorithm>
#include <gtest/gtest.h>

#include "../test_util.test.h"
#include "simd_util.h"

TEST(sparse_xor_table, inplace_xor) {
    SparseXorVec<uint32_t> v1;
    SparseXorVec<uint32_t> v2;
    v1 ^= 1;
    v1 ^= 3;
    v2 ^= 2;
    v2 ^= 3;
    ASSERT_EQ(v1.vec, (std::vector<uint32_t>{1, 3}));
    ASSERT_EQ(v2.vec, (std::vector<uint32_t>{2, 3}));
    v1 ^= v2;
    ASSERT_EQ(v1.vec, (std::vector<uint32_t>{1, 2}));
    ASSERT_EQ(v2.vec, (std::vector<uint32_t>{2, 3}));
}

TEST(sparse_xor_table, grow) {
    SparseXorVec<uint32_t> v1;
    SparseXorVec<uint32_t> v2;
    v1 ^= 1;
    v1 ^= 3;
    v1 ^= 6;
    v2 ^= 2;
    v2 ^= 3;
    v2 ^= 4;
    v2 ^= 5;
    v1 ^= v2;
    ASSERT_EQ(v1.vec, (std::vector<uint32_t>{1, 2, 4, 5, 6}));
}

TEST(sparse_xor_table, historical_failure_case) {
    SparseXorVec<uint32_t> v1;
    SparseXorVec<uint32_t> v2;
    v1 ^= 1;
    v1 ^= 2;
    v1 ^= 3;
    v1 ^= 6;
    v1 ^= 9;
    v2 ^= 2;
    v1 ^= 2;
    ASSERT_EQ(v1.vec, (std::vector<uint32_t>{1, 3, 6, 9}));
    ASSERT_EQ(v2.vec, (std::vector<uint32_t>{2}));
}

TEST(sparse_xor_table, comparison) {
    SparseXorVec<uint32_t> v1;
    v1 ^= 1;
    v1 ^= 3;
    SparseXorVec<uint32_t> v2;
    v2 ^= 1;
    ASSERT_TRUE(v1 != v2);
    ASSERT_TRUE(!(v1 == v2));
    ASSERT_TRUE(v2 < v1);
    ASSERT_TRUE(!(v1 < v2));
    v2 ^= 4;
    ASSERT_TRUE(v1 != v2);
    ASSERT_TRUE(!(v1 == v2));
    ASSERT_TRUE(!(v2 < v1));
    ASSERT_TRUE(v1 < v2);
    v2 ^= 4;
    v2 ^= 3;
    ASSERT_TRUE(v1 == v2);
    ASSERT_TRUE(!(v1 != v2));
    ASSERT_TRUE(!(v2 < v1));
    ASSERT_TRUE(!(v1 < v2));
}

TEST(sparse_xor_table, str) {
    SparseXorVec<uint32_t> v;
    ASSERT_EQ(v.str(), "SparseXorVec{}");
    v ^= 5;
    ASSERT_EQ(v.str(), "SparseXorVec{5}");
    v ^= 2;
    ASSERT_EQ(v.str(), "SparseXorVec{2, 5}");
    v ^= 5000;
    ASSERT_EQ(v.str(), "SparseXorVec{2, 5, 5000}");
}
