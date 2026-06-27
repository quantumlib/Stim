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

#include "stim/io/measure_record_batch_writer.h"
#include "stim/io/measure_record_writer.h"
#include "stim/mem/simd_word.test.h"
#include "stim/util_bot/probability_util.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

FILE *tmpfile_with_contents(std::string_view contents) {
    FILE *tmp = tmpfile();
    size_t written = fwrite(contents.data(), 1, contents.size(), tmp);
    if (written != contents.size()) {
        int en = errno;
        std::cerr << "Failed to write to tmpfile: " << strerror(en) << std::endl;
        throw std::invalid_argument("Failed to write to tmpfile");
    }
    rewind(tmp);
    return tmp;
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

template <size_t W>
void assert_contents_load_correctly(SampleFormat format, std::string_view contents) {
    FILE *tmp = tmpfile_with_contents(contents);
    auto reader = MeasureRecordReader<W>::make(tmp, format, 18);
    simd_bits<W> buf(18);
    reader->start_and_read_entire_record(buf);
    ASSERT_EQ(buf[0], 0);
    ASSERT_EQ(buf[1], 0);
    ASSERT_EQ(buf[2], 0);
    ASSERT_EQ(buf[3], 1);
    ASSERT_EQ(buf[4], 1);
    ASSERT_EQ(buf[5], 1);
    ASSERT_EQ(buf[6], 1);
    ASSERT_EQ(buf[7], 1);
    ASSERT_EQ(buf[8], 0);
    ASSERT_EQ(buf[9], 0);
    ASSERT_EQ(buf[10], 0);
    ASSERT_EQ(buf[11], 0);
    ASSERT_EQ(buf[12], 1);
    ASSERT_EQ(buf[13], 1);
    ASSERT_EQ(buf[14], 1);
    ASSERT_EQ(buf[15], 1);
    ASSERT_EQ(buf[16], 1);
    ASSERT_EQ(buf[17], 1);

    rewind(tmp);
    SparseShot sparse;
    reader->start_and_read_entire_record(sparse);
    ASSERT_EQ(sparse.hits, (std::vector<uint64_t>{3, 4, 5, 6, 7, 12, 13, 14, 15, 16, 17}));
}

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, Format01, {
    assert_contents_load_correctly<W>(SampleFormat::SAMPLE_FORMAT_01, "000111110000111111\n");
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, FormatB8, {
    assert_contents_load_correctly<W>(SampleFormat::SAMPLE_FORMAT_B8, "\xF8\xF0\x03");
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, FormatHits, {
    assert_contents_load_correctly<W>(SampleFormat::SAMPLE_FORMAT_HITS, "3,4,5,6,7,12,13,14,15,16,17\n");
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, FormatR8, {
    char tmp_data[]{3, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0};
    assert_contents_load_correctly<W>(
        SampleFormat::SAMPLE_FORMAT_R8, std::string(std::begin(tmp_data), std::end(tmp_data)));
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, FormatR8_LongGap, {
    FILE *tmp = tmpfile_with_contents("\xFF\xFF\x02\x20");
    auto reader = MeasureRecordReader<W>::make(tmp, SampleFormat::SAMPLE_FORMAT_R8, 8 * 64 + 32 + 1);
    SparseShot sparse;
    ASSERT_TRUE(reader->start_and_read_entire_record(sparse));
    ASSERT_EQ(sparse.hits, (std::vector<uint64_t>{255 + 255 + 2}));
    ASSERT_FALSE(reader->start_and_read_entire_record(sparse));
})

FILE *write_records(SpanRef<const uint8_t> data, SampleFormat format) {
    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, format);
    writer->write_bytes(data);
    writer->write_end();
    return tmp;
}

template <size_t W>
size_t read_records_as_bytes(FILE *in, SpanRef<uint8_t> buf, SampleFormat format, size_t bits_per_record) {
    auto reader = MeasureRecordReader<W>::make(in, format, bits_per_record);
    if (buf.size() * 8 < reader->bits_per_record()) {
        throw std::invalid_argument("buf too small");
    }
    simd_bits<W> buf2(reader->bits_per_record());
    bool success = reader->start_and_read_entire_record(buf2);
    EXPECT_TRUE(success);
    memcpy(buf.ptr_start, buf2.u8, (reader->bits_per_record() + 7) / 8);
    return reader->bits_per_record();
}

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, Format01_WriteRead, {
    uint8_t src[]{0, 1, 2, 3, 4, 0xFF, 0xBF, 0xFE, 80, 0, 0, 1, 20};
    constexpr size_t num_bytes = sizeof(src) / sizeof(uint8_t);
    uint8_t dst[num_bytes];
    memset(dst, 0, num_bytes);
    FILE *tmp = write_records({src, src + num_bytes}, SampleFormat::SAMPLE_FORMAT_01);
    rewind(tmp);
    ASSERT_EQ(
        num_bytes * 8,
        read_records_as_bytes<W>(tmp, {dst, dst + num_bytes}, SampleFormat::SAMPLE_FORMAT_01, 8 * num_bytes));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(src[i], dst[i]);
    }
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, FormatB8_WriteRead, {
    uint8_t src[]{0, 1, 2, 3, 4, 0xFF, 0xBF, 0xFE, 80, 0, 0, 1, 20};
    constexpr size_t num_bytes = sizeof(src) / sizeof(uint8_t);
    uint8_t dst[num_bytes];
    memset(dst, 0, num_bytes);
    FILE *tmp = write_records({src, src + num_bytes}, SampleFormat::SAMPLE_FORMAT_B8);
    rewind(tmp);
    ASSERT_EQ(
        num_bytes * 8,
        read_records_as_bytes<W>(tmp, {dst, dst + num_bytes}, SampleFormat::SAMPLE_FORMAT_B8, 8 * num_bytes));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(src[i], dst[i]);
    }
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, FormatR8_WriteRead, {
    uint8_t src[]{0, 1, 2, 3, 4, 0xFF, 0xBF, 0xFE, 80, 0, 0, 1, 20};
    constexpr size_t num_bytes = sizeof(src) / sizeof(uint8_t);
    uint8_t dst[num_bytes]{};
    FILE *tmp = write_records({src, src + num_bytes}, SampleFormat::SAMPLE_FORMAT_R8);
    rewind(tmp);
    ASSERT_EQ(
        num_bytes * 8,
        read_records_as_bytes<W>(tmp, {dst, dst + num_bytes}, SampleFormat::SAMPLE_FORMAT_R8, 8 * num_bytes));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(src[i], dst[i]);
    }
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, FormatHits_WriteRead, {
    uint8_t src[]{0, 1, 2, 3, 4, 0xFF, 0xBF, 0xFE, 80, 0, 0, 1, 20};
    constexpr size_t num_bytes = sizeof(src) / sizeof(uint8_t);
    uint8_t dst[num_bytes];
    memset(dst, 0, num_bytes);
    FILE *tmp = write_records({src, src + num_bytes}, SampleFormat::SAMPLE_FORMAT_HITS);
    rewind(tmp);
    ASSERT_EQ(
        num_bytes * 8 - 1,
        read_records_as_bytes<W>(tmp, {dst, dst + num_bytes}, SampleFormat::SAMPLE_FORMAT_HITS, 8 * num_bytes - 1));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(src[i], dst[i]);
    }
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, FormatDets_WriteRead, {
    uint8_t src[]{0, 1, 2, 3, 4, 0xFF, 0xBF, 0xFE, 80, 0, 0, 1, 20};
    constexpr size_t num_bytes = sizeof(src) / sizeof(uint8_t);
    uint8_t dst[num_bytes];
    memset(dst, 0, num_bytes);
    FILE *tmp = write_records({src, src + num_bytes}, SampleFormat::SAMPLE_FORMAT_DETS);
    rewind(tmp);
    ASSERT_EQ(
        num_bytes * 8 - 1,
        read_records_as_bytes<W>(tmp, {dst, dst + num_bytes}, SampleFormat::SAMPLE_FORMAT_DETS, 8 * num_bytes - 1));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(src[i], dst[i]);
    }
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, Format01_WriteRead_MultipleRecords, {
    uint8_t record1[]{0x12, 0xAB, 0x00, 0xFF, 0x75};
    uint8_t record2[]{0x80, 0xFF, 0x01, 0x56, 0x57};
    uint8_t record3[]{0x2F, 0x08, 0xF0, 0x1C, 0x60};
    constexpr size_t num_bytes = sizeof(record1) / sizeof(uint8_t);
    constexpr size_t bits_per_record = 8 * num_bytes;

    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, SampleFormat::SAMPLE_FORMAT_01);
    writer->write_bytes({record1, record1 + num_bytes});
    writer->write_end();
    writer->write_bytes({record2, record2 + num_bytes});
    writer->write_end();
    writer->write_bytes({record3, record3 + num_bytes});
    writer->write_end();

    rewind(tmp);

    auto reader = MeasureRecordReader<W>::make(tmp, SampleFormat::SAMPLE_FORMAT_01, bits_per_record);

    simd_bits<W> buf(num_bytes * 8);
    ASSERT_TRUE(reader->start_and_read_entire_record(buf));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(buf.u8[i], record1[i]);
    }

    ASSERT_TRUE(reader->start_and_read_entire_record(buf));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(buf.u8[i], record2[i]);
    }

    ASSERT_TRUE(reader->start_and_read_entire_record(buf));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(buf.u8[i], record3[i]);
    }

    ASSERT_FALSE(reader->start_and_read_entire_record(buf));
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, FormatHits_WriteRead_MultipleRecords, {
    uint8_t record1[]{0x12, 0xAB, 0x00, 0xFF, 0x75};
    uint8_t record2[]{0x80, 0xFF, 0x01, 0x56, 0x57};
    uint8_t record3[]{0x2F, 0x08, 0xF0, 0x1C, 0x60};
    constexpr size_t num_bytes = sizeof(record1) / sizeof(uint8_t);
    constexpr size_t bits_per_record = 8 * num_bytes - 1;

    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, SampleFormat::SAMPLE_FORMAT_HITS);
    writer->write_bytes({record1, record1 + num_bytes});
    writer->write_end();
    writer->write_bytes({record2, record2 + num_bytes});
    writer->write_end();
    writer->write_bytes({record3, record3 + num_bytes});
    writer->write_end();

    rewind(tmp);

    auto reader = MeasureRecordReader<W>::make(tmp, SampleFormat::SAMPLE_FORMAT_HITS, bits_per_record);
    simd_bits<W> buf(num_bytes * 8);
    ASSERT_TRUE(reader->start_and_read_entire_record(buf));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(buf.u8[i], record1[i]);
    }

    ASSERT_TRUE(reader->start_and_read_entire_record(buf));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(buf.u8[i], record2[i]);
    }

    ASSERT_TRUE(reader->start_and_read_entire_record(buf));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(buf.u8[i], record3[i]);
    }

    ASSERT_FALSE(reader->start_and_read_entire_record(buf));
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, FormatDets_WriteRead_MultipleRecords, {
    uint8_t record1[]{0x12, 0xAB, 0x00, 0xFF, 0x75};
    uint8_t record2[]{0x80, 0xFF, 0x01, 0x56, 0x57};
    uint8_t record3[]{0x2F, 0x08, 0xF0, 0x1C, 0x60};
    constexpr size_t num_bytes = sizeof(record1) / sizeof(uint8_t);
    constexpr size_t bits_per_record = 8 * num_bytes - 1;

    FILE *tmp = tmpfile();
    auto writer = MeasureRecordWriter::make(tmp, SampleFormat::SAMPLE_FORMAT_DETS);
    writer->write_bytes({record1, record1 + num_bytes});
    writer->write_end();
    writer->write_bytes({record2, record2 + num_bytes});
    writer->write_end();
    writer->write_bytes({record3, record3 + num_bytes});
    writer->write_end();

    rewind(tmp);

    auto reader = MeasureRecordReader<W>::make(tmp, SampleFormat::SAMPLE_FORMAT_DETS, bits_per_record);
    simd_bits<W> buf(num_bytes * 8);
    ASSERT_TRUE(reader->start_and_read_entire_record(buf));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(buf.u8[i], record1[i]);
    }

    ASSERT_TRUE(reader->start_and_read_entire_record(buf));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(buf.u8[i], record2[i]);
    }

    ASSERT_TRUE(reader->start_and_read_entire_record(buf));
    for (size_t i = 0; i < num_bytes; ++i) {
        ASSERT_EQ(buf.u8[i], record3[i]);
    }

    ASSERT_FALSE(reader->start_and_read_entire_record(buf));
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, Format01_MultipleRecords, {
    FILE *tmp = tmpfile_with_contents("111011001\n010000000\n101100011\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader<W>::make(tmp, SampleFormat::SAMPLE_FORMAT_01, 9);
    simd_bits<W> buf(9);
    ASSERT_TRUE(reader->start_and_read_entire_record(buf));
    ASSERT_TRUE(reader->start_and_read_entire_record(buf));
    ASSERT_TRUE(reader->start_and_read_entire_record(buf));
    ASSERT_FALSE(reader->start_and_read_entire_record(buf));
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, FormatDets_MultipleShortRecords, {
    FILE *tmp = tmpfile_with_contents("shot M0\nshot M1\nshot M0\nshot\n");
    auto reader = MeasureRecordReader<W>::make(tmp, SampleFormat::SAMPLE_FORMAT_DETS, 2);

    simd_bits<W> buf(2);
    ASSERT_TRUE(reader->start_and_read_entire_record(buf));
    ASSERT_EQ(buf[0], 1);
    ASSERT_EQ(buf[1], 0);
    ASSERT_TRUE(reader->start_and_read_entire_record(buf));
    ASSERT_EQ(buf[0], 0);
    ASSERT_EQ(buf[1], 1);
    ASSERT_TRUE(reader->start_and_read_entire_record(buf));
    ASSERT_EQ(buf[0], 1);
    ASSERT_EQ(buf[1], 0);
    ASSERT_TRUE(reader->start_and_read_entire_record(buf));
    ASSERT_EQ(buf[0], 0);
    ASSERT_EQ(buf[1], 0);
    ASSERT_FALSE(reader->start_and_read_entire_record(buf));
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, FormatDets_HugeObservables, {
    FILE *tmp = tmpfile_with_contents("shot L1000\nshot D2 L999\n");
    auto reader = MeasureRecordReader<W>::make(tmp, SampleFormat::SAMPLE_FORMAT_DETS, 0, 10, 1500);

    simd_bits<64> expected(1500);
    simd_bits<W> buf(2);
    SparseShot s;
    bool b = reader->start_and_read_entire_record(s);
    ASSERT_TRUE(b);
    ASSERT_EQ(s.hits, (std::vector<uint64_t>{}));
    expected[1000] = true;
    ASSERT_EQ(s.obs_mask, expected);

    s.clear();
    b = reader->start_and_read_entire_record(s);
    ASSERT_TRUE(b);
    ASSERT_EQ(s.hits, (std::vector<uint64_t>{2}));
    expected.clear();
    expected[999] = true;
    ASSERT_EQ(s.obs_mask, expected);
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, FormatDets_MultipleResultTypes_D0L0, {
    FILE *tmp = tmpfile_with_contents("shot D0 D3 D5 L1 L2\n");
    auto reader = MeasureRecordReader<W>::make(tmp, SampleFormat::SAMPLE_FORMAT_DETS, 0, 7, 4);
    simd_bits<W> buf(11);
    ASSERT_TRUE(reader->start_and_read_entire_record(buf));
    ASSERT_EQ(buf[0], 1);
    ASSERT_EQ(buf[1], 0);
    ASSERT_EQ(buf[2], 0);
    ASSERT_EQ(buf[3], 1);
    ASSERT_EQ(buf[4], 0);
    ASSERT_EQ(buf[5], 1);
    ASSERT_EQ(buf[6], 0);
    ASSERT_EQ(buf[7], 0);
    ASSERT_EQ(buf[8], 1);
    ASSERT_EQ(buf[9], 1);
    ASSERT_EQ(buf[10], 0);

    rewind(tmp);
    reader = MeasureRecordReader<W>::make(tmp, SampleFormat::SAMPLE_FORMAT_DETS, 0, 7, 4);
    SparseShot sparse;
    reader->start_and_read_entire_record(sparse);
    ASSERT_EQ(sparse.hits, (std::vector<uint64_t>{0, 3, 5}));
    ASSERT_EQ(sparse.obs_mask.ptr_simd[0], 6);
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, Format01_InvalidInput, {
    FILE *tmp = tmpfile_with_contents("012\n");
    ASSERT_NE(tmp, nullptr);
    auto reader = MeasureRecordReader<W>::make(tmp, SampleFormat::SAMPLE_FORMAT_01, 3);
    simd_bits<W> buf(3);
    ASSERT_THROW({ reader->start_and_read_entire_record(buf); }, std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, FormatHits_InvalidInput, {
    FILE *tmp = tmpfile_with_contents("100,1\n");
    auto reader = MeasureRecordReader<W>::make(tmp, SampleFormat::SAMPLE_FORMAT_HITS, 3);
    simd_bits<W> buf(3);
    ASSERT_THROW({ reader->start_and_read_entire_record(buf); }, std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, FormatHits_InvalidInput_sparse, {
    FILE *tmp = tmpfile_with_contents("100,1\n");
    auto reader = MeasureRecordReader<W>::make(tmp, SampleFormat::SAMPLE_FORMAT_HITS, 3);
    SparseShot sparse;
    ASSERT_THROW({ reader->start_and_read_entire_record(sparse); }, std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, FormatHits_Repeated_Dense, {
    FILE *tmp = tmpfile_with_contents("1,1\n");
    auto reader = MeasureRecordReader<W>::make(tmp, SampleFormat::SAMPLE_FORMAT_HITS, 3);
    simd_bits<W> buf(3);
    ASSERT_TRUE(reader->start_and_read_entire_record(buf));
    ASSERT_EQ(buf[0], 0);
    ASSERT_EQ(buf[1], 0);
    ASSERT_EQ(buf[2], 0);
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, FormatHits_Repeated_Sparse, {
    FILE *tmp = tmpfile_with_contents("1,1\n");
    auto reader = MeasureRecordReader<W>::make(tmp, SampleFormat::SAMPLE_FORMAT_HITS, 3);
    SparseShot sparse;
    ASSERT_TRUE(reader->start_and_read_entire_record(sparse));
    ASSERT_EQ(sparse.hits, (std::vector<uint64_t>{1, 1}));
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, FormatDets_InvalidInput_Sparse, {
    FILE *tmp = tmpfile_with_contents("D2\n");
    auto r = MeasureRecordReader<W>::make(tmp, SampleFormat::SAMPLE_FORMAT_DETS, 3);
    SparseShot sparse;
    ASSERT_THROW({ r->start_and_read_entire_record(sparse); }, std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, FormatDets_InvalidInput_Dense, {
    FILE *tmp = tmpfile_with_contents("D2\n");
    auto r = MeasureRecordReader<W>::make(tmp, SampleFormat::SAMPLE_FORMAT_DETS, 3);
    simd_bits<W> buf(3);
    ASSERT_THROW({ r->start_and_read_entire_record(buf); }, std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, read_records_into_RoundTrip, {
    size_t n_shots = 100;
    size_t n_results = 512 - 8;

    auto rng = INDEPENDENT_TEST_RNG();
    auto shot_maj_data = simd_bit_table<W>::random(n_shots, n_results, rng);
    auto shot_min_data = shot_maj_data.transposed();
    for (const auto &kv : format_name_to_enum_map()) {
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
            auto reader = MeasureRecordReader<W>::make(f, format, n_results, 0, 0);
            simd_bit_table<W> read_shot_min_data(n_results, n_shots);
            size_t n = reader->read_records_into(read_shot_min_data, false);
            EXPECT_EQ(n, n_shots) << kv.second.name << " (not striped)";
            EXPECT_EQ(read_shot_min_data, shot_min_data) << kv.second.name << " (not striped)";
        }

        // Check that read shot-maj data matches written data when transposing.
        rewind(f);
        {
            auto reader = MeasureRecordReader<W>::make(f, format, n_results, 0, 0);
            simd_bit_table<W> read_shot_maj_data(n_shots, n_results);
            size_t n = reader->read_records_into(read_shot_maj_data, true);
            EXPECT_EQ(n, n_shots) << kv.second.name << " (striped)";
            EXPECT_EQ(read_shot_maj_data, shot_maj_data) << kv.second.name << " (striped)";
        }

        fclose(f);
    }
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, read_bits_into_bytes_entire_record_across_result_type, {
    FILE *f = tmpfile_with_contents("shot D1 L1");
    auto reader = MeasureRecordReader<W>::make(f, SampleFormat::SAMPLE_FORMAT_DETS, 0, 3, 3);
    simd_bit_table<W> read(6, 1);
    size_t n = reader->read_records_into(read, false);
    fclose(f);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(read[0][0], false);
    ASSERT_EQ(read[1][0], true);
    ASSERT_EQ(read[2][0], false);
    ASSERT_EQ(read[3][0], false);
    ASSERT_EQ(read[4][0], true);
    ASSERT_EQ(read[5][0], false);
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, read_r8_detection_event_data, {
    FILE *f = tmpfile();
    putc(6, f);
    putc(1, f);
    putc(4, f);
    rewind(f);
    auto reader = MeasureRecordReader<W>::make(f, SampleFormat::SAMPLE_FORMAT_R8, 0, 3, 3);
    simd_bit_table<W> read(6, 2);
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
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, read_b8_detection_event_data_full_run_together, {
    FILE *f = tmpfile();
    putc(0, f);
    putc(1, f);
    putc(2, f);
    putc(3, f);
    rewind(f);
    auto reader = MeasureRecordReader<W>::make(f, SampleFormat::SAMPLE_FORMAT_B8, 0, 27, 0);
    simd_bit_table<W> read(27, 1);
    size_t n = reader->read_records_into(read, false);
    fclose(f);
    ASSERT_EQ(n, 1);
    auto t = read.transposed();
    ASSERT_EQ(t[0].u8[0], 0);
    ASSERT_EQ(t[0].u8[1], 1);
    ASSERT_EQ(t[0].u8[2], 2);
    ASSERT_EQ(t[0].u8[3], 3);
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, start_and_read_entire_record, {
    auto rng = INDEPENDENT_TEST_RNG();
    size_t n = 512 - 8;
    size_t no = 5;
    size_t nd = n - no;

    // Compute expected data.
    simd_bits<W> test_data(n);
    biased_randomize_bits(0.1, test_data.u64, test_data.u64 + test_data.num_u64_padded(), rng);
    SparseShot sparse_test_data;
    sparse_test_data.obs_mask = simd_bits<64>(no);
    for (size_t k = 0; k < nd; k++) {
        if (test_data[k]) {
            sparse_test_data.hits.push_back(k);
        }
    }
    for (size_t k = 0; k < no; k++) {
        if (test_data[k + nd]) {
            sparse_test_data.obs_mask[k] = true;
        }
    }

    for (const auto &kv : format_name_to_enum_map()) {
        SampleFormat format = kv.second.id;
        if (format == SampleFormat::SAMPLE_FORMAT_PTB64) {
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
            auto reader = MeasureRecordReader<W>::make(f, format, 0, nd, no);

            // Check sparse record read.
            SparseShot sparse_out;
            rewind(f);
            ASSERT_TRUE(reader->start_and_read_entire_record(sparse_out));
            ASSERT_EQ(sparse_out, sparse_test_data);
            ASSERT_FALSE(reader->start_and_read_entire_record(sparse_out));

            rewind(f);
            simd_bits<W> dense_out(n);
            ASSERT_TRUE(reader->start_and_read_entire_record(dense_out));
            for (size_t k = 0; k < n; k++) {
                ASSERT_EQ(dense_out[k], test_data[k]);
            }
            ASSERT_FALSE(reader->start_and_read_entire_record(dense_out));
        }

        fclose(f);
    }
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, start_and_read_entire_record_all_zero, {
    simd_bits<W> test_data(256);
    SparseShot sparse_test_data;

    for (const auto &kv : format_name_to_enum_map()) {
        SampleFormat format = kv.second.id;
        if (format == SampleFormat::SAMPLE_FORMAT_PTB64) {
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
            auto reader = MeasureRecordReader<W>::make(f, format, 256, 0, 0);

            // Check sparse record read.
            SparseShot sparse_out;
            rewind(f);
            ASSERT_TRUE(reader->start_and_read_entire_record(sparse_out));
            ASSERT_EQ(sparse_out, sparse_test_data);
            ASSERT_FALSE(reader->start_and_read_entire_record(sparse_out));

            rewind(f);
            simd_bits<W> dense_out(256);
            ASSERT_TRUE(reader->start_and_read_entire_record(dense_out));
            ASSERT_EQ(dense_out, test_data);
            ASSERT_FALSE(reader->start_and_read_entire_record(dense_out));
        }

        fclose(f);
    }
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, start_and_read_entire_record_ptb64_dense, {
    auto rng = INDEPENDENT_TEST_RNG();
    FILE *f = tmpfile();
    auto saved1 = simd_bits<W>::random(64 * 71, rng);
    auto saved2 = simd_bits<W>::random(64 * 71, rng);
    for (size_t k = 0; k < 64 * 71 / 8; k++) {
        putc(saved1.u8[k], f);
    }
    for (size_t k = 0; k < 64 * 71 / 8; k++) {
        putc(saved2.u8[k], f);
    }
    rewind(f);

    simd_bits<W> loaded(71);
    auto reader = MeasureRecordReader<W>::make(f, SampleFormat::SAMPLE_FORMAT_PTB64, 71, 0, 0);
    for (size_t shot = 0; shot < 64; shot++) {
        ASSERT_TRUE(reader->start_and_read_entire_record(loaded));
        for (size_t m = 0; m < 71; m++) {
            ASSERT_EQ(saved1[m * 64 + shot], loaded[m]);
        }
    }
    for (size_t shot = 0; shot < 64; shot++) {
        ASSERT_TRUE(reader->start_and_read_entire_record(loaded));
        for (size_t m = 0; m < 71; m++) {
            ASSERT_EQ(saved2[m * 64 + shot], loaded[m]);
        }
    }
    ASSERT_FALSE(reader->start_and_read_entire_record(loaded));
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, start_and_read_entire_record_ptb64_sparse, {
    auto rng = INDEPENDENT_TEST_RNG();
    FILE *tmp = tmpfile();
    simd_bit_table<W> ground_truth(71, 64 * 5);
    {
        MeasureRecordBatchWriter writer(tmp, 64 * 5, SampleFormat::SAMPLE_FORMAT_PTB64);
        for (size_t k = 0; k < 71; k++) {
            ground_truth[k].randomize(64 * 5, rng);
            writer.batch_write_bit<W>(ground_truth[k]);
        }
        writer.write_end();
    }
    rewind(tmp);

    auto reader = MeasureRecordReader<W>::make(tmp, SampleFormat::SAMPLE_FORMAT_PTB64, 71, 0, 0);
    for (size_t shot = 0; shot < 64 * 5; shot++) {
        SparseShot loaded;
        ASSERT_TRUE(reader->start_and_read_entire_record(loaded));
        std::vector<uint64_t> expected;
        for (size_t m = 0; m < 71; m++) {
            if (ground_truth[m][shot]) {
                expected.push_back(m);
            }
        }
        ASSERT_EQ(loaded.hits, expected);
    }

    SparseShot discard;
    ASSERT_FALSE(reader->start_and_read_entire_record(discard));
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, read_file_data_into_shot_table_vs_write_table, {
    auto rng = INDEPENDENT_TEST_RNG();
    for (const auto &format_data : format_name_to_enum_map()) {
        SampleFormat format = format_data.second.id;
        size_t num_shots = 500;
        if (format == SampleFormat::SAMPLE_FORMAT_PTB64) {
            num_shots = 512 + 64;
        }
        size_t bits_per_shot = 1000;

        simd_bit_table<W> expected(num_shots, bits_per_shot);
        for (size_t shot = 0; shot < num_shots; shot++) {
            expected[shot].randomize(bits_per_shot, rng);
        }
        simd_bit_table<W> expected_transposed = expected.transposed();

        RaiiTempNamedFile tmp;
        FILE *f = fopen(tmp.path.c_str(), "wb");
        write_table_data<W>(f, num_shots, bits_per_shot, simd_bits<W>(0), expected_transposed, format, 'M', 'M', 0);
        fclose(f);

        f = fopen(tmp.path.c_str(), "rb");
        simd_bit_table<W> output(num_shots, bits_per_shot);
        read_file_data_into_shot_table(f, num_shots, bits_per_shot, format, 'M', output, true);
        ASSERT_EQ(getc(f), EOF) << format_data.second.name << ", not transposed";
        fclose(f);
        ASSERT_EQ(output, expected) << format_data.second.name << ", not transposed";

        f = fopen(tmp.path.c_str(), "rb");
        simd_bit_table<W> output_transposed(bits_per_shot, num_shots);
        read_file_data_into_shot_table(f, num_shots, bits_per_shot, format, 'M', output_transposed, false);
        ASSERT_EQ(getc(f), EOF) << format_data.second.name << ", yes transposed";
        fclose(f);
        ASSERT_EQ(output_transposed, expected_transposed) << format_data.second.name << ", yes transposed";
    }
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, read_windows_newlines_01, {
    FILE *f = tmpfile();
    fprintf(f, "01\r\n01\r\n");
    rewind(f);
    auto reader = MeasureRecordReader<W>::make(f, SampleFormat::SAMPLE_FORMAT_01, 2, 0, 0);
    simd_bit_table<W> read(2, 2);
    size_t n = reader->read_records_into(read, false);
    ASSERT_EQ(n, 2);
    ASSERT_EQ(read[0][0], false);
    ASSERT_EQ(read[1][0], true);
    ASSERT_EQ(read[0][1], false);
    ASSERT_EQ(read[1][1], true);
    fclose(f);
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, read_windows_newlines_hits, {
    FILE *f = tmpfile();
    fprintf(f, "3\r\n1\r\n");
    rewind(f);
    auto reader = MeasureRecordReader<W>::make(f, SampleFormat::SAMPLE_FORMAT_HITS, 4, 0, 0);
    simd_bit_table<W> read(4, 2);
    size_t n = reader->read_records_into(read, false);
    ASSERT_EQ(n, 2);
    ASSERT_EQ(read[0][0], false);
    ASSERT_EQ(read[1][0], false);
    ASSERT_EQ(read[2][0], false);
    ASSERT_EQ(read[3][0], true);
    ASSERT_EQ(read[0][1], false);
    ASSERT_EQ(read[1][1], true);
    ASSERT_EQ(read[2][1], false);
    ASSERT_EQ(read[3][1], false);
    fclose(f);
})

TEST_EACH_WORD_SIZE_W(MeasureRecordReader, read_windows_newlines_dets, {
    FILE *f = tmpfile();
    fprintf(f, "shot M3\r\n\r\n\n   shot M1\r\n\n");
    rewind(f);
    auto reader = MeasureRecordReader<W>::make(f, SampleFormat::SAMPLE_FORMAT_DETS, 4, 0, 0);
    simd_bit_table<W> read(4, 2);
    size_t n = reader->read_records_into(read, false);
    ASSERT_EQ(n, 2);
    ASSERT_EQ(read[0][0], false);
    ASSERT_EQ(read[1][0], false);
    ASSERT_EQ(read[2][0], false);
    ASSERT_EQ(read[3][0], true);
    ASSERT_EQ(read[0][1], false);
    ASSERT_EQ(read[1][1], true);
    ASSERT_EQ(read[2][1], false);
    ASSERT_EQ(read[3][1], false);
    fclose(f);
})
