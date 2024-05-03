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

#include <fstream>

#include "stim/circuit/circuit_instruction.pybind.h"
#include "stim/circuit/circuit_repeat_block.pybind.h"
#include "stim/circuit/gate_target.pybind.h"
#include "stim/cmd/command_diagram.pybind.h"
#include "stim/dem/detector_error_model_target.pybind.h"
#include "stim/diagram/detector_slice/detector_slice_set.h"
#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_color_code.h"
#include "stim/gen/gen_rep_code.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/io/raii_file.h"
#include "stim/py/base.pybind.h"
#include "stim/py/compiled_detector_sampler.pybind.h"
#include "stim/py/compiled_measurement_sampler.pybind.h"
#include "stim/py/numpy.pybind.h"
#include "stim/search/search.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/simulators/error_matcher.h"
#include "stim/simulators/measurements_to_detection_events.pybind.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/stabilizers/flow.h"
#include "stim/util_top/circuit_inverse_qec.h"
#include "stim/util_top/circuit_to_detecting_regions.h"
#include "stim/util_top/circuit_vs_tableau.h"
#include "stim/util_top/count_determined_measurements.h"
#include "stim/util_top/export_crumble_url.h"
#include "stim/util_top/export_qasm.h"
#include "stim/util_top/export_quirk_url.h"
#include "stim/util_top/has_flow.h"
#include "stim/util_top/simplified_circuit.h"
#include "stim/util_top/transform_without_feedback.h"

using namespace stim;
using namespace stim_pybind;

std::set<DemTarget> py_dem_filter_to_dem_target_set(
    const Circuit &circuit, const CircuitStats &stats, const pybind11::object &included_targets_filter) {
    std::set<DemTarget> result;
    auto add_all_dets = [&]() {
        for (uint64_t k = 0; k < stats.num_detectors; k++) {
            result.insert(DemTarget::relative_detector_id(k));
        }
    };
    auto add_all_obs = [&]() {
        for (uint64_t k = 0; k < stats.num_observables; k++) {
            result.insert(DemTarget::observable_id(k));
        }
    };

    bool has_coords = false;
    std::map<uint64_t, std::vector<double>> cached_coords;
    auto get_coords_cached = [&]() -> const std::map<uint64_t, std::vector<double>> & {
        std::set<uint64_t> all_dets;
        for (uint64_t k = 0; k < stats.num_detectors; k++) {
            all_dets.insert(k);
        }
        if (!has_coords) {
            cached_coords = circuit.get_detector_coordinates(all_dets);
            has_coords = true;
        }
        return cached_coords;
    };

    if (included_targets_filter.is_none()) {
        add_all_dets();
        add_all_obs();
        return result;
    }
    for (const auto &filter : included_targets_filter) {
        bool fail = false;
        if (pybind11::isinstance<ExposedDemTarget>(filter)) {
            result.insert(pybind11::cast<ExposedDemTarget>(filter));
        } else if (pybind11::isinstance<pybind11::str>(filter)) {
            std::string_view s = pybind11::cast<std::string_view>(filter);
            if (s == "D") {
                add_all_dets();
            } else if (s == "L") {
                add_all_obs();
            } else if (s.starts_with("D") || s.starts_with("L")) {
                result.insert(DemTarget::from_text(s));
            } else {
                fail = true;
            }
        } else {
            std::vector<double> prefix;
            for (auto e : filter) {
                if (pybind11::isinstance<pybind11::int_>(e) || pybind11::isinstance<pybind11::float_>(e)) {
                    prefix.push_back(pybind11::cast<double>(e));
                } else {
                    fail = true;
                    break;
                }
            }
            if (!fail) {
                for (const auto &[target, coord] : get_coords_cached()) {
                    if (coord.size() >= prefix.size()) {
                        bool match = true;
                        for (size_t k = 0; k < prefix.size(); k++) {
                            match &= prefix[k] == coord[k];
                        }
                        if (match) {
                            result.insert(DemTarget::relative_detector_id(target));
                        }
                    }
                }
            }
        }
        if (fail) {
            std::stringstream ss;
            ss << "Don't know how to interpret '";
            ss << pybind11::cast<std::string_view>(pybind11::repr(filter));
            ss << "' as a dem target filter.";
            throw std::invalid_argument(ss.str());
        }
    }
    return result;
}

std::string circuit_repr(const Circuit &self) {
    if (self.operations.empty()) {
        return "stim.Circuit()";
    }
    std::stringstream ss;
    ss << "stim.Circuit('''\n";
    print_circuit(ss, self, 4);
    ss << "\n''')";
    return ss.str();
}

std::vector<ExplainedError> circuit_shortest_graphlike_error(
    const Circuit &self, bool ignore_ungraphlike_errors, bool reduce_to_representative) {
    DetectorErrorModel dem =
        ErrorAnalyzer::circuit_to_detector_error_model(self, !ignore_ungraphlike_errors, true, false, 1, false, false);
    DetectorErrorModel filter = shortest_graphlike_undetectable_logical_error(dem, ignore_ungraphlike_errors);
    return ErrorMatcher::explain_errors_from_circuit(self, &filter, reduce_to_representative);
}

std::vector<ExplainedError> py_find_undetectable_logical_error(
    const Circuit &self,
    size_t dont_explore_detection_event_sets_with_size_above,
    size_t dont_explore_edges_with_degree_above,
    bool dont_explore_edges_increasing_symptom_degree,
    bool reduce_to_representative) {
    DetectorErrorModel dem = ErrorAnalyzer::circuit_to_detector_error_model(self, false, true, false, 1, false, false);
    DetectorErrorModel filter = stim::find_undetectable_logical_error(
        dem,
        dont_explore_detection_event_sets_with_size_above,
        dont_explore_edges_with_degree_above,
        dont_explore_edges_increasing_symptom_degree);
    return ErrorMatcher::explain_errors_from_circuit(self, &filter, reduce_to_representative);
}

std::string py_shortest_error_sat_problem(const Circuit &self, std::string format) {
    DetectorErrorModel dem = ErrorAnalyzer::circuit_to_detector_error_model(self, false, true, false, 1, false, false);
    return stim::shortest_error_sat_problem(dem, format);
}

std::string py_likeliest_error_sat_problem(const Circuit &self, int quantization, std::string format) {
    DetectorErrorModel dem = ErrorAnalyzer::circuit_to_detector_error_model(self, false, true, false, 1, false, false);
    return stim::likeliest_error_sat_problem(dem, quantization, format);
}

void circuit_append(
    Circuit &self,
    const pybind11::object &obj,
    const pybind11::object &targets,
    const pybind11::object &arg,
    bool backwards_compat) {
    // Extract single target or list of targets.
    std::vector<uint32_t> raw_targets;
    try {
        raw_targets.push_back(obj_to_gate_target(targets).data);
    } catch (const std::invalid_argument &ex) {
        for (const auto &t : targets) {
            raw_targets.push_back(handle_to_gate_target(t).data);
        }
    }

    if (pybind11::isinstance<pybind11::str>(obj)) {
        std::string_view gate_name = pybind11::cast<std::string_view>(obj);

        // Maintain backwards compatibility to when there was always exactly one argument.
        pybind11::object used_arg;
        if (!arg.is_none()) {
            used_arg = arg;
        } else if (backwards_compat && GATE_DATA.at(gate_name).arg_count == 1) {
            used_arg = pybind11::make_tuple(0.0);
        } else {
            used_arg = pybind11::make_tuple();
        }

        // Extract single argument or list of arguments.
        try {
            auto d = pybind11::cast<double>(used_arg);
            self.safe_append_ua(gate_name, raw_targets, d);
            return;
        } catch (const pybind11::cast_error &ex) {
        }
        try {
            auto args = pybind11::cast<std::vector<double>>(used_arg);
            self.safe_append_u(gate_name, raw_targets, args);
            return;
        } catch (const pybind11::cast_error &ex) {
        }
        throw std::invalid_argument("Arg must be a double or sequence of doubles.");
    } else if (pybind11::isinstance<PyCircuitInstruction>(obj)) {
        if (!raw_targets.empty() || !arg.is_none()) {
            throw std::invalid_argument("Can't specify `targets` or `arg` when appending a stim.CircuitInstruction.");
        }

        const PyCircuitInstruction &instruction = pybind11::cast<PyCircuitInstruction>(obj);
        self.safe_append(instruction.gate_type, instruction.targets, instruction.gate_args);
    } else if (pybind11::isinstance<CircuitRepeatBlock>(obj)) {
        if (!raw_targets.empty() || !arg.is_none()) {
            throw std::invalid_argument("Can't specify `targets` or `arg` when appending a stim.CircuitRepeatBlock.");
        }

        const CircuitRepeatBlock &block = pybind11::cast<CircuitRepeatBlock>(obj);
        self.append_repeat_block(block.repeat_count, block.body);
    } else {
        throw std::invalid_argument(
            "First argument of append_operation must be a str (a gate name), "
            "a stim.CircuitInstruction, "
            "or a stim.CircuitRepeatBlock");
    }
}
void circuit_append_backwards_compat(
    Circuit &self, const pybind11::object &obj, const pybind11::object &targets, const pybind11::object &arg) {
    circuit_append(self, obj, targets, arg, true);
}
void circuit_append_strict(
    Circuit &self, const pybind11::object &obj, const pybind11::object &targets, const pybind11::object &arg) {
    circuit_append(self, obj, targets, arg, false);
}

pybind11::class_<Circuit> stim_pybind::pybind_circuit(pybind11::module &m) {
    auto c = pybind11::class_<Circuit>(
        m,
        "Circuit",
        clean_doc_string(R"DOC(
            A mutable stabilizer circuit.

            The stim.Circuit class is arguably the most important object in the
            entire library. It is the interface through which you explain a
            noisy quantum computation to Stim, in order to do fast bulk sampling
            or fast error analysis.

            For example, suppose you want to use a matching-based decoder on a
            new quantum error correction construction. Stim can help you do this
            but the very first step is to create a circuit implementing the
            construction. Once you have the circuit you can then use methods like
            stim.Circuit.detector_error_model() to create an object that can be
            used to configure the decoder, or like
            stim.Circuit.compile_detector_sampler() to produce problems for the
            decoder to solve, or like stim.Circuit.shortest_graphlike_error() to
            check for mistakes in the implementation of the code.

            Examples:
                >>> import stim
                >>> c = stim.Circuit()
                >>> c.append("X", 0)
                >>> c.append("M", 0)
                >>> c.compile_sampler().sample(shots=1)
                array([[ True]])

                >>> stim.Circuit('''
                ...    H 0
                ...    CNOT 0 1
                ...    M 0 1
                ...    DETECTOR rec[-1] rec[-2]
                ... ''').compile_detector_sampler().sample(shots=1)
                array([[False]])

        )DOC")
            .data());

    return c;
}

uint64_t obj_to_abs_detector_id(const pybind11::handle &obj, bool fail) {
    try {
        return obj.cast<uint64_t>();
    } catch (const pybind11::cast_error &) {
    }
    try {
        ExposedDemTarget t = obj.cast<ExposedDemTarget>();
        if (t.is_relative_detector_id()) {
            return t.data;
        }
    } catch (const pybind11::cast_error &) {
    }
    if (!fail) {
        return UINT64_MAX;
    }

    std::stringstream ss;
    ss << "Expected a detector id but didn't get a stim.DemTarget or a uint64_t.";
    ss << " Got " << pybind11::repr(obj);
    throw std::invalid_argument(ss.str());
}

std::set<uint64_t> obj_to_abs_detector_id_set(
    const pybind11::object &obj, const std::function<size_t(void)> &get_num_detectors) {
    std::set<uint64_t> filter;
    if (obj.is_none()) {
        size_t n = get_num_detectors();
        for (size_t k = 0; k < n; k++) {
            filter.insert(k);
        }
    } else {
        uint64_t single = obj_to_abs_detector_id(obj, false);
        if (single != UINT64_MAX) {
            filter.insert(single);
        } else {
            for (const auto &e : obj) {
                filter.insert(obj_to_abs_detector_id(e, true));
            }
        }
    }
    return filter;
}

void stim_pybind::pybind_circuit_methods(pybind11::module &, pybind11::class_<Circuit> &c) {
    c.def(
        pybind11::init([](std::string_view stim_program_text) {
            Circuit self;
            self.append_from_text(stim_program_text);
            return self;
        }),
        pybind11::arg("stim_program_text") = "",
        clean_doc_string(R"DOC(
            Creates a stim.Circuit.

            Args:
                stim_program_text: Defaults to empty. Describes operations to append into
                    the circuit.

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
        clean_doc_string(R"DOC(
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
        clean_doc_string(R"DOC(
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
        "num_ticks",
        &Circuit::count_ticks,
        clean_doc_string(R"DOC(
            Counts the number of TICK instructions executed when running the circuit.

            TICKs in loops are counted once per iteration.

            Returns:
                The number of ticks executed by the circuit.

            Examples:
                >>> import stim

                >>> stim.Circuit().num_ticks
                0

                >>> stim.Circuit('''
                ...    TICK
                ... ''').num_ticks
                1

                >>> stim.Circuit('''
                ...    H 0
                ...    TICK
                ...    CX 0 1
                ...    TICK
                ... ''').num_ticks
                2

                >>> stim.Circuit('''
                ...    H 0
                ...    TICK
                ...    REPEAT 100 {
                ...        CX 0 1
                ...        TICK
                ...    }
                ... ''').num_ticks
                101
        )DOC")
            .data());

    c.def(
        "count_determined_measurements",
        &count_determined_measurements<MAX_BITWORD_WIDTH>,
        clean_doc_string(R"DOC(
            Counts the number of predictable measurements in the circuit.

            This method ignores any noise in the circuit.

            This method works by performing a tableau stabilizer simulation of the circuit
            and, before each measurement is simulated, checking if its expectation is
            non-zero.

            A measurement is predictable if its result can be predicted by using other
            measurements that have already been performed, assuming the circuit is executed
            without any noise.

            Note that, when multiple measurements occur at the same time, re-ordering the
            order they are resolved can change which specific measurements are predictable
            but won't change how many of them were predictable in total.

            The number of predictable measurements is a useful quantity because it's
            related to the number of detectors and observables that a circuit should
            declare. If circuit.num_detectors + circuit.num_observables is less than
            circuit.count_determined_measurements(), this is a warning sign that you've
            missed some detector declarations.

            The exact relationship between the number of determined measurements and the
            number of detectors and observables can differ from code to code. For example,
            the toric code has an extra redundant measurement compared to the surface code
            because in the toric code the last X stabilizer to be measured is equal to the
            product of all other X stabilizers even in the first round when initializing in
            the Z basis. Typically this relationship is not declared as a detector, because
            it's not local, or as an observable, because it doesn't store a qubit.

            Returns:
                The number of measurements that were predictable.

            Examples:
                >>> import stim

                >>> stim.Circuit('''
                ...     R 0
                ...     M 0
                ... ''').count_determined_measurements()
                1

                >>> stim.Circuit('''
                ...     R 0
                ...     H 0
                ...     M 0
                ... ''').count_determined_measurements()
                0

                >>> stim.Circuit('''
                ...     R 0 1
                ...     MZZ 0 1
                ...     MYY 0 1
                ...     MXX 0 1
                ... ''').count_determined_measurements()
                2

                >>> circuit = stim.Circuit.generated(
                ...     "surface_code:rotated_memory_x",
                ...     distance=5,
                ...     rounds=9,
                ... )
                >>> circuit.count_determined_measurements()
                217
                >>> circuit.num_detectors + circuit.num_observables
                217
        )DOC")
            .data());

    c.def_property_readonly(
        "num_observables",
        &Circuit::count_observables,
        clean_doc_string(R"DOC(
            Counts the number of logical observables defined by the circuit.

            This is one more than the largest index that appears as an argument to an
            OBSERVABLE_INCLUDE instruction.

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
        clean_doc_string(R"DOC(
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
        clean_doc_string(R"DOC(
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
        pybind11::arg("reference_sample") = pybind11::none(),
        clean_doc_string(R"DOC(
            @signature def compile_sampler(self, *, skip_reference_sample: bool = False, seed: Optional[int] = None, reference_sample: Optional[np.ndarray] = None) -> stim.CompiledMeasurementSampler:
            Returns an object that can quickly batch sample measurements from the circuit.

            Args:
                skip_reference_sample: Defaults to False. When set to True, the reference
                    sample used by the sampler is initialized to all-zeroes instead of being
                    collected from the circuit. This means that the results returned by the
                    sampler are actually whether or not each measurement was *flipped*,
                    instead of true measurement results.

                    Forcing an all-zero reference sample is useful when you are only
                    interested in error propagation and don't want to have to deal with the
                    fact that some measurements want to be On when no errors occur. It is
                    also useful when you know for sure that the all-zero result is actually
                    a possible result from the circuit (under noiseless execution), meaning
                    it is a valid reference sample as good as any other. Computing the
                    reference sample is the most time consuming and memory intensive part of
                    simulating the circuit, so promising that the simulator can safely skip
                    that step is an effective optimization.
                seed: PARTIALLY determines simulation results by deterministically seeding
                    the random number generator.

                    Must be None or an integer in range(2**64).

                    Defaults to None. When None, the prng is seeded from system entropy.

                    When set to an integer, making the exact same series calls on the exact
                    same machine with the exact same version of Stim will produce the exact
                    same simulation results.

                    CAUTION: simulation results *WILL NOT* be consistent between versions of
                    Stim. This restriction is present to make it possible to have future
                    optimizations to the random sampling, and is enforced by introducing
                    intentional differences in the seeding strategy from version to version.

                    CAUTION: simulation results *MAY NOT* be consistent across machines that
                    differ in the width of supported SIMD instructions. For example, using
                    the same seed on a machine that supports AVX instructions and one that
                    only supports SSE instructions may produce different simulation results.

                    CAUTION: simulation results *MAY NOT* be consistent if you vary how many
                    shots are taken. For example, taking 10 shots and then 90 shots will
                    give different results from taking 100 shots in one call.
                reference_sample: The data to xor into the measurement flips produced by the
                    frame simulator, in order to produce proper measurement results.
                    This can either be specified as an `np.bool_` array or a bit packed
                    `np.uint8` array (little endian). Under normal conditions, the reference
                    sample should be a valid noiseless sample of the circuit, such as the
                    one returned by `circuit.reference_sample()`. If this argument is not
                    provided, the reference sample will be set to
                    `circuit.reference_sample()`, unless `skip_reference_sample=True`
                    is used, in which case it will be set to all-zeros.

            Raises:
                ValueError: skip_reference_sample is True and reference_sample is not None.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    X 2
                ...    M 0 1 2
                ... ''')
                >>> s = c.compile_sampler()
                >>> s.sample(shots=1)
                array([[False, False,  True]])
        )DOC")
            .data());

    c.def(
        "reference_sample",
        [](const Circuit &self, bool bit_packed) {
            auto ref = TableauSimulator<MAX_BITWORD_WIDTH>::reference_sample_circuit(self);
            simd_bits_range_ref<MAX_BITWORD_WIDTH> reference_sample(ref.ptr_simd, ref.num_simd_words);
            size_t num_measure = self.count_measurements();
            return simd_bits_to_numpy(reference_sample, num_measure, bit_packed);
        },
        pybind11::kw_only(),
        pybind11::arg("bit_packed") = false,
        clean_doc_string(R"DOC(
            @signature def reference_sample(self, *, bit_packed: bool = False) -> np.ndarray:
            Samples the given circuit in a deterministic fashion.

            Discards all noisy operations, and biases all collapse events
            towards +Z instead of randomly +Z/-Z.

            Args:
                circuit: The circuit to "sample" from.
                bit_packed: Defaults to False. Determines whether the output numpy arrays
                    use dtype=bool_ or dtype=uint8 with 8 bools packed into each byte.

            Returns:
                reference_sample: reference sample sampled from the given circuit.
        )DOC")
            .data());

    c.def(
        "compile_m2d_converter",
        &py_init_compiled_measurements_to_detection_events_converter,
        pybind11::kw_only(),
        pybind11::arg("skip_reference_sample") = false,
        clean_doc_string(R"DOC(
            Creates a measurement-to-detection-event converter for the given circuit.

            The converter uses a noiseless reference sample, collected from the circuit
            using stim's Tableau simulator during initialization of the converter, as a
            baseline for determining what the expected value of a detector is.

            Note that the expected behavior of gauge detectors (detectors that are not
            actually deterministic under noiseless execution) can vary depending on the
            reference sample. Stim mitigates this by always generating the same reference
            sample for a given circuit.

            Args:
                skip_reference_sample: Defaults to False. When set to True, the reference
                    sample used by the converter is initialized to all-zeroes instead of
                    being collected from the circuit. This should only be used if it's known
                    that the all-zeroes sample is actually a possible result from the
                    circuit (under noiseless execution).

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
                ...     measurements=np.array([[0], [1]], dtype=np.bool_),
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
        clean_doc_string(R"DOC(
            Returns an object that can batch sample detection events from the circuit.

            Args:
                seed: PARTIALLY determines simulation results by deterministically seeding
                    the random number generator.

                    Must be None or an integer in range(2**64).

                    Defaults to None. When None, the prng is seeded from system entropy.

                    When set to an integer, making the exact same series calls on the exact
                    same machine with the exact same version of Stim will produce the exact
                    same simulation results.

                    CAUTION: simulation results *WILL NOT* be consistent between versions of
                    Stim. This restriction is present to make it possible to have future
                    optimizations to the random sampling, and is enforced by introducing
                    intentional differences in the seeding strategy from version to version.

                    CAUTION: simulation results *MAY NOT* be consistent across machines that
                    differ in the width of supported SIMD instructions. For example, using
                    the same seed on a machine that supports AVX instructions and one that
                    only supports SSE instructions may produce different simulation results.

                    CAUTION: simulation results *MAY NOT* be consistent if you vary how many
                    shots are taken. For example, taking 10 shots and then 90 shots will
                    give different results from taking 100 shots in one call.

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
                array([[False]])
        )DOC")
            .data());

    c.def(
        "clear",
        &Circuit::clear,
        clean_doc_string(R"DOC(
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
        "flattened_operations",
        [](Circuit &self) {
            pybind11::list result;
            self.for_each_operation([&](const CircuitInstruction &op) {
                pybind11::list args;
                pybind11::list targets;
                for (auto t : op.targets) {
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
                for (auto t : op.args) {
                    args.append(t);
                }
                const auto &gate_data = GATE_DATA[op.gate_type];
                if (op.args.empty()) {
                    // Backwards compatibility.
                    result.append(pybind11::make_tuple(gate_data.name, targets, 0));
                } else if (op.args.size() == 1) {
                    // Backwards compatibility.
                    result.append(pybind11::make_tuple(gate_data.name, targets, op.args[0]));
                } else {
                    result.append(pybind11::make_tuple(gate_data.name, targets, args));
                }
            });
            return result;
        },
        clean_doc_string(R"DOC(
            [DEPRECATED]

            Returns a list of tuples encoding the contents of the circuit.
            Instead of this method, use `for instruction in circuit` or, to
            avoid REPEAT blocks, `for instruction in circuit.flattened()`.

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
        clean_doc_string(R"DOC(
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
                >>> c1 + c2
                stim.Circuit('''
                    X 0
                    Y 1 2
                    M 0 1 2
                ''')
        )DOC")
            .data());

    c.def(
        "__iadd__",
        &Circuit::operator+=,
        pybind11::arg("second"),
        clean_doc_string(R"DOC(
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
                >>> print(repr(c1))
                stim.Circuit('''
                    X 0
                    Y 1 2
                    M 0 1 2
                ''')
        )DOC")
            .data());

    c.def(
        "__imul__",
        &Circuit::operator*=,
        pybind11::arg("repetitions"),
        clean_doc_string(R"DOC(
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
                >>> print(repr(c))
                stim.Circuit('''
                    REPEAT 3 {
                        X 0
                        Y 1 2
                    }
                ''')
        )DOC")
            .data());

    c.def(
        "__mul__",
        &Circuit::operator*,
        pybind11::arg("repetitions"),
        clean_doc_string(R"DOC(
            Repeats the circuit using a REPEAT block.

            Has special cases for 0 repetitions and 1 repetitions.

            Args:
                repetitions: The number of times the REPEAT block should repeat.

            Returns:
                repetitions=0: An empty circuit.
                repetitions=1: A copy of this circuit.
                repetitions>=2: A circuit with a single REPEAT block, where the contents of
                    that repeat block are this circuit.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    X 0
                ...    Y 1 2
                ... ''')
                >>> c * 3
                stim.Circuit('''
                    REPEAT 3 {
                        X 0
                        Y 1 2
                    }
                ''')
        )DOC")
            .data());

    c.def(
        "__rmul__",
        &Circuit::operator*,
        pybind11::arg("repetitions"),
        clean_doc_string(R"DOC(
            Repeats the circuit using a REPEAT block.

            Has special cases for 0 repetitions and 1 repetitions.

            Args:
                repetitions: The number of times the REPEAT block should repeat.

            Returns:
                repetitions=0: An empty circuit.
                repetitions=1: A copy of this circuit.
                repetitions>=2: A circuit with a single REPEAT block, where the contents of
                    that repeat block are this circuit.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    X 0
                ...    Y 1 2
                ... ''')
                >>> 3 * c
                stim.Circuit('''
                    REPEAT 3 {
                        X 0
                        Y 1 2
                    }
                ''')
        )DOC")
            .data());

    for (size_t k = 0; k < 2; k++) {
        c.def(
            k == 0 ? "append_operation" : "append",
            k == 0 ? &circuit_append_backwards_compat : &circuit_append_strict,
            pybind11::arg("name"),
            pybind11::arg("targets") = pybind11::make_tuple(),
            pybind11::arg("arg") = pybind11::none(),
            k == 0 ? "[DEPRECATED] use stim.Circuit.append instead"
                   : clean_doc_string(R"DOC(
                Appends an operation into the circuit.
                @overload def append(self, name: str, targets: Union[int, stim.GateTarget, Iterable[Union[int, stim.GateTarget]]], arg: Union[float, Iterable[float]]) -> None:
                @overload def append(self, name: Union[stim.CircuitOperation, stim.CircuitRepeatBlock]) -> None:

                Note: `stim.Circuit.append_operation` is an alias of `stim.Circuit.append`.

                Examples:
                    >>> import stim
                    >>> c = stim.Circuit()
                    >>> c.append("X", 0)
                    >>> c.append("H", [0, 1])
                    >>> c.append("M", [0, stim.target_inv(1)])
                    >>> c.append("CNOT", [stim.target_rec(-1), 0])
                    >>> c.append("X_ERROR", [0], 0.125)
                    >>> c.append("CORRELATED_ERROR", [stim.target_x(0), stim.target_y(2)], 0.25)
                    >>> print(repr(c))
                    stim.Circuit('''
                        X 0
                        H 0 1
                        M 0 !1
                        CX rec[-1] 0
                        X_ERROR(0.125) 0
                        E(0.25) X0 Y2
                    ''')

                Args:
                    name: The name of the operation's gate (e.g. "H" or "M" or "CNOT").

                        This argument can also be set to a `stim.CircuitInstruction` or
                        `stim.CircuitInstructionBlock`, which results in the instruction or
                        block being appended to the circuit. The other arguments (targets
                        and arg) can't be specified when doing so.

                        (The argument being called `name` is no longer quite right, but
                        is being kept for backwards compatibility.)
                    targets: The objects operated on by the gate. This can be either a
                        single target or an iterable of multiple targets to broadcast the
                        gate over. Each target can be an integer (a qubit), a
                        stim.GateTarget, or a special target from one of the `stim.target_*`
                        methods (such as a measurement record target like `rec[-1]` from
                        `stim.target_rec(-1)`).
                    arg: The "parens arguments" for the gate, such as the probability for a
                        noise operation. A double or list of doubles parameterizing the
                        gate. Different gates take different parens arguments. For example,
                        X_ERROR takes a probability, OBSERVABLE_INCLUDE takes an observable
                        index, and PAULI_CHANNEL_1 takes three disjoint probabilities.

                        Note: Defaults to no parens arguments. Except, for backwards
                        compatibility reasons, `cirq.append_operation` (but not
                        `cirq.append`) will default to a single 0.0 argument for gates that
                        take exactly one argument.
            )DOC")
                         .data());
    }

    c.def(
        "append_from_stim_program_text",
        [](Circuit &self, const char *text) {
            self.append_from_text(text);
        },
        pybind11::arg("stim_program_text"),
        clean_doc_string(R"DOC(
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
                stim_program_text: The STIM program text containing the circuit operations
                    to append.
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
        clean_doc_string(R"DOC(
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
        [](std::string_view type,
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
        clean_doc_string(R"DOC(
            Generates common circuits.

            The generated circuits can include configurable noise.

            The generated circuits include DETECTOR and OBSERVABLE_INCLUDE annotations so
            that their detection events and logical observables can be sampled.

            The generated circuits include TICK annotations to mark the progression of time.
            (E.g. so that converting them using `stimcirq.stim_circuit_to_cirq_circuit` will
            produce a `cirq.Circuit` with the intended moment structure.)

            Args:
                code_task: A string identifying the type of circuit to generate. Available
                    code tasks are:
                        - "repetition_code:memory"
                        - "surface_code:rotated_memory_x"
                        - "surface_code:rotated_memory_z"
                        - "surface_code:unrotated_memory_x"
                        - "surface_code:unrotated_memory_z"
                        - "color_code:memory_xyz"
                distance: The desired code distance of the generated circuit. The code
                    distance is the minimum number of physical errors needed to cause a
                    logical error. This parameter indirectly determines how many qubits the
                    generated circuit uses.
                rounds: How many times the measurement qubits in the generated circuit will
                    be measured. Indirectly determines the duration of the generated
                    circuit.
                after_clifford_depolarization: Defaults to 0. The probability (p) of
                    `DEPOLARIZE1(p)` operations to add after every single-qubit Clifford
                    operation and `DEPOLARIZE2(p)` operations to add after every two-qubit
                    Clifford operation. The after-Clifford depolarizing operations are only
                    included if this probability is not 0.
                before_round_data_depolarization: Defaults to 0. The probability (p) of
                    `DEPOLARIZE1(p)` operations to apply to every data qubit at the start of
                    a round of stabilizer measurements. The start-of-round depolarizing
                    operations are only included if this probability is not 0.
                before_measure_flip_probability: Defaults to 0. The probability (p) of
                    `X_ERROR(p)` operations applied to qubits before each measurement (X
                    basis measurements use `Z_ERROR(p)` instead). The before-measurement
                    flips are only included if this probability is not 0.
                after_reset_flip_probability: Defaults to 0. The probability (p) of
                    `X_ERROR(p)` operations applied to qubits after each reset (X basis
                    resets use `Z_ERROR(p)` instead). The after-reset flips are only
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

    c.def_static(
        "from_file",
        [](pybind11::object &obj) {
            if (pybind11::isinstance<pybind11::str>(obj)) {
                std::string_view path = pybind11::cast<std::string_view>(obj);
                RaiiFile f(path, "rb");
                return Circuit::from_file(f.f);
            }

            pybind11::object py_path = pybind11::module::import("pathlib").attr("Path");
            if (pybind11::isinstance(obj, py_path)) {
                pybind11::object obj_str = pybind11::str(obj);
                std::string_view path = pybind11::cast<std::string_view>(obj_str);
                RaiiFile f(path, "rb");
                return Circuit::from_file(f.f);
            }

            pybind11::object py_text_io_base = pybind11::module::import("io").attr("TextIOBase");
            if (pybind11::isinstance(obj, py_text_io_base)) {
                pybind11::object contents = obj.attr("read")();
                return Circuit(pybind11::cast<std::string_view>(contents));
            }

            std::stringstream ss;
            ss << "Don't know how to read from ";
            ss << pybind11::repr(obj);
            throw std::invalid_argument(ss.str());
        },
        pybind11::arg("file"),
        clean_doc_string(R"DOC(
            @signature def from_file(file: Union[io.TextIOBase, str, pathlib.Path]) -> stim.Circuit:
            Reads a stim circuit from a file.

            The file format is defined at
            https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md

            Args:
                file: A file path or open file object to read from.

            Returns:
                The circuit parsed from the file.

            Examples:
                >>> import stim
                >>> import tempfile

                >>> with tempfile.TemporaryDirectory() as tmpdir:
                ...     path = tmpdir + '/tmp.stim'
                ...     with open(path, 'w') as f:
                ...         print('H 5', file=f)
                ...     circuit = stim.Circuit.from_file(path)
                >>> circuit
                stim.Circuit('''
                    H 5
                ''')

                >>> with tempfile.TemporaryDirectory() as tmpdir:
                ...     path = tmpdir + '/tmp.stim'
                ...     with open(path, 'w') as f:
                ...         print('CNOT 4 5', file=f)
                ...     with open(path) as f:
                ...         circuit = stim.Circuit.from_file(path)
                >>> circuit
                stim.Circuit('''
                    CX 4 5
                ''')
        )DOC")
            .data());

    c.def(
        "to_file",
        [](const Circuit &self, pybind11::object &obj) {
            if (pybind11::isinstance<pybind11::str>(obj)) {
                std::string path = pybind11::cast<std::string>(obj);
                std::ofstream out(path, std::ofstream::out);
                if (!out.is_open()) {
                    throw std::invalid_argument("Failed to open " + path);
                }
                out << self << '\n';
                return;
            }

            auto py_path = pybind11::module::import("pathlib").attr("Path");
            if (pybind11::isinstance(obj, py_path)) {
                auto path = pybind11::cast<std::string>(pybind11::str(obj));
                std::ofstream out(path, std::ofstream::out);
                if (!out.is_open()) {
                    throw std::invalid_argument("Failed to open " + path);
                }
                out << self << '\n';
                return;
            }

            auto py_text_io_base = pybind11::module::import("io").attr("TextIOBase");
            if (pybind11::isinstance(obj, py_text_io_base)) {
                obj.attr("write")(pybind11::str(self.str()));
                obj.attr("write")(pybind11::str("\n"));
                return;
            }

            std::stringstream ss;
            ss << "Don't know how to write to ";
            ss << pybind11::repr(obj);
            throw std::invalid_argument(ss.str());
        },
        pybind11::arg("file"),
        clean_doc_string(R"DOC(
            @signature def to_file(self, file: Union[io.TextIOBase, str, pathlib.Path]) -> None:
            Writes the stim circuit to a file.

            The file format is defined at
            https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md

            Args:
                file: A file path or an open file to write to.

            Examples:
                >>> import stim
                >>> import tempfile
                >>> c = stim.Circuit('H 5\nX 0')

                >>> with tempfile.TemporaryDirectory() as tmpdir:
                ...     path = tmpdir + '/tmp.stim'
                ...     with open(path, 'w') as f:
                ...         c.to_file(f)
                ...     with open(path) as f:
                ...         contents = f.read()
                >>> contents
                'H 5\nX 0\n'

                >>> with tempfile.TemporaryDirectory() as tmpdir:
                ...     path = tmpdir + '/tmp.stim'
                ...     c.to_file(path)
                ...     with open(path) as f:
                ...         contents = f.read()
                >>> contents
                'H 5\nX 0\n'
        )DOC")
            .data());

    c.def(
        "to_tableau",
        [](const Circuit &circuit, bool ignore_noise, bool ignore_measurement, bool ignore_reset) {
            return circuit_to_tableau<MAX_BITWORD_WIDTH>(circuit, ignore_noise, ignore_measurement, ignore_reset);
        },
        pybind11::kw_only(),
        pybind11::arg("ignore_noise") = false,
        pybind11::arg("ignore_measurement") = false,
        pybind11::arg("ignore_reset") = false,
        clean_doc_string(R"DOC(
            @signature def to_tableau(self, *, ignore_noise: bool = False, ignore_measurement: bool = False, ignore_reset: bool = False) -> stim.Tableau:
            Converts the circuit into an equivalent stabilizer tableau.

            Args:
                ignore_noise: Defaults to False. When False, any noise operations in the
                    circuit will cause the conversion to fail with an exception. When True,
                    noise operations are skipped over as if they weren't even present in the
                    circuit.
                ignore_measurement: Defaults to False. When False, any measurement
                    operations in the circuit will cause the conversion to fail with an
                    exception. When True, measurement operations are skipped over as if they
                    weren't even present in the circuit.
                ignore_reset: Defaults to False. When False, any reset operations in the
                    circuit will cause the conversion to fail with an exception. When True,
                    reset operations are skipped over as if they weren't even present in the
                    circuit.

            Returns:
                A tableau equivalent to the circuit (up to global phase).

            Raises:
                ValueError:
                    The circuit contains noise operations but ignore_noise=False.
                    OR
                    The circuit contains measurement operations but
                    ignore_measurement=False.
                    OR
                    The circuit contains reset operations but ignore_reset=False.

            Examples:
                >>> import stim
                >>> stim.Circuit('''
                ...     H 0
                ...     CNOT 0 1
                ... ''').to_tableau()
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+Z_"),
                        stim.PauliString("+_X"),
                    ],
                    zs=[
                        stim.PauliString("+XX"),
                        stim.PauliString("+ZZ"),
                    ],
                )
        )DOC")
            .data());

    c.def(
        "to_qasm",
        [](const Circuit &self, int open_qasm_version, bool skip_dets_and_obs) -> std::string {
            std::stringstream out;
            export_open_qasm(self, out, open_qasm_version, skip_dets_and_obs);
            return out.str();
        },
        pybind11::kw_only(),
        pybind11::arg("open_qasm_version"),
        pybind11::arg("skip_dets_and_obs") = false,
        clean_doc_string(R"DOC(
            @signature def to_qasm(self, *, open_qasm_version: int, skip_dets_and_obs: bool = False) -> str:
            Creates an equivalent OpenQASM implementation of the circuit.

            Args:
                open_qasm_version: The version of OpenQASM to target.
                    This should be set to 2 or to 3.

                    Differences between the versions are:
                        - Support for operations on classical bits operations (only version
                            3). This means DETECTOR and OBSERVABLE_INCLUDE only work with
                            version 3.
                        - Support for feedback operations (only version 3).
                        - Support for subroutines (only version 3). Without subroutines,
                            non-standard dissipative gates like MR and RX need to decompose
                            inline every single time they're used.
                        - Minor name changes (e.g. creg -> bit, qelib1.inc -> stdgates.inc).
                skip_dets_and_obs: Defaults to False. When set to False, the output will
                    include a `dets` register and an `obs` register (assuming the circuit
                    has detectors and observables). These registers will be computed as part
                    of running the circuit. This requires performing a simulation of the
                    circuit, in order to correctly account for the expected value of
                    measurements.

                    When set to True, the `dets` and `obs` registers are not included in the
                    output, and no simulation of the circuit is performed.

            Returns:
                The OpenQASM code as a string.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit('''
                ...     R 0 1
                ...     X 1
                ...     H 0
                ...     CX 0 1
                ...     M 0 1
                ...     DETECTOR rec[-1] rec[-2]
                ... ''');
                >>> qasm = circuit.to_qasm(open_qasm_version=3);
                >>> print(qasm.strip().replace('\n\n', '\n'))
                OPENQASM 3.0;
                include "stdgates.inc";
                qreg q[2];
                creg rec[2];
                creg dets[1];
                reset q[0];
                reset q[1];
                x q[1];
                h q[0];
                cx q[0], q[1];
                measure q[0] -> rec[0];
                measure q[1] -> rec[1];
                dets[0] = rec[1] ^ rec[0] ^ 1;
        )DOC")
            .data());

    c.def(
        "__len__",
        [](const Circuit &self) {
            return self.operations.size();
        },
        clean_doc_string(R"DOC(
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
        [](const Circuit &self, const pybind11::object &index_or_slice) -> pybind11::object {
            pybind11::ssize_t index, step, slice_length;
            if (normalize_index_or_slice(index_or_slice, self.operations.size(), &index, &step, &slice_length)) {
                return pybind11::cast(self.py_get_slice(index, step, slice_length));
            }

            auto &op = self.operations[index];
            if (op.gate_type == GateType::REPEAT) {
                return pybind11::cast(CircuitRepeatBlock{op.repeat_block_rep_count(), op.repeat_block_body(self)});
            }
            std::vector<GateTarget> targets;
            for (const auto &e : op.targets) {
                targets.push_back(GateTarget(e));
            }
            std::vector<double> args;
            for (const auto &e : op.args) {
                args.push_back(e);
            }
            return pybind11::cast(PyCircuitInstruction(op.gate_type, targets, args));
        },
        pybind11::arg("index_or_slice"),
        clean_doc_string(R"DOC(
            Returns copies of instructions from the circuit.
            @overload def __getitem__(self, index_or_slice: int) -> Union[stim.CircuitInstruction, stim.CircuitRepeatBlock]:
            @overload def __getitem__(self, index_or_slice: slice) -> stim.Circuit:

            Args:
                index_or_slice: An integer index picking out an instruction to return, or a
                    slice picking out a range of instructions to return as a circuit.

            Returns:
                If the index was an integer, then an instruction from the circuit.
                If the index was a slice, then a circuit made up of the instructions in that
                slice.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit('''
                ...    X 0
                ...    X_ERROR(0.5) 2
                ...    REPEAT 100 {
                ...        X 0
                ...        Y 1 2
                ...    }
                ...    TICK
                ...    M 0
                ...    DETECTOR rec[-1]
                ... ''')
                >>> circuit[1]
                stim.CircuitInstruction('X_ERROR', [stim.GateTarget(2)], [0.5])
                >>> circuit[2]
                stim.CircuitRepeatBlock(100, stim.Circuit('''
                    X 0
                    Y 1 2
                '''))
                >>> circuit[1::2]
                stim.Circuit('''
                    X_ERROR(0.5) 2
                    TICK
                    DETECTOR rec[-1]
                ''')
        )DOC")
            .data());

    c.def(
        "detector_error_model",
        [](const Circuit &self,
           bool decompose_errors,
           bool flatten_loops,
           bool allow_gauge_detectors,
           double approximate_disjoint_errors,
           bool ignore_decomposition_failures,
           bool block_decomposition_from_introducing_remnant_edges) -> DetectorErrorModel {
            return ErrorAnalyzer::circuit_to_detector_error_model(
                self,
                decompose_errors,
                !flatten_loops,
                allow_gauge_detectors,
                approximate_disjoint_errors,
                ignore_decomposition_failures,
                block_decomposition_from_introducing_remnant_edges);
        },
        pybind11::kw_only(),
        pybind11::arg("decompose_errors") = false,
        pybind11::arg("flatten_loops") = false,
        pybind11::arg("allow_gauge_detectors") = false,
        pybind11::arg("approximate_disjoint_errors") = false,
        pybind11::arg("ignore_decomposition_failures") = false,
        pybind11::arg("block_decomposition_from_introducing_remnant_edges") = false,
        clean_doc_string(R"DOC(
            Returns a stim.DetectorErrorModel describing the error processes in the circuit.

            Args:
                decompose_errors: Defaults to false. When set to true, the error analysis
                    attempts to decompose the components of composite error mechanisms (such
                    as depolarization errors) into simpler errors, and suggest this
                    decomposition via `stim.target_separator()` between the components. For
                    example, in an XZ surface code, single qubit depolarization has a Y
                    error term which can be decomposed into simpler X and Z error terms.
                    Decomposition fails (causing this method to throw) if it's not possible
                    to decompose large errors into simple errors that affect at most two
                    detectors.
                flatten_loops: Defaults to false. When set to True, the output will not
                    contain any `repeat` blocks. When set to False, the error analysis
                    watches for loops in the circuit reaching a periodic steady state with
                    respect to the detectors being introduced, the error mechanisms that
                    affect them, and the locations of the logical observables. When it
                    identifies such a steady state, it outputs a repeat block. This is
                    massively more efficient than flattening for circuits that contain
                    loops, but creates a more complex output.
                allow_gauge_detectors: Defaults to false. When set to false, the error
                    analysis verifies that detectors in the circuit are actually
                    deterministic under noiseless execution of the circuit. When set to
                    True, these detectors are instead considered to be part of degrees
                    freedom that can be removed from the error model. For example, if
                    detectors D1 and D3 both anti-commute with a reset, then the error model
                    has a gauge `error(0.5) D1 D3`. When gauges are identified, one of the
                    involved detectors is removed from the system using Gaussian
                    elimination.

                    Note that logical observables are still verified to be deterministic,
                    even if this option is set.
                approximate_disjoint_errors: Defaults to false. When set to false, composite
                    error mechanisms with disjoint components (such as
                    `PAULI_CHANNEL_1(0.1, 0.2, 0.0)`) can cause the error analysis to throw
                    exceptions (because detector error models can only contain independent
                    error mechanisms). When set to true, the probabilities of the disjoint
                    cases are instead assumed to be independent probabilities. For example,
                    a `PAULI_CHANNEL_1(0.1, 0.2, 0.0)` becomes equivalent to an
                    `X_ERROR(0.1)` followed by a `Y_ERROR(0.2)`. This assumption is an
                    approximation, but it is a good approximation for small probabilities.

                    This argument can also be set to a probability between 0 and 1, setting
                    a threshold below which the approximation is acceptable. Any error
                    mechanisms that have a component probability above the threshold will
                    cause an exception to be thrown.
                ignore_decomposition_failures: Defaults to False.
                    When this is set to True, circuit errors that fail to decompose into
                    graphlike detector error model errors no longer cause the conversion
                    process to abort. Instead, the undecomposed error is inserted into the
                    output. Whatever tool the detector error model is then given to is
                    responsible for dealing with the undecomposed errors (e.g. a tool may
                    choose to simply ignore them).

                    Irrelevant unless decompose_errors=True.
                block_decomposition_from_introducing_remnant_edges: Defaults to False.
                    Requires that both A B and C D be present elsewhere in the detector
                    error model in order to decompose A B C D into A B ^ C D. Normally, only
                    one of A B or C D needs to appear to allow this decomposition.

                    Remnant edges can be a useful feature for ensuring decomposition
                    succeeds, but they can also reduce the effective code distance by giving
                    the decoder single edges that actually represent multiple errors in the
                    circuit (resulting in the decoder making misinformed choices when
                    decoding).

                    Irrelevant unless decompose_errors=True.

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

    c.def(
        "approx_equals",
        [](const Circuit &self, const pybind11::object &obj, double atol) -> bool {
            try {
                return self.approx_equals(pybind11::cast<Circuit>(obj), atol);
            } catch (const pybind11::cast_error &ex) {
                return false;
            }
        },
        pybind11::arg("other"),
        pybind11::kw_only(),
        pybind11::arg("atol"),
        clean_doc_string(R"DOC(
            Checks if a circuit is approximately equal to another circuit.

            Two circuits are approximately equal if they are equal up to slight
            perturbations of instruction arguments such as probabilities. For example,
            `X_ERROR(0.100) 0` is approximately equal to `X_ERROR(0.099)` within an absolute
            tolerance of 0.002. All other details of the circuits (such as the ordering of
            instructions and targets) must be exactly the same.

            Args:
                other: The circuit, or other object, to compare to this one.
                atol: The absolute error tolerance. The maximum amount each probability may
                    have been perturbed by.

            Returns:
                True if the given object is a circuit approximately equal up to the
                receiving circuit up to the given tolerance, otherwise False.

            Examples:
                >>> import stim
                >>> base = stim.Circuit('''
                ...    X_ERROR(0.099) 0 1 2
                ...    M 0 1 2
                ... ''')

                >>> base.approx_equals(base, atol=0)
                True

                >>> base.approx_equals(stim.Circuit('''
                ...    X_ERROR(0.101) 0 1 2
                ...    M 0 1 2
                ... '''), atol=0)
                False

                >>> base.approx_equals(stim.Circuit('''
                ...    X_ERROR(0.101) 0 1 2
                ...    M 0 1 2
                ... '''), atol=0.0001)
                False

                >>> base.approx_equals(stim.Circuit('''
                ...    X_ERROR(0.101) 0 1 2
                ...    M 0 1 2
                ... '''), atol=0.01)
                True

                >>> base.approx_equals(stim.Circuit('''
                ...    DEPOLARIZE1(0.099) 0 1 2
                ...    MRX 0 1 2
                ... '''), atol=9999)
                False
        )DOC")
            .data());

    c.def(
        "get_detector_coordinates",
        [](const Circuit &self, const pybind11::object &obj) {
            return self.get_detector_coordinates(obj_to_abs_detector_id_set(obj, [&]() {
                return self.count_detectors();
            }));
        },
        pybind11::arg("only") = pybind11::none(),
        clean_doc_string(R"DOC(
            Returns the coordinate metadata of detectors in the circuit.

            Args:
                only: Defaults to None (meaning include all detectors). A list of detector
                    indices to include in the result. Detector indices beyond the end of the
                    detector error model of the circuit cause an error.

            Returns:
                A dictionary mapping integers (detector indices) to lists of floats
                (coordinates).

                Detectors with no specified coordinate data are mapped to an empty tuple.
                If `only` is specified, then `set(result.keys()) == set(only)`.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit('''
                ...    M 0
                ...    DETECTOR rec[-1]
                ...    DETECTOR(1, 2, 3) rec[-1]
                ...    REPEAT 3 {
                ...        DETECTOR(42) rec[-1]
                ...        SHIFT_COORDS(100)
                ...    }
                ... ''')
                >>> circuit.get_detector_coordinates()
                {0: [], 1: [1.0, 2.0, 3.0], 2: [42.0], 3: [142.0], 4: [242.0]}
                >>> circuit.get_detector_coordinates(only=[1])
                {1: [1.0, 2.0, 3.0]}
        )DOC")
            .data());

    c.def(
        "get_final_qubit_coordinates",
        &Circuit::get_final_qubit_coords,
        clean_doc_string(R"DOC(
            Returns the coordinate metadata of qubits in the circuit.

            If a qubit's coordinates are specified multiple times, only the last specified
            coordinates are returned.

            Returns:
                A dictionary mapping qubit indices (integers) to coordinates (lists of
                floats). Qubits that never had their coordinates specified are not included
                in the result.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit('''
                ...    QUBIT_COORDS(1, 2, 3) 1
                ... ''')
                >>> circuit.get_final_qubit_coordinates()
                {1: [1.0, 2.0, 3.0]}
        )DOC")
            .data());

    c.def(pybind11::pickle(
        [](const Circuit &self) -> pybind11::str {
            return self.str();
        },
        [](const pybind11::str &text) {
            return Circuit(pybind11::cast<std::string_view>(text));
        }));

    c.def(
        "shortest_graphlike_error",
        &circuit_shortest_graphlike_error,
        pybind11::kw_only(),
        pybind11::arg("ignore_ungraphlike_errors") = true,
        pybind11::arg("canonicalize_circuit_errors") = false,
        clean_doc_string(R"DOC(
            Finds a minimum set of graphlike errors to produce an undetected logical error.

            A "graphlike error" is an error that creates at most two detection events
            (causes a change in the parity of the measurement sets of at most two DETECTOR
            annotations).

            Note that this method does not pay attention to error probabilities (other than
            ignoring errors with probability 0). It searches for a logical error with the
            minimum *number* of physical errors, not the maximum probability of those
            physical errors all occurring.

            This method works by converting the circuit into a `stim.DetectorErrorModel`
            using `circuit.detector_error_model(...)`, computing the shortest graphlike
            error of the error model, and then converting the physical errors making up that
            logical error back into representative circuit errors.

            Args:
                ignore_ungraphlike_errors:
                    False: Attempt to decompose any ungraphlike errors in the circuit into
                        graphlike parts. If this fails, raise an exception instead of
                        continuing.

                        Note: in some cases, graphlike errors only appear as parts of
                        decomposed ungraphlike errors. This can produce a result that lists
                        DEM errors with zero matching circuit errors, because the only way
                        to achieve those errors is by combining a decomposed error with a
                        graphlike error. As a result, when using this option it is NOT
                        guaranteed that the length of the result is an upper bound on the
                        true code distance. That is only the case if every item in the
                        result lists at least one matching circuit error.
                    True (default): Ungraphlike errors are simply skipped as if they weren't
                        present, even if they could become graphlike if decomposed. This
                        guarantees the length of the result is an upper bound on the true
                        code distance.
                canonicalize_circuit_errors: Whether or not to use one representative for
                    equal-symptom circuit errors.

                    False (default): Each DEM error lists every possible circuit error that
                        single handedly produces those symptoms as a potential match. This
                        is verbose but gives complete information.
                    True: Each DEM error is matched with one possible circuit error that
                        single handedly produces those symptoms, with a preference towards
                        errors that are simpler (e.g. apply Paulis to fewer qubits). This
                        discards mostly-redundant information about different ways to
                        produce the same symptoms in order to give a succinct result.

            Returns:
                A list of error mechanisms that cause an undetected logical error.

                Each entry in the list is a `stim.ExplainedError` detailing the location
                and effects of a single physical error. The effects of the entire list
                combine to produce a logical frame change without any detection events.

            Examples:
                >>> import stim

                >>> circuit = stim.Circuit.generated(
                ...     "repetition_code:memory",
                ...     rounds=10,
                ...     distance=7,
                ...     before_round_data_depolarization=0.01)
                >>> len(circuit.shortest_graphlike_error())
                7
        )DOC")
            .data());

    c.def(
        "search_for_undetectable_logical_errors",
        &py_find_undetectable_logical_error,
        pybind11::kw_only(),
        pybind11::arg("dont_explore_detection_event_sets_with_size_above"),
        pybind11::arg("dont_explore_edges_with_degree_above"),
        pybind11::arg("dont_explore_edges_increasing_symptom_degree"),
        pybind11::arg("canonicalize_circuit_errors") = false,
        clean_doc_string(R"DOC(
            Searches for small sets of errors that form an undetectable logical error.

            THIS IS A HEURISTIC METHOD. It does not guarantee that it will find errors of
            particular sizes, or with particular properties. The errors it finds are a
            tangled combination of the truncation parameters you specify, internal
            optimizations which are correct when not truncating, and minutia of the circuit
            being considered.

            If you want a well behaved method that does provide guarantees of finding errors
            of a particular type, use `stim.Circuit.shortest_graphlike_error`. This method
            is more thorough than that (assuming you don't truncate so hard you omit
            graphlike edges), but exactly how thorough is difficult to describe. It's also
            not guaranteed that the behavior of this method will not be changed in the
            future in a way that permutes which logical errors are found and which are
            missed.

            This search method considers hyper errors, so it has worst case exponential
            runtime. It is important to carefully consider the arguments you are providing,
            which truncate the search space and trade cost for quality.

            The search progresses by starting from each error that crosses a logical
            observable, noting which detection events each error produces, and then
            iteratively adding in errors touching those detection events attempting to
            cancel out the detection event with the lowest index.

            Beware that the choice of logical observable can interact with the truncation
            options. Using different observables can change whether or not the search
            succeeds, even if those observables are equal modulo the stabilizers of the
            code. This is because the edges crossing logical observables are used as
            starting points for the search, and starting from different places along a path
            will result in different numbers of symptoms in intermediate states as the
            search progresses. For example, if the logical observable is next to a boundary,
            then the starting edges are likely boundary edges (degree 1) with 'room to
            grow', whereas if the observable was running through the bulk then the starting
            edges will have degree at least 2.

            Args:
                dont_explore_detection_event_sets_with_size_above: Truncates the search
                    space by refusing to cross an edge (i.e. add an error) when doing so
                    would produce an intermediate state that has more detection events than
                    this limit.
                dont_explore_edges_with_degree_above: Truncates the search space by refusing
                    to consider errors that cause a lot of detection events. For example,
                    you may only want to consider graphlike errors which have two or fewer
                    detection events.
                dont_explore_edges_increasing_symptom_degree: Truncates the search space by
                    refusing to cross an edge (i.e. add an error) when doing so would
                    produce an intermediate state that has more detection events that the
                    previous intermediate state. This massively improves the efficiency of
                    the search because instead of, for example, exploring all n^4 possible
                    detection event sets with 4 symptoms, the search will attempt to cancel
                    out symptoms one by one.
                canonicalize_circuit_errors: Whether or not to use one representative for
                    equal-symptom circuit errors.

                    False (default): Each DEM error lists every possible circuit error that
                        single handedly produces those symptoms as a potential match. This
                        is verbose but gives complete information.
                    True: Each DEM error is matched with one possible circuit error that
                        single handedly produces those symptoms, with a preference towards
                        errors that are simpler (e.g. apply Paulis to fewer qubits). This
                        discards mostly-redundant information about different ways to
                        produce the same symptoms in order to give a succinct result.

            Returns:
                A list of error mechanisms that cause an undetected logical error.

                Each entry in the list is a `stim.ExplainedError` detailing the location
                and effects of a single physical error. The effects of the entire list
                combine to produce a logical frame change without any detection events.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit.generated(
                ...     "surface_code:rotated_memory_x",
                ...     rounds=5,
                ...     distance=5,
                ...     after_clifford_depolarization=0.001)
                >>> print(len(circuit.search_for_undetectable_logical_errors(
                ...     dont_explore_detection_event_sets_with_size_above=4,
                ...     dont_explore_edges_with_degree_above=4,
                ...     dont_explore_edges_increasing_symptom_degree=True,
                ... )))
                5
        )DOC")
            .data());

    c.def(
        "shortest_error_sat_problem",
        &py_shortest_error_sat_problem,
        pybind11::kw_only(),
        pybind11::arg("format") = "WDIMACS",
        clean_doc_string(R"DOC(
            Makes a maxSAT problem of the circuit's distance, that other tools can solve.

            The output is a string describing the maxSAT problem in WDIMACS format
            (see https://maxhs.org/docs/wdimacs.html). The optimal solution to the
            problem is the fault distance of the circuit (the minimum number of error
            mechanisms that combine to flip any logical observable while producing no
            detection events). This method ignores the probabilities of the error
            mechanisms since it only cares about minimizing the number of errors
            triggered.

            There are many tools that can solve maxSAT problems in WDIMACS format.
            One quick way to get started is to install pysat by running this BASH
            terminal command:

                pip install python-sat

            Afterwards, you can run the included maxSAT solver "RC2" with this
            Python code:

                from pysat.examples.rc2 import RC2
                from pysat.formula import WCNF

                wcnf = WCNF(from_string="p wcnf 1 2 3\n3 -1 0\n3 1 0\n")

                with RC2(wcnf) as rc2:
                print(rc2.compute())
                print(rc2.cost)

            Much faster solvers are available online. For example, you can download
            one of the entries in the 2023 maxSAT competition (see
            https://maxsat-evaluations.github.io/2023) and run it on your problem by
            running these BASH terminal commands:

                wget https://maxsat-evaluations.github.io/2023/mse23-solver-src/exact/CASHWMaxSAT-CorePlus.zip
                unzip CASHWMaxSAT-CorePlus.zip
                ./CASHWMaxSAT-CorePlus/bin/cashwmaxsatcoreplus -bm -m your_problem.wcnf

            Args:
                format: Defaults to "WDIMACS", corresponding to WDIMACS format which is
                    described here: http://www.maxhs.org/docs/wdimacs.html

            Returns:
                A string corresponding to the contents of a maxSAT problem file in the
                requested format.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit('''
                ...   X_ERROR(0.1) 0
                ...   M 0
                ...   OBSERVABLE_INCLUDE(0) rec[-1]
                ...   X_ERROR(0.4) 0
                ...   M 0
                ...   DETECTOR rec[-1] rec[-2]
                ... ''')
                >>> print(circuit.shortest_error_sat_problem(), end='')
                p wcnf 2 4 5
                1 -1 0
                1 -2 0
                5 -1 0
                5 2 0
        )DOC")
            .data());

    c.def(
        "likeliest_error_sat_problem",
        &py_likeliest_error_sat_problem,
        pybind11::kw_only(),
        pybind11::arg("quantization") = 100,
        pybind11::arg("format") = "WDIMACS",
        clean_doc_string(R"DOC(
            Makes a maxSAT problem for the circuit's likeliest undetectable logical error.

            The output is a string describing the maxSAT problem in WDIMACS format
            (see https://maxhs.org/docs/wdimacs.html). The optimal solution to the
            problem is the highest likelihood set of error mechanisms that combine to
            flip any logical observable while producing no detection events).

            If there are any errors with probability p > 0.5, they are inverted so
            that the resulting weight ends up being positive. If there are errors
            with weight close or equal to 0.5, they can end up with 0 weight meaning
            that they can be included or not in the solution with no affect on the
            likelihood.

            There are many tools that can solve maxSAT problems in WDIMACS format.
            One quick way to get started is to install pysat by running this BASH
            terminal command:

                pip install python-sat

            Afterwards, you can run the included maxSAT solver "RC2" with this
            Python code:

                from pysat.examples.rc2 import RC2
                from pysat.formula import WCNF

                wcnf = WCNF(from_string="p wcnf 1 2 3\n3 -1 0\n3 1 0\n")

                with RC2(wcnf) as rc2:
                print(rc2.compute())
                print(rc2.cost)

            Much faster solvers are available online. For example, you can download
            one of the entries in the 2023 maxSAT competition (see
            https://maxsat-evaluations.github.io/2023) and run it on your problem by
            running these BASH terminal commands:

                wget https://maxsat-evaluations.github.io/2023/mse23-solver-src/exact/CASHWMaxSAT-CorePlus.zip
                unzip CASHWMaxSAT-CorePlus.zip
                ./CASHWMaxSAT-CorePlus/bin/cashwmaxsatcoreplus -bm -m your_problem.wcnf

            Args:
                format: Defaults to "WDIMACS", corresponding to WDIMACS format which is
                    described here: http://www.maxhs.org/docs/wdimacs.html
                quantization: Defaults to 10. Error probabilities are converted to log-odds
                    and scaled/rounded to be positive integers at most this large. Setting
                    this argument to a larger number results in more accurate quantization
                    such that the returned error set should have a likelihood closer to the
                    true most likely solution. This comes at the cost of making some maxSAT
                    solvers slower.

            Returns:
                A string corresponding to the contents of a maxSAT problem file in the
                requested format.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit('''
                ...   X_ERROR(0.1) 0
                ...   M 0
                ...   OBSERVABLE_INCLUDE(0) rec[-1]
                ...   X_ERROR(0.4) 0
                ...   M 0
                ...   DETECTOR rec[-1] rec[-2]
                ... ''')
                >>> print(circuit.likeliest_error_sat_problem(
                ...   quantization=1000
                ... ), end='')
                p wcnf 2 4 4001
                185 -1 0
                1000 -2 0
                4001 -1 0
                4001 2 0
        )DOC")
            .data());

    c.def(
        "explain_detector_error_model_errors",
        [](const Circuit &self,
           const pybind11::object &dem_filter,
           bool reduce_to_one_representative_error) -> std::vector<ExplainedError> {
            if (dem_filter.is_none()) {
                return ErrorMatcher::explain_errors_from_circuit(self, nullptr, reduce_to_one_representative_error);
            } else {
                const DetectorErrorModel &model = dem_filter.cast<const DetectorErrorModel &>();
                return ErrorMatcher::explain_errors_from_circuit(self, &model, reduce_to_one_representative_error);
            }
        },
        pybind11::kw_only(),
        pybind11::arg("dem_filter") = pybind11::none(),
        pybind11::arg("reduce_to_one_representative_error") = false,
        clean_doc_string(R"DOC(
            Explains how detector error model errors are produced by circuit errors.

            Args:
                dem_filter: Defaults to None (unused). When used, the output will only
                    contain detector error model errors that appear in the given
                    `stim.DetectorErrorModel`. Any error mechanisms from the detector error
                    model that can't be reproduced using one error from the circuit will
                    also be included in the result, but with an empty list of associated
                    circuit error mechanisms.
                reduce_to_one_representative_error: Defaults to False. When True, the items
                    in the result will contain at most one circuit error mechanism.

            Returns:
                A `List[stim.ExplainedError]` (see `stim.ExplainedError` for more
                information). Each item in the list describes how a detector error model
                error can be produced by individual circuit errors.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit('''
                ...     # Create Bell pair.
                ...     H 0
                ...     CNOT 0 1
                ...
                ...     # Noise.
                ...     DEPOLARIZE1(0.01) 0
                ...
                ...     # Bell basis measurement.
                ...     CNOT 0 1
                ...     H 0
                ...     M 0 1
                ...
                ...     # Both measurements should be False under noiseless execution.
                ...     DETECTOR rec[-1]
                ...     DETECTOR rec[-2]
                ... ''')
                >>> explained_errors = circuit.explain_detector_error_model_errors(
                ...     dem_filter=stim.DetectorErrorModel('error(1) D0 D1'),
                ...     reduce_to_one_representative_error=True,
                ... )
                >>> print(explained_errors[0].circuit_error_locations[0])
                CircuitErrorLocation {
                    flipped_pauli_product: Y0
                    Circuit location stack trace:
                        (after 0 TICKs)
                        at instruction #3 (DEPOLARIZE1) in the circuit
                        at target #1 of the instruction
                        resolving to DEPOLARIZE1(0.01) 0
                }
        )DOC")
            .data());

    c.def(
        "detecting_regions",
        [](const Circuit &self,
           const pybind11::object &included_targets,
           const pybind11::object &included_ticks,
           bool ignore_anticommutation_errors) -> std::map<ExposedDemTarget, std::map<uint64_t, FlexPauliString>> {
            auto stats = self.compute_stats();
            auto included_target_set = py_dem_filter_to_dem_target_set(self, stats, included_targets);
            std::set<uint64_t> included_tick_set;

            if (included_ticks.is_none()) {
                for (uint64_t k = 0; k < stats.num_ticks; k++) {
                    included_tick_set.insert(k);
                }
            } else {
                for (const auto &t : included_ticks) {
                    included_tick_set.insert(pybind11::cast<uint64_t>(t));
                }
            }
            auto result = circuit_to_detecting_regions(
                self, included_target_set, included_tick_set, ignore_anticommutation_errors);
            std::map<ExposedDemTarget, std::map<uint64_t, FlexPauliString>> exposed_result;
            for (const auto &[k, v] : result) {
                exposed_result.insert({ExposedDemTarget(k), std::move(v)});
            }
            return exposed_result;
        },
        pybind11::kw_only(),
        pybind11::arg("targets") = pybind11::none(),
        pybind11::arg("ticks") = pybind11::none(),
        pybind11::arg("ignore_anticommutation_errors") = false,
        clean_doc_string(R"DOC(
            @signature def detecting_regions(self, *, targets: Optional[Iterable[stim.DemTarget | str | Iterable[float]]] = None, ticks: Optional[Iterable[int]] = None) -> Dict[stim.DemTarget, Dict[int, stim.PauliString]]:
            Records where detectors and observables are sensitive to errors over time.

            The result of this method is a nested dictionary, mapping detectors/observables
            and ticks to Pauli sensitivities for that detector/observable at that time.

            For example, if observable 2 has Z-type sensitivity on qubits 5 and 6 during
            tick 3, then `result[stim.target_logical_observable_id(2)][3]` will be equal to
            `stim.PauliString("Z5*Z6")`.

            If you want sensitivities from more places in the circuit, besides just at the
            TICK instructions, you can work around this by making a version of the circuit
            with more TICKs.

            Args:
                targets: Defaults to everything (None).

                    When specified, this should be an iterable of filters where items
                    matching any one filter are included.

                    A variety of filters are supported:
                        stim.DemTarget: Includes the targeted detector or observable.
                        Iterable[float]: Coordinate prefix match. Includes detectors whose
                            coordinate data begins with the same floats.
                        "D": Includes all detectors.
                        "L": Includes all observables.
                        "D#" (e.g. "D5"): Includes the detector with the specified index.
                        "L#" (e.g. "L5"): Includes the observable with the specified index.

                ticks: Defaults to everything (None).
                    When specified, this should be a list of integers corresponding to
                    the tick indices to report sensitivities for.

                ignore_anticommutation_errors: Defaults to False.
                    When set to False, invalid detecting regions that anticommute with a
                    reset will cause the method to raise an exception. When set to True,
                    the offending component will simply be silently dropped. This can
                    result in broken detectors having apparently enormous detecting
                    regions.

            Returns:
                Nested dictionaries keyed first by a `stim.DemTarget` identifying the
                detector or observable, then by the index of the tick, leading to a
                PauliString with that target's error sensitivity at that tick.

                Note you can use `stim.PauliString.pauli_indices` to quickly get to the
                non-identity terms in the sensitivity.

            Examples:
                >>> import stim

                >>> detecting_regions = stim.Circuit('''
                ...     R 0
                ...     TICK
                ...     H 0
                ...     TICK
                ...     CX 0 1
                ...     TICK
                ...     MX 0 1
                ...     DETECTOR rec[-1] rec[-2]
                ... ''').detecting_regions()
                >>> for target, tick_regions in detecting_regions.items():
                ...     print("target", target)
                ...     for tick, sensitivity in tick_regions.items():
                ...         print("    tick", tick, "=", sensitivity)
                target D0
                    tick 0 = +Z_
                    tick 1 = +X_
                    tick 2 = +XX

                >>> circuit = stim.Circuit.generated(
                ...     "surface_code:rotated_memory_x",
                ...     rounds=5,
                ...     distance=4,
                ... )

                >>> detecting_regions = circuit.detecting_regions(
                ...     targets=["L0", (2, 4), stim.DemTarget.relative_detector_id(5)],
                ...     ticks=range(5, 15),
                ... )
                >>> for target, tick_regions in detecting_regions.items():
                ...     print("target", target)
                ...     for tick, sensitivity in tick_regions.items():
                ...         print("    tick", tick, "=", sensitivity)
                target D1
                    tick 5 = +____________________X______________________
                    tick 6 = +____________________Z______________________
                target D5
                    tick 5 = +______X____________________________________
                    tick 6 = +______Z____________________________________
                target D14
                    tick 5 = +__________X_X______XXX_____________________
                    tick 6 = +__________X_X______XZX_____________________
                    tick 7 = +__________X_X______XZX_____________________
                    tick 8 = +__________X_X______XXX_____________________
                    tick 9 = +__________XXX_____XXX______________________
                    tick 10 = +__________XXX_______X______________________
                    tick 11 = +__________X_________X______________________
                    tick 12 = +____________________X______________________
                    tick 13 = +____________________Z______________________
                target D29
                    tick 7 = +____________________Z______________________
                    tick 8 = +____________________X______________________
                    tick 9 = +____________________XX_____________________
                    tick 10 = +___________________XXX_______X_____________
                    tick 11 = +____________X______XXXX______X_____________
                    tick 12 = +__________X_X______XXX_____________________
                    tick 13 = +__________X_X______XZX_____________________
                    tick 14 = +__________X_X______XZX_____________________
                target D44
                    tick 14 = +____________________Z______________________
                target L0
                    tick 5 = +_X________X________X________X______________
                    tick 6 = +_X________X________X________X______________
                    tick 7 = +_X________X________X________X______________
                    tick 8 = +_X________X________X________X______________
                    tick 9 = +_X________X_______XX________X______________
                    tick 10 = +_X________X________X________X______________
                    tick 11 = +_X________XX_______X________XX_____________
                    tick 12 = +_X________X________X________X______________
                    tick 13 = +_X________X________X________X______________
                    tick 14 = +_X________X________X________X______________
            )DOC")
            .data());

    c.def(
        "without_noise",
        &Circuit::without_noise,
        clean_doc_string(R"DOC(
            Returns a copy of the circuit with all noise processes removed.

            Pure noise instructions, such as X_ERROR and DEPOLARIZE2, are not
            included in the result.

            Noisy measurement instructions, like `M(0.001)`, have their noise
            parameter removed.

            Returns:
                A `stim.Circuit` with the same instructions except all noise
                processes have been removed.

            Examples:
                >>> import stim
                >>> stim.Circuit('''
                ...     X_ERROR(0.25) 0
                ...     CNOT 0 1
                ...     M(0.125) 0
                ... ''').without_noise()
                stim.Circuit('''
                    CX 0 1
                    M 0
                ''')
        )DOC")
            .data());

    c.def(
        "flattened",
        &Circuit::flattened,
        clean_doc_string(R"DOC(
            Creates an equivalent circuit without REPEAT or SHIFT_COORDS.

            Returns:
                A `stim.Circuit` with the same instructions in the same order,
                but with loops flattened into repeated instructions and with
                all coordinate shifts inlined.

            Examples:
                >>> import stim
                >>> stim.Circuit('''
                ...     REPEAT 5 {
                ...         MR 0 1
                ...         DETECTOR(0, 0) rec[-2]
                ...         DETECTOR(1, 0) rec[-1]
                ...         SHIFT_COORDS(0, 1)
                ...     }
                ... ''').flattened()
                stim.Circuit('''
                    MR 0 1
                    DETECTOR(0, 0) rec[-2]
                    DETECTOR(1, 0) rec[-1]
                    MR 0 1
                    DETECTOR(0, 1) rec[-2]
                    DETECTOR(1, 1) rec[-1]
                    MR 0 1
                    DETECTOR(0, 2) rec[-2]
                    DETECTOR(1, 2) rec[-1]
                    MR 0 1
                    DETECTOR(0, 3) rec[-2]
                    DETECTOR(1, 3) rec[-1]
                    MR 0 1
                    DETECTOR(0, 4) rec[-2]
                    DETECTOR(1, 4) rec[-1]
                ''')
        )DOC")
            .data());

    c.def(
        "decomposed",
        &simplified_circuit,
        clean_doc_string(R"DOC(
            Recreates the circuit using (mostly) the {H,S,CX,M,R} gate set.

            The intent of this method is to simplify the circuit to use fewer gate types,
            so it's easier for other tools to consume. Currently, this method performs the
            following simplifications:

            - Single qubit cliffords are decomposed into {H,S}.
            - Multi-qubit cliffords are decomposed into {H,S,CX}.
            - Single qubit dissipative gates are decomposed into {H,S,M,R}.
            - Multi-qubit dissipative gates are decomposed into {H,S,CX,M,R}.

            Currently, the following types of gate *aren't* simplified, but they may be
            in the future:

            - Noise instructions (like X_ERROR, DEPOLARIZE2, and E).
            - Annotations (like TICK, DETECTOR, and SHIFT_COORDS).
            - The MPAD instruction.
            - Repeat blocks are not flattened.

            Returns:
                A `stim.Circuit` whose function is equivalent to the original circuit,
                but with most gates decomposed into the {H,S,CX,M,R} gate set.

            Examples:
                >>> import stim

                >>> stim.Circuit('''
                ...     SWAP 0 1
                ... ''').decomposed()
                stim.Circuit('''
                    CX 0 1 1 0 0 1
                ''')

                >>> stim.Circuit('''
                ...     ISWAP 0 1 2 1
                ...     TICK
                ...     MPP !X1*Y2*Z3
                ... ''').decomposed()
                stim.Circuit('''
                    H 0
                    CX 0 1 1 0
                    H 1
                    S 1 0
                    H 2
                    CX 2 1 1 2
                    H 1
                    S 1 2
                    TICK
                    H 1 2
                    S 2
                    H 2
                    S 2 2
                    CX 2 1 3 1
                    M !1
                    CX 2 1 3 1
                    H 2
                    S 2
                    H 2
                    S 2 2
                    H 1
                ''')
        )DOC")
            .data());

    c.def(
        "with_inlined_feedback",
        &circuit_with_inlined_feedback,
        clean_doc_string(R"DOC(
            Returns a circuit without feedback with rewritten detectors/observables.

            When a feedback operation affects the expected parity of a detector or
            observable, the measurement controlling that feedback operation is implicitly
            part of the measurement set that defines the detector or observable. This
            method removes all feedback, but avoids changing the meaning of detectors or
            observables by turning these implicit measurement dependencies into explicit
            measurement dependencies added to the observable or detector.

            This method guarantees that the detector error model derived from the original
            circuit, and the transformed circuit, will be equivalent (modulo floating point
            rounding errors and variations in where loops are placed). Specifically, the
            following should be true for any circuit:

                dem1 = circuit.flattened().detector_error_model()
                dem2 = circuit.with_inlined_feedback().flattened().detector_error_model()
                assert dem1.approx_equals(dem2, 1e-5)

            Returns:
                A `stim.Circuit` with feedback operations removed, with rewritten DETECTOR
                instructions (as needed to avoid changing the meaning of each detector), and
                with additional OBSERVABLE_INCLUDE instructions (as needed to avoid changing
                the meaning of each observable).

                The circuit's function is permitted to differ from the original in that
                any feedback operation can be pushed to the end of the circuit and
                discarded. All non-feedback operations must stay where they are, preserving
                the structure of the circuit.

            Examples:
                >>> import stim

                >>> stim.Circuit('''
                ...     CX 0 1        # copy to measure qubit
                ...     M 1           # measure first time
                ...     CX rec[-1] 1  # use feedback to reset measurement qubit
                ...     CX 0 1        # copy to measure qubit
                ...     M 1           # measure second time
                ...     DETECTOR rec[-1] rec[-2]
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').with_inlined_feedback()
                stim.Circuit('''
                    CX 0 1
                    M 1
                    OBSERVABLE_INCLUDE(0) rec[-1]
                    CX 0 1
                    M 1
                    DETECTOR rec[-1]
                    OBSERVABLE_INCLUDE(0) rec[-1]
                ''')
        )DOC")
            .data());

    c.def(
        "inverse",
        [](const Circuit &self) -> Circuit {
            return self.inverse();
        },
        clean_doc_string(R"DOC(
            Returns a circuit that applies the same operations but inverted and in reverse.

            If circuit starts with QUBIT_COORDS instructions, the returned circuit will
            still have the same QUBIT_COORDS instructions in the same order at the start.

            Returns:
                A `stim.Circuit` that applies inverted operations in the reverse order.

            Raises:
                ValueError: The circuit contains operations that don't have an inverse,
                    such as measurements. There are also some unsupported operations
                    such as SHIFT_COORDS.

            Examples:
                >>> import stim

                >>> stim.Circuit('''
                ...     S 0 1
                ...     ISWAP 0 1 1 2
                ... ''').inverse()
                stim.Circuit('''
                    ISWAP_DAG 1 2 0 1
                    S_DAG 1 0
                ''')

                >>> stim.Circuit('''
                ...     QUBIT_COORDS(1, 2) 0
                ...     QUBIT_COORDS(4, 3) 1
                ...     QUBIT_COORDS(9, 5) 2
                ...     H 0 1
                ...     REPEAT 100 {
                ...         CX 0 1 1 2
                ...         TICK
                ...         S 1 2
                ...     }
                ... ''').inverse()
                stim.Circuit('''
                    QUBIT_COORDS(1, 2) 0
                    QUBIT_COORDS(4, 3) 1
                    QUBIT_COORDS(9, 5) 2
                    REPEAT 100 {
                        S_DAG 2 1
                        TICK
                        CX 1 2 0 1
                    }
                    H 1 0
                ''')
        )DOC")
            .data());

    c.def(
        "has_flow",
        [](const Circuit &self, const Flow<MAX_BITWORD_WIDTH> &flow, bool unsigned_only) -> bool {
            std::span<const Flow<MAX_BITWORD_WIDTH>> flows = {&flow, &flow + 1};
            if (unsigned_only) {
                return check_if_circuit_has_unsigned_stabilizer_flows<MAX_BITWORD_WIDTH>(self, flows)[0];
            } else {
                auto rng = externally_seeded_rng();
                return sample_if_circuit_has_stabilizer_flows<MAX_BITWORD_WIDTH>(256, rng, self, flows)[0];
            }
        },
        pybind11::arg("flow"),
        pybind11::kw_only(),
        pybind11::arg("unsigned") = false,
        clean_doc_string(R"DOC(
            @signature def has_flow(self, flow: stim.Flow, *, unsigned: bool = False) -> bool:
            Determines if the circuit has the given stabilizer flow or not.

            A circuit has a stabilizer flow P -> Q if it maps the instantaneous stabilizer
            P at the start of the circuit to the instantaneous stabilizer Q at the end of
            the circuit. The flow may be mediated by certain measurements. For example,
            a lattice surgery CNOT involves an MXX measurement and an MZZ measurement, and
            the CNOT flows implemented by the circuit involve these measurements.

            A flow like P -> Q means the circuit transforms P into Q.
            A flow like 1 -> P means the circuit prepares P.
            A flow like P -> 1 means the circuit measures P.
            A flow like 1 -> 1 means the circuit contains a check (could be a DETECTOR).

            Args:
                flow: The flow to check for.
                unsigned: Defaults to False. When False, the flows must be correct including
                    the sign of the Pauli strings. When True, only the Pauli terms need to
                    be correct; the signs are permitted to be inverted. In effect, this
                    requires the circuit to be correct up to Pauli gates.

            Returns:
                True if the circuit has the given flow; False otherwise.

            Examples:
                >>> import stim

                >>> m = stim.Circuit('M 0')
                >>> m.has_flow(stim.Flow('Z -> Z'))
                True
                >>> m.has_flow(stim.Flow('X -> X'))
                False
                >>> m.has_flow(stim.Flow('Z -> I'))
                False
                >>> m.has_flow(stim.Flow('Z -> I xor rec[-1]'))
                True
                >>> m.has_flow(stim.Flow('Z -> rec[-1]'))
                True

                >>> cx58 = stim.Circuit('CX 5 8')
                >>> cx58.has_flow(stim.Flow('X5 -> X5*X8'))
                True
                >>> cx58.has_flow(stim.Flow('X_ -> XX'))
                False
                >>> cx58.has_flow(stim.Flow('_____X___ -> _____X__X'))
                True

                >>> stim.Circuit('''
                ...     RY 0
                ... ''').has_flow(stim.Flow(
                ...     output=stim.PauliString("Y"),
                ... ))
                True

                >>> stim.Circuit('''
                ...     RY 0
                ... ''').has_flow(stim.Flow(
                ...     output=stim.PauliString("X"),
                ... ))
                False

                >>> stim.Circuit('''
                ...     CX 0 1
                ... ''').has_flow(stim.Flow(
                ...     input=stim.PauliString("+X_"),
                ...     output=stim.PauliString("+XX"),
                ... ))
                True

                >>> stim.Circuit('''
                ...     # Lattice surgery CNOT
                ...     R 1
                ...     MXX 0 1
                ...     MZZ 1 2
                ...     MX 1
                ... ''').has_flow(stim.Flow(
                ...     input=stim.PauliString("+X_X"),
                ...     output=stim.PauliString("+__X"),
                ...     measurements=[0, 2],
                ... ))
                True

                >>> stim.Circuit('''
                ...     H 0
                ... ''').has_flow(
                ...     stim.Flow("Y -> Y"),
                ...     unsigned=True,
                ... )
                True

                >>> stim.Circuit('''
                ...     H 0
                ... ''').has_flow(
                ...     stim.Flow("Y -> Y"),
                ...     unsigned=False,
                ... )
                False

            Caveats:
                Currently, the unsigned=False version of this method is implemented by
                performing 256 randomized tests. Each test has a 50% chance of a false
                positive, and a 0% chance of a false negative. So, when the method returns
                True, there is technically still a 2^-256 chance the circuit doesn't have
                the flow. This is lower than the chance of a cosmic ray flipping the result.
        )DOC")
            .data());

    c.def(
        "has_all_flows",
        [](const Circuit &self, const std::vector<Flow<MAX_BITWORD_WIDTH>> &flows, bool unsigned_only) -> bool {
            std::vector<bool> results;
            if (unsigned_only) {
                results = check_if_circuit_has_unsigned_stabilizer_flows<MAX_BITWORD_WIDTH>(self, flows);
            } else {
                auto rng = externally_seeded_rng();
                results = sample_if_circuit_has_stabilizer_flows<MAX_BITWORD_WIDTH>(256, rng, self, flows);
            }
            for (auto b : results) {
                if (!b) {
                    return false;
                }
            }
            return true;
        },
        pybind11::arg("flows"),
        pybind11::kw_only(),
        pybind11::arg("unsigned") = false,
        clean_doc_string(R"DOC(
            @signature def has_all_flows(self, flows: Iterable[stim.Flow], *, unsigned: bool = False) -> bool:
            Determines if the circuit has all the given stabilizer flow or not.

            This is a faster version of `all(c.has_flow(f) for f in flows)`. It's faster
            because, behind the scenes, the circuit can be iterated once instead of once
            per flow.

            Args:
                flows: An iterable of `stim.Flow` instances representing the flows to check.
                unsigned: Defaults to False. When False, the flows must be correct including
                    the sign of the Pauli strings. When True, only the Pauli terms need to
                    be correct; the signs are permitted to be inverted. In effect, this
                    requires the circuit to be correct up to Pauli gates.

            Returns:
                True if the circuit has the given flow; False otherwise.

            Examples:
                >>> import stim

                >>> stim.Circuit('H 0').has_all_flows([
                ...     stim.Flow('X -> Z'),
                ...     stim.Flow('Y -> Y'),
                ...     stim.Flow('Z -> X'),
                ... ])
                False

                >>> stim.Circuit('H 0').has_all_flows([
                ...     stim.Flow('X -> Z'),
                ...     stim.Flow('Y -> -Y'),
                ...     stim.Flow('Z -> X'),
                ... ])
                True

                >>> stim.Circuit('H 0').has_all_flows([
                ...     stim.Flow('X -> Z'),
                ...     stim.Flow('Y -> Y'),
                ...     stim.Flow('Z -> X'),
                ... ], unsigned=True)
                True

            Caveats:
                Currently, the unsigned=False version of this method is implemented by
                performing 256 randomized tests. Each test has a 50% chance of a false
                positive, and a 0% chance of a false negative. So, when the method returns
                True, there is technically still a 2^-256 chance the circuit doesn't have
                the flow. This is lower than the chance of a cosmic ray flipping the result.
        )DOC")
            .data());

    c.def(
        "time_reversed_for_flows",
        [](const Circuit &self,
           const std::vector<Flow<MAX_BITWORD_WIDTH>> &flows,
           bool dont_turn_measurements_into_resets) -> pybind11::object {
            auto [inv_circuit, inv_flows] =
                circuit_inverse_qec<MAX_BITWORD_WIDTH>(self, flows, dont_turn_measurements_into_resets);
            return pybind11::make_tuple(inv_circuit, inv_flows);
        },
        pybind11::arg("flows"),
        pybind11::kw_only(),
        pybind11::arg("dont_turn_measurements_into_resets") = false,
        clean_doc_string(R"DOC(
            @signature def time_reversed_for_flows(self, flows: Iterable[stim.Flow], *, dont_turn_measurements_into_resets: bool = False) -> Tuple[stim.Circuit, List[stim.Flow]]:
            Time-reverses the circuit while preserving error correction structure.

            This method returns a circuit that has the same internal detecting regions
            as the given circuit, as well as the same internal-to-external flows given
            in the `flows` argument, except they are all time-reversed. For example, if
            you pass a fault tolerant preparation circuit into this method (1 -> Z), the
            result will be a fault tolerant *measurement* circuit (Z -> 1). Or, if you
            pass a fault tolerant C_XYZ circuit into this method (X->Y, Y->Z, and Z->X),
            the result will be a fault tolerant C_ZYX circuit (X->Z, Y->X, and Z->Y).

            Note that this method doesn't guarantee that it will preserve the *sign* of the
            detecting regions or stabilizer flows. For example, inverting a memory circuit
            that preserves a logical observable (X->X and Z->Z) may produce a
            memory circuit that always bit flips the logical observable (X->X and Z->-Z) or
            that dynamically adjusts the logical observable in response to measurements
            (like "X -> X xor rec[-1]" and "Z -> Z xor rec[-2]").

            This method will turn time-reversed resets into measurements, and attempts to
            turn time-reversed measurements into resets. A measurement will time-reverse
            into a reset if some annotated detectors, annotated observables, or given flows
            have detecting regions with sensitivity just before the measurement but none
            have detecting regions with sensitivity after the measurement.

            In some cases this method will have to introduce new operations. In particular,
            when a measurement-reset operation has a noisy result, time-reversing this
            measurement noise produces reset noise. But the measure-reset operations don't
            have built-in reset noise, so the reset noise is specified by adding an X_ERROR
            or Z_ERROR noise instruction after the time-reversed measure-reset operation.

            Args:
                flows: Flows you care about, that reach past the start/end of the given
                    circuit. The result will contain an inverted flow for each of these
                    given flows. You need this information because it reveals the
                    measurements needed to produce the inverted flows that you care
                    about.

                    An exception will be raised if the circuit doesn't have all these
                    flows. The inverted circuit will have the inverses of these flows
                    (ignoring sign).
                dont_turn_measurements_into_resets: Defaults to False. When set to
                    True, measurements will time-reverse into measurements even if
                    nothing is sensitive to the measured qubit after the measurement
                    completes. This guarantees the output circuit has *all* flows
                    that the input circuit has (up to sign and feedback), even ones
                    that aren't annotated.

            Returns:
                An (inverted_circuit, inverted_flows) tuple.

                inverted_circuit is the qec inverse of the given circuit.

                inverted_flows is a list of flows, matching up by index with the flows
                given as arguments to the method. The input, output, and sign fields
                of these flows are boring. The useful field is measurement_indices,
                because it's difficult to predict which measurements are needed for
                the inverted flow due to effects such as implicitly-included resets
                inverting into explicitly-included measurements.

            Caveats:
                Currently, this method doesn't compute the sign of the inverted flows.
                It unconditionally sets the sign to False.

            Examples:
                >>> import stim

                >>> inv_circuit, inv_flows = stim.Circuit('''
                ...     R 0
                ...     H 0
                ...     S 0
                ...     MY 0
                ...     DETECTOR rec[-1]
                ... ''').time_reversed_for_flows([])
                >>> inv_circuit
                stim.Circuit('''
                    RY 0
                    S_DAG 0
                    H 0
                    M 0
                    DETECTOR rec[-1]
                ''')
                >>> inv_flows
                []

                >>> inv_circuit, inv_flows = stim.Circuit('''
                ...     M 0
                ... ''').time_reversed_for_flows([
                ...     stim.Flow("Z -> rec[-1]"),
                ... ])
                >>> inv_circuit
                stim.Circuit('''
                    R 0
                ''')
                >>> inv_flows
                [stim.Flow("1 -> Z")]
                >>> inv_circuit.has_all_flows(inv_flows, unsigned=True)
                True

                >>> inv_circuit, inv_flows = stim.Circuit('''
                ...     R 0
                ... ''').time_reversed_for_flows([
                ...     stim.Flow("1 -> Z"),
                ... ])
                >>> inv_circuit
                stim.Circuit('''
                    M 0
                ''')
                >>> inv_flows
                [stim.Flow("Z -> rec[-1]")]

                >>> inv_circuit, inv_flows = stim.Circuit('''
                ...     M 0
                ... ''').time_reversed_for_flows([
                ...     stim.Flow("1 -> Z xor rec[-1]"),
                ... ])
                >>> inv_circuit
                stim.Circuit('''
                    M 0
                ''')
                >>> inv_flows
                [stim.Flow("Z -> rec[-1]")]

                >>> inv_circuit, inv_flows = stim.Circuit('''
                ...     M 0
                ... ''').time_reversed_for_flows(
                ...     flows=[stim.Flow("Z -> rec[-1]")],
                ...     dont_turn_measurements_into_resets=True,
                ... )
                >>> inv_circuit
                stim.Circuit('''
                    M 0
                ''')
                >>> inv_flows
                [stim.Flow("1 -> Z xor rec[-1]")]

                >>> inv_circuit, inv_flows = stim.Circuit('''
                ...     MR(0.125) 0
                ... ''').time_reversed_for_flows([])
                >>> inv_circuit
                stim.Circuit('''
                    MR 0
                    X_ERROR(0.125) 0
                ''')
                >>> inv_flows
                []

                >>> inv_circuit, inv_flows = stim.Circuit('''
                ...     MXX 0 1
                ...     H 0
                ... ''').time_reversed_for_flows([
                ...     stim.Flow("ZZ -> YY xor rec[-1]"),
                ...     stim.Flow("ZZ -> XZ"),
                ... ])
                >>> inv_circuit
                stim.Circuit('''
                    H 0
                    MXX 0 1
                ''')
                >>> inv_flows
                [stim.Flow("YY -> ZZ xor rec[-1]"), stim.Flow("XZ -> ZZ")]

                >>> stim.Circuit.generated(
                ...     "surface_code:rotated_memory_x",
                ...     distance=2,
                ...     rounds=1,
                ... ).time_reversed_for_flows([])[0]
                stim.Circuit('''
                    QUBIT_COORDS(1, 1) 1
                    QUBIT_COORDS(2, 0) 2
                    QUBIT_COORDS(3, 1) 3
                    QUBIT_COORDS(1, 3) 6
                    QUBIT_COORDS(2, 2) 7
                    QUBIT_COORDS(3, 3) 8
                    QUBIT_COORDS(2, 4) 12
                    RX 8 6 3 1
                    MR 12 7 2
                    TICK
                    H 12 2
                    TICK
                    CX 1 7 12 6
                    TICK
                    CX 6 7 12 8
                    TICK
                    CX 3 7 2 1
                    TICK
                    CX 8 7 2 3
                    TICK
                    H 12 2
                    TICK
                    M 12 7 2
                    DETECTOR(2, 0, 1) rec[-1]
                    DETECTOR(2, 4, 1) rec[-3]
                    MX 8 6 3 1
                    DETECTOR(2, 0, 0) rec[-5] rec[-2] rec[-1]
                    DETECTOR(2, 4, 0) rec[-7] rec[-4] rec[-3]
                    OBSERVABLE_INCLUDE(0) rec[-3] rec[-1]
                ''')
        )DOC")
            .data());

    c.def(
        "diagram",
        &circuit_diagram,
        pybind11::arg("type") = "timeline-text",
        pybind11::kw_only(),
        pybind11::arg("tick") = pybind11::none(),
        pybind11::arg("filter_coords") = pybind11::none(),
        clean_doc_string(R"DOC(
            @signature def diagram(self, type: str = 'timeline-text', *, tick: Union[None, int, range] = None, filter_coords: Iterable[Union[Iterable[float], stim.DemTarget]] = ((),)) -> 'stim._DiagramHelper':
            Returns a diagram of the circuit, from a variety of options.

            Args:
                type: The type of diagram. Available types are:
                    "timeline-text" (default): An ASCII diagram of the
                        operations applied by the circuit over time. Includes
                        annotations showing the measurement record index that
                        each measurement writes to, and the measurements used
                        by detectors.
                    "timeline-svg": An SVG image of the operations applied by
                        the circuit over time. Includes annotations showing the
                        measurement record index that each measurement writes
                        to, and the measurements used by detectors.
                    "timeline-svg-html": A resizable SVG image viewer of the
                        operations applied by the circuit over time. Includes
                        annotations showing the measurement record index that
                        each measurement writes to, and the measurements used
                        by detectors.
                    "timeline-3d": A 3d model, in GLTF format, of the operations
                        applied by the circuit over time.
                    "timeline-3d-html": Same 3d model as 'timeline-3d' but
                        embedded into an HTML web page containing an interactive
                        THREE.js viewer for the 3d model.
                    "detslice-text": An ASCII diagram of the stabilizers
                        that detectors declared by the circuit correspond to
                        during the TICK instruction identified by the `tick`
                        argument.
                    "detslice-svg": An SVG image of the stabilizers
                        that detectors declared by the circuit correspond to
                        during the TICK instruction identified by the `tick`
                        argument. For example, a detector slice diagram of a
                        CSS surface code circuit during the TICK between a
                        measurement layer and a reset layer will produce the
                        usual diagram of a surface code.

                        Uses the Pauli color convention XYZ=RGB.
                    "detslice-svg-html": Same as detslice-svg but the SVG image
                        is inside a resizable HTML iframe.
                    "matchgraph-svg": An SVG image of the match graph extracted
                        from the circuit by stim.Circuit.detector_error_model.
                    "matchgraph-svg-html": Same as matchgraph-svg but the SVG image
                        is inside a resizable HTML iframe.
                    "matchgraph-3d": An 3D model of the match graph extracted
                        from the circuit by stim.Circuit.detector_error_model.
                    "matchgraph-3d-html": Same 3d model as 'match-graph-3d' but
                        embedded into an HTML web page containing an interactive
                        THREE.js viewer for the 3d model.
                    "timeslice-svg": An SVG image of the operations applied
                        between two TICK instructions in the circuit, with the
                        operations laid out in 2d.
                    "timeslice-svg-html": Same as timeslice-svg but the SVG image
                        is inside a resizable HTML iframe.
                    "detslice-with-ops-svg": A combination of timeslice-svg
                        and detslice-svg, with the operations overlaid
                        over the detector slices taken from the TICK after the
                        operations were applied.
                    "detslice-with-ops-svg-html": Same as detslice-with-ops-svg
                        but the SVG image is inside a resizable HTML iframe.
                    "interactive" or "interactive-html": An HTML web page
                        containing Crumble (an interactive editor for 2D
                        stabilizer circuits) initialized with the given circuit
                        as its default contents.
                tick: Required for detector and time slice diagrams. Specifies
                    which TICK instruction, or range of TICK instructions, to
                    slice at. Note that the first TICK instruction in the
                    circuit corresponds tick=1. The value tick=0 refers to the
                    very start of the circuit.

                    Passing `range(A, B)` for a detector slice will show the
                    slices for ticks A through B including A but excluding B.

                    Passing `range(A, B)` for a time slice will show the
                    operations between tick A and tick B.
                filter_coords: A set of acceptable coordinate prefixes, or
                    desired stim.DemTargets. For detector slice diagrams, only
                    detectors match one of the filters are included. If no filter
                    is specified, all detectors are included (but no observables).
                    To include an observable, add it as one of the filters.

            Returns:
                An object whose `__str__` method returns the diagram, so that
                writing the diagram to a file works correctly. The returned
                object may also define methods such as `_repr_html_`, so that
                ipython notebooks recognize it can be shown using a specialized
                viewer instead of as raw text.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit('''
                ...     H 0
                ...     CNOT 0 1 1 2
                ... ''')

                >>> print(circuit.diagram())
                q0: -H-@---
                       |
                q1: ---X-@-
                         |
                q2: -----X-

                >>> circuit = stim.Circuit('''
                ...     H 0
                ...     CNOT 0 1
                ...     TICK
                ...     M 0 1
                ...     DETECTOR rec[-1] rec[-2]
                ... ''')

                >>> print(circuit.diagram("detslice-text", tick=1))
                q0: -Z:D0-
                     |
                q1: -Z:D0-
        )DOC")
            .data());

    c.def(
        "to_crumble_url",
        &export_crumble_url,
        clean_doc_string(R"DOC(
            Returns a URL that opens up crumble and loads this circuit into it.

            Crumble is a tool for editing stabilizer circuits, and visualizing their
            stabilizer flows. Its source code is in the `glue/crumble` directory of
            the stim code repository on github. A prebuilt version is made available
            at https://algassert.com/crumble, which is what the URL returned by this
            method will point to.

            Returns:
                A URL that can be opened in a web browser.

            Examples:
                >>> import stim
                >>> stim.Circuit('''
                ...     H 0
                ...     CNOT 0 1
                ...     S 1
                ... ''').to_crumble_url()
                'https://algassert.com/crumble#circuit=H_0;CX_0_1;S_1'
        )DOC")
            .data());

    c.def(
        "to_quirk_url",
        &export_quirk_url,
        clean_doc_string(R"DOC(
            Returns a URL that opens up quirk and loads this circuit into it.

            Quirk is an open source drag and drop circuit editor with support for up to 16
            qubits. Its source code is available at https://github.com/strilanc/quirk
            and a prebuilt version is available at https://algassert.com/quirk, which is
            what the URL returned by this method will point to.

            Quirk doesn't support features like noise, feedback, or detectors. This method
            will simply drop any unsupported operations from the circuit when producing
            the URL.

            Returns:
                A URL that can be opened in a web browser.

            Examples:
                >>> import stim
                >>> stim.Circuit('''
                ...     H 0
                ...     CNOT 0 1
                ...     S 1
                ... ''').to_quirk_url()
                'https://algassert.com/quirk#circuit={"cols":[["H"],["•","X"],[1,"Z^½"]]}'
        )DOC")
            .data());
}
