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

#include <stdexcept>

#include "command_help.h"
#include "stim/dem/detector_error_model.h"
#include "stim/io/measure_record_batch_writer.h"
#include "stim/io/measure_record_reader.h"
#include "stim/io/stim_data_formats.h"
#include "stim/mem/simd_bits.h"
#include "stim/util_bot/arg_parse.h"

using namespace stim;

struct DataDetails {
    int num_measurements;
    int num_detectors;
    int num_observables;
    int bits_per_shot;
    bool include_measurements;
    bool include_detectors;
    bool include_observables;
};

void process_num_flags(int argc, const char **argv, DataDetails *details_out) {
    details_out->num_measurements = find_int64_argument("--num_measurements", 0, 0, INT64_MAX, argc, argv);
    details_out->num_detectors = find_int64_argument("--num_detectors", 0, 0, INT64_MAX, argc, argv);
    details_out->num_observables = find_int64_argument("--num_observables", 0, 0, INT64_MAX, argc, argv);

    details_out->include_measurements = details_out->num_measurements > 0;
    details_out->include_detectors = details_out->num_detectors > 0;
    details_out->include_observables = details_out->num_observables > 0;
}

static void process_dem(const char *dem_path_c_str, DataDetails *details_out) {
    if (dem_path_c_str == nullptr) {
        return;
    }

    FILE *dem_file = fopen(dem_path_c_str, "rb");
    if (dem_file == nullptr) {
        std::stringstream msg;
        msg << "Failed to open '" << dem_path_c_str << "'";
        throw std::invalid_argument(msg.str());
    }
    auto dem = DetectorErrorModel::from_file(dem_file);
    fclose(dem_file);
    details_out->num_detectors = dem.count_detectors();
    details_out->num_observables = dem.count_observables();
    details_out->include_detectors = details_out->num_detectors > 0;
    details_out->include_observables = details_out->num_observables > 0;
}

static void process_circuit(const char *circuit_path_c_str, const char *types, DataDetails *details_out) {
    if (circuit_path_c_str == nullptr) {
        return;
    }
    if (types == nullptr) {
        throw std::invalid_argument("--types required when passing circuit");
    }
    FILE *circuit_file = fopen(circuit_path_c_str, "rb");
    if (circuit_file == nullptr) {
        std::stringstream msg;
        msg << "Failed to open '" << circuit_path_c_str << "'";
        throw std::invalid_argument(msg.str());
    }
    auto circuit = Circuit::from_file(circuit_file);
    fclose(circuit_file);
    CircuitStats circuit_stats = circuit.compute_stats();
    details_out->num_measurements = circuit_stats.num_measurements;
    details_out->num_detectors = circuit_stats.num_detectors;
    details_out->num_observables = circuit_stats.num_observables;

    while (types != nullptr && *types) {
        char c = *types;
        bool found_duplicate = false;
        if (c == 'M') {
            found_duplicate = details_out->include_measurements;
            details_out->include_measurements = true;
        } else if (c == 'D') {
            found_duplicate = details_out->include_detectors;
            details_out->include_detectors = true;
        } else if (c == 'L') {
            found_duplicate = details_out->include_observables;
            details_out->include_observables = true;
        } else {
            throw std::invalid_argument("Unknown type passed to --types");
        }

        if (found_duplicate) {
            throw std::invalid_argument("Each type in types should only be specified once");
        }
        ++types;
    }
}

int stim::command_convert(int argc, const char **argv) {
    check_for_unknown_arguments(
        {
            "--in_format",
            "--out_format",
            "--obs_out_format",
            "--in",
            "--out",
            "--obs_out",
            "--circuit",
            "--dem",
            "--types",
            "--num_measurements",
            "--num_detectors",
            "--num_observables",
            "--bits_per_shot",
        },
        {},
        "convert",
        argc,
        argv);

    DataDetails details;

    const auto &in_format = find_enum_argument("--in_format", nullptr, format_name_to_enum_map(), argc, argv);
    const auto &out_format = find_enum_argument("--out_format", "01", format_name_to_enum_map(), argc, argv);
    const auto &obs_out_format = find_enum_argument("--obs_out_format", "01", format_name_to_enum_map(), argc, argv);
    FILE *in = find_open_file_argument("--in", stdin, "rb", argc, argv);
    FILE *out = find_open_file_argument("--out", stdout, "wb", argc, argv);
    FILE *obs_out = find_open_file_argument("--obs_out", stdout, "wb", argc, argv);

    // Determine the necessary data needed to parse the input and
    // write to the new output.

    // First see if everything was just given directly.
    process_num_flags(argc, argv, &details);

    // Next see if we can infer from a given DEM file.
    const char *dem_path = find_argument("--dem", argc, argv);
    process_dem(dem_path, &details);

    // Finally see if we can infer from a given circuit file and
    // list of value types.
    const char *circuit_path_c_str = find_argument("--circuit", argc, argv);
    const char *types = find_argument("--types", argc, argv);
    try {
        process_circuit(circuit_path_c_str, types, &details);
    } catch (std::exception &e) {
        std::cerr << "\033[31m" << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // Not enough information to infer types, at this point we can only
    // convert arbitrary bits.
    if (!details.include_measurements && !details.include_detectors && !details.include_observables) {
        // dets outputs explicit value types, which we don't know if we get here.
        if (out_format.id == SampleFormat::SAMPLE_FORMAT_DETS) {
            std::cerr
                << "\033[31mNot enough information given to parse input file to write to dets. Please given a circuit "
                   "with --types, a DEM file, or explicit number of each desired type\n";
            return EXIT_FAILURE;
        }
        details.bits_per_shot = find_int64_argument("--bits_per_shot", 0, 0, INT64_MAX, argc, argv);
        if (details.bits_per_shot == 0) {
            std::cerr << "\033[31mNot enough information given to parse input file.\n";
            return EXIT_FAILURE;
        }
        details.include_measurements = true;
        details.num_measurements = details.bits_per_shot;
    }

    auto reader = MeasureRecordReader<MAX_BITWORD_WIDTH>::make(
        in,
        in_format.id,
        details.include_measurements ? details.num_measurements : 0,
        details.include_detectors ? details.num_detectors : 0,
        details.include_observables ? details.num_observables : 0);
    auto writer = MeasureRecordWriter::make(out, out_format.id);

    std::unique_ptr<MeasureRecordWriter> obs_writer;
    if (obs_out != stdout) {
        obs_writer = MeasureRecordWriter::make(obs_out, obs_out_format.id);
    } else {
        obs_out = nullptr;
    }

    simd_bits<MAX_BITWORD_WIDTH> buf(reader->bits_per_record());

    while (reader->start_and_read_entire_record(buf)) {
        int64_t offset = 0;
        if (details.include_measurements) {
            writer->begin_result_type('M');
            for (int64_t i = 0; i < details.num_measurements; ++i) {
                writer->write_bit(buf[i]);
            }
        }
        offset += reader->num_measurements;
        if (details.include_detectors) {
            writer->begin_result_type('D');
            for (int64_t i = 0; i < details.num_detectors; ++i) {
                writer->write_bit(buf[i + offset]);
            }
        }
        offset += reader->num_detectors;
        if (details.include_observables) {
            if (obs_writer) {
                obs_writer->begin_result_type('L');
            } else {
                writer->begin_result_type('L');
            }
            for (int64_t i = 0; i < details.num_observables; ++i) {
                if (obs_writer) {
                    obs_writer->write_bit(buf[i + offset]);
                } else {
                    writer->write_bit(buf[i + offset]);
                }
            }
        }
        if (obs_writer) {
            obs_writer->write_end();
        }
        writer->write_end();
    }

    if (in != stdin) {
        fclose(in);
    }
    if (out != stdout) {
        fclose(out);
    }
    if (obs_out != nullptr) {
        fclose(obs_out);
    }
    return EXIT_SUCCESS;
}

SubCommandHelp stim::command_convert_help() {
    SubCommandHelp result;
    result.subcommand_name = "convert";
    result.description = clean_doc_string(R"PARAGRAPH(
        Convert data between result formats.

        See the various formats here:
        https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md

        To read and write data, the size of the records must be known.
        If writing to a dets file, then the number of measurements, detectors
        and observables per record must also be known.

        Both of these pieces of information can either be given directly, or
        inferred from various data sources, such as circuit or dem files.
        )PARAGRAPH");

    result.examples.push_back(clean_doc_string(R"PARAGRAPH(
            >>> cat example.01
            10000
            11001
            00000
            01001

            >>> stim convert \
                --in example.01 \
                --in_format 01 \
                --out_format dets
                --num_measurements 5
            shot M0
            shot M0 M1 M4
            shot
            shot M1 M4
        )PARAGRAPH"));

    result.examples.push_back(clean_doc_string(R"PARAGRAPH(
            >>> cat example.dem
            detector D0
            detector D1
            logical_observable L2

            >>> cat example.dets
            shot D0
            shot D0 D1 L2
            shot
            shot D1 L2

            >>> stim convert \
                --in example.dets \
                --in_format dets \
                --out_format 01
                --dem example.dem
            10000
            11001
            00000
            01001
        )PARAGRAPH"));

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
        )PARAGRAPH"));

    result.examples.push_back(clean_doc_string(R"PARAGRAPH(
            >>> cat example.01
            0010
            0111
            1000
            1110

            >>> stim convert \
                --in example.01 \
                --in_format 01 \
                --out_format hits
                --bits_per_shot 4
            2
            1,2,3
            0
            0,1,2
        )PARAGRAPH"));

    result.flags.push_back(
        SubCommandHelpFlag{
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

    result.flags.push_back(
        SubCommandHelpFlag{
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

    result.flags.push_back(
        SubCommandHelpFlag{
            "--obs_out_format",
            "01|b8|r8|ptb64|hits|dets",
            "01",
            {"[none]", "format"},
            clean_doc_string(R"PARAGRAPH(
            Specifies the data format to use when writing observable flip data.

            Irrelevant unless `--obs_out` is specified.

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

    result.flags.push_back(
        SubCommandHelpFlag{
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

    result.flags.push_back(
        SubCommandHelpFlag{
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

    result.flags.push_back(
        SubCommandHelpFlag{
            "--obs_out",
            "filepath",
            "",
            {"[none]", "filepath"},
            clean_doc_string(R"PARAGRAPH(
            Specifies the file to write observable flip data to.

            When producing detection event data, the goal is typically to
            predict whether or not the logical observables were flipped by using
            the detection events. This argument specifies where to write that
            observable flip data.

            If this argument isn't specified, the observable flip data isn't
            written to a file.

            The output is in a format specified by `--obs_out_format`. See:
            https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--circuit",
            "filepath",
            "",
            {"[none]", "filepath"},
            clean_doc_string(R"PARAGRAPH(
            Specifies where the circuit that generated the data is.

            This argument is optional, but can be used to infer the number of
            measurements, detectors and observables to use per record.

            The circuit file should be a stim circuit. See:
            https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--types",
            "M|D|L",
            "",
            {"[none]"
             "types"},
            clean_doc_string(R"PARAGRAPH(
            Specifies the types of events in the files.

            This argument is required if a circuit is given as the circuit can
            give the number of each type of event, but not which events are
            contained within an input file.

            Note that in most cases, a file will have either measurements only,
            detections only, or detections and observables.

            The type values (M, D, L) correspond to the value prefix letters
            in dets files. See:
            https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md#dets
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--num_measurements",
            "int",
            "0",
            {"[none], int"},
            clean_doc_string(R"PARAGRAPH(
            Specifies the number of measurements in the input/output files.

            This argument is required if writing to a dets file and the circuit
            is not given.
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--num_detectors",
            "int",
            "0",
            {"[none], int"},
            clean_doc_string(R"PARAGRAPH(
            Specifies the number of detectors in the input/output files.

            This argument is required if writing to a dets file and the circuit
            or dem is not given.
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--num_observables",
            "int",
            "0",
            {"[none], int"},
            clean_doc_string(R"PARAGRAPH(
            Specifies the number of observables in the input/output files.

            This argument is required if writing to a dets file and the circuit
            or dem is not given.
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--bits_per_shot",
            "int",
            "0",
            {"[none], int"},
            clean_doc_string(R"PARAGRAPH(
            Specifies the number of bits per shot in the input/output files.

            This argument is required if the circuit, dem or num_* flags
            are not given, and not supported when writing to a dets file.

            In this case we just treat the bits aas arbitrary data. It is up
            to the user to interpert it correctly.
        )PARAGRAPH"),
        });

    return result;
}
