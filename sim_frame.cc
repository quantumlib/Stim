#include "sim_tableau.h"
#include "sim_frame.h"
#include <cstring>

PauliFrameProgram PauliFrameProgram::recorded_from_tableau_sim(const std::vector<Operation> &operations) {
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
    SimTableau sim(resulting_simulation.num_qubits);

    auto start_next_moment = [&](){
        resulting_simulation.cycles.push_back(partial_cycle);
        partial_cycle = {};
        qubit_phases.clear();
    };
    for (const auto &op : operations) {
        if (op.name == "I" || op.name == "X" || op.name == "Y" || op.name == "Z") {
            sim.func_op(op.name, op.targets);
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
            sim.reset_many(op.targets);
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
            sim.func_op(op.name, op.targets);
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
            out << "RANDOM_INTO_FRAME " << op.destabilizer.str().substr(1) << "\n";
        }
        if (!cycle.step3_measure.empty()) {
            out << "REPORT";
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

SimBulkPauliFrames::SimBulkPauliFrames(size_t init_num_qubits, size_t init_num_samples256, size_t init_num_measurements) :
    num_qubits(init_num_qubits),
    num_samples256(init_num_samples256),
    num_measurements(init_num_measurements),
    x_blocks(init_num_qubits * init_num_samples256 * 256),
    z_blocks(init_num_qubits * init_num_samples256 * 256),
    recorded_results(init_num_measurements * init_num_samples256 * 256),
    rng_buffer(num_samples256 * 256),
    rng((std::random_device {})()) {
}

void SimBulkPauliFrames::H_XZ(const std::vector<size_t> &qubits) {
    for (auto q : qubits) {
        auto x = x_start(q);
        auto z = z_start(q);
        auto x_end = x + num_samples256;
        while (x != x_end) {
            std::swap(*x, *z);
            x++;
            z++;
        }
    }
}

__m256i *SimBulkPauliFrames::x_start(size_t qubit) {
    return x_blocks.u256 + qubit * num_samples256;
}

__m256i *SimBulkPauliFrames::z_start(size_t qubit) {
    return z_blocks.u256 + qubit * num_samples256;
}

void SimBulkPauliFrames::CX(const std::vector<size_t> &qubits) {
    assert((qubits.size() & 1) == 0);
    for (size_t k = 0; k < qubits.size(); k += 2) {
        size_t c = qubits[k];
        size_t t = qubits[k + 1];
        auto cx = x_start(c);
        auto cz = z_start(c);
        auto tx = x_start(t);
        auto tz = z_start(t);
        auto tx_end = tx + num_samples256;
        while (tx != tx_end) {
            *cz ^= *tz;
            *tx ^= *cx;
            tx++;
            tz++;
            cx++;
            cz++;
        }
    }
}

void SimBulkPauliFrames::clear() {
    recorded_measurements = 0;
    x_blocks.clear();
    z_blocks.clear();
    recorded_results.clear();
}

void SimBulkPauliFrames::measure_deterministic(const std::vector<PauliFrameSimMeasurement> &measurements) {
    for (auto e : measurements) {
        auto q = e.target_qubit;
        auto x = x_start(q);
        auto m = recorded_results.u256 + recorded_measurements * num_samples256;
        if (e.invert) {
            auto m_end = m + num_samples256;
            while (m != m_end) {
                *m = *x ^ _mm256_set1_epi8(-1);
                m++;
                x++;
            }
        } else {
            memcpy(m, x, num_samples256 << 5);
        }
        recorded_measurements++;
    }
}

void SimBulkPauliFrames::MUL_INTO_FRAME(const SparsePauliString &pauli_string, const __m256i *mask) {
    for (const auto &w : pauli_string.indexed_words) {
        for (size_t k2 = 0; k2 < 64; k2++) {
            if ((w.wx >> k2) & 1) {
                auto q = w.index64 * 64 + k2;
                auto x = x_start(q);
                auto x_end = x + num_samples256;
                auto m = mask;
                while (x != x_end) {
                    *x ^= *m;
                    x++;
                    m++;
                }
            }
            if ((w.wz >> k2) & 1) {
                auto q = w.index64 * 64 + k2;
                auto z = z_start(q);
                auto z_end = z + num_samples256;
                auto m = mask;
                while (z != z_end) {
                    *z ^= *m;
                    z++;
                    m++;
                }
            }
        }

    }
}

void SimBulkPauliFrames::RANDOM_INTO_FRAME(const SparsePauliString &pauli_string) {
    auto n64 = num_samples256 << 2;
    for (size_t k = 0; k < n64; k++) {
        rng_buffer.u64[k] = rng();
    }
    MUL_INTO_FRAME(pauli_string, rng_buffer.u256);
}

void SimBulkPauliFrames::reset(const std::vector<size_t> &qubits) {
    for (auto q : qubits) {
        auto x = x_start(q);
        auto z = z_start(q);
        memset(x, 0, num_samples256 << 5);
        memset(z, 0, num_samples256 << 5);
    }
}

PauliStringVal SimBulkPauliFrames::current_frame(size_t sample_index) const {
    assert(sample_index < num_samples256 * 256);
    PauliStringVal result(num_qubits);
    for (size_t q = 0; q < num_qubits; q++) {
        result.ptr().set_x_bit(q, x_blocks.get_bit(q * num_samples256 * 256 + sample_index));
        result.ptr().set_z_bit(q, z_blocks.get_bit(q * num_samples256 * 256 + sample_index));
    }
    return result;
}
