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

#include "stim/io/measure_record_reader.h"

#include "gtest/gtest.h"

#include "stim/io/measure_record_writer.h"
#include "stim/probability_util.h"
#include "stim/test_util.test.h"

using namespace stim;

FILE *tmpfile_with_contents(const std::string &contents) {
    FILE *tmp = tmpfile();
    size_t written = fwrite(contents.c_str(), 1, contents.size(), tmp);
    if (written != contents.size()) {
        int en = errno;
        std::cerr << "Failed to write to tmpfile: " << strerror(en) << std::endl;
        return nullptr;
    }
    rewind(tmp);
    return tmp;
}

bool maybe_consume_keyword(FILE *in, const std::string &keyword, int &next);

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
    ASSERT_TRUE(reader->start_record());
    ASSERT_FALSE(reader->is_end_of_record());
    // bits 0..7 == 0xF8
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(0xF8, bytes[0]);
    // bit 8 == 0
    ASSERT_FALSE(reader->read_bit());
    // bits 9..16 == 0xF8
    bytes[0] = 0;
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(0xF8, bytes[0]);
    // bit 17 == 1
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
}

TEST(MeasureRecordReader, FormatB8) {
    FILE *tmp = tmpfile_with_contents("\xF8\xF0\x03");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_B8, 18);
    ASSERT_TRUE(reader->start_record());
    ASSERT_FALSE(reader->is_end_of_record());
    // bits 0..7 == 0xF8
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(0xF8, bytes[0]);
    // bit 8 == 0
    ASSERT_FALSE(reader->read_bit());
    // bits 9..16 == 0xF8
    bytes[0] = 0;
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(0xF8, bytes[0]);
    // bit 17 == 1
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
}

TEST(MeasureRecordReader, FormatHits) {
    FILE *tmp = tmpfile_with_contents("3,4,5,6,7,12,13,14,15,16,17\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_HITS, 18);
    ASSERT_TRUE(reader->start_record());
    ASSERT_FALSE(reader->is_end_of_record());
    // bits 0..7 == 0xF8
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(0xF8, bytes[0]);
    // bit 8 == 0
    ASSERT_FALSE(reader->read_bit());
    // bits 9..16 == 0xF8
    bytes[0] = 0;
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
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
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(0xF8, bytes[0]);
    // bit 8 == 0
    ASSERT_FALSE(reader->read_bit());
    // bits 9..16 == 0xF8
    bytes[0] = 0;
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(0xF8, bytes[0]);
    // bit 17 == 1
    ASSERT_TRUE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
}

TEST(MeasureRecordReader, FormatR8_LongGap) {
    FILE *tmp = tmpfile_with_contents("\xFF\xFF\x02\x20");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_R8, 8 * 64 + 32 + 1);
    ASSERT_FALSE(reader->is_end_of_record());
    uint8_t bytes[]{1, 2, 3, 4, 5, 6, 7, 8};
    for (int i = 0; i < 8; ++i) {
        ASSERT_EQ(64, reader->read_bits_into_bytes({bytes, bytes + 8}));
        for (int j = 0; j < 8; ++j) {
            ASSERT_EQ(0, bytes[j]);
            bytes[j] = 123;
        }
    }
    ASSERT_TRUE(reader->read_bit());
    ASSERT_EQ(32, reader->read_bits_into_bytes({bytes, bytes + 4}));
    ASSERT_EQ(0, bytes[0]);
    ASSERT_EQ(0, bytes[1]);
    ASSERT_EQ(0, bytes[2]);
    ASSERT_EQ(0, bytes[3]);
    ASSERT_TRUE(reader->is_end_of_record());

    // Read past end of record.
    ASSERT_THROW({ reader->read_bit(); }, std::invalid_argument);
    // No next record.
    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, FormatDets) {
    FILE *tmp = tmpfile_with_contents("shot D3 D4 D5 D6 D7 D12 D13 D14 D15 D16 L1\n");
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 0, 17, 2);
    ASSERT_TRUE(reader->start_record());
    ASSERT_FALSE(reader->is_end_of_record());

    // Detection events:
    // bits 0..7 == 0xF8
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(0xF8, bytes[0]);
    // bit 8 == 0
    ASSERT_FALSE(reader->read_bit());
    // bits 9..16 == 0xF8
    bytes[0] = 0;
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(0xF8, bytes[0]);

    // Logical observables:
    // bit 0 == 0
    ASSERT_FALSE(reader->read_bit());
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
    EXPECT_TRUE(reader->start_record());
    return reader->read_bits_into_bytes(buf);
}

TEST(MeasureRecordReader, Format01_WriteRead) {
    uint8_t src[]{0, 1, 2, 3, 4, 0xFF, 0xBF, 0xFE, 80, 0, 0, 1, 20};
    constexpr size_t num_bytes = sizeof(src) / sizeof(uint8_t);
    uint8_t dst[num_bytes];
    memset(dst, 0, num_bytes);
    FILE *tmp = write_records({src, src + num_bytes}, SAMPLE_FORMAT_01);
    rewind(tmp);
    ASSERT_EQ(num_bytes * 8, read_records_as_bytes(tmp, {dst, dst + num_bytes}, SAMPLE_FORMAT_01, 8 * num_bytes));
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
    ASSERT_EQ(num_bytes * 8, read_records_as_bytes(tmp, {dst, dst + num_bytes}, SAMPLE_FORMAT_B8, 8 * num_bytes));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(src[i], dst[i]);
    }
}

TEST(MeasureRecordReader, FormatR8_WriteRead) {
    uint8_t src[]{0, 1, 2, 3, 4, 0xFF, 0xBF, 0xFE, 80, 0, 0, 1, 20};
    constexpr size_t num_bytes = sizeof(src) / sizeof(uint8_t);
    uint8_t dst[num_bytes]{};
    FILE *tmp = write_records({src, src + num_bytes}, SAMPLE_FORMAT_R8);
    rewind(tmp);
    ASSERT_EQ(num_bytes * 8, read_records_as_bytes(tmp, {dst, dst + num_bytes}, SAMPLE_FORMAT_R8, 8 * num_bytes));
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
    ASSERT_EQ(
        num_bytes * 8 - 1, read_records_as_bytes(tmp, {dst, dst + num_bytes}, SAMPLE_FORMAT_HITS, 8 * num_bytes - 1));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(src[i], dst[i]);
    }
}

TEST(MeasureRecordReader, FormatHits_OutOfOrder) {
    FILE *tmp = tmpfile_with_contents("5,3\n");
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_HITS, 8, 0, 0);
    ASSERT_TRUE(reader->start_record());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_FALSE(reader->start_record());
}

TEST(MeasureRecordReader, FormatDets_OutOfOrder) {
    FILE *tmp = tmpfile_with_contents("shot L2 D3 D1\n");
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 0, 4, 4);
    ASSERT_TRUE(reader->start_record());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_FALSE(reader->start_record());
}

TEST(MeasureRecordReader, FormatDets_WriteRead) {
    uint8_t src[]{0, 1, 2, 3, 4, 0xFF, 0xBF, 0xFE, 80, 0, 0, 1, 20};
    constexpr size_t num_bytes = sizeof(src) / sizeof(uint8_t);
    uint8_t dst[num_bytes];
    memset(dst, 0, num_bytes);
    FILE *tmp = write_records({src, src + num_bytes}, SAMPLE_FORMAT_DETS);
    rewind(tmp);
    ASSERT_EQ(
        num_bytes * 8 - 1, read_records_as_bytes(tmp, {dst, dst + num_bytes}, SAMPLE_FORMAT_DETS, 8 * num_bytes - 1));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(src[i], dst[i]);
    }
}

TEST(MeasureRecordReader, Format01_WriteRead_MultipleRecords) {
    uint8_t record1[]{0x12, 0xAB, 0x00, 0xFF, 0x75};
    uint8_t record2[]{0x80, 0xFF, 0x01, 0x56, 0x57};
    uint8_t record3[]{0x2F, 0x08, 0xF0, 0x1C, 0x60};
    constexpr size_t num_bytes = sizeof(record1) / sizeof(uint8_t);
    constexpr size_t bits_per_record = 8 * num_bytes;

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
    ASSERT_TRUE(reader->start_record());

    memset(buf, 0, num_bytes);
    ASSERT_EQ(bits_per_record, reader->read_bits_into_bytes({buf, buf + num_bytes}));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(buf[i], record1[i]);
    }
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->next_record());

    memset(buf, 0, num_bytes);
    ASSERT_EQ(bits_per_record, reader->read_bits_into_bytes({buf, buf + num_bytes}));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(buf[i], record2[i]);
    }
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->next_record());

    memset(buf, 0, num_bytes);
    ASSERT_EQ(bits_per_record, reader->read_bits_into_bytes({buf, buf + num_bytes}));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(buf[i], record3[i]);
    }
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
    ASSERT_TRUE(reader->start_record());

    memset(buf, 0, num_bytes);
    ASSERT_EQ(bits_per_record, reader->read_bits_into_bytes({buf, buf + num_bytes}));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(buf[i], record1[i]);
    }
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->next_record());

    memset(buf, 0, num_bytes);
    ASSERT_EQ(bits_per_record, reader->read_bits_into_bytes({buf, buf + num_bytes}));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(buf[i], record2[i]);
    }
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->next_record());

    memset(buf, 0, num_bytes);
    ASSERT_EQ(bits_per_record, reader->read_bits_into_bytes({buf, buf + num_bytes}));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(buf[i], record3[i]);
    }
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
    ASSERT_TRUE(reader->start_record());

    memset(buf, 0, num_bytes);
    ASSERT_EQ(bits_per_record, reader->read_bits_into_bytes({buf, buf + num_bytes}));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(buf[i], record1[i]);
    }
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->next_record());

    memset(buf, 0, num_bytes);
    ASSERT_EQ(bits_per_record, reader->read_bits_into_bytes({buf, buf + num_bytes}));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(buf[i], record2[i]);
    }
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_TRUE(reader->next_record());

    memset(buf, 0, num_bytes);
    ASSERT_EQ(bits_per_record, reader->read_bits_into_bytes({buf, buf + num_bytes}));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(buf[i], record3[i]);
    }
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, Format01_MultipleRecords) {
    FILE *tmp = tmpfile_with_contents("111011001\n01000000\n101100011");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_01, 9);
    ASSERT_TRUE(reader->start_record());
    ASSERT_FALSE(reader->is_end_of_record());
    // first record
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
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
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
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
    ASSERT_TRUE(reader->start_record());
    ASSERT_FALSE(reader->is_end_of_record());
    // first record
    uint8_t bytes[]{0};
    ASSERT_EQ(2, reader->read_bits_into_bytes(bytes));
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
    ASSERT_EQ(2, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(1, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    // no more records
    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, FormatHits_MultipleRecords) {
    FILE *tmp = tmpfile_with_contents("0,1,2,4,5,8\n1\n0\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_HITS, 9);
    ASSERT_TRUE(reader->start_record());
    ASSERT_FALSE(reader->is_end_of_record());
    // first record
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
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
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
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
    ASSERT_TRUE(reader->start_record());
    ASSERT_FALSE(reader->is_end_of_record());
    // first record
    uint8_t bytes[]{0};
    ASSERT_EQ(2, reader->read_bits_into_bytes(bytes));
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
    ASSERT_EQ(2, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(1, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    // fourth record
    ASSERT_TRUE(reader->next_record());
    bytes[0] = 0xFF;
    ASSERT_EQ(2, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(0, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    // no more records
    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, FormatDets_MultipleRecords) {
    FILE *tmp = tmpfile_with_contents("shot M0 M1 M2 M4 M5 M8\nshot M1\nshot M0\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 9);
    ASSERT_TRUE(reader->start_record());
    ASSERT_FALSE(reader->is_end_of_record());
    // first record
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
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
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(1, bytes[0]);
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    // no more records
    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, FormatDets_MultipleShortRecords) {
    FILE *tmp = tmpfile_with_contents("shot M0\nshot M1\nshot M0\nshot\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 2);
    ASSERT_TRUE(reader->start_record());
    ASSERT_FALSE(reader->is_end_of_record());
    // first record
    uint8_t bytes[]{0};
    ASSERT_EQ(2, reader->read_bits_into_bytes(bytes));
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
    ASSERT_EQ(2, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(1, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    // fourth record (0 bits)
    ASSERT_TRUE(reader->next_record());
    bytes[0] = 0xFF;
    ASSERT_EQ(2, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(0, bytes[0]);
    ASSERT_TRUE(reader->is_end_of_record());
    // no more records
    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, FormatDets_MultipleResultTypes_D0L0) {
    FILE *tmp = tmpfile_with_contents("shot D0 D3 D5 L1 L2\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 0, 7, 4);
    ASSERT_TRUE(reader->start_record());
    ASSERT_FALSE(reader->is_end_of_record());
    // Detection events
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(0x29, bytes[0]);
    // Logical observables
    bytes[0] = 0;
    ASSERT_EQ(3, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(3, bytes[0]);

    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, FormatDets_MultipleResultTypes_D1L0) {
    FILE *tmp = tmpfile_with_contents("shot D0 D3 D5 D6 L1 L2\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 0, 7, 4);
    ASSERT_TRUE(reader->start_record());
    ASSERT_FALSE(reader->is_end_of_record());
    // Detection events
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(0x69, bytes[0]);
    // Logical observables
    bytes[0] = 0;
    ASSERT_EQ(3, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(3, bytes[0]);

    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, FormatDets_MultipleResultTypes_D0L1) {
    FILE *tmp = tmpfile_with_contents("shot D0 D3 D5 L0 L1 L2\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 0, 7, 4);
    ASSERT_TRUE(reader->start_record());
    ASSERT_FALSE(reader->is_end_of_record());
    // Detection events
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(0xA9, bytes[0]);
    // Logical observables
    bytes[0] = 0;
    ASSERT_EQ(3, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(3, bytes[0]);

    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, FormatDets_MultipleResultTypes_D1L1) {
    FILE *tmp = tmpfile_with_contents("shot D0 D3 D5 D6 L0 L1 L2\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 0, 7, 4);
    ASSERT_TRUE(reader->start_record());
    ASSERT_FALSE(reader->is_end_of_record());
    // Detection events
    uint8_t bytes[]{0};
    ASSERT_EQ(8, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(0xE9, bytes[0]);
    // Logical observables
    bytes[0] = 0;
    ASSERT_EQ(3, reader->read_bits_into_bytes(bytes));
    ASSERT_EQ(3, bytes[0]);

    ASSERT_FALSE(reader->next_record());
}

TEST(MeasureRecordReader, Format01_InvalidInput) {
    FILE *tmp = tmpfile_with_contents("012\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_01, 3);
    ASSERT_TRUE(reader->start_record());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->read_bit());
    ASSERT_THROW({ reader->read_bit(); }, std::runtime_error);
}

TEST(MeasureRecordReader, FormatHits_InvalidInput) {
    FILE *tmp = tmpfile_with_contents("100,1\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_HITS, 3);
    ASSERT_THROW({ reader->start_record(); }, std::runtime_error);
}

TEST(MeasureRecordReader, FormatHits_Repeated) {
    FILE *tmp = tmpfile_with_contents("1,1\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_HITS, 3);
    ASSERT_TRUE(reader->start_record());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_FALSE(reader->read_bit());
    ASSERT_TRUE(reader->is_end_of_record());
    ASSERT_FALSE(reader->start_record());
}

TEST(MeasureRecordReader, FormatDets_InvalidInput) {
    FILE *tmp = tmpfile_with_contents("D2\n");
    ASSERT_NE(tmp, nullptr);
    auto r = MeasureRecordReader::make(tmp, SAMPLE_FORMAT_DETS, 3);
    ASSERT_THROW({ r->start_record(); }, std::runtime_error);
}

TEST(MeasureRecordReader, read_records_into_RoundTrip) {
    size_t n_shots = 100;
    size_t n_results = 512 - 8;

    simd_bit_table shot_maj_data = simd_bit_table::random(n_shots, n_results, SHARED_TEST_RNG());
    simd_bit_table shot_min_data = shot_maj_data.transposed();
    for (const auto &kv : format_name_to_enum_map) {
        SampleFormat format = kv.second.id;
        if (format == SampleFormat::SAMPLE_FORMAT_PTB64) {
            // TODO: support this format.
            continue;
        }

        // Write data to file.
        FILE *f = tmpfile();
        {
            auto writer = MeasureRecordWriter::make(f, format);
            for (size_t k = 0; k < n_shots; k++) {
                writer->write_bytes({shot_maj_data[k].u8, shot_maj_data[k].u8 + n_results / 8});
                writer->write_end();
            }
        }

        // Check that read shot-min data matches written data.
        rewind(f);
        {
            auto reader = MeasureRecordReader::make(f, format, n_results, 0, 0);
            simd_bit_table read_shot_min_data(n_results, n_shots);
            size_t n = reader->read_records_into(read_shot_min_data, false);
            EXPECT_EQ(n, n_shots) << kv.second.name << " (not striped)";
            EXPECT_EQ(read_shot_min_data, shot_min_data) << kv.second.name << " (not striped)";
        }

        // Check that read shot-maj data matches written data when transposing.
        rewind(f);
        {
            auto reader = MeasureRecordReader::make(f, format, n_results, 0, 0);
            simd_bit_table read_shot_maj_data(n_shots, n_results);
            size_t n = reader->read_records_into(read_shot_maj_data, true);
            EXPECT_EQ(n, n_shots) << kv.second.name << " (striped)";
            EXPECT_EQ(read_shot_maj_data, shot_maj_data) << kv.second.name << " (striped)";
        }

        fclose(f);
    }
}

TEST(MeasureRecordReader, read_bits_into_bytes_entire_record_across_result_type) {
    FILE *f = tmpfile_with_contents("shot D1 L1");
    auto reader = MeasureRecordReader::make(f, SAMPLE_FORMAT_DETS, 0, 3, 3);
    simd_bit_table read(6, 1);
    size_t n = reader->read_records_into(read, false);
    fclose(f);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(read[0][0], false);
    ASSERT_EQ(read[1][0], true);
    ASSERT_EQ(read[2][0], false);
    ASSERT_EQ(read[3][0], false);
    ASSERT_EQ(read[4][0], true);
    ASSERT_EQ(read[5][0], false);
}

TEST(MeasureRecordReader, read_r8_detection_event_data) {
    FILE *f = tmpfile();
    putc(6, f);
    putc(1, f);
    putc(4, f);
    rewind(f);
    auto reader = MeasureRecordReader::make(f, SAMPLE_FORMAT_R8, 0, 3, 3);
    simd_bit_table read(6, 2);
    size_t n = reader->read_records_into(read, false);
    fclose(f);
    ASSERT_EQ(n, 2);
    ASSERT_EQ(read[0][0], false);
    ASSERT_EQ(read[1][0], false);
    ASSERT_EQ(read[2][0], false);
    ASSERT_EQ(read[3][0], false);
    ASSERT_EQ(read[4][0], false);
    ASSERT_EQ(read[5][0], false);
    ASSERT_EQ(read[0][1], false);
    ASSERT_EQ(read[1][1], true);
    ASSERT_EQ(read[2][1], false);
    ASSERT_EQ(read[3][1], false);
    ASSERT_EQ(read[4][1], false);
    ASSERT_EQ(read[5][1], false);
}

TEST(MeasureRecordReader, read_b8_detection_event_data_full_run_together) {
    FILE *f = tmpfile();
    putc(0, f);
    putc(1, f);
    putc(2, f);
    putc(3, f);
    rewind(f);
    auto reader = MeasureRecordReader::make(f, SAMPLE_FORMAT_B8, 0, 27, 0);
    simd_bit_table read(27, 1);
    size_t n = reader->read_records_into(read, false);
    fclose(f);
    ASSERT_EQ(n, 1);
    auto t = read.transposed();
    ASSERT_EQ(t[0].u8[0], 0);
    ASSERT_EQ(t[0].u8[1], 1);
    ASSERT_EQ(t[0].u8[2], 2);
    ASSERT_EQ(t[0].u8[3], 3);
}

TEST(MeasureRecordReader, start_and_read_entire_record) {
    size_t n = 512 - 8;
    size_t no = 5;
    size_t nd = n - no;

    // Compute expected data.
    simd_bits test_data(n);
    biased_randomize_bits(0.1, test_data.u64, test_data.u64 + test_data.num_u64_padded(), SHARED_TEST_RNG());
    SparseShot sparse_test_data;
    for (size_t k = 0; k < nd; k++) {
        if (test_data[k]) {
            sparse_test_data.hits.push_back(k);
        }
    }
    for (size_t k = 0; k < no; k++) {
        if (test_data[k + nd]) {
            sparse_test_data.obs_mask |= 1 << k;
        }
    }

    for (const auto &kv : format_name_to_enum_map) {
        SampleFormat format = kv.second.id;
        if (format == SampleFormat::SAMPLE_FORMAT_PTB64) {
            // TODO: support this format.
            continue;
        }

        // Write data to file.
        FILE *f = tmpfile();
        {
            auto writer = MeasureRecordWriter::make(f, format);
            writer->begin_result_type('D');
            for (size_t k = 0; k < nd; k++) {
                writer->write_bit(test_data[k]);
            }
            writer->begin_result_type('L');
            for (size_t k = 0; k < no; k++) {
                writer->write_bit(test_data[k + nd]);
            }
            writer->write_end();
        }

        {
            auto reader = MeasureRecordReader::make(f, format, 0, nd, no);

            // Check sparse record read.
            SparseShot sparse_out;
            rewind(f);
            ASSERT_TRUE(reader->start_and_read_entire_record(sparse_out));
            ASSERT_EQ(sparse_out, sparse_test_data);
            ASSERT_FALSE(reader->start_and_read_entire_record(sparse_out));

            rewind(f);
            simd_bits dense_out(n);
            ASSERT_TRUE(reader->start_and_read_entire_record(dense_out));
            for (size_t k = 0; k < n; k++) {
                ASSERT_EQ(dense_out[k], test_data[k]);
            }
            ASSERT_FALSE(reader->start_and_read_entire_record(dense_out));
        }

        fclose(f);
    }
}

TEST(MeasureRecordReader, start_and_read_entire_record_all_zero) {
    simd_bits test_data(256);
    SparseShot sparse_test_data;

    for (const auto &kv : format_name_to_enum_map) {
        SampleFormat format = kv.second.id;
        if (format == SampleFormat::SAMPLE_FORMAT_PTB64) {
            // TODO: support this format.
            continue;
        }

        // Write data to file.
        FILE *f = tmpfile();
        {
            auto writer = MeasureRecordWriter::make(f, format);
            writer->write_bytes({test_data.u8, test_data.u8 + 32});
            writer->write_end();
        }

        {
            auto reader = MeasureRecordReader::make(f, format, 256, 0, 0);

            // Check sparse record read.
            SparseShot sparse_out;
            rewind(f);
            ASSERT_TRUE(reader->start_and_read_entire_record(sparse_out));
            ASSERT_EQ(sparse_out, sparse_test_data);
            ASSERT_FALSE(reader->start_and_read_entire_record(sparse_out));

            rewind(f);
            simd_bits dense_out(256);
            ASSERT_TRUE(reader->start_and_read_entire_record(dense_out));
            ASSERT_EQ(dense_out, test_data);
            ASSERT_FALSE(reader->start_and_read_entire_record(dense_out));
        }

        fclose(f);
    }
}
