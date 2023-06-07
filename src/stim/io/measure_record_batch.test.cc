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

#include "gtest/gtest.h"

#include "stim/io/measure_record_batch.h"
#include "stim/mem/simd_word.test.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(MeasureRecordBatch, basic_usage, {
    simd_bits<W> s0(5);
    simd_bits<W> s1(5);
    s0[0] = true;
    s1[1] = true;
    s0[2] = true;
    s1[3] = true;
    s0[4] = true;
    MeasureRecordBatch<W> r(5, 20);
    ASSERT_EQ(r.stored, 0);
    r.record_result(s0);
    ASSERT_EQ(r.stored, 1);
    ASSERT_EQ(r.lookback(1), s0);
    r.record_result(s1);
    ASSERT_EQ(r.stored, 2);
    ASSERT_EQ(r.lookback(1), s1);
    ASSERT_EQ(r.lookback(2), s0);

    for (size_t k = 0; k < 50; k++) {
        r.record_result(s0);
        r.record_result(s1);
    }
    ASSERT_EQ(r.unwritten, 102);
    ASSERT_EQ(r.stored, 102);
    FILE *tmp = tmpfile();
    MeasureRecordBatchWriter w(tmp, 5, SAMPLE_FORMAT_01);
    r.intermediate_write_unwritten_results_to(w, simd_bits<W>(0));
    ASSERT_EQ(r.unwritten, 102);

    for (size_t k = 0; k < 500; k++) {
        r.record_result(s0);
        r.record_result(s1);
    }
    ASSERT_EQ(r.unwritten, 1102);
    ASSERT_EQ(r.stored, 1102);
    r.intermediate_write_unwritten_results_to(w, simd_bits<W>(0));
    ASSERT_LT(r.unwritten, 100);
    ASSERT_LT(r.stored, 100);
    r.final_write_unwritten_results_to(w, simd_bits<W>(0));
    ASSERT_EQ(r.unwritten, 0);
    ASSERT_LT(r.stored, 100);

    rewind(tmp);
    for (size_t s = 0; s < 5; s++) {
        simd_bits<W> sk = (s & 1) ? s1 : s0;
        for (size_t k = 0; k < 1102; k++) {
            ASSERT_EQ(getc(tmp), '0' + ((s + k + 1) & 1));
        }
        ASSERT_EQ(getc(tmp), '\n');
    }
    ASSERT_EQ(getc(tmp), EOF);
})

TEST_EACH_WORD_SIZE_W(MeasureRecordBatch, record_zero_result, {
    MeasureRecordBatch<W> r(5, 2);
    ASSERT_EQ(r.stored, 0);
    auto v = r.record_zero_result_to_edit();
    v[2] = 1;
    ASSERT_EQ(r.stored, 1);
    ASSERT_EQ(r.storage[0][1], 0);
    ASSERT_EQ(r.storage[0][2], 1);
    ASSERT_EQ(r.storage[0][3], 0);
    ASSERT_EQ(r.lookback(1)[1], 0);
    ASSERT_EQ(r.lookback(1)[2], 1);

    r.record_zero_result_to_edit()[3] = 4;
    ASSERT_EQ(r.storage[0][1], 0);
    ASSERT_EQ(r.storage[0][2], 1);
    ASSERT_EQ(r.storage[0][3], 0);
    ASSERT_EQ(r.storage[1][1], 0);
    ASSERT_EQ(r.storage[1][2], 0);
    ASSERT_EQ(r.storage[1][3], 1);
})
