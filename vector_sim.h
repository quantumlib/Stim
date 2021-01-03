#ifndef VECTOR_SIM_H
#define VECTOR_SIM_H

#include <iostream>
#include <map>
#include <complex>
#include "pauli_string.h"

struct VectorSim {
    std::vector<std::complex<float>> state;
    explicit VectorSim(size_t num_qubits);
    void apply(const std::vector<std::vector<std::complex<float>>> &matrix, const std::vector<size_t> &qubits);
    void apply(const std::string &gate, size_t qubit);
    void apply(const std::string &gate, size_t qubit1, size_t qubit2);
    void apply(const PauliString &gate, size_t qubit_offset);
};

extern const std::map<std::string, const std::vector<std::vector<std::complex<float>>>> GATE_UNITARIES;

#endif
