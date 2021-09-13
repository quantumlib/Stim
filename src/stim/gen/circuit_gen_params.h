#ifndef _STIM_GEN_CIRCUIT_GEN_PARAMS_H
#define _STIM_GEN_CIRCUIT_GEN_PARAMS_H

#include <map>
#include <stddef.h>
#include <stdint.h>

#include "stim/circuit/circuit.h"

namespace stim {
struct CircuitGenParameters {
    uint64_t rounds;
    uint32_t distance;
    std::string task;
    double after_clifford_depolarization = 0;
    double before_round_data_depolarization = 0;
    double before_measure_flip_probability = 0;
    double after_reset_flip_probability = 0;

    void validate_params() const;

    CircuitGenParameters(uint64_t rounds, uint32_t distance, std::string task);
    void append_begin_round_tick(Circuit &circuit, const std::vector<uint32_t> &data_qubits) const;
    void append_unitary_1(Circuit &circuit, const std::string &name, const std::vector<uint32_t> targets) const;
    void append_unitary_2(Circuit &circuit, const std::string &name, const std::vector<uint32_t> targets) const;
    void append_reset(Circuit &circuit, const std::vector<uint32_t> targets, char basis = 'Z') const;
    void append_measure(Circuit &circuit, const std::vector<uint32_t> targets, char basis = 'Z') const;
    void append_measure_reset(Circuit &circuit, const std::vector<uint32_t> targets, char basis = 'Z') const;
};

struct GeneratedCircuit {
    Circuit circuit;
    std::map<std::pair<uint32_t, uint32_t>, std::pair<std::string, uint32_t>> layout;
    std::string hint_str;
    std::string layout_str() const;
};
}  // namespace stim

#endif
