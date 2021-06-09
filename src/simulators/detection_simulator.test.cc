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

#include "detection_simulator.h"

#include <gtest/gtest.h>

#include "../test_util.test.h"
#include "frame_simulator.h"

using namespace stim_internal;

static std::string rewind_read_all(FILE *f) {
    rewind(f);
    std::string result;
    while (true) {
        int c = getc(f);
        if (c == EOF) {
            fclose(f);
            return result;
        }
        result.push_back((char)c);
    }
}

TEST(DetectionSimulator, detector_samples) {
    auto circuit = Circuit::from_text(
        "X_ERROR(1) 0\n"
        "M 0\n"
        "DETECTOR rec[-1]\n");
    auto samples = detector_samples(circuit, 5, false, false, SHARED_TEST_RNG());
    ASSERT_EQ(
        samples.str(5),
        "11111\n"
        ".....\n"
        ".....\n"
        ".....\n"
        ".....");
}

TEST(DetectionSimulator, bad_detector) {
    ASSERT_THROW(
        { detector_samples(Circuit::from_text("rec[-1]"), 5, false, false, SHARED_TEST_RNG()); }, std::out_of_range);
}

TEST(DetectionSimulator, detector_samples_out) {
    auto circuit = Circuit::from_text(R"circuit(
        X_ERROR(1) 0
        M 0 1
        DETECTOR rec[-1]
        DETECTOR rec[-2]
        OBSERVABLE_INCLUDE(4) rec[-2]
    )circuit");

    FILE *tmp = tmpfile();
    detector_samples_out(circuit, 2, false, false, tmp, SAMPLE_FORMAT_DETS, SHARED_TEST_RNG());
    rewind(tmp);
    for (char c : "shot D1\nshot D1\n") {
        ASSERT_EQ(getc(tmp), c == '\0' ? EOF : c);
    }

    tmp = tmpfile();
    detector_samples_out(circuit, 2, true, false, tmp, SAMPLE_FORMAT_DETS, SHARED_TEST_RNG());
    rewind(tmp);
    for (char c : "shot L4 D1\nshot L4 D1\n") {
        ASSERT_EQ(getc(tmp), c == '\0' ? EOF : c);
    }

    tmp = tmpfile();
    detector_samples_out(circuit, 2, false, true, tmp, SAMPLE_FORMAT_DETS, SHARED_TEST_RNG());
    rewind(tmp);
    for (char c : "shot D1 L4\nshot D1 L4\n") {
        ASSERT_EQ(getc(tmp), c == '\0' ? EOF : c);
    }

    tmp = tmpfile();
    detector_samples_out(circuit, 2, false, true, tmp, SAMPLE_FORMAT_HITS, SHARED_TEST_RNG());
    rewind(tmp);
    for (char c : "1,6\n1,6\n") {
        ASSERT_EQ(getc(tmp), c == '\0' ? EOF : c);
    }
}

TEST(DetectionSimulator, stream_many_shots) {
    DebugForceResultStreamingRaii force_streaming;
    auto circuit = Circuit::from_text(R"circuit(
        X_ERROR(1) 1
        M 0 1 2
        DETECTOR rec[-1]
        DETECTOR rec[-2]
        DETECTOR rec[-3]
    )circuit");
    FILE *tmp = tmpfile();
    detector_samples_out(circuit, 2048, false, false, tmp, SAMPLE_FORMAT_01, SHARED_TEST_RNG());

    auto result = rewind_read_all(tmp);
    for (size_t k = 0; k < 2048 * 4; k += 4) {
        ASSERT_EQ(result[k], '0') << k;
        ASSERT_EQ(result[k + 1], '1') << (k + 1);
        ASSERT_EQ(result[k + 2], '0') << (k + 2);
        ASSERT_EQ(result[k + 3], '\n') << (k + 3);
    }
}

TEST(DetectionSimulator, block_results_single_shot) {
    auto circuit = Circuit::from_text(R"circuit(
        REPEAT 10000 {
            M 0
            X_ERROR(1) 0
            M 0
            R 0
            DETECTOR rec[-1] rec[-2]
            M 0
            DETECTOR rec[-1]
            DETECTOR rec[-1]
        }
    )circuit");
    FILE *tmp = tmpfile();
    detector_samples_out(circuit, 1, false, true, tmp, SAMPLE_FORMAT_01, SHARED_TEST_RNG());

    auto result = rewind_read_all(tmp);
    for (size_t k = 0; k < 30000; k += 3) {
        ASSERT_EQ(result[k], '1') << k;
        ASSERT_EQ(result[k + 1], '0') << (k + 1);
        ASSERT_EQ(result[k + 2], '0') << (k + 2);
    }
    ASSERT_EQ(result[30000], '\n');
}

TEST(DetectionSimulator, block_results_triple_shot) {
    auto circuit = Circuit::from_text(R"circuit(
        REPEAT 10000 {
            M 0
            X_ERROR(1) 0
            M 0
            R 0
            DETECTOR rec[-1] rec[-2]
            M 0
            DETECTOR rec[-1]
            DETECTOR rec[-1]
        }
    )circuit");
    FILE *tmp = tmpfile();
    detector_samples_out(circuit, 3, false, true, tmp, SAMPLE_FORMAT_01, SHARED_TEST_RNG());

    auto result = rewind_read_all(tmp);
    for (size_t rep = 0; rep < 3; rep++) {
        size_t s = rep * 30001;
        for (size_t k = 0; k < 30000; k += 3) {
            ASSERT_EQ(result[s + k], '1') << (s + k);
            ASSERT_EQ(result[s + k + 1], '0') << (s + k + 1);
            ASSERT_EQ(result[s + k + 2], '0') << (s + k + 2);
        }
        ASSERT_EQ(result[s + 30000], '\n');
    }
}

TEST(DetectionSimulator, stream_results) {
    DebugForceResultStreamingRaii force_streaming;
    auto circuit = Circuit::from_text(R"circuit(
        REPEAT 10000 {
            M 0
            X_ERROR(1) 0
            M 0
            R 0
            DETECTOR rec[-1] rec[-2]
            M 0
            DETECTOR rec[-1]
            DETECTOR rec[-1]
        }
    )circuit");
    FILE *tmp = tmpfile();
    detector_samples_out(circuit, 1, false, true, tmp, SAMPLE_FORMAT_01, SHARED_TEST_RNG());

    auto result = rewind_read_all(tmp);
    for (size_t k = 0; k < 30000; k += 3) {
        ASSERT_EQ(result[k], '1') << k;
        ASSERT_EQ(result[k + 1], '0') << (k + 1);
        ASSERT_EQ(result[k + 2], '0') << (k + 2);
    }
    ASSERT_EQ(result[30000], '\n');
}

TEST(DetectionSimulator, stream_results_triple_shot) {
    DebugForceResultStreamingRaii force_streaming;
    auto circuit = Circuit::from_text(R"circuit(
        REPEAT 10000 {
            M 0
            X_ERROR(1) 0
            M 0
            R 0
            DETECTOR rec[-1] rec[-2]
            M 0
            DETECTOR rec[-1]
            DETECTOR rec[-1]
        }
    )circuit");
    FILE *tmp = tmpfile();
    detector_samples_out(circuit, 3, false, true, tmp, SAMPLE_FORMAT_01, SHARED_TEST_RNG());

    auto result = rewind_read_all(tmp);
    for (size_t rep = 0; rep < 3; rep++) {
        size_t s = rep * 30001;
        for (size_t k = 0; k < 30000; k += 3) {
            ASSERT_EQ(result[s + k], '1') << (s + k);
            ASSERT_EQ(result[s + k + 1], '0') << (s + k + 1);
            ASSERT_EQ(result[s + k + 2], '0') << (s + k + 2);
        }
        ASSERT_EQ(result[s + 30000], '\n');
    }
}
