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

#include "stim/mem/monotonic_buffer.h"

#include "gtest/gtest.h"

using namespace stim;

TEST(pointer_range, equality) {
    int data[100]{};
    SpanRef<int> r1{&data[0], &data[10]};
    SpanRef<int> r2{&data[10], &data[20]};
    SpanRef<int> r4{&data[30], &data[50]};
    ASSERT_TRUE(r1 == r2);
    ASSERT_FALSE(r1 != r2);
    ASSERT_EQ(r1, r2);
    ASSERT_NE(r1, r4);
    r2[0] = 1;
    ASSERT_EQ(data[10], 1);
    ASSERT_NE(r1, r2);
    ASSERT_TRUE(r1 != r2);
    ASSERT_FALSE(r1 == r2);
    ASSERT_NE(r1, r4);
    r2[0] = 0;
    ASSERT_EQ(r1, r2);
    r2[6] = 1;
    ASSERT_NE(r1, r2);
}

TEST(monotonic_buffer, append_tail) {
    MonotonicBuffer<int> buf;
    for (size_t k = 0; k < 100; k++) {
        buf.append_tail(k);
    }

    SpanRef<int> rng = buf.commit_tail();
    ASSERT_EQ(rng.size(), 100);
    for (size_t k = 0; k < 100; k++) {
        ASSERT_EQ(rng[k], k);
    }
}

TEST(monotonic_buffer, ensure_available) {
    MonotonicBuffer<int> buf;
    buf.append_tail(std::vector<int>{1, 2, 3, 4});
    buf.append_tail(std::vector<int>{5, 6});
    buf.append_tail(std::vector<int>{7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

    SpanRef<const int> rng = buf.commit_tail();
    std::vector<int> expected{1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    SpanRef<const int> v = expected;
    ASSERT_EQ(rng, v);
}
