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

#include "../simulators/tableau_simulator.h"

#include "../py/base.pybind.h"
#include "../stabilizers/tableau.h"
#include "tableau_simulator.pybind.h"

struct TempViewableData {
    std::vector<uint32_t> targets;
    TempViewableData(std::vector<uint32_t> targets) : targets(std::move(targets)) {
    }
    operator OperationData() const {
        // Temporarily remove const correctness but then immediately restore it.
        VectorView<uint32_t> v{(std::vector<uint32_t> *)&targets, 0, targets.size()};
        return {0, v};
    }
};

TableauSimulator create_tableau_simulator() {
    return TableauSimulator(64, PYBIND_SHARED_RNG());
}

TempViewableData args_to_targets(TableauSimulator &self, const pybind11::args &args) {
    std::vector<uint32_t> arguments;
    size_t max_q = 0;
    try {
        for (const auto &e : args) {
            size_t q = e.cast<uint32_t>();
            max_q = std::max(max_q, q);
            arguments.push_back(q);
        }
    } catch (const pybind11::cast_error &) {
        throw std::out_of_range("Target qubits must be non-negative integers.");
    }

    // Note: quadratic behavior.
    self.ensure_large_enough_for_qubits(max_q + 1);

    return TempViewableData(arguments);
}

TempViewableData args_to_target_pairs(TableauSimulator &self, const pybind11::args &args) {
    if (pybind11::len(args) & 1) {
        throw std::out_of_range("Two qubit operation requires an even number of targets.");
    }
    return args_to_targets(self, args);
}

void pybind_tableau_simulator(pybind11::module &m) {
    pybind11::class_<TableauSimulator>(
        m, "TableauSimulator", "A quantum stabilizer circuit simulator whose state is a stabilizer tableau.")
        .def(
            "h",
            [](TableauSimulator &self, pybind11::args args) {
                self.H_XZ(args_to_targets(self, args));
            })
        .def(
            "x",
            [](TableauSimulator &self, pybind11::args args) {
                self.X(args_to_targets(self, args));
            })
        .def(
            "y",
            [](TableauSimulator &self, pybind11::args args) {
                self.Y(args_to_targets(self, args));
            })
        .def(
            "z",
            [](TableauSimulator &self, pybind11::args args) {
                self.Z(args_to_targets(self, args));
            })
        .def(
            "s",
            [](TableauSimulator &self, pybind11::args args) {
                self.SQRT_Z(args_to_targets(self, args));
            })
        .def(
            "s_dag",
            [](TableauSimulator &self, pybind11::args args) {
                self.SQRT_Z_DAG(args_to_targets(self, args));
            })
        .def(
            "sqrt_x",
            [](TableauSimulator &self, pybind11::args args) {
                self.SQRT_X(args_to_targets(self, args));
            })
        .def(
            "sqrt_x_dag",
            [](TableauSimulator &self, pybind11::args args) {
                self.SQRT_X_DAG(args_to_targets(self, args));
            })
        .def(
            "sqrt_y",
            [](TableauSimulator &self, pybind11::args args) {
                self.SQRT_Y(args_to_targets(self, args));
            })
        .def(
            "sqrt_y_dag",
            [](TableauSimulator &self, pybind11::args args) {
                self.SQRT_Y_DAG(args_to_targets(self, args));
            })
        .def(
            "cnot",
            [](TableauSimulator &self, pybind11::args args) {
                self.ZCX(args_to_target_pairs(self, args));
            })
        .def(
            "cz",
            [](TableauSimulator &self, pybind11::args args) {
                self.ZCZ(args_to_target_pairs(self, args));
            })
        .def(
            "cy",
            [](TableauSimulator &self, pybind11::args args) {
                self.ZCY(args_to_target_pairs(self, args));
            })
        .def(
            "reset",
            [](TableauSimulator &self, pybind11::args args) {
                self.reset(args_to_targets(self, args));
            })
        .def(
            "measure",
            [](TableauSimulator &self, uint32_t target) {
                self.measure(TempViewableData({target}));
                return (bool)self.measurement_record.back();
            },
            R"DOC(
            Measures a single qubit.

            Unlike the other methods on TableauSimulator, this one does not broadcast
            over multiple targets. This is to avoid returning a list, which would
            create a pitfall where typing `if sim.measure(qubit)` would be a bug.

            To measure multiple qubits, use `TableauSimulator.measure_many`.
         )DOC")
        .def(
            "measure_many",
            [](TableauSimulator &self, pybind11::args args) {
                auto converted_args = args_to_targets(self, args);
                self.measure(converted_args);
                auto e = self.measurement_record.end();
                return std::vector<bool>(e - converted_args.targets.size(), e);
            })
        .def(pybind11::init(&create_tableau_simulator));
}
