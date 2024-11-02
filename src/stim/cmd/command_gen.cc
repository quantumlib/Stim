#include "stim/cmd/command_gen.h"

#include "command_help.h"
#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_color_code.h"
#include "stim/gen/gen_rep_code.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/util_bot/arg_parse.h"

using namespace stim;

int stim::command_gen(int argc, const char **argv) {
    std::map<std::string_view, GeneratedCircuit (*)(const CircuitGenParameters &)> code_name_to_func_map{
        {"color_code", &generate_color_code_circuit},
        {"repetition_code", &generate_rep_code_circuit},
        {"surface_code", &generate_surface_code_circuit}};
    std::vector<const char *> known_flags{
        "--after_clifford_depolarization",
        "--after_reset_flip_probability",
        "--code",
        "--task",
        "--before_measure_flip_probability",
        "--before_round_data_depolarization",
        "--distance",
        "--out",
        "--in",
        "--rounds",
    };
    std::vector<const char *> known_flags_deprecated{
        "--gen",
    };

    check_for_unknown_arguments(known_flags, known_flags_deprecated, "gen", argc, argv);
    const char *code_flag_name = find_argument("--gen", argc, argv) ? "--gen" : "--code";
    auto func = find_enum_argument(code_flag_name, nullptr, code_name_to_func_map, argc, argv);
    CircuitGenParameters params(
        (uint64_t)find_int64_argument("--rounds", -1, 1, INT64_MAX, argc, argv),
        (uint32_t)find_int64_argument("--distance", -1, 2, 2047, argc, argv),
        require_find_argument("--task", argc, argv));
    params.before_round_data_depolarization =
        find_float_argument("--before_round_data_depolarization", 0, 0, 1, argc, argv);
    params.before_measure_flip_probability =
        find_float_argument("--before_measure_flip_probability", 0, 0, 1, argc, argv);
    params.after_reset_flip_probability = find_float_argument("--after_reset_flip_probability", 0, 0, 1, argc, argv);
    params.after_clifford_depolarization = find_float_argument("--after_clifford_depolarization", 0, 0, 1, argc, argv);
    auto out_stream = find_output_stream_argument("--out", true, argc, argv);
    std::ostream &out = out_stream.stream();
    out << "# Generated " << find_argument(code_flag_name, argc, argv) << " circuit.\n";
    out << "# task: " << params.task << "\n";
    out << "# rounds: " << params.rounds << "\n";
    out << "# distance: " << params.distance << "\n";
    out << "# before_round_data_depolarization: " << params.before_round_data_depolarization << "\n";
    out << "# before_measure_flip_probability: " << params.before_measure_flip_probability << "\n";
    out << "# after_reset_flip_probability: " << params.after_reset_flip_probability << "\n";
    out << "# after_clifford_depolarization: " << params.after_clifford_depolarization << "\n";
    out << "# layout:\n";
    auto generated = func(params);
    out << generated.layout_str();
    out << generated.hint_str;
    out << generated.circuit;
    out << "\n";
    return 0;
}

SubCommandHelp stim::command_gen_help() {
    SubCommandHelp result;
    result.subcommand_name = "gen";
    result.description = clean_doc_string(R"PARAGRAPH(
        Generates example circuits.

        The generated circuits include annotations for noise, detectors, logical
        observables, the spatial locations of qubits, the spacetime locations
        of detectors, and the inexorable passage of TICKs.

        Note that the generated circuits are not intended to be sufficient for
        research. They are really just examples to make it easier to get started
        using Stim, so you can try things without having to first go through
        the entire effort of making a correctly annotated quantum error
        correction circuit.
    )PARAGRAPH");

    result.examples.push_back(clean_doc_string(R"PARAGRAPH(
            >>> stim gen \
                --code repetition_code \
                --task memory \
                --distance 3 \
                --rounds 100 \
                --after_clifford_depolarization 0.001
            # Generated repetition_code circuit.
            # task: memory
            # rounds: 100
            # distance: 3
            # before_round_data_depolarization: 0
            # before_measure_flip_probability: 0
            # after_reset_flip_probability: 0
            # after_clifford_depolarization: 0.001
            # layout:
            # L0 Z1 d2 Z3 d4
            # Legend:
            #     d# = data qubit
            #     L# = data qubit with logical observable crossing
            #     Z# = measurement qubit
            R 0 1 2 3 4
            TICK
            CX 0 1 2 3
            DEPOLARIZE2(0.001) 0 1 2 3
            TICK
            CX 2 1 4 3
            DEPOLARIZE2(0.001) 2 1 4 3
            TICK
            MR 1 3
            DETECTOR(1, 0) rec[-2]
            DETECTOR(3, 0) rec[-1]
            REPEAT 99 {
                TICK
                CX 0 1 2 3
                DEPOLARIZE2(0.001) 0 1 2 3
                TICK
                CX 2 1 4 3
                DEPOLARIZE2(0.001) 2 1 4 3
                TICK
                MR 1 3
                SHIFT_COORDS(0, 1)
                DETECTOR(1, 0) rec[-2] rec[-4]
                DETECTOR(3, 0) rec[-1] rec[-3]
            }
            M 0 2 4
            DETECTOR(1, 1) rec[-2] rec[-3] rec[-5]
            DETECTOR(3, 1) rec[-1] rec[-2] rec[-4]
            OBSERVABLE_INCLUDE(0) rec[-1]
        )PARAGRAPH"));

    result.flags.push_back(
        SubCommandHelpFlag{
            "--code",
            "surface_code|repetition_code|color_code",
            "",
            {"surface_code|repetition_code|color_code"},
            clean_doc_string(R"PARAGRAPH(
            The error correcting code to use.

            The available error correcting codes are:

            `surface_code`
                The surface code. A quantum code with a checkerboard pattern of
                alternating X and Z stabilizers.
            `repetition_code`
                The repetition code. The simplest classical code.
            `color_code`
                The color code. A quantum code with a hexagonal pattern of
                overlapping X and Z stabilizers.
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--task",
            "name",
            "",
            {"name"},
            clean_doc_string(R"PARAGRAPH(
            What the generated circuit should do; the experiment it should run.

            Different error correcting codes support different tasks. The
            available tasks are:

            `memory` (repetition_code):
                Initialize a logical `|0>`,
                preserve it against noise for the given number of rounds,
                then measure.
            `rotated_memory_x` (surface_code):
                Initialize a logical `|+>` in a rotated surface code,
                preserve it against noise for the given number of rounds,
                then measure in the X basis.
            `rotated_memory_z` (surface_code):
                Initialize a logical `|0>` in a rotated surface code,
                preserve it against noise for the given number of rounds,
                then measure in the X basis.
            `unrotated_memory_x` (surface_code):
                Initialize a logical `|+>` in an unrotated surface code,
                preserve it against noise for the given number of rounds,
                then measure in the Z basis.
            `unrotated_memory_z` (surface_code):
                Initialize a logical `|0>` in an unrotated surface code,
                preserve it against noise for the given number of rounds,
                then measure in the Z basis.
            `memory_xyz` (color_code):
                Initialize a logical `|0>`,
                preserve it against noise for the given number of rounds,
                then measure. Use a color code that alternates between measuring
                X, then Y, then Z stabilizers.
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--distance",
            "int",
            "",
            {"int"},
            clean_doc_string(R"PARAGRAPH(
            The minimum number of physical errors that cause a logical error.

            The code distance determines how spatially large the generated
            circuit has to be. Conventionally, the code distance specifically
            refers to single-qubit errors between rounds instead of circuit
            errors during rounds.

            The distance must always be a positive integer. Different
            codes/tasks may place additional constraints on the distance (e.g.
            must be larger than 2 or must be odd or etc).
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--rounds",
            "int",
            "",
            {"int"},
            clean_doc_string(R"PARAGRAPH(
            The number of times the circuit's measurement qubits are measured.

            The number of rounds must be an integer between 1 and a quintillion
            (10^18). Different codes/tasks may place additional constraints on
            the number of rounds (e.g. enough rounds to have measured all the
            stabilizers at least once).
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--after_clifford_depolarization",
            "probability",
            "0",
            {"[none]", "probability"},
            clean_doc_string(R"PARAGRAPH(
            Specifies a depolarizing noise level for unitary gates.

            Defaults to 0 when not specified.
            Must be a probability (a number between 0 and 1).

            Adds a `DEPOLARIZE1(p)` operation after every single-qubit Clifford
            operation, and a `DEPOLARIZE2(p)` noise operation after every
            two-qubit Clifford operation. When set to 0 or not set, these noise
            operations are not inserted at all.
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--after_reset_flip_probability",
            "probability",
            "0",
            {"[none]", "probability"},
            clean_doc_string(R"PARAGRAPH(
            Specifies a reset noise level.

            Defaults to 0 when not specified.
            Must be a probability (a number between 0 and 1).

            Adds an `X_ERROR(p)` after `R` and `RY` operations, and a
            `Z_ERROR(p)` after `RX` operations. When set to 0 or not set,
            these noise operations are not inserted at all.
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--before_measure_flip_probability",
            "probability",
            "0",
            {"[none]", "probability"},
            clean_doc_string(R"PARAGRAPH(
            Specifies a measurement noise level.

            Defaults to 0 when not specified.
            Must be a probability (a number between 0 and 1).

            Adds an `X_ERROR(p)` before `M` and `MY` operations, and a
            `Z_ERROR(p)` before `MX` operations. When set to 0 or not set,
            these noise operations are not inserted at all.
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--before_round_data_depolarization",
            "probability",
            "0",
            {"[none]", "probability"},
            clean_doc_string(R"PARAGRAPH(
            Specifies a quantum phenomenological noise level.

            Defaults to 0 when not specified.
            Must be a probability (a number between 0 and 1).

            Adds a `DEPOLARIZE1(p)` operation to each data qubit at the start of
            each round of stabilizer measurements. When set to 0 or not set,
            these noise operations are not inserted at all.
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--out",
            "filepath",
            "{stdout}",
            {"[none]", "filepath"},
            clean_doc_string(R"PARAGRAPH(
            Chooses where to write the generated circuit to.

            By default, the output is written to stdout. When `--out $FILEPATH`
            is specified, the output is instead written to the file at $FILEPATH.

            The output is a stim circuit. See:
            https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md
        )PARAGRAPH"),
        });

    return result;
}
