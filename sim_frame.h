#ifndef SIM_FRAME_H
#define SIM_FRAME_H

#include <random>

#include "circuit.h"
#include "pauli_string.h"

struct PauliFrameSimCollapse {
    SparsePauliString destabilizer;
};

struct PauliFrameSimMeasurement {
    size_t target_qubit;
    bool invert;
};

struct PauliFrameProgramCycle {
    std::vector<Operation> step1_unitary;
    std::vector<PauliFrameSimCollapse> step2_collapse;
    std::vector<PauliFrameSimMeasurement> step3_measure;
    std::vector<size_t> step4_reset;
};

struct PauliFrameProgram {
    size_t num_qubits = 0;
    size_t num_measurements = 0;
    std::vector<PauliFrameProgramCycle> cycles;

    static PauliFrameProgram recorded_from_tableau_sim(const std::vector<Operation> &operations);
    std::string str() const;
};

/// A Pauli Frame simulator that computes many samples simultaneously.
///
/// This simulator tracks, for each qubit, whether or not that qubit is bit flipped and/or phase flipped.
/// Instead of reporting qubit measurements, it reports whether a measurement is inverted or not.
/// This requires some care, because it can leak unobservable information (e.g. which errors have occurred)
/// if applied to a random measurement. Correct usage requires all random measurements to be split into
/// two parts: a randomization part (e.g. `RANDOM_INTO_FRAME X1*X2`) and a reporting part (e.g. `REPORT !0 1`).
/// Deterministic measurements only require the reporting part.
struct SimBulkPauliFrames {
    size_t num_qubits;
    size_t num_samples256;
    size_t num_measurements;
    size_t recorded_measurements;
    aligned_bits256 x_blocks;
    aligned_bits256 z_blocks;
    aligned_bits256 recorded_results;
    aligned_bits256 rng_buffer;
    std::mt19937 rng;

    SimBulkPauliFrames(size_t num_qubits, size_t num_samples, size_t num_measurements);

    PauliStringVal current_frame(size_t sample_index) const;
    __m256i *x_start(size_t qubit);
    __m256i *z_start(size_t qubit);

    void clear();
    void H_XZ(const std::vector<size_t> &qubits);
    void CX(const std::vector<size_t> &qubits);
    void MUL_INTO_FRAME(const SparsePauliString &pauli_string, const __m256i *mask);
    void RANDOM_INTO_FRAME(const SparsePauliString &pauli_string);
    void measure_deterministic(const std::vector<PauliFrameSimMeasurement> &measurements);
    void reset(const std::vector<size_t> &qubits);
};

std::ostream &operator<<(std::ostream &out, const PauliFrameProgram &ps);

#endif
