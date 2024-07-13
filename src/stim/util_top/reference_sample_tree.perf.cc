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
    }).goal_millis(25);
    if (total) {
        std::cerr << "data dependence";
    }
}

BENCHMARK(reference_sample_tree_nested_circuit) {
    Circuit circuit(R"CIRCUIT(
        M 0
        REPEAT 100000 {
            REPEAT 100000 {
                REPEAT 100000 {
                    X 0
                    M 0
                }
                X 0
                M 0
            }
            X 0
            M 0
        }
        X 0
        M 0
    )CIRCUIT");
    simd_bits<MAX_BITWORD_WIDTH> ref(0);
    auto total = 0;
    benchmark_go([&]() {
        auto result = ReferenceSampleTree::from_circuit_reference_sample(circuit);
        total += result.empty();
    }).goal_micros(230);
    if (total) {
        std::cerr << "data dependence";
    }
}
