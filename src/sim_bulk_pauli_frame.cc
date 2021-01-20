#include <bit>
#include <cstring>

#include "sim_tableau.h"
#include "sim_bulk_pauli_frame.h"
#include "simd/simd_util.h"
#include "probability_util.h"

// Iterates over the X and Z frame components of a pair of qubits, applying a custom BODY to each.
//
// HACK: Templating the body function type makes inlining significantly more likely.
struct XZ_PAIR {
    __m256i *x1;
    __m256i *z1;
    __m256i *x2;
    __m256i *z2;
};
template <typename BODY>
inline void FOR_EACH_XZ_PAIR(SimBulkPauliFrames &sim, const std::vector<size_t> &targets, BODY body) {
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k];
        size_t q2 = targets[k + 1];
        auto x1 = sim.x_start(q1);
        auto z1 = sim.z_start(q1);
        auto x2 = sim.x_start(q2);
        auto z2 = sim.z_start(q2);
        auto x2_end = x2 + sim.num_sample_blocks256;
        while (x2 != x2_end) {
            body({x1, z1, x2, z2});
            x1++;
            z1++;
            x2++;
            z2++;
        }
    }
}

size_t SimBulkPauliFrames::recorded_bit_address(size_t sample_index, size_t measure_index) const {
    size_t s_high = sample_index >> 8;
    size_t s_low = sample_index & 0xFF;
    size_t m_high = measure_index >> 8;
    size_t m_low = measure_index & 0xFF;
    if (results_block_transposed) {
        std::swap(m_low, s_low);
    }
    return s_low + (m_low << 8) + (s_high << 16) + (m_high * num_sample_blocks256 << 16);
}

SimBulkPauliFrames::SimBulkPauliFrames(size_t init_num_qubits, size_t num_samples, size_t num_measurements) :
        num_qubits(init_num_qubits),
        num_samples_raw(num_samples),
        num_sample_blocks256(ceil256(num_samples) >> 8),
        num_measurements_raw(num_measurements),
        num_measurement_blocks(ceil256(num_measurements) >> 8),
        x_blocks(init_num_qubits * ceil256(num_samples)),
        z_blocks(init_num_qubits * ceil256(num_samples)),
        recorded_results(ceil256(num_measurements) * ceil256(num_samples)),
        rng_buffer(ceil256(num_samples)),
        rng((std::random_device {})()) {
}

SimdRange SimBulkPauliFrames::x_rng(size_t qubit) {
    return {x_blocks.u256 + qubit * num_sample_blocks256, num_sample_blocks256};
}

SimdRange SimBulkPauliFrames::z_rng(size_t qubit) {
    return {z_blocks.u256 + qubit * num_sample_blocks256, num_sample_blocks256};
}

__m256i *SimBulkPauliFrames::x_start(size_t qubit) {
    return x_blocks.u256 + qubit * num_sample_blocks256;
}

void SimBulkPauliFrames::unpack_sample_measurements_into(size_t sample_index, simd_bits &out) {
    if (!results_block_transposed) {
        do_transpose();
    }
    for (size_t m = 0; m < num_measurements_raw; m += 256) {
        out.u256[m >> 8] = recorded_results.u256[recorded_bit_address(sample_index, m) >> 8];
    }
}

std::vector<simd_bits> SimBulkPauliFrames::unpack_measurements() {
    std::vector<simd_bits> result;
    for (size_t s = 0; s < num_samples_raw; s++) {
        result.emplace_back(num_measurements_raw);
        unpack_sample_measurements_into(s, result.back());
    }
    return result;
}

void SimBulkPauliFrames::unpack_write_measurements(FILE *out, SampleFormat format) {
    if (results_block_transposed != (format == SAMPLE_FORMAT_RAW_UNSTABLE)) {
        do_transpose();
    }

    if (format == SAMPLE_FORMAT_RAW_UNSTABLE) {
        fwrite(recorded_results.u64, 1, recorded_results.num_bits >> 3, out);
        return;
    }

    simd_bits buf(num_measurements_raw);
    for (size_t s = 0; s < num_samples_raw; s++) {
        unpack_sample_measurements_into(s, buf);

        if (format == SAMPLE_FORMAT_BINLE8) {
            static_assert(std::endian::native == std::endian::little);
            fwrite(buf.u64, 1, (buf.num_bits + 7) >> 3, out);
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

void SimBulkPauliFrames::do_transpose() {
    results_block_transposed = !results_block_transposed;
    blockwise_transpose_256x256(recorded_results.u64, recorded_results.num_bits);
}

__m256i *SimBulkPauliFrames::z_start(size_t qubit) {
    return z_blocks.u256 + qubit * num_sample_blocks256;
}

void SimBulkPauliFrames::clear() {
    num_recorded_measurements = 0;
    x_blocks.clear();
    z_blocks.clear();
    recorded_results.clear();
    results_block_transposed = false;
}

void SimBulkPauliFrames::clear_and_run(const PauliFrameProgram &program) {
    assert(program.num_measurements == num_measurements_raw);
    clear();
    for (const auto &cycle : program.cycles) {
        for (const auto &op : cycle.step1_unitary) {
            do_named_op(op.name, op.targets);
        }
        for (const auto &collapse : cycle.step2_collapse) {
            RANDOM_KICKBACK(collapse.destabilizer);
        }
        measure_deterministic(cycle.step3_measure);
        reset(cycle.step4_reset);
    }
}

void SimBulkPauliFrames::do_named_op(const std::string &name, const std::vector<size_t> &targets) {
    SIM_BULK_PAULI_FRAMES_GATE_DATA.at(name)(*this, targets);
}

void SimBulkPauliFrames::measure_deterministic(const std::vector<PauliFrameProgramMeasurement> &measurements) {
    for (auto e : measurements) {
        auto q = e.target_qubit;
        auto x = x_start(q);
        auto x_end = x + num_sample_blocks256;
        auto m = recorded_results.u256 + (recorded_bit_address(0, num_recorded_measurements) >> 8);
        auto m_stride = recorded_bit_address(256, 0) >> 8;
        if (e.invert) {
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

void SimBulkPauliFrames::MUL_INTO_FRAME(const SparsePauliString &pauli_string, const __m256i *mask) {
    for (const auto &w : pauli_string.indexed_words) {
        for (size_t k2 = 0; k2 < 64; k2++) {
            if ((w.wx >> k2) & 1) {
                *x_rng(w.index64 * 64 + k2) ^= mask;
            }
            if ((w.wz >> k2) & 1) {
                *z_rng(w.index64 * 64 + k2) ^= mask;
            }
        }

    }
}

void SimBulkPauliFrames::RANDOM_KICKBACK(const SparsePauliString &pauli_string) {
    auto n32 = num_sample_blocks256 << 3;
    auto u32 = (uint32_t *)rng_buffer.u64;
    for (size_t k = 0; k < n32; k++) {
        u32[k] = rng();
    }
    MUL_INTO_FRAME(pauli_string, rng_buffer.u256);
}

void SimBulkPauliFrames::reset(const std::vector<size_t> &qubits) {
    for (auto q : qubits) {
        x_rng(q).clear();
        z_rng(q).clear();
    }
}

PauliStringVal SimBulkPauliFrames::get_frame(size_t sample_index) const {
    assert(sample_index < num_samples_raw);
    PauliStringVal result(num_qubits);
    for (size_t q = 0; q < num_qubits; q++) {
        result.x_data[q] = x_blocks[q * num_sample_blocks256 * 256 + sample_index];
        result.z_data[q] = z_blocks[q * num_sample_blocks256 * 256 + sample_index];
    }
    return result;
}

void SimBulkPauliFrames::set_frame(size_t sample_index, const PauliStringRef &new_frame) {
    assert(sample_index < num_samples_raw);
    assert(new_frame.num_qubits == num_qubits);
    for (size_t q = 0; q < num_qubits; q++) {
        x_blocks[q * num_sample_blocks256 * 256 + sample_index] = new_frame.x_ref[q];
        z_blocks[q * num_sample_blocks256 * 256 + sample_index] = new_frame.z_ref[q];
    }
}

void SimBulkPauliFrames::H_XZ(const std::vector<size_t> &targets) {
    for (auto q : targets) {
        x_rng(q).swap_with(z_rng(q));
    }
}

void SimBulkPauliFrames::H_XY(const std::vector<size_t> &targets) {
    for (auto q : targets) {
        *z_rng(q) ^= *x_rng(q);
    }
}

void SimBulkPauliFrames::H_YZ(const std::vector<size_t> &targets) {
    for (auto q : targets) {
        *x_rng(q) ^= *z_rng(q);
    }
}

void SimBulkPauliFrames::CX(const std::vector<size_t> &targets) {
    FOR_EACH_XZ_PAIR(*this, targets, [](XZ_PAIR s) {
        *s.z1 ^= *s.z2;
        *s.x2 ^= *s.x1;
    });
}

void SimBulkPauliFrames::CY(const std::vector<size_t> &targets) {
    FOR_EACH_XZ_PAIR(*this, targets, [](XZ_PAIR s) {
        *s.z1 ^= *s.x2 ^ *s.z2;
        *s.z2 ^= *s.x1;
        *s.x2 ^= *s.x1;
    });
}

void SimBulkPauliFrames::CZ(const std::vector<size_t> &targets) {
    FOR_EACH_XZ_PAIR(*this, targets, [](XZ_PAIR s) {
        *s.z1 ^= *s.x2;
        *s.z2 ^= *s.x1;
    });
}

void SimBulkPauliFrames::SWAP(const std::vector<size_t> &targets) {
    FOR_EACH_XZ_PAIR(*this, targets, [](XZ_PAIR s) {
        std::swap(*s.z1, *s.z2);
        std::swap(*s.x1, *s.x2);
    });
}

void SimBulkPauliFrames::ISWAP(const std::vector<size_t> &targets) {
    FOR_EACH_XZ_PAIR(*this, targets, [](XZ_PAIR s) {
        auto dx = *s.x1 ^ *s.x2;
        auto t1 = *s.z1 ^ dx;
        auto t2 = *s.z2 ^ dx;
        *s.z1 = t2;
        *s.z2 = t1;
        std::swap(*s.x1, *s.x2);
    });
}

void SimBulkPauliFrames::XCX(const std::vector<size_t> &targets) {
    FOR_EACH_XZ_PAIR(*this, targets, [](XZ_PAIR s) {
        *s.x1 ^= *s.z2;
        *s.x2 ^= *s.z1;
    });
}

void SimBulkPauliFrames::XCY(const std::vector<size_t> &targets) {
    FOR_EACH_XZ_PAIR(*this, targets, [](XZ_PAIR s) {
        *s.x1 ^= *s.x2 ^ *s.z2;
        *s.x2 ^= *s.z1;
        *s.z2 ^= *s.z1;
    });
}

void SimBulkPauliFrames::XCZ(const std::vector<size_t> &targets) {
    FOR_EACH_XZ_PAIR(*this, targets, [](XZ_PAIR s) {
        *s.z2 ^= *s.z1;
        *s.x1 ^= *s.x2;
    });
}

void SimBulkPauliFrames::YCX(const std::vector<size_t> &targets) {
    FOR_EACH_XZ_PAIR(*this, targets, [](XZ_PAIR s) {
        *s.x2 ^= *s.x1 ^ *s.z1;
        *s.x1 ^= *s.z2;
        *s.z1 ^= *s.z2;
    });
}

void SimBulkPauliFrames::YCY(const std::vector<size_t> &targets) {
    FOR_EACH_XZ_PAIR(*this, targets, [](XZ_PAIR s) {
        auto y1 = *s.x1 ^ *s.z1;
        auto y2 = *s.x2 ^ *s.z2;
        *s.x1 ^= y2;
        *s.z1 ^= y2;
        *s.x2 ^= y1;
        *s.z2 ^= y1;
    });
}

void SimBulkPauliFrames::YCZ(const std::vector<size_t> &targets) {
    FOR_EACH_XZ_PAIR(*this, targets, [](XZ_PAIR s) {
        *s.z2 ^= *s.x1 ^ *s.z1;
        *s.z1 ^= *s.x2;
        *s.x1 ^= *s.x2;
    });
}

void SimBulkPauliFrames::DEPOLARIZE(const std::vector<size_t> &targets, float probability) {
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
        size_t i = targets[target_index] * num_sample_blocks256 + sample_index;
        x_blocks[i] ^= p & 1;
        z_blocks[i] ^= p & 2;
    }
}

void SimBulkPauliFrames::DEPOLARIZE2(const std::vector<size_t> &targets, float probability) {
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
        size_t i1 = targets[target_index] * num_sample_blocks256 + sample_index;
        size_t i2 = targets[target_index + 1] * num_sample_blocks256 + sample_index;
        x_blocks[i1] ^= p & 1;
        z_blocks[i1] ^= p & 2;
        x_blocks[i2] ^= p & 4;
        z_blocks[i2] ^= p & 8;
    }
}

void do_nothing_pst(SimBulkPauliFrames &p, const std::vector<size_t> &target) {
}

const std::unordered_map<std::string, std::function<void(SimBulkPauliFrames &, const std::vector<size_t> &)>> SIM_BULK_PAULI_FRAMES_GATE_DATA{
    // Pauli gates.
    {"I",          &do_nothing_pst},
    {"X",          &do_nothing_pst},
    {"Y",          &do_nothing_pst},
    {"Z",          &do_nothing_pst},
    // Axis exchange gates.
    {"H", &SimBulkPauliFrames::H_XZ},
    {"H_XY",       &SimBulkPauliFrames::H_XY},
    {"H_XZ", &SimBulkPauliFrames::H_XZ},
    {"H_YZ",       &SimBulkPauliFrames::H_YZ},
    // 90 degree rotation gates.
    {"SQRT_X",     &SimBulkPauliFrames::H_YZ},
    {"SQRT_X_DAG", &SimBulkPauliFrames::H_YZ},
    {"SQRT_Y",     &SimBulkPauliFrames::H_XZ},
    {"SQRT_Y_DAG", &SimBulkPauliFrames::H_XZ},
    {"SQRT_Z",     &SimBulkPauliFrames::H_XY},
    {"SQRT_Z_DAG", &SimBulkPauliFrames::H_XY},
    {"S",          &SimBulkPauliFrames::H_XY},
    {"S_DAG",      &SimBulkPauliFrames::H_XY},
    // Swap gates.
    {"SWAP", &SimBulkPauliFrames::SWAP},
    {"ISWAP", &SimBulkPauliFrames::ISWAP},
    {"ISWAP_DAG", &SimBulkPauliFrames::ISWAP},
    // Controlled gates.
    {"CNOT", &SimBulkPauliFrames::CX},
    {"CX", &SimBulkPauliFrames::CX},
    {"CY", &SimBulkPauliFrames::CY},
    {"CZ", &SimBulkPauliFrames::CZ},
    // Controlled interactions in other bases.
    {"XCX", &SimBulkPauliFrames::XCX},
    {"XCY", &SimBulkPauliFrames::XCY},
    {"XCZ", &SimBulkPauliFrames::XCZ},
    {"YCX", &SimBulkPauliFrames::YCX},
    {"YCY", &SimBulkPauliFrames::YCY},
    {"YCZ", &SimBulkPauliFrames::YCZ},
};
