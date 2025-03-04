// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <algorithm>
#include <cstring>

#include "stim/circuit/gate_decomposition.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/util_bot/probability_util.h"

namespace stim {

// Iterates over the X and Z frame components of a pair of qubits, applying a custom FUNC to each.
//
// HACK: Templating the body function type makes inlining significantly more likely.
template <typename FUNC, size_t W>
inline void for_each_target_pair(FrameSimulator<W> &sim, const CircuitInstruction &target_data, FUNC body) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k].data;
        size_t q2 = targets[k + 1].data;
        sim.x_table[q1].for_each_word(sim.z_table[q1], sim.x_table[q2], sim.z_table[q2], body);
    }
}

template <size_t W>
FrameSimulator<W>::FrameSimulator(
    CircuitStats circuit_stats, FrameSimulatorMode mode, size_t batch_size, std::mt19937_64 &&rng)
    : num_qubits(0),
      num_observables(0),
      keeping_detection_data(false),
      batch_size(0),
      x_table(0, 0),
      z_table(0, 0),
      m_record(0, 0),
      det_record(0, 0),
      obs_record(0, 0),
      rng_buffer(0),
      tmp_storage(0),
      last_correlated_error_occurred(0),
      sweep_table(0, 0),
      rng(std::move(rng)) {
    configure_for(circuit_stats, mode, batch_size);
}

template <size_t W>
void FrameSimulator<W>::configure_for(
    CircuitStats new_circuit_stats, FrameSimulatorMode new_mode, size_t new_batch_size) {
    bool storing_all_measurements = new_mode == FrameSimulatorMode::STORE_MEASUREMENTS_TO_MEMORY ||
                                    new_mode == FrameSimulatorMode::STORE_EVERYTHING_TO_MEMORY;
    bool storing_all_detections = new_mode == FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY ||
                                  new_mode == FrameSimulatorMode::STORE_EVERYTHING_TO_MEMORY;
    bool storing_any_detections = new_mode == FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY ||
                                  new_mode == FrameSimulatorMode::STORE_EVERYTHING_TO_MEMORY ||
                                  new_mode == FrameSimulatorMode::STREAM_DETECTIONS_TO_DISK;

    batch_size = new_batch_size;
    num_qubits = new_circuit_stats.num_qubits;
    keeping_detection_data = storing_any_detections;
    x_table.destructive_resize(new_circuit_stats.num_qubits, batch_size);
    z_table.destructive_resize(new_circuit_stats.num_qubits, batch_size);
    rng_buffer.destructive_resize(batch_size);
    tmp_storage.destructive_resize(batch_size);
    last_correlated_error_occurred.destructive_resize(batch_size);
    sweep_table.destructive_resize(0, batch_size);

    uint64_t num_stored_measurements = new_circuit_stats.max_lookback;
    if (storing_all_measurements) {
        num_stored_measurements = std::max(new_circuit_stats.num_measurements, num_stored_measurements);
    }
    m_record.destructive_resize(batch_size, num_stored_measurements);

    num_observables = storing_any_detections ? new_circuit_stats.num_observables : 0;
    det_record.destructive_resize(
        batch_size,
        storing_all_detections   ? new_circuit_stats.num_detectors
        : storing_any_detections ? 1
                                 : 0),
        obs_record.destructive_resize(num_observables, batch_size);
}

template <size_t W>
void FrameSimulator<W>::ensure_safe_to_do_circuit_with_stats(const CircuitStats &stats) {
    if (x_table.num_major_bits_padded() < stats.num_qubits) {
        x_table.resize(stats.num_qubits * 2, batch_size);
        z_table.resize(stats.num_qubits * 2, batch_size);
    }
    while (num_qubits < stats.num_qubits) {
        if (guarantee_anticommutation_via_frame_randomization) {
            z_table[num_qubits].randomize(batch_size, rng);
        }
        num_qubits += 1;
    }

    size_t num_used_measurements = m_record.stored + stats.num_measurements;
    if (m_record.storage.num_major_bits_padded() < num_used_measurements) {
        m_record.storage.resize(num_used_measurements * 2, batch_size);
    }

    if (keeping_detection_data) {
        size_t num_detectors = det_record.stored + stats.num_detectors;
        if (det_record.storage.num_major_bits_padded() < num_detectors) {
            det_record.storage.resize(num_detectors * 2, batch_size);
        }
        if (obs_record.num_major_bits_padded() < stats.num_observables) {
            obs_record.resize(stats.num_observables * 2, batch_size);
        }
        num_observables = std::max(stats.num_observables, num_observables);
    }
}

template <size_t W>
void FrameSimulator<W>::safe_do_circuit(const Circuit &circuit, uint64_t repetitions) {
    ensure_safe_to_do_circuit_with_stats(circuit.compute_stats().repeated(repetitions));
    for (size_t rep = 0; rep < repetitions; rep++) {
        do_circuit(circuit);
    }
}

template <size_t W>
void FrameSimulator<W>::safe_do_instruction(const CircuitInstruction &instruction) {
    ensure_safe_to_do_circuit_with_stats(instruction.compute_stats(nullptr));
    do_gate(instruction);
}

template <size_t W>
void FrameSimulator<W>::xor_control_bit_into(uint32_t control, simd_bits_range_ref<W> target) {
    uint32_t raw_control = control & ~(TARGET_RECORD_BIT | TARGET_SWEEP_BIT);
    assert(control != raw_control);
    if (control & TARGET_RECORD_BIT) {
        target ^= m_record.lookback(raw_control);
    } else {
        if (raw_control < sweep_table.num_major_bits_padded()) {
            target ^= sweep_table[raw_control];
        }
    }
}

template <size_t W>
void FrameSimulator<W>::reset_all() {
    x_table.clear();
    if (guarantee_anticommutation_via_frame_randomization) {
        z_table.data.randomize(z_table.data.num_bits_padded(), rng);
    } else {
        z_table.clear();
    }
    m_record.clear();
    det_record.clear();
    obs_record.clear();
}

template <size_t W>
void FrameSimulator<W>::do_circuit(const Circuit &circuit) {
    circuit.for_each_operation([&](const CircuitInstruction &op) {
        do_gate(op);
    });
}

template <size_t W>
void FrameSimulator<W>::do_MX(const CircuitInstruction &inst) {
    m_record.reserve_noisy_space_for_results(inst, rng);
    for (auto t : inst.targets) {
        auto q = t.qubit_value();  // Flipping is ignored because it is accounted for in the reference sample.
        m_record.xor_record_reserved_result(z_table[q]);
        if (guarantee_anticommutation_via_frame_randomization) {
            x_table[q].randomize(x_table[q].num_bits_padded(), rng);
        }
    }
}

template <size_t W>
void FrameSimulator<W>::do_MY(const CircuitInstruction &inst) {
    m_record.reserve_noisy_space_for_results(inst, rng);
    for (auto t : inst.targets) {
        auto q = t.qubit_value();  // Flipping is ignored because it is accounted for in the reference sample.
        x_table[q] ^= z_table[q];
        m_record.xor_record_reserved_result(x_table[q]);
        if (guarantee_anticommutation_via_frame_randomization) {
            z_table[q].randomize(z_table[q].num_bits_padded(), rng);
        }
        x_table[q] ^= z_table[q];
    }
}

template <size_t W>
void FrameSimulator<W>::do_MZ(const CircuitInstruction &inst) {
    m_record.reserve_noisy_space_for_results(inst, rng);
    for (auto t : inst.targets) {
        auto q = t.qubit_value();  // Flipping is ignored because it is accounted for in the reference sample.
        m_record.xor_record_reserved_result(x_table[q]);
        if (guarantee_anticommutation_via_frame_randomization) {
            z_table[q].randomize(z_table[q].num_bits_padded(), rng);
        }
    }
}

template <size_t W>
void FrameSimulator<W>::do_RX(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        if (guarantee_anticommutation_via_frame_randomization) {
            x_table[q].randomize(z_table[q].num_bits_padded(), rng);
        }
        z_table[q].clear();
    }
}

template <size_t W>
void FrameSimulator<W>::do_DETECTOR(const CircuitInstruction &inst) {
    if (keeping_detection_data) {
        auto r = det_record.record_zero_result_to_edit();
        for (auto t : inst.targets) {
            uint32_t lookback = t.data & TARGET_VALUE_MASK;
            r ^= m_record.lookback(lookback);
        }
    }
}

template <size_t W>
void FrameSimulator<W>::do_OBSERVABLE_INCLUDE(const CircuitInstruction &inst) {
    if (keeping_detection_data) {
        auto r = obs_record[(size_t)inst.args[0]];
        for (auto t : inst.targets) {
            if (t.is_measurement_record_target()) {
                uint32_t lookback = t.data & TARGET_VALUE_MASK;
                r ^= m_record.lookback(lookback);
            } else if (t.is_pauli_target()) {
                if (t.data & TARGET_PAULI_X_BIT) {
                    r ^= x_table[t.qubit_value()];
                }
                if (t.data & TARGET_PAULI_Z_BIT) {
                    r ^= z_table[t.qubit_value()];
                }
            } else {
                throw std::invalid_argument("Unexpected target for OBSERVABLE_INCLUDE: " + t.str());
            }
        }
    }
}

template <size_t W>
void FrameSimulator<W>::do_RY(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        if (guarantee_anticommutation_via_frame_randomization) {
            z_table[q].randomize(z_table[q].num_bits_padded(), rng);
        }
        x_table[q] = z_table[q];
    }
}

template <size_t W>
void FrameSimulator<W>::do_RZ(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        x_table[q].clear();
        if (guarantee_anticommutation_via_frame_randomization) {
            z_table[q].randomize(z_table[q].num_bits_padded(), rng);
        }
    }
}

template <size_t W>
void FrameSimulator<W>::do_MRX(const CircuitInstruction &target_data) {
    // Note: Caution when implementing this. Can't group the resets. because the same qubit target may appear twice.
    m_record.reserve_noisy_space_for_results(target_data, rng);
    for (auto t : target_data.targets) {
        auto q = t.qubit_value();  // Flipping is ignored because it is accounted for in the reference sample.
        m_record.xor_record_reserved_result(z_table[q]);
        z_table[q].clear();
        if (guarantee_anticommutation_via_frame_randomization) {
            x_table[q].randomize(x_table[q].num_bits_padded(), rng);
        }
    }
}

template <size_t W>
void FrameSimulator<W>::do_MRY(const CircuitInstruction &target_data) {
    // Note: Caution when implementing this. Can't group the resets. because the same qubit target may appear twice.
    m_record.reserve_noisy_space_for_results(target_data, rng);
    for (auto t : target_data.targets) {
        auto q = t.qubit_value();  // Flipping is ignored because it is accounted for in the reference sample.
        x_table[q] ^= z_table[q];
        m_record.xor_record_reserved_result(x_table[q]);
        if (guarantee_anticommutation_via_frame_randomization) {
            z_table[q].randomize(z_table[q].num_bits_padded(), rng);
        }
        x_table[q] = z_table[q];
    }
}

template <size_t W>
void FrameSimulator<W>::do_MRZ(const CircuitInstruction &target_data) {
    // Note: Caution when implementing this. Can't group the resets. because the same qubit target may appear twice.
    m_record.reserve_noisy_space_for_results(target_data, rng);
    for (auto t : target_data.targets) {
        auto q = t.qubit_value();  // Flipping is ignored because it is accounted for in the reference sample.
        m_record.xor_record_reserved_result(x_table[q]);
        x_table[q].clear();
        if (guarantee_anticommutation_via_frame_randomization) {
            z_table[q].randomize(z_table[q].num_bits_padded(), rng);
        }
    }
}

template <size_t W>
void FrameSimulator<W>::do_I(const CircuitInstruction &target_data) {
}

template <size_t W>
PauliString<W> FrameSimulator<W>::get_frame(size_t sample_index) const {
    assert(sample_index < batch_size);
    PauliString<W> result(num_qubits);
    for (size_t q = 0; q < num_qubits; q++) {
        result.xs[q] = x_table[q][sample_index];
        result.zs[q] = z_table[q][sample_index];
    }
    return result;
}

template <size_t W>
void FrameSimulator<W>::set_frame(size_t sample_index, const PauliStringRef<W> &new_frame) {
    assert(sample_index < batch_size);
    assert(new_frame.num_qubits == num_qubits);
    for (size_t q = 0; q < num_qubits; q++) {
        x_table[q][sample_index] = new_frame.xs[q];
        z_table[q][sample_index] = new_frame.zs[q];
    }
}

template <size_t W>
void FrameSimulator<W>::do_H_XZ(const CircuitInstruction &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        x_table[q].swap_with(z_table[q]);
    }
}

template <size_t W>
void FrameSimulator<W>::do_H_XY(const CircuitInstruction &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        z_table[q] ^= x_table[q];
    }
}

template <size_t W>
void FrameSimulator<W>::do_H_YZ(const CircuitInstruction &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        x_table[q] ^= z_table[q];
    }
}

template <size_t W>
void FrameSimulator<W>::do_C_XYZ(const CircuitInstruction &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        x_table[q] ^= z_table[q];
        z_table[q] ^= x_table[q];
    }
}

template <size_t W>
void FrameSimulator<W>::do_C_ZYX(const CircuitInstruction &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        z_table[q] ^= x_table[q];
        x_table[q] ^= z_table[q];
    }
}

template <size_t W>
void FrameSimulator<W>::single_cx(uint32_t c, uint32_t t) {
    c &= ~TARGET_INVERTED_BIT;
    t &= ~TARGET_INVERTED_BIT;
    if (!((c | t) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        x_table[c].for_each_word(
            z_table[c],
            x_table[t],
            z_table[t],
            [](simd_word<W> &x1, simd_word<W> &z1, simd_word<W> &x2, simd_word<W> &z2) {
                z1 ^= z2;
                x2 ^= x1;
            });
    } else if (t & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT)) {
        throw std::invalid_argument(
            "Controlled X had a bit (" + GateTarget{t}.str() + ") as its target, instead of its control.");
    } else {
        xor_control_bit_into(c, x_table[t]);
    }
}

template <size_t W>
void FrameSimulator<W>::single_cy(uint32_t c, uint32_t t) {
    c &= ~TARGET_INVERTED_BIT;
    t &= ~TARGET_INVERTED_BIT;
    if (!((c | t) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        x_table[c].for_each_word(
            z_table[c],
            x_table[t],
            z_table[t],
            [](simd_word<W> &x1, simd_word<W> &z1, simd_word<W> &x2, simd_word<W> &z2) {
                z1 ^= x2 ^ z2;
                z2 ^= x1;
                x2 ^= x1;
            });
    } else if (t & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT)) {
        throw std::invalid_argument(
            "Controlled Y had a bit (" + GateTarget{t}.str() + ") as its target, instead of its control.");
    } else {
        xor_control_bit_into(c, x_table[t]);
        xor_control_bit_into(c, z_table[t]);
    }
}

template <size_t W>
void FrameSimulator<W>::do_ZCX(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cx(targets[k].data, targets[k + 1].data);
    }
}

template <size_t W>
void FrameSimulator<W>::do_ZCY(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cy(targets[k].data, targets[k + 1].data);
    }
}

template <size_t W>
void FrameSimulator<W>::do_ZCZ(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t c = targets[k].data;
        size_t t = targets[k + 1].data;
        c &= ~TARGET_INVERTED_BIT;
        t &= ~TARGET_INVERTED_BIT;
        if (!((c | t) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
            x_table[c].for_each_word(
                z_table[c],
                x_table[t],
                z_table[t],
                [](simd_word<W> &x1, simd_word<W> &z1, simd_word<W> &x2, simd_word<W> &z2) {
                    z1 ^= x2;
                    z2 ^= x1;
                });
        } else if (!(t & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
            xor_control_bit_into(c, z_table[t]);
        } else if (!(c & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
            xor_control_bit_into(t, z_table[c]);
        } else {
            // Both targets are bits. No effect.
        }
    }
}

template <size_t W>
void FrameSimulator<W>::do_SWAP(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k].data;
        size_t q2 = targets[k + 1].data;
        x_table[q1].for_each_word(
            z_table[q1],
            x_table[q2],
            z_table[q2],
            [](simd_word<W> &x1, simd_word<W> &z1, simd_word<W> &x2, simd_word<W> &z2) {
                std::swap(z1, z2);
                std::swap(x1, x2);
            });
    }
}

template <size_t W>
void FrameSimulator<W>::do_ISWAP(const CircuitInstruction &target_data) {
    for_each_target_pair(
        *this, target_data, [](simd_word<W> &x1, simd_word<W> &z1, simd_word<W> &x2, simd_word<W> &z2) {
            auto dx = x1 ^ x2;
            auto t1 = z1 ^ dx;
            auto t2 = z2 ^ dx;
            z1 = t2;
            z2 = t1;
            std::swap(x1, x2);
        });
}

template <size_t W>
void FrameSimulator<W>::do_CXSWAP(const CircuitInstruction &target_data) {
    for_each_target_pair(
        *this, target_data, [](simd_word<W> &x1, simd_word<W> &z1, simd_word<W> &x2, simd_word<W> &z2) {
            z2 ^= z1;
            z1 ^= z2;
            x1 ^= x2;
            x2 ^= x1;
        });
}

template <size_t W>
void FrameSimulator<W>::do_CZSWAP(const CircuitInstruction &target_data) {
    for_each_target_pair(
        *this, target_data, [](simd_word<W> &x1, simd_word<W> &z1, simd_word<W> &x2, simd_word<W> &z2) {
            std::swap(z1, z2);
            std::swap(x1, x2);
            z1 ^= x2;
            z2 ^= x1;
        });
}

template <size_t W>
void FrameSimulator<W>::do_SWAPCX(const CircuitInstruction &target_data) {
    for_each_target_pair(
        *this, target_data, [](simd_word<W> &x1, simd_word<W> &z1, simd_word<W> &x2, simd_word<W> &z2) {
            z1 ^= z2;
            z2 ^= z1;
            x2 ^= x1;
            x1 ^= x2;
        });
}

template <size_t W>
void FrameSimulator<W>::do_SQRT_XX(const CircuitInstruction &target_data) {
    for_each_target_pair(
        *this, target_data, [](simd_word<W> &x1, simd_word<W> &z1, simd_word<W> &x2, simd_word<W> &z2) {
            auto dz = z1 ^ z2;
            x1 ^= dz;
            x2 ^= dz;
        });
}

template <size_t W>
void FrameSimulator<W>::do_SQRT_YY(const CircuitInstruction &target_data) {
    for_each_target_pair(
        *this, target_data, [](simd_word<W> &x1, simd_word<W> &z1, simd_word<W> &x2, simd_word<W> &z2) {
            auto d = x1 ^ z1 ^ x2 ^ z2;
            x1 ^= d;
            z1 ^= d;
            x2 ^= d;
            z2 ^= d;
        });
}

template <size_t W>
void FrameSimulator<W>::do_SQRT_ZZ(const CircuitInstruction &target_data) {
    for_each_target_pair(
        *this, target_data, [](simd_word<W> &x1, simd_word<W> &z1, simd_word<W> &x2, simd_word<W> &z2) {
            auto dx = x1 ^ x2;
            z1 ^= dx;
            z2 ^= dx;
        });
}

template <size_t W>
void FrameSimulator<W>::do_XCX(const CircuitInstruction &target_data) {
    for_each_target_pair(
        *this, target_data, [](simd_word<W> &x1, simd_word<W> &z1, simd_word<W> &x2, simd_word<W> &z2) {
            x1 ^= z2;
            x2 ^= z1;
        });
}

template <size_t W>
void FrameSimulator<W>::do_XCY(const CircuitInstruction &target_data) {
    for_each_target_pair(
        *this, target_data, [](simd_word<W> &x1, simd_word<W> &z1, simd_word<W> &x2, simd_word<W> &z2) {
            x1 ^= x2 ^ z2;
            x2 ^= z1;
            z2 ^= z1;
        });
}

template <size_t W>
void FrameSimulator<W>::do_XCZ(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cx(targets[k + 1].data, targets[k].data);
    }
}

template <size_t W>
void FrameSimulator<W>::do_YCX(const CircuitInstruction &target_data) {
    for_each_target_pair(
        *this, target_data, [](simd_word<W> &x1, simd_word<W> &z1, simd_word<W> &x2, simd_word<W> &z2) {
            x2 ^= x1 ^ z1;
            x1 ^= z2;
            z1 ^= z2;
        });
}

template <size_t W>
void FrameSimulator<W>::do_YCY(const CircuitInstruction &target_data) {
    for_each_target_pair(
        *this, target_data, [](simd_word<W> &x1, simd_word<W> &z1, simd_word<W> &x2, simd_word<W> &z2) {
            auto y1 = x1 ^ z1;
            auto y2 = x2 ^ z2;
            x1 ^= y2;
            z1 ^= y2;
            x2 ^= y1;
            z2 ^= y1;
        });
}

template <size_t W>
void FrameSimulator<W>::do_YCZ(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cy(targets[k + 1].data, targets[k].data);
    }
}

template <size_t W>
void FrameSimulator<W>::do_DEPOLARIZE1(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    RareErrorIterator::for_samples(target_data.args[0], targets.size() * batch_size, rng, [&](size_t s) {
        auto p = 1 + (rng() % 3);
        auto target_index = s / batch_size;
        auto sample_index = s % batch_size;
        auto t = targets[target_index];
        x_table[t.data][sample_index] ^= p & 1;
        z_table[t.data][sample_index] ^= p & 2;
    });
}

template <size_t W>
void FrameSimulator<W>::do_DEPOLARIZE2(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    auto n = (targets.size() * batch_size) >> 1;
    RareErrorIterator::for_samples(target_data.args[0], n, rng, [&](size_t s) {
        auto p = 1 + (rng() % 15);
        auto target_index = (s / batch_size) << 1;
        auto sample_index = s % batch_size;
        size_t t1 = targets[target_index].data;
        size_t t2 = targets[target_index + 1].data;
        x_table[t1][sample_index] ^= (bool)(p & 1);
        z_table[t1][sample_index] ^= (bool)(p & 2);
        x_table[t2][sample_index] ^= (bool)(p & 4);
        z_table[t2][sample_index] ^= (bool)(p & 8);
    });
}

template <size_t W>
void FrameSimulator<W>::do_X_ERROR(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    RareErrorIterator::for_samples(target_data.args[0], targets.size() * batch_size, rng, [&](size_t s) {
        auto target_index = s / batch_size;
        auto sample_index = s % batch_size;
        auto t = targets[target_index];
        x_table[t.data][sample_index] ^= true;
    });
}

template <size_t W>
void FrameSimulator<W>::do_Y_ERROR(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    RareErrorIterator::for_samples(target_data.args[0], targets.size() * batch_size, rng, [&](size_t s) {
        auto target_index = s / batch_size;
        auto sample_index = s % batch_size;
        auto t = targets[target_index];
        x_table[t.data][sample_index] ^= true;
        z_table[t.data][sample_index] ^= true;
    });
}

template <size_t W>
void FrameSimulator<W>::do_Z_ERROR(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    RareErrorIterator::for_samples(target_data.args[0], targets.size() * batch_size, rng, [&](size_t s) {
        auto target_index = s / batch_size;
        auto sample_index = s % batch_size;
        auto t = targets[target_index];
        z_table[t.data][sample_index] ^= true;
    });
}

template <size_t W>
void FrameSimulator<W>::do_MPP(const CircuitInstruction &target_data) {
    decompose_mpp_operation(target_data, num_qubits, [&](const CircuitInstruction &inst) {
        safe_do_instruction(inst);
    });
}

template <size_t W>
void FrameSimulator<W>::do_SPP(const CircuitInstruction &target_data) {
    decompose_spp_or_spp_dag_operation(target_data, num_qubits, false, [&](const CircuitInstruction &inst) {
        safe_do_instruction(inst);
    });
}

template <size_t W>
void FrameSimulator<W>::do_SPP_DAG(const CircuitInstruction &target_data) {
    decompose_spp_or_spp_dag_operation(target_data, num_qubits, false, [&](const CircuitInstruction &inst) {
        safe_do_instruction(inst);
    });
}

template <size_t W>
void FrameSimulator<W>::do_PAULI_CHANNEL_1(const CircuitInstruction &target_data) {
    tmp_storage = last_correlated_error_occurred;
    perform_pauli_errors_via_correlated_errors<1>(
        target_data,
        [&]() {
            last_correlated_error_occurred.clear();
        },
        [&](const CircuitInstruction &d) {
            do_ELSE_CORRELATED_ERROR(d);
        });
    last_correlated_error_occurred = tmp_storage;
}

template <size_t W>
void FrameSimulator<W>::do_PAULI_CHANNEL_2(const CircuitInstruction &target_data) {
    tmp_storage = last_correlated_error_occurred;
    perform_pauli_errors_via_correlated_errors<2>(
        target_data,
        [&]() {
            last_correlated_error_occurred.clear();
        },
        [&](const CircuitInstruction &d) {
            do_ELSE_CORRELATED_ERROR(d);
        });
    last_correlated_error_occurred = tmp_storage;
}

template <size_t W>
void FrameSimulator<W>::do_CORRELATED_ERROR(const CircuitInstruction &target_data) {
    last_correlated_error_occurred.clear();
    do_ELSE_CORRELATED_ERROR(target_data);
}

template <size_t W>
void FrameSimulator<W>::do_ELSE_CORRELATED_ERROR(const CircuitInstruction &target_data) {
    // Sample error locations.
    biased_randomize_bits(target_data.args[0], rng_buffer.u64, rng_buffer.u64 + ((batch_size + 63) >> 6), rng);
    if (batch_size & 63) {
        rng_buffer.u64[batch_size >> 6] &= (uint64_t{1} << (batch_size & 63)) - 1;
    }
    // Omit locations blocked by prev error, while updating prev error mask.
    simd_bits_range_ref<W>{rng_buffer}.for_each_word(
        last_correlated_error_occurred, [](simd_word<W> &buf, simd_word<W> &prev) {
            buf = prev.andnot(buf);
            prev |= buf;
        });

    // Apply error to only the indicated frames.
    for (auto qxz : target_data.targets) {
        auto q = qxz.qubit_value();
        if (qxz.data & TARGET_PAULI_X_BIT) {
            x_table[q] ^= rng_buffer;
        }
        if (qxz.data & TARGET_PAULI_Z_BIT) {
            z_table[q] ^= rng_buffer;
        }
    }
}

template <size_t W>
void FrameSimulator<W>::do_HERALDED_PAULI_CHANNEL_1(const CircuitInstruction &inst) {
    auto nt = inst.targets.size();
    m_record.reserve_space_for_results(nt);
    for (size_t k = 0; k < nt; k++) {
        m_record.storage[m_record.stored + k].clear();
    }

    double hi = inst.args[0];
    double hx = inst.args[1];
    double hy = inst.args[2];
    double hz = inst.args[3];
    double t = hi + hx + hy + hz;
    std::uniform_real_distribution<double> dist(0, 1);
    RareErrorIterator::for_samples(t, nt * batch_size, rng, [&](size_t s) {
        auto shot = s % batch_size;
        auto target = s / batch_size;
        auto qubit = inst.targets[target].qubit_value();
        m_record.storage[m_record.stored + target][shot] = 1;

        double p = dist(rng) * t;
        if (p < hx) {
            x_table[qubit][shot] ^= 1;
        } else if (p < hx + hz) {
            z_table[qubit][shot] ^= 1;
        } else if (p < hx + hz + hy) {
            x_table[qubit][shot] ^= 1;
            z_table[qubit][shot] ^= 1;
        }
    });

    m_record.stored += nt;
    m_record.unwritten += nt;
}

template <size_t W>
void FrameSimulator<W>::do_HERALDED_ERASE(const CircuitInstruction &inst) {
    auto nt = inst.targets.size();
    m_record.reserve_space_for_results(nt);
    for (size_t k = 0; k < nt; k++) {
        m_record.storage[m_record.stored + k].clear();
    }

    uint64_t rng_buf = 0;
    size_t buf_size = 0;
    RareErrorIterator::for_samples(inst.args[0], nt * batch_size, rng, [&](size_t s) {
        auto shot = s % batch_size;
        auto target = s / batch_size;
        auto qubit = inst.targets[target].qubit_value();
        if (buf_size == 0) {
            rng_buf = rng();
            buf_size = 64;
        }
        x_table[qubit][shot] ^= (bool)(rng_buf & 1);
        z_table[qubit][shot] ^= (bool)(rng_buf & 2);
        m_record.storage[m_record.stored + target][shot] = 1;
        rng_buf >>= 2;
        buf_size -= 2;
    });
    m_record.stored += nt;
    m_record.unwritten += nt;
}

template <size_t W>
void FrameSimulator<W>::do_MXX_disjoint_controls_segment(const CircuitInstruction &inst) {
    // Transform from 2 qubit measurements to single qubit measurements.
    do_ZCX(CircuitInstruction{GateType::CX, {}, inst.targets, ""});

    // Record measurement results.
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        do_MX(CircuitInstruction{GateType::MX, inst.args, SpanRef<const GateTarget>{&inst.targets[k]}, ""});
    }

    // Untransform from single qubit measurements back to 2 qubit measurements.
    do_ZCX(CircuitInstruction{GateType::CX, {}, inst.targets, ""});
}

template <size_t W>
void FrameSimulator<W>::do_MYY_disjoint_controls_segment(const CircuitInstruction &inst) {
    // Transform from 2 qubit measurements to single qubit measurements.
    do_ZCY(CircuitInstruction{GateType::CY, {}, inst.targets, ""});

    // Record measurement results.
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        do_MY(CircuitInstruction{GateType::MY, inst.args, SpanRef<const GateTarget>{&inst.targets[k]}, ""});
    }

    // Untransform from single qubit measurements back to 2 qubit measurements.
    do_ZCY(CircuitInstruction{GateType::CY, {}, inst.targets, ""});
}

template <size_t W>
void FrameSimulator<W>::do_MZZ_disjoint_controls_segment(const CircuitInstruction &inst) {
    // Transform from 2 qubit measurements to single qubit measurements.
    do_XCZ(CircuitInstruction{GateType::XCZ, {}, inst.targets, ""});

    // Record measurement results.
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        do_MZ(CircuitInstruction{GateType::M, inst.args, SpanRef<const GateTarget>{&inst.targets[k]}, ""});
    }

    // Untransform from single qubit measurements back to 2 qubit measurements.
    do_XCZ(CircuitInstruction{GateType::XCZ, {}, inst.targets, ""});
}

template <size_t W>
void FrameSimulator<W>::do_MXX(const CircuitInstruction &inst) {
    decompose_pair_instruction_into_disjoint_segments(inst, num_qubits, [&](CircuitInstruction segment) {
        do_MXX_disjoint_controls_segment(segment);
    });
}

template <size_t W>
void FrameSimulator<W>::do_MYY(const CircuitInstruction &inst) {
    decompose_pair_instruction_into_disjoint_segments(inst, num_qubits, [&](CircuitInstruction segment) {
        do_MYY_disjoint_controls_segment(segment);
    });
}

template <size_t W>
void FrameSimulator<W>::do_MZZ(const CircuitInstruction &inst) {
    decompose_pair_instruction_into_disjoint_segments(inst, num_qubits, [&](CircuitInstruction segment) {
        do_MZZ_disjoint_controls_segment(segment);
    });
}

template <size_t W>
void FrameSimulator<W>::do_MPAD(const CircuitInstruction &inst) {
    m_record.reserve_noisy_space_for_results(inst, rng);
    simd_bits<W> empty(batch_size);
    for (size_t k = 0; k < inst.targets.size(); k++) {
        // 0-vs-1 is ignored because it's accounted for in the reference sample.
        m_record.xor_record_reserved_result(empty);
    }
}

template <size_t W>
void FrameSimulator<W>::do_gate(const CircuitInstruction &inst) {
    switch (inst.gate_type) {
        case GateType::DETECTOR:
            do_DETECTOR(inst);
            break;
        case GateType::OBSERVABLE_INCLUDE:
            do_OBSERVABLE_INCLUDE(inst);
            break;
        case GateType::MX:
            do_MX(inst);
            break;
        case GateType::MY:
            do_MY(inst);
            break;
        case GateType::M:
            do_MZ(inst);
            break;
        case GateType::MRX:
            do_MRX(inst);
            break;
        case GateType::MRY:
            do_MRY(inst);
            break;
        case GateType::MR:
            do_MRZ(inst);
            break;
        case GateType::RX:
            do_RX(inst);
            break;
        case GateType::RY:
            do_RY(inst);
            break;
        case GateType::R:
            do_RZ(inst);
            break;
        case GateType::MPP:
            do_MPP(inst);
            break;
        case GateType::SPP:
            do_SPP(inst);
            break;
        case GateType::SPP_DAG:
            do_SPP_DAG(inst);
            break;
        case GateType::MPAD:
            do_MPAD(inst);
            break;
        case GateType::MXX:
            do_MXX(inst);
            break;
        case GateType::MYY:
            do_MYY(inst);
            break;
        case GateType::MZZ:
            do_MZZ(inst);
            break;
        case GateType::XCX:
            do_XCX(inst);
            break;
        case GateType::XCY:
            do_XCY(inst);
            break;
        case GateType::XCZ:
            do_XCZ(inst);
            break;
        case GateType::YCX:
            do_YCX(inst);
            break;
        case GateType::YCY:
            do_YCY(inst);
            break;
        case GateType::YCZ:
            do_YCZ(inst);
            break;
        case GateType::CX:
            do_ZCX(inst);
            break;
        case GateType::CY:
            do_ZCY(inst);
            break;
        case GateType::CZ:
            do_ZCZ(inst);
            break;
        case GateType::DEPOLARIZE1:
            do_DEPOLARIZE1(inst);
            break;
        case GateType::DEPOLARIZE2:
            do_DEPOLARIZE2(inst);
            break;
        case GateType::X_ERROR:
            do_X_ERROR(inst);
            break;
        case GateType::Y_ERROR:
            do_Y_ERROR(inst);
            break;
        case GateType::Z_ERROR:
            do_Z_ERROR(inst);
            break;
        case GateType::PAULI_CHANNEL_1:
            do_PAULI_CHANNEL_1(inst);
            break;
        case GateType::PAULI_CHANNEL_2:
            do_PAULI_CHANNEL_2(inst);
            break;
        case GateType::E:
            do_CORRELATED_ERROR(inst);
            break;
        case GateType::ELSE_CORRELATED_ERROR:
            do_ELSE_CORRELATED_ERROR(inst);
            break;
        case GateType::C_XYZ:
        case GateType::C_NXYZ:
        case GateType::C_XNYZ:
        case GateType::C_XYNZ:
            do_C_XYZ(inst);
            break;
        case GateType::C_ZYX:
        case GateType::C_NZYX:
        case GateType::C_ZNYX:
        case GateType::C_ZYNX:
            do_C_ZYX(inst);
            break;
        case GateType::SWAP:
            do_SWAP(inst);
            break;
        case GateType::CXSWAP:
            do_CXSWAP(inst);
            break;
        case GateType::CZSWAP:
            do_CZSWAP(inst);
            break;
        case GateType::SWAPCX:
            do_SWAPCX(inst);
            break;
        case GateType::HERALDED_ERASE:
            do_HERALDED_ERASE(inst);
            break;
        case GateType::HERALDED_PAULI_CHANNEL_1:
            do_HERALDED_PAULI_CHANNEL_1(inst);
            break;

        case GateType::SQRT_XX:
        case GateType::SQRT_XX_DAG:
            do_SQRT_XX(inst);
            break;

        case GateType::SQRT_YY:
        case GateType::SQRT_YY_DAG:
            do_SQRT_YY(inst);
            break;

        case GateType::SQRT_ZZ:
        case GateType::SQRT_ZZ_DAG:
            do_SQRT_ZZ(inst);
            break;

        case GateType::ISWAP:
        case GateType::ISWAP_DAG:
            do_ISWAP(inst);
            break;

        case GateType::SQRT_X:
        case GateType::SQRT_X_DAG:
        case GateType::H_YZ:
        case GateType::H_NYZ:
            do_H_YZ(inst);
            break;

        case GateType::SQRT_Y:
        case GateType::SQRT_Y_DAG:
        case GateType::H:
        case GateType::H_NXZ:
            do_H_XZ(inst);
            break;

        case GateType::S:
        case GateType::S_DAG:
        case GateType::H_XY:
        case GateType::H_NXY:
            do_H_XY(inst);
            break;

        case GateType::TICK:
        case GateType::QUBIT_COORDS:
        case GateType::SHIFT_COORDS:
        case GateType::X:
        case GateType::Y:
        case GateType::Z:
        case GateType::I:
        case GateType::II:
        case GateType::I_ERROR:
        case GateType::II_ERROR:
            do_I(inst);
            break;

        default:
            throw std::invalid_argument("Not implemented in FrameSimulator<W>::do_gate: " + inst.str());
    }
}

}  // namespace stim
