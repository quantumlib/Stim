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

#include "stim/circuit/circuit.pybind.h"

#include "stim/circuit/circuit_gate_target.pybind.h"
#include "stim/circuit/circuit_instruction.pybind.h"
#include "stim/circuit/circuit_repeat_block.pybind.h"
#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_color_code.h"
#include "stim/gen/gen_rep_code.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/py/base.pybind.h"
#include "stim/py/compiled_detector_sampler.pybind.h"
#include "stim/py/compiled_measurement_sampler.pybind.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/simulators/measurements_to_detection_events.pybind.h"

using namespace stim;

std::string circuit_repr(const Circuit &self) {
    if (self.operations.empty()) {
        return "stim.Circuit()";
    }
    std::stringstream ss;
    ss << "stim.Circuit('''\n";
    print_circuit(ss, self, "    ");
    ss << "\n''')";
    return ss.str();
}

void pybind_circuit(pybind11::module &m) {
    auto c = pybind11::class_<Circuit>(
        m,
        "Circuit",
        clean_doc_string(u8R"DOC(
            A mutable stabilizer circuit.

            Examples:
                >>> import stim
                >>> c = stim.Circuit()
                >>> c.append_operation("X", [0])
                >>> c.append_operation("M", [0])
                >>> c.compile_sampler().sample(shots=1)
                array([[1]], dtype=uint8)

                >>> stim.Circuit('''
                ...    H 0
                ...    CNOT 0 1
                ...    M 0 1
                ...    DETECTOR rec[-1] rec[-2]
                ... ''').compile_detector_sampler().sample(shots=1)
                array([[0]], dtype=uint8)

        )DOC")
            .data());

    pybind_circuit_repeat_block(m);
    pybind_circuit_gate_target(m);
    pybind_circuit_instruction(m);

    c.def(
        pybind11::init([](const char *stim_program_text) {
            Circuit self;
            self.append_from_text(stim_program_text);
            return self;
        }),
        pybind11::arg("stim_program_text") = "",
        clean_doc_string(u8R"DOC(
            Creates a stim.Circuit.

            Args:
                stim_program_text: Defaults to empty. Describes operations to append into the circuit.

            Examples:
                >>> import stim
                >>> empty = stim.Circuit()
                >>> not_empty = stim.Circuit('''
                ...    X 0
                ...    CNOT 0 1
                ...    M 1
                ... ''')
        )DOC")
            .data());

    c.def_property_readonly(
        "num_measurements",
        &Circuit::count_measurements,
        clean_doc_string(u8R"DOC(
            Counts the number of bits produced when sampling the circuit's measurements.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    M 0
                ...    REPEAT 100 {
                ...        M 0 1
                ...    }
                ... ''')
                >>> c.num_measurements
                201
        )DOC")
            .data());

    c.def_property_readonly(
        "num_detectors",
        &Circuit::count_detectors,
        clean_doc_string(u8R"DOC(
            Counts the number of bits produced when sampling the circuit's detectors.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    M 0
                ...    DETECTOR rec[-1]
                ...    REPEAT 100 {
                ...        M 0 1 2
                ...        DETECTOR rec[-1]
                ...        DETECTOR rec[-2]
                ...    }
                ... ''')
                >>> c.num_detectors
                201
        )DOC")
            .data());

    c.def_property_readonly(
        "num_observables",
        &Circuit::count_observables,
        clean_doc_string(u8R"DOC(
            Counts the number of bits produced when sampling the circuit's logical observables.

            This is one more than the largest observable index given to OBSERVABLE_INCLUDE.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    M 0
                ...    OBSERVABLE_INCLUDE(2) rec[-1]
                ...    OBSERVABLE_INCLUDE(5) rec[-1]
                ... ''')
                >>> c.num_observables
                6
        )DOC")
            .data());

    c.def_property_readonly(
        "num_qubits",
        &Circuit::count_qubits,
        clean_doc_string(u8R"DOC(
            Counts the number of qubits used when simulating the circuit.

            This is always one more than the largest qubit index used by the circuit.

            Examples:
                >>> import stim
                >>> stim.Circuit('''
                ...    X 0
                ...    M 0 1
                ... ''').num_qubits
                2
                >>> stim.Circuit('''
                ...    X 0
                ...    M 0 1
                ...    H 100
                ... ''').num_qubits
                101
        )DOC")
            .data());

    c.def_property_readonly(
        "num_sweep_bits",
        &Circuit::count_sweep_bits,
        clean_doc_string(u8R"DOC(
            Returns the number of sweep bits needed to completely configure the circuit.

            This is always one more than the largest sweep bit index used by the circuit.

            Examples:
                >>> import stim
                >>> stim.Circuit('''
                ...    CX sweep[2] 0
                ... ''').num_sweep_bits
                3
                >>> stim.Circuit('''
                ...    CZ sweep[5] 0
                ...    CX sweep[2] 0
                ... ''').num_sweep_bits
                6
        )DOC")
            .data());

    c.def(
        "compile_sampler",
        &py_init_compiled_sampler,
        pybind11::kw_only(),
        pybind11::arg("skip_reference_sample") = false,
        pybind11::arg("seed") = pybind11::none(),
        clean_doc_string(u8R"DOC(
            Returns a CompiledMeasurementSampler, which can quickly batch sample measurements, for the circuit.

            Args:
                skip_reference_sample: Defaults to False. When set to True, the reference sample used by the sampler is
                    initialized to all-zeroes instead of being collected from the circuit. This means that the results
                    returned by the sampler are actually whether or not each measurement was *flipped*, instead of true
                    measurement results.

                    Forcing an all-zero reference sample is useful when you are only interested in error propagation and
                    don't want to have to deal with the fact that some measurements want to be On when no errors occur.
                    It is also useful when you know for sure that the all-zero result is actually a possible result from
                    the circuit (under noiseless execution), meaning it is a valid reference sample as good as any
                    other. Computing the reference sample is the most time consuming and memory intensive part of
                    simulating the circuit, so promising that the simulator can safely skip that step is an effective
                    optimization.
                seed: PARTIALLY determines simulation results by deterministically seeding the random number generator.
                    Must be None or an integer in range(2**64).

                    Defaults to None. When set to None, a prng seeded by system entropy is used.

                    When set to an integer, making the exact same series calls on the exact same machine with the exact
                    same version of Stim will produce the exact same simulation results.

                    CAUTION: simulation results *WILL NOT* be consistent between versions of Stim. This restriction is
                    present to make it possible to have future optimizations to the random sampling, and is enforced by
                    introducing intentional differences in the seeding strategy from version to version.

                    CAUTION: simulation results *MAY NOT* be consistent across machines that differ in the width of
                    supported SIMD instructions. For example, using the same seed on a machine that supports AVX
                    instructions and one that only supports SSE instructions may produce different simulation results.

                    CAUTION: simulation results *MAY NOT* be consistent if you vary how many shots are taken. For
                    example, taking 10 shots and then 90 shots will give different results from taking 100 shots in one
                    call.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    X 2
                ...    M 0 1 2
                ... ''')
                >>> s = c.compile_sampler()
                >>> s.sample(shots=1)
                array([[0, 0, 1]], dtype=uint8)
        )DOC")
            .data());

    c.def(
        "compile_m2d_converter",
        &py_init_compiled_measurements_to_detection_events_converter,
        pybind11::kw_only(),
        pybind11::arg("skip_reference_sample") = false,
        clean_doc_string(u8R"DOC(
            Returns an object that can efficiently convert measurements into detection events for the given circuit.

            The converter uses a noiseless reference sample, collected from the circuit using stim's Tableau simulator
            during initialization of the converter, as a baseline for determining what the expected value of a detector
            is.

            Note that the expected behavior of gauge detectors (detectors that are not actually deterministic under
            noiseless execution) can vary depending on the reference sample. Stim mitigates this by always generating
            the same reference sample for a given circuit.

            Args:
                skip_reference_sample: Defaults to False. When set to True, the reference sample used by the converter
                    is initialized to all-zeroes instead of being collected from the circuit. This should only be used
                    if it's known that the all-zeroes sample is actually a possible result from the circuit (under
                    noiseless execution).

            Returns:
                An initialized stim.CompiledMeasurementsToDetectionEventsConverter.

            Examples:
                >>> import stim
                >>> import numpy as np
                >>> converter = stim.Circuit('''
                ...    X 0
                ...    M 0
                ...    DETECTOR rec[-1]
                ... ''').compile_m2d_converter()
                >>> converter.convert(
                ...     measurements=np.array([[0], [1]], dtype=np.bool8),
                ...     append_observables=False,
                ... )
                array([[ True],
                       [False]])
        )DOC")
            .data());

    c.def(
        "compile_detector_sampler",
        &py_init_compiled_detector_sampler,
        pybind11::kw_only(),
        pybind11::arg("seed") = pybind11::none(),
        clean_doc_string(u8R"DOC(
            Returns a CompiledDetectorSampler, which can quickly batch sample detection events, for the circuit.

            Args:
                seed: PARTIALLY determines simulation results by deterministically seeding the random number generator.
                    Must be None or an integer in range(2**64).

                    Defaults to None. When set to None, a prng seeded by system entropy is used.

                    When set to an integer, making the exact same series calls on the exact same machine with the exact
                    same version of Stim will produce the exact same simulation results.

                    CAUTION: simulation results *WILL NOT* be consistent between versions of Stim. This restriction is
                    present to make it possible to have future optimizations to the random sampling, and is enforced by
                    introducing intentional differences in the seeding strategy from version to version.

                    CAUTION: simulation results *MAY NOT* be consistent across machines that differ in the width of
                    supported SIMD instructions. For example, using the same seed on a machine that supports AVX
                    instructions and one that only supports SSE instructions may produce different simulation results.

                    CAUTION: simulation results *MAY NOT* be consistent if you vary how many shots are taken. For
                    example, taking 10 shots and then 90 shots will give different results from taking 100 shots in one
                    call.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    H 0
                ...    CNOT 0 1
                ...    M 0 1
                ...    DETECTOR rec[-1] rec[-2]
                ... ''')
                >>> s = c.compile_detector_sampler()
                >>> s.sample(shots=1)
                array([[0]], dtype=uint8)
        )DOC")
            .data());

    c.def(
        "clear",
        &Circuit::clear,
        clean_doc_string(u8R"DOC(
            Clears the contents of the circuit.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    X 0
                ...    Y 1 2
                ... ''')
                >>> c.clear()
                >>> c
                stim.Circuit()
        )DOC")
            .data());

    c.def(
        "__iadd__",
        &Circuit::operator+=,
        pybind11::arg("second"),
        clean_doc_string(u8R"DOC(
            Appends a circuit into the receiving circuit (mutating it).

            Examples:
                >>> import stim
                >>> c1 = stim.Circuit('''
                ...    X 0
                ...    Y 1 2
                ... ''')
                >>> c2 = stim.Circuit('''
                ...    M 0 1 2
                ... ''')
                >>> c1 += c2
                >>> print(c1)
                X 0
                Y 1 2
                M 0 1 2
        )DOC")
            .data());

    c.def(
        "flattened_operations",
        [](Circuit &self) {
            pybind11::list result;
            self.for_each_operation([&](const Operation &op) {
                pybind11::list args;
                pybind11::list targets;
                for (auto t : op.target_data.targets) {
                    auto v = t.qubit_value();
                    if (t.data & TARGET_INVERTED_BIT) {
                        targets.append(pybind11::make_tuple("inv", v));
                    } else if (t.data & (TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT)) {
                        if (!(t.data & TARGET_PAULI_Z_BIT)) {
                            targets.append(pybind11::make_tuple("X", v));
                        } else if (!(t.data & TARGET_PAULI_X_BIT)) {
                            targets.append(pybind11::make_tuple("Z", v));
                        } else {
                            targets.append(pybind11::make_tuple("Y", v));
                        }
                    } else if (t.data & TARGET_RECORD_BIT) {
                        targets.append(pybind11::make_tuple("rec", -(long long)v));
                    } else if (t.data & TARGET_SWEEP_BIT) {
                        targets.append(pybind11::make_tuple("sweep", v));
                    } else {
                        targets.append(pybind11::int_(v));
                    }
                }
                for (auto t : op.target_data.args) {
                    args.append(t);
                }
                if (op.target_data.args.size() == 0) {
                    // Backwards compatibility.
                    result.append(pybind11::make_tuple(op.gate->name, targets, 0));
                } else if (op.target_data.args.size() == 1) {
                    // Backwards compatibility.
                    result.append(pybind11::make_tuple(op.gate->name, targets, op.target_data.args[0]));
                } else {
                    result.append(pybind11::make_tuple(op.gate->name, targets, args));
                }
            });
            return result;
        },
        clean_doc_string(u8R"DOC(
            Flattens the circuit's operations into a list.

            The operations within repeat blocks are actually repeated in the output.

            Returns:
                A List[Tuple[name, targets, arg]] of the operations in the circuit.
                    name: A string with the gate's name.
                    targets: A list of things acted on by the gate. Each thing can be:
                        int: The index of a qubit.
                        Tuple["inv", int]: The index of a qubit to measure with an inverted result.
                        Tuple["rec", int]: A measurement record target like `rec[-1]`.
                        Tuple["X", int]: A Pauli X operation to apply during a correlated error.
                        Tuple["Y", int]: A Pauli Y operation to apply during a correlated error.
                        Tuple["Z", int]: A Pauli Z operation to apply during a correlated error.
                    arg: The gate's numeric argument. For most gates this is just 0. For noisy
                        gates this is the probability of the noise being applied.

            Examples:
                >>> import stim
                >>> stim.Circuit('''
                ...    H 0
                ...    X_ERROR(0.125) 1
                ...    M 0 !1
                ... ''').flattened_operations()
                [('H', [0], 0), ('X_ERROR', [1], 0.125), ('M', [0, ('inv', 1)], 0)]

                >>> stim.Circuit('''
                ...    REPEAT 2 {
                ...        H 6
                ...    }
                ... ''').flattened_operations()
                [('H', [6], 0), ('H', [6], 0)]
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self, "Determines if two circuits have identical contents.");
    c.def(pybind11::self != pybind11::self, "Determines if two circuits have non-identical contents.");

    c.def(
        "__add__",
        &Circuit::operator+,
        pybind11::arg("second"),
        clean_doc_string(u8R"DOC(
            Creates a circuit by appending two circuits.

            Examples:
                >>> import stim
                >>> c1 = stim.Circuit('''
                ...    X 0
                ...    Y 1 2
                ... ''')
                >>> c2 = stim.Circuit('''
                ...    M 0 1 2
                ... ''')
                >>> print(c1 + c2)
                X 0
                Y 1 2
                M 0 1 2
        )DOC")
            .data());

    c.def(
        "__imul__",
        &Circuit::operator*=,
        pybind11::arg("repetitions"),
        clean_doc_string(u8R"DOC(
            Mutates the circuit by putting its contents into a REPEAT block.

            Special case: if the repetition count is 0, the circuit is cleared.
            Special case: if the repetition count is 1, nothing happens.

            Args:
                repetitions: The number of times the REPEAT block should repeat.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    X 0
                ...    Y 1 2
                ... ''')
                >>> c *= 3
                >>> print(c)
                REPEAT 3 {
                    X 0
                    Y 1 2
                }
        )DOC")
            .data());

    c.def(
        "__mul__",
        &Circuit::operator*,
        pybind11::arg("repetitions"),
        clean_doc_string(u8R"DOC(
            Returns a circuit with a REPEAT block containing the current circuit's instructions.

            Special case: if the repetition count is 0, an empty circuit is returned.
            Special case: if the repetition count is 1, an equal circuit with no REPEAT block is returned.

            Args:
                repetitions: The number of times the REPEAT block should repeat.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    X 0
                ...    Y 1 2
                ... ''')
                >>> print(c * 3)
                REPEAT 3 {
                    X 0
                    Y 1 2
                }
        )DOC")
            .data());

    c.def(
        "__rmul__",
        &Circuit::operator*,
        pybind11::arg("repetitions"),
        clean_doc_string(u8R"DOC(
            Returns a circuit with a REPEAT block containing the current circuit's instructions.

            Special case: if the repetition count is 0, an empty circuit is returned.
            Special case: if the repetition count is 1, an equal circuit with no REPEAT block is returned.

            Args:
                repetitions: The number of times the REPEAT block should repeat.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    X 0
                ...    Y 1 2
                ... ''')
                >>> print(3 * c)
                REPEAT 3 {
                    X 0
                    Y 1 2
                }
        )DOC")
            .data());

    c.def(
        "append_operation",
        [](Circuit &self,
           const pybind11::object &obj,
           const std::vector<pybind11::object> &targets,
           pybind11::object arg) {
            if (pybind11::isinstance<pybind11::str>(obj)) {
                const std::string &gate_name = pybind11::cast<std::string>(obj);
                if (arg.is(pybind11::none())) {
                    if (GATE_DATA.at(gate_name).arg_count == 1) {
                        arg = pybind11::make_tuple(0.0);
                    } else {
                        arg = pybind11::make_tuple();
                    }
                }
                std::vector<uint32_t> raw_targets;
                for (const auto &t : targets) {
                    raw_targets.push_back(obj_to_gate_target(t).data);
                }
                try {
                    auto d = pybind11::cast<double>(arg);
                    self.append_op(gate_name, raw_targets, d);
                    return;
                } catch (const pybind11::cast_error &ex) {
                }
                try {
                    auto args = pybind11::cast<std::vector<double>>(arg);
                    self.append_op(gate_name, raw_targets, args);
                    return;
                } catch (const pybind11::cast_error &ex) {
                }
                throw std::invalid_argument("Arg must be a double or sequence of doubles.");
            } else if (pybind11::isinstance<CircuitInstruction>(obj)) {
                if (!targets.empty() || !arg.is_none()) {
                    throw std::invalid_argument(
                        "Can't specify `targets` or `arg` when appending a stim.CircuitInstruction.");
                }

                const CircuitInstruction &instruction = pybind11::cast<CircuitInstruction>(obj);
                self.append_op(instruction.gate.name, instruction.raw_targets(), instruction.gate_args);
            } else if (pybind11::isinstance<CircuitRepeatBlock>(obj)) {
                if (!targets.empty() || !arg.is_none()) {
                    throw std::invalid_argument(
                        "Can't specify `targets` or `arg` when appending a stim.CircuitRepeatBlock.");
                }

                const CircuitRepeatBlock &block = pybind11::cast<CircuitRepeatBlock>(obj);
                self.append_repeat_block(block.repeat_count, block.body);
            } else {
                throw std::invalid_argument(
                    "First argument of append_operation must be a str (a gate name), "
                    "a stim.CircuitInstruction, "
                    "or a stim.Circuit");
            }
        },
        pybind11::arg("name"),
        pybind11::arg("targets") = pybind11::make_tuple(),
        pybind11::arg("arg") = pybind11::none(),
        clean_doc_string(u8R"DOC(
            Appends an operation into the circuit.

            Examples:
                >>> import stim
                >>> c = stim.Circuit()
                >>> c.append_operation("X", [0])
                >>> c.append_operation("H", [0, 1])
                >>> c.append_operation("M", [0, stim.target_inv(1)])
                >>> c.append_operation("CNOT", [stim.target_rec(-1), 0])
                >>> c.append_operation("X_ERROR", [0], 0.125)
                >>> c.append_operation("CORRELATED_ERROR", [stim.target_x(0), stim.target_y(2)], 0.25)
                >>> print(c)
                X 0
                H 0 1
                M 0 !1
                CX rec[-1] 0
                X_ERROR(0.125) 0
                E(0.25) X0 Y2

            Args:
                name: The name of the operation's gate (e.g. "H" or "M" or "CNOT").

                    This argument can also be set to a `stim.CircuitInstruction` or `stim.CircuitInstructionBlock`, which
                    results in the instruction or block being appended to the circuit. The other arguments (targets and
                    arg) can't be specified when doing so.

                    (The argument name `name` is no longer quite right, but being kept for backwards compatibility.)
                targets: The gate targets. Gates implicitly broadcast over their targets.
                arg: A double or list of doubles parameterizing the gate. Different gates take different arguments. For
                    example, X_ERROR takes a probability, OBSERVABLE_INCLUDE takes an observable index, and PAULI_CHANNEL_1
                    takes three disjoint probabilities. For backwards compatibility reasons, defaults to (0,) for gates
                    that take exactly one argument. Otherwise defaults to no arguments.
        )DOC")
            .data());

    c.def(
        "append_from_stim_program_text",
        [](Circuit &self, const char *text) {
            self.append_from_text(text);
        },
        pybind11::arg("stim_program_text"),
        clean_doc_string(u8R"DOC(
            Appends operations described by a STIM format program into the circuit.

            Examples:
                >>> import stim
                >>> c = stim.Circuit()
                >>> c.append_from_stim_program_text('''
                ...    H 0  # comment
                ...    CNOT 0 2
                ...
                ...    M 2
                ...    CNOT rec[-1] 1
                ... ''')
                >>> print(c)
                H 0
                CX 0 2
                M 2
                CX rec[-1] 1

            Args:
                text: The STIM program text containing the circuit operations to append.
        )DOC")
            .data());

    c.def(
        "__str__",
        &Circuit::str,
        "Returns stim instructions (that can be saved to a file and parsed by stim) for the current circuit.");
    c.def(
        "__repr__",
        &circuit_repr,
        "Returns text that is a valid python expression evaluating to an equivalent `stim.Circuit`.");

    c.def(
        "copy",
        [](Circuit &self) {
            Circuit copy = self;
            return copy;
        },
        clean_doc_string(u8R"DOC(
            Returns a copy of the circuit. An independent circuit with the same contents.

            Examples:
                >>> import stim

                >>> c1 = stim.Circuit("H 0")
                >>> c2 = c1.copy()
                >>> c2 is c1
                False
                >>> c2 == c1
                True
        )DOC")
            .data());

    c.def_static(
        "generated",
        [](const std::string &type,
           size_t distance,
           size_t rounds,
           double after_clifford_depolarization,
           double before_round_data_depolarization,
           double before_measure_flip_probability,
           double after_reset_flip_probability) {
            auto r = type.find(':');
            std::string code;
            std::string task;
            if (r == std::string::npos) {
                code = "";
                task = type;
            } else {
                code = type.substr(0, r);
                task = type.substr(r + 1);
            }

            CircuitGenParameters params(rounds, distance, task);
            params.after_clifford_depolarization = after_clifford_depolarization;
            params.after_reset_flip_probability = after_reset_flip_probability;
            params.before_measure_flip_probability = before_measure_flip_probability;
            params.before_round_data_depolarization = before_round_data_depolarization;
            params.validate_params();

            if (code == "surface_code") {
                return generate_surface_code_circuit(params).circuit;
            } else if (code == "repetition_code") {
                return generate_rep_code_circuit(params).circuit;
            } else if (code == "color_code") {
                return generate_color_code_circuit(params).circuit;
            } else {
                throw std::invalid_argument(
                    "Unrecognized circuit type. Expected type to start with "
                    "'surface_code:', 'repetition_code:', or 'color_code:'.");
            }
        },
        pybind11::arg("code_task"),
        pybind11::kw_only(),
        pybind11::arg("distance"),
        pybind11::arg("rounds"),
        pybind11::arg("after_clifford_depolarization") = 0.0,
        pybind11::arg("before_round_data_depolarization") = 0.0,
        pybind11::arg("before_measure_flip_probability") = 0.0,
        pybind11::arg("after_reset_flip_probability") = 0.0,
        clean_doc_string(u8R"DOC(
            Generates common circuits.

            The generated circuits can include configurable noise.

            The generated circuits include DETECTOR and OBSERVABLE_INCLUDE annotations so that their detection events
            and logical observables can be sampled.

            The generated circuits include TICK annotations to mark the progression of time. (E.g. so that converting
            them using `stimcirq.stim_circuit_to_cirq_circuit` will produce a `cirq.Circuit` with the intended moment
            structure.)

            Args:
                code_task: A string identifying the type of circuit to generate. Available types are:
                    - `repetition_code:memory`
                    - `surface_code:rotated_memory_x`
                    - `surface_code:rotated_memory_z`
                    - `surface_code:unrotated_memory_x`
                    - `surface_code:unrotated_memory_z`
                    - `color_code:memory_xyz`
                distance: The desired code distance of the generated circuit. The code distance is the minimum number
                    of physical errors needed to cause a logical error. This parameter indirectly determines how many
                    qubits the generated circuit uses.
                rounds: How many times the measurement qubits in the generated circuit will be measured. Indirectly
                    determines the duration of the generated circuit.
                after_clifford_depolarization: Defaults to 0. The probability (p) of `DEPOLARIZE1(p)` operations to add
                    after every single-qubit Clifford operation and `DEPOLARIZE2(p)` operations to add after every
                    two-qubit Clifford operation. The after-Clifford depolarizing operations are only included if this
                    probability is not 0.
                before_round_data_depolarization: Defaults to 0. The probability (p) of `DEPOLARIZE1(p)` operations to
                    apply to every data qubit at the start of a round of stabilizer measurements. The start-of-round
                    depolarizing operations are only included if this probability is not 0.
                before_measure_flip_probability: Defaults to 0. The probability (p) of `X_ERROR(p)` operations applied
                    to qubits before each measurement (X basis measurements use `Z_ERROR(p)` instead). The
                    before-measurement flips are only included if this probability is not 0.
                after_reset_flip_probability: Defaults to 0. The probability (p) of `X_ERROR(p)` operations applied
                    to qubits after each reset (X basis resets use `Z_ERROR(p)` instead). The after-reset flips are only
                    included if this probability is not 0.

            Returns:
                The generated circuit.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit.generated(
                ...     "repetition_code:memory",
                ...     distance=4,
                ...     rounds=10000,
                ...     after_clifford_depolarization=0.0125)
                >>> print(circuit)
                R 0 1 2 3 4 5 6
                TICK
                CX 0 1 2 3 4 5
                DEPOLARIZE2(0.0125) 0 1 2 3 4 5
                TICK
                CX 2 1 4 3 6 5
                DEPOLARIZE2(0.0125) 2 1 4 3 6 5
                TICK
                MR 1 3 5
                DETECTOR(1, 0) rec[-3]
                DETECTOR(3, 0) rec[-2]
                DETECTOR(5, 0) rec[-1]
                REPEAT 9999 {
                    TICK
                    CX 0 1 2 3 4 5
                    DEPOLARIZE2(0.0125) 0 1 2 3 4 5
                    TICK
                    CX 2 1 4 3 6 5
                    DEPOLARIZE2(0.0125) 2 1 4 3 6 5
                    TICK
                    MR 1 3 5
                    SHIFT_COORDS(0, 1)
                    DETECTOR(1, 0) rec[-3] rec[-6]
                    DETECTOR(3, 0) rec[-2] rec[-5]
                    DETECTOR(5, 0) rec[-1] rec[-4]
                }
                M 0 2 4 6
                DETECTOR(1, 1) rec[-3] rec[-4] rec[-7]
                DETECTOR(3, 1) rec[-2] rec[-3] rec[-6]
                DETECTOR(5, 1) rec[-1] rec[-2] rec[-5]
                OBSERVABLE_INCLUDE(0) rec[-1]
        )DOC")
            .data());

    c.def(
        "__len__",
        [](const Circuit &self) {
            return self.operations.size();
        },
        clean_doc_string(u8R"DOC(
            Returns the number of top-level instructions and blocks in the circuit.

            Instructions inside of blocks are not included in this count.

            Examples:
                >>> import stim
                >>> len(stim.Circuit())
                0
                >>> len(stim.Circuit('''
                ...    X 0
                ...    X_ERROR(0.5) 1 2
                ...    TICK
                ...    M 0
                ...    DETECTOR rec[-1]
                ... '''))
                5
                >>> len(stim.Circuit('''
                ...    REPEAT 100 {
                ...        X 0
                ...        Y 1 2
                ...    }
                ... '''))
                1
        )DOC")
            .data());

    c.def(
        "__getitem__",
        [](const Circuit &self, pybind11::object index_or_slice) -> pybind11::object {
            pybind11::ssize_t index, step, slice_length;
            if (normalize_index_or_slice(index_or_slice, self.operations.size(), &index, &step, &slice_length)) {
                return pybind11::cast(self.py_get_slice(index, step, slice_length));
            }

            auto &op = self.operations[index];
            if (op.gate->id == gate_name_to_id("REPEAT")) {
                return pybind11::cast(
                    CircuitRepeatBlock{op_data_rep_count(op.target_data), op_data_block_body(self, op.target_data)});
            }
            std::vector<GateTarget> targets;
            for (const auto &e : op.target_data.targets) {
                targets.push_back(GateTarget(e));
            }
            std::vector<double> args;
            for (const auto &e : op.target_data.args) {
                args.push_back(e);
            }
            return pybind11::cast(CircuitInstruction(*op.gate, targets, args));
        },
        pybind11::arg("index_or_slice"),
        clean_doc_string(u8R"DOC(
            Returns copies of instructions from the circuit.

            Args:
                index_or_slice: An integer index picking out an instruction to return, or a slice picking out a range
                    of instructions to return as a circuit.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit('''
                ...    X 0
                ...    X_ERROR(0.5) 1 2
                ...    REPEAT 100 {
                ...        X 0
                ...        Y 1 2
                ...    }
                ...    TICK
                ...    M 0
                ...    DETECTOR rec[-1]
                ... ''')
                >>> circuit[1]
                stim.CircuitInstruction('X_ERROR', [stim.GateTarget(1), stim.GateTarget(2)], [0.5])
                >>> circuit[2]
                stim.CircuitRepeatBlock(100, stim.Circuit('''
                    X 0
                    Y 1 2
                '''))
                >>> circuit[1::2]
                stim.Circuit('''
                    X_ERROR(0.5) 1 2
                    TICK
                    DETECTOR rec[-1]
                ''')
        )DOC")
            .data());

    c.def(
        "detector_error_model",
        [](Circuit &self,
           bool decompose_errors,
           bool flatten_loops,
           bool allow_gauge_detectors,
           double approximate_disjoint_errors) {
            return ErrorAnalyzer::circuit_to_detector_error_model(
                self, decompose_errors, !flatten_loops, allow_gauge_detectors, approximate_disjoint_errors);
        },
        pybind11::kw_only(),
        pybind11::arg("decompose_errors") = false,
        pybind11::arg("flatten_loops") = false,
        pybind11::arg("allow_gauge_detectors") = false,
        pybind11::arg("approximate_disjoint_errors") = false,
        clean_doc_string(u8R"DOC(
            Returns a stim.DetectorErrorModel describing the error processes in the circuit.

            Args:
                decompose_errors: Defaults to false. When set to true, the error analysis attempts to decompose the
                    components of composite error mechanisms (such as depolarization errors) into simpler errors, and
                    suggest this decomposition via `stim.target_separator()` between the components. For example, in an
                    XZ surface code, single qubit depolarization has a Y error term which can be decomposed into simpler
                    X and Z error terms. Decomposition fails (causing this method to throw) if it's not possible to
                    decompose large errors into simple errors that affect at most two detectors.
                flatten_loops: Defaults to false. When set to true, the output will not contain any `repeat` blocks.
                    When set to false, the error analysis watches for loops in the circuit reaching a periodic steady
                    state with respect to the detectors being introduced, the error mechanisms that affect them, and the
                    locations of the logical observables. When it identifies such a steady state, it outputs a repeat
                    block. This is massively more efficient than flattening for circuits that contain loops, but creates
                    a more complex output.
                allow_gauge_detectors: Defaults to false. When set to false, the error analysis verifies that detectors
                    in the circuit are actually deterministic under noiseless execution of the circuit. When set to
                    true, these detectors are instead considered to be part of degrees freedom that can be removed from
                    the error model. For example, if detectors D1 and D3 both anti-commute with a reset, then the error
                    model has a gauge `error(0.5) D1 D3`. When gauges are identified, one of the involved detectors is
                    removed from the system using Gaussian elimination.

                    Note that logical observables are still verified to be deterministic, even if this option is set.
                approximate_disjoint_errors: Defaults to false. When set to false, composite error mechanisms with
                    disjoint components (such as `PAULI_CHANNEL_1(0.1, 0.2, 0.0)`) can cause the error analysis to throw
                    exceptions (because detector error models can only contain independent error mechanisms). When set
                    to true, the probabilities of the disjoint cases are instead assumed to be independent
                    probabilities. For example, a ``PAULI_CHANNEL_1(0.1, 0.2, 0.0)` becomes equivalent to an
                    `X_ERROR(0.1)` followed by a `Z_ERROR(0.2)`. This assumption is an approximation, but it is a good
                    approximation for small probabilities.

                    This argument can also be set to a probability between 0 and 1, setting a threshold below which the
                    approximation is acceptable. Any error mechanisms that have a component probability above the
                    threshold will cause an exception to be thrown.

            Examples:
                >>> import stim

                >>> stim.Circuit('''
                ...     X_ERROR(0.125) 0
                ...     X_ERROR(0.25) 1
                ...     CORRELATED_ERROR(0.375) X0 X1
                ...     M 0 1
                ...     DETECTOR rec[-2]
                ...     DETECTOR rec[-1]
                ... ''').detector_error_model()
                stim.DetectorErrorModel('''
                    error(0.125) D0
                    error(0.375) D0 D1
                    error(0.25) D1
                ''')
        )DOC")
            .data());
}
