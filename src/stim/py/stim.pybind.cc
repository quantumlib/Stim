#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>

#include "stim/py/base.pybind.h"
#include "stim/py/numpy.pybind.h"
#include "stim/simulators/tableau_simulator.h"

#define xstr_literal(s) str_literal(s)
#define str_literal(s) #s

using namespace stim;
using namespace stim_pybind;

PYBIND11_MODULE(STIM_PYBIND11_MODULE_NAME, m) {
    m.attr("__version__") = xstr_literal(VERSION_INFO);
    m.doc() = R"pbdoc(
        Stim: A fast stabilizer circuit library.
    )pbdoc";

    m.def(
        "test",
        []() {
            Circuit self;
            self.append_from_text(R"CIRCUIT(
                MPP X0*X1 Y0*Y1
            )CIRCUIT");
            auto ref = TableauSimulator<MAX_BITWORD_WIDTH>::reference_sample_circuit(self);
            simd_bits_range_ref<MAX_BITWORD_WIDTH> reference_sample(ref.ptr_simd, ref.num_simd_words);
            size_t num_measure = self.count_measurements();
            return simd_bits_to_numpy(reference_sample, num_measure, false);
        });
}
