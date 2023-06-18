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

#include "stim/simulators/frame_simulator.h"

#include <algorithm>
#include <cstring>

#include "stim/circuit/gate_decomposition.h"
#include "stim/probability_util.h"
#include "stim/simulators/tableau_simulator.h"

using namespace stim;

static size_t force_stream_count = 0;
DebugForceResultStreamingRaii::DebugForceResultStreamingRaii() {
    force_stream_count++;
}
DebugForceResultStreamingRaii::~DebugForceResultStreamingRaii() {
    force_stream_count--;
}

bool stim::should_use_streaming_because_bit_count_is_too_large_to_store(uint64_t bit_count) {
    return force_stream_count > 0 || bit_count > (uint64_t{1} << 32);
}

// Iterates over the X and Z frame components of a pair of qubits, applying a custom FUNC to each.
//
// HACK: Templating the body function type makes inlining significantly more likely.
template <typename FUNC>
inline void for_each_target_pair(FrameSimulator &sim, const CircuitInstruction &target_data, FUNC body) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k].data;
        size_t q2 = targets[k + 1].data;
        sim.x_table[q1].for_each_word(sim.z_table[q1], sim.x_table[q2], sim.z_table[q2], body);
    }
}

FrameSimulator::FrameSimulator(
    CircuitStats circuit_stats, FrameSimulatorMode mode, size_t batch_size, std::mt19937_64 &rng)
    : num_qubits(0),
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
      rng(rng) {
    configure_for(circuit_stats, mode, batch_size);
}

void FrameSimulator::configure_for(CircuitStats new_circuit_stats, FrameSimulatorMode new_mode, size_t new_batch_size) {
    batch_size = new_batch_size;
    num_qubits = new_circuit_stats.num_qubits;
    keeping_detection_data = new_mode == STREAM_DETECTIONS_TO_DISK || new_mode == STORE_DETECTIONS_TO_MEMORY;
    x_table.destructive_resize(new_circuit_stats.num_qubits, batch_size);
    z_table.destructive_resize(new_circuit_stats.num_qubits, batch_size);
    m_record.destructive_resize(batch_size, new_mode == STORE_MEASUREMENTS_TO_MEMORY ? new_circuit_stats.num_measurements : new_circuit_stats.max_lookback);
    det_record.destructive_resize(batch_size, new_mode == STORE_DETECTIONS_TO_MEMORY ? new_circuit_stats.num_detectors : new_mode == STREAM_DETECTIONS_TO_DISK ? 1 : 0),
    obs_record.destructive_resize(new_mode == STORE_DETECTIONS_TO_MEMORY || new_mode == STREAM_DETECTIONS_TO_DISK ? new_circuit_stats.num_observables : 0, batch_size);
    rng_buffer.destructive_resize(batch_size);
    tmp_storage.destructive_resize(batch_size);
    last_correlated_error_occurred.destructive_resize(batch_size);
    sweep_table.destructive_resize(0, batch_size);
}

void FrameSimulator::xor_control_bit_into(uint32_t control, simd_bits_range_ref<MAX_BITWORD_WIDTH> target) {
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

void FrameSimulator::reset_all() {
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

void FrameSimulator::reset_all_and_run(const Circuit &circuit) {
    reset_all();
    circuit.for_each_operation([&](const CircuitInstruction &op) {
        do_gate(op);
    });
}

void FrameSimulator::do_MX(const CircuitInstruction &target_data) {
    m_record.reserve_noisy_space_for_results(target_data, rng);
    for (auto t : target_data.targets) {
        auto q = t.qubit_value();  // Flipping is ignored because it is accounted for in the reference sample.
        m_record.xor_record_reserved_result(z_table[q]);
        if (guarantee_anticommutation_via_frame_randomization) {
            x_table[q].randomize(x_table[q].num_bits_padded(), rng);
        }
    }
}

void FrameSimulator::do_MY(const CircuitInstruction &target_data) {
    m_record.reserve_noisy_space_for_results(target_data, rng);
    for (auto t : target_data.targets) {
        auto q = t.qubit_value();  // Flipping is ignored because it is accounted for in the reference sample.
        x_table[q] ^= z_table[q];
        m_record.xor_record_reserved_result(x_table[q]);
        if (guarantee_anticommutation_via_frame_randomization) {
            z_table[q].randomize(z_table[q].num_bits_padded(), rng);
        }
        x_table[q] ^= z_table[q];
    }
}

void FrameSimulator::do_MZ(const CircuitInstruction &target_data) {
    m_record.reserve_noisy_space_for_results(target_data, rng);
    for (auto t : target_data.targets) {
        auto q = t.qubit_value();  // Flipping is ignored because it is accounted for in the reference sample.
        m_record.xor_record_reserved_result(x_table[q]);
        if (guarantee_anticommutation_via_frame_randomization) {
            z_table[q].randomize(z_table[q].num_bits_padded(), rng);
        }
    }
}
void FrameSimulator::do_RX(const CircuitInstruction &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        if (guarantee_anticommutation_via_frame_randomization) {
            x_table[q].randomize(z_table[q].num_bits_padded(), rng);
        }
        z_table[q].clear();
    }
}
void FrameSimulator::do_DETECTOR(const CircuitInstruction &target_data) {
    if (keeping_detection_data) {
        auto r = det_record.record_zero_result_to_edit();
        for (auto t : target_data.targets) {
            uint32_t lookback = t.data & TARGET_VALUE_MASK;
            r ^= m_record.lookback(lookback);
        }
    }
}
void FrameSimulator::do_OBSERVABLE_INCLUDE(const CircuitInstruction &target_data) {
    if (keeping_detection_data) {
        auto r = obs_record[(size_t)target_data.args[0]];
        for (auto t : target_data.targets) {
            uint32_t lookback = t.data & TARGET_VALUE_MASK;
            r ^= m_record.lookback(lookback);
        }
    }
}

void FrameSimulator::do_RY(const CircuitInstruction &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        if (guarantee_anticommutation_via_frame_randomization) {
            z_table[q].randomize(z_table[q].num_bits_padded(), rng);
        }
        x_table[q] = z_table[q];
    }
}

void FrameSimulator::do_RZ(const CircuitInstruction &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        x_table[q].clear();
        if (guarantee_anticommutation_via_frame_randomization) {
            z_table[q].randomize(z_table[q].num_bits_padded(), rng);
        }
    }
}

void FrameSimulator::do_MRX(const CircuitInstruction &target_data) {
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

void FrameSimulator::do_MRY(const CircuitInstruction &target_data) {
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

void FrameSimulator::do_MRZ(const CircuitInstruction &target_data) {
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

void FrameSimulator::do_I(const CircuitInstruction &target_data) {
}

PauliString<MAX_BITWORD_WIDTH> FrameSimulator::get_frame(size_t sample_index) const {
    assert(sample_index < batch_size);
    PauliString<MAX_BITWORD_WIDTH> result(num_qubits);
    for (size_t q = 0; q < num_qubits; q++) {
        result.xs[q] = x_table[q][sample_index];
        result.zs[q] = z_table[q][sample_index];
    }
    return result;
}

void FrameSimulator::set_frame(size_t sample_index, const PauliStringRef<MAX_BITWORD_WIDTH> &new_frame) {
    assert(sample_index < batch_size);
    assert(new_frame.num_qubits == num_qubits);
    for (size_t q = 0; q < num_qubits; q++) {
        x_table[q][sample_index] = new_frame.xs[q];
        z_table[q][sample_index] = new_frame.zs[q];
    }
}

void FrameSimulator::do_H_XZ(const CircuitInstruction &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        x_table[q].swap_with(z_table[q]);
    }
}

void FrameSimulator::do_H_XY(const CircuitInstruction &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        z_table[q] ^= x_table[q];
    }
}

void FrameSimulator::do_H_YZ(const CircuitInstruction &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        x_table[q] ^= z_table[q];
    }
}

void FrameSimulator::do_C_XYZ(const CircuitInstruction &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        x_table[q] ^= z_table[q];
        z_table[q] ^= x_table[q];
    }
}

void FrameSimulator::do_C_ZYX(const CircuitInstruction &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        z_table[q] ^= x_table[q];
        x_table[q] ^= z_table[q];
    }
}

void FrameSimulator::single_cx(uint32_t c, uint32_t t) {
    c &= ~TARGET_INVERTED_BIT;
    t &= ~TARGET_INVERTED_BIT;
    if (!((c | t) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        x_table[c].for_each_word(
            z_table[c], x_table[t], z_table[t], [](
                simd_word<MAX_BITWORD_WIDTH> &x1, simd_word<MAX_BITWORD_WIDTH> &z1,
                simd_word<MAX_BITWORD_WIDTH> &x2, simd_word<MAX_BITWORD_WIDTH> &z2) {
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

void FrameSimulator::single_cy(uint32_t c, uint32_t t) {
    c &= ~TARGET_INVERTED_BIT;
    t &= ~TARGET_INVERTED_BIT;
    if (!((c | t) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        x_table[c].for_each_word(
            z_table[c], x_table[t], z_table[t], [](
                simd_word<MAX_BITWORD_WIDTH> &x1, simd_word<MAX_BITWORD_WIDTH> &z1,
                simd_word<MAX_BITWORD_WIDTH> &x2, simd_word<MAX_BITWORD_WIDTH> &z2) {
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

void FrameSimulator::do_ZCX(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cx(targets[k].data, targets[k + 1].data);
    }
}

void FrameSimulator::do_ZCY(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cy(targets[k].data, targets[k + 1].data);
    }
}

void FrameSimulator::do_ZCZ(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t c = targets[k].data;
        size_t t = targets[k + 1].data;
        c &= ~TARGET_INVERTED_BIT;
        t &= ~TARGET_INVERTED_BIT;
        if (!((c | t) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
            x_table[c].for_each_word(
                z_table[c], x_table[t], z_table[t], [](
                    simd_word<MAX_BITWORD_WIDTH> &x1, simd_word<MAX_BITWORD_WIDTH> &z1,
                    simd_word<MAX_BITWORD_WIDTH> &x2, simd_word<MAX_BITWORD_WIDTH> &z2) {
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

void FrameSimulator::do_SWAP(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k].data;
        size_t q2 = targets[k + 1].data;
        x_table[q1].for_each_word(
            z_table[q1], x_table[q2], z_table[q2], [](
                simd_word<MAX_BITWORD_WIDTH> &x1, simd_word<MAX_BITWORD_WIDTH> &z1,
                simd_word<MAX_BITWORD_WIDTH> &x2, simd_word<MAX_BITWORD_WIDTH> &z2) {
                std::swap(z1, z2);
                std::swap(x1, x2);
            });
    }
}

void FrameSimulator::do_ISWAP(const CircuitInstruction &target_data) {
    for_each_target_pair(*this, target_data, [](
        simd_word<MAX_BITWORD_WIDTH> &x1, simd_word<MAX_BITWORD_WIDTH> &z1,
        simd_word<MAX_BITWORD_WIDTH> &x2, simd_word<MAX_BITWORD_WIDTH> &z2) {
        auto dx = x1 ^ x2;
        auto t1 = z1 ^ dx;
        auto t2 = z2 ^ dx;
        z1 = t2;
        z2 = t1;
        std::swap(x1, x2);
    });
}

void FrameSimulator::do_CXSWAP(const CircuitInstruction &target_data) {
    for_each_target_pair(*this, target_data, [](
        simd_word<MAX_BITWORD_WIDTH> &x1, simd_word<MAX_BITWORD_WIDTH> &z1,
        simd_word<MAX_BITWORD_WIDTH> &x2, simd_word<MAX_BITWORD_WIDTH> &z2) {
        z2 ^= z1;
        z1 ^= z2;
        x1 ^= x2;
        x2 ^= x1;
    });
}

void FrameSimulator::do_SWAPCX(const CircuitInstruction &target_data) {
    for_each_target_pair(*this, target_data, [](
        simd_word<MAX_BITWORD_WIDTH> &x1, simd_word<MAX_BITWORD_WIDTH> &z1,
        simd_word<MAX_BITWORD_WIDTH> &x2, simd_word<MAX_BITWORD_WIDTH> &z2) {
        z1 ^= z2;
        z2 ^= z1;
        x2 ^= x1;
        x1 ^= x2;
    });
}

void FrameSimulator::do_SQRT_XX(const CircuitInstruction &target_data) {
    for_each_target_pair(*this, target_data, [](
        simd_word<MAX_BITWORD_WIDTH> &x1, simd_word<MAX_BITWORD_WIDTH> &z1,
        simd_word<MAX_BITWORD_WIDTH> &x2, simd_word<MAX_BITWORD_WIDTH> &z2) {
        auto dz = z1 ^ z2;
        x1 ^= dz;
        x2 ^= dz;
    });
}

void FrameSimulator::do_SQRT_YY(const CircuitInstruction &target_data) {
    for_each_target_pair(*this, target_data, [](
        simd_word<MAX_BITWORD_WIDTH> &x1, simd_word<MAX_BITWORD_WIDTH> &z1,
        simd_word<MAX_BITWORD_WIDTH> &x2, simd_word<MAX_BITWORD_WIDTH> &z2) {
        auto d = x1 ^ z1 ^ x2 ^ z2;
        x1 ^= d;
        z1 ^= d;
        x2 ^= d;
        z2 ^= d;
    });
}

void FrameSimulator::do_SQRT_ZZ(const CircuitInstruction &target_data) {
    for_each_target_pair(*this, target_data, [](
        simd_word<MAX_BITWORD_WIDTH> &x1, simd_word<MAX_BITWORD_WIDTH> &z1,
        simd_word<MAX_BITWORD_WIDTH> &x2, simd_word<MAX_BITWORD_WIDTH> &z2) {
        auto dx = x1 ^ x2;
        z1 ^= dx;
        z2 ^= dx;
    });
}

void FrameSimulator::do_XCX(const CircuitInstruction &target_data) {
    for_each_target_pair(*this, target_data, [](
        simd_word<MAX_BITWORD_WIDTH> &x1, simd_word<MAX_BITWORD_WIDTH> &z1,
        simd_word<MAX_BITWORD_WIDTH> &x2, simd_word<MAX_BITWORD_WIDTH> &z2) {
        x1 ^= z2;
        x2 ^= z1;
    });
}

void FrameSimulator::do_XCY(const CircuitInstruction &target_data) {
    for_each_target_pair(*this, target_data, [](
        simd_word<MAX_BITWORD_WIDTH> &x1, simd_word<MAX_BITWORD_WIDTH> &z1,
        simd_word<MAX_BITWORD_WIDTH> &x2, simd_word<MAX_BITWORD_WIDTH> &z2) {
        x1 ^= x2 ^ z2;
        x2 ^= z1;
        z2 ^= z1;
    });
}

void FrameSimulator::do_XCZ(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cx(targets[k + 1].data, targets[k].data);
    }
}

void FrameSimulator::do_YCX(const CircuitInstruction &target_data) {
    for_each_target_pair(*this, target_data, [](
        simd_word<MAX_BITWORD_WIDTH> &x1, simd_word<MAX_BITWORD_WIDTH> &z1,
        simd_word<MAX_BITWORD_WIDTH> &x2, simd_word<MAX_BITWORD_WIDTH> &z2) {
        x2 ^= x1 ^ z1;
        x1 ^= z2;
        z1 ^= z2;
    });
}

void FrameSimulator::do_YCY(const CircuitInstruction &target_data) {
    for_each_target_pair(*this, target_data, [](
        simd_word<MAX_BITWORD_WIDTH> &x1, simd_word<MAX_BITWORD_WIDTH> &z1,
        simd_word<MAX_BITWORD_WIDTH> &x2, simd_word<MAX_BITWORD_WIDTH> &z2) {
        auto y1 = x1 ^ z1;
        auto y2 = x2 ^ z2;
        x1 ^= y2;
        z1 ^= y2;
        x2 ^= y1;
        z2 ^= y1;
    });
}

void FrameSimulator::do_YCZ(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cy(targets[k + 1].data, targets[k].data);
    }
}

void FrameSimulator::do_DEPOLARIZE1(const CircuitInstruction &target_data) {
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

void FrameSimulator::do_DEPOLARIZE2(const CircuitInstruction &target_data) {
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

void FrameSimulator::do_X_ERROR(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    RareErrorIterator::for_samples(target_data.args[0], targets.size() * batch_size, rng, [&](size_t s) {
        auto target_index = s / batch_size;
        auto sample_index = s % batch_size;
        auto t = targets[target_index];
        x_table[t.data][sample_index] ^= true;
    });
}

void FrameSimulator::do_Y_ERROR(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    RareErrorIterator::for_samples(target_data.args[0], targets.size() * batch_size, rng, [&](size_t s) {
        auto target_index = s / batch_size;
        auto sample_index = s % batch_size;
        auto t = targets[target_index];
        x_table[t.data][sample_index] ^= true;
        z_table[t.data][sample_index] ^= true;
    });
}

void FrameSimulator::do_Z_ERROR(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    RareErrorIterator::for_samples(target_data.args[0], targets.size() * batch_size, rng, [&](size_t s) {
        auto target_index = s / batch_size;
        auto sample_index = s % batch_size;
        auto t = targets[target_index];
        z_table[t.data][sample_index] ^= true;
    });
}

void FrameSimulator::do_MPP(const CircuitInstruction &target_data) {
    decompose_mpp_operation(
        target_data,
        num_qubits,
        [&](const CircuitInstruction &h_xz,
            const CircuitInstruction &h_yz,
            const CircuitInstruction &cnot,
            const CircuitInstruction &meas) {
            do_H_XZ(h_xz);
            do_H_YZ(h_yz);
            do_ZCX(cnot);
            do_MZ(meas);
            do_ZCX(cnot);
            do_H_YZ(h_yz);
            do_H_XZ(h_xz);
        });
}

void FrameSimulator::do_PAULI_CHANNEL_1(const CircuitInstruction &target_data) {
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

void FrameSimulator::do_PAULI_CHANNEL_2(const CircuitInstruction &target_data) {
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

void FrameSimulator::do_CORRELATED_ERROR(const CircuitInstruction &target_data) {
    last_correlated_error_occurred.clear();
    do_ELSE_CORRELATED_ERROR(target_data);
}

void FrameSimulator::do_ELSE_CORRELATED_ERROR(const CircuitInstruction &target_data) {
    // Sample error locations.
    biased_randomize_bits(target_data.args[0], rng_buffer.u64, rng_buffer.u64 + ((batch_size + 63) >> 6), rng);
    if (batch_size & 63) {
        rng_buffer.u64[batch_size >> 6] &= (uint64_t{1} << (batch_size & 63)) - 1;
    }
    // Omit locations blocked by prev error, while updating prev error mask.
    simd_bits_range_ref<MAX_BITWORD_WIDTH>{rng_buffer}.for_each_word(
        last_correlated_error_occurred, [](
            simd_word<MAX_BITWORD_WIDTH> &buf, simd_word<MAX_BITWORD_WIDTH> &prev) {
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

void FrameSimulator::do_MXX_disjoint_controls_segment(const CircuitInstruction &inst) {
    // Transform from 2 qubit measurements to single qubit measurements.
    do_ZCX(CircuitInstruction{GateType::CX, {}, inst.targets});

    // Record measurement results.
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        do_MX(CircuitInstruction{GateType::MX, inst.args, SpanRef<const GateTarget>{&inst.targets[k]}});
    }

    // Untransform from single qubit measurements back to 2 qubit measurements.
    do_ZCX(CircuitInstruction{GateType::CX, {}, inst.targets});
}

void FrameSimulator::do_MYY_disjoint_controls_segment(const CircuitInstruction &inst) {
    // Transform from 2 qubit measurements to single qubit measurements.
    do_ZCY(CircuitInstruction{GateType::CY, {}, inst.targets});

    // Record measurement results.
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        do_MY(CircuitInstruction{GateType::MY, inst.args, SpanRef<const GateTarget>{&inst.targets[k]}});
    }

    // Untransform from single qubit measurements back to 2 qubit measurements.
    do_ZCY(CircuitInstruction{GateType::CY, {}, inst.targets});
}

void FrameSimulator::do_MZZ_disjoint_controls_segment(const CircuitInstruction &inst) {
    // Transform from 2 qubit measurements to single qubit measurements.
    do_XCZ(CircuitInstruction{GateType::XCZ, {}, inst.targets});

    // Record measurement results.
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        do_MZ(CircuitInstruction{GateType::M, inst.args, SpanRef<const GateTarget>{&inst.targets[k]}});
    }

    // Untransform from single qubit measurements back to 2 qubit measurements.
    do_XCZ(CircuitInstruction{GateType::XCZ, {}, inst.targets});
}

void FrameSimulator::do_MXX(const CircuitInstruction &inst) {
    decompose_pair_instruction_into_segments_with_single_use_controls(
        inst,
        num_qubits,
        [&](CircuitInstruction segment){
            do_MXX_disjoint_controls_segment(segment);
        });
}

void FrameSimulator::do_MYY(const CircuitInstruction &inst) {
    decompose_pair_instruction_into_segments_with_single_use_controls(
        inst,
        num_qubits,
        [&](CircuitInstruction segment){
            do_MYY_disjoint_controls_segment(segment);
        });
}

void FrameSimulator::do_MZZ(const CircuitInstruction &inst) {
    decompose_pair_instruction_into_segments_with_single_use_controls(
        inst,
        num_qubits,
        [&](CircuitInstruction segment){
            do_MZZ_disjoint_controls_segment(segment);
        });
}

void FrameSimulator::do_MPAD(const CircuitInstruction &inst) {
    simd_bits<MAX_BITWORD_WIDTH> empty(batch_size);
    assert(inst.args.empty());
    for (size_t k = 0; k < inst.targets.size(); k++) {
        m_record.record_result(empty);
    }
}

void FrameSimulator::do_gate(const CircuitInstruction &inst) {
    switch (inst.gate_type) {
        case GateType::DETECTOR:
            do_DETECTOR(inst);
            break;
        case GateType::OBSERVABLE_INCLUDE:
            do_OBSERVABLE_INCLUDE(inst);
            break;
        case GateType::TICK:
            do_I(inst);
            break;
        case GateType::QUBIT_COORDS:
            do_I(inst);
            break;
        case GateType::SHIFT_COORDS:
            do_I(inst);
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
        case GateType::H:
            do_H_XZ(inst);
            break;
        case GateType::H_XY:
            do_H_XY(inst);
            break;
        case GateType::H_YZ:
            do_H_YZ(inst);
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
        case GateType::I:
            do_I(inst);
            break;
        case GateType::X:
            do_I(inst);
            break;
        case GateType::Y:
            do_I(inst);
            break;
        case GateType::Z:
            do_I(inst);
            break;
        case GateType::C_XYZ:
            do_C_XYZ(inst);
            break;
        case GateType::C_ZYX:
            do_C_ZYX(inst);
            break;
        case GateType::SQRT_X:
            do_H_YZ(inst);
            break;
        case GateType::SQRT_X_DAG:
            do_H_YZ(inst);
            break;
        case GateType::SQRT_Y:
            do_H_XZ(inst);
            break;
        case GateType::SQRT_Y_DAG:
            do_H_XZ(inst);
            break;
        case GateType::S:
            do_H_XY(inst);
            break;
        case GateType::S_DAG:
            do_H_XY(inst);
            break;
        case GateType::SQRT_XX:
            do_SQRT_XX(inst);
            break;
        case GateType::SQRT_XX_DAG:
            do_SQRT_XX(inst);
            break;
        case GateType::SQRT_YY:
            do_SQRT_YY(inst);
            break;
        case GateType::SQRT_YY_DAG:
            do_SQRT_YY(inst);
            break;
        case GateType::SQRT_ZZ:
            do_SQRT_ZZ(inst);
            break;
        case GateType::SQRT_ZZ_DAG:
            do_SQRT_ZZ(inst);
            break;
        case GateType::SWAP:
            do_SWAP(inst);
            break;
        case GateType::ISWAP:
            do_ISWAP(inst);
            break;
        case GateType::ISWAP_DAG:
            do_ISWAP(inst);
            break;
        case GateType::CXSWAP:
            do_CXSWAP(inst);
            break;
        case GateType::SWAPCX:
            do_SWAPCX(inst);
            break;
        default:
            throw std::invalid_argument("Not implemented: " + inst.str());
    }
}
