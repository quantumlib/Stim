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

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "../probability_util.h"
#include "../simulators/detection_simulator.h"
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
    // Temporarily remove const correctness but then immediately restore it.
    VectorView<uint32_t> v{(std::vector<uint32_t> *)&targets, 0, targets.size()};
    return {0, v};
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

uint32_t target_rec(int32_t lookback) {
    if (lookback >= 0 || lookback <= -(1 << 24)) {
        throw std::out_of_range("Need -16777215 <= lookback <= -1");
    }
    return uint32_t(-lookback) | TARGET_RECORD_BIT;
}

uint32_t target_inv(uint32_t qubit) {
    return qubit | TARGET_INVERTED_BIT;
}

uint32_t target_x(uint32_t qubit) {
    return qubit | TARGET_PAULI_X_BIT;
}

uint32_t target_y(uint32_t qubit) {
    return qubit | TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT;
}

uint32_t target_z(uint32_t qubit) {
    return qubit | TARGET_PAULI_Z_BIT;
}

struct CompiledMeasurementSampler {
    const simd_bits ref;
    const Circuit circuit;

    CompiledMeasurementSampler(Circuit circuit)
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

struct CompiledDetectorSampler {
    const DetectorsAndObservables dets_obs;
    const Circuit circuit;

    CompiledDetectorSampler(Circuit circuit) : dets_obs(circuit), circuit(std::move(circuit)) {
    }

    pybind11::array_t<uint8_t> sample(size_t num_shots, bool prepend_observables, bool append_observables) {
        auto sample =
            detector_samples(circuit, dets_obs, num_shots, prepend_observables, append_observables, SHARED_RNG())
                .transposed();

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

        size_t n = dets_obs.detectors.size() + dets_obs.observables.size() * (prepend_observables + append_observables);
        return pybind11::array_t<uint8_t>(pybind11::buffer_info(
            bytes.data(), sizeof(uint8_t), pybind11::format_descriptor<uint8_t>::value, 2, {num_shots, n},
            {(long long)sample.num_minor_bits_padded(), (long long)1}, true));
    }

    pybind11::array_t<uint8_t> sample_bit_packed(size_t num_shots, bool prepend_observables, bool append_observables) {
        auto sample =
            detector_samples(circuit, dets_obs, num_shots, prepend_observables, append_observables, SHARED_RNG())
                .transposed();
        size_t n = dets_obs.detectors.size() + dets_obs.observables.size() * (prepend_observables + append_observables);
        return pybind11::array_t<uint8_t>(pybind11::buffer_info(
            sample.data.u8, sizeof(uint8_t), pybind11::format_descriptor<uint8_t>::value, 2, {num_shots, (n + 7) / 8},
            {(long long)sample.num_minor_u8_padded(), (long long)1}, true));
    }

    std::string str() const {
        std::stringstream result;
        result << "# num_detectors: " << dets_obs.detectors.size() << "\n";
        result << "# num_observables: " << dets_obs.observables.size() << "\n";
        result << circuit;
        return result.str();
    }
};

PYBIND11_MODULE(stim, m) {
    m.doc() = R"pbdoc(
        Stim: A stabilizer circuit simulator.
    )pbdoc";

    m.attr("__version__") = STRINGIFY(VERSION_INFO);

    m.def(
        "target_rec", &target_rec, R"DOC(
        Returns a record target that can be passed into Circuit.append_operation.
        For example, the 'rec[-2]' in 'DETECTOR rec[-2]' is a record target.
    )DOC",
        pybind11::arg("lookback_index"));
    m.def(
        "target_inv", &target_inv, R"DOC(
        Returns a target flagged as inverted that can be passed into Circuit.append_operation
        For example, the '!1' in 'M 0 !1 2' is qubit 1 flagged as inverted,
        meaning the measurement result from qubit 1 should be inverted when reported.
    )DOC",
        pybind11::arg("qubit_index"));
    m.def(
        "target_x", &target_x, R"DOC(
        Returns a target flagged as Pauli X that can be passed into Circuit.append_operation
        For example, the 'X1' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 1 flagged as Pauli X.
    )DOC",
        pybind11::arg("qubit_index"));
    m.def(
        "target_y", &target_y, R"DOC(
        Returns a target flagged as Pauli Y that can be passed into Circuit.append_operation
        For example, the 'Y2' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 2 flagged as Pauli Y.
    )DOC",
        pybind11::arg("qubit_index"));
    m.def(
        "target_z", &target_z, R"DOC(
        Returns a target flagged as Pauli Z that can be passed into Circuit.append_operation
        For example, the 'Z3' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 3 flagged as Pauli Z.
    )DOC",
        pybind11::arg("qubit_index"));

    pybind11::class_<CompiledMeasurementSampler>(
        m, "CompiledMeasurementSampler", "An analyzed stabilizer circuit whose measurements can be sampled quickly.")
        .def(pybind11::init<Circuit>())
        .def(
            "sample", &CompiledMeasurementSampler::sample, R"DOC(
            Returns a numpy array containing a batch of measurement samples from the circuit.

            Args:
                shots: The number of times to sample every measurement in the circuit.

            Returns:
                A numpy array with `dtype=uint8` and `shape=(shots, num_measurements)`.
                The bit for measurement `m` in shot `s` is at `result[s, m]`.
        )DOC",
            pybind11::arg("shots"))
        .def(
            "sample_bit_packed", &CompiledMeasurementSampler::sample_bit_packed, R"DOC(
            Returns a numpy array containing a bit packed batch of measurement samples from the circuit.

            Args:
                shots: The number of times to sample every measurement in the circuit.

            Returns:
                A numpy array with `dtype=uint8` and `shape=(shots, (num_measurements + 7) // 8)`.
                The bit for measurement `m` in shot `s` is at `result[s, (m // 8)] & 2**(m % 8)`.
        )DOC",
            pybind11::arg("shots"))
        .def("__str__", &CompiledMeasurementSampler::str);

    pybind11::class_<CompiledDetectorSampler>(
        m, "CompiledDetectorSampler", "An analyzed stabilizer circuit whose detection events can be sampled quickly.")
        .def(pybind11::init<Circuit>())
        .def(
            "sample", &CompiledDetectorSampler::sample, R"DOC(
            Returns a numpy array containing a batch of detector samples from the circuit.

            The circuit must define the detectors using DETECTOR instructions. Observables defined by OBSERVABLE_INCLUDE
            instructions can also be included in the results as honorary detectors.

            Args:
                shots: The number of times to sample every detector in the circuit.
                prepend_observables: Defaults to false. When set, observables are included with the detectors and are
                    placed at the start of the results.
                prepend_observables: Defaults to false. When set, observables are included with the detectors and are
                    placed at the end of the results.

            Returns:
                A numpy array with `dtype=uint8` and `shape=(shots, n)` where
                `n = num_detectors + num_observables*(append_observables + prepend_observables)`.
                The bit for detection event `m` in shot `s` is at `result[s, m]`.
            )DOC",
            pybind11::arg("shots"), pybind11::kw_only(), pybind11::arg("prepend_observables") = false,
            pybind11::arg("append_observables") = false)
        .def(
            "sample_bit_packed", &CompiledDetectorSampler::sample_bit_packed, R"DOC(
            Returns a numpy array containing bit packed batch of detector samples from the circuit.

            The circuit must define the detectors using DETECTOR instructions. Observables defined by OBSERVABLE_INCLUDE
            instructions can also be included in the results as honorary detectors.

            Args:
                shots: The number of times to sample every detector in the circuit.
                prepend_observables: Defaults to false. When set, observables are included with the detectors and are
                    placed at the start of the results.
                prepend_observables: Defaults to false. When set, observables are included with the detectors and are
                    placed at the end of the results.

            Returns:
                A numpy array with `dtype=uint8` and `shape=(shots, n)` where
                `n = num_detectors + num_observables*(append_observables + prepend_observables)`.
                The bit for detection event `m` in shot `s` is at `result[s, (m // 8)] & 2**(m % 8)`.
            )DOC",
            pybind11::arg("shots"), pybind11::kw_only(), pybind11::arg("prepend_observables") = false,
            pybind11::arg("append_observables") = false)
        .def("__str__", &CompiledDetectorSampler::str);

    pybind11::class_<Circuit>(m, "Circuit", "A mutable stabilizer circuit.")
        .def(pybind11::init())
        .def_readonly("num_measurements", &Circuit::num_measurements, R"DOC(
            The number of measurement bits produced when sampling from the circuit.
         )DOC")
        .def_readonly("num_qubits", &Circuit::num_qubits, R"DOC(
            The number of qubits used when simulating the circuit.
         )DOC")
        .def(
            "compile_sampler",
            [](Circuit &self) {
                return CompiledMeasurementSampler(self);
            },
            R"DOC(
            Returns a CompiledMeasurementSampler, which can quickly batch sample measurements, for the circuit.
         )DOC")
        .def(
            "compile_detector_sampler",
            [](Circuit &self) {
                return CompiledDetectorSampler(self);
            },
            R"DOC(
            Returns a CompiledDetectorSampler, which can quickly batch sample detection events, for the circuit.
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
        .def(
            "append_from_stim_program_text", &Circuit::append_from_text, R"DOC(
            Appends operations described by a STIM format program into the circuit.

            Example STIM program:

                H 0  # comment
                CNOT 0 2
                M 2
                CNOT rec[-1] 1

            Args:
                text: The STIM program text containing the circuit operations to append.
         )DOC",
            pybind11::arg("stim_program_text"))
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
