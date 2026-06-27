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

#include "stim/io/measure_record_writer.h"

#include "gtest/gtest.h"

#include "stim/mem/simd_word.test.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

TEST(MeasureRecordWriter, Format01) {
    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, SampleFormat::SAMPLE_FORMAT_01);
    uint8_t bytes[]{0xF8};
    writer->write_bytes({bytes});
    writer->write_bit(false);
    writer->write_bytes({bytes});
    writer->write_bit(true);
    writer->write_end();
    ASSERT_EQ(rewind_read_close(tmp), "000111110000111111\n");
}

TEST(MeasureRecordWriter, FormatB8) {
    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, SampleFormat::SAMPLE_FORMAT_B8);
    uint8_t bytes[]{0xF8};
    writer->write_bytes({bytes});
    writer->write_bit(false);
    writer->write_bytes({bytes});
    writer->write_bit(true);
    writer->write_end();
    auto s = rewind_read_close(tmp);
    ASSERT_EQ(s.size(), 3);
    ASSERT_EQ(s[0], (char)0xF8);
    ASSERT_EQ(s[1], (char)0xF0);
    ASSERT_EQ(s[2], (char)0x03);
}

TEST(MeasureRecordWriter, FormatHits) {
    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, SampleFormat::SAMPLE_FORMAT_HITS);
    uint8_t bytes[]{0xF8};
    writer->write_bytes({bytes});
    writer->write_bit(false);
    writer->write_bytes({bytes});
    writer->write_bit(true);
    writer->write_end();
    ASSERT_EQ(rewind_read_close(tmp), "3,4,5,6,7,12,13,14,15,16,17\n");
}

TEST(MeasureRecordWriter, FormatDets) {
    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, SampleFormat::SAMPLE_FORMAT_DETS);
    uint8_t bytes[]{0xF8};
    writer->begin_result_type('D');
    writer->write_bytes({bytes});
    writer->write_bit(false);
    writer->write_bytes({bytes});
    writer->begin_result_type('L');
    writer->write_bit(false);
    writer->write_bit(true);
    writer->write_end();
    ASSERT_EQ(rewind_read_close(tmp), "shot D3 D4 D5 D6 D7 D12 D13 D14 D15 D16 L1\n");
}

TEST(MeasureRecordWriter, FormatDets_EmptyRecords) {
    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, SampleFormat::SAMPLE_FORMAT_DETS);
    writer->write_end();
    writer->write_end();
    writer->write_end();
    ASSERT_EQ(rewind_read_close(tmp), "shot\nshot\nshot\n");
}

TEST(MeasureRecordWriter, FormatDets_MultipleRecords) {
    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, SampleFormat::SAMPLE_FORMAT_DETS);

    // First record
    writer->begin_result_type('D');
    writer->write_bit(false);
    writer->write_bit(false);
    writer->write_bit(true);
    writer->begin_result_type('L');
    writer->write_bit(false);
    writer->write_bit(true);
    writer->write_end();

    // Second record
    writer->begin_result_type('D');
    writer->write_bit(true);
    writer->write_bit(false);
    writer->write_bit(false);
    writer->write_bit(true);
    writer->begin_result_type('L');
    writer->write_bit(true);
    writer->write_bit(false);
    writer->write_bit(true);
    writer->write_end();

    ASSERT_EQ(rewind_read_close(tmp), "shot D2 L1\nshot D0 D3 L0 L2\n");
}

TEST(MeasureRecordWriter, FormatR8) {
    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, SampleFormat::SAMPLE_FORMAT_R8);
    uint8_t bytes[]{0xF8};
    writer->write_bytes({bytes});
    writer->write_bit(false);
    writer->write_bytes({bytes});
    writer->write_bit(true);
    writer->write_end();
    auto s = rewind_read_close(tmp);
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
    auto writer = MeasureRecordWriter::make(tmp, SampleFormat::SAMPLE_FORMAT_R8);
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
    auto s = rewind_read_close(tmp);
    ASSERT_EQ(s.size(), 4);
    ASSERT_EQ(s[0], (char)255);
    ASSERT_EQ(s[1], (char)255);
    ASSERT_EQ(s[2], (char)2);
    ASSERT_EQ(s[3], (char)32);
}

TEST_EACH_WORD_SIZE_W(MeasureRecordWriter, write_table_data_small, {
    simd_bit_table<W> results(4, 5);
    simd_bits<W> ref_sample(0);
    results[1][0] ^= 1;
    results[1][1] ^= 1;
    results[1][2] ^= 1;
    results[1][3] ^= 1;
    results[1][4] ^= 1;

    FILE *tmp;

    tmp = tmpfile();
    write_table_data<W>(tmp, 5, 4, ref_sample, results, SampleFormat::SAMPLE_FORMAT_01, 'M', 'M', 0);
    ASSERT_EQ(rewind_read_close(tmp), "0100\n0100\n0100\n0100\n0100\n");

    tmp = tmpfile();
    write_table_data<W>(tmp, 5, 4, ref_sample, results, SampleFormat::SAMPLE_FORMAT_HITS, 'M', 'M', 0);
    ASSERT_EQ(rewind_read_close(tmp), "1\n1\n1\n1\n1\n");

    tmp = tmpfile();
    write_table_data<W>(tmp, 5, 4, ref_sample, results, SampleFormat::SAMPLE_FORMAT_DETS, 'M', 'M', 0);
    ASSERT_EQ(rewind_read_close(tmp), "shot M1\nshot M1\nshot M1\nshot M1\nshot M1\n");

    tmp = tmpfile();
    write_table_data<W>(tmp, 5, 4, ref_sample, results, SampleFormat::SAMPLE_FORMAT_DETS, 'D', 'L', 1);
    ASSERT_EQ(rewind_read_close(tmp), "shot L0\nshot L0\nshot L0\nshot L0\nshot L0\n");

    tmp = tmpfile();
    write_table_data<W>(tmp, 5, 4, ref_sample, results, SampleFormat::SAMPLE_FORMAT_R8, 'M', 'M', 0);
    ASSERT_EQ(rewind_read_close(tmp), "\x01\x02\x01\x02\x01\x02\x01\x02\x01\x02");

    tmp = tmpfile();
    write_table_data<W>(tmp, 5, 4, ref_sample, results, SampleFormat::SAMPLE_FORMAT_B8, 'M', 'M', 0);
    ASSERT_EQ(rewind_read_close(tmp), "\x02\x02\x02\x02\x02");

    tmp = tmpfile();
    write_table_data<W>(tmp, 64, 4, ref_sample, results, SampleFormat::SAMPLE_FORMAT_PTB64, 'M', 'M', 0);
    ASSERT_EQ(
        rewind_read_close(tmp),
        std::string(
            "\0\0\0\0\0\0\0\0"
            "\x1F\0\0\0\0\0\0\0"
            "\0\0\0\0\0\0\0\0"
            "\0\0\0\0\0\0\0\0",
            8 * 4));
})

TEST_EACH_WORD_SIZE_W(MeasureRecordWriter, write_table_data_large, {
    simd_bit_table<W> results(100, 2);
    simd_bits<W> ref_sample(100);
    ref_sample[2] ^= true;
    ref_sample[3] ^= true;
    ref_sample[5] ^= true;
    ref_sample[7] ^= true;
    ref_sample[11] ^= true;
    results[7][1] ^= true;

    FILE *tmp;

    tmp = tmpfile();
    write_table_data<W>(tmp, 2, 100, ref_sample, results, SampleFormat::SAMPLE_FORMAT_01, 'M', 'M', 0);
    ASSERT_EQ(
        rewind_read_close(tmp),
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
    write_table_data<W>(tmp, 2, 100, ref_sample, results, SampleFormat::SAMPLE_FORMAT_HITS, 'M', 'M', 0);
    ASSERT_EQ(rewind_read_close(tmp), "2,3,5,7,11\n2,3,5,11\n");

    tmp = tmpfile();
    write_table_data<W>(tmp, 2, 100, ref_sample, results, SampleFormat::SAMPLE_FORMAT_DETS, 'D', 'L', 5);
    ASSERT_EQ(rewind_read_close(tmp), "shot D2 D3 L0 L2 L6\nshot D2 D3 L0 L6\n");

    tmp = tmpfile();
    write_table_data<W>(tmp, 2, 100, ref_sample, results, SampleFormat::SAMPLE_FORMAT_DETS, 'D', 'L', 90);
    ASSERT_EQ(rewind_read_close(tmp), "shot D2 D3 D5 D7 D11\nshot D2 D3 D5 D11\n");

    tmp = tmpfile();
    write_table_data<W>(tmp, 2, 100, ref_sample, results, SampleFormat::SAMPLE_FORMAT_R8, 'M', 'M', 0);
    ASSERT_EQ(
        rewind_read_close(tmp),
        std::string(
            "\x02\x00\x01\x01\x03\x58"
            "\x02\x00\x01\x05\x58",
            11));

    tmp = tmpfile();
    write_table_data<W>(tmp, 2, 100, ref_sample, results, SampleFormat::SAMPLE_FORMAT_B8, 'M', 'M', 0);
    ASSERT_EQ(
        rewind_read_close(tmp),
        std::string(
            "\xAC\x08\0\0\0\0\0\0\0\0\0\0\0"
            "\x2C\x08\0\0\0\0\0\0\0\0\0\0\0",
            26));

    tmp = tmpfile();
    write_table_data<W>(tmp, 64, 100, ref_sample, results, SampleFormat::SAMPLE_FORMAT_PTB64, 'M', 'M', 0);
    auto actual = rewind_read_close(tmp);
    ASSERT_EQ(
        actual,
        std::string(
            "\0\0\0\0\0\0\0\0"
            "\0\0\0\0\0\0\0\0"
            "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
            "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
            "\0\0\0\0\0\0\0\0"
            "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
            "\0\0\0\0\0\0\0\0"
            "\xFD\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
            "\0\0\0\0\0\0\0\0"
            "\0\0\0\0\0\0\0\0"
            "\0\0\0\0\0\0\0\0"
            "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
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
})

TEST(MeasureRecordWriter, write_bits_01_a) {
    FILE *f = tmpfile();
    uint8_t data[]{0x0, 0xFF};
    auto writer = MeasureRecordWriter::make(f, SampleFormat::SAMPLE_FORMAT_01);
    writer->write_bits(&data[0], 11);
    writer->write_end();
    ASSERT_EQ(rewind_read_close(f), "00000000111\n");
}

TEST(MeasureRecordWriter, write_bits_01_b) {
    FILE *f = tmpfile();
    uint8_t data[]{0xFF, 0x0};
    auto writer = MeasureRecordWriter::make(f, SampleFormat::SAMPLE_FORMAT_01);
    writer->write_bits(&data[0], 11);
    writer->write_end();
    ASSERT_EQ(rewind_read_close(f), "11111111000\n");
}

TEST(MeasureRecordWriter, write_bits_b8_a) {
    FILE *f = tmpfile();
    uint8_t data[]{0x0, 0xFF};
    auto writer = MeasureRecordWriter::make(f, SampleFormat::SAMPLE_FORMAT_B8);
    writer->write_bits(&data[0], 11);
    writer->write_end();
    ASSERT_EQ(rewind_read_close(f), std::string("\x00\x07", 2));
}

TEST(MeasureRecordWriter, write_bits_b8_b) {
    FILE *f = tmpfile();
    uint8_t data[]{0xFF, 0x0};
    auto writer = MeasureRecordWriter::make(f, SampleFormat::SAMPLE_FORMAT_B8);
    writer->write_bits(&data[0], 11);
    writer->write_end();
    ASSERT_EQ(rewind_read_close(f), std::string("\xFF\x00", 2));
}

TEST(MeasureRecordWriter, write_bits_r8_a) {
    FILE *f = tmpfile();
    uint8_t data[]{0x0, 0xFF};
    auto writer = MeasureRecordWriter::make(f, SampleFormat::SAMPLE_FORMAT_R8);
    writer->write_bits(&data[0], 11);
    writer->write_end();
    ASSERT_EQ(rewind_read_close(f), std::string("\x08\x00\x00\x00", 4));
}

TEST(MeasureRecordWriter, write_bits_r8_b) {
    FILE *f = tmpfile();
    uint8_t data[]{0xFF, 0x0};
    auto writer = MeasureRecordWriter::make(f, SampleFormat::SAMPLE_FORMAT_R8);
    writer->write_bits(&data[0], 11);
    writer->write_end();
    ASSERT_EQ(rewind_read_close(f), std::string("\x00\x00\x00\x00\x00\x00\x00\x00\x03", 9));
}
