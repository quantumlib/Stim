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

#include "measure_record_writer.h"

#include "gtest/gtest.h"

#include "../test_util.test.h"

static std::string rewind_read_all(FILE *f) {
    rewind(f);
    std::string result;
    while (true) {
        int c = getc(f);
        if (c == EOF) {
            return result;
        }
        result.push_back((char)c);
    }
    fclose(f);
}

TEST(MeasureRecordWriter, Format01) {
    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, SAMPLE_FORMAT_01);
    uint8_t bytes[]{0xF8};
    writer->write_bytes({bytes, bytes + 1});
    writer->write_bit(false);
    writer->write_bytes({bytes, bytes + 1});
    writer->write_bit(true);
    writer->write_end();
    ASSERT_EQ(rewind_read_all(tmp), "000111110000111111\n");
}

TEST(MeasureRecordWriter, FormatB8) {
    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, SAMPLE_FORMAT_B8);
    uint8_t bytes[]{0xF8};
    writer->write_bytes({bytes, bytes + 1});
    writer->write_bit(false);
    writer->write_bytes({bytes, bytes + 1});
    writer->write_bit(true);
    writer->write_end();
    auto s = rewind_read_all(tmp);
    ASSERT_EQ(s.size(), 3);
    ASSERT_EQ(s[0], (char)0xF8);
    ASSERT_EQ(s[1], (char)0xF0);
    ASSERT_EQ(s[2], (char)0x03);
}

TEST(MeasureRecordWriter, FormatHits) {
    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, SAMPLE_FORMAT_HITS);
    uint8_t bytes[]{0xF8};
    writer->write_bytes({bytes, bytes + 1});
    writer->write_bit(false);
    writer->write_bytes({bytes, bytes + 1});
    writer->write_bit(true);
    writer->write_end();
    ASSERT_EQ(rewind_read_all(tmp), "3,4,5,6,7,12,13,14,15,16,17\n");
}

TEST(MeasureRecordWriter, FormatDets) {
    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, SAMPLE_FORMAT_DETS);
    uint8_t bytes[]{0xF8};
    writer->begin_result_type('D');
    writer->write_bytes({bytes, bytes + 1});
    writer->write_bit(false);
    writer->write_bytes({bytes, bytes + 1});
    writer->begin_result_type('L');
    writer->write_bit(false);
    writer->write_bit(true);
    writer->write_end();
    ASSERT_EQ(rewind_read_all(tmp), "shot D3 D4 D5 D6 D7 D12 D13 D14 D15 D16 L1\n");
}

TEST(MeasureRecordWriter, FormatR8) {
    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, SAMPLE_FORMAT_R8);
    uint8_t bytes[]{0xF8};
    writer->write_bytes({bytes, bytes + 1});
    writer->write_bit(false);
    writer->write_bytes({bytes, bytes + 1});
    writer->write_bit(true);
    writer->write_end();
    auto s = rewind_read_all(tmp);
    ASSERT_EQ(s.size(), 12);
    ASSERT_EQ(s[0], (char)3);
    ASSERT_EQ(s[1], (char)0);
    ASSERT_EQ(s[2], (char)0);
    ASSERT_EQ(s[3], (char)0);
    ASSERT_EQ(s[4], (char)0);
    ASSERT_EQ(s[5], (char)4);
    ASSERT_EQ(s[6], (char)0);
    ASSERT_EQ(s[7], (char)0);
    ASSERT_EQ(s[8], (char)0);
    ASSERT_EQ(s[9], (char)0);
    ASSERT_EQ(s[10], (char)0);
    ASSERT_EQ(s[11], (char)0);
}

TEST(MeasureRecordWriter, FormatR8_LongGap) {
    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, SAMPLE_FORMAT_R8);
    uint8_t bytes[]{0, 0, 0, 0, 0, 0, 0, 0};
    writer->write_bytes({bytes, bytes + 8});
    writer->write_bytes({bytes, bytes + 8});
    writer->write_bytes({bytes, bytes + 8});
    writer->write_bytes({bytes, bytes + 8});
    writer->write_bytes({bytes, bytes + 8});
    writer->write_bytes({bytes, bytes + 8});
    writer->write_bytes({bytes, bytes + 8});
    writer->write_bytes({bytes, bytes + 8});
    writer->write_bit(true);
    writer->write_bytes({bytes, bytes + 4});
    writer->write_end();
    auto s = rewind_read_all(tmp);
    ASSERT_EQ(s.size(), 4);
    ASSERT_EQ(s[0], (char)255);
    ASSERT_EQ(s[1], (char)255);
    ASSERT_EQ(s[2], (char)2);
    ASSERT_EQ(s[3], (char)32);
}

TEST(MeasureRecordWriter, write_table_data_small) {
    simd_bit_table results(4, 5);
    simd_bits ref_sample(0);
    results[1][0] ^= 1;
    results[1][1] ^= 1;
    results[1][2] ^= 1;
    results[1][3] ^= 1;
    results[1][4] ^= 1;

    FILE *tmp;

    tmp = tmpfile();
    write_table_data(tmp, 5, 4, ref_sample, results, SAMPLE_FORMAT_01, 'M', 'M', 0);
    ASSERT_EQ(rewind_read_all(tmp), "0100\n0100\n0100\n0100\n0100\n");

    tmp = tmpfile();
    write_table_data(tmp, 5, 4, ref_sample, results, SAMPLE_FORMAT_HITS, 'M', 'M', 0);
    ASSERT_EQ(rewind_read_all(tmp), "1\n1\n1\n1\n1\n");

    tmp = tmpfile();
    write_table_data(tmp, 5, 4, ref_sample, results, SAMPLE_FORMAT_DETS, 'M', 'M', 0);
    ASSERT_EQ(rewind_read_all(tmp), "shot M1\nshot M1\nshot M1\nshot M1\nshot M1\n");

    tmp = tmpfile();
    write_table_data(tmp, 5, 4, ref_sample, results, SAMPLE_FORMAT_DETS, 'D', 'L', 1);
    ASSERT_EQ(rewind_read_all(tmp), "shot L0\nshot L0\nshot L0\nshot L0\nshot L0\n");

    tmp = tmpfile();
    write_table_data(tmp, 5, 4, ref_sample, results, SAMPLE_FORMAT_R8, 'M', 'M', 0);
    ASSERT_EQ(rewind_read_all(tmp), "\x01\x02\x01\x02\x01\x02\x01\x02\x01\x02");

    tmp = tmpfile();
    write_table_data(tmp, 5, 4, ref_sample, results, SAMPLE_FORMAT_B8, 'M', 'M', 0);
    ASSERT_EQ(rewind_read_all(tmp), "\x02\x02\x02\x02\x02");

    tmp = tmpfile();
    write_table_data(tmp, 5, 4, ref_sample, results, SAMPLE_FORMAT_PTB64, 'M', 'M', 0);
    ASSERT_EQ(
        rewind_read_all(tmp), std::string(
                                  "\0\0\0\0\0\0\0\0"
                                  "\x1F\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0",
                                  8 * 4));
}

TEST(MeasureRecordWriter, write_table_data_large) {
    simd_bit_table results(100, 2);
    simd_bits ref_sample(100);
    ref_sample[2] ^= true;
    ref_sample[3] ^= true;
    ref_sample[5] ^= true;
    ref_sample[7] ^= true;
    ref_sample[11] ^= true;
    results[7][1] ^= true;

    FILE *tmp;

    tmp = tmpfile();
    write_table_data(tmp, 2, 100, ref_sample, results, SAMPLE_FORMAT_01, 'M', 'M', 0);
    ASSERT_EQ(
        rewind_read_all(tmp),
        "0011010100"
        "0100000000"
        "0000000000"
        "0000000000"
        "0000000000"
        "0000000000"
        "0000000000"
        "0000000000"
        "0000000000"
        "0000000000\n"
        "0011010000"
        "0100000000"
        "0000000000"
        "0000000000"
        "0000000000"
        "0000000000"
        "0000000000"
        "0000000000"
        "0000000000"
        "0000000000\n");

    tmp = tmpfile();
    write_table_data(tmp, 2, 100, ref_sample, results, SAMPLE_FORMAT_HITS, 'M', 'M', 0);
    ASSERT_EQ(rewind_read_all(tmp), "2,3,5,7,11\n2,3,5,11\n");

    tmp = tmpfile();
    write_table_data(tmp, 2, 100, ref_sample, results, SAMPLE_FORMAT_DETS, 'D', 'L', 5);
    ASSERT_EQ(rewind_read_all(tmp), "shot D2 D3 L0 L2 L6\nshot D2 D3 L0 L6\n");

    tmp = tmpfile();
    write_table_data(tmp, 2, 100, ref_sample, results, SAMPLE_FORMAT_DETS, 'D', 'L', 90);
    ASSERT_EQ(rewind_read_all(tmp), "shot D2 D3 D5 D7 D11\nshot D2 D3 D5 D11\n");

    tmp = tmpfile();
    write_table_data(tmp, 2, 100, ref_sample, results, SAMPLE_FORMAT_R8, 'M', 'M', 0);
    ASSERT_EQ(
        rewind_read_all(tmp), std::string(
                                  "\x02\x00\x01\x01\x03\x58"
                                  "\x02\x00\x01\x05\x58",
                                  11));

    tmp = tmpfile();
    write_table_data(tmp, 2, 100, ref_sample, results, SAMPLE_FORMAT_B8, 'M', 'M', 0);
    ASSERT_EQ(
        rewind_read_all(tmp), std::string(
                                  "\xAC\x08\0\0\0\0\0\0\0\0\0\0\0"
                                  "\x2C\x08\0\0\0\0\0\0\0\0\0\0\0",
                                  26));

    tmp = tmpfile();
    write_table_data(tmp, 2, 100, ref_sample, results, SAMPLE_FORMAT_PTB64, 'M', 'M', 0);
    auto actual = rewind_read_all(tmp);
    ASSERT_EQ(
        rewind_read_all(tmp), std::string(
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\x03\0\0\0\0\0\0\0"
                                  "\x03\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\x03\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\x01\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\x03\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0"
                                  "\0\0\0\0\0\0\0\0",
                                  8 * 100));
}
