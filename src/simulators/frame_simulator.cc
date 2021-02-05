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

#include "frame_simulator.h"

#include <cstring>

#include "../probability_util.h"
#include "../simd/simd_util.h"
#include "gate_data.h"
#include "tableau_simulator.h"

// Iterates over the X and Z frame components of a pair of qubits, applying a custom FUNC to each.
//
// HACK: Templating the body function type makes inlining significantly more likely.
template <typename FUNC>
inline void for_each_target_pair(FrameSimulator &sim, const OperationData &target_data, FUNC body) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k];
        size_t q2 = targets[k + 1];
        sim.x_table[q1].for_each_word(sim.z_table[q1], sim.x_table[q2], sim.z_table[q2], body);
    }
}

FrameSimulator::FrameSimulator(
    size_t init_num_qubits, size_t num_samples, size_t num_measurements, std::mt19937_64 &rng)
    : num_qubits(init_num_qubits),
      num_samples_raw(num_samples),
      num_measurements_raw(num_measurements),
      num_recorded_measurements(0),
      x_table(init_num_qubits, num_samples),
      z_table(init_num_qubits, num_samples),
      m_table(num_measurements, num_samples),
      rng_buffer(num_samples),
      rng(rng) {
}

simd_bit_table transposed_vs_ref(
    size_t num_samples_raw, const simd_bit_table &table, const simd_bits &reference_sample) {
    auto result = table.transposed();
    for (size_t s = 0; s < num_samples_raw; s++) {
        result[s] ^= reference_sample;
    }
    return result;
}

void write_table_data(
    FILE *out, size_t num_shots_raw, size_t num_sample_locations_raw, const simd_bits &reference_sample,
    const simd_bit_table &table, SampleFormat format) {
    if (format == SAMPLE_FORMAT_01) {
        auto result = transposed_vs_ref(num_shots_raw, table, reference_sample);
        for (size_t s = 0; s < num_shots_raw; s++) {
            for (size_t k = 0; k < num_sample_locations_raw; k++) {
                putc('0' + result[s][k], out);
            }
            putc('\n', out);
        }
        return;
    }

    if (format == SAMPLE_FORMAT_B8) {
        auto result = transposed_vs_ref(num_shots_raw, table, reference_sample);
        auto n = (num_sample_locations_raw + 7) >> 3;
        for (size_t s = 0; s < num_shots_raw; s++) {
            fwrite(result[s].u8, 1, n, out);
        }
        return;
    }

    if (format == SAMPLE_FORMAT_PTB64) {
        auto f64 = num_shots_raw >> 6;
        for (size_t s = 0; s < f64; s++) {
            for (size_t m = 0; m < num_sample_locations_raw; m++) {
                uint64_t v = table[m].u64[s];
                if (reference_sample[m]) {
                    v = ~v;
                }
                fwrite(&v, 1, 64 >> 3, out);
            }
        }
        if (num_shots_raw & 63) {
            uint64_t mask = (uint64_t{1} << (num_shots_raw & 63)) - 1ULL;
            for (size_t m = 0; m < num_sample_locations_raw; m++) {
                uint64_t v = table[m].u64[f64];
                if (reference_sample[m]) {
                    v = ~v;
                }
                v &= mask;
                fwrite(&v, 1, 64 >> 3, out);
            }
        }
        return;
    }

    if (format == SAMPLE_FORMAT_HITS) {
        auto result = transposed_vs_ref(num_shots_raw, table, reference_sample);
        size_t wn = result.num_minor_u64_padded();
        for (size_t s = 0; s < num_shots_raw; s++) {
            uint64_t *row = result[s].u64;
            for (size_t k = 0; k < wn; k++) {
                auto v = row[k];
                if (v) {
                    for (size_t j = 0; j < 64; j++) {
                        if ((v >> j) & 1) {
                            fprintf(out, "%lld,", (unsigned long long)(k * 64 + j));
                        }
                    }
                }
            }
            putc('\n', out);
        }
        return;
    }

    throw std::out_of_range("Unrecognized output format.");
}

void FrameSimulator::write_measurements(FILE *out, const simd_bits &reference_sample, SampleFormat format) {
    write_table_data(out, num_samples_raw, num_measurements_raw, reference_sample, m_table, format);
}

void FrameSimulator::reset_all() {
    num_recorded_measurements = 0;
    x_table.clear();
    z_table.data.randomize(z_table.data.num_bits_padded(), rng);
    m_table.clear();
}

void FrameSimulator::reset_all_and_run(const Circuit &circuit) {
    assert(circuit.num_measurements == num_measurements_raw);
    reset_all();
    for (const auto &op : circuit.operations) {
        (this->*op.gate->frame_simulator_function)(op.target_data);
    }
}

void FrameSimulator::measure(const OperationData &target_data) {
    // Note: measurement flags (inversions) are ignored because they are accounted for in the reference sample.
    for (auto q : target_data.targets) {
        z_table[q].randomize(z_table[q].num_bits_padded(), rng);
        m_table[num_recorded_measurements] = x_table[q];
        num_recorded_measurements++;
    }
}

void FrameSimulator::reset(const OperationData &target_data) {
    for (auto q : target_data.targets) {
        x_table[q].clear();
        z_table[q].randomize(z_table[q].num_bits_padded(), rng);
    }
}

void FrameSimulator::measure_reset(const OperationData &target_data) {
    // Note: Caution when implementing this. Can't group the resets. because the same qubit target may appear twice.
    // Note: measurement flags (inversions) are ignored because they are accounted for in the reference sample.
    for (auto q : target_data.targets) {
        m_table[num_recorded_measurements] = x_table[q];
        x_table[q].clear();
        z_table[q].randomize(z_table[q].num_bits_padded(), rng);
        num_recorded_measurements++;
    }
}

void FrameSimulator::I(const OperationData &target_data) {
}

PauliString FrameSimulator::get_frame(size_t sample_index) const {
    assert(sample_index < num_samples_raw);
    PauliString result(num_qubits);
    for (size_t q = 0; q < num_qubits; q++) {
        result.xs[q] = x_table[q][sample_index];
        result.zs[q] = z_table[q][sample_index];
    }
    return result;
}

void FrameSimulator::set_frame(size_t sample_index, const PauliStringRef &new_frame) {
    assert(sample_index < num_samples_raw);
    assert(new_frame.num_qubits == num_qubits);
    for (size_t q = 0; q < num_qubits; q++) {
        x_table[q][sample_index] = new_frame.xs[q];
        z_table[q][sample_index] = new_frame.zs[q];
    }
}

void FrameSimulator::H_XZ(const OperationData &target_data) {
    for (auto q : target_data.targets) {
        x_table[q].swap_with(z_table[q]);
    }
}

void FrameSimulator::H_XY(const OperationData &target_data) {
    for (auto q : target_data.targets) {
        z_table[q] ^= x_table[q];
    }
}

void FrameSimulator::H_YZ(const OperationData &target_data) {
    for (auto q : target_data.targets) {
        x_table[q] ^= z_table[q];
    }
}

void FrameSimulator::ZCX(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
        z1 ^= z2;
        x2 ^= x1;
    });
}

void FrameSimulator::ZCY(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
        z1 ^= x2 ^ z2;
        z2 ^= x1;
        x2 ^= x1;
    });
}

void FrameSimulator::ZCZ(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
        z1 ^= x2;
        z2 ^= x1;
    });
}

void FrameSimulator::SWAP(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
        std::swap(z1, z2);
        std::swap(x1, x2);
    });
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
    for_each_target_pair(*this, target_data, [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
        z2 ^= z1;
        x1 ^= x2;
    });
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
    for_each_target_pair(*this, target_data, [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
        z2 ^= x1 ^ z1;
        z1 ^= x2;
        x1 ^= x2;
    });
}

void FrameSimulator::DEPOLARIZE1(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    RareErrorIterator::for_samples(target_data.arg, targets.size() * num_samples_raw, rng, [&](size_t s) {
        auto p = 1 + (rng() % 3);
        auto target_index = s / num_samples_raw;
        auto sample_index = s % num_samples_raw;
        auto t = targets[target_index];
        x_table[t][sample_index] ^= p & 1;
        z_table[t][sample_index] ^= p & 2;
    });
}

void FrameSimulator::DEPOLARIZE2(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    auto n = (targets.size() * num_samples_raw) >> 1;
    RareErrorIterator::for_samples(target_data.arg, n, rng, [&](size_t s) {
        auto p = 1 + (rng() % 15);
        auto target_index = (s / num_samples_raw) << 1;
        auto sample_index = s % num_samples_raw;
        size_t t1 = targets[target_index];
        size_t t2 = targets[target_index + 1];
        x_table[t1][sample_index] ^= p & 1;
        z_table[t1][sample_index] ^= p & 2;
        x_table[t2][sample_index] ^= p & 4;
        z_table[t2][sample_index] ^= p & 8;
    });
}

void FrameSimulator::X_ERROR(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    RareErrorIterator::for_samples(target_data.arg, targets.size() * num_samples_raw, rng, [&](size_t s) {
        auto target_index = s / num_samples_raw;
        auto sample_index = s % num_samples_raw;
        auto t = targets[target_index];
        x_table[t][sample_index] ^= true;
    });
}

void FrameSimulator::Y_ERROR(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    RareErrorIterator::for_samples(target_data.arg, targets.size() * num_samples_raw, rng, [&](size_t s) {
        auto target_index = s / num_samples_raw;
        auto sample_index = s % num_samples_raw;
        auto t = targets[target_index];
        x_table[t][sample_index] ^= true;
        z_table[t][sample_index] ^= true;
    });
}

void FrameSimulator::Z_ERROR(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    RareErrorIterator::for_samples(target_data.arg, targets.size() * num_samples_raw, rng, [&](size_t s) {
        auto target_index = s / num_samples_raw;
        auto sample_index = s % num_samples_raw;
        auto t = targets[target_index];
        z_table[t][sample_index] ^= true;
    });
}

simd_bit_table FrameSimulator::sample_flipped_measurements(
    const Circuit &circuit, size_t num_samples, std::mt19937_64 &rng) {
    FrameSimulator sim(circuit.num_qubits, num_samples, circuit.num_measurements, rng);
    sim.reset_all_and_run(circuit);
    return sim.m_table;
}

simd_bit_table FrameSimulator::sample(
    const Circuit &circuit, const simd_bits &reference_sample, size_t num_samples, std::mt19937_64 &rng) {
    return transposed_vs_ref(
        num_samples, FrameSimulator::sample_flipped_measurements(circuit, num_samples, rng), reference_sample);
}

void FrameSimulator::sample_out(
    const Circuit &circuit, const simd_bits &reference_sample, size_t num_samples, FILE *out, SampleFormat format,
    std::mt19937_64 &rng) {
    constexpr size_t GOOD_BLOCK_SIZE = 1024;
    if (num_samples >= GOOD_BLOCK_SIZE) {
        auto sim = FrameSimulator(circuit.num_qubits, GOOD_BLOCK_SIZE, circuit.num_measurements, rng);
        while (num_samples > GOOD_BLOCK_SIZE) {
            sim.reset_all_and_run(circuit);
            sim.write_measurements(out, reference_sample, format);
            num_samples -= GOOD_BLOCK_SIZE;
        }
    }
    if (num_samples) {
        auto sim = FrameSimulator(circuit.num_qubits, num_samples, circuit.num_measurements, rng);
        sim.reset_all_and_run(circuit);
        sim.write_measurements(out, reference_sample, format);
    }
}
