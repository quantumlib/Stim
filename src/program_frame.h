#ifndef PROGRAM_FRAME_H
#define PROGRAM_FRAME_H

#include "circuit.h"
#include "pauli_string.h"

struct PauliFrameProgramCollapse {
    SparsePauliString destabilizer;
};

struct PauliFrameProgramMeasurement {
    size_t target_qubit;
    bool invert;
};

struct PauliFrameProgramCycle {
    std::vector<Operation> step1_unitary;
    std::vector<PauliFrameProgramCollapse> step2_collapse;
    std::vector<PauliFrameProgramMeasurement> step3_measure;
    std::vector<size_t> step4_reset;
};

struct PauliFrameProgram {
    size_t num_qubits = 0;
    size_t num_measurements = 0;
    std::vector<PauliFrameProgramCycle> cycles;

    static PauliFrameProgram from_stabilizer_circuit(const std::vector<Operation> &operations);
    std::string str() const;

    std::vector<simd_bits> sample(size_t num_samples, std::mt19937 &rng);
    void sample_out(size_t num_samples, FILE *out, SampleFormat format, std::mt19937 &rng);
};

std::ostream &operator<<(std::ostream &out, const PauliFrameProgram &ps);

#endif
