#include "sim_tableau.h"
#include "sim_frame.h"
#include "simd_util.h"
#include <cstring>

size_t SimBulkPauliFrames::recorded_bit_address(size_t sample_index, size_t measure_index, bool finished) const {
    size_t s_high = sample_index >> 8;
    size_t s_low = sample_index & 0xFF;
    size_t m_high = measure_index >> 8;
    size_t m_low = measure_index & 0xFF;
    if (finished) {
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

struct IterMeasure {
    size_t sample_index;
    size_t measurement_word256_index;
    __m256i *bits;
};

std::vector<aligned_bits256> SimBulkPauliFrames::unpack_measurements() const {
    std::vector<aligned_bits256> result;
    for (size_t s = 0; s < num_samples_raw; s++) {
        result.emplace_back(num_measurements_raw);
        for (size_t m = 0; m < num_measurements_raw; m += 256) {
            result[s].u256[m >> 8] = recorded_results.u256[recorded_bit_address(s, m, true) >> 8];
        }
    }
    return result;
}

void SimBulkPauliFrames::unpack_write_measurements_ascii(FILE *out) const {
    for (size_t s = 0; s < num_samples_raw; s++) {
        for (size_t m = 0; m < num_measurements_raw; m += 256) {
            auto pt = (uint64_t *)&recorded_results.u256[recorded_bit_address(s, m, true) >> 8];
            for (size_t m2 = m; m2 < m + 256 && m2 < num_measurements_raw; m2++) {
                bool bit = (pt[m2 >> 6] >> (m2 & 63)) & 1;
                putc_unlocked("01"[bit], out);
            }
        }
        putc_unlocked('\n', out);
    }
}

void SimBulkPauliFrames::finish() {
    blockwise_transpose_256x256(recorded_results.u64, recorded_results.num_bits, false);
}

__m256i *SimBulkPauliFrames::z_start(size_t qubit) {
    return z_blocks.u256 + qubit * num_sample_blocks256;
}

void SimBulkPauliFrames::begin() {
    num_recorded_measurements = 0;
    x_blocks.clear();
    z_blocks.clear();
    recorded_results.clear();
}

void SimBulkPauliFrames::begin_and_run_and_finish(const PauliFrameProgram &program) {
    assert(program.num_measurements == num_measurements_raw);
    begin();
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
    finish();
}

void SimBulkPauliFrames::do_named_op(const std::string &name, const std::vector<size_t> &targets) {
    SIM_BULK_PAULI_FRAMES_GATE_DATA.at(name)(*this, targets);
}


void SimBulkPauliFrames::measure_deterministic(const std::vector<PauliFrameProgramMeasurement> &measurements) {
    for (auto e : measurements) {
        auto q = e.target_qubit;
        auto x = x_start(q);
        auto x_end = x + num_sample_blocks256;
        auto m = recorded_results.u256 + (recorded_bit_address(0, num_recorded_measurements, false) >> 8);
        auto m_stride = recorded_bit_address(256, 0, false) >> 8;
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

struct XZ {
    __m256i *x;
    __m256i *z;
};
struct XZ2 {
    __m256i *x1;
    __m256i *z1;
    __m256i *x2;
    __m256i *z2;
};

inline void for_xz(SimBulkPauliFrames &frames, const std::vector<size_t> &targets, std::function<void(XZ &)> func) {
    for (auto q : targets) {
        XZ state {frames.x_start(q), frames.z_start(q) };
        auto x_end = state.x + frames.num_sample_blocks256;
        while (state.x != x_end) {
            func(state);
            state.x++;
            state.z++;
        }
    }
}

inline void for_xz_pairs(SimBulkPauliFrames &frames, const std::vector<size_t> &targets, std::function<void(XZ2 &)> func) {
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t q1 = targets[k];
        size_t q2 = targets[k + 1];
        XZ2 state{
                frames.x_start(q1),
                frames.z_start(q1),
                frames.x_start(q2),
                frames.z_start(q2),
        };
        auto x2_end = state.x2 + frames.num_sample_blocks256;
        while (state.x2 != x2_end) {
            func(state);
            state.x1++;
            state.z1++;
            state.x2++;
            state.z2++;
        }
    }
}

void SimBulkPauliFrames::H_XZ(const std::vector<size_t> &targets) {
    for_xz(*this, targets, [](XZ &state){
        std::swap(*state.x, *state.z);
    });
}

void SimBulkPauliFrames::H_XY(const std::vector<size_t> &targets) {
    for_xz(*this, targets, [](XZ &state){
        *state.z ^= *state.x;
    });
}

void SimBulkPauliFrames::H_YZ(const std::vector<size_t> &targets) {
    for_xz(*this, targets, [](XZ &state){
        *state.x ^= *state.z;
    });
}

void SimBulkPauliFrames::CX(const std::vector<size_t> &targets) {
    for_xz_pairs(*this, targets, [](XZ2 &state){
        *state.z1 ^= *state.z2;
        *state.x2 ^= *state.x1;
    });
}

void SimBulkPauliFrames::CY(const std::vector<size_t> &targets) {
    for_xz_pairs(*this, targets, [](XZ2 &state){
        *state.z1 ^= *state.x2 ^ *state.z2;
        *state.z2 ^= *state.x1;
        *state.x2 ^= *state.x1;
    });
}

void SimBulkPauliFrames::CZ(const std::vector<size_t> &targets) {
    for_xz_pairs(*this, targets, [](XZ2 &state){
        *state.z1 ^= *state.x2;
        *state.z2 ^= *state.x1;
    });
}

void SimBulkPauliFrames::SWAP(const std::vector<size_t> &targets) {
    for_xz_pairs(*this, targets, [](XZ2 &state){
        std::swap(*state.z1, *state.z2);
        std::swap(*state.x1, *state.x2);
    });
}

void SimBulkPauliFrames::ISWAP(const std::vector<size_t> &targets) {
    for_xz_pairs(*this, targets, [](XZ2 &state){
        auto dx = *state.x1 ^ *state.x2;
        auto t1 = *state.z1 ^ dx;
        auto t2 = *state.z2 ^ dx;
        *state.z1 = t2;
        *state.z2 = t1;
        std::swap(*state.x1, *state.x2);
    });
}

void SimBulkPauliFrames::XCX(const std::vector<size_t> &targets) {
    for_xz_pairs(*this, targets, [](XZ2 &state){
        *state.x1 ^= *state.z2;
        *state.x2 ^= *state.z1;
    });
}

void SimBulkPauliFrames::XCY(const std::vector<size_t> &targets) {
    for_xz_pairs(*this, targets, [](XZ2 &state){
        *state.x1 ^= *state.x2 ^ *state.z2;
        *state.x2 ^= *state.z1;
        *state.z2 ^= *state.z1;
    });
}

void SimBulkPauliFrames::XCZ(const std::vector<size_t> &targets) {
    for_xz_pairs(*this, targets, [](XZ2 &state){
        *state.z2 ^= *state.z1;
        *state.x1 ^= *state.x2;
    });
}

void SimBulkPauliFrames::YCX(const std::vector<size_t> &targets) {
    for_xz_pairs(*this, targets, [](XZ2 &state){
        *state.x2 ^= *state.x1 ^ *state.z1;
        *state.x1 ^= *state.z2;
        *state.z1 ^= *state.z2;
    });
}

void SimBulkPauliFrames::YCY(const std::vector<size_t> &targets) {
    for_xz_pairs(*this, targets, [](XZ2 &state){
        auto y1 = *state.x1 ^ *state.z1;
        auto y2 = *state.x2 ^ *state.z2;
        *state.x1 ^= y2;
        *state.z1 ^= y2;
        *state.x2 ^= y1;
        *state.z2 ^= y1;
    });
}

void SimBulkPauliFrames::YCZ(const std::vector<size_t> &targets) {
    for_xz_pairs(*this, targets, [](XZ2 &state){
        *state.z2 ^= *state.x1 ^ *state.z1;
        *state.z1 ^= *state.x2;
        *state.x1 ^= *state.x2;
    });
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
