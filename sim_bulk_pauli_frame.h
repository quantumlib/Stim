#ifndef SIM_FRAME_H
#define SIM_FRAME_H

#include <random>

#include "program_frame.h"

/// A Pauli Frame simulator that computes many samples simultaneously.
///
/// This simulator tracks, for each qubit, whether or not that qubit is bit flipped and/or phase flipped.
/// Instead of reporting qubit measurements, it reports whether a measurement is inverted or not.
/// This requires some care, because it can leak unobservable information (e.g. which errors have occurred)
/// if applied to a random measurement. Correct usage requires all random measurements to be split into
/// two parts: a randomization part (e.g. `RANDOM_KICKBACK X1*X2`) and a reporting part (e.g. `M_DET !0 1`).
/// Deterministic measurements only require the reporting part.
struct SimBulkPauliFrames {
    size_t num_qubits;
    size_t num_samples_raw;
    size_t num_sample_blocks256;
    size_t num_measurements_raw;
    size_t num_measurement_blocks;
    size_t num_recorded_measurements;
    aligned_bits256 x_blocks;
    aligned_bits256 z_blocks;
    aligned_bits256 recorded_results;
    aligned_bits256 rng_buffer;
    std::mt19937 rng;
    bool results_block_transposed = false;

    SimBulkPauliFrames(size_t num_qubits, size_t num_samples, size_t num_measurements);

    PauliStringVal get_frame(size_t sample_index) const;
    void set_frame(size_t sample_index, const PauliStringPtr &new_frame);
    __m256i *x_start(size_t qubit);
    __m256i *z_start(size_t qubit);

    void unpack_sample_measurements_into(size_t sample_index, aligned_bits256 &out);
    void clear_and_run(const PauliFrameProgram &program);
    void clear();
    void do_transpose();
    void MUL_INTO_FRAME(const SparsePauliString &pauli_string, const __m256i *mask);
    void RANDOM_KICKBACK(const SparsePauliString &pauli_string);
    void measure_deterministic(const std::vector<PauliFrameProgramMeasurement> &measurements);
    void reset(const std::vector<size_t> &qubits);

    size_t recorded_bit_address(size_t sample_index, size_t measure_index) const;
    void unpack_write_measurements(FILE *out, SampleFormat format);
    std::vector<aligned_bits256> unpack_measurements();

    void H_XZ(const std::vector<size_t> &targets);
    void H_XY(const std::vector<size_t> &targets);
    void H_YZ(const std::vector<size_t> &targets);
    void CX(const std::vector<size_t> &targets2);
    void CY(const std::vector<size_t> &targets2);
    void CZ(const std::vector<size_t> &targets2);
    void XCX(const std::vector<size_t> &targets2);
    void XCY(const std::vector<size_t> &targets2);
    void XCZ(const std::vector<size_t> &targets2);
    void YCX(const std::vector<size_t> &targets2);
    void YCY(const std::vector<size_t> &targets2);
    void YCZ(const std::vector<size_t> &targets2);
    void SWAP(const std::vector<size_t> &targets2);
    void ISWAP(const std::vector<size_t> &targets2);
    void do_named_op(const std::string &name, const std::vector<size_t> &targets);

    void DEPOLARIZE(const std::vector<size_t> &targets, float probability);
    void DEPOLARIZE2(const std::vector<size_t> &targets, float probability);
};

std::ostream &operator<<(std::ostream &out, const PauliFrameProgram &ps);
extern const std::unordered_map<std::string, std::function<void(SimBulkPauliFrames &, const std::vector<size_t> &)>> SIM_BULK_PAULI_FRAMES_GATE_DATA;

#endif
