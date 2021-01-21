#include "simulators/sim_tableau.h"
#include "program_frame.h"
#include "simulators/sim_bulk_pauli_frame.h"
#include <cstring>

PauliFrameProgram PauliFrameProgram::from_stabilizer_circuit(const std::vector<Operation> &operations) {
    constexpr uint8_t PHASE_UNITARY = 0;
    constexpr uint8_t PHASE_COLLAPSED = 1;
    constexpr uint8_t PHASE_RESET = 2;
    PauliFrameProgram resulting_simulation {};

    for (const auto &op : operations) {
        for (auto q : op.targets) {
            resulting_simulation.num_qubits = std::max(resulting_simulation.num_qubits, q + 1);
        }
        if (op.name == "M") {
            resulting_simulation.num_measurements += op.targets.size();
        }
    }

    PauliFrameProgramCycle partial_cycle {};
    std::unordered_map<size_t, uint8_t> qubit_phases {};
    std::mt19937_64 unused_rng(0);
    SimTableau sim(resulting_simulation.num_qubits, unused_rng);

    auto start_next_moment = [&](){
        resulting_simulation.cycles.push_back(partial_cycle);
        partial_cycle = {};
        qubit_phases.clear();
    };
    for (const auto &op : operations) {
        if (op.name == "I" || op.name == "X" || op.name == "Y" || op.name == "Z") {
            sim.broadcast_op(op.name, op.targets);
            continue;
        }
        if (op.name == "TICK") {
            continue;
        }
        if (op.name == "M") {
            for (auto q : op.targets) {
                if (qubit_phases[q] > PHASE_COLLAPSED) {
                    start_next_moment();
                    break;
                }
            }

            auto collapse_results = sim.inspected_collapse(op.targets);
            for (size_t k = 0; k < collapse_results.size(); k++) {
                auto q = op.targets[k];
                const auto &collapse_result = collapse_results[k];
                if (!collapse_result.indexed_words.empty()) {
                    partial_cycle.step2_collapse.emplace_back(collapse_result);
                }
                qubit_phases[q] = PHASE_COLLAPSED;
                partial_cycle.step3_measure.emplace_back(q, collapse_result.sign);
            }
        } else if (op.name == "R") {
            auto collapse_results = sim.inspected_collapse(op.targets);
            for (size_t k = 0; k < collapse_results.size(); k++) {
                auto q = op.targets[k];
                const auto &collapse_result = collapse_results[k];
                if (!collapse_result.indexed_words.empty()) {
                    partial_cycle.step2_collapse.emplace_back(collapse_result);
                }
                partial_cycle.step4_reset.push_back(q);
                qubit_phases[q] = PHASE_RESET;
            }
            sim.reset(op.targets);
        } else {
            for (auto q : op.targets) {
                if (qubit_phases[q] > PHASE_UNITARY) {
                    start_next_moment();
                    break;
                }
            }
            if (!partial_cycle.step1_unitary.empty() && partial_cycle.step1_unitary.back().name == op.name) {
                auto &back = partial_cycle.step1_unitary.back();
                back.targets.insert(back.targets.end(), op.targets.begin(), op.targets.end());
            } else {
                partial_cycle.step1_unitary.push_back(op);
            }
            sim.broadcast_op(op.name, op.targets);
        }
    }

    if (partial_cycle.step1_unitary.size()
            || partial_cycle.step2_collapse.size()
            || partial_cycle.step3_measure.size()
            || partial_cycle.step4_reset.size()) {
        resulting_simulation.cycles.push_back(partial_cycle);
    }

    return resulting_simulation;
}

std::ostream &operator<<(std::ostream &out, const PauliFrameProgram &ps) {
    for (const auto &cycle : ps.cycles) {
        for (const auto &op : cycle.step1_unitary) {
            out << op.name;
            for (auto q : op.targets) {
                out << " " << q;
            }
            out << "\n";
        }
        for (const auto &op : cycle.step2_collapse) {
            out << "RANDOM_KICKBACK " << op.destabilizer.str().substr(1) << "\n";
        }
        if (!cycle.step3_measure.empty()) {
            out << "M_DET";
            for (const auto &q : cycle.step3_measure) {
                out << " ";
                if (q.invert) {
                    out << "!";
                }
                out << q.target_qubit;
            }
            out << "\n";
        }
        if (!cycle.step4_reset.empty()) {
            out << "R";
            for (const auto &q : cycle.step4_reset) {
                out << " " << q;
            }
            out << "\n";
        }
    }
    return out;
}

std::string PauliFrameProgram::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}

std::vector<simd_bits> PauliFrameProgram::sample(size_t num_samples, std::mt19937_64 &rng) {
    SimBulkPauliFrames sim(num_qubits, num_samples, num_measurements, rng);
    sim.clear_and_run(*this);
    return sim.unpack_measurements();
}

void PauliFrameProgram::sample_out(size_t num_samples, FILE *out, SampleFormat format, std::mt19937_64 &rng) {
    constexpr size_t GOOD_BLOCK_SIZE = 1024;
    if (num_samples >= GOOD_BLOCK_SIZE) {
        auto sim = SimBulkPauliFrames(num_qubits, GOOD_BLOCK_SIZE, num_measurements, rng);
        while (num_samples > GOOD_BLOCK_SIZE) {
            sim.clear_and_run(*this);
            sim.unpack_write_measurements(out, format);
            num_samples -= GOOD_BLOCK_SIZE;
        }
    }
    if (num_samples) {
        auto sim = SimBulkPauliFrames(num_qubits, num_samples, num_measurements, rng);
        sim.clear_and_run(*this);
        sim.unpack_write_measurements(out, format);
    }
}
