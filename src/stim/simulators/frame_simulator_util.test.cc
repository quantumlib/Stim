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

#include "stim/simulators/frame_simulator_util.h"

#include "gtest/gtest.h"

#include "stim/mem/simd_word.test.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

template <size_t W>
simd_bit_table<W> sample_test_detection_events(const Circuit &circuit, size_t num_shots) {
    auto rng = INDEPENDENT_TEST_RNG();
    return sample_batch_detection_events<W>(circuit, num_shots, rng).first;
}

TEST_EACH_WORD_SIZE_W(DetectionSimulator, sample_test_detection_events, {
    auto circuit = Circuit(
        "X_ERROR(1) 0\n"
        "M 0\n"
        "DETECTOR rec[-1]\n");
    auto samples = sample_test_detection_events<W>(circuit, 5);
    ASSERT_EQ(
        samples.str(5),
        "11111\n"
        ".....\n"
        ".....\n"
        ".....\n"
        ".....");
})

TEST_EACH_WORD_SIZE_W(DetectionSimulator, bad_detector, {
    ASSERT_THROW({ sample_test_detection_events<W>(Circuit("rec[-1]"), 5); }, std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(DetectionSimulator, sample_batch_detection_events_writing_results_to_disk, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto circuit = Circuit(R"circuit(
        X_ERROR(1) 0
        M 0 1
        DETECTOR rec[-1]
        DETECTOR rec[-2]
        OBSERVABLE_INCLUDE(4) rec[-2]
    )circuit");

    FILE *tmp = tmpfile();
    sample_batch_detection_events_writing_results_to_disk<W>(
        circuit, 2, false, false, tmp, SampleFormat::SAMPLE_FORMAT_DETS, rng, nullptr, SampleFormat::SAMPLE_FORMAT_01);
    ASSERT_EQ(rewind_read_close(tmp), "shot D1\nshot D1\n");

    tmp = tmpfile();
    sample_batch_detection_events_writing_results_to_disk<W>(
        circuit, 2, true, false, tmp, SampleFormat::SAMPLE_FORMAT_DETS, rng, nullptr, SampleFormat::SAMPLE_FORMAT_01);
    ASSERT_EQ(rewind_read_close(tmp), "shot L4 D1\nshot L4 D1\n");

    tmp = tmpfile();
    sample_batch_detection_events_writing_results_to_disk<W>(
        circuit, 2, false, true, tmp, SampleFormat::SAMPLE_FORMAT_DETS, rng, nullptr, SampleFormat::SAMPLE_FORMAT_01);
    ASSERT_EQ(rewind_read_close(tmp), "shot D1 L4\nshot D1 L4\n");

    tmp = tmpfile();
    sample_batch_detection_events_writing_results_to_disk<W>(
        circuit, 2, false, true, tmp, SampleFormat::SAMPLE_FORMAT_HITS, rng, nullptr, SampleFormat::SAMPLE_FORMAT_01);
    ASSERT_EQ(rewind_read_close(tmp), "1,6\n1,6\n");
})

TEST_EACH_WORD_SIZE_W(DetectionSimulator, stream_many_shots, {
    auto rng = INDEPENDENT_TEST_RNG();
    DebugForceResultStreamingRaii force_streaming;
    auto circuit = Circuit(R"circuit(
        X_ERROR(1) 1
        M 0 1 2
        DETECTOR rec[-1]
        DETECTOR rec[-2]
        DETECTOR rec[-3]
    )circuit");
    FILE *tmp = tmpfile();
    sample_batch_detection_events_writing_results_to_disk<W>(
        circuit, 2048, false, false, tmp, SampleFormat::SAMPLE_FORMAT_01, rng, nullptr, SampleFormat::SAMPLE_FORMAT_01);

    auto result = rewind_read_close(tmp);
    for (size_t k = 0; k < 2048 * 4; k += 4) {
        ASSERT_EQ(result[k], '0') << k;
        ASSERT_EQ(result[k + 1], '1') << (k + 1);
        ASSERT_EQ(result[k + 2], '0') << (k + 2);
        ASSERT_EQ(result[k + 3], '\n') << (k + 3);
    }
})

TEST_EACH_WORD_SIZE_W(DetectionSimulator, block_results_single_shot, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto circuit = Circuit(R"circuit(
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
    sample_batch_detection_events_writing_results_to_disk<W>(
        circuit, 1, false, true, tmp, SampleFormat::SAMPLE_FORMAT_01, rng, nullptr, SampleFormat::SAMPLE_FORMAT_01);

    auto result = rewind_read_close(tmp);
    for (size_t k = 0; k < 30000; k += 3) {
        ASSERT_EQ(result[k], '1') << k;
        ASSERT_EQ(result[k + 1], '0') << (k + 1);
        ASSERT_EQ(result[k + 2], '0') << (k + 2);
    }
    ASSERT_EQ(result[30000], '\n');
})

TEST_EACH_WORD_SIZE_W(DetectionSimulator, block_results_triple_shot, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto circuit = Circuit(R"circuit(
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
    sample_batch_detection_events_writing_results_to_disk<W>(
        circuit, 3, false, true, tmp, SampleFormat::SAMPLE_FORMAT_01, rng, nullptr, SampleFormat::SAMPLE_FORMAT_01);

    auto result = rewind_read_close(tmp);
    for (size_t rep = 0; rep < 3; rep++) {
        size_t s = rep * 30001;
        for (size_t k = 0; k < 30000; k += 3) {
            ASSERT_EQ(result[s + k], '1') << (s + k);
            ASSERT_EQ(result[s + k + 1], '0') << (s + k + 1);
            ASSERT_EQ(result[s + k + 2], '0') << (s + k + 2);
        }
        ASSERT_EQ(result[s + 30000], '\n');
    }
})

TEST_EACH_WORD_SIZE_W(DetectionSimulator, stream_results, {
    auto rng = INDEPENDENT_TEST_RNG();
    DebugForceResultStreamingRaii force_streaming;
    auto circuit = Circuit(R"circuit(
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

    RaiiTempNamedFile tmp;
    FILE *f = fopen(tmp.path.c_str(), "w");
    sample_batch_detection_events_writing_results_to_disk<W>(
        circuit, 1, false, true, f, SampleFormat::SAMPLE_FORMAT_01, rng, nullptr, SampleFormat::SAMPLE_FORMAT_01);
    fclose(f);

    auto result = tmp.read_contents();
    for (size_t k = 0; k < 30000; k += 3) {
        ASSERT_EQ(result[k], '1') << k;
        ASSERT_EQ(result[k + 1], '0') << (k + 1);
        ASSERT_EQ(result[k + 2], '0') << (k + 2);
    }
    ASSERT_EQ(result[30000], '\n');
})

TEST_EACH_WORD_SIZE_W(DetectionSimulator, stream_results_triple_shot, {
    auto rng = INDEPENDENT_TEST_RNG();
    DebugForceResultStreamingRaii force_streaming;
    auto circuit = Circuit(R"circuit(
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
    sample_batch_detection_events_writing_results_to_disk<W>(
        circuit, 3, false, true, tmp, SampleFormat::SAMPLE_FORMAT_01, rng, nullptr, SampleFormat::SAMPLE_FORMAT_01);

    auto result = rewind_read_close(tmp);
    for (size_t rep = 0; rep < 3; rep++) {
        size_t s = rep * 30001;
        for (size_t k = 0; k < 30000; k += 3) {
            ASSERT_EQ(result[s + k], '1') << (s + k);
            ASSERT_EQ(result[s + k + 1], '0') << (s + k + 1);
            ASSERT_EQ(result[s + k + 2], '0') << (s + k + 2);
        }
        ASSERT_EQ(result[s + 30000], '\n');
    }
})

TEST_EACH_WORD_SIZE_W(DetectionSimulator, noisy_measurement_x, {
    auto r = sample_test_detection_events<W>(
        Circuit(R"CIRCUIT(
        RX 0
        MX(0.05) 0
        MX 0
        DETECTOR rec[-2]
        DETECTOR rec[-1]
    )CIRCUIT"),
        10000);
    ASSERT_FALSE(r[1].not_zero());
    auto m1 = r[0].popcnt();
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);

    r = sample_test_detection_events<W>(
        Circuit(R"CIRCUIT(
        RX 0 1
        Z_ERROR(1) 0 1
        MX(0.05) 0 1
        MX 0 1
        DETECTOR rec[-4]
        DETECTOR rec[-3]
        DETECTOR rec[-2]
        DETECTOR rec[-1]
    )CIRCUIT"),
        5000);
    auto m2 = r[0].popcnt() + r[1].popcnt();
    ASSERT_LT(m2, 10000 - 300);
    ASSERT_GT(m2, 10000 - 700);
    ASSERT_EQ(r[2].popcnt(), 5000);
    ASSERT_EQ(r[3].popcnt(), 5000);
})

TEST_EACH_WORD_SIZE_W(DetectionSimulator, noisy_measurement_y, {
    auto r = sample_test_detection_events<W>(
        Circuit(R"CIRCUIT(
        RY 0
        MY(0.05) 0
        MY 0
        DETECTOR rec[-2]
        DETECTOR rec[-1]
    )CIRCUIT"),
        10000);
    ASSERT_FALSE(r[1].not_zero());
    auto m1 = r[0].popcnt();
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);

    r = sample_test_detection_events<W>(
        Circuit(R"CIRCUIT(
        RY 0 1
        Z_ERROR(1) 0 1
        MY(0.05) 0 1
        MY 0 1
        DETECTOR rec[-4]
        DETECTOR rec[-3]
        DETECTOR rec[-2]
        DETECTOR rec[-1]
    )CIRCUIT"),
        5000);
    auto m2 = r[0].popcnt() + r[1].popcnt();
    ASSERT_LT(m2, 10000 - 300);
    ASSERT_GT(m2, 10000 - 700);
    ASSERT_EQ(r[2].popcnt(), 5000);
    ASSERT_EQ(r[3].popcnt(), 5000);
})

TEST_EACH_WORD_SIZE_W(DetectionSimulator, noisy_measurement_z, {
    auto r = sample_test_detection_events<W>(
        Circuit(R"CIRCUIT(
        RZ 0
        MZ(0.05) 0
        MZ 0
        DETECTOR rec[-2]
        DETECTOR rec[-1]
    )CIRCUIT"),
        10000);
    ASSERT_FALSE(r[1].not_zero());
    auto m1 = r[0].popcnt();
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);

    r = sample_test_detection_events<W>(
        Circuit(R"CIRCUIT(
        RZ 0 1
        X_ERROR(1) 0 1
        MZ(0.05) 0 1
        MZ 0 1
        DETECTOR rec[-4]
        DETECTOR rec[-3]
        DETECTOR rec[-2]
        DETECTOR rec[-1]
    )CIRCUIT"),
        5000);
    auto m2 = r[0].popcnt() + r[1].popcnt();
    ASSERT_LT(m2, 10000 - 300);
    ASSERT_GT(m2, 10000 - 700);
    ASSERT_EQ(r[2].popcnt(), 5000);
    ASSERT_EQ(r[3].popcnt(), 5000);
})

TEST_EACH_WORD_SIZE_W(DetectionSimulator, noisy_measure_reset_x, {
    auto r = sample_test_detection_events<W>(
        Circuit(R"CIRCUIT(
        RX 0
        MRX(0.05) 0
        MRX 0
        DETECTOR rec[-2]
        DETECTOR rec[-1]
    )CIRCUIT"),
        10000);
    ASSERT_FALSE(r[1].not_zero());
    auto m1 = r[0].popcnt();
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);

    r = sample_test_detection_events<W>(
        Circuit(R"CIRCUIT(
        RX 0 1
        Z_ERROR(1) 0 1
        MRX(0.05) 0 1
        MRX 0 1
        DETECTOR rec[-4]
        DETECTOR rec[-3]
        DETECTOR rec[-2]
        DETECTOR rec[-1]
    )CIRCUIT"),
        5000);
    auto m2 = r[0].popcnt() + r[1].popcnt();
    ASSERT_LT(m2, 10000 - 300);
    ASSERT_GT(m2, 10000 - 700);
    ASSERT_EQ(r[2].popcnt(), 0);
    ASSERT_EQ(r[3].popcnt(), 0);
})

TEST_EACH_WORD_SIZE_W(DetectionSimulator, noisy_measure_reset_y, {
    auto r = sample_test_detection_events<W>(
        Circuit(R"CIRCUIT(
        RY 0
        MRY(0.05) 0
        MRY 0
        DETECTOR rec[-2]
        DETECTOR rec[-1]
    )CIRCUIT"),
        10000);
    ASSERT_FALSE(r[1].not_zero());
    auto m1 = r[0].popcnt();
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);

    r = sample_test_detection_events<W>(
        Circuit(R"CIRCUIT(
        RY 0 1
        Z_ERROR(1) 0 1
        MRY(0.05) 0 1
        MRY 0 1
        DETECTOR rec[-4]
        DETECTOR rec[-3]
        DETECTOR rec[-2]
        DETECTOR rec[-1]
    )CIRCUIT"),
        5000);
    auto m2 = r[0].popcnt() + r[1].popcnt();
    ASSERT_LT(m2, 10000 - 300);
    ASSERT_GT(m2, 10000 - 700);
    ASSERT_EQ(r[2].popcnt(), 0);
    ASSERT_EQ(r[3].popcnt(), 0);
})

TEST_EACH_WORD_SIZE_W(DetectionSimulator, noisy_measure_reset_z, {
    auto r = sample_test_detection_events<W>(
        Circuit(R"CIRCUIT(
        RZ 0
        MRZ(0.05) 0
        MRZ 0
        DETECTOR rec[-2]
        DETECTOR rec[-1]
    )CIRCUIT"),
        10000);
    ASSERT_FALSE(r[1].not_zero());
    auto m1 = r[0].popcnt();
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);

    r = sample_test_detection_events<W>(
        Circuit(R"CIRCUIT(
        RZ 0 1
        X_ERROR(1) 0 1
        MRZ(0.05) 0 1
        MRZ 0 1
        DETECTOR rec[-4]
        DETECTOR rec[-3]
        DETECTOR rec[-2]
        DETECTOR rec[-1]
    )CIRCUIT"),
        5000);
    auto m2 = r[0].popcnt() + r[1].popcnt();
    ASSERT_LT(m2, 10000 - 300);
    ASSERT_GT(m2, 10000 - 700);
    ASSERT_EQ(r[2].popcnt(), 0);
    ASSERT_EQ(r[3].popcnt(), 0);
})

TEST_EACH_WORD_SIZE_W(DetectionSimulator, obs_data, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto circuit = Circuit(R"circuit(
        REPEAT 399 {
            X_ERROR(1) 0
            MR 0
            DETECTOR rec[-1]
        }
        REPEAT 600 {
            MR 0
            DETECTOR rec[-1]
            OBSERVABLE_INCLUDE(0) rec[-1]
        }
        X_ERROR(1) 0
        MR 0
        OBSERVABLE_INCLUDE(0) rec[-1]
        OBSERVABLE_INCLUDE(1) rec[-1]
        MR 0
        OBSERVABLE_INCLUDE(2) rec[-1]
    )circuit");
    FILE *det_tmp = tmpfile();
    FILE *obs_tmp = tmpfile();
    sample_batch_detection_events_writing_results_to_disk<W>(
        circuit,
        1001,
        false,
        false,
        det_tmp,
        SampleFormat::SAMPLE_FORMAT_B8,
        rng,
        obs_tmp,
        SampleFormat::SAMPLE_FORMAT_B8);

    auto det_saved = rewind_read_close(det_tmp);
    auto obs_saved = rewind_read_close(obs_tmp);
    ASSERT_EQ(det_saved.size(), 125 * 1001);
    ASSERT_EQ(obs_saved.size(), 1 * 1001);
    for (size_t k = 0; k < det_saved.size(); k++) {
        for (size_t b = 0; b < 8; b++) {
            size_t det_index = (k % 125) * 8 + b;
            bool bit = (((uint8_t)det_saved[k]) >> b) & 1;
            ASSERT_EQ(bit, det_index < 399) << k;
        }
    }
    for (size_t k = 0; k < obs_saved.size(); k++) {
        ASSERT_EQ(obs_saved[k], 0x3);
    }
})
