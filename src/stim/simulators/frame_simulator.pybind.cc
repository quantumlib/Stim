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

static void generate_biased_samples_bit_packed_contiguous(uint8_t *out, size_t num_bytes, float p, std::mt19937_64 &rng) {
    uintptr_t start = (uintptr_t)out;
    uintptr_t end = start + num_bytes;
    uintptr_t aligned64_start = start & ~0b111ULL;
    uintptr_t aligned64_end = end & ~0b111ULL;
    if (aligned64_start != start) {
        aligned64_start += 8;
    }
    biased_randomize_bits(p, (uint64_t *)aligned64_start, (uint64_t *)aligned64_end, rng);
    if (start < aligned64_start) {
        uint64_t pad;
        biased_randomize_bits(p, &pad, &pad + 1, rng);
        for (size_t k = 0; k < aligned64_start - start; k++) {
            out[k] = (uint8_t)(pad & 0xFF);
            pad >>= 8;
        }
    }
    if (aligned64_end < end) {
        uint64_t pad;
        biased_randomize_bits(p, &pad, &pad + 1, rng);
        while (aligned64_end < end) {
            *(uint8_t *)aligned64_end = (uint8_t)(pad & 0xFF);
            aligned64_end++;
            pad >>= 8;
        }
    }
}

static void generate_biased_samples_bit_packed_with_stride(uint8_t *out, pybind11::ssize_t stride, size_t num_bytes, float p, std::mt19937_64 &rng) {
    uint64_t stack[64];
    uint64_t *stack_ptr = &stack[0];
    for (size_t k1 = 0; k1 < num_bytes; k1 += 64*8) {
        size_t n2 = std::min(num_bytes - k1, (size_t)(64*8));
        biased_randomize_bits(p, stack_ptr, stack_ptr + (n2 + 7) / 8, rng);
        uint8_t *stack_data = (uint8_t *)(void *)stack_ptr;
        for (size_t k2 = 0; k2 < n2; k2++) {
            *out = *stack_data;
            stack_data += 1;
            out += stride;
        }
    }
}

static void generate_biased_samples_bool(uint8_t *out, pybind11::ssize_t stride, size_t num_samples, float p, std::mt19937_64 &rng) {
    uint64_t stack[64];
    uint64_t *stack_ptr = &stack[0];
    for (size_t k1 = 0; k1 < num_samples; k1 += 64*64) {
        size_t n2 = std::min(num_samples - k1, (size_t)(64*64));
        biased_randomize_bits(p, stack_ptr, stack_ptr + (n2 + 63) / 64, rng);
        for (size_t k2 = 0; k2 < n2; k2++) {
            bool bit = (stack[k2 / 64] >> (k2 & 63)) & 1;
            *out++ = bit;
        }
    }
}

template <size_t W>
pybind11::object generate_bernoulli_samples(FrameSimulator<W> &self, size_t num_samples, float p, bool bit_packed, pybind11::object out) {
    if (bit_packed) {
        size_t num_bytes = (num_samples + 7) / 8;
        if (out.is_none()) {
            // Allocate u64 aligned memory.
            void *buffer = (void *)new uint64_t[(num_bytes + 7) / 8];
            pybind11::capsule free_when_done(buffer, [](void *f) {
                delete[] reinterpret_cast<uint64_t *>(f);
            });
            out = pybind11::array_t<uint8_t>(
                {(pybind11::ssize_t)num_bytes},
                {(pybind11::ssize_t)1},
                (uint8_t *)buffer,
                free_when_done);
        } else if (!pybind11::isinstance<pybind11::array_t<uint8_t>>(out)) {
            throw std::invalid_argument("`out` wasn't `None` or a uint8 numpy array.");
        }
        auto buf = pybind11::cast<pybind11::array_t<uint8_t>>(out);
        if (buf.ndim() != 1) {
            throw std::invalid_argument("Output buffer wasn't one dimensional.");
        }
        if ((size_t)buf.shape(0) != num_bytes) {
            std::stringstream ss;
            ss << "Expected output buffer to have size " << num_bytes;
            ss << " but its size is " << buf.shape(0) << ".";
            throw std::invalid_argument(ss.str());
        }
        auto stride = buf.strides(0);
        void *start_of_data = (void *)buf.mutable_data();

        if (stride == 1) {
            generate_biased_samples_bit_packed_contiguous(
                (uint8_t *)start_of_data,
                num_bytes,
                p,
                self.rng);
        } else {
            generate_biased_samples_bit_packed_with_stride(
                (uint8_t *)start_of_data,
                stride,
                num_bytes,
                p,
                self.rng);
        }
        if (num_samples & 0b111) {
            uint8_t mask = (1 << (num_samples & 0b111)) - 1;
            buf.mutable_at(num_bytes - 1) &= mask;
        }
    } else {
        if (out.is_none()) {
            auto numpy = pybind11::module::import("numpy");
            out = numpy.attr("empty")(num_samples, numpy.attr("bool_"));
        } else if (!pybind11::isinstance<pybind11::array_t<bool>>(out)) {
            throw std::invalid_argument("`out` wasn't `None` or a bool_ numpy array.");
        }
        auto buf = pybind11::cast<pybind11::array_t<bool>>(out);
        if (buf.ndim() != 1) {
            throw std::invalid_argument("Output buffer wasn't one dimensional.");
        }
        if ((size_t)buf.shape(0) != num_samples) {
            std::stringstream ss;
            ss << "Expected output buffer to have size " << num_samples;
            ss << " but its size is " << buf.shape(0) << ".";
            throw std::invalid_argument(ss.str());
        }
        auto stride = buf.strides(0);
        void *start_of_data = (void *)buf.mutable_data();

        generate_biased_samples_bool(
            (uint8_t *)start_of_data,
            stride,
            num_samples,
            p,
            self.rng);
    }
    return out;
}

uint8_t pybind11_object_to_pauli_ixyz(const pybind11::object &obj) {
    if (pybind11::isinstance<pybind11::str>(obj)) {
        std::string_view s = pybind11::cast<std::string_view>(obj);
        if (s == "X") {
            return 1;
        } else if (s == "Y") {
            return 2;
        } else if (s == "Z") {
            return 3;
        } else if (s == "I" || s == "_") {
            return 0;
        }
    } else if (pybind11::isinstance<pybind11::int_>(obj)) {
        uint8_t v = 255;
        try {
            v = pybind11::cast<uint8_t>(obj);
        } catch (const pybind11::cast_error &) {
        }
        if (v < 4) {
            return (uint8_t)v;
        }
    }

    throw std::invalid_argument("Need pauli in ['I', 'X', 'Y', 'Z', 0, 1, 2, 3, '_'].");
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
        return pybind11::cast(FlexPauliString(self.get_frame(*instance_index)));
    }

    std::vector<FlexPauliString> result;
    for (size_t k = 0; k < self.batch_size; k++) {
        result.push_back(FlexPauliString(self.get_frame(k)));
    }
    return pybind11::cast(std::move(result));
}

pybind11::object pick_output_numpy_array(pybind11::object output_vs, bool bit_packed, bool transpose, size_t shape1, size_t shape2, const char *name) {
    auto numpy = pybind11::module::import("numpy");
    auto dtype = bit_packed ? numpy.attr("uint8") : numpy.attr("bool_");
    auto py_bool = pybind11::module::import("builtins").attr("bool");
    if (transpose) {
        std::swap(shape1, shape2);
    }
    if (bit_packed) {
        shape2 = (shape2 + 7) >> 3;
    }
    auto shape = pybind11::make_tuple(shape1, shape2);
    if (pybind11::isinstance<pybind11::bool_>(output_vs) && pybind11::bool_(false).equal(output_vs)) {
        return pybind11::none();
    } else if (pybind11::isinstance<pybind11::bool_>(output_vs) && pybind11::bool_(true).equal(output_vs)) {
        return numpy.attr("empty")(shape, dtype);
    } else if (bit_packed && pybind11::isinstance<pybind11::array_t<uint8_t>>(output_vs) && shape.equal(output_vs.attr("shape"))) {
        return output_vs;
    } else if (!bit_packed && pybind11::isinstance<pybind11::array_t<bool>>(output_vs) && shape.equal(output_vs.attr("shape"))) {
        return output_vs;
    } else {
        std::stringstream ss;
        ss << name << " wasn't set to False, True, or a numpy array with dtype=" << pybind11::str(dtype) << " and shape=" << shape;
        throw std::invalid_argument(ss.str());
    }
}

template <size_t W>
pybind11::object to_numpy(
    const FrameSimulator<W> &self,
    bool bit_packed,
    bool transpose,
    pybind11::object output_xs,
    pybind11::object output_zs,
    pybind11::object output_measure_flips,
    pybind11::object output_detector_flips,
    pybind11::object output_observable_flips) {
    output_xs = pick_output_numpy_array(output_xs, bit_packed, transpose, self.num_qubits, self.batch_size, "output_xs");
    output_zs = pick_output_numpy_array(output_zs, bit_packed, transpose, self.num_qubits, self.batch_size, "output_zs");
    output_measure_flips = pick_output_numpy_array(output_measure_flips, bit_packed, transpose, self.m_record.stored, self.batch_size, "output_measure_flips");
    output_detector_flips = pick_output_numpy_array(output_detector_flips, bit_packed, transpose, self.det_record.stored, self.batch_size, "output_detector_flips");
    output_observable_flips = pick_output_numpy_array(output_observable_flips, bit_packed, transpose, self.num_observables, self.batch_size, "output_observable_flips");

    if (!output_xs.is_none()) {
        simd_bit_table_to_numpy(
            self.x_table,
            self.num_qubits,
            self.batch_size,
            bit_packed,
            transpose,
            output_xs);
    }
    if (!output_zs.is_none()) {
        simd_bit_table_to_numpy(
            self.z_table,
            self.num_qubits,
            self.batch_size,
            bit_packed,
            transpose,
            output_zs);
    }
    if (!output_measure_flips.is_none()) {
        simd_bit_table_to_numpy(
            self.m_record.storage,
            self.m_record.stored,
            self.batch_size,
            bit_packed,
            transpose,
            output_measure_flips);
    }
    if (!output_detector_flips.is_none()) {
        simd_bit_table_to_numpy(
            self.det_record.storage,
            self.det_record.stored,
            self.batch_size,
            bit_packed,
            transpose,
            output_detector_flips);
    }
    if (!output_observable_flips.is_none()) {
        simd_bit_table_to_numpy(
            self.obs_record,
            self.num_observables,
            self.batch_size,
            bit_packed,
            transpose,
            output_observable_flips);
    }
    if (output_xs.is_none() + output_zs.is_none() + output_measure_flips.is_none() + output_detector_flips.is_none() + output_observable_flips.is_none() == 5) {
        throw std::invalid_argument("No outputs requested! Specify at least one output_*= argument.");
    }
    return pybind11::make_tuple(output_xs, output_zs, output_measure_flips, output_detector_flips, output_observable_flips);
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
            return simd_bit_table_to_numpy(
                table, num_major_exact, num_minor_exact, bit_packed, false, pybind11::none());
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
            uint8_t p = pybind11_object_to_pauli_ixyz(pauli);
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
        "to_numpy",
        &to_numpy<MAX_BITWORD_WIDTH>,
        pybind11::kw_only(),
        pybind11::arg("bit_packed") = false,
        pybind11::arg("transpose") = false,
        pybind11::arg("output_xs") = false,
        pybind11::arg("output_zs") = false,
        pybind11::arg("output_measure_flips") = false,
        pybind11::arg("output_detector_flips") = false,
        pybind11::arg("output_observable_flips") = false,
        clean_doc_string(R"DOC(
            @signature def to_numpy(self, *, bit_packed: bool = False, transpose: bool = False, output_xs: Union[bool, np.ndarray] = False, output_zs: Union[bool, np.ndarray] = False, output_measure_flips: Union[bool, np.ndarray] = False, output_detector_flips: Union[bool, np.ndarray] = False, output_observable_flips: Union[bool, np.ndarray] = False) -> Optional[Tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray, np.ndarray]]:
            Writes the simulator state into numpy arrays.

            Args:
                bit_packed: Whether or not the result is bit packed, storing 8 bits per
                    byte instead of 1 bit per byte. Bit packing always applies to
                    the second index of the result. Bits are packed in little endian
                    order (as if by `np.packbits(X, axis=1, order='little')`).
                transpose: Defaults to False. When set to False, the second index of the
                    returned array (the index affected by bit packing) is the shot index
                    (meaning the first index is the qubit index or measurement index or
                    etc). When set to True, results are transposed so that the first
                    index is the shot index.
                output_xs: Defaults to False. When set to False, the X flip data is not
                    generated and the corresponding array in the result tuple is set to
                    None. When set to True, a new array is allocated to hold the X flip
                    data and this array is returned via the result tuple. When set to
                    a numpy array, the results are written into that array (the shape and
                    dtype of the array must be exactly correct).
                output_zs: Defaults to False. When set to False, the Z flip data is not
                    generated and the corresponding array in the result tuple is set to
                    None. When set to True, a new array is allocated to hold the Z flip
                    data and this array is returned via the result tuple. When set to
                    a numpy array, the results are written into that array (the shape and
                    dtype of the array must be exactly correct).
                output_measure_flips: Defaults to False. When set to False, the measure
                    flip data is not generated and the corresponding array in the result
                    tuple is set to None. When set to True, a new array is allocated to
                    hold the measure flip data and this array is returned via the result
                    tuple. When set to a numpy array, the results are written into that
                    array (the shape and dtype of the array must be exactly correct).
                output_detector_flips: Defaults to False. When set to False, the detector
                    flip data is not generated and the corresponding array in the result
                    tuple is set to None. When set to True, a new array is allocated to
                    hold the detector flip data and this array is returned via the result
                    tuple. When set to a numpy array, the results are written into that
                    array (the shape and dtype of the array must be exactly correct).
                output_observable_flips: Defaults to False. When set to False, the obs
                    flip data is not generated and the corresponding array in the result
                    tuple is set to None. When set to True, a new array is allocated to
                    hold the obs flip data and this array is returned via the result
                    tuple. When set to a numpy array, the results are written into that
                    array (the shape and dtype of the array must be exactly correct).

            Returns:
                A tuple (xs, zs, ms, ds, os) of numpy arrays. The xs and zs arrays are
                the pauli flip data specified using XZ encoding (00=I, 10=X, 11=Y, 01=Z).
                The ms array is the measure flip data, the ds array is the detector flip
                data, and the os array is the obs flip data. The arrays default to
                `None` when the corresponding `output_*` argument was left False.

                The shape and dtype of the data depends on arguments given to the function.
                The following specifies each array's shape and dtype for each case:

                    if not transpose and not bit_packed:
                        xs.shape = (sim.batch_size, sim.num_qubits)
                        zs.shape = (sim.batch_size, sim.num_qubits)
                        ms.shape = (sim.batch_size, sim.num_measurements)
                        ds.shape = (sim.batch_size, sim.num_detectors)
                        os.shape = (sim.batch_size, sim.num_observables)
                        xs.dtype = np.bool_
                        zs.dtype = np.bool_
                        ms.dtype = np.bool_
                        ds.dtype = np.bool_
                        os.dtype = np.bool_
                    elif not transpose and bit_packed:
                        xs.shape = (sim.batch_size, math.ceil(sim.num_qubits / 8))
                        zs.shape = (sim.batch_size, math.ceil(sim.num_qubits / 8))
                        ms.shape = (sim.batch_size, math.ceil(sim.num_measurements / 8))
                        ds.shape = (sim.batch_size, math.ceil(sim.num_detectors / 8))
                        os.shape = (sim.batch_size, math.ceil(sim.num_observables / 8))
                        xs.dtype = np.uint8
                        zs.dtype = np.uint8
                        ms.dtype = np.uint8
                        ds.dtype = np.uint8
                        os.dtype = np.uint8
                    elif transpose and not bit_packed:
                        xs.shape = (sim.num_qubits, sim.batch_size)
                        zs.shape = (sim.num_qubits, sim.batch_size)
                        ms.shape = (sim.num_measurements, sim.batch_size)
                        ds.shape = (sim.num_detectors, sim.batch_size)
                        os.shape = (sim.num_observables, sim.batch_size)
                        xs.dtype = np.bool_
                        zs.dtype = np.bool_
                        ms.dtype = np.bool_
                        ds.dtype = np.bool_
                        os.dtype = np.bool_
                    elif transpose and bit_packed:
                        xs.shape = (sim.num_qubits, math.ceil(sim.batch_size / 8))
                        zs.shape = (sim.num_qubits, math.ceil(sim.batch_size / 8))
                        ms.shape = (sim.num_measurements, math.ceil(sim.batch_size / 8))
                        ds.shape = (sim.num_detectors, math.ceil(sim.batch_size / 8))
                        os.shape = (sim.num_observables, math.ceil(sim.batch_size / 8))
                        xs.dtype = np.uint8
                        zs.dtype = np.uint8
                        ms.dtype = np.uint8
                        ds.dtype = np.uint8
                        os.dtype = np.uint8

            Raises:
                ValueError:
                    All the `output_*` arguments were False, or an `output_*` argument
                    had a shape or dtype inconsistent with the requested data.

            Examples:
                >>> import stim
                >>> import numpy as np
                >>> sim = stim.FlipSimulator(batch_size=9)
                >>> sim.do(stim.Circuit('M(1) 0 1 2'))

                >>> ms_buf = np.empty(shape=(9, 1), dtype=np.uint8)
                >>> xs, zs, ms, ds, os = sim.to_numpy(
                ...     transpose=True,
                ...     bit_packed=True,
                ...     output_xs=True,
                ...     output_measure_flips=ms_buf,
                ... )
                >>> assert ms is ms_buf
                >>> xs
                array([[0],
                       [0],
                       [0],
                       [0],
                       [0],
                       [0],
                       [0],
                       [0],
                       [0]], dtype=uint8)
                >>> zs
                >>> ms
                array([[7],
                       [7],
                       [7],
                       [7],
                       [7],
                       [7],
                       [7],
                       [7],
                       [7]], dtype=uint8)
                >>> ds
                >>> os
        )DOC")
            .data());

    c.def(
        "generate_bernoulli_samples",
        &generate_bernoulli_samples<MAX_BITWORD_WIDTH>,
        pybind11::arg("num_samples"),
        pybind11::kw_only(),
        pybind11::arg("p"),
        pybind11::arg("bit_packed") = false,
        pybind11::arg("out") = pybind11::none(),
        clean_doc_string(R"DOC(
            @signature def generate_bernoulli_samples(self, num_samples: int, *, p: float, bit_packed: bool = False, out: Optional[np.ndarray] = None) -> np.ndarray:
            Uses the simulator's random number generator to produce biased coin flips.

            This method has best performance when specifying `bit_packed=True` and
            when specifying an `out=` parameter pointing to a numpy array that has
            contiguous data aligned to a 64 bit boundary. (If `out` isn't specified,
            the returned numpy array will have this property.)

            Args:
                num_samples: The number of samples to produce.
                p: The probability of each sample being True instead of False.
                bit_packed: Defaults to False (no bit packing). When True, the result
                    has type np.uint8 instead of np.bool_ and 8 samples are packed into
                    each byte as if by np.packbits(bitorder='little'). (The bit order
                    is relevant when producing a number of samples that isn't a multiple
                    of 8.)
                out: Defaults to None (allocate new). A numpy array to write the samples
                    into. Must have the correct size and dtype.

            Returns:
                A numpy array containing the samples. The shape and dtype depends on
                the bit_packed argument:

                    if not bit_packed:
                        shape = (num_samples,)
                        dtype = np.bool_
                    elif not transpose and bit_packed:
                        shape = (math.ceil(num_samples / 8),)
                        dtype = np.uint8

            Raises:
                ValueError:
                    The given `out` argument had a shape or dtype inconsistent with the
                    requested data.

            Examples:
                >>> import stim
                >>> sim = stim.FlipSimulator(batch_size=256)
                >>> r = sim.generate_bernoulli_samples(1001, p=0.25)
                >>> r.dtype
                dtype('bool')
                >>> r.shape
                (1001,)

                >>> r = sim.generate_bernoulli_samples(53, p=0.1, bit_packed=True)
                >>> r.dtype
                dtype('uint8')
                >>> r.shape
                (7,)
                >>> r[6] & 0b1110_0000  # zero'd padding bits
                np.uint8(0)

                >>> r2 = sim.generate_bernoulli_samples(53, p=0.2, bit_packed=True, out=r)
                >>> r is r2  # Check request to reuse r worked.
                True
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
                std::stringstream ss;
                ss << "Don't know how to do a '";
                ss << pybind11::repr(obj);
                ss << "'.";
                throw std::invalid_argument(ss.str());
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

    c.def(
        "broadcast_pauli_errors",
        [](FrameSimulator<MAX_BITWORD_WIDTH> &self, const pybind11::object &pauli, const pybind11::object &mask) {
            uint8_t p = pybind11_object_to_pauli_ixyz(pauli);

            if (!pybind11::isinstance<pybind11::array_t<bool>>(mask)) {
                throw std::invalid_argument("Need isinstance(mask, np.ndarray) and mask.dtype == np.bool_");
            }
            const pybind11::array_t<bool> &arr = pybind11::cast<pybind11::array_t<bool>>(mask);

            if (arr.ndim() != 2) {
                throw std::invalid_argument(
                    "Need a 2d mask (first axis is qubits, second axis is simulation instances). Need len(mask.shape) "
                    "== 2.");
            }

            pybind11::ssize_t s_mask_num_qubits = arr.shape(0);
            pybind11::ssize_t s_mask_batch_size = arr.shape(1);
            if ((uint64_t)s_mask_batch_size != self.batch_size) {
                throw std::invalid_argument("Need mask.shape[1] == flip_sim.batch_size");
            }
            if (s_mask_num_qubits > UINT32_MAX) {
                throw std::invalid_argument("Mask exceeds maximum number of simulated qubits.");
            }
            uint32_t mask_num_qubits = (uint32_t)s_mask_num_qubits;
            uint32_t mask_batch_size = (uint32_t)s_mask_batch_size;

            self.ensure_safe_to_do_circuit_with_stats(CircuitStats{.num_qubits = mask_num_qubits});
            auto u = arr.unchecked<2>();
            bool p_x = (0b0110 >> p) & 1;  // parity of 2 bit number
            bool p_z = p & 2;
            for (size_t i = 0; i < mask_num_qubits; i++) {
                for (size_t j = 0; j < mask_batch_size; j++) {
                    bool b = *u.data(i, j);
                    self.x_table[i][j] ^= b & p_x;
                    self.z_table[i][j] ^= b & p_z;
                }
            }
        },
        pybind11::kw_only(),
        pybind11::arg("pauli"),
        pybind11::arg("mask"),
        clean_doc_string(R"DOC(
            @signature def broadcast_pauli_errors(self, *, pauli: Union[str, int], mask: np.ndarray) -> None:
            Applies a pauli error to all qubits in all instances, filtered by a mask.

            Args:
                pauli: The pauli, specified as an integer or string.
                    Uses the convention 0=I, 1=X, 2=Y, 3=Z.
                    Any value from [0, 1, 2, 3, 'X', 'Y', 'Z', 'I', '_'] is allowed.
                mask: A 2d numpy array specifying where to apply errors. The first axis
                    is qubits, the second axis is simulation instances. The first axis
                    can have a length less than the current number of qubits (or more,
                    which adds qubits to the simulation). The length of the second axis
                    must match the simulator's `batch_size`. The array must satisfy

                        mask.dtype == np.bool_
                        len(mask.shape) == 2
                        mask.shape[1] == flip_sim.batch_size

                    The error is only applied to qubit q in instance k when

                        mask[q, k] == True.

            Examples:
                >>> import stim
                >>> import numpy as np
                >>> sim = stim.FlipSimulator(
                ...     batch_size=2,
                ...     num_qubits=3,
                ...     disable_stabilizer_randomization=True,
                ... )
                >>> sim.broadcast_pauli_errors(
                ...     pauli='X',
                ...     mask=np.asarray([[True, False],[False, False],[True, True]]),
                ... )
                >>> sim.peek_pauli_flips()
                [stim.PauliString("+X_X"), stim.PauliString("+__X")]

                >>> sim.broadcast_pauli_errors(
                ...     pauli='Z',
                ...     mask=np.asarray([[False, True],[False, False],[True, True]]),
                ... )
                >>> sim.peek_pauli_flips()
                [stim.PauliString("+X_Y"), stim.PauliString("+Z_Y")]

        )DOC")
            .data());

    c.def(
        "copy",
        [](const FrameSimulator<MAX_BITWORD_WIDTH> &self, bool copy_rng, pybind11::object &seed) {
            if (copy_rng && !seed.is_none()) {
                throw std::invalid_argument("seed and copy_rng are incompatible");
            }

            FrameSimulator<MAX_BITWORD_WIDTH> copy = self;
            if (!copy_rng || !seed.is_none()) {
                copy.rng = make_py_seeded_rng(seed);
            }
            return copy;
        },
        pybind11::kw_only(),
        pybind11::arg("copy_rng") = false,
        pybind11::arg("seed") = pybind11::none(),
        clean_doc_string(R"DOC(
            @signature def copy(self, *, copy_rng: bool = False, seed: Optional[int] = None) -> stim.FlipSimulator:
            Returns a simulator with the same internal state, except perhaps its prng.

            Args:
                copy_rng: Defaults to False. When False, the copy's pseudo random number
                    generator is reinitialized with a random seed instead of being a copy
                    of the original simulator's pseudo random number generator. This
                    causes the copy and the original to sample independent randomness,
                    instead of identical randomness, for future random operations. When set
                    to true, the copy will have the exact same pseudo random number
                    generator state as the original, and so will produce identical results
                    if told to do the same noisy operations. This argument is incompatible
                    with the `seed` argument.

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

            Returns:
                The copy of the simulator.

            Examples:
                >>> import stim
                >>> import numpy as np

                >>> s1 = stim.FlipSimulator(batch_size=256)
                >>> s1.set_pauli_flip('X', qubit_index=2, instance_index=3)
                >>> s2 = s1.copy()
                >>> s2 is s1
                False
                >>> s2.peek_pauli_flips() == s1.peek_pauli_flips()
                True

                >>> s1 = stim.FlipSimulator(batch_size=256)
                >>> s2 = s1.copy(copy_rng=True)
                >>> s1.do(stim.Circuit("X_ERROR(0.25) 0 \n M 0"))
                >>> s2.do(stim.Circuit("X_ERROR(0.25) 0 \n M 0"))
                >>> np.array_equal(s1.get_measurement_flips(), s2.get_measurement_flips())
                True
        )DOC")
            .data());

    c.def(
        "clear",
        [](FrameSimulator<MAX_BITWORD_WIDTH> &self) {
            self.reset_all();
        },
        clean_doc_string(R"DOC(
            Clears the simulator's state, so it can be reused for another simulation.

            This clears the measurement flip history, clears the detector flip history,
            and zeroes the observable flip state. It also resets all qubits to |0>. If
            stabilizer randomization is disabled, this zeros all pauli flip data. Otherwise
            it randomizes all pauli flips to be I or Z with equal probability.

            Behind the scenes, this doesn't free memory or resize the simulator. So,
            repeating the same simulation with calls to `clear` in between will be faster
            than allocating a new simulator each time (by avoiding re-allocations).

            Examples:
                >>> import stim
                >>> sim = stim.FlipSimulator(batch_size=256)
                >>> sim.do(stim.Circuit("M(0.1) 9"))
                >>> sim.num_qubits
                10
                >>> sim.get_measurement_flips().shape
                (1, 256)

                >>> sim.clear()
                >>> sim.num_qubits
                10
                >>> sim.get_measurement_flips().shape
                (0, 256)
        )DOC")
            .data());
}
