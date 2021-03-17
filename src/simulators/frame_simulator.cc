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

#include "../circuit/gate_data.h"
#include "../probability_util.h"
#include "../simd/simd_util.h"
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

FrameSimulator::FrameSimulator(size_t num_qubits, size_t batch_size, size_t max_lookback, std::mt19937_64 &rng)
    : num_qubits(num_qubits),
      batch_size(batch_size),
      num_recorded_measurements(0),
      x_table(num_qubits, batch_size),
      z_table(num_qubits, batch_size),
      m_record(batch_size, max_lookback, 1),
      rng_buffer(batch_size),
      last_correlated_error_occurred(batch_size),
      rng(rng) {
}

simd_bit_table transposed_vs_ref(
    size_t num_samples_raw, const simd_bit_table &table, const simd_bits &reference_sample) {
    auto result = table.transposed();
    for (size_t s = 0; s < num_samples_raw; s++) {
        result[s].word_range_ref(0, reference_sample.num_simd_words) ^= reference_sample;
    }
    return result;
}

void write_table_data(
        FILE *out,
        size_t num_shots_raw,
        size_t num_sample_locations_raw,
        const simd_bits &reference_sample,
        const simd_bit_table &table,
        SampleFormat format,
        char dets_prefix_1,
        char dets_prefix_2,
        size_t dets_prefix_transition) {
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
        for (size_t s = 0; s < num_shots_raw; s++) {
            bool rest = false;
            result[s].for_each_set_bit([&](size_t k) {
                if (rest) {
                    putc(',', out);
                }
                rest = true;
                fprintf(out, "%lld", (unsigned long long)(k));
            });
            putc('\n', out);
        }
        return;
    }

    if (format == SAMPLE_FORMAT_DETS) {
        auto result = transposed_vs_ref(num_shots_raw, table, reference_sample);
        for (size_t s = 0; s < num_shots_raw; s++) {
            fprintf(out, "shot");
            result[s].for_each_set_bit([&](size_t k) {
                if (k < dets_prefix_transition) {
                    fprintf(out, " %c%lld", dets_prefix_1, (unsigned long long)k);
                } else {
                    fprintf(out, " %c%lld", dets_prefix_2, (unsigned long long)k - dets_prefix_transition);
                }
            });
            putc('\n', out);
        }
        return;
    }

    if (format == SAMPLE_FORMAT_R8) {
        auto result = transposed_vs_ref(num_shots_raw, table, reference_sample);
        for (size_t s = 0; s < num_shots_raw; s++) {
            size_t prev = 0;
            auto write_gap = [&](size_t k) {
                size_t gap = k - prev;
                while (gap >= 0xFF) {
                    gap -= 0xFF;
                    putc(0xFF, out);
                }
                putc((char)gap, out);
                prev = k + 1;
            };
            result[s].for_each_set_bit(write_gap);

            // Always encode a trailing 1 just past the end of the measurement results.
            write_gap(num_sample_locations_raw);
        }
        return;
    }

    throw std::out_of_range("Unrecognized output format.");
}

simd_bits_range_ref FrameSimulator::measurement_record_ref(uint32_t encoded_target) {
    assert(encoded_target & TARGET_RECORD_BIT);
    return m_record.lookback(encoded_target ^ TARGET_RECORD_BIT);
}

void FrameSimulator::reset_all() {
    num_recorded_measurements = 0;
    x_table.clear();
    z_table.data.randomize(z_table.data.num_bits_padded(), rng);
    m_record.clear();
}

void FrameSimulator::reset_all_and_run(const Circuit &circuit) {
    reset_all();
    circuit.for_each_operation([&](const Operation &op) {
        (this->*op.gate->frame_simulator_function)(op.target_data);
    });
}

void FrameSimulator::measure(const OperationData &target_data) {
    for (auto q : target_data.targets) {
        q &= TARGET_VALUE_MASK;  // Flipping is ignored because it is accounted for in the reference sample.
        z_table[q].randomize(z_table[q].num_bits_padded(), rng);
        m_record.record_result(x_table[q]);
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
    for (auto q : target_data.targets) {
        q &= TARGET_VALUE_MASK;  // Flipping is ignored because it is accounted for in the reference sample.
        m_record.record_result(x_table[q]);
        x_table[q].clear();
        z_table[q].randomize(z_table[q].num_bits_padded(), rng);
        num_recorded_measurements++;
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

void FrameSimulator::single_cx(uint32_t c, uint32_t t) {
    if (!((c | t) & TARGET_RECORD_BIT)) {
        x_table[c].for_each_word(
            z_table[c], x_table[t], z_table[t], [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
                z1 ^= z2;
                x2 ^= x1;
            });
    } else if (t & TARGET_RECORD_BIT) {
        throw std::out_of_range("Measurement record editing is not supported.");
    } else {
        x_table[t] ^= measurement_record_ref(c);
    }
}

void FrameSimulator::single_cy(uint32_t c, uint32_t t) {
    if (!((c | t) & TARGET_RECORD_BIT)) {
        x_table[c].for_each_word(
            z_table[c], x_table[t], z_table[t], [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
                z1 ^= x2 ^ z2;
                z2 ^= x1;
                x2 ^= x1;
            });
    } else if (t & TARGET_RECORD_BIT) {
        throw std::out_of_range("Measurement record editing is not supported.");
    } else {
        x_table[t].for_each_word(z_table[t], measurement_record_ref(c), [](simd_word &x, simd_word &z, simd_word &m) {
            x ^= m;
            z ^= m;
        });
    }
}

void FrameSimulator::ZCX(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cx(targets[k], targets[k + 1]);
    }
}

void FrameSimulator::ZCY(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cy(targets[k], targets[k + 1]);
    }
}

void FrameSimulator::ZCZ(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t c = targets[k];
        size_t t = targets[k + 1];
        if (!((c | t) & TARGET_RECORD_BIT)) {
            x_table[c].for_each_word(
                z_table[c], x_table[t], z_table[t], [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
                    z1 ^= x2;
                    z2 ^= x1;
                });
        } else if (c & t & TARGET_RECORD_BIT) {
            // No op.
        } else if (c & TARGET_RECORD_BIT) {
            z_table[t] ^= measurement_record_ref(c);
        } else {
            z_table[c] ^= measurement_record_ref(t);
        }
    }
}

void FrameSimulator::SWAP(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k];
        size_t q2 = targets[k + 1];
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
        single_cx(targets[k + 1], targets[k]);
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
        single_cy(targets[k + 1], targets[k]);
    }
}

void FrameSimulator::DEPOLARIZE1(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    RareErrorIterator::for_samples(target_data.arg, targets.size() * batch_size, rng, [&](size_t s) {
        auto p = 1 + (rng() % 3);
        auto target_index = s / batch_size;
        auto sample_index = s % batch_size;
        auto t = targets[target_index];
        x_table[t][sample_index] ^= p & 1;
        z_table[t][sample_index] ^= p & 2;
    });
}

void FrameSimulator::DEPOLARIZE2(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    auto n = (targets.size() * batch_size) >> 1;
    RareErrorIterator::for_samples(target_data.arg, n, rng, [&](size_t s) {
        auto p = 1 + (rng() % 15);
        auto target_index = (s / batch_size) << 1;
        auto sample_index = s % batch_size;
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
    RareErrorIterator::for_samples(target_data.arg, targets.size() * batch_size, rng, [&](size_t s) {
        auto target_index = s / batch_size;
        auto sample_index = s % batch_size;
        auto t = targets[target_index];
        x_table[t][sample_index] ^= true;
    });
}

void FrameSimulator::Y_ERROR(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    RareErrorIterator::for_samples(target_data.arg, targets.size() * batch_size, rng, [&](size_t s) {
        auto target_index = s / batch_size;
        auto sample_index = s % batch_size;
        auto t = targets[target_index];
        x_table[t][sample_index] ^= true;
        z_table[t][sample_index] ^= true;
    });
}

void FrameSimulator::Z_ERROR(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    RareErrorIterator::for_samples(target_data.arg, targets.size() * batch_size, rng, [&](size_t s) {
        auto target_index = s / batch_size;
        auto sample_index = s % batch_size;
        auto t = targets[target_index];
        z_table[t][sample_index] ^= true;
    });
}

simd_bit_table FrameSimulator::sample_flipped_measurements(
    const Circuit &circuit, size_t num_samples, std::mt19937_64 &rng) {
    FrameSimulator sim(circuit.count_qubits(), num_samples, SIZE_MAX, rng);
    sim.reset_all_and_run(circuit);
    return sim.m_record.storage;
}

simd_bit_table FrameSimulator::sample(
    const Circuit &circuit, const simd_bits &reference_sample, size_t num_samples, std::mt19937_64 &rng) {
    return transposed_vs_ref(
        num_samples, FrameSimulator::sample_flipped_measurements(circuit, num_samples, rng), reference_sample);
}

void FrameSimulator::CORRELATED_ERROR(const OperationData &target_data) {
    last_correlated_error_occurred.clear();
    ELSE_CORRELATED_ERROR(target_data);
}

void FrameSimulator::ELSE_CORRELATED_ERROR(const OperationData &target_data) {
    // Sample error locations.
    biased_randomize_bits(target_data.arg, rng_buffer.u64, rng_buffer.u64 + ((batch_size + 63) >> 6), rng);
    if (batch_size & 63) {
        rng_buffer.u64[batch_size >> 6] &= (uint64_t{1} << (batch_size & 63)) - 1;
    }
    // Omit locations blocked by prev error, while updating prev error mask.
    simd_bits_range_ref{rng_buffer}.for_each_word(last_correlated_error_occurred, [](simd_word &buf, simd_word &prev) {
        buf = prev.andnot(buf);
        prev |= buf;
    });

    // Apply error to only the indicated frames.
    for (auto qxz : target_data.targets) {
        auto q = qxz & TARGET_VALUE_MASK;
        if (qxz & TARGET_RECORD_BIT) {
            measurement_record_ref(qxz) ^= rng_buffer;
        }
        if (qxz & TARGET_PAULI_X_BIT) {
            x_table[q] ^= rng_buffer;
        }
        if (qxz & TARGET_PAULI_Z_BIT) {
            z_table[q] ^= rng_buffer;
        }
    }
}

void sample_out_helper(
    const Circuit &circuit, FrameSimulator &sim, simd_bits_range_ref ref_sample, size_t num_samples, FILE *out, SampleFormat format) {
    BatchResultWriter writer(out, num_samples, format);
    sim.reset_all();
    circuit.for_each_operation([&](const Operation &op) {
        (sim.*op.gate->frame_simulator_function)(op.target_data);
        sim.m_record.intermediate_write_unwritten_results_to(writer, ref_sample);
    });
    sim.m_record.final_write_unwritten_results_to(writer, ref_sample);
}

void FrameSimulator::sample_out(
        const Circuit &circuit, const simd_bits &reference_sample, size_t num_samples, FILE *out, SampleFormat format,
        std::mt19937_64 &rng) {
    constexpr size_t GOOD_BLOCK_SIZE = 1024;
    size_t num_qubits = circuit.count_qubits();
    size_t max_lookback = circuit.max_lookback();
    if (num_samples >= GOOD_BLOCK_SIZE) {
        auto sim = FrameSimulator(num_qubits, GOOD_BLOCK_SIZE, max_lookback, rng);
        while (num_samples > GOOD_BLOCK_SIZE) {
            sample_out_helper(circuit, sim, reference_sample, GOOD_BLOCK_SIZE, out, format);
            num_samples -= GOOD_BLOCK_SIZE;
        }
    }
    if (num_samples) {
        auto sim = FrameSimulator(num_qubits, num_samples, max_lookback, rng);
        sample_out_helper(circuit, sim, reference_sample, num_samples, out, format);
    }
}
