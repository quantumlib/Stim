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

#include "stim/simulators/frame_simulator.pybind.h"

#include "stim/py/base.pybind.h"
#include "stim/py/numpy.pybind.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/circuit/circuit_instruction.pybind.h"
#include "stim/circuit/circuit_repeat_block.pybind.h"

using namespace stim;
using namespace stim_pybind;

pybind11::class_<FrameSimulator<MAX_BITWORD_WIDTH>> stim_pybind::pybind_frame_simulator(pybind11::module &m) {
    return pybind11::class_<FrameSimulator<MAX_BITWORD_WIDTH>>(
        m,
        "FlipSimulator",
        clean_doc_string(R"DOC(
            A simulator that tracks whether things are flipped, instead of what they are.

            Tracking flips is significantly cheaper than tracking actual values, requiring
            O(1) work per gate (compared to O(n) for unitary operations and O(n^2) for
            collapsing operations in the tableau simulator, where n is the qubit count).

            Supports interactive usage, where gates and measurements are applied on demand.

            Examples:
                >>> import stim
                >>> s = stim.FlipSimulator()
                >>> assert False
        )DOC")
            .data());
}

template <size_t W>
pybind11::object peek_current_pauli_errors(const FrameSimulator<W> &self) {
    uint8_t *buffer = new uint8_t[self.num_qubits * self.batch_size];
    size_t t = 0;
    for (size_t k_maj = 0; k_maj < self.batch_size; k_maj++) {
        for (size_t k_min = 0; k_min < self.num_qubits; k_min++) {
            uint8_t x = self.x_table[k_maj][k_min];
            uint8_t z = self.z_table[k_maj][k_min];
            buffer[t++] = (x ^ z) + z * 2;
        }
    }

    pybind11::capsule free_when_done(buffer, [](void *f) {
        delete[] reinterpret_cast<uint8_t *>(f);
    });

    return pybind11::array_t<uint8_t>(
        {(pybind11::ssize_t)self.num_qubits, (pybind11::ssize_t)self.batch_size},
        {(pybind11::ssize_t)self.batch_size, (pybind11::ssize_t)1},
        buffer,
        free_when_done);
}

template <size_t W>
FrameSimulator<W> create_frame_simulator(size_t batch_size, bool disable_heisenberg_uncertainty, uint32_t num_qubits, const pybind11::object &seed) {
    FrameSimulator<W> result(
        CircuitStats{
            0, // num_detectors
            0, // num_observables
            0, // num_measurements
            num_qubits,
            0, // num_ticks
            (uint32_t)(1 << 24), // max_lookback
            0, // num_sweep_bits
        },
        FrameSimulatorMode::STORE_EVERYTHING_TO_MEMORY,
        batch_size,
        make_py_seeded_rng(seed));
    result.guarantee_anticommutation_via_frame_randomization = !disable_heisenberg_uncertainty;
    result.reset_all();
    return result;
}

template <size_t W>
pybind11::object sliced_table_to_numpy(
    const simd_bit_table<W> &table,
    size_t num_major_exact,
    size_t num_minor_exact,
    std::optional<size_t> major_index,
    std::optional<size_t> minor_index,
    bool bit_packed) {
    if (major_index.has_value()) {
        simd_bits_range_ref<W> row = table[major_index.value()];
        if (minor_index.has_value()) {
            bool b = row[minor_index.value()];
            auto np = pybind11::module::import("numpy");
            return np.attr("array")(b, bit_packed ? np.attr("bool_") : np.attr("uint8"));
        } else {
            return simd_bits_to_numpy(row, num_minor_exact, bit_packed);
        }
    } else {
        if (minor_index.has_value()) {
            auto data = table.read_across_majors_at_minor_index(0, num_major_exact, minor_index.value());
            return simd_bits_to_numpy(data, num_major_exact, bit_packed);
        } else {
            return simd_bit_table_to_numpy(table, num_major_exact, num_minor_exact, bit_packed);
        }
    }
}

std::optional<size_t> py_index_to_optional_size_t(pybind11::object index, size_t length, const char *val_name, const char *len_name) {
    std::optional<size_t> instance_index;
    if (index.is_none()) {
        return {};
    }
    int64_t i = pybind11::cast<int64_t>(index);
    if (i < -(int64_t)length || (uint64_t)i >= length) {
        std::stringstream msg;
        msg << "not (";
        msg << "-" << len_name << " <= ";
        msg << val_name << "=" << index;
        msg << " < ";
        msg << len_name << "=" << length;
        msg << ")";
        throw std::out_of_range(msg.str());
    }
    if (i < 0) {
        i += length;
    }
    assert(i >= 0);
    return (size_t)i;
}

template <size_t W>
pybind11::object get_measurement_flips(
    FrameSimulator<W> &self,
    const pybind11::object &py_record_index,
    const pybind11::object &py_instance_index,
    bool bit_packed) {

    size_t num_measurements = self.m_record.stored;

    std::optional<size_t> instance_index = py_index_to_optional_size_t(
        py_instance_index,
        self.batch_size,
        "instance_index",
        "batch_size");

    std::optional<size_t> record_index = py_index_to_optional_size_t(
        py_record_index,
        num_measurements,
        "record_index",
        "num_measurements");

    return sliced_table_to_numpy(
        self.m_record.storage,
        num_measurements,
        self.batch_size,
        record_index,
        instance_index,
        bit_packed);
}

template <size_t W>
pybind11::object get_detector_flips(
    FrameSimulator<W> &self,
    const pybind11::object &py_detector_index,
    const pybind11::object &py_instance_index,
    bool bit_packed) {

    size_t num_detectors = self.det_record.stored;

    std::optional<size_t> instance_index = py_index_to_optional_size_t(
        py_instance_index,
        self.batch_size,
        "instance_index",
        "batch_size");

    std::optional<size_t> detector_index = py_index_to_optional_size_t(
        py_detector_index,
        num_detectors,
        "detector_index",
        "num_measurements");

    return sliced_table_to_numpy(
        self.det_record.storage,
        num_detectors,
        self.batch_size,
        detector_index,
        instance_index,
        bit_packed);
}

template <size_t W>
pybind11::object get_obs_flips(
    FrameSimulator<W> &self,
    const pybind11::object &py_observable_index,
    const pybind11::object &py_instance_index,
    bool bit_packed) {

    std::optional<size_t> instance_index = py_index_to_optional_size_t(
        py_instance_index,
        self.batch_size,
        "instance_index",
        "batch_size");

    std::optional<size_t> observable_index = py_index_to_optional_size_t(
        py_observable_index,
        self.num_observables,
        "observable_index",
        "num_observables");

    return sliced_table_to_numpy(
        self.obs_record,
        self.num_observables,
        self.batch_size,
        observable_index,
        instance_index,
        bit_packed);
}

template <size_t W>
pybind11::object get_xz_flips(
    FrameSimulator<W> &self,
    bool x,
    bool z,
    const pybind11::object &py_observable_index,
    const pybind11::object &py_instance_index,
    bool bit_packed) {

    std::optional<size_t> instance_index = py_index_to_optional_size_t(
        py_instance_index,
        self.batch_size,
        "instance_index",
        "batch_size");

    std::optional<size_t> qubit_index = py_index_to_optional_size_t(
        py_observable_index,
        self.num_observables,
        "qubit_index",
        "num_qubits");

    simd_bit_table<W> *table;
    if (x & z) {
        self.x_table.data ^= self.z_table.data;
        table = &self.x_table;
    } else if (x) {
        table = &self.x_table;
    } else {
        assert(z);
        table = &self.z_table;
    }
    auto result = sliced_table_to_numpy(
        *table,
        self.num_qubits,
        self.batch_size,
        qubit_index,
        instance_index,
        bit_packed);
    if (x & z) {
        self.x_table.data ^= self.z_table.data;
    }
    return result;
}

//template <size_t W>
//PyCircuitInstruction build_single_qubit_gate_instruction_ensure_size(
//    TableauSimulator<W> &self, GateType gate_type, const pybind11::args &args, SpanRef<const double> gate_args = {}) {
//    std::vector<GateTarget> targets;
//    uint32_t max_q = 0;
//    try {
//        for (const auto &e : args) {
//            if (pybind11::isinstance<GateTarget>(e)) {
//                targets.push_back(pybind11::cast<GateTarget>(e));
//            } else {
//                uint32_t q = e.cast<uint32_t>();
//                max_q = std::max(max_q, q & TARGET_VALUE_MASK);
//                targets.push_back(GateTarget{q});
//            }
//        }
//    } catch (const pybind11::cast_error &) {
//        throw std::out_of_range("Target qubits must be non-negative integers.");
//    }
//
//    std::vector<double> gate_args_vec;
//    for (const auto &e : gate_args) {
//        gate_args_vec.push_back(e);
//    }
//
//    // Note: quadratic behavior.
//    self.ensure_large_enough_for_qubits(max_q + 1);
//
//    return PyCircuitInstruction(gate_type, targets, gate_args_vec);
//}
//
void stim_pybind::pybind_frame_simulator_methods(
    pybind11::module &m, pybind11::class_<FrameSimulator<MAX_BITWORD_WIDTH>> &c) {
    c.def(
        pybind11::init(&create_frame_simulator<MAX_BITWORD_WIDTH>),
        pybind11::kw_only(),
        pybind11::arg("batch_size"),
        pybind11::arg("disable_stabilizer_randomization") = false,
        pybind11::arg("num_qubits") = 0,
        pybind11::arg("seed") = pybind11::none(),
        clean_doc_string(R"DOC(
            @signature def __init__(self, *, batch_size: int, disable_stabilizer_randomization: bool = False, num_qubits: int = 0, seed: Optional[int] = None) -> None:
            Initializes a stim.FlipSimulator.

            Args:
                batch_size: For speed, the flip simulator simulates many instances in
                    parallel. This argument determines the number of parallel instances.

                    It's recommended to use a multiple of 256, because internally the state
                    of the instances is striped across SSE (128 bit) or AVX (256 bit)
                    words with one bit in the word belonging to each instance. The result is
                    that, even if you only ask for 1 instance, probably the same amount of
                    work is being done as if you'd asked for 256 instances. The extra
                    results just aren't being used, creating waste.

                disable_stabilizer_randomization: Determines whether or not the flip
                    simulator uses stabilizer randomization. Defaults to False (stabilizer
                    randomization used). Set to True to disable stabilizer randomization.

                    Stabilizer randomization means that, when a qubit is initialized or
                    measured in the Z basis, a Z error is added to the qubit with 50%
                    probability. More generally, anytime a stabilizer is introduced into
                    the system by any means, an error equal to that stabilizer is applied
                    with 50% probability. This ensures that observables anticommuting with
                    stabilizers of the system must be maximally uncertain. In other words,
                    this feature enforces Heisenberg's uncertainty principle.

                    This is a safety feature that you should not turn off unless you have a
                    reason to do so. Stabilizer randomization is turned on by default
                    because it catches mistakes. For example, suppose you are trying to
                    create a stabilizer code but you accidentally have the code measure two
                    anticommuting stabilizers. With stabilizer randomization turned off, it
                    will look like this code works. With stabilizer randomization turned on,
                    the two measurements will correctly randomize each other revealing that
                    the code doesn't work.

                    In some use cases, stabilizer randomization is a hindrance instead of
                    helpful. For example, if you are using the flip simulator to understand
                    how an error propagates through the system, the stabilizer randomization
                    will be introducing error terms that you don't want.

                num_qubits: Sets the initial number of qubits tracked by the simulation.
                    The simulator will still automatically resize as needed when qubits
                    beyond this limit are touched.

                    This parameter exists as a way to hint at the desired size of the
                    simulator's state for performance, and to ensure methods that
                    peek at the size have the expected size from the start instead of
                    only after the relevant qubits have been touched.

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
                An initialized stim.FlipSimulator.

            Examples:
                >>> import stim
                >>> s = stim.FlipSimulator(seed=0)
                >>> s2 = stim.FlipSimulator(seed=0)
                >>> s.x_error(0, p=0.1)
                >>> s2.x_error(0, p=0.1)
                >>> s.measure(0) == s2.measure(0)
                True
        )DOC")
            .data());

    c.def_property_readonly(
         "batch_size",
         [](FrameSimulator<MAX_BITWORD_WIDTH> &self) -> size_t {
            return self.batch_size;
        },
        clean_doc_string(R"DOC(
            Returns the number of instances being simulated by the simulator.

            Examples:
                >>> import stim
                >>> sim = stim.FrameSimulator(batch_size=256)
                >>> sim.batch_size
                256
                >>> sim = stim.FrameSimulator(batch_size=42)
                >>> sim.batch_size
                42
        )DOC")
            .data());

    c.def_property_readonly(
         "num_qubits",
         [](FrameSimulator<MAX_BITWORD_WIDTH> &self) -> size_t {
            return self.num_qubits;
        },
        clean_doc_string(R"DOC(
            Returns the number of qubits currently tracked by the simulator.

            Examples:
                >>> import stim
                >>> sim = stim.FrameSimulator(batch_size=256)
                >>> sim.num_qubits
                0
                >>> sim.h(5)
                >>> sim.num_qubits
                6
        )DOC")
            .data());

    c.def_property_readonly(
         "num_observables",
         [](FrameSimulator<MAX_BITWORD_WIDTH> &self) -> size_t {
            return self.num_observables;
        },
        clean_doc_string(R"DOC(
            Returns the number of observables currently tracked by the simulator.

            Examples:
                >>> import stim
                >>> sim = stim.FrameSimulator(batch_size=256)
                >>> sim.num_observables
                0
                >>> sim.do_circuit(stim.Circuit('''
                ...     M 0
                ...     OBSERVABLE_INCLUDE(4) rec[-1]
                ... '''))
                >>> sim.num_observables
                5
        )DOC")
            .data());

    c.def_property_readonly(
         "num_measurements",
         [](FrameSimulator<MAX_BITWORD_WIDTH> &self) -> size_t {
            return self.m_record.stored;
        },
        clean_doc_string(R"DOC(
            Returns the number of measurements that have been simulated and stored.

            Examples:
                >>> import stim
                >>> sim = stim.FrameSimulator(batch_size=256)
                >>> sim.num_measurements
                0
                >>> sim.measure(5)
                >>> sim.num_measurements
                1
        )DOC")
            .data());

    c.def_property_readonly(
         "num_detectors",
         [](FrameSimulator<MAX_BITWORD_WIDTH> &self) -> size_t {
            return self.det_record.stored;
        },
        clean_doc_string(R"DOC(
            Returns the number of detectors that have been simulated and stored.

            Examples:
                >>> import stim
                >>> sim = stim.FrameSimulator(batch_size=256)
                >>> sim.num_detectors
                0
                >>> sim.do_circuit(stim.Circuit('''
                ...     M 0 0
                ...     DETECTOR rec[-1] rec[-2]
                ... '''))
                >>> sim.num_detectors
                1
        )DOC")
            .data());

    c.def(
        "peek_current_pauli_errors",
        &peek_current_pauli_errors<MAX_BITWORD_WIDTH>,
        clean_doc_string(R"DOC(
            @signature def peek_current_errors(self) -> np.ndarray:
            Creates a numpy array describing the current pauli errors.

            Returns:
                A numpy array with shape=(self.num_qubits, self.batch_size), dtype=np.uint8.

                Each entry in the array is the Pauli error on one qubit in one instance,
                using the convention 0=I, 1=X, 2=Y, 3=Z. For example, if result[5][3] == 2
                then there's a Y error on the qubit with index 5 in the shot with index 3.

            Examples:
                >>> import stim
                >>> assert False
        )DOC")
            .data());

    c.def(
        "get_measurement_flips",
        &get_measurement_flips<MAX_BITWORD_WIDTH>,
        pybind11::kw_only(),
        pybind11::arg("record_index") = pybind11::none(),
        pybind11::arg("instance_index") = pybind11::none(),
        pybind11::arg("bit_packed") = false,
        clean_doc_string(R"DOC(
            @signature def get_measurement_flips(self, *, record_index: Optional[int] = None, instance_index: Optional[int] = None, bit_packed: bool = False) -> np.ndarray:
            Retrieves measurement flip data from the simulator's measurement record.

            Args:
                record_index: Identifies a measurement to read results from.
                    Setting this to None (default) returns results from all measurements.
                    Setting this to a non-negative integer indexes measurements by the order
                        they occurred. For example, record index 0 is the first measurement.
                    Setting this to a negative integer indexes measurements by recency.
                        For example, recording index -1 is the most recent measurement.
                instance_index: Identifies a simulation instance to read results from.
                    Setting this to None (the default) returns results from all instances.
                    Otherwise this should be set to an integer in range(0, self.batch_size).
                bit_packed: Defaults to False. Determines whether the result is bit packed.
                    If this is set to true, the returned numpy array will be bit packed as
                    if by applying

                        out = np.packbits(out, axis=len(out.shape) - 1, bitorder='little')

                    Behind the scenes the data is always bit packed, so setting this
                    argument avoids ever unpacking in the first place. This substantially
                    improves performance when there is a lot of data.

            Returns:
                A numpy array containing the requested data. By default this is a 2d array
                of shape (self.num_measurements, self.batch_size), where the first index is
                the measurement_index and the second index is the instance_index and the
                dtype is np.bool_.

                Specifying record_index slices away the first index, leaving a 1d array
                with only an instance_index.

                Specifying instance_index slices away the last index, leaving a 1d array
                with only a measurement_index (or a 0d array, a boolean, if record_index
                was also specified).

                Specifying bit_packed=True bit packs the last remaining index, changing
                the dtype to np.uint8.

            Examples:
                >>> import stim
                >>> assert False
        )DOC")
            .data());

    c.def(
        "get_detector_flips",
        &get_detector_flips<MAX_BITWORD_WIDTH>,
        pybind11::kw_only(),
        pybind11::arg("detector_index") = pybind11::none(),
        pybind11::arg("instance_index") = pybind11::none(),
        pybind11::arg("bit_packed") = false,
        clean_doc_string(R"DOC(
            @signature def get_detector_flips(self, *, detector_index: Optional[int] = None, instance_index: Optional[int] = None, bit_packed: bool = False) -> np.ndarray:
            Retrieves detector flip data from the simulator's detection event record.

            Args:
                record_index: Identifies a detector to read results from.
                    Setting this to None (default) returns results from all detectors.
                    Otherwise this should be an integer in range(0, self.num_detectors).
                instance_index: Identifies a simulation instance to read results from.
                    Setting this to None (the default) returns results from all instances.
                    Otherwise this should be an integer in range(0, self.batch_size).
                bit_packed: Defaults to False. Determines whether the result is bit packed.
                    If this is set to true, the returned numpy array will be bit packed as
                    if by applying

                        out = np.packbits(out, axis=len(out.shape) - 1, bitorder='little')

                    Behind the scenes the data is always bit packed, so setting this
                    argument avoids ever unpacking in the first place. This substantially
                    improves performance when there is a lot of data.

            Returns:
                A numpy array containing the requested data. By default this is a 2d array
                of shape (self.num_detectors, self.batch_size), where the first index is
                the detector_index and the second index is the instance_index and the
                dtype is np.bool_.

                Specifying detector_index slices away the first index, leaving a 1d array
                with only an instance_index.

                Specifying instance_index slices away the last index, leaving a 1d array
                with only a detector_index (or a 0d array, a boolean, if detector_index
                was also specified).

                Specifying bit_packed=True bit packs the last remaining index, changing
                the dtype to np.uint8.

            Examples:
                >>> import stim
                >>> assert False
        )DOC")
            .data());

    c.def(
        "get_observable_flips",
        &get_obs_flips<MAX_BITWORD_WIDTH>,
        pybind11::kw_only(),
        pybind11::arg("observable_index") = pybind11::none(),
        pybind11::arg("instance_index") = pybind11::none(),
        pybind11::arg("bit_packed") = false,
        clean_doc_string(R"DOC(
            @signature def get_observable_flips(self, *, observable_index: Optional[int] = None, instance_index: Optional[int] = None, bit_packed: bool = False) -> np.ndarray:
            Retrieves observable flip data from the simulator's detection event record.

            Args:
                record_index: Identifies a observable to read results from.
                    Setting this to None (default) returns results from all observables.
                    Otherwise this should be an integer in range(0, self.num_observables).
                instance_index: Identifies a simulation instance to read results from.
                    Setting this to None (the default) returns results from all instances.
                    Otherwise this should be an integer in range(0, self.batch_size).
                bit_packed: Defaults to False. Determines whether the result is bit packed.
                    If this is set to true, the returned numpy array will be bit packed as
                    if by applying

                        out = np.packbits(out, axis=len(out.shape) - 1, bitorder='little')

                    Behind the scenes the data is always bit packed, so setting this
                    argument avoids ever unpacking in the first place. This substantially
                    improves performance when there is a lot of data.

            Returns:
                A numpy array containing the requested data. By default this is a 2d array
                of shape (self.num_observables, self.batch_size), where the first index is
                the observable_index and the second index is the instance_index and the
                dtype is np.bool_.

                Specifying observable_index slices away the first index, leaving a 1d array
                with only an instance_index.

                Specifying instance_index slices away the last index, leaving a 1d array
                with only a observable_index (or a 0d array, a boolean, if observable_index
                was also specified).

                Specifying bit_packed=True bit packs the last remaining index, changing
                the dtype to np.uint8.

            Examples:
                >>> import stim
                >>> assert False
        )DOC")
            .data());

    c.def(
        "do_circuit",
        [](FrameSimulator<MAX_BITWORD_WIDTH> &self, const Circuit &circuit) {
            self.safe_do_circuit(circuit);
        },
        pybind11::arg("circuit"),
        clean_doc_string(R"DOC(
            Applies a all the instructions in a circuit to the simulator's state.

            The results of any measurements performed can be retrieved using the
            `get_measurement_flips` method.

            Args:
                circuit: The circuit to apply to the simulator's state.

            Examples:
                >>> import stim
                >>> assert False
        )DOC")
            .data());

    c.def(
        "do_instruction",
        [](FrameSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::object &instruction) {
            if (pybind11::isinstance<PyCircuitInstruction>(instruction)) {
                self.safe_do_instruction(pybind11::cast<const PyCircuitInstruction &>(instruction).as_operation_ref());
            } else if (pybind11::isinstance<CircuitRepeatBlock>(instruction)) {
                const CircuitRepeatBlock &block = pybind11::cast<const CircuitRepeatBlock &>(instruction);
                self.safe_do_circuit(block.body, block.repeat_count);
            } else {
                throw std::invalid_argument("Not a stim.CircuitInstruction or stim.CircuitRepeatBlock '" + pybind11::cast<std::string>(pybind11::repr(instruction)) + "'.");
            }
        },
        pybind11::arg("instruction"),
        clean_doc_string(R"DOC(
            @signature def do_instruction(self, instruction: Union[stim.CircuitInstruction, stim.CircuitRepeatBlock]) -> None:
            Applies a circuit instruction to the simulator's state.

            The results of any measurements performed can be retrieved using the
            `get_measurement_flips` method.

            Args:
                circuit: The circuit to apply to the simulator's state.

            Examples:
                >>> import stim
                >>> assert False
        )DOC")
            .data());

    c.def(
        "do",
        [](FrameSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::object &obj) {
            if (pybind11::isinstance<PyCircuitInstruction>(obj)) {
                self.safe_do_circuit(pybind11::cast<const Circuit &>(obj));
            } else if (pybind11::isinstance<Circuit>(obj)) {
                const CircuitRepeatBlock &block = pybind11::cast<const CircuitRepeatBlock &>(obj);
                self.safe_do_circuit(block.body, block.repeat_count);
            } else if (pybind11::isinstance<CircuitRepeatBlock>(obj)) {
                const CircuitRepeatBlock &block = pybind11::cast<const CircuitRepeatBlock &>(obj);
                self.safe_do_circuit(block.body, block.repeat_count);
            } else {
                throw std::invalid_argument("Don't know how to do a '" + pybind11::cast<std::string>(pybind11::repr(obj)) + "'.");
            }
        },
        pybind11::arg("obj"),
        clean_doc_string(R"DOC(
            @signature def do(self, obj: Union[stim.Circuit, stim.CircuitInstruction, stim.CircuitRepeatBlock]) -> None:
            Applies a circuit or circuit instruction to the simulator's state.

            The results of any measurements performed can be retrieved using the
            `get_measurement_flips` method.

            Args:
                obj: The circuit or instruction to apply to the simulator's state.

            Examples:
                >>> import stim
                >>> assert False
        )DOC")
            .data());
}
