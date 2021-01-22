#include <bit>
#include <cstring>

#include "tableau_simulator.h"
#include "frame_simulator.h"
#include "gate_data.h"
#include "../simd/simd_util.h"
#include "../probability_util.h"

// Iterates over the X and Z frame components of a pair of qubits, applying a custom BODY to each.
//
// HACK: Templating the body function type makes inlining significantly more likely.
template <typename BODY>
inline void for_each_target_pair(FrameSimulator &sim, const OperationData &target_data, BODY body) {
    const auto &targets = target_data.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k];
        size_t q2 = targets[k + 1];
        sim.x_table[q1].for_each_word(sim.z_table[q1], sim.x_table[q2], sim.z_table[q2], body);
    }
}

size_t FrameSimulator::recorded_bit_address(size_t sample_index, size_t measure_index) const {
    size_t s_high = sample_index >> 8;
    size_t s_low = sample_index & 0xFF;
    size_t m_high = measure_index >> 8;
    size_t m_low = measure_index & 0xFF;
    if (results_block_transposed) {
        std::swap(m_low, s_low);
    }
    return s_low + (m_low << 8) + (s_high << 16) + (m_high * x_table.num_simd_words_minor << 16);
}

FrameSimulator::FrameSimulator(size_t init_num_qubits, size_t num_samples, size_t num_measurements, std::mt19937_64 &rng) :
        num_qubits(init_num_qubits),
        num_samples_raw(num_samples),
        num_measurements_raw(num_measurements),
        x_table(init_num_qubits, num_samples),
        z_table(init_num_qubits, num_samples),
        m_table(num_measurements, num_samples),
        rng_buffer(num_samples),
        rng(rng) {
}

void FrameSimulator::unpack_sample_measurements_into(size_t sample_index, simd_bits &out) {
    if (!results_block_transposed) {
        do_transpose();
    }
    auto p = (__m256i *)out.ptr_simd;
    auto p2 = (__m256i *)m_table.data.ptr_simd;
    for (size_t m = 0; m < num_measurements_raw; m += 256) {
        p[m >> 8] = p2[recorded_bit_address(sample_index, m) >> 8];
    }
}

std::vector<simd_bits> FrameSimulator::unpack_measurements() {
    std::vector<simd_bits> result;
    for (size_t s = 0; s < num_samples_raw; s++) {
        result.emplace_back(num_measurements_raw);
        unpack_sample_measurements_into(s, result.back());
    }
    return result;
}

void FrameSimulator::unpack_write_measurements(FILE *out, SampleFormat format) {
    if (results_block_transposed != (format == SAMPLE_FORMAT_RAW_UNSTABLE)) {
        do_transpose();
    }

    if (format == SAMPLE_FORMAT_RAW_UNSTABLE) {
        fwrite(m_table.data.u64, 1, m_table.data.num_bits_padded() >> 3, out);
        return;
    }

    simd_bits buf(num_measurements_raw);
    for (size_t s = 0; s < num_samples_raw; s++) {
        unpack_sample_measurements_into(s, buf);

        if (format == SAMPLE_FORMAT_BINLE8) {
            static_assert(std::endian::native == std::endian::little);
            fwrite(buf.u64, 1, (num_measurements_raw + 7) >> 3, out);
        } else if (format == SAMPLE_FORMAT_ASCII) {
            for (size_t k = 0; k < num_measurements_raw; k++) {
                putc_unlocked('0' + buf[k], out);
            }
            putc_unlocked('\n', out);
        } else {
            assert(false);
        }
    }
}

void FrameSimulator::do_transpose() {
    results_block_transposed = !results_block_transposed;
    blockwise_transpose_256x256(m_table.data.u64, m_table.data.num_bits_padded());
}

void FrameSimulator::clear() {
    num_recorded_measurements = 0;
    x_table.clear();
    z_table.data.randomize(z_table.data.num_bits_padded(), rng);
    m_table.clear();
    results_block_transposed = false;
}

void FrameSimulator::clear_and_run(const Circuit &circuit) {
    assert(circuit.num_measurements == num_measurements_raw);
    clear();
    for (const auto &op : circuit.operations) {
        do_named_op(op.name, op.target_data);
    }
}

void FrameSimulator::do_named_op(const std::string &name, const OperationData &target_data) {
    try {
        SIM_BULK_PAULI_FRAMES_GATE_DATA.at(name)(*this, target_data);
    } catch (const std::out_of_range &ex) {
        auto message = "Gate isn't supported by FrameSimulator: " + name;
        if (name == "M" || name == "M_PREFER_0") {
            message += ".\nMeasurements need to be converted to measurement-with-reference-result (M_REF) operations, "
                       "so that the tracked Pauli frame has something to invert with respect to."
                       "\nDid you forget a conversion step? Did you intend to use the Tableau simulator instead?";
        }
        throw std::out_of_range(message);
    }
}

void FrameSimulator::measure_ref(const OperationData &target_data) {
    for (size_t k = 0; k < target_data.targets.size(); k++) {
        size_t q = target_data.targets[k];
        z_table[q].randomize(z_table[q].num_bits_padded(), rng);

        auto x = (__m256i *)x_table[q].ptr_simd;
        auto x_end = x + x_table[q].num_simd_words;
        auto m = (__m256i *)m_table.data.ptr_simd + (recorded_bit_address(0, num_recorded_measurements) >> 8);
        auto m_stride = recorded_bit_address(256, 0) >> 8;
        if (target_data.flags[k]) {
            while (x != x_end) {
                *m = *x ^ _mm256_set1_epi8(-1);
                x++;
                m += m_stride;
            }
        } else {
            while (x != x_end) {
                *m = *x;
                x++;
                m += m_stride;
            }
        }
        num_recorded_measurements++;
    }
}

void FrameSimulator::reset(const OperationData &target_data) {
    for (auto q : target_data.targets) {
        x_table[q].clear();
        z_table[q].randomize(z_table[q].num_bits_padded(), rng);
    }
}

PauliStringVal FrameSimulator::get_frame(size_t sample_index) const {
    assert(sample_index < num_samples_raw);
    PauliStringVal result(num_qubits);
    for (size_t q = 0; q < num_qubits; q++) {
        result.x_data[q] = x_table[q][sample_index];
        result.z_data[q] = z_table[q][sample_index];
    }
    return result;
}

void FrameSimulator::set_frame(size_t sample_index, const PauliStringRef &new_frame) {
    assert(sample_index < num_samples_raw);
    assert(new_frame.num_qubits == num_qubits);
    for (size_t q = 0; q < num_qubits; q++) {
        x_table[q][sample_index] = new_frame.x_ref[q];
        z_table[q][sample_index] = new_frame.z_ref[q];
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

void FrameSimulator::CX(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](auto &x1, auto &z1, auto &x2, auto &z2) {
        z1 ^= z2;
        x2 ^= x1;
    });
}

void FrameSimulator::CY(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](auto &x1, auto &z1, auto &x2, auto &z2) {
        z1 ^= x2 ^ z2;
        z2 ^= x1;
        x2 ^= x1;
    });
}

void FrameSimulator::CZ(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](auto &x1, auto &z1, auto &x2, auto &z2) {
        z1 ^= x2;
        z2 ^= x1;
    });
}

void FrameSimulator::SWAP(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](auto &x1, auto &z1, auto &x2, auto &z2) {
        std::swap(z1, z2);
        std::swap(x1, x2);
    });
}

void FrameSimulator::ISWAP(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](auto &x1, auto &z1, auto &x2, auto &z2) {
        auto dx = x1 ^ x2;
        auto t1 = z1 ^ dx;
        auto t2 = z2 ^ dx;
        z1 = t2;
        z2 = t1;
        std::swap(x1, x2);
    });
}

void FrameSimulator::XCX(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](auto &x1, auto &z1, auto &x2, auto &z2) {
        x1 ^= z2;
        x2 ^= z1;
    });
}

void FrameSimulator::XCY(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](auto &x1, auto &z1, auto &x2, auto &z2) {
        x1 ^= x2 ^ z2;
        x2 ^= z1;
        z2 ^= z1;
    });
}

void FrameSimulator::XCZ(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](auto &x1, auto &z1, auto &x2, auto &z2) {
        z2 ^= z1;
        x1 ^= x2;
    });
}

void FrameSimulator::YCX(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](auto &x1, auto &z1, auto &x2, auto &z2) {
        x2 ^= x1 ^ z1;
        x1 ^= z2;
        z1 ^= z2;
    });
}

void FrameSimulator::YCY(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](auto &x1, auto &z1, auto &x2, auto &z2) {
        auto y1 = x1 ^z1;
        auto y2 = x2 ^z2;
        x1 ^= y2;
        z1 ^= y2;
        x2 ^= y1;
        z2 ^= y1;
    });
}

void FrameSimulator::YCZ(const OperationData &target_data) {
    for_each_target_pair(*this, target_data, [](auto &x1, auto &z1, auto &x2, auto &z2) {
        z2 ^= x1 ^ z1;
        z1 ^= x2;
        x1 ^= x2;
    });
}

void FrameSimulator::DEPOLARIZE(const OperationData &target_data, float probability) {
    const auto &targets = target_data.targets;
    RareErrorIterator skipper(probability);
    auto n = targets.size() * num_samples_raw;
    while (true) {
        size_t s = skipper.next(rng);
        if (s >= n) {
            break;
        }
        auto p = 1 + (rng() % 3);
        auto target_index = s / num_samples_raw;
        auto sample_index = s % num_samples_raw;
        auto t = targets[target_index];
        x_table[t][sample_index] ^= p & 1;
        z_table[t][sample_index] ^= p & 2;
    }
}

void FrameSimulator::DEPOLARIZE2(const OperationData &target_data, float probability) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    RareErrorIterator skipper(probability);
    auto n = (targets.size() * num_samples_raw) >> 1;
    while (true) {
        size_t s = skipper.next(rng);
        if (s >= n) {
            break;
        }
        auto p = 1 + (rng() % 15);
        auto target_index = (s / num_samples_raw) << 1;
        auto sample_index = s % num_samples_raw;
        size_t t1 = targets[target_index];
        size_t t2 = targets[target_index + 1];
        x_table[t1][sample_index] ^= p & 1;
        z_table[t1][sample_index] ^= p & 2;
        x_table[t2][sample_index] ^= p & 4;
        z_table[t2][sample_index] ^= p & 8;
    }
}

std::vector<simd_bits> FrameSimulator::sample(const Circuit &circuit, size_t num_samples, std::mt19937_64 &rng) {
    FrameSimulator sim(circuit.num_qubits, num_samples, circuit.num_measurements, rng);
    sim.clear_and_run(circuit);
    return sim.unpack_measurements();
}

void FrameSimulator::sample_out(const Circuit &circuit, size_t num_samples, FILE *out, SampleFormat format, std::mt19937_64 &rng) {
    constexpr size_t GOOD_BLOCK_SIZE = 1024;
    if (num_samples >= GOOD_BLOCK_SIZE) {
        auto sim = FrameSimulator(circuit.num_qubits, GOOD_BLOCK_SIZE, circuit.num_measurements, rng);
        while (num_samples > GOOD_BLOCK_SIZE) {
            sim.clear_and_run(circuit);
            sim.unpack_write_measurements(out, format);
            num_samples -= GOOD_BLOCK_SIZE;
        }
    }
    if (num_samples) {
        auto sim = FrameSimulator(circuit.num_qubits, num_samples, circuit.num_measurements, rng);
        sim.clear_and_run(circuit);
        sim.unpack_write_measurements(out, format);
    }
}
