// Copyright 2023 Google LLC
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

#include "stim/cmd/command_convert.h"

#include "command_help.h"
#include "stim/arg_parse.h"
#include "stim/io/measure_record_batch_writer.h"
#include "stim/io/measure_record_reader.h"
#include "stim/io/stim_data_formats.h"
#include "stim/mem/simd_bits.h"

using namespace stim;

int stim::command_convert(int argc, const char **argv) {
    check_for_unknown_arguments(
        {
            "--in_format",
            "--out_format",
            "--in",
            "--out",
            "--circuit",
            "--types",
        },
        {},
        "convert",
        argc,
        argv);

    const auto &in_format = find_enum_argument("--in_format", nullptr, format_name_to_enum_map(), argc, argv);
    const auto &out_format = find_enum_argument("--out_format", "01", format_name_to_enum_map(), argc, argv);
    FILE *in = find_open_file_argument("--in", stdin, "rb", argc, argv);
    FILE *out = find_open_file_argument("--out", stdout, "wb", argc, argv);
    FILE *circuit_file = find_open_file_argument("--circuit", nullptr, "rb", argc, argv);
    auto circuit = Circuit::from_file(circuit_file);
    fclose(circuit_file);
    CircuitStats circuit_stats = circuit.compute_stats();

    const char *types = require_find_argument("--types", argc, argv);
    bool include_measurements = false, include_detectors = false, include_observables = false;
    include_measurements = strchr(types, 'M') != nullptr;
    include_detectors = strchr(types, 'D') != nullptr;
    include_observables = strchr(types, 'L') != nullptr;

    auto reader = MeasureRecordReader<MAX_BITWORD_WIDTH>::make(
        in,
        in_format.id,
        include_measurements ? circuit_stats.num_measurements : 0,
        include_detectors ? circuit_stats.num_detectors : 0,
        include_observables ? circuit_stats.num_observables : 0);
    auto writer = MeasureRecordWriter::make(out, out_format.id);
    simd_bits<MAX_BITWORD_WIDTH> buf(reader->bits_per_record());

    while (reader->start_and_read_entire_record(buf)) {
        if (include_measurements) {
            writer->begin_result_type('M');
            for (uint64_t i = 0; i < circuit_stats.num_measurements; ++i) {
                writer->write_bit(buf[i]);
            }
        }
        if (include_detectors) {
            writer->begin_result_type('D');
            for (uint64_t i = 0; i < circuit_stats.num_detectors; ++i) {
                writer->write_bit(buf[i + reader->num_measurements]);
            }
        }
        if (include_observables) {
            writer->begin_result_type('L');
            for (uint64_t i = 0; i < circuit_stats.num_observables; ++i) {
                writer->write_bit(buf[i + reader->num_measurements + reader->num_detectors]);
            }
        }
        writer->write_end();
    }

    if (in != stdin) {
        fclose(in);
    }
    if (out != stdout) {
        fclose(out);
    }
    return EXIT_SUCCESS;
}

SubCommandHelp stim::command_convert_help() {
    SubCommandHelp result;
    result.subcommand_name = "convert";
    result.description = clean_doc_string(R"PARAGRAPH(
        Convert data between result formats.
    )PARAGRAPH");

    result.examples.push_back(clean_doc_string(R"PARAGRAPH(
            >>> cat example_circuit.stim
            X 0
            M 0 1
            DETECTOR rec[-2]
            DETECTOR rec[-1]
            OBSERVABLE_INCLUDE(2) rec[-1]

            >>> cat example_measure_data.01
            00
            01
            10
            11

            >>> stim convert \
                --in example_measure_data.01 \
                --in_format 01 \
                --out_format dets
                --circuit example_circuit.stim \
                --types M
            shot
            shot M1
            shot M0
            shot M0 M1

            >>> cat example_detection_data.01
            10000
            11001
            00000
            01001

            >>> stim convert \
                --in example_detection_data.01 \
                --in_format 01 \
                --out_format dets
                --circuit example_circuit.stim \
                --types DL
            shot D0
            shot D0 D1 L2
            shot
            shot D1 L2
        )PARAGRAPH"));

    result.flags.push_back(SubCommandHelpFlag{
        "--in_format",
        "01|b8|r8|ptb64|hits|dets",
        "01",
        {"[none]", "format"},
        clean_doc_string(R"PARAGRAPH(
            Specifies the data format to use when reading data.

            The available formats are:

                01 (default): dense human readable
                b8: bit packed binary
                r8: run length binary
                ptb64: partially transposed bit packed binary for SIMD
                hits: sparse human readable
                dets: sparse human readable with type hints

            For a detailed description of each result format, see the result
            format reference:
            https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        )PARAGRAPH"),
    });

    result.flags.push_back(SubCommandHelpFlag{
        "--out_format",
        "01|b8|r8|ptb64|hits|dets",
        "01",
        {"[none]", "format"},
        clean_doc_string(R"PARAGRAPH(
            Specifies the data format to use when writing output data.

            The available formats are:

                01 (default): dense human readable
                b8: bit packed binary
                r8: run length binary
                ptb64: partially transposed bit packed binary for SIMD
                hits: sparse human readable
                dets: sparse human readable with type hints

            For a detailed description of each result format, see the result
            format reference:
            https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        )PARAGRAPH"),
    });

    result.flags.push_back(SubCommandHelpFlag{
        "--in",
        "filepath",
        "{stdin}",
        {"[none]", "filepath"},
        clean_doc_string(R"PARAGRAPH(
            Chooses the file to read data from.

            By default, the circuit is read from stdin. When `--in $FILEPATH` is
            specified, the circuit is instead read from the file at $FILEPATH.

            The input's format is specified by `--in_format`. See:
            https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        )PARAGRAPH"),
    });

    result.flags.push_back(SubCommandHelpFlag{
        "--out",
        "filepath",
        "{stdout}",
        {"[none]", "filepath"},
        clean_doc_string(R"PARAGRAPH(
            Chooses where to write the data to.

            By default, the output is written to stdout. When `--out $FILEPATH`
            is specified, the output is instead written to the file at $FILEPATH.

            The output's format is specified by `--out_format`. See:
            https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        )PARAGRAPH"),
    });

    result.flags.push_back(SubCommandHelpFlag{
        "--circuit",
        "filepath",
        "",
        {"filepath"},
        clean_doc_string(R"PARAGRAPH(
            Specifies where the circuit that generated the data is.

            This argument is required, because the circuit is what specifies
            the number of measurements, detectors and observables to use per record.

            The circuit file should be a stim circuit. See:
            https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md
        )PARAGRAPH"),
    });

    result.flags.push_back(SubCommandHelpFlag{
        "--types",
        "M|D|L",
        "",
        {"filepath"},
        clean_doc_string(R"PARAGRAPH(
            Specifies the types of events in the files.

            This argument is required to decode the input file and determine
            if it includes measurements, detections or observable frame changes.

            Note that in most cases, a file will have either measurements only,
            detections only, or detections and observables. 

            The type values (M, D, L) correspond to the value prefix letters
            in dets files. See:
            https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md#dets
        )PARAGRAPH"),
    });

    return result;
}
