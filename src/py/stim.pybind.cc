#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "../probability_util.h"
#include "../simulators/frame_simulator.h"
#include "../simulators/tableau_simulator.h"
#include "../stabilizers/tableau.h"

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

uint32_t target_rec(uint32_t qubit, int16_t lookback) {
    if (lookback >= 0 || lookback < -256) {
        throw std::out_of_range("Need -256 <= lookback <= -1");
    }
    return qubit | (uint32_t(-lookback) << TARGET_RECORD_SHIFT);
}

uint32_t target_inv(uint32_t qubit) {
    return qubit | TARGET_INVERTED_MASK;
}

uint32_t target_x(uint32_t qubit) {
    return qubit | TARGET_PAULI_X_MASK;
}

uint32_t target_y(uint32_t qubit) {
    return qubit | TARGET_PAULI_X_MASK | TARGET_PAULI_Z_MASK;
}

uint32_t target_z(uint32_t qubit) {
    return qubit | TARGET_PAULI_Z_MASK;
}

struct CompiledCircuitSampler {
    const simd_bits ref;
    const Circuit circuit;

    CompiledCircuitSampler(Circuit circuit)
        : ref(TableauSimulator::reference_sample_circuit(circuit)), circuit(std::move(circuit)) {
    }

    pybind11::array_t<uint8_t> sample(size_t num_samples) {
        auto sample = FrameSimulator::sample(circuit, ref, num_samples, SHARED_RNG());

        const simd_bits &flat = sample.data;
        std::vector<uint8_t> bytes;
        bytes.reserve(flat.num_bits_padded());
        auto *end = flat.u64 + flat.num_u64_padded();
        for (auto u64 = flat.u64; u64 != end; u64++) {
            auto v = *u64;
            for (size_t k = 0; k < 64; k++) {
                bytes.push_back((v >> k) & 1);
            }
        }

        return pybind11::array_t<uint8_t>(pybind11::buffer_info(
            bytes.data(), sizeof(uint8_t), pybind11::format_descriptor<uint8_t>::value, 2,
            {num_samples, circuit.num_measurements}, {(long long)sample.num_minor_bits_padded(), (long long)1}, true));
    }

    pybind11::array_t<uint8_t> sample_bit_packed(size_t num_samples) {
        auto sample = FrameSimulator::sample(circuit, ref, num_samples, SHARED_RNG());
        return pybind11::array_t<uint8_t>(pybind11::buffer_info(
            sample.data.u8, sizeof(uint8_t), pybind11::format_descriptor<uint8_t>::value, 2,
            {num_samples, (circuit.num_measurements + 7) / 8}, {(long long)sample.num_minor_u8_padded(), (long long)1},
            true));
    }

    std::string str() const {
        std::stringstream result;
        result << "# reference sample: ";
        for (size_t k = 0; k < circuit.num_measurements; k++) {
            result << "01"[ref[k]];
        }
        result << "\n";
        result << circuit;
        return result.str();
    }
};

PYBIND11_MODULE(stim, m) {
    m.doc() = R"pbdoc(
        Stim: A stabilizer circuit simulator.
    )pbdoc";

    m.attr("__version__") = STRINGIFY(VERSION_INFO);

    m.def("target_rec", &target_rec, R"DOC(
        Returns a record target that can be passed into Circuit.append_operation.
        For example, the '1@-2' in 'DETECTOR 1@-2' is a record target.
    )DOC");
    m.def("target_inv", &target_inv, R"DOC(
        Returns a target flagged as inverted that can be passed into Circuit.append_operation
        For example, the '!1' in 'M 0 !1 2' is qubit 1 flagged as inverted,
        meaning the measurement result from qubit 1 should be inverted when reported.
    )DOC");
    m.def("target_x", &target_x, R"DOC(
        Returns a target flagged as Pauli X that can be passed into Circuit.append_operation
        For example, the 'X1' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 1 flagged as Pauli X.
    )DOC");
    m.def("target_y", &target_y, R"DOC(
        Returns a target flagged as Pauli Y that can be passed into Circuit.append_operation
        For example, the 'Y2' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 2 flagged as Pauli Y.
    )DOC");
    m.def("target_z", &target_z, R"DOC(
        Returns a target flagged as Pauli Z that can be passed into Circuit.append_operation
        For example, the 'Z3' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 3 flagged as Pauli Z.
    )DOC");

    pybind11::class_<CompiledCircuitSampler>(
        m, "CompiledCircuitSampler", "An analyzed stabilizer circuit that can be sampled quickly.")
        .def(pybind11::init<Circuit>())
        .def("sample", &CompiledCircuitSampler::sample, R"DOC(
            Returns a batch of samples from the circuit as a numpy array with dtype uint8
            and shape (num_samples, num_measurements).
            The measurement result for measurement m in sample s is at result[s, m].
        )DOC")
        .def("sample_bit_packed", &CompiledCircuitSampler::sample_bit_packed, R"DOC(
            Returns a bit packed batch of samples from the circuit as a numpy array
            with dtype uint8 and shape (num_samples, (num_measurements + 7) // 8).
            The measurement result for measurement m in sample s is at result[s, (m // 8)] & 2**(m % 8).
        )DOC")
        .def("__str__", &CompiledCircuitSampler::str);

    pybind11::class_<Circuit>(m, "Circuit", "A mutable stabilizer circuit.")
        .def(pybind11::init())
        .def_readonly("num_measurements", &Circuit::num_measurements, R"DOC(
            The number of measurement bits produced when sampling from the circuit.
         )DOC")
        .def_readonly("num_qubits", &Circuit::num_qubits, R"DOC(
            The number of qubits used when simulating the circuit.
         )DOC")
        .def(
            "compile",
            [](Circuit &self) {
                return CompiledCircuitSampler(self);
            },
            R"DOC(
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
        .def(
            "append_operation", &Circuit::append_op, R"DOC(
            Appends an operation into the circuit.

            Args:
                name: The name of the operation's gate (e.g. "H" or "M" or "CNOT").
                targets: The gate targets. Gates implicitly broadcast over their targets.
                arg: A modifier for the gate, e.g. the probability of an error. Defaults to 0.
                fuse: Defaults to true. If the gate being added is compatible with the last gate in the circuit, the
                    new targets will be appended to the last gate instead of adding a new one. This is particularly
                    important for measurement operations, because batched measurements are significantly more efficient
                    in some cases. Set to false if you don't want this to occur.
         )DOC",
            pybind11::arg("name"), pybind11::arg("targets"), pybind11::arg("arg") = 0.0, pybind11::arg("fuse") = true)
        .def("__str__", &Circuit::str);

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
                self.ZCX(args_to_targets2(self, args));
            })
        .def(
            "cz",
            [](TableauSimulator &self, pybind11::args args) {
                self.ZCZ(args_to_targets2(self, args));
            })
        .def(
            "cy",
            [](TableauSimulator &self, pybind11::args args) {
                self.ZCY(args_to_targets2(self, args));
            })
        .def(
            "reset",
            [](TableauSimulator &self, pybind11::args args) {
                self.reset(args_to_targets(self, args));
            })
        .def(
            "measure",
            [](TableauSimulator &self, uint32_t target) {
                self.measure(Dat({target}));
                return self.measurement_record.back();
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
                return std::vector<bool>(e - converted_args.size(), e);
            })
        .def(pybind11::init(&create_tableau_simulator));
}
