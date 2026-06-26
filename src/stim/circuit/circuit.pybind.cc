#include "stim/circuit/circuit.pybind.h"

#include <fstream>

#include "stim/io/raii_file.h"
#include "stim/py/base.pybind.h"
#include "stim/py/numpy.pybind.h"
#include "stim/simulators/tableau_simulator.h"

using namespace stim;
using namespace stim_pybind;

pybind11::class_<Circuit> stim_pybind::pybind_circuit(pybind11::module &m) {
    auto c = pybind11::class_<Circuit>(
        m,
        "Circuit");

    return c;
}

void stim_pybind::pybind_circuit_methods(pybind11::module &, pybind11::class_<Circuit> &c) {
    c.def(
        pybind11::init([](std::string_view stim_program_text) {
            Circuit self;
            self.append_from_text(stim_program_text);
            return self;
        }),
        pybind11::arg("stim_program_text") = "");

    c.def(
        "reference_sample",
        [](const Circuit &self, bool bit_packed) {
            auto ref = TableauSimulator<MAX_BITWORD_WIDTH>::reference_sample_circuit(self);
            simd_bits_range_ref<MAX_BITWORD_WIDTH> reference_sample(ref.ptr_simd, ref.num_simd_words);
            size_t num_measure = self.count_measurements();
            return simd_bits_to_numpy(reference_sample, num_measure, bit_packed);
        },
        pybind11::kw_only(),
        pybind11::arg("bit_packed") = false);
}
