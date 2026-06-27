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

#include "stim/cmd/command_analyze_errors.h"

#include "stim/cmd/command_help.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/util_bot/arg_parse.h"

using namespace stim;

int stim::command_analyze_errors(int argc, const char **argv) {
    check_for_unknown_arguments(
        {
            "--allow_gauge_detectors",
            "--approximate_disjoint_errors",
            "--block_decompose_from_introducing_remnant_edges",
            "--decompose_errors",
            "--fold_loops",
            "--ignore_decomposition_failures",
            "--in",
            "--out",
        },
        {"--analyze_errors", "--detector_hypergraph"},
        "analyze_errors",
        argc,
        argv);
    bool decompose_errors = find_bool_argument("--decompose_errors", argc, argv);
    bool fold_loops = find_bool_argument("--fold_loops", argc, argv);
    bool allow_gauge_detectors = find_bool_argument("--allow_gauge_detectors", argc, argv);
    bool ignore_decomposition_failures = find_bool_argument("--ignore_decomposition_failures", argc, argv);
    bool block_decompose_from_introducing_remnant_edges =
        find_bool_argument("--block_decompose_from_introducing_remnant_edges", argc, argv);

    const char *approximate_disjoint_errors_arg = find_argument("--approximate_disjoint_errors", argc, argv);
    float approximate_disjoint_errors_threshold;
    if (approximate_disjoint_errors_arg != nullptr && *approximate_disjoint_errors_arg == '\0') {
        approximate_disjoint_errors_threshold = 1;
    } else {
        approximate_disjoint_errors_threshold =
            find_float_argument("--approximate_disjoint_errors", 0, 0, 1, argc, argv);
    }

    FILE *in = find_open_file_argument("--in", stdin, "rb", argc, argv);
    auto out_stream = find_output_stream_argument("--out", true, argc, argv);
    std::ostream &out = out_stream.stream();
    auto circuit = Circuit::from_file(in);
    if (in != stdin) {
        fclose(in);
    }
    out << ErrorAnalyzer::circuit_to_detector_error_model(
               circuit,
               decompose_errors,
               fold_loops,
               allow_gauge_detectors,
               approximate_disjoint_errors_threshold,
               ignore_decomposition_failures,
               block_decompose_from_introducing_remnant_edges)
        << "\n";
    return EXIT_SUCCESS;
}

SubCommandHelp stim::command_analyze_errors_help() {
    SubCommandHelp result;
    result.subcommand_name = "analyze_errors";
    result.description = "Converts a circuit into a detector error model.";

    result.examples.push_back(clean_doc_string(R"PARAGRAPH(
            >>> cat example_circuit.stim
            R 0 1
            X_ERROR(0.125) 0 1
            CNOT 0 1
            M 0 1
            DETECTOR rec[-1]
            DETECTOR rec[-2]

            >>> stim analyze_errors --in example_circuit.stim
            error(0.125) D0
            error(0.125) D0 D1
        )PARAGRAPH"));
    result.examples.push_back(clean_doc_string(R"PARAGRAPH(
            >>> stim gen \
                    --code repetition_code \
                    --task memory \
                    --distance 3 \
                    --rounds 1000 \
                    --after_reset_flip_probability 0.125 \
                    > rep_code.stim
            >>> stim analyze_errors --fold_loops --in rep_code.stim
            error(0.125) D0
            error(0.125) D0 D1
            error(0.125) D0 D2
            error(0.125) D1 D3
            error(0.125) D1 L0
            error(0.125) D2 D4
            error(0.125) D3 D5
            detector(1, 0) D0
            detector(3, 0) D1
            repeat 998 {
                error(0.125) D4 D6
                error(0.125) D5 D7
                shift_detectors(0, 1) 0
                detector(1, 0) D2
                detector(3, 0) D3
                shift_detectors 2
            }
            shift_detectors(0, 1) 0
            detector(1, 0) D2
            detector(3, 0) D3
            detector(1, 1) D4
            detector(3, 1) D5
        )PARAGRAPH"));

    result.flags.push_back(
        SubCommandHelpFlag{
            "--allow_gauge_detectors",
            "bool",
            "false",
            {"[none]", "[switch]"},
            clean_doc_string(R"PARAGRAPH(
            Allows non-deterministic detectors to appear in the circuit.

            Normally (without `--allow_gauge_detectors`), when a detector's
            detecting region anti-commutes with a reset or measurement, stim
            will raise an exception when analyzing the circuit. When
            `--allow_gauge_detectors` is set, stim will instead append an error
            mechanism into the detector error model that has a probability of
            50% and flips all the detectors that anticommute with the operation.

            This is potentially useful in situations where the layout of
            detectors is supposed to stay fixed despite variations in the
            circuit structure. Decoders can interpret the existence of the 50%
            error as a weight 0 edge saying that the detectors should be fused
            together.

            For example, in the following stim circuit, the two detectors each
            anticommute with the reset operation:

                R 0
                H 0
                CNOT 0 1
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]

            Without `--allow_gauge_detectors`, stim will raise an exception when
            analyzing this circuit. With `--allow_gauge_detectors`, stim will
            add `error(0.5) D1 D2` to the output detector error model.

            BEWARE that gauge detectors are very tricky to work with, and not
            necessarily supported by all tools (even within stim itself). For
            example, when converting from measurements to detection events,
            there isn't a single choice for whether or not each individual gauge
            detector produced a detection event. This means that it is valid
            behavior for one conversion from measurements to detection events
            to give different results from another, as long as the gauge
            detectors that anticommute with the same operations flip together in
            a consistent fashion that respects the structure of the circuit.
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--approximate_disjoint_errors",
            "probability",
            "0.0",
            {"[none]", "[switch]", "probability"},
            clean_doc_string(R"PARAGRAPH(
            Allows disjoint errors to be approximated during the conversion.

            Detector error models require that all error mechanisms be
            specified as independent mechanisms. But some of the circuit error
            mechanisms that Stim allows can express errors that don't correspond
            to independent mechanisms. For example, the custom error channel
            `PAULI_CHANNEL_1(0.1, 0.2, 0.0)` can't be expressed exactly as a set
            of independent error mechanisms. But it can be approximated as an
            `X_ERROR(0.1)` followed by a `Y_ERROR(0.2)`.

            This flag can be set to any probability between 0 (the default when
            not specified) and 1 (the default when specified without a value).
            When set to a value strictly between 0 and 1, this determines the
            maximum disjoint probability that is allowed to be approximated as
            an independent probability.

            Without `--approximate_disjoint_errors`, attempting to convert a
            circuit containing `PAULI_CHANNEL_1(0.1, 0.2, 0.0)` will fail with
            an error stating an approximation is needed. With
            `--approximate_disjoint_errors`, the conversion will succeed by
            approximating the error into an `X_ERROR(0.1)` followed by an
            independent `Y_ERROR(0.2)`.

            Note that, although `DEPOLARIZE1` and `DEPOLARIZE2` are often
            defined in terms of disjoint errors, they can be exactly converted
            into a set of independent errors (unless the probability of the
            depolarizing error occurring exceeds maximum mixing, which is 75%
            for `DEPOLARIZE1` and 93.75% for `DEPOLARIZE2`). So the
            `--approximate_disjoint_errors` flag isn't needed for depolarizing
            errors that appear in practice.

            The error mechanisms that require approximations are:
            - PAULI_CHANNEL_1
            - PAULI_CHANNEL_2
            - ELSE_CORRELATED_ERROR

            In principle some custom Pauli channels can be converted exactly,
            but Stim does not currently contain logic that attempts to do this.
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--block_decompose_from_introducing_remnant_edges",
            "bool",
            "false",
            {"[none]", "[switch]"},
            clean_doc_string(R"PARAGRAPH(
            Prevents A*B from being decomposed unless A,B BOTH appear elsewhere.

            Irrelevant unless `--decompose_errors` is specified.

            When `--decompose_errors` is specified, any circuit error that
            causes more than two detection events must be decomposed into a
            set of errors with at most two detection events. The main constraint
            on this process is that it must not use errors that couldn't
            otherwise occur, since introducing such errors could violate
            important properties that are used for decoding. For example, in the
            normal surface code, it is very important that the decoding graphs
            for X errors and Z errors are disjoint in the bulk, and decomposing
            an error into a set of errors that violated this property would be
            disastrous.

            However, a corner case in this logic occurs if an error E1 that
            produces detection events A*B needs to be decomposed when an error
            E2 that produces detection events A appears elsewhere but no error
            producing detection events B appears elsewhere. The detection events
            B can be produced by both E1 and E2 occurring, but this a
            combination of two errors and so treating it as one error can cause
            problems. For example, it can result in the code distance appearing
            to be smaller than it actually is. Introducing B is referred to as
            introducing a "remnant edge" because B *only* appears in the
            detector error model as a remnant of removing A from A*B.

            By default, Stim does allow remnant edges to be introduced. Stim
            will only do this if it is absolutely necessary, but it *will* do
            it. And there are in fact QEC circuits where the decomposition
            requires these edges to succeed. But sometimes the presence of a
            remnant edge is a hint that the DETECTOR declarations in the circuit
            are subtly wrong. To cause the decomposition process to fail in
            this case, the `--block_decompose_from_introducing_remnant_edges`
            can be specified.
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--decompose_errors",
            "bool",
            "false",
            {"[none]", "[switch]"},
            clean_doc_string(R"PARAGRAPH(
            Decomposes errors with many detection events into "graphlike" parts.

            When `--decompose_errors` is specified, Stim will suggest how errors
            that cause more than 2 detection events (non-graphlike errors) can
            be decomposed into errors with at most 2 detection events (graphlike
            errors). For example, an error like `error(0.1) D0 D1 D2 D3` may be
            instead output as `error(0.1) D0 D1 ^ D2 D3` or as
            `error(0.1) D0 D3 ^ D1 ^ D2`.

            The purpose of this feature is to make matching apply to more cases.
            A common decoding strategy is "matching", where detection events are
            paired up in order to determine which errors occurred. Matching only
            works when the Tanner graph of the problem is a graph, not a
            hypergraph. In other words, it requires all errors to produce at
            most two detection events. This is a problem, because in practice
            there are essentially always circuit error mechanisms that produce
            more than two detection events. For example, in a CSS surface code,
            Y type errors on the data qubits will produce four detection events.
            For matching to work in these cases, non-graphlike errors (errors
            with more than two detection events) need to be approximated as a
            combination of graphlike errors.

            When Stim is decomposing errors, the main guarantee that it provides
            is that it will not introduce error mechanisms with symptoms that
            are otherwise impossible or that would require a combination of
            non-local errors to actually achieve. Informally, Stim guarantees it
            will preserve the "structure" of the detector error model when
            suggesting decompositions.

            It's also worth noting that the suggested decompositions are
            information preserving: the undecomposed model can always be
            recovered by simply filtering out all `^` characters splitting the
            errors into suggested components.

            Stim uses two strategies for decomposing errors: intra-channel and
            inter-channel.

            The *intra-channel* strategy is always applied first, and works by
            looking at the various detector/observable sets produced by each
            case of a single noise channel. If some cases are products of other
            cases, that product is *always* decomposed. For example, suppose
            that a single qubit depolarizing channel has a `Y5` case that
            produces four detection events `D0 D1 D2 D3`, an `X5` case that
            produces two detection events `D0 D1`, and a `Z5` case that produces
            two detection events `D2 D3`. Because `D0 D1 D2 D3` is the
            combination of `D0 D1` and `D2 D3`, the `Y5` case will be decomposed
            into `D0 D1 ^ D2 D3`. An important corner case here is the corner of
            the CSS surface code, where a Y error has two symptoms which is
            graphlike but because the intra-channel strategy is aggressive the
            Y error will still be decomposed into X and Z pieces. This can keep
            the X and Z decoding graphs disjoint.

            The *inter-channel* strategy is used when an error component is
            still not graphlike after the intra-channel strategy was applied.
            This strategy searches over all other error mechanisms looking for a
            combination that explains the error. If
            `--block_decompose_from_introducing_remnant_edges` is specified then
            this must be an exact match, otherwise the match can omit up to two
            of the symptoms in the error (resulting in the producing of a
            "remnant edge").

            Note that the code implementing these strategies does not special
            case any Pauli basis. For example, it does not prefer to decompose
            Y into X*Z as opposed to X into Y*Z. It also does not prefer to
            decompose YY into IY*YI as opposed to IY into YY*YI. The code
            operates purely in terms of the sets of symptoms produced by the
            various cases, with little regard for how those sets were produced.

            If these strategies fail to decompose error into graphlike pieces,
            Stim will throw an error saying it failed to find a satisfying
            decomposition.
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--fold_loops",
            "bool",
            "false",
            {"[none]", "[switch]"},
            clean_doc_string(R"PARAGRAPH(
            Allows the output to contain `repeat` blocks.

            This flag substantially improves performance on circuits with
            `REPEAT` blocks with large repetition counts. The analysis will take
            less time and the output will be more compact. This option is only
            OFF by default to maintain strict backwards compatibility of the
            output.

            When a circuit contains a `REPEAT` block, the structure of the
            detectors often settles into a form that is identical from iteration
            to iteration. Specifying the `--fold_loops` option tells Stim to
            watch for periodicity in the structure of detectors by using a
            "tortoise and hare" algorithm (see
            https://en.wikipedia.org/wiki/Cycle_detection ).
            This improves the asymptotic complexity of analyzing the loop from
            O(total_repetitions) to O(cycle_period).

            Note that, although logical observables can "cross" from the end of
            the loop to the start of the loop without preventing loop folding,
            detectors CANNOT. If there is any detector introduced after the
            loop, whose sensitivity region extends to before the loop, loop
            folding will fail and the code will fall back to flattening the
            loop. This is disastrous for loops with huge repetition counts (e.g.
            in the billions) because in that case loop folding is the difference
            between the error analysis finishing in seconds instead of in days.
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--ignore_decomposition_failures",
            "bool",
            "false",
            {"[none]", "[switch]"},
            clean_doc_string(R"PARAGRAPH(
            Allows non-graphlike errors into the output when decomposing errors.

            Irrelevant unless `--decompose_errors` is specified.

            Without `--ignore_decomposition_failures`, circuit errors that fail
            to decompose into graphlike detector error model errors will cause
            an error and abort the conversion process.

            When `--ignore_decomposition_failures` is specified, circuit errors
            that fail to decompose into graphlike detector error model errors
            produce non-graphlike detector error models. Whatever processes
            the detector error model is then responsible for dealing with the
            undecomposed errors (e.g. a tool may choose to simply ignore them).
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--in",
            "filepath",
            "{stdin}",
            {"[none]", "filepath"},
            clean_doc_string(R"PARAGRAPH(
            Chooses the stim circuit file to read the circuit to convert from.

            By default, the circuit is read from stdin. When `--in $FILEPATH` is
            specified, the circuit is instead read from the file at $FILEPATH.

            The input should be a stim circuit. See:
            https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--out",
            "filepath",
            "{stdout}",
            {"[none]", "filepath"},
            clean_doc_string(R"PARAGRAPH(
            Chooses where to write the output detector error model.

            By default, the output is written to stdout. When `--out $FILEPATH`
            is specified, the output is instead written to the file at $FILEPATH.

            The output is a stim detector error model. See:
            https://github.com/quantumlib/Stim/blob/main/doc/file_format_dem_detector_error_model.md
        )PARAGRAPH"),
        });

    return result;
}
