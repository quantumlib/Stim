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

#include "stim/simulators/measurements_to_detection_events.h"

#include <gtest/gtest.h>

#include "stim/test_util.test.h"

using namespace stim;

TEST(measurements_to_detection_events, single_detector) {
    simd_bit_table measurement_data(256, 256);
    simd_bit_table converted(256, 256);

    // Matches false expectation.
    converted = measurements_to_detection_events(
        measurement_data,
        Circuit(R"CIRCUIT(
            M 0
            DETECTOR rec[-1]
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted[0][0], false);

    // Violates true expectation.
    converted = measurements_to_detection_events(
        measurement_data,
        Circuit(R"CIRCUIT(
            M !0
            DETECTOR rec[-1]
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted[0][0], true);
    converted = measurements_to_detection_events(
        measurement_data,
        Circuit(R"CIRCUIT(
            X 0
            M 0
            DETECTOR rec[-1]
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted[0][0], true);

    // Violates false expectation.
    measurement_data[0][0] = true;
    converted = measurements_to_detection_events(
        measurement_data,
        Circuit(R"CIRCUIT(
            M 0
            DETECTOR rec[-1]
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted[0][0], true);

    // Matches true expectation.
    converted = measurements_to_detection_events(
        measurement_data,
        Circuit(R"CIRCUIT(
            M !0
            DETECTOR rec[-1]
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted[0][0], false);
    converted = measurements_to_detection_events(
        measurement_data,
        Circuit(R"CIRCUIT(
            X 0
            M 0
            DETECTOR rec[-1]
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted[0][0], false);

    // Indexing.
    converted = measurements_to_detection_events(
        measurement_data,
        Circuit(R"CIRCUIT(
            M 0 1
            DETECTOR rec[-1]
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted[0][0], false);
    converted = measurements_to_detection_events(
        measurement_data,
        Circuit(R"CIRCUIT(
            M 0 1
            DETECTOR rec[-2]
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted[0][0], true);
    converted = measurements_to_detection_events(
        measurement_data,
        Circuit(R"CIRCUIT(
            M 0 1
            DETECTOR rec[-1] rec[-2]
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted[0][0], true);
    converted = measurements_to_detection_events(
        measurement_data,
        Circuit(R"CIRCUIT(
            M 0 1
            DETECTOR
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted[0][0], false);
}

TEST(measurements_to_detection_events, empty_cases) {
    simd_bit_table measurement_data(256, 256);
    simd_bit_table converted(256, 256);

    converted = measurements_to_detection_events(
        measurement_data,
        Circuit(R"CIRCUIT(
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted.num_major_bits_padded(), 0);
    ASSERT_EQ(converted.num_minor_bits_padded(), 256);

    converted = measurements_to_detection_events(
        measurement_data,
        Circuit(R"CIRCUIT(
            X 0
            M 0
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted.num_major_bits_padded(), 0);
    ASSERT_EQ(converted.num_minor_bits_padded(), 256);
}

TEST(measurements_to_detection_events, big_shots) {
    simd_bit_table measurement_data(256, 512);
    simd_bit_table converted(256, 512);

    converted = measurements_to_detection_events(
        measurement_data,
        Circuit(R"CIRCUIT(
            M 0 !1
            REPEAT 50 {
                DETECTOR rec[-2]
                DETECTOR rec[-1]
            }
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted[0].popcnt(), 0);
    ASSERT_EQ(converted[1].popcnt(), 512);
    ASSERT_EQ(converted[2].popcnt(), 0);
    ASSERT_EQ(converted[3].popcnt(), 512);
    ASSERT_EQ(converted[98].popcnt(), 0);
    ASSERT_EQ(converted[99].popcnt(), 512);
    ASSERT_EQ(converted[100].popcnt(), 0);
    ASSERT_EQ(converted[101].popcnt(), 0);
}

TEST(measurements_to_detection_events, big_data) {
    simd_bit_table measurement_data(512, 256);
    simd_bit_table converted(512, 256);

    converted = measurements_to_detection_events(
        measurement_data,
        Circuit(R"CIRCUIT(
            M 0 !1
            REPEAT 200 {
                DETECTOR rec[-2]
                DETECTOR rec[-1]
            }
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted[0].popcnt(), 0);
    ASSERT_EQ(converted[1].popcnt(), 256);
    ASSERT_EQ(converted[2].popcnt(), 0);
    ASSERT_EQ(converted[3].popcnt(), 256);
    ASSERT_EQ(converted[398].popcnt(), 0);
    ASSERT_EQ(converted[399].popcnt(), 256);
    ASSERT_EQ(converted[400].popcnt(), 0);
    ASSERT_EQ(converted[401].popcnt(), 0);
}

TEST(measurements_to_detection_events, append_observables) {
    simd_bit_table measurement_data(256, 256);
    simd_bit_table converted(256, 256);
    size_t min_bits = sizeof(simd_word) * 8;

    // Appended.
    converted = measurements_to_detection_events(
        measurement_data,
        Circuit(R"CIRCUIT(
            X 0
            M 0
            OBSERVABLE_INCLUDE(9) rec[-1]
        )CIRCUIT"),
        true,
        false);
    ASSERT_EQ(converted.num_major_bits_padded(), min_bits);
    ASSERT_EQ(converted.num_minor_bits_padded(), min_bits);
    ASSERT_EQ(converted[0][0], 0);
    ASSERT_EQ(converted[1][0], 0);
    ASSERT_EQ(converted[9][0], 1);
    ASSERT_EQ(converted[11][0], 0);
    converted = measurements_to_detection_events(
        measurement_data,
        Circuit(R"CIRCUIT(
            X 0
            M 0
            DETECTOR rec[-1]
            OBSERVABLE_INCLUDE(9) rec[-1]
            DETECTOR rec[-1]
        )CIRCUIT"),
        true,
        false);
    ASSERT_EQ(converted.num_major_bits_padded(), min_bits);
    ASSERT_EQ(converted.num_minor_bits_padded(), min_bits);
    ASSERT_EQ(converted[0][0], 1);
    ASSERT_EQ(converted[1][0], 1);
    ASSERT_EQ(converted[9][0], 0);
    ASSERT_EQ(converted[11][0], 1);

    // Not appended.
    converted = measurements_to_detection_events(
        measurement_data,
        Circuit(R"CIRCUIT(
            X 0
            M 0
            OBSERVABLE_INCLUDE(9) rec[-1]
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted.num_major_bits_padded(), 0);
    ASSERT_EQ(converted.num_minor_bits_padded(), min_bits);
    converted = measurements_to_detection_events(
        measurement_data,
        Circuit(R"CIRCUIT(
            X 0
            M 0
            DETECTOR rec[-1]
            OBSERVABLE_INCLUDE(9) rec[-1]
            DETECTOR rec[-1]
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted[0][0], 1);
    ASSERT_EQ(converted[1][0], 1);
    ASSERT_EQ(converted[9][0], 0);
    ASSERT_EQ(converted[11][0], 0);
}

TEST(measurements_to_detection_events, file_01_to_dets_no_obs) {
    FILE *in = tmpfile();
    fprintf(in, "%s", "0\n0\n1\n");
    rewind(in);
    FILE *out = tmpfile();

    measurements_to_detection_events(
        in,
        SampleFormat::SAMPLE_FORMAT_01,
        out,
        SampleFormat::SAMPLE_FORMAT_DETS,
        Circuit(R"CIRCUIT(
            X 0
            M 0
            DETECTOR rec[-1]
            OBSERVABLE_INCLUDE(9) rec[-1]
            DETECTOR
            DETECTOR rec[-1]
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(rewind_read_close(out), "shot D0 D2\nshot D0 D2\nshot\n");
    fclose(in);
}

TEST(measurements_to_detection_events, file_01_to_dets_yes_obs) {
    FILE *in = tmpfile();
    fprintf(in, "%s", "0\n0\n1\n");
    rewind(in);
    FILE *out = tmpfile();

    measurements_to_detection_events(
        in,
        SampleFormat::SAMPLE_FORMAT_01,
        out,
        SampleFormat::SAMPLE_FORMAT_DETS,
        Circuit(R"CIRCUIT(
            X 0
            M 0
            DETECTOR rec[-1]
            OBSERVABLE_INCLUDE(9) rec[-1]
            DETECTOR
            DETECTOR rec[-1]
        )CIRCUIT"),
        true,
        false);
    fclose(in);
    ASSERT_EQ(rewind_read_close(out), "shot D0 D2 L9\nshot D0 D2 L9\nshot\n");
}

TEST(measurements_to_detection_events, many_shots) {
    FILE *in = tmpfile();
    std::string expected;
    for (size_t k = 0; k < 500; k++) {
        fprintf(in, "%s", "0\n1\n");
        expected += "shot D0 D1\nshot\n";
    }
    rewind(in);
    FILE *out = tmpfile();

    measurements_to_detection_events(
        in,
        SampleFormat::SAMPLE_FORMAT_01,
        out,
        SampleFormat::SAMPLE_FORMAT_DETS,
        Circuit(R"CIRCUIT(
            X 0
            M 0
            DETECTOR rec[-1]
            DETECTOR rec[-1]
        )CIRCUIT"),
        true,
        false);
    fclose(in);
    ASSERT_EQ(rewind_read_close(out), expected);
}

TEST(measurements_to_detection_events, many_measurements_and_detectors) {
    FILE *in = tmpfile();
    std::string expected = "shot";
    for (size_t k = 0; k < 500; k++) {
        fprintf(in, "%s", "01");
        expected += " D" + std::to_string(2 * k + 1);
    }
    expected += "\n";
    fprintf(in, "%s", "\n");
    rewind(in);
    FILE *out = tmpfile();

    measurements_to_detection_events(
        in,
        SampleFormat::SAMPLE_FORMAT_01,
        out,
        SampleFormat::SAMPLE_FORMAT_DETS,
        Circuit(R"CIRCUIT(
            REPEAT 500 {
                M 0 1
                DETECTOR rec[-2]
                DETECTOR rec[-1]
            }
        )CIRCUIT"),
        true,
        false);
    fclose(in);
    ASSERT_EQ(rewind_read_close(out), expected);
}

TEST(measurements_to_detection_events, file_01_to_01_yes_obs) {
    FILE *in = tmpfile();
    fprintf(in, "%s", "0\n0\n1\n");
    rewind(in);
    FILE *out = tmpfile();

    measurements_to_detection_events(
        in,
        SampleFormat::SAMPLE_FORMAT_01,
        out,
        SampleFormat::SAMPLE_FORMAT_01,
        Circuit(R"CIRCUIT(
            X 0
            M 0
            DETECTOR rec[-1]
            OBSERVABLE_INCLUDE(9) rec[-1]
            DETECTOR
            DETECTOR rec[-1]
        )CIRCUIT"),
        true,
        false);
    fclose(in);
    ASSERT_EQ(rewind_read_close(out), "1010000000001\n1010000000001\n0000000000000\n");
}
