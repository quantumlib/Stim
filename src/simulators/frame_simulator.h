#ifndef SIM_FRAME_H
#define SIM_FRAME_H

#include <random>

#include "../circuit.h"
#include "../simd/simd_bit_table.h"
#include "../stabilizers/pauli_string.h"

/// A Pauli Frame simulator that computes many samples simultaneously.
///
/// This simulator tracks, for each qubit, whether or not that qubit is bit flipped and/or phase flipped.
/// Instead of reporting qubit measurements, it reports whether a measurement is inverted or not.
/// This requires a set of reference measurements to diff against.
struct FrameSimulator {
    size_t num_qubits;
    size_t num_samples_raw;
    size_t num_measurements_raw;
    size_t num_recorded_measurements;
    simd_bit_table x_table;
    simd_bit_table z_table;
    simd_bit_table m_table;
    simd_bits rng_buffer;
    std::mt19937_64 &rng;
    bool results_block_transposed = false;

    FrameSimulator(size_t num_qubits, size_t num_samples, size_t num_measurements, std::mt19937_64 &rng);

    static simd_bit_table sample(
            const Circuit &circuit,
            const simd_bits &reference_sample,
            size_t num_samples,
            std::mt19937_64 &rng);
    static void sample_out(
            const Circuit &circuit,
            const simd_bits &reference_sample,
            size_t num_samples,
            FILE *out,
            SampleFormat format,
            std::mt19937_64 &rng);

    PauliString get_frame(size_t sample_index) const;
    void set_frame(size_t sample_index, const PauliStringRef &new_frame);

    void unpack_sample_measurements_into(size_t sample_index, const simd_bits &reference_sample, simd_bits_range_ref out);
    void clear_and_run(const Circuit &circuit);
    void clear();
    void do_transpose();

    size_t recorded_bit_address(size_t sample_index, size_t measure_index) const;
    void unpack_write_measurements(FILE *out, const simd_bits &reference_sample, SampleFormat format);
    simd_bit_table unpack_measurements(const simd_bits &reference_sample);

    void measure(const OperationData &target_data);
    void reset(const OperationData &target_data);

    void H_XZ(const OperationData &target_data);
    void H_XY(const OperationData &target_data);
    void H_YZ(const OperationData &target_data);
    void CX(const OperationData &target_data);
    void CY(const OperationData &target_data);
    void CZ(const OperationData &target_data);
    void XCX(const OperationData &target_data);
    void XCY(const OperationData &target_data);
    void XCZ(const OperationData &target_data);
    void YCX(const OperationData &target_data);
    void YCY(const OperationData &target_data);
    void YCZ(const OperationData &target_data);
    void SWAP(const OperationData &target_data);
    void ISWAP(const OperationData &target_data);
    void do_named_op(const std::string &name, const OperationData &target_data);

    void DEPOLARIZE(const OperationData &target_data, float probability);
    void DEPOLARIZE2(const OperationData &target_data, float probability);
};

#endif
