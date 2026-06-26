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
            Circuit circuit;
            circuit.append_from_text(R"CIRCUIT(
                MPP X0*X1 Y0*Y1
            )CIRCUIT");
            std::mt19937_64 irrelevant_rng(0);
            std::cerr << "ref start\n";
            auto ref = TableauSimulator<64>::sample_circuit(circuit.aliased_noiseless_circuit(), irrelevant_rng, +1);
            std::cerr << "ref done\n";
            std::cerr << "alloc start\n";
            simd_bits_range_ref<64> reference_sample(ref.ptr_simd, ref.num_simd_words);
            std::cerr << "alloc done\n";
            std::cerr << "count start\n";
            size_t num_measure = circuit.count_measurements();
            std::cerr << "count done\n";
            std::cerr << "to numpy start\n";
            auto result = simd_bits_to_numpy(reference_sample, num_measure, false);
            std::cerr << "to numpy done\n";
            return result;
        });
}
