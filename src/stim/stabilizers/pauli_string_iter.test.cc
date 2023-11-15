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

#include "stim/stabilizers/pauli_string_iter.h"

#include "gtest/gtest.h"

#include "stim/mem/simd_word.test.h"

using namespace stim;

std::vector<uint64_t> loop_state(const NestedLooper &looper) {
    std::vector<uint64_t> state;
    for (const auto &e : looper.loops) {
        state.push_back(e.cur);
    }
    return state;
}

std::vector<std::vector<uint64_t>> loop_drain(NestedLooper &looper) {
    std::vector<std::vector<uint64_t>> state;
    looper.start();
    while (looper.iter_next([](size_t k) {
    })) {
        state.push_back(loop_state(looper));
    }
    return state;
}

TEST(pauli_string_iter, NestedLooper_simple) {
    NestedLooper looper;
    looper.loops.push_back(NestedLooperLoop{0, 3});
    looper.loops.push_back(NestedLooperLoop{2, 6});

    ASSERT_EQ(
        loop_drain(looper),
        (std::vector<std::vector<uint64_t>>{
            {0, 2},
            {0, 3},
            {0, 4},
            {0, 5},
            {1, 2},
            {1, 3},
            {1, 4},
            {1, 5},
            {2, 2},
            {2, 3},
            {2, 4},
            {2, 5},
        }));
}

TEST(pauli_string_iter, NestedLooper_shifted) {
    NestedLooper looper;
    looper.loops.push_back(NestedLooperLoop{0, 3});
    looper.loops.push_back(NestedLooperLoop{2, 6, 0});
    ASSERT_EQ(
        loop_drain(looper),
        (std::vector<std::vector<uint64_t>>{
            {0, 2},
            {0, 3},
            {0, 4},
            {0, 5},
            {1, 3},
            {1, 4},
            {1, 5},
            {2, 4},
            {2, 5},
        }));
}

TEST(pauli_string_iter, NestedLooper_append_combination_loops) {
    NestedLooper looper;
    looper.append_combination_loops(6, 3);
    ASSERT_EQ(
        loop_drain(looper),
        (std::vector<std::vector<uint64_t>>{
            {0, 1, 2}, {0, 1, 3}, {0, 1, 4}, {0, 1, 5}, {0, 2, 3}, {0, 2, 4}, {0, 2, 5},
            {0, 3, 4}, {0, 3, 5}, {0, 4, 5}, {1, 2, 3}, {1, 2, 4}, {1, 2, 5}, {1, 3, 4},
            {1, 3, 5}, {1, 4, 5}, {2, 3, 4}, {2, 3, 5}, {2, 4, 5}, {3, 4, 5},
        }));

    looper.loops.clear();
    looper.append_combination_loops(10, 9);
    ASSERT_EQ(
        loop_drain(looper),
        (std::vector<std::vector<uint64_t>>{
            {0, 1, 2, 3, 4, 5, 6, 7, 8},
            {0, 1, 2, 3, 4, 5, 6, 7, 9},
            {0, 1, 2, 3, 4, 5, 6, 8, 9},
            {0, 1, 2, 3, 4, 5, 7, 8, 9},
            {0, 1, 2, 3, 4, 6, 7, 8, 9},
            {0, 1, 2, 3, 5, 6, 7, 8, 9},
            {0, 1, 2, 4, 5, 6, 7, 8, 9},
            {0, 1, 3, 4, 5, 6, 7, 8, 9},
            {0, 2, 3, 4, 5, 6, 7, 8, 9},
            {1, 2, 3, 4, 5, 6, 7, 8, 9},
        }));
}

TEST(pauli_string_iter, NestedLooper_inplace_edit) {
    NestedLooper looper;
    looper.loops.push_back(NestedLooperLoop{1, 3});

    std::vector<std::vector<uint64_t>> state;
    looper.start();
    while (looper.iter_next([&](size_t k) {
        if (k == 0 && looper.loops[0].cur == 2) {
            looper.loops.push_back(NestedLooperLoop{2, 4});
        }
    })) {
        state.push_back(loop_state(looper));
    }

    ASSERT_EQ(
        state,
        (std::vector<std::vector<uint64_t>>{
            {1},
            {2, 2},
            {2, 3},
        }));
}

template <size_t W>
std::vector<std::string> record_pauli_string(PauliStringIterator<W> iter) {
    std::vector<std::string> results;
    while (iter.iter_next()) {
        results.push_back(iter.result.str());
    }
    return results;
}

TEST_EACH_WORD_SIZE_W(pauli_string_iter, small_cases, {
    // Empty.
    ASSERT_EQ(
        record_pauli_string(PauliStringIterator<W>(3, 0, 0, true, true, true)),
        (std::vector<std::string>{
            "+___",
        }));

    // Empty or single.
    ASSERT_EQ(
        record_pauli_string(PauliStringIterator<W>(2, 0, 1, true, true, true)),
        (std::vector<std::string>{
            "+__",
            "+X_",
            "+Y_",
            "+Z_",
            "+_X",
            "+_Y",
            "+_Z",
        }));

    // Single.
    ASSERT_EQ(
        record_pauli_string(PauliStringIterator<W>(3, 1, 1, true, true, true)),
        (std::vector<std::string>{
            "+X__",
            "+Y__",
            "+Z__",
            "+_X_",
            "+_Y_",
            "+_Z_",
            "+__X",
            "+__Y",
            "+__Z",
        }));

    // Full doubles.
    ASSERT_EQ(
        record_pauli_string(PauliStringIterator<W>(2, 2, 2, true, true, true)),
        (std::vector<std::string>{
            "+XX",
            "+XY",
            "+XZ",
            "+YX",
            "+YY",
            "+YZ",
            "+ZX",
            "+ZY",
            "+ZZ",
        }));

    // All length 2.
    ASSERT_EQ(
        record_pauli_string(PauliStringIterator<W>(2, 0, 2, true, true, true)),
        (std::vector<std::string>{
            "+__",
            "+X_",
            "+Y_",
            "+Z_",
            "+_X",
            "+_Y",
            "+_Z",
            "+XX",
            "+XY",
            "+XZ",
            "+YX",
            "+YY",
            "+YZ",
            "+ZX",
            "+ZY",
            "+ZZ",
        }));

    // XY subset.
    ASSERT_EQ(
        record_pauli_string(PauliStringIterator<W>(2, 0, 2, false, true, true)),
        (std::vector<std::string>{
            "+__",
            "+Y_",
            "+Z_",
            "+_Y",
            "+_Z",
            "+YY",
            "+YZ",
            "+ZY",
            "+ZZ",
        }));
    // XZ subset.
    ASSERT_EQ(
        record_pauli_string(PauliStringIterator<W>(2, 0, 2, true, false, true)),
        (std::vector<std::string>{
            "+__",
            "+X_",
            "+Z_",
            "+_X",
            "+_Z",
            "+XX",
            "+XZ",
            "+ZX",
            "+ZZ",
        }));
    // YZ subset.
    ASSERT_EQ(
        record_pauli_string(PauliStringIterator<W>(2, 0, 2, true, true, false)),
        (std::vector<std::string>{
            "+__",
            "+X_",
            "+Y_",
            "+_X",
            "+_Y",
            "+XX",
            "+XY",
            "+YX",
            "+YY",
        }));

    // X subset.
    ASSERT_EQ(
        record_pauli_string(PauliStringIterator<W>(2, 0, 2, true, false, false)),
        (std::vector<std::string>{
            "+__",
            "+X_",
            "+_X",
            "+XX",
        }));
    // Y subset.
    ASSERT_EQ(
        record_pauli_string(PauliStringIterator<W>(2, 0, 2, false, true, false)),
        (std::vector<std::string>{
            "+__",
            "+Y_",
            "+_Y",
            "+YY",
        }));
    // Z subset.
    ASSERT_EQ(
        record_pauli_string(PauliStringIterator<W>(2, 0, 2, false, false, true)),
        (std::vector<std::string>{
            "+__",
            "+Z_",
            "+_Z",
            "+ZZ",
        }));

    // No pauli subset.
    ASSERT_EQ(
        record_pauli_string(PauliStringIterator<W>(2, 0, 2, false, false, false)),
        (std::vector<std::string>{
            "+__",
        }));
    ASSERT_EQ(record_pauli_string(PauliStringIterator<W>(2, 1, 2, false, false, false)), (std::vector<std::string>{}));
    ASSERT_EQ(record_pauli_string(PauliStringIterator<W>(2, 3, 6, false, false, false)), (std::vector<std::string>{}));
    ASSERT_EQ(record_pauli_string(PauliStringIterator<W>(2, 2, 1, false, false, false)), (std::vector<std::string>{}));
})
