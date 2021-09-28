/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "stim/io/sparse_shot.h"

#include "gtest/gtest.h"

#include "stim/test_util.test.h"

using namespace stim;

TEST(sparse_shot, equality) {
    ASSERT_TRUE((SparseShot{}) == (SparseShot{}));
    ASSERT_FALSE((SparseShot{}) != (SparseShot{}));
    ASSERT_TRUE((SparseShot{}) != (SparseShot{{2}, 0}));
    ASSERT_FALSE((SparseShot{}) == (SparseShot{{2}, 0}));

    ASSERT_EQ((SparseShot{{1, 2, 3}, 4}), (SparseShot{{1, 2, 3}, 4}));
    ASSERT_NE((SparseShot{{1, 2, 3}, 4}), (SparseShot{{1, 2, 3}, 5}));
    ASSERT_NE((SparseShot{{1, 2, 3}, 4}), (SparseShot{{1, 2, 4}, 4}));
    ASSERT_NE((SparseShot{{1, 2, 3}, 4}), (SparseShot{{1, 2}, 4}));
}

TEST(sparse_shot, str) {
    ASSERT_EQ((SparseShot{{1, 2, 3}, 4}.str()), "SparseShot{{1, 2, 3}, 4}");
}
