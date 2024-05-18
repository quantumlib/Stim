#include "stim/util_top/reference_sample_tree.h"

#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/perf.perf.h"

using namespace stim;

BENCHMARK(reference_sample_tree_surface_code_d31_r1000000000) {
    CircuitGenParameters params(1000000000, 31, "rotated_memory_x");
    auto circuit = generate_surface_code_circuit(params).circuit;
    simd_bits<MAX_BITWORD_WIDTH> ref(0);
    auto total = 0;
    benchmark_go([&]() {
        auto result = ReferenceSampleTree::from_circuit_reference_sample(circuit);
        total += result.empty();
    })
        .goal_millis(25)
        .show_rate("Samples", circuit.count_measurements());
    if (total) {
        std::cerr << "data dependence";
    }
}
