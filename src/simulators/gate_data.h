#ifndef GATE_DATA_H
#define GATE_DATA_H

#include <complex>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct TableauSimulator;
struct FrameSimulator;
struct OperationData;
struct Tableau;

/// Maps alternate gate names to a canonical name, e.g. `CNOT` -> `ZCX`.
extern const std::unordered_map<std::string, const std::string> GATE_CANONICAL_NAMES;
/// Maps the name of a gate to the name of its inverse gate.
extern const std::unordered_map<std::string, const std::string> GATE_INVERSE_NAMES;
/// Names of operations that are part of the noise model.
extern const std::unordered_set<std::string> NOISY_GATE_NAMES;
/// Names of operations that take measurement history arguments like `DETECTOR 2@-1 2@-2`.
extern const std::unordered_set<std::string> BACKTRACK_ARG_OP_NAMES;
/// Names of operations that require a parenthesized argument, like `X_ERROR(0.0001) 1 2 3`.
extern const std::unordered_set<std::string> PARENS_ARG_OP_NAMES;
/// Names of operations that produce measurement results.
extern const std::unordered_set<std::string> MEASUREMENT_OP_NAMES;

/// Unitary matrix data by gate name..
extern const std::unordered_map<std::string, const std::vector<std::vector<std::complex<float>>>> GATE_UNITARIES;
/// Tableau data by gate name..
extern const std::unordered_map<std::string, const std::vector<const char *>> GATE_TABLEAUS;

/// Specialized gate methods by name for the tableau simulator.
extern const std::unordered_map<std::string, std::function<void(TableauSimulator &, const OperationData &)>>
    SIM_TABLEAU_GATE_FUNC_DATA;
/// Specialized gate methods by name for the frame simulator.
extern const std::unordered_map<std::string, std::function<void(FrameSimulator &, const OperationData &)>>
    SIM_BULK_PAULI_FRAMES_GATE_DATA;
/// Operations which shouldn't be merged together.
extern const std::unordered_set<std::string> UNFUSABLE_OP_NAMES;

#endif
