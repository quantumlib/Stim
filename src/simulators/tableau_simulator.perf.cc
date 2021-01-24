#include "tableau_simulator.h"

#include "../benchmark_util.h"
#include "../simulators/gate_data.h"

BENCHMARK(TableauSimulator_sample_unrotated_surface_code_d5) {
    size_t distance = 5;
    auto circuit = unrotated_surface_code_circuit(distance);
    std::mt19937_64 rng(0);
    benchmark_go([&]() { TableauSimulator::sample_circuit(circuit, rng); })
        .goal_micros(100)
        .show_rate("Layers", distance)
        .show_rate("OutBits", circuit.num_measurements);
}

BENCHMARK(TableauSimulator_sample_unrotated_surface_code_d41) {
    size_t distance = 41;
    auto circuit = unrotated_surface_code_circuit(distance);
    std::mt19937_64 rng(0);
    benchmark_go([&]() { TableauSimulator::sample_circuit(circuit, rng); })
        .goal_millis(420)
        .show_rate("Layers", distance)
        .show_rate("OutBits", circuit.num_measurements);
}
