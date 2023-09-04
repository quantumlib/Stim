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

#include <bitset>

#include "gtest/gtest.h"

#include "stim/mem/simd_word.test.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(pauli_string_iter, iter_pauli_string, {
    size_t num_qubits = 4;
    size_t min_weight = 0;
    size_t max_weight = 2;
    PauliStringIterator<W> iter(num_qubits, min_weight, max_weight);
    iter.iter_next();
    ASSERT_EQ(iter.result, PauliString<W>::from_str("+____"));
    iter.iter_next();
    ASSERT_EQ(iter.result, PauliString<W>::from_str("+X___"));
    iter.iter_next();
    ASSERT_EQ(iter.result, PauliString<W>::from_str("+Z___"));
    iter.iter_next();
    ASSERT_EQ(iter.result, PauliString<W>::from_str("+Y___"));
    for (size_t i = 0; i < 3 * num_qubits - 3; i++) {
        iter.iter_next();
    }
    ASSERT_EQ(iter.result, PauliString<W>::from_str("+___Y"));  //
    num_qubits = 6;
    min_weight = 4;
    max_weight = 6;
    PauliStringIterator<W> iter2(num_qubits, min_weight, max_weight);
    iter2.iter_next();
    ASSERT_EQ(iter2.result, PauliString<W>::from_str("+XXXX__"));
    iter2.iter_next();
    ASSERT_EQ(iter2.result, PauliString<W>::from_str("+ZXXX__"));
    iter2.iter_next();
    ASSERT_EQ(iter2.result, PauliString<W>::from_str("+YXXX__"));
    iter2.iter_next();
    ASSERT_EQ(iter2.result, PauliString<W>::from_str("+XZXX__"));
    iter2.iter_next();
    ASSERT_EQ(iter2.result, PauliString<W>::from_str("+ZZXX__"));
    iter2.iter_next();
    ASSERT_EQ(iter2.result, PauliString<W>::from_str("+YZXX__"));
    iter2.iter_next();
    ASSERT_EQ(iter2.result, PauliString<W>::from_str("+XYXX__"));
    for (size_t i = 0; i < 81 - 7; i++) {
        iter2.iter_next();
    }
    ASSERT_EQ(iter2.result, PauliString<W>::from_str("+YYYY__"));
    // Skip ahead to final combination (there are 6 C 4 = 15 ways to put the 3^4
    // Paulis, but we've completed one combination so that leaves us with 14.)
    for (size_t i = 0; i < 14 * 81; i++) {
        iter2.iter_next();
    }
    ASSERT_EQ(iter2.result, PauliString<W>::from_str("+__YYYY"));
    // Should be on weight 5 paulis now
    iter2.iter_next();
    ASSERT_EQ(iter2.result, PauliString<W>::from_str("+XXXXX_"));
    // 243 = 3^5
    for (size_t i = 0; i < 243 - 1; i++) {
        iter2.iter_next();
    }
    ASSERT_EQ(iter2.result, PauliString<W>::from_str("+YYYYY_"));
    // 6 C 5 = 6, -1 for previous loop
    for (size_t i = 0; i < 5 * 243; i++) {
        iter2.iter_next();
    }
    ASSERT_EQ(iter2.result, PauliString<W>::from_str("+_YYYYY"));
    iter2.iter_next();
    // Should be on weight 6 paulis now
    ASSERT_EQ(iter2.result, PauliString<W>::from_str("+XXXXXX"));
    num_qubits = 4;
    min_weight = 3;
    max_weight = 4;
    PauliStringIterator<W> iter3(num_qubits, min_weight, max_weight);
    for (int i = 0; i < 4 * pow(3, 3) + pow(3, 4); i++) {
        iter3.iter_next();
    }
    ASSERT_EQ(iter3.result, PauliString<W>::from_str("+YYYY"));
    ASSERT_EQ(iter3.iter_next(), false);
})

TEST_EACH_WORD_SIZE_W(pauli_string_iter, count_trailing_zeros, {
    PauliStringIterator<W> iter(513, 2, 7);
    iter.cur_perm.u64[0] = (1ULL << 2) - 1;  // 11
    iter.cur_perm.u64[1] = (1ULL << 4) - 1;  // 1111
    iter.cur_perm.u64[3] = (1ULL << 1) - 1;  // 1
    size_t ctz = iter.count_trailing_zeros(iter.cur_perm);
    ASSERT_EQ(ctz, 0);
    iter.cur_perm.u64[0] = ((1ULL << 2) - 1) << 17;
    ctz = iter.count_trailing_zeros(iter.cur_perm);
    ASSERT_EQ(ctz, 17);
    iter.cur_perm.u64[0] = 0;
    ctz = iter.count_trailing_zeros(iter.cur_perm);
    ASSERT_EQ(ctz, 64);
    iter.cur_perm.u64[1] = iter.cur_perm.u64[1] << 13;
    ctz = iter.count_trailing_zeros(iter.cur_perm);
    ASSERT_EQ(ctz, 13 + 64);
    iter.cur_perm.u64[1] = 0;
    iter.cur_perm.u64[3] = 0;
    iter.cur_perm.u64[8] = ~((1ULL << 8) - 1);
    ctz = iter.count_trailing_zeros(iter.cur_perm);
    ASSERT_EQ(ctz, 8 + 8 * 64);
})

uint64_t next_perm_ref(uint64_t v) {
    uint64_t t = (v | (v - 1ULL)) + 1ULL;
    uint64_t w = t | ((((t & -t) / (v & -v)) >> 1ULL) - 1ULL);
    return w;
}

TEST_EACH_WORD_SIZE_W(pauli_string_iter, permutation_termination, {
    PauliStringIterator<W> iter(64, 2, 2);
    iter.cur_perm.u64[0] = (1ULL << 2) - 1;  // 0011
    bool return_val = false;
    for (size_t perm = 0; perm < 18144; perm++) {
        return_val = iter.iter_next_weight();
        ASSERT_EQ(return_val, true);
    }
    ASSERT_EQ(iter.cur_perm.u64[0], ((1ULL << 2) - 1) << 62);
    return_val = iter.iter_next_weight();
    ASSERT_EQ(return_val, false);
    // Test multi-word
    PauliStringIterator<W> iter2(97, 2, 2);
    iter.cur_perm.u64[0] = (1ULL << 2) - 1;  // 0011
    return_val = false;
    for (size_t perm = 0; perm < 41904; perm++) {
        return_val = iter2.iter_next_weight();
        ASSERT_EQ(return_val, true);
    }
    ASSERT_EQ(iter2.cur_perm.u64[0], 0);
    ASSERT_EQ(iter2.cur_perm.u64[1], ((1ULL << 2) - 1) << (97 - 64 - 2));
    ASSERT_EQ(iter2.iter_next_weight(), false);
})

TEST_EACH_WORD_SIZE_W(pauli_string_iter, next_permutation, {
    PauliStringIterator<W> iter(192, 2, 7);
    simd_bits<W> cur_perm(192);
    cur_perm.u64[0] = (1ULL << 4) - 1;  // 1111
    uint64_t cur_perm_ref = cur_perm.u64[0];
    //  Check all combinations within a word
    // 64 choose 4 , 635376
    for (size_t perm = 0; perm < 635375; perm++) {
        uint64_t np_ref = next_perm_ref(cur_perm_ref);
        iter.next_bitstring_of_same_hamming_weight(cur_perm);
        ASSERT_EQ(cur_perm.u64[0], np_ref) << perm;
        cur_perm_ref = np_ref;
        for (size_t w = 1; w < cur_perm.num_u64_padded(); w++) {
            ASSERT_EQ(cur_perm.u64[w], 0);
        }
    }
    cur_perm.u64[0] = 0;
    cur_perm.u64[1] = 3 << 2;
    // ..1100 00..00 -> ...1000 00..01 -> ...1000 00..10 -> ....
    // permuting in the zeroth word, there is only 1 bit so only 64 possibilities
    for (int i = 0; i < 64; i++) {
        iter.next_bitstring_of_same_hamming_weight(cur_perm);
        ASSERT_EQ(cur_perm.u64[0], 1ULL << i);
        ASSERT_EQ(cur_perm.u64[1], 16);
    }
    // Crossed word boundary so now permuting in the first word
    // i.e. 1001 00..00 -> 1010 00..00 -> 1100 00..00
    for (int i = 0; i < 3; i++) {
        iter.next_bitstring_of_same_hamming_weight(cur_perm);
        ASSERT_EQ(cur_perm.u64[0], 0);
        ASSERT_EQ(cur_perm.u64[1], 16 | (1 << i));
    }
    // More complicated example
    // 00011000 100..01 00..11100
    // 00011000 100..01 00..11100
    cur_perm.u64[0] = 7 << 2;
    cur_perm.u64[1] = (1ULL << 63) | 1;
    cur_perm.u64[2] = 3 << 3;
    iter.next_bitstring_of_same_hamming_weight(cur_perm);
    // 35 == 00..10011
    ASSERT_EQ(cur_perm.u64[0], 35);
    iter.next_bitstring_of_same_hamming_weight(cur_perm);
    // 37 == 00..10101
    ASSERT_EQ(cur_perm.u64[0], 37);
    // Fast forward to word boundary
    // 00011000 100..01 11100..000 -> 00011000 100..10 00..111
    cur_perm.u64[0] = (uint64_t)7 << (63 - 2);
    iter.next_bitstring_of_same_hamming_weight(cur_perm);
    ASSERT_EQ(cur_perm.u64[0], 7);
    ASSERT_EQ(cur_perm.u64[1], ((1ULL << 63) | 2));
    for (size_t w = 0; w < cur_perm.num_u64_padded(); w++) {
        cur_perm.u64[w] = 0;
    }
    // Test going across boundary with not many bits (caught single qubit example)
    cur_perm.u64[0] = 1;
    cur_perm.u64[1] = 0;
    cur_perm.u64[2] = 0;
    for (size_t i = 0; i < 191; i++) {
        iter.next_bitstring_of_same_hamming_weight(cur_perm);
    }
    ASSERT_EQ(cur_perm.u64[0], 0);
    ASSERT_EQ(cur_perm.u64[1], 0);
    ASSERT_EQ(cur_perm.u64[2], (1ULL << 63));
})

TEST_EACH_WORD_SIZE_W(pauli_string_iter, iter_pauli_string_multi_word, {
    size_t num_qubits = 72;
    size_t min_weight = 1;
    size_t max_weight = 1;
    PauliStringIterator<W> iter(num_qubits, min_weight, max_weight);
    size_t count = 0;
    while (iter.iter_next()) {
        count++;
    }
    ASSERT_EQ(count, 216);
    num_qubits = 65;
    min_weight = 3;
    max_weight = 3;
    PauliStringIterator<W> iter2(num_qubits, min_weight, max_weight);
    count = 0;
    while (iter2.iter_next()) {
        count++;
    }
    ASSERT_EQ(count, 1179360);
    num_qubits = 129;
    min_weight = 0;
    max_weight = 2;
    PauliStringIterator<W> iter3(num_qubits, min_weight, max_weight);
    count = 0;
    for (size_t i = 0; i < 1 + num_qubits * 3; i++) {
        iter3.iter_next();
        count++;
    }
    auto expected = PauliString<W>::from_func(false, 129, [](size_t i) {
        if (i == 128) {
            return 'Y';
        } else {
            return '_';
        }
    });
    ASSERT_EQ(iter3.result, expected);
    while (iter3.iter_next()) {
        count++;
    }
    ASSERT_EQ(count, 1 + 387 + 74304);
    expected = PauliString<W>::from_func(false, 129, [](size_t i) {
        if (i == 127 || i == 128) {
            return 'Y';
        } else {
            return '_';
        }
    });
    ASSERT_EQ(iter3.result, expected);
})