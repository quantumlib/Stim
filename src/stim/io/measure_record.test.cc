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

#include "stim/io/measure_record.h"

#include "gtest/gtest.h"

#include "stim/test_util.test.h"

using namespace stim;

TEST(MeasureRecord, basic_usage) {
    MeasureRecord r(20);
    r.record_result(true);
    ASSERT_EQ(r.lookback(1), true);
    r.record_result(false);
    ASSERT_EQ(r.lookback(1), false);
    ASSERT_EQ(r.lookback(2), true);
    for (size_t k = 0; k < 50; k++) {
        r.record_result(true);
        r.record_result(false);
    }
    ASSERT_EQ(r.storage.size(), 102);

    FILE *tmp = tmpfile();
    r.write_unwritten_results_to(*MeasureRecordWriter::make(tmp, SAMPLE_FORMAT_01));
    rewind(tmp);
    for (size_t k = 0; k < 102; k++) {
        ASSERT_EQ(getc(tmp), '0' + (~k & 1));
    }
    ASSERT_EQ(getc(tmp), EOF);

    ASSERT_LE(r.storage.size(), 40);
}
