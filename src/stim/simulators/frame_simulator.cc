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

bool stim::should_use_streaming_instead_of_memory(uint64_t result_count) {
    return force_stream_count > 0 || result_count > 100000000;
}

// Iterates over the X and Z frame components of a pair of qubits, applying a custom FUNC to each.
//
// HACK: Templating the body function type makes inlining significantly more likely.
template <typename FUNC>
inline void for_each_target_pair(FrameSimulator &sim, const OperationData &target_data, FUNC body) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k].data;
        size_t q2 = targets[k + 1].data;
        sim.x_table[q1].for_each_word(sim.z_table[q1], sim.x_table[q2], sim.z_table[q2], body);
    }
}

FrameSimulator::FrameSimulator(size_t num_qubits, size_t batch_size, size_t max_lookback, std::mt19937_64 &rng)
    : num_qubits(num_qubits),
      batch_size(batch_size),
      x_table(num_qubits, batch_size),
      z_table(num_qubits, batch_size),
      m_record(batch_size, max_lookback),
      rng_buffer(batch_size),
      tmp_storage(batch_size),
      last_correlated_error_occurred(batch_size),
      sweep_table(0, batch_size),
      rng(rng) {
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
    }
    m_record.clear();
}

void FrameSimulator::reset_all_and_run(const Circuit &circuit) {
    reset_all();
    circuit.for_each_operation([&](const Operation &op) {
        (this->*op.gate->frame_simulator_function)(op.target_data);
    });
}

void FrameSimulator::measure_x(const OperationData &target_data) {
    m_record.reserve_noisy_space_for_results(target_data, rng);
    for (auto t : target_data.targets) {
        auto q = t.qubit_value();  // Flipping is ignored because it is accounted for in the reference sample.
        m_record.xor_record_reserved_result(z_table[q]);
        if (guarantee_anticommutation_via_frame_randomization) {
            x_table[q].randomize(x_table[q].num_bits_padded(), rng);
        }
    }
}

void FrameSimulator::measure_y(const OperationData &target_data) {
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

void FrameSimulator::measure_z(const OperationData &target_data) {
    m_record.reserve_noisy_space_for_results(target_data, rng);
    for (auto t : target_data.targets) {
        auto q = t.qubit_value();  // Flipping is ignored because it is accounted for in the reference sample.
        m_record.xor_record_reserved_result(x_table[q]);
        if (guarantee_anticommutation_via_frame_randomization) {
            z_table[q].randomize(z_table[q].num_bits_padded(), rng);
        }
    }
}
void FrameSimulator::reset_x(const OperationData &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        if (guarantee_anticommutation_via_frame_randomization) {
            x_table[q].randomize(z_table[q].num_bits_padded(), rng);
        }
        z_table[q].clear();
    }
}

void FrameSimulator::reset_y(const OperationData &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        if (guarantee_anticommutation_via_frame_randomization) {
            z_table[q].randomize(z_table[q].num_bits_padded(), rng);
        }
        x_table[q] = z_table[q];
    }
}

void FrameSimulator::reset_z(const OperationData &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        x_table[q].clear();
        if (guarantee_anticommutation_via_frame_randomization) {
            z_table[q].randomize(z_table[q].num_bits_padded(), rng);
        }
    }
}

void FrameSimulator::measure_reset_x(const OperationData &target_data) {
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

void FrameSimulator::measure_reset_y(const OperationData &target_data) {
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

void FrameSimulator::measure_reset_z(const OperationData &target_data) {
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

void FrameSimulator::I(const OperationData &target_data) {
}

PauliString FrameSimulator::get_frame(size_t sample_index) const {
    assert(sample_index < batch_size);
    PauliString result(num_qubits);
    for (size_t q = 0; q < num_qubits; q++) {
        result.xs[q] = x_table[q][sample_index];
        result.zs[q] = z_table[q][sample_index];
    }
    return result;
}

void FrameSimulator::set_frame(size_t sample_index, const PauliStringRef &new_frame) {
    assert(sample_index < batch_size);
    assert(new_frame.num_qubits == num_qubits);
    for (size_t q = 0; q < num_qubits; q++) {
        x_table[q][sample_index] = new_frame.xs[q];
        z_table[q][sample_index] = new_frame.zs[q];
    }
}

void FrameSimulator::H_XZ(const OperationData &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        x_table[q].swap_with(z_table[q]);
    }
}

void FrameSimulator::H_XY(const OperationData &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        z_table[q] ^= x_table[q];
    }
}

void FrameSimulator::H_YZ(const OperationData &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        x_table[q] ^= z_table[q];
    }
}

void FrameSimulator::C_XYZ(const OperationData &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        x_table[q] ^= z_table[q];
        z_table[q] ^= x_table[q];
    }
}

void FrameSimulator::C_ZYX(const OperationData &target_data) {
    for (auto t : target_data.targets) {
        auto q = t.data;
        z_table[q] ^= x_table[q];
        x_table[q] ^= z_table[q];
    }
}

void FrameSimulator::single_cx(uint32_t c, uint32_t t) {
    if (!((c | t) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        x_table[c].for_each_word(
            z_table[c], x_table[t], z_table[t], [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
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
    if (!((c | t) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        x_table[c].for_each_word(
            z_table[c], x_table[t], z_table[t], [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
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

void FrameSimulator::ZCX(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cx(targets[k].data, targets[k + 1].data);
    }
}

void FrameSimulator::ZCY(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cy(targets[k].data, targets[k + 1].data);
    }
}

void FrameSimulator::ZCZ(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t c = targets[k].data;
        size_t t = targets[k + 1].data;
        if (!((c | t) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
            x_table[c].for_each_word(
                z_table[c], x_table[t], z_table[t], [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
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

void FrameSimulator::SWAP(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k].data;
        size_t q2 = targets[k + 1].data;
        x_table[q1].for_each_word(
            z_table[q1], x_table[q2], z_table[q2], [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
                std::swap(z1, z2);
                std::swap(x1, x2);
            });
    }
}

void FrameSimulator::ISWAP(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
        auto dx = x1 ^ x2;
        auto t1 = z1 ^ dx;
        auto t2 = z2 ^ dx;
        z1 = t2;
        z2 = t1;
        std::swap(x1, x2);
    });
}

void FrameSimulator::SQRT_XX(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
        simd_word dz = z1 ^ z2;
        x1 ^= dz;
        x2 ^= dz;
    });
}

void FrameSimulator::SQRT_YY(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
        simd_word d = x1 ^ z1 ^ x2 ^ z2;
        x1 ^= d;
        z1 ^= d;
        x2 ^= d;
        z2 ^= d;
    });
}

void FrameSimulator::SQRT_ZZ(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
        auto dx = x1 ^ x2;
        z1 ^= dx;
        z2 ^= dx;
    });
}

void FrameSimulator::XCX(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
        x1 ^= z2;
        x2 ^= z1;
    });
}

void FrameSimulator::XCY(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
        x1 ^= x2 ^ z2;
        x2 ^= z1;
        z2 ^= z1;
    });
}

void FrameSimulator::XCZ(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cx(targets[k + 1].data, targets[k].data);
    }
}

void FrameSimulator::YCX(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
        x2 ^= x1 ^ z1;
        x1 ^= z2;
        z1 ^= z2;
    });
}

void FrameSimulator::YCY(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
        auto y1 = x1 ^ z1;
        auto y2 = x2 ^ z2;
        x1 ^= y2;
        z1 ^= y2;
        x2 ^= y1;
        z2 ^= y1;
    });
}

void FrameSimulator::YCZ(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cy(targets[k + 1].data, targets[k].data);
    }
}

void FrameSimulator::DEPOLARIZE1(const OperationData &target_data) {
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

void FrameSimulator::DEPOLARIZE2(const OperationData &target_data) {
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

void FrameSimulator::X_ERROR(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    RareErrorIterator::for_samples(target_data.args[0], targets.size() * batch_size, rng, [&](size_t s) {
        auto target_index = s / batch_size;
        auto sample_index = s % batch_size;
        auto t = targets[target_index];
        x_table[t.data][sample_index] ^= true;
    });
}

void FrameSimulator::Y_ERROR(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    RareErrorIterator::for_samples(target_data.args[0], targets.size() * batch_size, rng, [&](size_t s) {
        auto target_index = s / batch_size;
        auto sample_index = s % batch_size;
        auto t = targets[target_index];
        x_table[t.data][sample_index] ^= true;
        z_table[t.data][sample_index] ^= true;
    });
}

void FrameSimulator::Z_ERROR(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    RareErrorIterator::for_samples(target_data.args[0], targets.size() * batch_size, rng, [&](size_t s) {
        auto target_index = s / batch_size;
        auto sample_index = s % batch_size;
        auto t = targets[target_index];
        z_table[t.data][sample_index] ^= true;
    });
}

void FrameSimulator::MPP(const OperationData &target_data) {
    decompose_mpp_operation(
        target_data,
        num_qubits,
        [&](const OperationData &h_xz,
            const OperationData &h_yz,
            const OperationData &cnot,
            const OperationData &meas) {
            H_XZ(h_xz);
            H_YZ(h_yz);
            ZCX(cnot);
            measure_z(meas);
            ZCX(cnot);
            H_YZ(h_yz);
            H_XZ(h_xz);
        });
}

void FrameSimulator::PAULI_CHANNEL_1(const OperationData &target_data) {
    tmp_storage = last_correlated_error_occurred;
    perform_pauli_errors_via_correlated_errors<1>(
        target_data,
        [&]() {
            last_correlated_error_occurred.clear();
        },
        [&](const OperationData &d) {
            ELSE_CORRELATED_ERROR(d);
        });
    last_correlated_error_occurred = tmp_storage;
}

void FrameSimulator::PAULI_CHANNEL_2(const OperationData &target_data) {
    tmp_storage = last_correlated_error_occurred;
    perform_pauli_errors_via_correlated_errors<2>(
        target_data,
        [&]() {
            last_correlated_error_occurred.clear();
        },
        [&](const OperationData &d) {
            ELSE_CORRELATED_ERROR(d);
        });
    last_correlated_error_occurred = tmp_storage;
}

simd_bit_table<MAX_BITWORD_WIDTH> FrameSimulator::sample_flipped_measurements(
    const Circuit &circuit, size_t num_samples, std::mt19937_64 &rng) {
    FrameSimulator sim(circuit.count_qubits(), num_samples, SIZE_MAX, rng);
    sim.reset_all_and_run(circuit);
    return sim.m_record.storage;
}

simd_bit_table<MAX_BITWORD_WIDTH> FrameSimulator::sample(
    const Circuit &circuit, const simd_bits<MAX_BITWORD_WIDTH> &reference_sample, size_t num_samples, std::mt19937_64 &rng) {
    return transposed_vs_ref(
        num_samples, FrameSimulator::sample_flipped_measurements(circuit, num_samples, rng), reference_sample);
}

void FrameSimulator::CORRELATED_ERROR(const OperationData &target_data) {
    last_correlated_error_occurred.clear();
    ELSE_CORRELATED_ERROR(target_data);
}

void FrameSimulator::ELSE_CORRELATED_ERROR(const OperationData &target_data) {
    // Sample error locations.
    biased_randomize_bits(target_data.args[0], rng_buffer.u64, rng_buffer.u64 + ((batch_size + 63) >> 6), rng);
    if (batch_size & 63) {
        rng_buffer.u64[batch_size >> 6] &= (uint64_t{1} << (batch_size & 63)) - 1;
    }
    // Omit locations blocked by prev error, while updating prev error mask.
    simd_bits_range_ref<MAX_BITWORD_WIDTH>{rng_buffer}.for_each_word(last_correlated_error_occurred, [](simd_word &buf, simd_word &prev) {
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

void sample_out_helper(
    const Circuit &circuit,
    FrameSimulator &sim,
    simd_bits_range_ref<MAX_BITWORD_WIDTH> ref_sample,
    size_t num_shots,
    FILE *out,
    SampleFormat format) {
    sim.reset_all();

    if (should_use_streaming_instead_of_memory(std::max(num_shots, size_t{256}) * circuit.count_measurements())) {
        // Results getting quite large. Stream them (with buffering to disk) instead of trying to store them all.
        MeasureRecordBatchWriter writer(out, num_shots, format);
        circuit.for_each_operation([&](const Operation &op) {
            (sim.*op.gate->frame_simulator_function)(op.target_data);
            sim.m_record.intermediate_write_unwritten_results_to(writer, ref_sample);
        });
        sim.m_record.final_write_unwritten_results_to(writer, ref_sample);
    } else {
        // Small case. Just do everything in memory.
        circuit.for_each_operation([&](const Operation &op) {
            (sim.*op.gate->frame_simulator_function)(op.target_data);
        });
        write_table_data(
            out, num_shots, circuit.count_measurements(), ref_sample, sim.m_record.storage, format, 'M', 'M', 0);
    }
}

void FrameSimulator::sample_out(
    const Circuit &circuit,
    const simd_bits<MAX_BITWORD_WIDTH> &reference_sample,
    uint64_t num_shots,
    FILE *out,
    SampleFormat format,
    std::mt19937_64 &rng) {
    constexpr size_t GOOD_BLOCK_SIZE = 768;
    size_t num_qubits = circuit.count_qubits();
    size_t max_lookback = circuit.max_lookback();
    if (num_shots >= GOOD_BLOCK_SIZE) {
        auto sim = FrameSimulator(num_qubits, GOOD_BLOCK_SIZE, max_lookback, rng);
        while (num_shots > GOOD_BLOCK_SIZE) {
            sample_out_helper(circuit, sim, reference_sample, GOOD_BLOCK_SIZE, out, format);
            num_shots -= GOOD_BLOCK_SIZE;
        }
    }
    if (num_shots) {
        auto sim = FrameSimulator(num_qubits, num_shots, max_lookback, rng);
        sample_out_helper(circuit, sim, reference_sample, num_shots, out, format);
    }
}
