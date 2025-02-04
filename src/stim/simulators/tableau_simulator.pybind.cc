#include "stim/simulators/tableau_simulator.pybind.h"

#include "stim/circuit/circuit_instruction.pybind.h"
#include "stim/circuit/circuit_repeat_block.pybind.h"
#include "stim/py/base.pybind.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/stabilizers/pauli_string.pybind.h"
#include "stim/stabilizers/tableau.h"
#include "stim/util_bot/str_util.h"
#include "stim/util_top/circuit_vs_amplitudes.h"
#include "stim/util_top/stabilizers_to_tableau.h"

using namespace stim;
using namespace stim_pybind;

template <size_t W>
void do_obj(TableauSimulator<W> &self, const pybind11::object &obj) {
    if (pybind11::isinstance<Circuit>(obj)) {
        self.safe_do_circuit(pybind11::cast<Circuit>(obj));
    } else if (pybind11::isinstance<CircuitRepeatBlock>(obj)) {
        const CircuitRepeatBlock &block = pybind11::cast<CircuitRepeatBlock>(obj);
        self.safe_do_circuit(block.body, block.repeat_count);
    } else if (pybind11::isinstance<FlexPauliString>(obj)) {
        const FlexPauliString &pauli_string = pybind11::cast<FlexPauliString>(obj);
        self.ensure_large_enough_for_qubits(pauli_string.value.num_qubits);
        self.paulis(pauli_string.value);
    } else if (pybind11::isinstance<PyCircuitInstruction>(obj)) {
        const PyCircuitInstruction &circuit_instruction = pybind11::cast<PyCircuitInstruction>(obj);
        self.do_operation_ensure_size(circuit_instruction);
    } else {
        std::stringstream ss;
        ss << "Don't know how to handle ";
        ss << pybind11::repr(obj);
        throw std::invalid_argument(ss.str());
    }
}

template <size_t W>
TableauSimulator<W> create_tableau_simulator(const pybind11::object &seed) {
    return TableauSimulator<W>(make_py_seeded_rng(seed));
}

template <size_t W>
std::vector<GateTarget> arg_to_qubit_or_qubits(TableauSimulator<W> &self, const pybind11::object &obj) {
    std::vector<GateTarget> arguments;
    uint32_t max_q = 0;
    try {
        try {
            uint32_t q = pybind11::cast<uint32_t>(obj);
            arguments.push_back(GateTarget::qubit(q));
            max_q = q;
        } catch (const pybind11::cast_error &) {
            for (const auto &e : obj) {
                uint32_t q = e.cast<uint32_t>();
                max_q = std::max(max_q, q);
                arguments.push_back(GateTarget::qubit(q));
            }
        }
    } catch (const pybind11::cast_error &) {
        throw std::out_of_range("'targets' must be a non-negative integer or iterable of non-negative integers.");
    }

    // Note: quadratic behavior.
    self.ensure_large_enough_for_qubits(max_q + 1);

    return arguments;
}

template <size_t W>
PyCircuitInstruction build_single_qubit_gate_instruction_ensure_size(
    TableauSimulator<W> &self, GateType gate_type, const pybind11::args &args, SpanRef<const double> gate_args = {}) {
    std::vector<GateTarget> targets;
    uint32_t max_q = 0;
    try {
        for (const auto &e : args) {
            if (pybind11::isinstance<GateTarget>(e)) {
                targets.push_back(pybind11::cast<GateTarget>(e));
            } else {
                uint32_t q = e.cast<uint32_t>();
                max_q = std::max(max_q, q & TARGET_VALUE_MASK);
                targets.push_back(GateTarget{q});
            }
        }
    } catch (const pybind11::cast_error &) {
        throw std::out_of_range("Target qubits must be non-negative integers.");
    }

    std::vector<double> gate_args_vec;
    for (const auto &e : gate_args) {
        gate_args_vec.push_back(e);
    }

    // Note: quadratic behavior.
    self.ensure_large_enough_for_qubits(max_q + 1);

    return PyCircuitInstruction(gate_type, targets, gate_args_vec, "");
}

template <size_t W>
PyCircuitInstruction build_two_qubit_gate_instruction_ensure_size(
    TableauSimulator<W> &self, GateType gate_type, const pybind11::args &args, SpanRef<const double> gate_args = {}) {
    if (pybind11::len(args) & 1) {
        throw std::invalid_argument("Two qubit operation requires an even number of targets.");
    }
    auto result = build_single_qubit_gate_instruction_ensure_size<W>(self, gate_type, args, gate_args);
    for (size_t k = 0; k < result.targets.size(); k += 2) {
        if (result.targets[k] == result.targets[k + 1]) {
            throw std::invalid_argument("Two qubit operation can't target the same qubit twice.");
        }
    }
    return result;
}

pybind11::class_<TableauSimulator<MAX_BITWORD_WIDTH>> stim_pybind::pybind_tableau_simulator(pybind11::module &m) {
    return pybind11::class_<TableauSimulator<MAX_BITWORD_WIDTH>>(
        m,
        "TableauSimulator",
        clean_doc_string(R"DOC(
            A stabilizer circuit simulator that tracks an inverse stabilizer tableau.

            Supports interactive usage, where gates and measurements are applied on demand.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.h(0)
                >>> if s.measure(0):
                ...     s.h(1)
                ...     s.cnot(1, 2)
                >>> s.measure(1) == s.measure(2)
                True

                >>> s = stim.TableauSimulator()
                >>> s.h(0)
                >>> s.cnot(0, 1)
                >>> s.current_inverse_tableau()
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+ZX"),
                        stim.PauliString("+_X"),
                    ],
                    zs=[
                        stim.PauliString("+X_"),
                        stim.PauliString("+XZ"),
                    ],
                )
        )DOC")
            .data());
}

void stim_pybind::pybind_tableau_simulator_methods(
    pybind11::module &m, pybind11::class_<TableauSimulator<MAX_BITWORD_WIDTH>> &c) {
    c.def(
        pybind11::init(&create_tableau_simulator<MAX_BITWORD_WIDTH>),
        pybind11::kw_only(),
        pybind11::arg("seed") = pybind11::none(),
        clean_doc_string(R"DOC(
            @signature def __init__(self, *, seed: Optional[int] = None) -> None:
            Initializes a stim.TableauSimulator.

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

                    CAUTION: simulation results *MAY NOT* be consistent if you vary how the
                    circuit is executed. For example, reordering whether a reset on one
                    qubit happens before or after a reset on another qubit can result in
                    different measurement results being observed starting from the same
                    seed.

            Returns:
                An initialized stim.TableauSimulator.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator(seed=0)
                >>> s2 = stim.TableauSimulator(seed=0)
                >>> s.h(0)
                >>> s2.h(0)
                >>> s.measure(0) == s2.measure(0)
                True
        )DOC")
            .data());

    c.def(
        "current_inverse_tableau",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self) {
            return self.inv_state;
        },
        clean_doc_string(R"DOC(
            Returns a copy of the internal state of the simulator as a stim.Tableau.

            Returns:
                A stim.Tableau copy of the simulator's state.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.h(0)
                >>> s.current_inverse_tableau()
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+Z"),
                    ],
                    zs=[
                        stim.PauliString("+X"),
                    ],
                )
                >>> s.cnot(0, 1)
                >>> s.current_inverse_tableau()
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+ZX"),
                        stim.PauliString("+_X"),
                    ],
                    zs=[
                        stim.PauliString("+X_"),
                        stim.PauliString("+XZ"),
                    ],
                )
        )DOC")
            .data());

    c.def(
        "state_vector",
        [](const TableauSimulator<MAX_BITWORD_WIDTH> &self, std::string_view endian) {
            bool little_endian;
            if (endian == "little") {
                little_endian = true;
            } else if (endian == "big") {
                little_endian = false;
            } else {
                throw std::invalid_argument("endian not in ['little', 'big']");
            }
            auto complex_vec = self.to_state_vector(little_endian);

            std::complex<float> *buffer = new std::complex<float>[complex_vec.size()];
            for (size_t k = 0; k < complex_vec.size(); k++) {
                buffer[k] = complex_vec[k];
            }

            pybind11::capsule free_when_done(buffer, [](void *f) {
                delete[] reinterpret_cast<std::complex<float> *>(f);
            });

            return pybind11::array_t<std::complex<float>>(
                {(pybind11::ssize_t)complex_vec.size()},
                {(pybind11::ssize_t)sizeof(std::complex<float>)},
                buffer,
                free_when_done);
        },
        pybind11::kw_only(),
        pybind11::arg("endian") = "little",
        clean_doc_string(R"DOC(
            @signature def state_vector(self, *, endian: str = 'little') -> np.ndarray[np.complex64]:
            Returns a wavefunction for the simulator's current state.

            This function takes O(n * 2**n) time and O(2**n) space, where n is the number of
            qubits. The computation is done by initialization a random state vector and
            iteratively projecting it into the +1 eigenspace of each stabilizer of the
            state. The state is then canonicalized so that zero values are actually exactly
            0, and so that the first non-zero entry is positive.

            Args:
                endian:
                    "little" (default): state vector is in little endian order, where higher
                        index qubits correspond to larger changes in the state index.
                    "big": state vector is in big endian order, where higher index qubits
                        correspond to smaller changes in the state index.

            Returns:
                A `numpy.ndarray[numpy.complex64]` of computational basis amplitudes.

                If the result is in little endian order then the amplitude at offset
                b_0 + b_1*2 + b_2*4 + ... + b_{n-1}*2^{n-1} is the amplitude for the
                computational basis state where the qubit with index 0 is storing the bit
                b_0, the qubit with index 1 is storing the bit b_1, etc.

                If the result is in big endian order then the amplitude at offset
                b_0 + b_1*2 + b_2*4 + ... + b_{n-1}*2^{n-1} is the amplitude for the
                computational basis state where the qubit with index 0 is storing the bit
                b_{n-1}, the qubit with index 1 is storing the bit b_{n-2}, etc.

            Examples:
                >>> import stim
                >>> import numpy as np
                >>> s = stim.TableauSimulator()
                >>> s.x(2)
                >>> s.state_vector(endian='little')
                array([0.+0.j, 0.+0.j, 0.+0.j, 0.+0.j, 1.+0.j, 0.+0.j, 0.+0.j, 0.+0.j],
                      dtype=complex64)

                >>> s.state_vector(endian='big')
                array([0.+0.j, 1.+0.j, 0.+0.j, 0.+0.j, 0.+0.j, 0.+0.j, 0.+0.j, 0.+0.j],
                      dtype=complex64)

                >>> s.sqrt_x(1, 2)
                >>> s.state_vector()
                array([0.5+0.j , 0. +0.j , 0. -0.5j, 0. +0.j , 0. +0.5j, 0. +0.j ,
                       0.5+0.j , 0. +0.j ], dtype=complex64)
        )DOC")
            .data());

    c.def(
        "canonical_stabilizers",
        [](const TableauSimulator<MAX_BITWORD_WIDTH> &self) {
            auto stabilizers = self.canonical_stabilizers();
            std::vector<FlexPauliString> result;
            result.reserve(stabilizers.size());
            for (auto &s : stabilizers) {
                result.emplace_back(std::move(s), false);
            }
            return result;
        },
        clean_doc_string(R"DOC(
            Returns a standardized list of the simulator's current stabilizer generators.

            Two simulators have the same canonical stabilizers if and only if their current
            quantum state is equal (and tracking the same number of qubits).

            The canonical form is computed as follows:

                1. Get a list of stabilizers using the `z_output`s of
                    `simulator.current_inverse_tableau()**-1`.
                2. Perform Gaussian elimination on each generator g.
                    2a) The generators are considered in order X0, Z0, X1, Z1, X2, Z2, etc.
                    2b) Pick any stabilizer that uses the generator g. If there are none,
                        go to the next g.
                    2c) Multiply that stabilizer into all other stabilizers that use the
                        generator g.
                    2d) Swap that stabilizer with the stabilizer at position `next_output`
                        then increment `next_output`.

            Returns:
                A List[stim.PauliString] of the simulator's state's stabilizers.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.h(0)
                >>> s.cnot(0, 1)
                >>> s.x(2)
                >>> for e in s.canonical_stabilizers():
                ...     print(repr(e))
                stim.PauliString("+XX_")
                stim.PauliString("+ZZ_")
                stim.PauliString("-__Z")

                >>> # Scramble the stabilizers then check the canonical form is unchanged.
                >>> s.set_inverse_tableau(s.current_inverse_tableau()**-1)
                >>> s.cnot(0, 1)
                >>> s.cz(0, 2)
                >>> s.s(0, 2)
                >>> s.cy(2, 1)
                >>> s.set_inverse_tableau(s.current_inverse_tableau()**-1)
                >>> for e in s.canonical_stabilizers():
                ...     print(repr(e))
                stim.PauliString("+XX_")
                stim.PauliString("+ZZ_")
                stim.PauliString("-__Z")
        )DOC")
            .data());

    c.def(
        "current_measurement_record",
        [](const TableauSimulator<MAX_BITWORD_WIDTH> &self) {
            return self.measurement_record.storage;
        },
        clean_doc_string(R"DOC(
            Returns a copy of the record of all measurements performed by the simulator.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.current_measurement_record()
                []
                >>> s.measure(0)
                False
                >>> s.x(0)
                >>> s.measure(0)
                True
                >>> s.current_measurement_record()
                [False, True]
                >>> s.do(stim.Circuit("M 0"))
                >>> s.current_measurement_record()
                [False, True, True]

            Returns:
                A list of booleans containing the result of every measurement performed by
                the simulator so far.
        )DOC")
            .data());

    c.def(
        "do",
        &do_obj<MAX_BITWORD_WIDTH>,
        pybind11::arg("circuit_or_pauli_string"),
        clean_doc_string(R"DOC(
            Applies a circuit or pauli string to the simulator's state.
            @signature def do(self, circuit_or_pauli_string: Union[stim.Circuit, stim.PauliString, stim.CircuitInstruction, stim.CircuitRepeatBlock]) -> None:

            Args:
                circuit_or_pauli_string: A stim.Circuit, stim.PauliString,
                    stim.CircuitInstruction, or stim.CircuitRepeatBlock
                    with operations to apply to the simulator's state.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.do(stim.Circuit('''
                ...     X 0
                ...     M 0
                ... '''))
                >>> s.current_measurement_record()
                [True]

                >>> s = stim.TableauSimulator()
                >>> s.do(stim.PauliString("IXYZ"))
                >>> s.measure_many(0, 1, 2, 3)
                [False, True, True, False]
        )DOC")
            .data());

    c.def(
        "do_pauli_string",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, FlexPauliString &pauli_string) {
            self.ensure_large_enough_for_qubits(pauli_string.value.num_qubits);
            self.paulis(pauli_string.value);
        },
        pybind11::arg("pauli_string"),
        clean_doc_string(R"DOC(
            Applies the paulis from a pauli string to the simulator's state.

            Args:
                pauli_string: A stim.PauliString containing Paulis to apply.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.do_pauli_string(stim.PauliString("IXYZ"))
                >>> s.measure_many(0, 1, 2, 3)
                [False, True, True, False]
        )DOC")
            .data());

    c.def(
        "do_circuit",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const Circuit &circuit) {
            self.safe_do_circuit(circuit);
        },
        pybind11::arg("circuit"),
        clean_doc_string(R"DOC(
            Applies a circuit to the simulator's state.

            Args:
                circuit: A stim.Circuit containing operations to apply.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.do_circuit(stim.Circuit('''
                ...     X 0
                ...     M 0
                ... '''))
                >>> s.current_measurement_record()
                [True]
        )DOC")
            .data());

    c.def(
        "do_tableau",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self,
           const Tableau<MAX_BITWORD_WIDTH> &tableau,
           const std::vector<size_t> &targets) {
            if (targets.size() != tableau.num_qubits) {
                throw std::invalid_argument("len(tableau) != len(targets)");
            }
            size_t max_target = 0;
            for (size_t i = 0; i < targets.size(); i++) {
                max_target = std::max(max_target, targets[i]);
                for (size_t j = i + 1; j < targets.size(); j++) {
                    if (targets[i] == targets[j]) {
                        std::stringstream ss;
                        ss << "targets contains duplicates: ";
                        ss << comma_sep(targets);
                        throw std::invalid_argument(ss.str());
                    }
                }
            }
            self.ensure_large_enough_for_qubits(max_target + 1);
            self.apply_tableau(tableau, targets);
        },
        pybind11::arg("tableau"),
        pybind11::arg("targets"),
        clean_doc_string(R"DOC(
            Applies a custom tableau operation to qubits in the simulator.

            Note that this method has to compute the inverse of the tableau, because the
            simulator's internal state is an inverse tableau.

            Args:
                tableau: A stim.Tableau representing the Clifford operation to apply.
                targets: The indices of the qubits to operate on.

            Examples:
                >>> import stim
                >>> sim = stim.TableauSimulator()
                >>> sim.h(1)
                >>> sim.h_yz(2)
                >>> [str(sim.peek_bloch(k)) for k in range(4)]
                ['+Z', '+X', '+Y', '+Z']
                >>> rot3 = stim.Tableau.from_conjugated_generators(
                ...     xs=[
                ...         stim.PauliString("_X_"),
                ...         stim.PauliString("__X"),
                ...         stim.PauliString("X__"),
                ...     ],
                ...     zs=[
                ...         stim.PauliString("_Z_"),
                ...         stim.PauliString("__Z"),
                ...         stim.PauliString("Z__"),
                ...     ],
                ... )

                >>> sim.do_tableau(rot3, [1, 2, 3])
                >>> [str(sim.peek_bloch(k)) for k in range(4)]
                ['+Z', '+Z', '+X', '+Y']

                >>> sim.do_tableau(rot3, [1, 2, 3])
                >>> [str(sim.peek_bloch(k)) for k in range(4)]
                ['+Z', '+Y', '+Z', '+X']
        )DOC")
            .data());

    c.def(
        "h",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_H_XZ(build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::H, args));
        },
        clean_doc_string(R"DOC(
            Applies a Hadamard gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +X +Y +Z
                >>> s.h(0, 1, 2)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +Z -Y +X
        )DOC")
            .data());

    c.def(
        "depolarize1",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args, const pybind11::kwargs &kwargs) {
            double p = pybind11::cast<double>(kwargs["p"]);
            if (kwargs.size() != 1) {
                throw std::invalid_argument("Unexpected argument. Expected position-only targets and p=probability.");
            }
            self.do_DEPOLARIZE1(
                build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(
                    self, GateType::DEPOLARIZE1, args, &p));
        },
        clean_doc_string(R"DOC(
            @signature def depolarize1(self, *targets: int, p: float):
            Probabilistically applies single-qubit depolarization to targets.

            Args:
                *targets: The indices of the qubits to target with the noise.
                p: The chance of the error being applied,
                    independently, to each qubit.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.depolarize1(0, 1, 2, p=0.01)
        )DOC")
            .data());

    c.def(
        "depolarize2",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args, const pybind11::kwargs &kwargs) {
            double p = pybind11::cast<double>(kwargs["p"]);
            if (kwargs.size() != 1) {
                throw std::invalid_argument("Unexpected argument. Expected position-only targets and p=probability.");
            }
            self.do_DEPOLARIZE2(
                build_two_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::DEPOLARIZE2, args, &p));
        },
        clean_doc_string(R"DOC(
            @signature def depolarize2(self, *targets: int, p: float):
            Probabilistically applies two-qubit depolarization to targets.

            Args:
                *targets: The indices of the qubits to target with the noise.
                    The pairs of qubits are formed by
                    zip(targets[::1], targets[1::2]).
                p: The chance of the error being applied,
                    independently, to each qubit pair.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.depolarize1(0, 1, 4, 5, p=0.01)
        )DOC")
            .data());

    c.def(
        "x_error",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args, const pybind11::kwargs &kwargs) {
            double p = pybind11::cast<double>(kwargs["p"]);
            if (kwargs.size() != 1) {
                throw std::invalid_argument("Unexpected argument. Expected position-only targets and p=probability.");
            }
            self.do_X_ERROR(
                build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(
                    self, GateType::X_ERROR, args, {&p}));
        },
        clean_doc_string(R"DOC(
            @signature def x_error(self, *targets: int, p: float):
            Probabilistically applies X errors to targets.

            Args:
                *targets: The indices of the qubits to target with the noise.
                p: The chance of the X error being applied,
                    independently, to each qubit.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.x_error(0, 1, 2, p=0.01)
        )DOC")
            .data());

    c.def(
        "y_error",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args, const pybind11::kwargs &kwargs) {
            double p = pybind11::cast<double>(kwargs["p"]);
            if (kwargs.size() != 1) {
                throw std::invalid_argument("Unexpected argument. Expected position-only targets and p=probability.");
            }
            self.do_Y_ERROR(
                build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::Y_ERROR, args, &p));
        },
        clean_doc_string(R"DOC(
            @signature def y_error(self, *targets: int, p: float):
            Probabilistically applies Y errors to targets.

            Args:
                *targets: The indices of the qubits to target with the noise.
                p: The chance of the Y error being applied,
                    independently, to each qubit.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.y_error(0, 1, 2, p=0.01)
        )DOC")
            .data());

    c.def(
        "z_error",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args, const pybind11::kwargs &kwargs) {
            double p = pybind11::cast<double>(kwargs["p"]);
            if (kwargs.size() != 1) {
                throw std::invalid_argument("Unexpected argument. Expected position-only targets and p=probability.");
            }
            self.do_Z_ERROR(
                build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::Z_ERROR, args, &p));
        },
        clean_doc_string(R"DOC(
            @signature def z_error(self, *targets: int, p: float):
            Probabilistically applies Z errors to targets.

            Args:
                *targets: The indices of the qubits to target with the noise.
                p: The chance of the Z error being applied,
                    independently, to each qubit.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.z_error(0, 1, 2, p=0.01)
        )DOC")
            .data());

    c.def(
        "h_xz",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_H_XZ(build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::H, args));
        },
        clean_doc_string(R"DOC(
            Applies a Hadamard gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +X +Y +Z
                >>> s.h_xz(0, 1, 2)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +Z -Y +X
        )DOC")
            .data());

    c.def(
        "c_xyz",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_C_XYZ(
                build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::C_XYZ, args));
        },
        clean_doc_string(R"DOC(
            Applies a C_XYZ gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +X +Y +Z
                >>> s.c_xyz(0, 1, 2)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +Y +Z +X
        )DOC")
            .data());

    c.def(
        "c_zyx",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_C_ZYX(
                build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::C_ZYX, args));
        },
        clean_doc_string(R"DOC(
            Applies a C_ZYX gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +X +Y +Z
                >>> s.c_zyx(0, 1, 2)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +Z +X +Y
        )DOC")
            .data());

    c.def(
        "h_xy",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_H_XY(
                build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::H_XY, args));
        },
        clean_doc_string(R"DOC(
            Applies an operation that swaps the X and Y axes to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +X +Y +Z
                >>> s.h_xy(0, 1, 2)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +Y +X -Z
        )DOC")
            .data());

    c.def(
        "h_yz",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_H_YZ(
                build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::H_YZ, args));
        },
        clean_doc_string(R"DOC(
            Applies an operation that swaps the Y and Z axes to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +X +Y +Z
                >>> s.h_yz(0, 1, 2)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                -X +Z +Y
        )DOC")
            .data());

    c.def(
        "x",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_X(build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::X, args));
        },
        clean_doc_string(R"DOC(
            Applies a Pauli X gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +X +Y +Z
                >>> s.x(0, 1, 2)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +X -Y -Z
        )DOC")
            .data());

    c.def(
        "y",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_Y(build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::Y, args));
        },
        clean_doc_string(R"DOC(
            Applies a Pauli Y gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +X +Y +Z
                >>> s.y(0, 1, 2)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                -X +Y -Z
        )DOC")
            .data());

    c.def(
        "z",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, pybind11::args targets) {
            self.do_Z(build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::Z, targets));
        },
        clean_doc_string(R"DOC(
            Applies a Pauli Z gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +X +Y +Z
                >>> s.z(0, 1, 2)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                -X -Y +Z
        )DOC")
            .data());

    c.def(
        "s",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_SQRT_Z(build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::S, args));
        },
        clean_doc_string(R"DOC(
            Applies a SQRT_Z gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +X +Y +Z
                >>> s.s(0, 1, 2)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +Y -X +Z
        )DOC")
            .data());

    c.def(
        "s_dag",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_SQRT_Z_DAG(
                build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::S_DAG, args));
        },
        clean_doc_string(R"DOC(
            Applies a SQRT_Z_DAG gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +X +Y +Z
                >>> s.s_dag(0, 1, 2)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                -Y +X +Z
        )DOC")
            .data());

    c.def(
        "sqrt_x",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_SQRT_X(
                build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::SQRT_X, args));
        },
        clean_doc_string(R"DOC(
            Applies a SQRT_X gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +X +Y +Z
                >>> s.sqrt_x(0, 1, 2)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +X +Z -Y
        )DOC")
            .data());

    c.def(
        "sqrt_x_dag",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_SQRT_X_DAG(
                build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::SQRT_X_DAG, args));
        },
        clean_doc_string(R"DOC(
            Applies a SQRT_X_DAG gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +X +Y +Z
                >>> s.sqrt_x_dag(0, 1, 2)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +X -Z +Y
        )DOC")
            .data());

    c.def(
        "sqrt_y",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_SQRT_Y(
                build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::SQRT_Y, args));
        },
        clean_doc_string(R"DOC(
            Applies a SQRT_Y gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +X +Y +Z
                >>> s.sqrt_y(0, 1, 2)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                -Z +Y +X
        )DOC")
            .data());

    c.def(
        "sqrt_y_dag",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_SQRT_Y_DAG(
                build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::SQRT_Y_DAG, args));
        },
        clean_doc_string(R"DOC(
            Applies a SQRT_Y_DAG gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +X +Y +Z
                >>> s.sqrt_y_dag(0, 1, 2)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(3)))
                +Z +Y -X
        )DOC")
            .data());

    c.def(
        "swap",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_SWAP(build_two_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::SWAP, args));
        },
        clean_doc_string(R"DOC(
            Applies a swap gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets,
                    and so forth. There must be an even number of targets.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0, 3)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +Z +X
                >>> s.swap(0, 1, 2, 3)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +Y +X +X +Z
        )DOC")
            .data());

    c.def(
        "iswap",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_ISWAP(build_two_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::ISWAP, args));
        },
        clean_doc_string(R"DOC(
            Applies an ISWAP gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets,
                    and so forth. There must be an even number of targets.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0, 3)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +Z +X
                >>> s.iswap(0, 1, 2, 3)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +_ +_ +Y +Z
        )DOC")
            .data());

    c.def(
        "iswap_dag",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_ISWAP_DAG(
                build_two_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::ISWAP_DAG, args));
        },
        clean_doc_string(R"DOC(
            Applies an ISWAP_DAG gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets,
                    and so forth. There must be an even number of targets.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0, 3)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +Z +X
                >>> s.iswap_dag(0, 1, 2, 3)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +_ +_ -Y +Z
        )DOC")
            .data());

    c.def(
        "cnot",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_ZCX(build_two_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::CX, args));
        },
        clean_doc_string(R"DOC(
            Applies a controlled X gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets,
                    and so forth. There must be an even number of targets.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0, 3)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +Z +X
                >>> s.cnot(0, 1, 2, 3)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +_ +_ +Z +X
        )DOC")
            .data());

    c.def(
        "zcx",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_ZCX(build_two_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::CX, args));
        },
        clean_doc_string(R"DOC(
            Applies a controlled X gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets,
                    and so forth. There must be an even number of targets.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0, 3)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +Z +X
                >>> s.zcx(0, 1, 2, 3)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +_ +_ +Z +X
        )DOC")
            .data());

    c.def(
        "cx",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_ZCX(build_two_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::CX, args));
        },
        clean_doc_string(R"DOC(
            Applies a controlled X gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets,
                    and so forth. There must be an even number of targets.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0, 3)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +Z +X
                >>> s.cx(0, 1, 2, 3)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +_ +_ +Z +X
        )DOC")
            .data());

    c.def(
        "cz",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_ZCZ(build_two_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::CZ, args));
        },
        clean_doc_string(R"DOC(
            Applies a controlled Z gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets,
                    and so forth. There must be an even number of targets.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0, 3)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +Z +X
                >>> s.cz(0, 1, 2, 3)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +_ +_ +Z +X
        )DOC")
            .data());

    c.def(
        "zcz",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_ZCZ(build_two_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::CZ, args));
        },
        clean_doc_string(R"DOC(
            Applies a controlled Z gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets,
                    and so forth. There must be an even number of targets.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0, 3)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +Z +X
                >>> s.zcz(0, 1, 2, 3)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +_ +_ +Z +X
        )DOC")
            .data());

    c.def(
        "cy",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_ZCY(build_two_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::CY, args));
        },
        clean_doc_string(R"DOC(
            Applies a controlled Y gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets,
                    and so forth. There must be an even number of targets.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0, 3)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +Z +X
                >>> s.cy(0, 1, 2, 3)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +Z +X
        )DOC")
            .data());

    c.def(
        "zcy",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_ZCY(build_two_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::CY, args));
        },
        clean_doc_string(R"DOC(
            Applies a controlled Y gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets,
                    and so forth. There must be an even number of targets.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0, 3)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +Z +X
                >>> s.zcy(0, 1, 2, 3)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +Z +X
        )DOC")
            .data());

    c.def(
        "xcx",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_XCX(build_two_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::XCX, args));
        },
        clean_doc_string(R"DOC(
            Applies an X-controlled X gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets,
                    and so forth. There must be an even number of targets.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0, 3)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +Z +X
                >>> s.xcx(0, 1, 2, 3)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +Z +X
        )DOC")
            .data());

    c.def(
        "xcy",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_XCY(build_two_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::XCY, args));
        },
        clean_doc_string(R"DOC(
            Applies an X-controlled Y gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets,
                    and so forth. There must be an even number of targets.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0, 3)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +Z +X
                >>> s.xcy(0, 1, 2, 3)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +_ +_
        )DOC")
            .data());

    c.def(
        "xcz",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_XCZ(build_two_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::XCZ, args));
        },
        clean_doc_string(R"DOC(
            Applies an X-controlled Z gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets,
                    and so forth. There must be an even number of targets.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0, 3)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +Z +X
                >>> s.xcz(0, 1, 2, 3)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +_ +_
        )DOC")
            .data());

    c.def(
        "ycx",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_YCX(build_two_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::YCX, args));
        },
        clean_doc_string(R"DOC(
            Applies a Y-controlled X gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets,
                    and so forth. There must be an even number of targets.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0, 3)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +Z +X
                >>> s.ycx(0, 1, 2, 3)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +_ +_ +Z +X
        )DOC")
            .data());

    c.def(
        "ycy",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_YCY(build_two_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::YCY, args));
        },
        clean_doc_string(R"DOC(
            Applies a Y-controlled Y gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets,
                    and so forth. There must be an even number of targets.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0, 3)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +Z +X
                >>> s.ycy(0, 1, 2, 3)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +_ +_
        )DOC")
            .data());

    c.def(
        "ycz",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_YCZ(build_two_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::YCZ, args));
        },
        clean_doc_string(R"DOC(
            Applies a Y-controlled Z gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets,
                    and so forth. There must be an even number of targets.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0, 3)
                >>> s.reset_y(1)

                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +X +Y +Z +X
                >>> s.ycz(0, 1, 2, 3)
                >>> print(" ".join(str(s.peek_bloch(k)) for k in range(4)))
                +_ +_ +_ +_
        )DOC")
            .data());

    c.def(
        "reset",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_RZ(build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::R, args));
        },
        clean_doc_string(R"DOC(
            Resets qubits to the |0> state.

            Args:
                *targets: The indices of the qubits to reset.

            Example:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.x(0)
                >>> s.reset(0)
                >>> s.peek_bloch(0)
                stim.PauliString("+Z")
        )DOC")
            .data());

    c.def(
        "reset_x",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_RX(build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::RX, args));
        },
        clean_doc_string(R"DOC(
            Resets qubits to the |+> state.

            Args:
                *targets: The indices of the qubits to reset.

            Example:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0)
                >>> s.peek_bloch(0)
                stim.PauliString("+X")
        )DOC")
            .data());

    c.def(
        "reset_y",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_RY(build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::RY, args));
        },
        clean_doc_string(R"DOC(
            Resets qubits to the |i> state.

            Args:
                *targets: The indices of the qubits to reset.

            Example:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_y(0)
                >>> s.peek_bloch(0)
                stim.PauliString("+Y")
        )DOC")
            .data());

    c.def(
        "reset_z",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            self.do_RZ(build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::R, args));
        },
        clean_doc_string(R"DOC(
            Resets qubits to the |0> state.

            Args:
                *targets: The indices of the qubits to reset.

            Example:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.h(0)
                >>> s.reset_z(0)
                >>> s.peek_bloch(0)
                stim.PauliString("+Z")
        )DOC")
            .data());

    c.def(
        "peek_x",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, uint32_t target) -> int8_t {
            self.ensure_large_enough_for_qubits(target + 1);
            return self.peek_x(target);
        },
        pybind11::arg("target"),
        clean_doc_string(R"DOC(
            Returns the expected value of a qubit's X observable.

            Because the simulator's state is always a stabilizer state, the expectation will
            always be exactly -1, 0, or +1.

            This is a non-physical operation.
            It reports information about the quantum state without disturbing it.

            Args:
                target: The qubit to analyze.

            Returns:
                +1: Qubit is in the |+> state.
                -1: Qubit is in the |-> state.
                0: Qubit is in some other state.

            Example:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_z(0)
                >>> s.peek_x(0)
                0
                >>> s.reset_x(0)
                >>> s.peek_x(0)
                1
                >>> s.z(0)
                >>> s.peek_x(0)
                -1
        )DOC")
            .data());

    c.def(
        "peek_y",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, uint32_t target) -> int8_t {
            self.ensure_large_enough_for_qubits(target + 1);
            return self.peek_y(target);
        },
        pybind11::arg("target"),
        clean_doc_string(R"DOC(
            Returns the expected value of a qubit's Y observable.

            Because the simulator's state is always a stabilizer state, the expectation will
            always be exactly -1, 0, or +1.

            This is a non-physical operation.
            It reports information about the quantum state without disturbing it.

            Args:
                target: The qubit to analyze.

            Returns:
                +1: Qubit is in the |i> state.
                -1: Qubit is in the |-i> state.
                0: Qubit is in some other state.

            Example:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_z(0)
                >>> s.peek_y(0)
                0
                >>> s.reset_y(0)
                >>> s.peek_y(0)
                1
                >>> s.z(0)
                >>> s.peek_y(0)
                -1
        )DOC")
            .data());

    c.def(
        "peek_z",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, uint32_t target) -> int8_t {
            self.ensure_large_enough_for_qubits(target + 1);
            return self.peek_z(target);
        },
        pybind11::arg("target"),
        clean_doc_string(R"DOC(
            Returns the expected value of a qubit's Z observable.

            Because the simulator's state is always a stabilizer state, the expectation will
            always be exactly -1, 0, or +1.

            This is a non-physical operation.
            It reports information about the quantum state without disturbing it.

            Args:
                target: The qubit to analyze.

            Returns:
                +1: Qubit is in the |0> state.
                -1: Qubit is in the |1> state.
                0: Qubit is in some other state.

            Example:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.reset_x(0)
                >>> s.peek_z(0)
                0
                >>> s.reset_z(0)
                >>> s.peek_z(0)
                1
                >>> s.x(0)
                >>> s.peek_z(0)
                -1
        )DOC")
            .data());

    c.def(
        "peek_bloch",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, size_t target) {
            self.ensure_large_enough_for_qubits(target + 1);
            return FlexPauliString(self.peek_bloch(target));
        },
        pybind11::arg("target"),
        clean_doc_string(R"DOC(
            Returns the state of the qubit as a single-qubit stim.PauliString stabilizer.

            This is a non-physical operation. It reports information about the qubit without
            disturbing it.

            Args:
                target: The qubit to peek at.

            Returns:
                stim.PauliString("I"):
                    The qubit is entangled. Its bloch vector is x=y=z=0.
                stim.PauliString("+Z"):
                    The qubit is in the |0> state. Its bloch vector is z=+1, x=y=0.
                stim.PauliString("-Z"):
                    The qubit is in the |1> state. Its bloch vector is z=-1, x=y=0.
                stim.PauliString("+Y"):
                    The qubit is in the |i> state. Its bloch vector is y=+1, x=z=0.
                stim.PauliString("-Y"):
                    The qubit is in the |-i> state. Its bloch vector is y=-1, x=z=0.
                stim.PauliString("+X"):
                    The qubit is in the |+> state. Its bloch vector is x=+1, y=z=0.
                stim.PauliString("-X"):
                    The qubit is in the |-> state. Its bloch vector is x=-1, y=z=0.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.peek_bloch(0)
                stim.PauliString("+Z")
                >>> s.x(0)
                >>> s.peek_bloch(0)
                stim.PauliString("-Z")
                >>> s.h(0)
                >>> s.peek_bloch(0)
                stim.PauliString("-X")
                >>> s.sqrt_x(1)
                >>> s.peek_bloch(1)
                stim.PauliString("-Y")
                >>> s.cz(0, 1)
                >>> s.peek_bloch(0)
                stim.PauliString("+_")
        )DOC")
            .data());

    c.def(
        "peek_observable_expectation",
        [](const TableauSimulator<MAX_BITWORD_WIDTH> &self, const FlexPauliString &observable) -> int8_t {
            if (observable.imag) {
                throw std::invalid_argument(
                    "Observable isn't Hermitian; it has imaginary sign. Need observable.sign in [1, -1].");
            }
            return self.peek_observable_expectation(observable.value);
        },
        pybind11::arg("observable"),
        clean_doc_string(R"DOC(
            Determines the expected value of an observable.

            Because the simulator's state is always a stabilizer state, the expectation will
            always be exactly -1, 0, or +1.

            This is a non-physical operation.
            It reports information about the quantum state without disturbing it.

            Args:
                observable: The observable to determine the expected value of.
                    This observable must have a real sign, not an imaginary sign.

            Returns:
                +1: Observable will be deterministically false when measured.
                -1: Observable will be deterministically true when measured.
                0: Observable will be random when measured.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.peek_observable_expectation(stim.PauliString("+Z"))
                1
                >>> s.peek_observable_expectation(stim.PauliString("+X"))
                0
                >>> s.peek_observable_expectation(stim.PauliString("-Z"))
                -1

                >>> s.do(stim.Circuit('''
                ...     H 0
                ...     CNOT 0 1
                ... '''))
                >>> queries = ['XX', 'YY', 'ZZ', '-ZZ', 'ZI', 'II', 'IIZ']
                >>> for q in queries:
                ...     print(q, s.peek_observable_expectation(stim.PauliString(q)))
                XX 1
                YY -1
                ZZ 1
                -ZZ -1
                ZI 0
                II 1
                IIZ 1
        )DOC")
            .data());

    c.def(
        "measure_observable",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self,
           const FlexPauliString &observable,
           double flip_probability) -> bool {
            if (observable.imag) {
                throw std::invalid_argument(
                    "Observable isn't Hermitian; it has imaginary sign. Need observable.sign in [1, -1].");
            }
            return self.measure_pauli_string(observable.value, flip_probability);
        },
        pybind11::arg("observable"),
        pybind11::kw_only(),
        pybind11::arg("flip_probability") = 0.0,
        clean_doc_string(R"DOC(
            Measures an pauli string observable, as if by an MPP instruction.

            Args:
                observable: The observable to measure, specified as a stim.PauliString.
                flip_probability: Probability of the recorded measurement result being
                    flipped.

            Returns:
                The result of the measurement.

                The result is also recorded into the measurement record.

            Raises:
                ValueError: The given pauli string isn't Hermitian, or the given probability
                    isn't a valid probability.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.h(0)
                >>> s.cnot(0, 1)

                >>> s.measure_observable(stim.PauliString("XX"))
                False

                >>> s.measure_observable(stim.PauliString("YY"))
                True

                >>> s.measure_observable(stim.PauliString("-ZZ"))
                True
        )DOC")
            .data());

    c.def(
        "postselect_observable",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const FlexPauliString &observable, bool desired_value) {
            if (observable.imag) {
                throw std::invalid_argument(
                    "Observable isn't Hermitian; it has imaginary sign. Need observable.sign in [1, -1].");
            }
            self.postselect_observable(observable.value, desired_value);
        },
        pybind11::arg("observable"),
        pybind11::kw_only(),
        pybind11::arg("desired_value") = false,
        clean_doc_string(R"DOC(
            Projects into a desired observable, or raises an exception if it was impossible.

            Postselecting an observable forces it to collapse to a specific eigenstate,
            as if it was measured and that state was the result of the measurement.

            Args:
                observable: The observable to postselect, specified as a pauli string.
                    The pauli string's sign must be -1 or +1 (not -i or +i).
                desired_value:
                    False (default): Postselect into the +1 eigenstate of the observable.
                    True: Postselect into the -1 eigenstate of the observable.

            Raises:
                ValueError:
                    The given observable had an imaginary sign.
                    OR
                    The postselection was impossible. The observable was in the opposite
                    eigenstate, so measuring it would never ever return the desired result.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.postselect_observable(stim.PauliString("+XX"))
                >>> s.postselect_observable(stim.PauliString("+ZZ"))
                >>> s.peek_observable_expectation(stim.PauliString("+YY"))
                -1
        )DOC")
            .data());

    c.def(
        "measure",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, uint32_t target) {
            self.ensure_large_enough_for_qubits(target + 1);
            GateTarget g{target};
            self.do_MZ(CircuitInstruction{GateType::M, {}, &g, ""});
            return (bool)self.measurement_record.storage.back();
        },
        pybind11::arg("target"),
        clean_doc_string(R"DOC(
            Measures a single qubit.

            Unlike the other methods on TableauSimulator, this one does not broadcast
            over multiple targets. This is to avoid returning a list, which would
            create a pitfall where typing `if sim.measure(qubit)` would be a bug.

            To measure multiple qubits, use `TableauSimulator.measure_many`.

            Args:
                target: The index of the qubit to measure.

            Returns:
                The measurement result as a bool.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.x(1)
                >>> s.measure(0)
                False
                >>> s.measure(1)
                True
        )DOC")
            .data());

    c.def(
        "measure_many",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::args &args) {
            auto converted_args =
                build_single_qubit_gate_instruction_ensure_size<MAX_BITWORD_WIDTH>(self, GateType::M, args);
            self.do_MZ(converted_args);
            auto e = self.measurement_record.storage.end();
            return std::vector<bool>(e - converted_args.targets.size(), e);
        },
        clean_doc_string(R"DOC(
            Measures multiple qubits.

            Args:
                *targets: The indices of the qubits to measure.

            Returns:
                The measurement results as a list of bools.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.x(1)
                >>> s.measure_many(0, 1)
                [False, True]
        )DOC")
            .data());

    c.def(
        "postselect_x",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::object &targets, bool desired_value) {
            auto gate_targets = arg_to_qubit_or_qubits<MAX_BITWORD_WIDTH>(self, targets);
            self.postselect_x(gate_targets, desired_value);
        },
        pybind11::arg("targets"),
        pybind11::kw_only(),
        pybind11::arg("desired_value"),
        clean_doc_string(R"DOC(
            @signature def postselect_x(self, targets: Union[int, Iterable[int]], *, desired_value: bool) -> None:
            Postselects qubits in the X basis, or raises an exception.

            Postselecting a qubit forces it to collapse to a specific state, as
            if it was measured and that state was the result of the measurement.

            Args:
                targets: The qubit index or indices to postselect.
                desired_value:
                    False: postselect targets into the |+> state.
                    True: postselect targets into the |-> state.

            Raises:
                ValueError:
                    The postselection failed. One of the qubits was in a state
                    orthogonal to the desired state, so it was literally
                    impossible for a measurement of the qubit to return the
                    desired result.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.peek_x(0)
                0
                >>> s.postselect_x(0, desired_value=False)
                >>> s.peek_x(0)
                1
                >>> s.h(0)
                >>> s.peek_x(0)
                0
                >>> s.postselect_x(0, desired_value=True)
                >>> s.peek_x(0)
                -1
        )DOC")
            .data());

    c.def(
        "postselect_y",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::object &targets, bool desired_value) {
            auto gate_targets = arg_to_qubit_or_qubits<MAX_BITWORD_WIDTH>(self, targets);
            self.postselect_y(gate_targets, desired_value);
        },
        pybind11::arg("targets"),
        pybind11::kw_only(),
        pybind11::arg("desired_value"),
        clean_doc_string(R"DOC(
            @signature def postselect_y(self, targets: Union[int, Iterable[int]], *, desired_value: bool) -> None:
            Postselects qubits in the Y basis, or raises an exception.

            Postselecting a qubit forces it to collapse to a specific state, as
            if it was measured and that state was the result of the measurement.

            Args:
                targets: The qubit index or indices to postselect.
                desired_value:
                    False: postselect targets into the |i> state.
                    True: postselect targets into the |-i> state.

            Raises:
                ValueError:
                    The postselection failed. One of the qubits was in a state
                    orthogonal to the desired state, so it was literally
                    impossible for a measurement of the qubit to return the
                    desired result.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.peek_y(0)
                0
                >>> s.postselect_y(0, desired_value=False)
                >>> s.peek_y(0)
                1
                >>> s.reset_x(0)
                >>> s.peek_y(0)
                0
                >>> s.postselect_y(0, desired_value=True)
                >>> s.peek_y(0)
                -1
        )DOC")
            .data());

    c.def(
        "postselect_z",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::object &targets, bool desired_value) {
            auto gate_targets = arg_to_qubit_or_qubits<MAX_BITWORD_WIDTH>(self, targets);
            self.postselect_z(gate_targets, desired_value);
        },
        pybind11::arg("targets"),
        pybind11::kw_only(),
        pybind11::arg("desired_value"),
        clean_doc_string(R"DOC(
            @signature def postselect_z(self, targets: Union[int, Iterable[int]], *, desired_value: bool) -> None:
            Postselects qubits in the Z basis, or raises an exception.

            Postselecting a qubit forces it to collapse to a specific state, as if it was
            measured and that state was the result of the measurement.

            Args:
                targets: The qubit index or indices to postselect.
                desired_value:
                    False: postselect targets into the |0> state.
                    True: postselect targets into the |1> state.

            Raises:
                ValueError:
                    The postselection failed. One of the qubits was in a state
                    orthogonal to the desired state, so it was literally
                    impossible for a measurement of the qubit to return the
                    desired result.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.h(0)
                >>> s.peek_z(0)
                0
                >>> s.postselect_z(0, desired_value=False)
                >>> s.peek_z(0)
                1
                >>> s.h(0)
                >>> s.peek_z(0)
                0
                >>> s.postselect_z(0, desired_value=True)
                >>> s.peek_z(0)
                -1
        )DOC")
            .data());

    c.def_property_readonly(
        "num_qubits",
        [](const TableauSimulator<MAX_BITWORD_WIDTH> &self) -> size_t {
            return self.inv_state.num_qubits;
        },
        clean_doc_string(R"DOC(
            Returns the number of qubits currently being tracked by the simulator.

            Note that the number of qubits being tracked will implicitly increase if qubits
            beyond the current limit are touched. Untracked qubits are always assumed to be
            in the |0> state.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.num_qubits
                0
                >>> s.h(2)
                >>> s.num_qubits
                3
        )DOC")
            .data());

    c.def(
        "set_num_qubits",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, uint32_t new_num_qubits) {
            self.set_num_qubits(new_num_qubits);
        },
        pybind11::arg("new_num_qubits"),
        clean_doc_string(R"DOC(
            Resizes the simulator's internal state.

            This forces the simulator's internal state to track exactly the qubits whose
            indices are in `range(new_num_qubits)`.

            Note that untracked qubits are always assumed to be in the |0> state. Therefore,
            calling this method will effectively force any qubit whose index is outside
            `range(new_num_qubits)` to be reset to |0>.

            Note that this method does not prevent future operations from implicitly
            expanding the size of the tracked state (e.g. setting the number of qubits to 5
            will not prevent a Hadamard from then being applied to qubit 100, increasing the
            number of qubits back to 101).

            Args:
                new_num_qubits: The length of the range of qubits the internal simulator
                    should be tracking.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> len(s.current_inverse_tableau())
                0

                >>> s.set_num_qubits(5)
                >>> len(s.current_inverse_tableau())
                5

                >>> s.x(0, 1, 2, 3)
                >>> s.set_num_qubits(2)
                >>> s.measure_many(0, 1, 2, 3)
                [True, True, False, False]
        )DOC")
            .data());

    c.def(
        "set_inverse_tableau",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, const Tableau<MAX_BITWORD_WIDTH> &new_inverse_tableau) {
            self.inv_state = new_inverse_tableau;
        },
        pybind11::arg("new_inverse_tableau"),
        clean_doc_string(R"DOC(
            Overwrites the simulator's internal state with the given inverse tableau.

            The inverse tableau specifies how Pauli product observables of qubits at the
            current time transform into equivalent Pauli product observables at the
            beginning of time, when all qubits were in the |0> state. For example, if the Z
            observable on qubit 5 maps to a product of Z observables at the start of time
            then a Z basis measurement on qubit 5 will be deterministic and equal to the
            sign of the product. Whereas if it mapped to a product of observables including
            an X or a Y then the Z basis measurement would be random.

            Any qubits not within the length of the tableau are implicitly in the |0> state.

            Args:
                new_inverse_tableau: The tableau to overwrite the internal state with.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> t = stim.Tableau.random(4)
                >>> s.set_inverse_tableau(t)
                >>> s.current_inverse_tableau() == t
                True
        )DOC")
            .data());

    c.def(
        "copy",
        [](const TableauSimulator<MAX_BITWORD_WIDTH> &self, bool copy_rng, pybind11::object &seed) {
            if (copy_rng && !seed.is_none()) {
                throw std::invalid_argument("seed and copy_rng are incompatible");
            }

            if (!copy_rng || !seed.is_none()) {
                TableauSimulator<MAX_BITWORD_WIDTH> copy_with_new_rng(self, make_py_seeded_rng(seed));
                return copy_with_new_rng;
            }

            TableauSimulator<MAX_BITWORD_WIDTH> copy = self;
            return copy;
        },
        pybind11::kw_only(),
        pybind11::arg("copy_rng") = false,
        pybind11::arg("seed") = pybind11::none(),
        clean_doc_string(R"DOC(
            @signature def copy(self, *, copy_rng: bool = False, seed: Optional[int] = None) -> stim.TableauSimulator:
            Returns a simulator with the same internal state, except perhaps its prng.

            Args:
                copy_rng: By default, new simulator's prng is reinitialized with a random
                    seed. However, one can set this argument to True in order to have the
                    prng state copied together with the rest of the original simulator's
                    state. Consequently, in this case the two simulators will produce the
                    same measurement outcomes for the same quantum circuits.  If both seed
                    and copy_rng are set, an exception is raised. Defaults to False.
                seed: PARTIALLY determines simulation results by deterministically seeding
                    the random number generator.

                    Must be None or an integer in range(2**64).

                    Defaults to None. When None, the prng state is either copied from the
                    original simulator or reseeded from system entropy, depending on the
                    copy_rng argument.

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

                    CAUTION: simulation results *MAY NOT* be consistent if you vary how the
                    circuit is executed. For example, reordering whether a reset on one
                    qubit happens before or after a reset on another qubit can result in
                    different measurement results being observed starting from the same
                    seed.

            Examples:
                >>> import stim

                >>> s1 = stim.TableauSimulator()
                >>> s1.set_inverse_tableau(stim.Tableau.random(1))
                >>> s2 = s1.copy()
                >>> s2 is s1
                False
                >>> s2.current_inverse_tableau() == s1.current_inverse_tableau()
                True

                >>> s1 = stim.TableauSimulator()
                >>> s2 = s1.copy(copy_rng=True)
                >>> s1.h(0)
                >>> s2.h(0)
                >>> assert s1.measure(0) == s2.measure(0)

                >>> s = stim.TableauSimulator()
                >>> def brute_force_post_select(qubit, desired_result):
                ...     global s
                ...     while True:
                ...         s2 = s.copy()
                ...         if s2.measure(qubit) == desired_result:
                ...             s = s2
                ...             break
                >>> s.h(0)
                >>> brute_force_post_select(qubit=0, desired_result=True)
                >>> s.measure(0)
                True
        )DOC")
            .data());

    c.def(
        "measure_kickback",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, uint32_t target) {
            self.ensure_large_enough_for_qubits(target + 1);
            auto result = self.measure_kickback_z({target});
            if (result.second.num_qubits == 0) {
                return pybind11::make_tuple(result.first, pybind11::none());
            }
            return pybind11::make_tuple(result.first, FlexPauliString(result.second));
        },
        pybind11::arg("target"),
        clean_doc_string(R"DOC(
            Measures a qubit and returns the result as well as its Pauli kickback (if any).

            The "Pauli kickback" of a stabilizer circuit measurement is a set of Pauli
            operations that flip the post-measurement system state between the two possible
            post-measurement states. For example, consider measuring one of the qubits in
            the state |00>+|11> in the Z basis. If the measurement result is False, then the
            system projects into the state |00>. If the measurement result is True, then the
            system projects into the state |11>. Applying a Pauli X operation to both qubits
            flips between |00> and |11>. Therefore the Pauli kickback of the measurement is
            `stim.PauliString("XX")`. Note that there are often many possible equivalent
            Pauli kickbacks. For example, if in the previous example there was a third qubit
            in the |0> state, then both `stim.PauliString("XX_")` and
            `stim.PauliString("XXZ")` are valid kickbacks.

            Measurements with deterministic results don't have a Pauli kickback.

            Args:
                target: The index of the qubit to measure.

            Returns:
                A (result, kickback) tuple.
                The result is a bool containing the measurement's output.
                The kickback is either None (meaning the measurement was deterministic) or a
                stim.PauliString (meaning the measurement was random, and the operations in
                the Pauli string flip between the two possible post-measurement states).

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()

                >>> s.measure_kickback(0)
                (False, None)

                >>> s.h(0)
                >>> s.measure_kickback(0)[1]
                stim.PauliString("+X")

                >>> def pseudo_post_select(qubit, desired_result):
                ...     m, kick = s.measure_kickback(qubit)
                ...     if m != desired_result:
                ...         if kick is None:
                ...             raise ValueError("Post-selected the impossible!")
                ...         s.do(kick)
                >>> s = stim.TableauSimulator()
                >>> s.h(0)
                >>> s.cnot(0, 1)
                >>> s.cnot(0, 2)
                >>> pseudo_post_select(qubit=2, desired_result=True)
                >>> s.measure_many(0, 1, 2)
                [True, True, True]
        )DOC")
            .data());

    c.def(
        "set_state_from_stabilizers",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self,
           pybind11::object &stabilizers,
           bool allow_redundant,
           bool allow_underconstrained) {
            std::vector<PauliString<MAX_BITWORD_WIDTH>> converted_stabilizers;
            for (const auto &stabilizer : stabilizers) {
                const FlexPauliString &p = pybind11::cast<FlexPauliString>(stabilizer);
                if (p.imag) {
                    throw std::invalid_argument("Stabilizers can't have imaginary sign.");
                }
                converted_stabilizers.push_back(p.value);
            }
            self.inv_state = stabilizers_to_tableau<MAX_BITWORD_WIDTH>(
                converted_stabilizers, allow_redundant, allow_underconstrained, true);
        },
        pybind11::arg("stabilizers"),
        pybind11::kw_only(),
        pybind11::arg("allow_redundant") = false,
        pybind11::arg("allow_underconstrained") = false,
        clean_doc_string(R"DOC(
            @signature def set_state_from_stabilizers(self, stabilizers: Iterable[stim.PauliString], *, allow_redundant: bool = False, allow_underconstrained: bool = False) -> None:
            Sets the tableau simulator's state to a state satisfying the given stabilizers.

            The old quantum state is completely overwritten, even if the new state is
            underconstrained by the given stabilizers. The number of qubits is changed to
            exactly match the number of qubits in the longest given stabilizer.

            Args:
                stabilizers: A list of `stim.PauliString`s specifying the stabilizers that
                    the new state must have. It is permitted for stabilizers to have
                    different lengths. All stabilizers are padded up to the length of the
                    longest stabilizer by appending identity terms.
                allow_redundant: Defaults to False. If set to False, then the given
                    stabilizers must all be independent. If any one of them is a product of
                    the others (including the empty product), an exception will be raised.
                    If set to True, then redundant stabilizers are simply ignored.
                allow_underconstrained: Defaults to False. If set to False, then the given
                    stabilizers must form a complete set of generators. They must exactly
                    specify the desired stabilizer state, with no degrees of freedom left
                    over. For an n-qubit state there must be n independent stabilizers. If
                    set to True, then there can be leftover degrees of freedom which can be
                    set arbitrarily.

            Returns:
                Nothing. Mutates the states of the simulator to match the desired
                stabilizers.

                Guarantees that self.current_inverse_tableau().inverse_z_output(k) will be
                equal to the k'th independent stabilizer from the `stabilizers` argument.

            Raises:
                ValueError:
                    A stabilizer is redundant but allow_redundant=True wasn't set.
                    OR
                    The given stabilizers are contradictory (e.g. "+Z" and "-Z" both
                    specified).
                    OR
                    The given stabilizers anticommute (e.g. "+Z" and "+X" both specified).
                    OR
                    The stabilizers left behind a degree of freedom but
                    allow_underconstrained=True wasn't set.
                    OR
                    A stabilizer has an imaginary sign (i or -i).

            Examples:

                >>> import stim
                >>> tab_sim = stim.TableauSimulator()
                >>> tab_sim.set_state_from_stabilizers([
                ...     stim.PauliString("XX"),
                ...     stim.PauliString("ZZ"),
                ... ])
                >>> tab_sim.current_inverse_tableau().inverse()
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

                >>> tab_sim.set_state_from_stabilizers([
                ...     stim.PauliString("XX_"),
                ...     stim.PauliString("ZZ_"),
                ...     stim.PauliString("-YY_"),
                ...     stim.PauliString(""),
                ... ], allow_underconstrained=True, allow_redundant=True)
                >>> tab_sim.current_inverse_tableau().inverse()
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+Z__"),
                        stim.PauliString("+_X_"),
                        stim.PauliString("+__X"),
                    ],
                    zs=[
                        stim.PauliString("+XX_"),
                        stim.PauliString("+ZZ_"),
                        stim.PauliString("+__Z"),
                    ],
                )
        )DOC")
            .data());

    c.def(
        "set_state_from_state_vector",
        [](TableauSimulator<MAX_BITWORD_WIDTH> &self, pybind11::object &state_vector, std::string_view endian) {
            bool little_endian;
            if (endian == "little") {
                little_endian = true;
            } else if (endian == "big") {
                little_endian = false;
            } else {
                throw std::invalid_argument("endian not in ['little', 'big']");
            }

            std::vector<std::complex<float>> v;
            for (const auto &obj : state_vector) {
                v.push_back(pybind11::cast<std::complex<float>>(obj));
            }

            self.inv_state = circuit_to_tableau<MAX_BITWORD_WIDTH>(
                                 stabilizer_state_vector_to_circuit(v, little_endian), false, false, false)
                                 .inverse();
        },
        pybind11::arg("state_vector"),
        pybind11::kw_only(),
        pybind11::arg("endian"),
        clean_doc_string(R"DOC(
            @signature def set_state_from_state_vector(self, state_vector: Iterable[float], *, endian: str) -> None:
            Sets the simulator's state to a superposition specified by an amplitude vector.

            Args:
                state_vector: A list of complex amplitudes specifying a superposition. The
                    vector must correspond to a state that is reachable using Clifford
                    operations, and must be normalized (i.e. it must be a unit vector).
                endian:
                    "little": state vector is in little endian order, where higher index
                        qubits correspond to larger changes in the state index.
                    "big": state vector is in big endian order, where higher index qubits
                        correspond to smaller changes in the state index.

            Returns:
                Nothing. Mutates the states of the simulator to match the desired state.

            Raises:
                ValueError:
                    The given state vector isn't a list of complex values specifying a
                    stabilizer state.
                    OR
                    The given endian value isn't 'little' or 'big'.

            Examples:

                >>> import stim
                >>> tab_sim = stim.TableauSimulator()
                >>> tab_sim.set_state_from_state_vector([
                ...     0.5**0.5,
                ...     0.5**0.5 * 1j,
                ... ], endian='little')
                >>> tab_sim.current_inverse_tableau().inverse()
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+Z"),
                    ],
                    zs=[
                        stim.PauliString("+Y"),
                    ],
                )
                >>> tab_sim.set_state_from_state_vector([
                ...     0.5**0.5,
                ...     0,
                ...     0,
                ...     0.5**0.5,
                ... ], endian='little')
                >>> tab_sim.current_inverse_tableau().inverse()
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
}
