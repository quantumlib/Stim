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

#include "measure_record_reader.h"
#include "measure_record_writer.h"

#include "gtest/gtest.h"

#include "../test_util.test.h"

using namespace stim_internal;

FILE* tmpfile_with_contents(const std::string &contents) {
    FILE* tmp = tmpfile();
    size_t written = fwrite(contents.c_str(), 1, contents.size(), tmp);
    if (written != contents.size()) {
        int en = errno;
        std::cerr << "Failed to write to tmpfile: " << strerror(en) << std::endl;
        return nullptr;
    }
    rewind(tmp);
    return tmp;
}

TEST(MeasureRecordReader, Format01) {
    // We'll read the data like this:  |-0xF8-|0|-0xF8-|1|
    FILE *tmp = tmpfile_with_contents("000111110000111111\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_01);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    // bits 0..7 == 0xF8
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bytes(bytes));
    ASSERT_EQ(0xF8, bytes[0]);
    // bit 8 == 0
    ASSERT_FALSE(reader->read_bit());
    // bits 9..16 == 0xF8
    bytes[0] = 0;
    ASSERT_EQ(8, reader->read_bytes(bytes));
    ASSERT_EQ(0xF8, bytes[0]);
    // bit 17 == 1
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}

TEST(MeasureRecordReader, FormatB8) {
    FILE *tmp = tmpfile_with_contents("\xF8\xF0\x03");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_B8);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    // bits 0..7 == 0xF8
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bytes(bytes));
    ASSERT_EQ(0xF8, bytes[0]);
    // bit 8 == 0
    ASSERT_FALSE(reader->read_bit());
    // bits 9..16 == 0xF8
    bytes[0] = 0;
    ASSERT_EQ(8, reader->read_bytes(bytes));
    ASSERT_EQ(0xF8, bytes[0]);
    // bit 17 == 1
    ASSERT_TRUE(reader->read_bit());
    // bits 18..24 == 0
    for (int i = 0; i < 6; ++i) {
        ASSERT_FALSE(reader->is_end_of_record());
        ASSERT_FALSE(reader->is_end_of_file());
        ASSERT_FALSE(reader->read_bit());
    }
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}

TEST(MeasureRecordReader, FormatHits) {
    FILE *tmp = tmpfile_with_contents("3,4,5,6,7,12,13,14,15,16,17\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_HITS, 18);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    // bits 0..7 == 0xF8
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bytes(bytes));
    ASSERT_EQ(0xF8, bytes[0]);
    // bit 8 == 0
    ASSERT_FALSE(reader->read_bit());
    // bits 9..16 == 0xF8
    bytes[0] = 0;
    ASSERT_EQ(8, reader->read_bytes(bytes));
    ASSERT_EQ(0xF8, bytes[0]);
    // bit 17 == 1
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}

TEST(MeasureRecordReader, FormatR8) {
    char tmp_data[]{3, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0};
    FILE *tmp = tmpfile_with_contents(std::string(std::begin(tmp_data), std::end(tmp_data)));
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_R8);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    // bits 0..7 == 0xF8
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bytes(bytes));
    ASSERT_EQ(0xF8, bytes[0]);
    // bit 8 == 0
    ASSERT_FALSE(reader->read_bit());
    // bits 9..16 == 0xF8
    bytes[0] = 0;
    ASSERT_EQ(8, reader->read_bytes(bytes));
    ASSERT_EQ(0xF8, bytes[0]);
    // bit 17 == 1
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}

TEST(MeasureRecordReader, FormatR8_LongGap) {
    FILE *tmp = tmpfile_with_contents("\xFF\xFF\x02\x20");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_R8);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    uint8_t bytes[]{1, 2, 3, 4, 5, 6, 7, 8};
    for (int i = 0; i < 8; ++i) {
        ASSERT_EQ(64, reader->read_bytes({bytes, bytes + 8}));
        for (int j = 0; j < 8; ++j) {
            ASSERT_EQ(0, bytes[j]);
            bytes[j] = 123;
        }
    }
    ASSERT_TRUE(reader->read_bit());
    ASSERT_EQ(32, reader->read_bytes({bytes, bytes + 4}));
    ASSERT_EQ(0, bytes[0]);
    ASSERT_EQ(0, bytes[1]);
    ASSERT_EQ(0, bytes[2]);
    ASSERT_EQ(0, bytes[3]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}

TEST(MeasureRecordReader, FormatDets) {
    FILE *tmp = tmpfile_with_contents("shot D3 D4 D5 D6 D7 D12 D13 D14 D15 D16 L1\n");
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 19);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());

    // Detection events:
    // bits 0..7 == 0xF8
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bytes(bytes));
    ASSERT_EQ(0xF8, bytes[0]);
    ASSERT_EQ(reader->current_result_type(), 'D');
    // bit 8 == 0
    ASSERT_FALSE(reader->read_bit());
    // bits 9..16 == 0xF8
    bytes[0] = 0;
    ASSERT_EQ(8, reader->read_bytes(bytes));
    ASSERT_EQ(0xF8, bytes[0]);

    // Logical observables:
    // bit 0 == 0
    ASSERT_FALSE(reader->read_bit());
    ASSERT_EQ(reader->current_result_type(), 'L');
    // bit 1 == 1
    ASSERT_TRUE(reader->read_bit());

    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}

FILE *write_records(ConstPointerRange<uint8_t> data, SampleFormat format) {
    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, format);
    writer->write_bytes(data);
    writer->write_end();
    return tmp;
}

size_t read_records_as_bytes(FILE *in, PointerRange<uint8_t> buf, SampleFormat format, size_t num_bits) {
    auto reader = MeasureRecordReader::make(in, format, num_bits);
    return reader->read_bytes(buf);
}

TEST(MeasureRecordReader, Format01_WriteRead) {
    uint8_t src[]{0, 1, 2, 3, 4, 0xFF, 0xBF, 0xFE, 80, 0, 0, 1, 20};
    constexpr size_t num_bytes = sizeof(src) / sizeof(uint8_t);
    uint8_t dst[num_bytes];
    memset(dst, 0, num_bytes);
    FILE *tmp = write_records({src, src + num_bytes}, SAMPLE_FORMAT_01);
    rewind(tmp);
    ASSERT_EQ(num_bytes * 8, read_records_as_bytes(tmp, {dst, dst + num_bytes}, SAMPLE_FORMAT_01, num_bytes * 8));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(src[i], dst[i]);
    }
}

TEST(MeasureRecordReader, FormatB8_WriteRead) {
    uint8_t src[]{0, 1, 2, 3, 4, 0xFF, 0xBF, 0xFE, 80, 0, 0, 1, 20};
    constexpr size_t num_bytes = sizeof(src) / sizeof(uint8_t);
    uint8_t dst[num_bytes];
    memset(dst, 0, num_bytes);
    FILE *tmp = write_records({src, src + num_bytes}, SAMPLE_FORMAT_B8);
    rewind(tmp);
    ASSERT_EQ(num_bytes * 8, read_records_as_bytes(tmp, {dst, dst + num_bytes}, SAMPLE_FORMAT_B8, num_bytes * 8));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(src[i], dst[i]);
    }
}

TEST(MeasureRecordReader, FormatR8_WriteRead) {
    uint8_t src[]{0, 1, 2, 3, 4, 0xFF, 0xBF, 0xFE, 80, 0, 0, 1, 20};
    constexpr size_t num_bytes = sizeof(src) / sizeof(uint8_t);
    uint8_t dst[num_bytes];
    memset(dst, 0, num_bytes);
    FILE *tmp = write_records({src, src + num_bytes}, SAMPLE_FORMAT_R8);
    rewind(tmp);
    ASSERT_EQ(num_bytes * 8, read_records_as_bytes(tmp, {dst, dst + num_bytes}, SAMPLE_FORMAT_R8, num_bytes * 8));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(src[i], dst[i]);
    }
}

TEST(MeasureRecordReader, FormatHits_WriteRead) {
    uint8_t src[]{0, 1, 2, 3, 4, 0xFF, 0xBF, 0xFE, 80, 0, 0, 1, 20};
    constexpr size_t num_bytes = sizeof(src) / sizeof(uint8_t);
    uint8_t dst[num_bytes];
    memset(dst, 0, num_bytes);
    FILE *tmp = write_records({src, src + num_bytes}, SAMPLE_FORMAT_HITS);
    rewind(tmp);
    ASSERT_EQ(num_bytes * 8, read_records_as_bytes(tmp, {dst, dst + num_bytes}, SAMPLE_FORMAT_HITS, num_bytes * 8));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(src[i], dst[i]);
    }
}

TEST(MeasureRecordReader, FormatDets_WriteRead) {
    uint8_t src[]{0, 1, 2, 3, 4, 0xFF, 0xBF, 0xFE, 80, 0, 0, 1, 20};
    constexpr size_t num_bytes = sizeof(src) / sizeof(uint8_t);
    uint8_t dst[num_bytes];
    memset(dst, 0, num_bytes);
    FILE *tmp = write_records({src, src + num_bytes}, SAMPLE_FORMAT_DETS);
    rewind(tmp);
    // TODO: Use max_bits for record size rather than overall size
    ASSERT_EQ(101, read_records_as_bytes(tmp, {dst, dst + num_bytes}, SAMPLE_FORMAT_DETS, num_bytes * 8));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(src[i], dst[i]);
    }
}

TEST(MeasureRecordReader, Format01_MultipleRecords) {
    FILE *tmp = tmpfile_with_contents("111011001\n01\n1");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_01);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    // first record (9 bits)
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bytes(bytes));
    ASSERT_EQ(0x37, bytes[0]);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    // second_record (2 bits)
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    // third record (1 bit)
    bytes[0] = 0;
    ASSERT_EQ(1, reader->read_bytes(bytes));
    ASSERT_EQ(1, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}

TEST(MeasureRecordReader, Format01_MultipleShortRecords) {
    FILE *tmp = tmpfile_with_contents("1\n01\n1\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_01);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    // first record (1 bit)
    uint8_t bytes[]{0};
    ASSERT_EQ(1, reader->read_bytes(bytes));
    ASSERT_EQ(1, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    // second_record (2 bits)
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    // third record (1 bit)
    bytes[0] = 0;
    ASSERT_EQ(1, reader->read_bytes(bytes));
    ASSERT_EQ(1, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}

TEST(MeasureRecordReader, FormatDets_MultipleRecords) {
    FILE *tmp = tmpfile_with_contents("shot M0 M1 M2 M4 M5 M8\nshot M1\nshot M0\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    // first record
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bytes(bytes));
    ASSERT_EQ(0x37, bytes[0]);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    // second_record
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    // third record
    bytes[0] = 0;
    ASSERT_EQ(1, reader->read_bytes(bytes));
    ASSERT_EQ(1, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}

TEST(MeasureRecordReader, FormatDets_MultipleShortRecords) {
    FILE *tmp = tmpfile_with_contents("shot M0\nshot M1\nshot M0\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    // first record
    uint8_t bytes[]{0};
    ASSERT_EQ(1, reader->read_bytes(bytes));
    ASSERT_EQ(1, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    // second_record
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    // third record
    bytes[0] = 0;
    ASSERT_EQ(1, reader->read_bytes(bytes));
    ASSERT_EQ(1, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}

TEST(MeasureRecordReader, Format01_Limit_ReadBytes) {
    FILE *tmp = tmpfile_with_contents("01111111\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_01, 5);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    uint8_t bytes[]{0};
    ASSERT_EQ(5, reader->read_bytes(bytes));
    ASSERT_EQ(0x1E, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}

TEST(MeasureRecordReader, FormatB8_Limit_ReadBytes) {
    FILE *tmp = tmpfile_with_contents("\xFE");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_B8, 5);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    uint8_t bytes[]{0};
    ASSERT_EQ(5, reader->read_bytes(bytes));
    ASSERT_EQ(0x1E, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}

TEST(MeasureRecordReader, FormatHits_Limit_ReadBytes) {
    FILE *tmp = tmpfile_with_contents("1,2,3,4,5,6,7\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_HITS, 5);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    uint8_t bytes[]{0};
    ASSERT_EQ(5, reader->read_bytes(bytes));
    ASSERT_EQ(0x1E, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}

TEST(MeasureRecordReader, FormatR8_Limit_ReadBytes) {
    char tmp_data[]{1, 0, 0, 0, 0, 0, 0, 0};
    FILE *tmp = tmpfile_with_contents(std::string(std::begin(tmp_data), std::end(tmp_data)));
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_R8, 5);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    uint8_t bytes[]{0};
    ASSERT_EQ(5, reader->read_bytes(bytes));
    ASSERT_EQ(0x1E, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}

TEST(MeasureRecordReader, FormatDets_Limit_ReadBytes) {
    FILE *tmp = tmpfile_with_contents("shot M1 M2 M3 M4 M5 M6 M7\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 5);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    uint8_t bytes[]{0};
    ASSERT_EQ(5, reader->read_bytes(bytes));
    ASSERT_EQ(0x1E, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}

TEST(MeasureRecordReader, Format01_Limit_ReadBits) {
    FILE *tmp = tmpfile_with_contents("01111111\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_01, 2);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}

TEST(MeasureRecordReader, FormatB8_Limit_ReadBits) {
    FILE *tmp = tmpfile_with_contents("\xFE");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_B8, 2);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}

TEST(MeasureRecordReader, FormatHits_Limit_ReadBits) {
    FILE *tmp = tmpfile_with_contents("1,2,3,4,5,6,7\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_HITS, 2);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}

TEST(MeasureRecordReader, FormatR8_Limit_ReadBits) {
    char tmp_data[]{1, 0, 0, 0, 0, 0, 0, 0};
    FILE *tmp = tmpfile_with_contents(std::string(std::begin(tmp_data), std::end(tmp_data)));
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_R8, 2);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}

TEST(MeasureRecordReader, FormatDets_Limit_ReadBits) {
    FILE *tmp = tmpfile_with_contents("shot M1 M2 M3 M4 M5 M6 M7\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 2);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_FALSE(reader->is_end_of_file());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->is_end_of_file());
}
