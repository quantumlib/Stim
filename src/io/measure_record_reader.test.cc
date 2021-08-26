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

bool maybe_consume_keyword(FILE *in, const std::string& keyword, int &next);
bool read_uint64(FILE* in, uint64_t &value, int &next);

TEST(maybe_consume_keyword, FoundKeyword) {
    int next = 0;
    FILE *tmp = tmpfile_with_contents("shot\n");
    ASSERT_NE(tmp, nullptr);
    ASSERT_TRUE(maybe_consume_keyword(tmp, "shot", next));
    ASSERT_EQ(next, '\n');
}

TEST(maybe_consume_keyword, NotFoundKeyword) {
    int next = 0;
    FILE *tmp = tmpfile_with_contents("hit\n");
    ASSERT_NE(tmp, nullptr);
    ASSERT_THROW({ maybe_consume_keyword(tmp, "shot", next); }, std::runtime_error);
}

TEST(maybe_consume_keyword, FoundEOF) {
    int next = 0;
    FILE *tmp = tmpfile_with_contents("");
    ASSERT_NE(tmp, nullptr);
    ASSERT_FALSE(maybe_consume_keyword(tmp, "shot", next));
    ASSERT_EQ(next, EOF);
}

TEST(read_unsigned_int, BasicUsage) {
    int next = 0;
    uint64_t value = 0;
    FILE *tmp = tmpfile_with_contents("105\n");
    ASSERT_NE(tmp, nullptr);
    ASSERT_TRUE(read_uint64(tmp, value, next));
    ASSERT_EQ(next, '\n');
    ASSERT_EQ(value, 105);
}

TEST(read_unsigned_int, ValueTooBig) {
    int next = 0;
    uint64_t value = 0;
    FILE *tmp = tmpfile_with_contents("18446744073709551616\n");
    ASSERT_NE(tmp, nullptr);
    ASSERT_THROW({ read_uint64(tmp, value, next); }, std::runtime_error);
}

TEST(MeasureRecordReader, Format01) {
    // We'll read the data like this:  |-0xF8-|0|-0xF8-|1|
    FILE *tmp = tmpfile_with_contents("000111110000111111\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_01, 18);
    ASSERT_FALSE(reader->is_end_of_record());
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
}

TEST(MeasureRecordReader, FormatB8) {
    FILE *tmp = tmpfile_with_contents("\xF8\xF0\x03");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_B8, 18);
    ASSERT_FALSE(reader->is_end_of_record());
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
}

TEST(MeasureRecordReader, FormatHits) {
    FILE *tmp = tmpfile_with_contents("3,4,5,6,7,12,13,14,15,16,17\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_HITS, 18);
    ASSERT_FALSE(reader->is_end_of_record());
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
}

TEST(MeasureRecordReader, FormatR8) {
    char tmp_data[]{3, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0};
    FILE *tmp = tmpfile_with_contents(std::string(std::begin(tmp_data), std::end(tmp_data)));
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_R8, 18);
    ASSERT_FALSE(reader->is_end_of_record());
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
}

TEST(MeasureRecordReader, FormatR8_LongGap) {
    FILE *tmp = tmpfile_with_contents("\xFF\xFF\x02\x20");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_R8, 8 * 64 + 4 * 32 + 1);
    ASSERT_FALSE(reader->is_end_of_record());
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
}

TEST(MeasureRecordReader, FormatDets) {
    FILE *tmp = tmpfile_with_contents("shot D3 D4 D5 D6 D7 D12 D13 D14 D15 D16 L1\n");
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 0, 17, 2);
    ASSERT_FALSE(reader->is_end_of_record());

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
}

FILE *write_records(ConstPointerRange<uint8_t> data, SampleFormat format) {
    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, format);
    writer->write_bytes(data);
    writer->write_end();
    return tmp;
}

size_t read_records_as_bytes(FILE *in, PointerRange<uint8_t> buf, SampleFormat format, size_t bits_per_record) {
    auto reader = MeasureRecordReader::make(in, format, bits_per_record);
    return reader->read_bytes(buf);
}

TEST(MeasureRecordReader, Format01_WriteRead) {
    uint8_t src[]{0, 1, 2, 3, 4, 0xFF, 0xBF, 0xFE, 80, 0, 0, 1, 20};
    constexpr size_t num_bytes = sizeof(src) / sizeof(uint8_t);
    uint8_t dst[num_bytes];
    memset(dst, 0, num_bytes);
    FILE *tmp = write_records({src, src + num_bytes}, SAMPLE_FORMAT_01);
    rewind(tmp);
    ASSERT_EQ(num_bytes * 8 - 1, read_records_as_bytes(tmp, {dst, dst + num_bytes}, SAMPLE_FORMAT_01, 8 * num_bytes - 1));
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
    ASSERT_EQ(num_bytes * 8 - 1, read_records_as_bytes(tmp, {dst, dst + num_bytes}, SAMPLE_FORMAT_B8, 8 * num_bytes - 1));
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
    ASSERT_EQ(num_bytes * 8 - 1, read_records_as_bytes(tmp, {dst, dst + num_bytes}, SAMPLE_FORMAT_R8, 8 * num_bytes - 1));
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
    ASSERT_EQ(num_bytes * 8 - 1, read_records_as_bytes(tmp, {dst, dst + num_bytes}, SAMPLE_FORMAT_HITS, 8 * num_bytes - 1));
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
    ASSERT_EQ(num_bytes * 8 - 1, read_records_as_bytes(tmp, {dst, dst + num_bytes}, SAMPLE_FORMAT_DETS, 8 * num_bytes - 1));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(src[i], dst[i]);
    }
}

TEST(MeasureRecordReader, Format01_WriteRead_MultipleRecords) {
    uint8_t record1[]{0x12, 0xAB, 0x00, 0xFF, 0x75};
    uint8_t record2[]{0x80, 0xFF, 0x01, 0x56, 0x57};
    uint8_t record3[]{0x2F, 0x08, 0xF0, 0x1C, 0x60};
    constexpr size_t num_bytes = sizeof(record1) / sizeof(uint8_t);
    constexpr size_t bits_per_record = 8 * num_bytes - 1;

    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, SAMPLE_FORMAT_01);
    writer->write_bytes({record1, record1 + num_bytes});
    writer->write_end();
    writer->write_bytes({record2, record2 + num_bytes});
    writer->write_end();
    writer->write_bytes({record3, record3 + num_bytes});
    writer->write_end();

    rewind(tmp);

    uint8_t buf[num_bytes];
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_01, bits_per_record);

    memset(buf, 0, num_bytes);
    ASSERT_EQ(bits_per_record, reader->read_bytes({buf, buf + num_bytes}));
    for (size_t i = 0; i < num_bytes; ++i) ASSERT_EQ(buf[i], record1[i]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->next_record());

    memset(buf, 0, num_bytes);
    ASSERT_EQ(bits_per_record, reader->read_bytes({buf, buf + num_bytes}));
    for (size_t i = 0; i < num_bytes; ++i) ASSERT_EQ(buf[i], record2[i]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->next_record());

    memset(buf, 0, num_bytes);
    ASSERT_EQ(bits_per_record, reader->read_bytes({buf, buf + num_bytes}));
    for (size_t i = 0; i < num_bytes; ++i) ASSERT_EQ(buf[i], record3[i]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, FormatHits_WriteRead_MultipleRecords) {
    uint8_t record1[]{0x12, 0xAB, 0x00, 0xFF, 0x75};
    uint8_t record2[]{0x80, 0xFF, 0x01, 0x56, 0x57};
    uint8_t record3[]{0x2F, 0x08, 0xF0, 0x1C, 0x60};
    constexpr size_t num_bytes = sizeof(record1) / sizeof(uint8_t);
    constexpr size_t bits_per_record = 8 * num_bytes - 1;

    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, SAMPLE_FORMAT_HITS);
    writer->write_bytes({record1, record1 + num_bytes});
    writer->write_end();
    writer->write_bytes({record2, record2 + num_bytes});
    writer->write_end();
    writer->write_bytes({record3, record3 + num_bytes});
    writer->write_end();

    rewind(tmp);

    uint8_t buf[num_bytes];
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_HITS, bits_per_record);

    memset(buf, 0, num_bytes);
    ASSERT_EQ(bits_per_record, reader->read_bytes({buf, buf + num_bytes}));
    for (size_t i = 0; i < num_bytes; ++i) ASSERT_EQ(buf[i], record1[i]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->next_record());

    memset(buf, 0, num_bytes);
    ASSERT_EQ(bits_per_record, reader->read_bytes({buf, buf + num_bytes}));
    for (size_t i = 0; i < num_bytes; ++i) ASSERT_EQ(buf[i], record2[i]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->next_record());

    memset(buf, 0, num_bytes);
    ASSERT_EQ(bits_per_record, reader->read_bytes({buf, buf + num_bytes}));
    for (size_t i = 0; i < num_bytes; ++i) ASSERT_EQ(buf[i], record3[i]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, FormatDets_WriteRead_MultipleRecords) {
    uint8_t record1[]{0x12, 0xAB, 0x00, 0xFF, 0x75};
    uint8_t record2[]{0x80, 0xFF, 0x01, 0x56, 0x57};
    uint8_t record3[]{0x2F, 0x08, 0xF0, 0x1C, 0x60};
    constexpr size_t num_bytes = sizeof(record1) / sizeof(uint8_t);
    constexpr size_t bits_per_record = 8 * num_bytes - 1;

    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, SAMPLE_FORMAT_DETS);
    writer->write_bytes({record1, record1 + num_bytes});
    writer->write_end();
    writer->write_bytes({record2, record2 + num_bytes});
    writer->write_end();
    writer->write_bytes({record3, record3 + num_bytes});
    writer->write_end();

    rewind(tmp);

    uint8_t buf[num_bytes];
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, bits_per_record);

    memset(buf, 0, num_bytes);
    ASSERT_EQ(bits_per_record, reader->read_bytes({buf, buf + num_bytes}));
    for (size_t i = 0; i < num_bytes; ++i) ASSERT_EQ(buf[i], record1[i]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->next_record());

    memset(buf, 0, num_bytes);
    ASSERT_EQ(bits_per_record, reader->read_bytes({buf, buf + num_bytes}));
    for (size_t i = 0; i < num_bytes; ++i) ASSERT_EQ(buf[i], record2[i]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->next_record());

    memset(buf, 0, num_bytes);
    ASSERT_EQ(bits_per_record, reader->read_bytes({buf, buf + num_bytes}));
    for (size_t i = 0; i < num_bytes; ++i) ASSERT_EQ(buf[i], record3[i]);
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, Format01_MultipleRecords) {
    FILE *tmp = tmpfile_with_contents("111011001\n01000000\n101100011");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_01, 9);
    ASSERT_FALSE(reader->is_end_of_record());
    // first record
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bytes(bytes));
    ASSERT_EQ(0x37, bytes[0]);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    // second_record
    ASSERT_TRUE(reader->next_record());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_FALSE(reader->is_end_of_record());
    // third record
    ASSERT_TRUE(reader->next_record());
    bytes[0] = 0;
    ASSERT_EQ(8, reader->read_bytes(bytes));
    ASSERT_EQ(0x8D, bytes[0]);
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    // no more records
    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, Format01_MultipleShortRecords) {
    FILE *tmp = tmpfile_with_contents("10\n01\n10\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_01, 2);
    ASSERT_FALSE(reader->is_end_of_record());
    // first record
    uint8_t bytes[]{0};
    ASSERT_EQ(2, reader->read_bytes(bytes));
    ASSERT_EQ(1, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    // second_record
    ASSERT_TRUE(reader->next_record());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    // third record
    ASSERT_TRUE(reader->next_record());
    bytes[0] = 0;
    ASSERT_EQ(2, reader->read_bytes(bytes));
    ASSERT_EQ(1, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    // no more records
    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, FormatHits_MultipleRecords) {
    FILE *tmp = tmpfile_with_contents("0,1,2,4,5,8\n1\n0\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_HITS, 9);
    ASSERT_FALSE(reader->is_end_of_record());
    // first record
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bytes(bytes));
    ASSERT_EQ(0x37, bytes[0]);
    ASSERT_FALSE(reader->is_end_of_record());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    // second_record
    ASSERT_TRUE(reader->next_record());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_FALSE(reader->is_end_of_record());
    // third record
    ASSERT_TRUE(reader->next_record());
    bytes[0] = 0;
    ASSERT_EQ(8, reader->read_bytes(bytes));
    ASSERT_EQ(1, bytes[0]);
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    // no more records
    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, FormatHits_MultipleShortRecords) {
    FILE *tmp = tmpfile_with_contents("0\n1\n0\n\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_HITS, 2);
    ASSERT_FALSE(reader->is_end_of_record());
    // first record
    uint8_t bytes[]{0};
    ASSERT_EQ(2, reader->read_bytes(bytes));
    ASSERT_EQ(1, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    // second_record
    ASSERT_TRUE(reader->next_record());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    // third record
    ASSERT_TRUE(reader->next_record());
    bytes[0] = 0;
    ASSERT_EQ(2, reader->read_bytes(bytes));
    ASSERT_EQ(1, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    // fourth record
    ASSERT_TRUE(reader->next_record());
    bytes[0] = 0xFF;
    ASSERT_EQ(2, reader->read_bytes(bytes));
    ASSERT_EQ(0, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    // no more records
    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, FormatDets_MultipleRecords) {
    FILE *tmp = tmpfile_with_contents("shot M0 M1 M2 M4 M5 M8\nshot M1\nshot M0\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 9);
    ASSERT_FALSE(reader->is_end_of_record());
    // first record
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bytes(bytes));
    ASSERT_EQ(0x37, bytes[0]);
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    // second_record
    ASSERT_TRUE(reader->next_record());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_FALSE(reader->is_end_of_record());
    // third record
    ASSERT_TRUE(reader->next_record());
    bytes[0] = 0;
    ASSERT_EQ(8, reader->read_bytes(bytes));
    ASSERT_EQ(1, bytes[0]);
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    // no more recocrds
    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, FormatDets_MultipleShortRecords) {
    FILE *tmp = tmpfile_with_contents("shot M0\nshot M1\nshot M0\nshot\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 2);
    ASSERT_FALSE(reader->is_end_of_record());
    // first record
    uint8_t bytes[]{0};
    ASSERT_EQ(2, reader->read_bytes(bytes));
    ASSERT_EQ(1, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    // second_record
    ASSERT_TRUE(reader->next_record());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    // third record
    ASSERT_TRUE(reader->next_record());
    bytes[0] = 0;
    ASSERT_EQ(2, reader->read_bytes(bytes));
    ASSERT_EQ(1, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    // fourth record (0 bits)
    ASSERT_TRUE(reader->next_record());
    bytes[0] = 0xFF;
    ASSERT_EQ(2, reader->read_bytes(bytes));
    ASSERT_EQ(0, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    // no more recocrds
    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, FormatDets_MultipleResultTypes_D0L0) {
    FILE *tmp = tmpfile_with_contents("shot D0 D3 D5 L1 L2\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 0, 7, 4);
    ASSERT_FALSE(reader->is_end_of_record());
    // Detection events
    uint8_t bytes[]{0};
    ASSERT_EQ('D', reader->current_result_type());
    ASSERT_EQ(7, reader->read_bytes(bytes));
    ASSERT_EQ(0x29, bytes[0]);
    // Logical observables
    bytes[0] = 0;
    ASSERT_EQ('L', reader->current_result_type());
    ASSERT_EQ(4, reader->read_bytes(bytes));
    ASSERT_EQ(6, bytes[0]);

    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, FormatDets_MultipleResultTypes_D1L0) {
    FILE *tmp = tmpfile_with_contents("shot D0 D3 D5 D6 L1 L2\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 0, 7, 4);
    ASSERT_FALSE(reader->is_end_of_record());
    // Detection events
    uint8_t bytes[]{0};
    ASSERT_EQ('D', reader->current_result_type());
    ASSERT_EQ(7, reader->read_bytes(bytes));
    ASSERT_EQ(0x69, bytes[0]);
    // Logical observables
    bytes[0] = 0;
    ASSERT_EQ('L', reader->current_result_type());
    ASSERT_EQ(4, reader->read_bytes(bytes));
    ASSERT_EQ(6, bytes[0]);

    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, FormatDets_MultipleResultTypes_D0L1) {
    FILE *tmp = tmpfile_with_contents("shot D0 D3 D5 L0 L1 L2\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 0, 7, 4);
    ASSERT_FALSE(reader->is_end_of_record());
    // Detection events
    uint8_t bytes[]{0};
    ASSERT_EQ('D', reader->current_result_type());
    ASSERT_EQ(7, reader->read_bytes(bytes));
    ASSERT_EQ(0x29, bytes[0]);
    // Logical observables
    bytes[0] = 0;
    ASSERT_EQ('L', reader->current_result_type());
    ASSERT_EQ(4, reader->read_bytes(bytes));
    ASSERT_EQ(7, bytes[0]);

    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, FormatDets_MultipleResultTypes_D1L1) {
    FILE *tmp = tmpfile_with_contents("shot D0 D3 D5 D6 L0 L1 L2\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 0, 7, 4);
    ASSERT_FALSE(reader->is_end_of_record());
    // Detection events
    uint8_t bytes[]{0};
    ASSERT_EQ('D', reader->current_result_type());
    ASSERT_EQ(7, reader->read_bytes(bytes));
    ASSERT_EQ(0x69, bytes[0]);
    // Logical observables
    bytes[0] = 0;
    ASSERT_EQ('L', reader->current_result_type());
    ASSERT_EQ(4, reader->read_bytes(bytes));
    ASSERT_EQ(7, bytes[0]);

    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, Format01_InvalidInput) {
    FILE *tmp = tmpfile_with_contents("012\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_01, 3);
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_THROW({ reader->read_bit(); }, std::runtime_error);
}

TEST(MeasureRecordReader, FormatHits_InvalidInput) {
    FILE *tmp = tmpfile_with_contents("1,1\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_HITS, 3);
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_THROW({ reader->read_bit(); }, std::runtime_error);
}

TEST(MeasureRecordReader, FormatDets_InvalidInput) {
    FILE *tmp = tmpfile_with_contents("D2\n");
    ASSERT_NE(tmp, nullptr);
    ASSERT_THROW({ MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 3); }, std::runtime_error);
}

