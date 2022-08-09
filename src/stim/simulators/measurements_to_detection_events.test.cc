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

#include <stim/gen/gen_surface_code.h>

#include "gtest/gtest.h"

#include "stim/test_util.test.h"

using namespace stim;

TEST(measurements_to_detection_events, single_detector_no_sweep_data) {
    simd_bit_table<MAX_BITWORD_WIDTH> measurement_data(1, 256);
    simd_bit_table<MAX_BITWORD_WIDTH> sweep_data(0, 256);
    simd_bit_table<MAX_BITWORD_WIDTH> converted(1, 256);

    // Matches false expectation.
    ASSERT_EQ(
        measurements_to_detection_events(
            measurement_data,
            sweep_data,
            Circuit(R"CIRCUIT(
                M 0
                DETECTOR rec[-1]
            )CIRCUIT"),
            false,
            false),
        simd_bit_table<MAX_BITWORD_WIDTH>::from_text("0", 1, 256));

    // Violates true expectation.
    converted = measurements_to_detection_events(
        measurement_data,
        sweep_data,
        Circuit(R"CIRCUIT(
            M !0
            DETECTOR rec[-1]
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted[0][0], true);
    converted = measurements_to_detection_events(
        measurement_data,
        sweep_data,
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
        sweep_data,
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
        sweep_data,
        Circuit(R"CIRCUIT(
            M !0
            DETECTOR rec[-1]
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted[0][0], false);
    converted = measurements_to_detection_events(
        measurement_data,
        sweep_data,
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
        sweep_data,
        Circuit(R"CIRCUIT(
            M 0 1
            DETECTOR rec[-1]
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted[0][0], false);
    converted = measurements_to_detection_events(
        measurement_data,
        sweep_data,
        Circuit(R"CIRCUIT(
            M 0 1
            DETECTOR rec[-2]
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted[0][0], true);
    converted = measurements_to_detection_events(
        measurement_data,
        sweep_data,
        Circuit(R"CIRCUIT(
            M 0 1
            DETECTOR rec[-1] rec[-2]
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted[0][0], true);
    converted = measurements_to_detection_events(
        measurement_data,
        sweep_data,
        Circuit(R"CIRCUIT(
            M 0 1
            DETECTOR
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted[0][0], false);
}

TEST(measurements_to_detection_events, sweep_data) {
    auto expected = simd_bit_table<MAX_BITWORD_WIDTH>::from_text(R"DETECTOR_DATA(
            ..1.....1
            111....1.
            .._1..1..
            ..1..111.
            ..1.1..11
            ..11.....
            11_......
            .1_1.....
            1.1...11.
            ..1....11
            11_1..1_1
        )DETECTOR_DATA");
    for (size_t k = 11; k < expected.num_major_bits_padded(); k++) {
        expected[k][2] = true;
    }

    ASSERT_EQ(
        measurements_to_detection_events(
            simd_bit_table<MAX_BITWORD_WIDTH>::from_text(R"MEASUREMENT_DATA(
                .........1
                ........1.
                .......1..
                ......1...
                .....1....
                ....1.....
                ...1......
                ..1.......
                .1........
                1.........
                ..........
            )MEASUREMENT_DATA")
                .transposed(),
            simd_bit_table<MAX_BITWORD_WIDTH>::from_text(R"SWEEP_DATA(
                ....
                1...
                .1..
                ..1.
                ...1
                ....
                1...
                .1..
                ..1.
                ...1
                1111
            )SWEEP_DATA")
                .transposed(),
            Circuit(R"CIRCUIT(
                CNOT sweep[0] 1 sweep[0] 2
                CNOT sweep[1] 3 sweep[1] 4
                CNOT sweep[3] 8 sweep[3] 9
                CNOT sweep[2] 8 sweep[2] 7
                X 3
                X_ERROR(1) 4
                M 0 1 2 3 4 5 6 7 8 9
                DETECTOR rec[-9]
                DETECTOR rec[-8]
                DETECTOR rec[-7]
                DETECTOR rec[-6]
                DETECTOR rec[-5]
                DETECTOR rec[-4]
                DETECTOR rec[-3]
                DETECTOR rec[-2]
                DETECTOR rec[-1]
            )CIRCUIT"),
            false,
            false)
            .transposed(),
        expected);
}

TEST(measurements_to_detection_events, empty_cases) {
    simd_bit_table<MAX_BITWORD_WIDTH> measurement_data(256, 256);
    simd_bit_table<MAX_BITWORD_WIDTH> converted(256, 256);
    simd_bit_table<MAX_BITWORD_WIDTH> sweep_data(0, 256);

    converted = measurements_to_detection_events(
        measurement_data,
        sweep_data,
        Circuit(R"CIRCUIT(
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted.num_major_bits_padded(), 0);
    ASSERT_EQ(converted.num_minor_bits_padded(), 256);

    converted = measurements_to_detection_events(
        measurement_data,
        sweep_data,
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
    simd_bit_table<MAX_BITWORD_WIDTH> measurement_data(256, 512);
    simd_bit_table<MAX_BITWORD_WIDTH> converted(256, 512);
    simd_bit_table<MAX_BITWORD_WIDTH> sweep_data(0, 512);

    converted = measurements_to_detection_events(
        measurement_data,
        sweep_data,
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
    simd_bit_table<MAX_BITWORD_WIDTH> measurement_data(512, 256);
    simd_bit_table<MAX_BITWORD_WIDTH> converted(512, 256);
    simd_bit_table<MAX_BITWORD_WIDTH> sweep_data(0, 256);

    converted = measurements_to_detection_events(
        measurement_data,
        sweep_data,
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
    simd_bit_table<MAX_BITWORD_WIDTH> measurement_data(256, 256);
    simd_bit_table<MAX_BITWORD_WIDTH> sweep_data(0, 256);
    simd_bit_table<MAX_BITWORD_WIDTH> converted(256, 256);
    size_t min_bits = sizeof(simd_word) * 8;

    // Appended.
    converted = measurements_to_detection_events(
        measurement_data,
        sweep_data,
        Circuit(R"CIRCUIT(
            X 0
            M 0
            OBSERVABLE_INCLUDE(9) rec[-1]
        )CIRCUIT"),
        true,
        false);
    ASSERT_EQ(converted.num_major_bits_padded(), min_bits);
    ASSERT_EQ(converted.num_minor_bits_padded(), 256);
    ASSERT_EQ(converted[0][0], 0);
    ASSERT_EQ(converted[1][0], 0);
    ASSERT_EQ(converted[9][0], 1);
    ASSERT_EQ(converted[11][0], 0);
    converted = measurements_to_detection_events(
        measurement_data,
        sweep_data,
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
    ASSERT_EQ(converted.num_minor_bits_padded(), 256);
    ASSERT_EQ(converted[0][0], 1);
    ASSERT_EQ(converted[1][0], 1);
    ASSERT_EQ(converted[9][0], 0);
    ASSERT_EQ(converted[11][0], 1);

    // Not appended.
    converted = measurements_to_detection_events(
        measurement_data,
        sweep_data,
        Circuit(R"CIRCUIT(
            X 0
            M 0
            OBSERVABLE_INCLUDE(9) rec[-1]
        )CIRCUIT"),
        false,
        false);
    ASSERT_EQ(converted.num_major_bits_padded(), 0);
    ASSERT_EQ(converted.num_minor_bits_padded(), 256);
    converted = measurements_to_detection_events(
        measurement_data,
        sweep_data,
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

    stream_measurements_to_detection_events(
        in,
        SampleFormat::SAMPLE_FORMAT_01,
        nullptr,
        (SampleFormat)0,
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
        false,
        nullptr,
        SampleFormat::SAMPLE_FORMAT_01);
    ASSERT_EQ(rewind_read_close(out), "shot D0 D2\nshot D0 D2\nshot\n");
    fclose(in);
}

TEST(measurements_to_detection_events, file_01_to_dets_yes_obs) {
    FILE *in = tmpfile();
    fprintf(in, "%s", "0\n0\n1\n");
    rewind(in);
    FILE *out = tmpfile();

    stream_measurements_to_detection_events(
        in,
        SampleFormat::SAMPLE_FORMAT_01,
        nullptr,
        (SampleFormat)0,
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
        false,
        nullptr,
        SampleFormat::SAMPLE_FORMAT_01);
    fclose(in);
    ASSERT_EQ(rewind_read_close(out), "shot D0 D2 L9\nshot D0 D2 L9\nshot\n");
}

TEST(measurements_to_detection_events, with_error_propagation) {
    FILE *in = tmpfile();
    fprintf(
        in,
        "%s",
        "00\n"
        "00\n"
        "00\n"
        "00\n"
        "00\n"
        "00\n"

        "01\n"
        "01\n"

        "11\n"
        "11\n");
    rewind(in);
    FILE *in_sweep_bits = tmpfile();
    fprintf(
        in_sweep_bits,
        "%s",
        "0000\n"
        "1000\n"
        "0100\n"
        "0010\n"
        "0001\n"
        "1111\n"

        "0000\n"
        "1111\n"

        "0000\n"
        "1111\n");
    rewind(in_sweep_bits);
    FILE *out = tmpfile();

    stream_measurements_to_detection_events(
        in,
        SampleFormat::SAMPLE_FORMAT_01,
        in_sweep_bits,
        SampleFormat::SAMPLE_FORMAT_01,
        out,
        SampleFormat::SAMPLE_FORMAT_DETS,
        Circuit(R"CIRCUIT(
            CX sweep[0] 0
            CX sweep[1] 1
            CZ sweep[2] 0
            CZ sweep[3] 1
            H 0 1
            CZ 0 1
            M 0
            MX 1
            DETECTOR rec[-1] rec[-2]
        )CIRCUIT"),
        true,
        false,
        nullptr,
        SampleFormat::SAMPLE_FORMAT_01);
    fclose(in);
    fclose(in_sweep_bits);
    ASSERT_EQ(
        rewind_read_close(out),
        "shot\n"     // No error no flip.
        "shot\n"     // X0 doesn't flip.
        "shot D0\n"  // X1 does flip.
        "shot\n"     // Z0 doesn't flip.
        "shot\n"     // Z1 doesn't flip.
        "shot D0\n"  // All together flips.

        "shot D0\n"  // One excited measurement causes a detection event.
        "shot\n"     // All together restores.

        "shot\n"     // Two excited measurements is not a detection.
        "shot D0\n"  // All together still flips.
    );
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

    stream_measurements_to_detection_events(
        in,
        SampleFormat::SAMPLE_FORMAT_01,
        nullptr,
        (SampleFormat)0,
        out,
        SampleFormat::SAMPLE_FORMAT_DETS,
        Circuit(R"CIRCUIT(
            X 0
            M 0
            DETECTOR rec[-1]
            DETECTOR rec[-1]
        )CIRCUIT"),
        true,
        false,
        nullptr,
        SampleFormat::SAMPLE_FORMAT_01);
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

    stream_measurements_to_detection_events(
        in,
        SampleFormat::SAMPLE_FORMAT_01,
        nullptr,
        (SampleFormat)0,
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
        false,
        nullptr,
        SampleFormat::SAMPLE_FORMAT_01);
    fclose(in);
    ASSERT_EQ(rewind_read_close(out), expected);
}

TEST(measurements_to_detection_events, file_01_to_01_yes_obs) {
    FILE *in = tmpfile();
    fprintf(in, "%s", "0\n0\n1\n");
    rewind(in);
    FILE *out = tmpfile();

    stream_measurements_to_detection_events(
        in,
        SampleFormat::SAMPLE_FORMAT_01,
        nullptr,
        (SampleFormat)0,
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
        false,
        nullptr,
        SampleFormat::SAMPLE_FORMAT_01);
    fclose(in);
    ASSERT_EQ(rewind_read_close(out), "1010000000001\n1010000000001\n0000000000000\n");
}

TEST(measurements_to_detection_events, empty_input_01_empty_sweep_b8) {
    FILE *in = tmpfile();
    FILE *sweep = tmpfile();
    FILE *out = tmpfile();

    stream_measurements_to_detection_events(
        in,
        SampleFormat::SAMPLE_FORMAT_01,
        sweep,
        SampleFormat::SAMPLE_FORMAT_B8,
        out,
        SampleFormat::SAMPLE_FORMAT_01,
        Circuit(),
        true,
        false,
        nullptr,
        SampleFormat::SAMPLE_FORMAT_01);
    fclose(in);
    fclose(sweep);
    ASSERT_EQ(rewind_read_close(out), "");
}

TEST(measurements_to_detection_events, some_input_01_empty_sweep_b8) {
    FILE *in = tmpfile();
    fprintf(in, "%s", "\n\n");
    rewind(in);
    FILE *sweep = tmpfile();
    FILE *out = tmpfile();

    stream_measurements_to_detection_events(
        in,
        SampleFormat::SAMPLE_FORMAT_01,
        sweep,
        SampleFormat::SAMPLE_FORMAT_B8,
        out,
        SampleFormat::SAMPLE_FORMAT_01,
        Circuit(),
        true,
        false,
        nullptr,
        SampleFormat::SAMPLE_FORMAT_01);
    fclose(in);
    fclose(sweep);
    ASSERT_EQ(rewind_read_close(out), "\n\n");
}

TEST(measurements_to_detection_events, empty_input_b8_empty_sweep_b8) {
    FILE *in = tmpfile();
    FILE *sweep = tmpfile();
    FILE *out = tmpfile();

    ASSERT_THROW(
        {
            stream_measurements_to_detection_events(
                in,
                SampleFormat::SAMPLE_FORMAT_B8,
                sweep,
                SampleFormat::SAMPLE_FORMAT_B8,
                out,
                SampleFormat::SAMPLE_FORMAT_01,
                Circuit(),
                true,
                false,
                nullptr,
                SampleFormat::SAMPLE_FORMAT_01);
        },
        std::invalid_argument);
    fclose(in);
    fclose(sweep);
    ASSERT_EQ(rewind_read_close(out), "");
}
