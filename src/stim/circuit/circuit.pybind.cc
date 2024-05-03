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
#include "stim/dem/detector_error_model_target.pybind.h"
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
#include "stim/stabilizers/flow.h"

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
}
