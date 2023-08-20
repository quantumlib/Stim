#include "stim/simulators/frame_simulator.pybind.h"

#include "stim/circuit/circuit_instruction.pybind.h"
#include "stim/circuit/circuit_repeat_block.pybind.h"
#include "stim/py/base.pybind.h"
#include "stim/py/numpy.pybind.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/stabilizers/pauli_string.pybind.h"

using namespace stim;
using namespace stim_pybind;

std::optional<size_t> py_index_to_optional_size_t(
    const pybind11::object &index, size_t length, const char *val_name, const char *len_name) {
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
                >>> sim = stim.FlipSimulator(batch_size=256)
        )DOC")
            .data());
}

template <size_t W>
pybind11::object peek_pauli_flips(const FrameSimulator<W> &self, const pybind11::object &py_instance_index) {
    std::optional<size_t> instance_index =
        py_index_to_optional_size_t(py_instance_index, self.batch_size, "instance_index", "batch_size");

    if (instance_index.has_value()) {
        return pybind11::cast(PyPauliString(self.get_frame(*instance_index)));
    }

    std::vector<PyPauliString> result;
    for (size_t k = 0; k < self.batch_size; k++) {
        result.push_back(PyPauliString(self.get_frame(k)));
    }
    return pybind11::cast(std::move(result));
}

template <size_t W>
FrameSimulator<W> create_frame_simulator(
    size_t batch_size, bool disable_heisenberg_uncertainty, uint32_t num_qubits, const pybind11::object &seed) {
    FrameSimulator<W> result(
        CircuitStats{
            0,  // num_detectors
            0,  // num_observables
            0,  // num_measurements
            num_qubits,
            0,                    // num_ticks
            (uint32_t)(1 << 24),  // max_lookback
            0,                    // num_sweep_bits
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
        simd_bits_range_ref<W> row = table[*major_index];
        if (minor_index.has_value()) {
            bool b = row[*minor_index];
            auto np = pybind11::module::import("numpy");
            return np.attr("array")(b, bit_packed ? np.attr("uint8") : np.attr("bool_"));
        } else {
            return simd_bits_to_numpy(row, num_minor_exact, bit_packed);
        }
    } else {
        if (minor_index.has_value()) {
            auto data = table.read_across_majors_at_minor_index(0, num_major_exact, *minor_index);
            return simd_bits_to_numpy(data, num_major_exact, bit_packed);
        } else {
            return simd_bit_table_to_numpy(table, num_major_exact, num_minor_exact, bit_packed);
        }
    }
}

template <size_t W>
pybind11::object get_measurement_flips(
    FrameSimulator<W> &self,
    const pybind11::object &py_record_index,
    const pybind11::object &py_instance_index,
    bool bit_packed) {
    size_t num_measurements = self.m_record.stored;

    std::optional<size_t> instance_index =
        py_index_to_optional_size_t(py_instance_index, self.batch_size, "instance_index", "batch_size");

    std::optional<size_t> record_index =
        py_index_to_optional_size_t(py_record_index, num_measurements, "record_index", "num_measurements");

    return sliced_table_to_numpy(
        self.m_record.storage, num_measurements, self.batch_size, record_index, instance_index, bit_packed);
}

template <size_t W>
pybind11::object get_detector_flips(
    FrameSimulator<W> &self,
    const pybind11::object &py_detector_index,
    const pybind11::object &py_instance_index,
    bool bit_packed) {
    size_t num_detectors = self.det_record.stored;

    std::optional<size_t> instance_index =
        py_index_to_optional_size_t(py_instance_index, self.batch_size, "instance_index", "batch_size");

    std::optional<size_t> detector_index =
        py_index_to_optional_size_t(py_detector_index, num_detectors, "detector_index", "num_detectors");

    return sliced_table_to_numpy(
        self.det_record.storage, num_detectors, self.batch_size, detector_index, instance_index, bit_packed);
}

template <size_t W>
pybind11::object get_obs_flips(
    FrameSimulator<W> &self,
    const pybind11::object &py_observable_index,
    const pybind11::object &py_instance_index,
    bool bit_packed) {
    std::optional<size_t> instance_index =
        py_index_to_optional_size_t(py_instance_index, self.batch_size, "instance_index", "batch_size");

    std::optional<size_t> observable_index =
        py_index_to_optional_size_t(py_observable_index, self.num_observables, "observable_index", "num_observables");

    return sliced_table_to_numpy(
        self.obs_record, self.num_observables, self.batch_size, observable_index, instance_index, bit_packed);
}

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
                >>> sim = stim.FlipSimulator(batch_size=256)
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
                >>> sim = stim.FlipSimulator(batch_size=256)
                >>> sim.batch_size
                256
                >>> sim = stim.FlipSimulator(batch_size=42)
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
                >>> sim = stim.FlipSimulator(batch_size=256)
                >>> sim.num_qubits
                0
                >>> sim = stim.FlipSimulator(batch_size=256, num_qubits=4)
                >>> sim.num_qubits
                4
                >>> sim.do(stim.Circuit('H 5'))
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
                >>> sim = stim.FlipSimulator(batch_size=256)
                >>> sim.num_observables
                0
                >>> sim.do(stim.Circuit('''
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
                >>> sim = stim.FlipSimulator(batch_size=256)
                >>> sim.num_measurements
                0
                >>> sim.do(stim.Circuit('M 3 5'))
                >>> sim.num_measurements
                2
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
                >>> sim = stim.FlipSimulator(batch_size=256)
                >>> sim.num_detectors
                0
                >>> sim.do(stim.Circuit('''
                ...     M 0 0
                ...     DETECTOR rec[-1] rec[-2]
                ... '''))
                >>> sim.num_detectors
                1
        )DOC")
            .data());

    c.def(
        "set_pauli_flip",
        [](FrameSimulator<MAX_BITWORD_WIDTH> &self,
           const pybind11::object &pauli,
           int64_t qubit_index,
           int64_t instance_index) {
            uint8_t p = 255;
            try {
                p = pybind11::cast<uint8_t>(pauli);
            } catch (const pybind11::cast_error &) {
                try {
                    std::string s = pybind11::cast<std::string>(pauli);
                    if (s == "X") {
                        p = 1;
                    } else if (s == "Y") {
                        p = 2;
                    } else if (s == "Z") {
                        p = 3;
                    } else if (s == "I" || s == "_") {
                        p = 0;
                    }
                } catch (const pybind11::cast_error &) {
                }
            }
            if (p > 3) {
                throw std::invalid_argument("Expected pauli in [0, 1, 2, 3, '_', 'I', 'X', 'Y', 'Z']");
            }
            if (instance_index < 0) {
                instance_index += self.batch_size;
            }
            if (qubit_index < 0) {
                throw std::out_of_range("qubit_index");
            }
            if (instance_index < 0 || (uint64_t)instance_index >= self.batch_size) {
                throw std::out_of_range("instance_index");
            }
            if ((uint64_t)qubit_index >= self.num_qubits) {
                CircuitStats stats;
                stats.num_qubits = qubit_index + 1;
                self.ensure_safe_to_do_circuit_with_stats(stats);
            }
            p ^= p >> 1;
            self.x_table[qubit_index][instance_index] = (p & 1) != 0;
            self.z_table[qubit_index][instance_index] = (p & 2) != 0;
        },
        pybind11::arg("pauli"),
        pybind11::kw_only(),
        pybind11::arg("qubit_index"),
        pybind11::arg("instance_index"),
        clean_doc_string(R"DOC(
            @signature def set_pauli_flip(self, pauli: Union[str, int], *, qubit_index: int, instance_index: int) -> None:
            Sets the pauli flip on a given qubit in a given simulation instance.

            Args:
                pauli: The pauli, specified as an integer or string.
                    Uses the convention 0=I, 1=X, 2=Y, 3=Z.
                    Any value from [0, 1, 2, 3, 'X', 'Y', 'Z', 'I', '_'] is allowed.
                qubit_index: The qubit to put the error on. Must be non-negative. The state
                    will automatically expand as needed to store the error.
                instance_index: The simulation index to put the error inside. Use negative
                    indices to index from the end of the list.

            Examples:
                >>> import stim
                >>> sim = stim.FlipSimulator(
                ...     batch_size=2,
                ...     num_qubits=3,
                ...     disable_stabilizer_randomization=True,
                ... )
                >>> sim.set_pauli_flip('X', qubit_index=2, instance_index=1)
                >>> sim.peek_pauli_flips()
                [stim.PauliString("+___"), stim.PauliString("+__X")]
        )DOC")
            .data());

    c.def(
        "peek_pauli_flips",
        &peek_pauli_flips<MAX_BITWORD_WIDTH>,
        pybind11::kw_only(),
        pybind11::arg("instance_index") = pybind11::none(),
        clean_doc_string(R"DOC(
            @overload def peek_pauli_flips(self) -> List[stim.PauliString]:
            @overload def peek_pauli_flips(self, *, instance_index: int) -> stim.PauliString:
            @signature def peek_pauli_flips(self, *, instance_index: Optional[int] = None) -> Union[stim.PauliString, List[stim.PauliString]]:
            Returns the current pauli errors packed into stim.PauliString instances.

            Args:
                instance_index: Defaults to None. When set to None, the pauli errors from
                    all instances are returned as a list of `stim.PauliString`. When set to
                    an integer, a single `stim.PauliString` is returned containing the
                    errors for the indexed instance.

            Returns:
                if instance_index is None:
                    A list of stim.PauliString, with the k'th entry being the errors from
                    the k'th simulation instance.
                else:
                    A stim.PauliString with the errors from the k'th simulation instance.

            Examples:
                >>> import stim
                >>> sim = stim.FlipSimulator(
                ...     batch_size=2,
                ...     disable_stabilizer_randomization=True,
                ...     num_qubits=10,
                ... )

                >>> sim.peek_pauli_flips()
                [stim.PauliString("+__________"), stim.PauliString("+__________")]

                >>> sim.peek_pauli_flips(instance_index=0)
                stim.PauliString("+__________")

                >>> sim.do(stim.Circuit('''
                ...     X_ERROR(1) 0 3 5
                ...     Z_ERROR(1) 3 6
                ... '''))

                >>> sim.peek_pauli_flips()
                [stim.PauliString("+X__Y_XZ___"), stim.PauliString("+X__Y_XZ___")]

                >>> sim = stim.FlipSimulator(
                ...     batch_size=1,
                ...     num_qubits=100,
                ... )
                >>> flips: stim.PauliString = sim.peek_pauli_flips(instance_index=0)
                >>> sorted(set(str(flips)))  # Should have Zs from stabilizer randomization
                ['+', 'Z', '_']

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
                >>> sim = stim.FlipSimulator(batch_size=9)
                >>> sim.do(stim.Circuit('M 0 1 2'))

                >>> sim.get_measurement_flips()
                array([[False, False, False, False, False, False, False, False, False],
                       [False, False, False, False, False, False, False, False, False],
                       [False, False, False, False, False, False, False, False, False]])

                >>> sim.get_measurement_flips(bit_packed=True)
                array([[0, 0],
                       [0, 0],
                       [0, 0]], dtype=uint8)

                >>> sim.get_measurement_flips(instance_index=1)
                array([False, False, False])

                >>> sim.get_measurement_flips(record_index=2)
                array([False, False, False, False, False, False, False, False, False])

                >>> sim.get_measurement_flips(instance_index=1, record_index=2)
                array(False)
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
                >>> sim = stim.FlipSimulator(batch_size=9)
                >>> sim.do(stim.Circuit('''
                ...     M 0 0 0
                ...     DETECTOR rec[-2] rec[-3]
                ...     DETECTOR rec[-1] rec[-2]
                ... '''))

                >>> sim.get_detector_flips()
                array([[False, False, False, False, False, False, False, False, False],
                       [False, False, False, False, False, False, False, False, False]])

                >>> sim.get_detector_flips(bit_packed=True)
                array([[0, 0],
                       [0, 0]], dtype=uint8)

                >>> sim.get_detector_flips(instance_index=2)
                array([False, False])

                >>> sim.get_detector_flips(detector_index=1)
                array([False, False, False, False, False, False, False, False, False])

                >>> sim.get_detector_flips(instance_index=2, detector_index=1)
                array(False)

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
                >>> sim = stim.FlipSimulator(batch_size=9)
                >>> sim.do(stim.Circuit('''
                ...     M 0 0 0
                ...     OBSERVABLE_INCLUDE(0) rec[-2]
                ...     OBSERVABLE_INCLUDE(1) rec[-1]
                ... '''))

                >>> sim.get_observable_flips()
                array([[False, False, False, False, False, False, False, False, False],
                       [False, False, False, False, False, False, False, False, False]])

                >>> sim.get_observable_flips(bit_packed=True)
                array([[0, 0],
                       [0, 0]], dtype=uint8)

                >>> sim.get_observable_flips(instance_index=2)
                array([False, False])

                >>> sim.get_observable_flips(observable_index=1)
                array([False, False, False, False, False, False, False, False, False])

                >>> sim.get_observable_flips(instance_index=2, observable_index=1)
                array(False)
        )DOC")
            .data());

    c.def(
        "do",
        [](FrameSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::object &obj) {
            if (pybind11::isinstance<Circuit>(obj)) {
                self.safe_do_circuit(pybind11::cast<const Circuit &>(obj));
            } else if (pybind11::isinstance<PyCircuitInstruction>(obj)) {
                self.safe_do_instruction(pybind11::cast<const PyCircuitInstruction &>(obj));
            } else if (pybind11::isinstance<CircuitRepeatBlock>(obj)) {
                const CircuitRepeatBlock &block = pybind11::cast<const CircuitRepeatBlock &>(obj);
                self.safe_do_circuit(block.body, block.repeat_count);
            } else {
                throw std::invalid_argument(
                    "Don't know how to do a '" + pybind11::cast<std::string>(pybind11::repr(obj)) + "'.");
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
                >>> sim = stim.FlipSimulator(
                ...     batch_size=1,
                ...     disable_stabilizer_randomization=True,
                ... )
                >>> circuit = stim.Circuit('''
                ...     X_ERROR(1) 0 1 3
                ...     REPEAT 5 {
                ...         H 0
                ...         C_XYZ 1
                ...     }
                ... ''')
                >>> sim.do(circuit)
                >>> sim.peek_pauli_flips()
                [stim.PauliString("+ZZ_X")]

                >>> sim.do(circuit[0])
                >>> sim.peek_pauli_flips()
                [stim.PauliString("+YY__")]

                >>> sim.do(circuit[1])
                >>> sim.peek_pauli_flips()
                [stim.PauliString("+YX__")]
        )DOC")
            .data());
}
