#include <bit>
#include <cstring>

#include "sim_tableau.h"
#include "sim_bulk_pauli_frame.h"
#include "simd_util.h"

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

__m256i *SimBulkPauliFrames::x_start(size_t qubit) {
    return x_blocks.u256 + qubit * num_sample_blocks256;
}

void SimBulkPauliFrames::unpack_sample_measurements_into(size_t sample_index, aligned_bits256 &out) {
    if (!results_block_transposed) {
        do_transpose();
    }
    for (size_t m = 0; m < num_measurements_raw; m += 256) {
        out.u256[m >> 8] = recorded_results.u256[recorded_bit_address(sample_index, m) >> 8];
    }
}

std::vector<aligned_bits256> SimBulkPauliFrames::unpack_measurements() {
    std::vector<aligned_bits256> result;
    for (size_t s = 0; s < num_samples_raw; s++) {
        result.emplace_back(num_measurements_raw);
        unpack_sample_measurements_into(s, result.back());
    }
    return result;
}

void SimBulkPauliFrames::unpack_write_measurements(FILE *out, SampleFormat format) {
    if (results_block_transposed != (format == SAMPLE_FORMAT_RAW)) {
        do_transpose();
    }

    if (format == SAMPLE_FORMAT_RAW) {
        fwrite(recorded_results.u64, 1, recorded_results.num_bits >> 3, out);
        return;
    }

    aligned_bits256 buf(num_measurements_raw);
    for (size_t s = 0; s < num_samples_raw; s++) {
        unpack_sample_measurements_into(s, buf);

        if (format == SAMPLE_FORMAT_BINLE8) {
            static_assert(std::endian::native == std::endian::little);
            fwrite(buf.u64, 1, (buf.num_bits + 7) >> 3, out);
        } else if (format == SAMPLE_FORMAT_ASCII) {
            for (size_t k = 0; k < num_measurements_raw; k++) {
                putc_unlocked('0' + (char)buf.get_bit(k), out);
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
                auto q = w.index64 * 64 + k2;
                auto x = x_start(q);
                auto x_end = x + num_sample_blocks256;
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
                auto z_end = z + num_sample_blocks256;
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
        auto x = x_start(q);
        auto z = z_start(q);
        memset(x, 0, num_sample_blocks256 << 5);
        memset(z, 0, num_sample_blocks256 << 5);
    }
}

PauliStringVal SimBulkPauliFrames::get_frame(size_t sample_index) const {
    assert(sample_index < num_samples_raw);
    PauliStringVal result(num_qubits);
    for (size_t q = 0; q < num_qubits; q++) {
        result.ptr().set_x_bit(q, x_blocks.get_bit(q * num_sample_blocks256 * 256 + sample_index));
        result.ptr().set_z_bit(q, z_blocks.get_bit(q * num_sample_blocks256 * 256 + sample_index));
    }
    return result;
}

void SimBulkPauliFrames::set_frame(size_t sample_index, const PauliStringPtr &new_frame) {
    assert(sample_index < num_samples_raw);
    assert(new_frame.size == num_qubits);
    for (size_t q = 0; q < num_qubits; q++) {
        x_blocks.set_bit(q * num_sample_blocks256 * 256 + sample_index, new_frame.get_x_bit(q));
        z_blocks.set_bit(q * num_sample_blocks256 * 256 + sample_index, new_frame.get_z_bit(q));
    }
}

void SimBulkPauliFrames::H_XZ(const std::vector<size_t> &targets) {
    for (auto q : targets) {
        auto x = x_start(q);
        auto z = z_start(q);
        auto x_end = x + num_sample_blocks256;
        while (x != x_end) {
            std::swap(*x, *z);
            x++;
            z++;
        }
    }
}

void SimBulkPauliFrames::H_XY(const std::vector<size_t> &targets) {
    for (auto q : targets) {
        auto x = x_start(q);
        auto z = z_start(q);
        auto x_end = x + num_sample_blocks256;
        while (x != x_end) {
            *z ^= *x;
            x++;
            z++;
        }
    }
}

void SimBulkPauliFrames::H_YZ(const std::vector<size_t> &targets) {
    for (auto q : targets) {
        auto x = x_start(q);
        auto z = z_start(q);
        auto x_end = x + num_sample_blocks256;
        while (x != x_end) {
            *x ^= *z;
            x++;
            z++;
        }
    }
}

void SimBulkPauliFrames::CX(const std::vector<size_t> &targets) {
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k];
        size_t q2 = targets[k + 1];
        auto x1 = x_start(q1);
        auto z1 = z_start(q1);
        auto x2 = x_start(q2);
        auto z2 = z_start(q2);
        auto x2_end = x2 + num_sample_blocks256;
        while (x2 != x2_end) {
            *z1 ^= *z2;
            *x2 ^= *x1;
            x1++;
            z1++;
            x2++;
            z2++;
        }
    }
}

void SimBulkPauliFrames::CY(const std::vector<size_t> &targets) {
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k];
        size_t q2 = targets[k + 1];
        auto x1 = x_start(q1);
        auto z1 = z_start(q1);
        auto x2 = x_start(q2);
        auto z2 = z_start(q2);
        auto x2_end = x2 + num_sample_blocks256;
        while (x2 != x2_end) {
            *z1 ^= *x2 ^ *z2;
            *z2 ^= *x1;
            *x2 ^= *x1;
            x1++;
            z1++;
            x2++;
            z2++;
        }
    }
}

void SimBulkPauliFrames::CZ(const std::vector<size_t> &targets) {
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k];
        size_t q2 = targets[k + 1];
        auto x1 = x_start(q1);
        auto z1 = z_start(q1);
        auto x2 = x_start(q2);
        auto z2 = z_start(q2);
        auto x2_end = x2 + num_sample_blocks256;
        while (x2 != x2_end) {
            *z1 ^= *x2;
            *z2 ^= *x1;
            x1++;
            z1++;
            x2++;
            z2++;
        }
    }
}

void SimBulkPauliFrames::SWAP(const std::vector<size_t> &targets) {
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k];
        size_t q2 = targets[k + 1];
        auto x1 = x_start(q1);
        auto z1 = z_start(q1);
        auto x2 = x_start(q2);
        auto z2 = z_start(q2);
        auto x2_end = x2 + num_sample_blocks256;
        while (x2 != x2_end) {
            std::swap(*z1, *z2);
            std::swap(*x1, *x2);
            x1++;
            z1++;
            x2++;
            z2++;
        }
    }
}

void SimBulkPauliFrames::ISWAP(const std::vector<size_t> &targets) {
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k];
        size_t q2 = targets[k + 1];
        auto x1 = x_start(q1);
        auto z1 = z_start(q1);
        auto x2 = x_start(q2);
        auto z2 = z_start(q2);
        auto x2_end = x2 + num_sample_blocks256;
        while (x2 != x2_end) {
            auto dx = *x1 ^ *x2;
            auto t1 = *z1 ^ dx;
            auto t2 = *z2 ^ dx;
            *z1 = t2;
            *z2 = t1;
            std::swap(*x1, *x2);
            x1++;
            z1++;
            x2++;
            z2++;
        }
    }
}

void SimBulkPauliFrames::XCX(const std::vector<size_t> &targets) {
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k];
        size_t q2 = targets[k + 1];
        auto x1 = x_start(q1);
        auto z1 = z_start(q1);
        auto x2 = x_start(q2);
        auto z2 = z_start(q2);
        auto x2_end = x2 + num_sample_blocks256;
        while (x2 != x2_end) {
            *x1 ^= *z2;
            *x2 ^= *z1;
            x1++;
            z1++;
            x2++;
            z2++;
        }
    }
}

void SimBulkPauliFrames::XCY(const std::vector<size_t> &targets) {
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k];
        size_t q2 = targets[k + 1];
        auto x1 = x_start(q1);
        auto z1 = z_start(q1);
        auto x2 = x_start(q2);
        auto z2 = z_start(q2);
        auto x2_end = x2 + num_sample_blocks256;
        while (x2 != x2_end) {
            *x1 ^= *x2 ^ *z2;
            *x2 ^= *z1;
            *z2 ^= *z1;
            x1++;
            z1++;
            x2++;
            z2++;
        }
    }
}

void SimBulkPauliFrames::XCZ(const std::vector<size_t> &targets) {
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k];
        size_t q2 = targets[k + 1];
        auto x1 = x_start(q1);
        auto z1 = z_start(q1);
        auto x2 = x_start(q2);
        auto z2 = z_start(q2);
        auto x2_end = x2 + num_sample_blocks256;
        while (x2 != x2_end) {
            *z2 ^= *z1;
            *x1 ^= *x2;
            x1++;
            z1++;
            x2++;
            z2++;
        }
    }
}

void SimBulkPauliFrames::YCX(const std::vector<size_t> &targets) {
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k];
        size_t q2 = targets[k + 1];
        auto x1 = x_start(q1);
        auto z1 = z_start(q1);
        auto x2 = x_start(q2);
        auto z2 = z_start(q2);
        auto x2_end = x2 + num_sample_blocks256;
        while (x2 != x2_end) {
            *x2 ^= *x1 ^ *z1;
            *x1 ^= *z2;
            *z1 ^= *z2;
            x1++;
            z1++;
            x2++;
            z2++;
        }
    }
}

void SimBulkPauliFrames::YCY(const std::vector<size_t> &targets) {
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k];
        size_t q2 = targets[k + 1];
        auto x1 = x_start(q1);
        auto z1 = z_start(q1);
        auto x2 = x_start(q2);
        auto z2 = z_start(q2);
        auto x2_end = x2 + num_sample_blocks256;
        while (x2 != x2_end) {
            auto y1 = *x1 ^ *z1;
            auto y2 = *x2 ^ *z2;
            *x1 ^= y2;
            *z1 ^= y2;
            *x2 ^= y1;
            *z2 ^= y1;
            x1++;
            z1++;
            x2++;
            z2++;
        }
    }
}

void SimBulkPauliFrames::YCZ(const std::vector<size_t> &targets) {
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k];
        size_t q2 = targets[k + 1];
        auto x1 = x_start(q1);
        auto z1 = z_start(q1);
        auto x2 = x_start(q2);
        auto z2 = z_start(q2);
        auto x2_end = x2 + num_sample_blocks256;
        while (x2 != x2_end) {
            *z2 ^= *x1 ^ *z1;
            *z1 ^= *x2;
            *x1 ^= *x2;
            x1++;
            z1++;
            x2++;
            z2++;
        }
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
