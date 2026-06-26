#include "stim/circuit/circuit.pybind.h"

#include <fstream>

#include "stim/circuit/circuit_instruction.pybind.h"
#include "stim/circuit/circuit_repeat_block.pybind.h"
#include "stim/dem/detector_error_model_target.pybind.h"
#include "stim/diagram/detector_slice/detector_slice_set.h"
#include "stim/gen/circuit_gen_params.h"
#include "stim/io/raii_file.h"
#include "stim/py/base.pybind.h"
#include "stim/py/compiled_detector_sampler.pybind.h"
#include "stim/py/compiled_measurement_sampler.pybind.h"
#include "stim/py/numpy.pybind.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/simulators/tableau_simulator.h"

using namespace stim;
using namespace stim_pybind;

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

    c.def(
        "reference_sample",
        [](const Circuit &self, bool bit_packed) {
            auto ref = TableauSimulator<MAX_BITWORD_WIDTH>::reference_sample_circuit(self);
            simd_bits_range_ref<MAX_BITWORD_WIDTH> reference_sample(ref.ptr_simd, ref.num_simd_words);
            size_t num_measure = self.count_measurements();
            return simd_bits_to_numpy(reference_sample, num_measure, bit_packed);
        },
        pybind11::kw_only(),
        pybind11::arg("bit_packed") = false,
        clean_doc_string(R"DOC(
            @signature def reference_sample(self, *, bit_packed: bool = False) -> np.ndarray:
            Samples the given circuit in a deterministic fashion.

            Discards all noisy operations, and biases all collapse events
            towards +Z instead of randomly +Z/-Z.

            Args:
                bit_packed: Defaults to False. Determines whether the output numpy arrays
                    use dtype=bool_ or dtype=uint8 with 8 bools packed into each byte.

            Returns:
                A numpy array containing the reference sample.

                if bit_packed:
                    shape == (math.ceil(num_measurements / 8),)
                    dtype == np.uint8
                else:
                    shape == (num_measurements,)
                    dtype == np.bool_

            Examples:
                >>> import stim
                >>> stim.Circuit('''
                ...     X 1
                ...     M 0 1
                ... ''').reference_sample()
                array([False,  True])
        )DOC")
            .data());
}
