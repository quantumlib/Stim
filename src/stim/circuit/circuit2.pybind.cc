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

#include <pybind11/stl.h>

#include "stim/circuit/circuit.pybind.h"
#include "stim/cmd/command_help.h"

#include "stim/dem/detector_error_model_target.pybind.h"
#include "stim/util_top/circuit_flow_generators.h"
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

static std::set<DemTarget> py_dem_filter_to_dem_target_set(
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

void stim_pybind::pybind_circuit_methods_extra(pybind11::module &, pybind11::class_<Circuit> &c) {
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
                        - Support for operations on classical bits (only version 3).
                            This means DETECTOR and OBSERVABLE_INCLUDE only work with
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

            This method ignores any noise in the circuit.

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
                ...     X_ERROR(0.1) 0
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
            for (bool b : results) {
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

            This method ignores any noise in the circuit.

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
        "flow_generators",
        &circuit_flow_generators<MAX_BITWORD_WIDTH>,
        clean_doc_string(R"DOC(
            @signature def flow_generators(self) -> List[stim.Flow]:
            Returns a list of flows that generate all of the circuit's flows.

            Every stabilizer flow that the circuit implements is a product of some
            subset of the returned generators. Every returned flow will be a flow
            of the circuit.

            Returns:
                A list of flow generators for the circuit.

            Examples:
                >>> import stim

                >>> stim.Circuit("H 0").flow_generators()
                [stim.Flow("X -> Z"), stim.Flow("Z -> X")]

                >>> stim.Circuit("M 0").flow_generators()
                [stim.Flow("1 -> Z xor rec[0]"), stim.Flow("Z -> rec[0]")]

                >>> stim.Circuit("RX 0").flow_generators()
                [stim.Flow("1 -> X")]

                >>> for flow in stim.Circuit("MXX 0 1").flow_generators():
                ...     print(flow)
                1 -> XX xor rec[0]
                _X -> _X
                X_ -> _X xor rec[0]
                ZZ -> ZZ

                >>> for flow in stim.Circuit.generated(
                ...     "repetition_code:memory",
                ...     rounds=2,
                ...     distance=3,
                ...     after_clifford_depolarization=1e-3,
                ... ).flow_generators():
                ...     print(flow)
                1 -> rec[0]
                1 -> rec[1]
                1 -> rec[2]
                1 -> rec[3]
                1 -> rec[4]
                1 -> rec[5]
                1 -> rec[6]
                1 -> ____Z
                1 -> ___Z_
                1 -> __Z__
                1 -> _Z___
                1 -> Z____
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
        "to_crumble_url",
        [](const Circuit &self, bool skip_detectors, pybind11::object &obj_mark) {
            std::map<int, std::vector<ExplainedError>> mark;
            if (!obj_mark.is_none()) {
                mark = pybind11::cast<std::map<int, std::vector<ExplainedError>>>(obj_mark);
            }
            return export_crumble_url(self, skip_detectors, mark);
        },
        pybind11::kw_only(),
        pybind11::arg("skip_detectors") = false,
        pybind11::arg("mark") = pybind11::none(),
        clean_doc_string(R"DOC(
            @signature def to_crumble_url(self, *, skip_detectors: bool = False, mark: Optional[dict[int, list[stim.ExplainedError]]] = None) -> str:
            Returns a URL that opens up crumble and loads this circuit into it.

            Crumble is a tool for editing stabilizer circuits, and visualizing their
            stabilizer flows. Its source code is in the `glue/crumble` directory of
            the stim code repository on github. A prebuilt version is made available
            at https://algassert.com/crumble, which is what the URL returned by this
            method will point to.

            Args:
                skip_detectors: Defaults to False. If set to True, detectors from the
                    circuit aren't included in the crumble URL. This can reduce visual
                    clutter in crumble, and improve its performance, since it doesn't
                    need to indicate or track the sensitivity regions of detectors.
                mark: Defaults to None (no marks). If set to a dictionary from int to
                    errors, such as `mark={1: circuit.shortest_graphlike_error()}`,
                    then the errors will be highlighted and tracked forward by crumble.

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

                >>> circuit = stim.Circuit('''
                ...     M(0.25) 0 1 2
                ...     DETECTOR rec[-1] rec[-2]
                ...     DETECTOR rec[-2] rec[-3]
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''')
                >>> err = circuit.shortest_graphlike_error(canonicalize_circuit_errors=True)
                >>> circuit.to_crumble_url(skip_detectors=True, mark={1: err})
                'https://algassert.com/crumble#circuit=;TICK;MARKX(1)1;MARKX(1)2;MARKX(1)0;TICK;M(0.25)0_1_2;OI(0)rec[-1]'
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
}
