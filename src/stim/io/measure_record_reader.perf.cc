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

#include "stim/io/measure_record_reader.h"

#include "stim/io/measure_record_writer.h"
#include "stim/perf.perf.h"
#include "stim/util_bot/probability_util.h"

using namespace stim;

template <size_t n, size_t denom, SampleFormat format>
void dense_reader_benchmark(double goal_micros) {
    double p = 1 / (double)denom;
    FILE *f = tmpfile();
    {
        auto writer = MeasureRecordWriter::make(f, format);
        simd_bits<MAX_BITWORD_WIDTH> data(n);
        std::mt19937_64 rng(0);
        biased_randomize_bits(p, data.u64, data.u64 + (n >> 6), rng);
        writer->write_bytes({data.u8, data.u8 + (n >> 3)});
        writer->write_end();
    }

    auto reader = MeasureRecordReader<MAX_BITWORD_WIDTH>::make(f, format, n, 0, 0);
    simd_bits<MAX_BITWORD_WIDTH> buffer(n);
    benchmark_go([&]() {
        rewind(f);
        reader->start_and_read_entire_record(buffer);
    })
        .goal_micros(goal_micros)
        .show_rate("Bits", n);
    if (!buffer.not_zero()) {
        std::cerr << "data dependence!\n";
    }
    fclose(f);
}

template <size_t n, size_t denom, SampleFormat format>
void sparse_reader_benchmark(double goal_micros) {
    double p = 1 / (double)denom;
    FILE *f = tmpfile();
    {
        auto writer = MeasureRecordWriter::make(f, format);
        simd_bits<MAX_BITWORD_WIDTH> data(n);
        std::mt19937_64 rng(0);
        biased_randomize_bits(p, data.u64, data.u64 + (n >> 6), rng);
        writer->write_bytes({data.u8, data.u8 + (n >> 3)});
        writer->write_end();
    }

    auto reader = MeasureRecordReader<MAX_BITWORD_WIDTH>::make(f, format, n, 0, 0);
    SparseShot buffer;
    buffer.hits.reserve((size_t)ceil(n * p * 1.1));
    benchmark_go([&]() {
        rewind(f);
        buffer.clear();
        reader->start_and_read_entire_record(buffer);
    })
        .goal_micros(goal_micros)
        .show_rate("Pops", n * p);
    if (buffer.hits.empty()) {
        std::cerr << "data dependence!\n";
    }
    fclose(f);
}

BENCHMARK(read_01_dense_per10) {
    dense_reader_benchmark<10000, 10, SampleFormat::SAMPLE_FORMAT_01>(60);
}
BENCHMARK(read_01_sparse_per10) {
    sparse_reader_benchmark<10000, 10, SampleFormat::SAMPLE_FORMAT_01>(45);
}

BENCHMARK(read_b8_dense_per10) {
    dense_reader_benchmark<10000, 10, SampleFormat::SAMPLE_FORMAT_B8>(0.65);
}
BENCHMARK(read_b8_sparse_per10) {
    sparse_reader_benchmark<10000, 10, SampleFormat::SAMPLE_FORMAT_B8>(6);
}

BENCHMARK(read_hits_dense_per10) {
    dense_reader_benchmark<10000, 10, SampleFormat::SAMPLE_FORMAT_HITS>(16);
}
BENCHMARK(read_hits_dense_per100) {
    dense_reader_benchmark<10000, 100, SampleFormat::SAMPLE_FORMAT_HITS>(2.1);
}
BENCHMARK(read_hits_sparse_per10) {
    sparse_reader_benchmark<10000, 10, SampleFormat::SAMPLE_FORMAT_HITS>(15);
}
BENCHMARK(read_hits_sparse_per100) {
    sparse_reader_benchmark<10000, 100, SampleFormat::SAMPLE_FORMAT_HITS>(2.2);
}

BENCHMARK(read_dets_dense_per10) {
    dense_reader_benchmark<10000, 10, SampleFormat::SAMPLE_FORMAT_DETS>(23);
}
BENCHMARK(read_dets_dense_per100) {
    dense_reader_benchmark<10000, 100, SampleFormat::SAMPLE_FORMAT_DETS>(3.0);
}
BENCHMARK(read_dets_sparse_per10) {
    sparse_reader_benchmark<10000, 10, SampleFormat::SAMPLE_FORMAT_DETS>(23);
}
BENCHMARK(read_dets_sparse_per100) {
    sparse_reader_benchmark<10000, 100, SampleFormat::SAMPLE_FORMAT_DETS>(3.0);
}

BENCHMARK(read_r8_dense_per10) {
    dense_reader_benchmark<10000, 10, SampleFormat::SAMPLE_FORMAT_R8>(5);
}
BENCHMARK(read_r8_dense_per100) {
    dense_reader_benchmark<10000, 100, SampleFormat::SAMPLE_FORMAT_R8>(1.3);
}
BENCHMARK(read_r8_sparse_per10) {
    sparse_reader_benchmark<10000, 10, SampleFormat::SAMPLE_FORMAT_R8>(3.5);
}
BENCHMARK(read_r8_sparse_per100) {
    sparse_reader_benchmark<10000, 100, SampleFormat::SAMPLE_FORMAT_R8>(1.0);
}
