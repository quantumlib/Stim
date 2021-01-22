#ifndef VECTOR_SIM_H
#define VECTOR_SIM_H

#include <iostream>
#include <unordered_map>
#include <complex>
#include "../stabilizers/pauli_string.h"
#include <random>

/// A state vector quantum circuit simulator.
///
/// Not intended to be particularly performant. Mostly used as a reference when testing.
struct VectorSimulator {
    std::vector<std::complex<float>> state;

    /// Creates a state vector for the given number of qubits, initialized to the zero state.
    explicit VectorSimulator(size_t num_qubits);

    /// Returns a VectorSimulator with a state vector satisfying all the given stabilizers.
    ///
    /// Assumes the stabilizers commute. Works by generating a random state vector and projecting onto
    /// each of the given stabilizers. Global phase will vary.
    static VectorSimulator from_stabilizers(const std::vector<PauliStringRef> stabilizers, std::mt19937_64 &rng);

    /// Applies a unitary operation to the given qubits, updating the state vector.
    void apply(const std::vector<std::vector<std::complex<float>>> &matrix, const std::vector<size_t> &qubits);

    /// Helper method for applying named single qubit gates.
    void apply(const std::string &gate, size_t qubit);
    /// Helper method for applying named two qubit gates.
    void apply(const std::string &gate, size_t qubit1, size_t qubit2);
    /// Helper method for applying the gates in a Pauli string.
    void apply(const PauliStringRef &gate, size_t qubit_offset);

    float project(const PauliStringRef &observable);

    bool approximate_equals(const VectorSimulator &other, bool up_to_global_phase = false) const;

    std::string str() const;
};

/// Unitary matrices for common gates, keyed by name.
extern const std::unordered_map<std::string, const std::vector<std::vector<std::complex<float>>>> GATE_UNITARIES;

std::ostream &operator<<(std::ostream &out, const VectorSimulator &sim);

#endif
