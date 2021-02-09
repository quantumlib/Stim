#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "../stabilizers/tableau.h"
#include "../probability_util.h"
#include "../simulators/tableau_simulator.h"

#define STRINGIFY(x) #x

struct Dat {
    std::vector<uint32_t> targets;
    Dat(std::vector<uint32_t> targets);
    operator OperationData() const;
};
Dat::Dat(std::vector<uint32_t> targets) : targets(std::move(targets)) {
}
Dat::operator OperationData() const {
    return {0, {&targets, 0, targets.size()}};
}

static bool shared_rng_initialized;
static std::mt19937_64 shared_rng;

std::mt19937_64 &SHARED_RNG() {
    if (!shared_rng_initialized) {
        shared_rng = externally_seeded_rng();
        shared_rng_initialized = true;
    }
    return shared_rng;
}

TableauSimulator create_tableau_simulator() {
    return TableauSimulator(64, SHARED_RNG());
}

Dat args_to_targets(TableauSimulator &self, const pybind11::args &args) {
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

    return Dat(arguments);
}

Dat args_to_targets2(TableauSimulator &self, const pybind11::args &args) {
    if (pybind11::len(args) & 1) {
        throw std::out_of_range("Two qubit operation requires an even number of targets.");
    }
    return args_to_targets(self, args);
}


PYBIND11_MODULE(stim, m) {
    m.doc() = R"pbdoc(
        Stim: A stabilizer circuit simulator.
    )pbdoc";

    m.attr("__version__") = STRINGIFY(VERSION_INFO);

    pybind11::class_<TableauSimulator>(m, "TableauSimulator")
        .def("h", [](TableauSimulator &self, pybind11::args args) {
            self.H_XZ(args_to_targets(self, args));
        })
        .def("x", [](TableauSimulator &self, pybind11::args args) {
            self.X(args_to_targets(self, args));
        })
        .def("y", [](TableauSimulator &self, pybind11::args args) {
            self.Y(args_to_targets(self, args));
        })
        .def("z", [](TableauSimulator &self, pybind11::args args) {
            self.Z(args_to_targets(self, args));
        })
        .def("s", [](TableauSimulator &self, pybind11::args args) {
            self.SQRT_Z(args_to_targets(self, args));
        })
        .def("s_dag", [](TableauSimulator &self, pybind11::args args) {
            self.SQRT_Z_DAG(args_to_targets(self, args));
        })
        .def("sqrt_x", [](TableauSimulator &self, pybind11::args args) {
            self.SQRT_X(args_to_targets(self, args));
        })
        .def("sqrt_x_dag", [](TableauSimulator &self, pybind11::args args) {
            self.SQRT_X_DAG(args_to_targets(self, args));
        })
        .def("sqrt_y", [](TableauSimulator &self, pybind11::args args) {
            self.SQRT_Y(args_to_targets(self, args));
        })
        .def("sqrt_y_dag", [](TableauSimulator &self, pybind11::args args) {
            self.SQRT_Y_DAG(args_to_targets(self, args));
        })
        .def("cnot", [](TableauSimulator &self, pybind11::args args) {
            self.ZCX(args_to_targets2(self, args));
        })
        .def("cz", [](TableauSimulator &self, pybind11::args args) {
            self.ZCZ(args_to_targets2(self, args));
        })
        .def("cy", [](TableauSimulator &self, pybind11::args args) {
            self.ZCY(args_to_targets2(self, args));
        })
        .def("reset", [](TableauSimulator &self, pybind11::args args) {
            self.reset(args_to_targets(self, args));
        })
        .def("measure", [](TableauSimulator &self, pybind11::args args) {
            self.measure(args_to_targets(self, args));
            std::vector<bool> results;
            while (!self.recorded_measurement_results.empty()) {
                results.push_back(self.recorded_measurement_results.front());
                self.recorded_measurement_results.pop();
            }
            return results;
        })
        .def(pybind11::init(&create_tableau_simulator));
}
