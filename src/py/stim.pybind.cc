#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include "../stabilizers/tableau.h"
#include "../probability_util.h"
#include "../simulators/tableau_simulator.h"
#include "../simulators/frame_simulator.h"

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

struct CompiledCircuitSampler {
    const simd_bits ref;
    const Circuit circuit;

    CompiledCircuitSampler(Circuit circuit) : ref(TableauSimulator::reference_sample_circuit(circuit)), circuit(std::move(circuit)) {
    }

    pybind11::array_t<uint8_t> sample(size_t num_samples) {
        auto sample = FrameSimulator::sample(circuit, ref, num_samples, SHARED_RNG());
        return pybind11::array_t<uint8_t>(pybind11::buffer_info(
                sample.data.u8,
                sizeof(uint8_t),
                pybind11::format_descriptor<uint8_t>::value,
                2,
                {num_samples, (circuit.num_measurements + 7) / 8},
                {(long long)sample.num_minor_u8_padded(), (long long)1},
                true));
    }
};

PYBIND11_MODULE(stim, m) {
    m.doc() = R"pbdoc(
        Stim: A stabilizer circuit simulator.
    )pbdoc";

    m.attr("__version__") = STRINGIFY(VERSION_INFO);

    pybind11::class_<CompiledCircuitSampler>(m, "CompiledCircuitSampler", "An analyzed stabilizer circuit that can be sampled quickly.")
        .def("sample", &CompiledCircuitSampler::sample);

    pybind11::class_<Circuit>(m, "Circuit", "A mutable stabilizer circuit.")
        .def(pybind11::init())
        .def_readonly("num_measurements", &Circuit::num_measurements, R"DOC(
            The number of measurement bits produced when sampling from the circuit.
         )DOC")
        .def_readonly("num_qubits", &Circuit::num_qubits, R"DOC(
            The number of qubits used when simulating the circuit.
         )DOC")
        .def("compile", [](Circuit &self) {
            return CompiledCircuitSampler(self);
        }, R"DOC(
            Returns a CompiledCircuitSampler for the circuit.
         )DOC")
        .def("__iadd__", &Circuit::operator+=, R"DOC(
            Appends a circuit into the receiving circuit (mutating it).
         )DOC")
        .def("__imul__", &Circuit::operator*=, R"DOC(
            Mutates the circuit into multiple copies of itself.
         )DOC")
        .def("__add__", &Circuit::operator+, R"DOC(
            Creates a circuit by appending two circuits.
         )DOC")
        .def("__mul__", &Circuit::operator*, R"DOC(
            Creates a circuit by repeating a circuit multiple times.
         )DOC")
        .def("__rmul__", &Circuit::operator*, R"DOC(
            Creates a circuit by repeating a circuit multiple times.
         )DOC")
        .def("append_operation", &Circuit::append_op, R"DOC(
            Appends an operation into the circuit.

            Args:
                name: The name of the operation's gate (e.g. "H" or "M" or "CNOT").
                targets: The gate targets. Gates implicitly broadcast over their targets.
                arg: A modifier for the gate, e.g. the probability of an error. Defaults to 0.
                fuse: Defaults to true. If the gate being added is compatible with the last gate in the circuit, the
                    new targets will be appended to the last gate instead of adding a new one. This is particularly
                    important for measurement operations, because batched measurements are significantly more efficient
                    in some cases. Set to false if you don't want this to occur.
         )DOC", pybind11::arg("name"), pybind11::arg("targets"), pybind11::arg("arg") = 0.0, pybind11::arg("fuse") = true)
        .def("__str__", &Circuit::str);

    pybind11::class_<TableauSimulator>(m, "TableauSimulator", "A quantum stabilizer circuit simulator whose state is a stabilizer tableau.")
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
