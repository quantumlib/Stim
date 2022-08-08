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

#include "stim/io/measure_record_batch_writer.h"

#include "gtest/gtest.h"

#include "stim/test_util.test.h"

using namespace stim;

TEST(MeasureRecordBatchWriter, basic_usage) {
    FILE *tmp = tmpfile();
    MeasureRecordBatchWriter w(tmp, 5, SAMPLE_FORMAT_01);
    simd_bits<MAX_BITWORD_WIDTH> v(5);
    v[1] = true;
    w.batch_write_bit(v);
    w.write_end();
    rewind(tmp);
    ASSERT_EQ(getc(tmp), '0');
    ASSERT_EQ(getc(tmp), '\n');
    ASSERT_EQ(getc(tmp), '1');
    ASSERT_EQ(getc(tmp), '\n');
    ASSERT_EQ(getc(tmp), '0');
    ASSERT_EQ(getc(tmp), '\n');
    ASSERT_EQ(getc(tmp), '0');
    ASSERT_EQ(getc(tmp), '\n');
    ASSERT_EQ(getc(tmp), '0');
    ASSERT_EQ(getc(tmp), '\n');
}
