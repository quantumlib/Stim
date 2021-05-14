#ifndef STIM_CIRCUIT_GEN_MAIN_H
#define STIM_CIRCUIT_GEN_MAIN_H

#include <map>
#include <stdint.h>
#include <stddef.h>
#include "../circuit/circuit.h"

namespace stim_internal {
    struct CircuitGenParameters {
        size_t rounds;
        uint32_t distance;
        std::string task;
        double after_clifford_depolarization = 0;
        double before_round_data_depolarization = 0;
        double before_measure_flip_probability = 0;
        double after_reset_flip_probability = 0;

        CircuitGenParameters(size_t rounds, uint32_t distance, std::string task);
        void append_round_transition(Circuit &circuit, const std::vector<uint32_t> &data_qubits) const;
        void append_unitary_1(Circuit &circuit, const std::string &name, const std::vector<uint32_t> targets) const;
        void append_unitary_2(Circuit &circuit, const std::string &name, const std::vector<uint32_t> targets) const;
        void append_reset(Circuit &circuit, const std::vector<uint32_t> targets) const;
        void append_measure(Circuit &circuit, const std::vector<uint32_t> targets) const;
        void append_measure_reset(Circuit &circuit, const std::vector<uint32_t> targets) const;
    };

    struct GeneratedCircuit {
        Circuit circuit;
        std::map<std::pair<uint32_t, uint32_t>, std::pair<std::string, uint32_t>> layout;
        std::string layout_str() const;
    };

    int main_generate_circuit(int argc, const char **argv, FILE *out);
}

#endif
