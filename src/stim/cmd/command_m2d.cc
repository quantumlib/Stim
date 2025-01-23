#include "stim/cmd/command_m2d.h"

#include "command_help.h"
#include "stim/io/stim_data_formats.h"
#include "stim/simulators/measurements_to_detection_events.h"
#include "stim/util_bot/arg_parse.h"
#include "stim/util_top/transform_without_feedback.h"

using namespace stim;

int stim::command_m2d(int argc, const char **argv) {
    check_for_unknown_arguments(
        {
            "--circuit",
            "--in_format",
            "--append_observables",
            "--out_format",
            "--out",
            "--in",
            "--skip_reference_sample",
            "--sweep",
            "--sweep_format",
            "--obs_out",
            "--obs_out_format",
            "--ran_without_feedback",
        },
        {
            "--m2d",
        },
        "m2d",
        argc,
        argv);
    const auto &in_format = find_enum_argument("--in_format", nullptr, format_name_to_enum_map(), argc, argv);
    const auto &out_format = find_enum_argument("--out_format", "01", format_name_to_enum_map(), argc, argv);
    const auto &sweep_format = find_enum_argument("--sweep_format", "01", format_name_to_enum_map(), argc, argv);
    const auto &obs_out_format = find_enum_argument("--obs_out_format", "01", format_name_to_enum_map(), argc, argv);
    bool append_observables = find_bool_argument("--append_observables", argc, argv);
    bool skip_reference_sample = find_bool_argument("--skip_reference_sample", argc, argv);
    bool ran_without_feedback = find_bool_argument("--ran_without_feedback", argc, argv);
    FILE *circuit_file = find_open_file_argument("--circuit", nullptr, "rb", argc, argv);
    auto circuit = Circuit::from_file(circuit_file);
    fclose(circuit_file);
    if (ran_without_feedback) {
        circuit = circuit_with_inlined_feedback(circuit);
    }

    FILE *in = find_open_file_argument("--in", stdin, "rb", argc, argv);
    FILE *out = find_open_file_argument("--out", stdout, "wb", argc, argv);
    FILE *sweep_in = find_open_file_argument("--sweep", stdin, "rb", argc, argv);
    FILE *obs_out = find_open_file_argument("--obs_out", stdout, "wb", argc, argv);
    if (sweep_in == stdin) {
        sweep_in = nullptr;
    }
    if (obs_out == stdout) {
        obs_out = nullptr;
    }

    stream_measurements_to_detection_events<MAX_BITWORD_WIDTH>(
        in,
        in_format.id,
        sweep_in,
        sweep_format.id,
        out,
        out_format.id,
        circuit,
        append_observables,
        skip_reference_sample,
        obs_out,
        obs_out_format.id);
    if (in != stdin) {
        fclose(in);
    }
    if (sweep_in != nullptr) {
        fclose(sweep_in);
    }
    if (obs_out != nullptr) {
        fclose(obs_out);
    }
    if (out != stdout) {
        fclose(out);
    }
    return EXIT_SUCCESS;
}

SubCommandHelp stim::command_m2d_help() {
    SubCommandHelp result;
    result.subcommand_name = "m2d";
    result.description = clean_doc_string(R"PARAGRAPH(
        Convert measurement data into detection event data.

        When sampling data from hardware, instead of from simulators, it's
        necessary to convert the measurement data into detection event data that
        can be fed into a decoder. This is necessary both because of
        complexities in the exact sets of measurements being compared by a
        circuit to produce detection events and also because the expected parity
        of a detector's measurement set can vary due to (for example) spin echo
        operations in the circuit.

        Stim performs this conversion by simulating taking a reference sample
        from the circuit, in order to determine the expected parity of the
        measurements sets defining detectors and observables, and then comparing
        the sampled measurement data to these expectations.
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

            >>> stim m2d \
                --append_observables \
                --circuit example_circuit.stim \
                --in example_measure_data.01 \
                --in_format 01 \
                --out_format dets
            shot D0
            shot D0 D1 L2
            shot
            shot D1 L2
        )PARAGRAPH"));

    result.flags.push_back(
        SubCommandHelpFlag{
            "--out_format",
            "01|b8|r8|ptb64|hits|dets",
            "01",
            {"[none]", "format"},
            clean_doc_string(R"PARAGRAPH(
            Specifies the data format to use when writing output detection data.

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
            "--in_format",
            "01|b8|r8|ptb64|hits|dets",
            "01",
            {"[none]", "format"},
            clean_doc_string(R"PARAGRAPH(
            Specifies the data format to use when reading measurement data.

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
            "--sweep",
            "filepath",
            "",
            {"filepath"},
            clean_doc_string(R"PARAGRAPH(
            Specifies a file to read sweep configuration data from.

            Sweep bits are used to vary whether certain Pauli gates are included
            in a circuit, or not, from shot to shot. For example, if a circuit
            contains the instruction "CX sweep[5] 0" then there is an X pauli
            that is included only in shots where the corresponding sweep data
            has the bit at index 5 set to True.

            If `--sweep` is not specified, all sweep bits default to OFF. If
            `--sweep` is specified, each shot's sweep configuratoin data is
            read from the specified file.

            The sweep data's format is specified by `--sweep_format`. See:
            https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--sweep_format",
            "01|b8|r8|ptb64|hits|dets",
            "01",
            {"[none]", "format"},
            clean_doc_string(R"PARAGRAPH(
            Specifies the data format to use when reading sweep config data.

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
            Chooses the file to read measurement data from.

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
            Chooses where to write the sampled data to.

            By default, the output is written to stdout. When `--out $FILEPATH`
            is specified, the output is instead written to the file at $FILEPATH.

            The output's format is specified by `--out_format`. See:
            https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--circuit",
            "filepath",
            "",
            {"filepath"},
            clean_doc_string(R"PARAGRAPH(
            Specifies where the circuit that generated the measurements is.

            This argument is required, because the circuit is what specifies how
            to derive detection event data from measurement data.

            The circuit file should be a stim circuit. See:
            https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--append_observables",
            "bool",
            "false",
            {"[none]", "[switch]"},
            clean_doc_string(R"PARAGRAPH(
            Appends observable flips to the end of samples as extra detectors.

            PREFER --obs_out OVER THIS FLAG. Mixing the observable flip data
            into detection event data tends to require simply separating them
            again immediately, creating unnecessary work. For example, when
            testing a decoder, you do not want to give the observable flips to
            the decoder because that is the information the decoder is supposed
            to be predicting from the detection events.

            This flag causes observable flip data to be appended to each sample,
            as if the observables were extra detectors at the end of the
            circuit. For example, if there are 100 detectors and 10 observables
            in the circuit, then the output will contain 110 detectors and the
            last 10 are the observables.
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--ran_without_feedback",
            "bool",
            "false",
            {"[none]", "[switch]"},
            clean_doc_string(R"PARAGRAPH(
            Converts the results assuming all feedback operations were skipped.

            Pauli feedback operations don't need to be performed on the quantum
            computer. They can be performed within the classical control system.
            As such, it often makes sense to skip them when sampling from the
            circuit on hardware, and then account for this later during data
            analysis. Turning on this flag means that the quantum computer
            didn't apply the feedback operations, and it's the job of the m2d
            conversion to read the measurement data, rewrite it to account for
            feedback effects, then convert to detection events.

            In the python API, the same effect can be achieved by using
            stim.Circuit.with_inlined_feedback().compile_m2d_converter().
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--skip_reference_sample",
            "bool",
            "false",
            {"[none]", "[switch]"},
            clean_doc_string(R"PARAGRAPH(
            Asserts the circuit can produce a noiseless sample that is just 0s.

            When this argument is specified, the reference sample (that the
            measurement data will be compared to) is generated by simply setting
            all measurements to 0 instead of by simulating the circuit without
            noise.

            Skipping the reference sample can significantly improve performance,
            because acquiring the reference sample requires using the tableau
            simulator. If the vacuous reference sample is actually a result that
            can be produced by the circuit, under noiseless execution, then
            specifying this flag has no observable outcome other than improving
            performance.

            CAUTION. When the all-zero sample isn't a result that can be
            produced by the circuit under noiseless execution, specifying this
            flag will cause incorrect output to be produced.
        )PARAGRAPH"),
        });

    return result;
}
